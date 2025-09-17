// =============================================================================
// Copyright 2024 Simeon Manolov <s.manolloff@gmail.com>.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#include "Global.h"
#include "StdInc.h"

#include "TorchModel.h"
#include "schema/v13/types.h"


#include <executorch/extension/tensor/tensor.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <chrono>

namespace MMAI::BAI {

    using executorch::aten::ScalarType;
    using executorch::extension::TensorPtr;

    struct ScopedTimer {
        const char* name;
        std::chrono::steady_clock::time_point t0;
        explicit ScopedTimer(const char* n) : name(n), t0(std::chrono::steady_clock::now()) {}
        ~ScopedTimer() {
            using namespace std::chrono;
            auto dt = duration_cast<milliseconds>(steady_clock::now() - t0).count();
            std::fprintf(stderr, "%s: %lld ms\n", name, dt);
        }
    };

    static inline const char* dtype_name(ScalarType dt) {
        switch (dt) {
            case ScalarType::Float:  return "float32";
            case ScalarType::Double: return "float64";
            case ScalarType::Long:   return "int64";
            case ScalarType::Int:    return "int32";
            case ScalarType::Short:  return "int16";
            case ScalarType::Byte:   return "uint8";
            case ScalarType::Char:   return "int8";
            default:                 return "unknown";
        }
    }

    template <typename T>
    static void print_like_torch_impl(const TensorPtr& t, int max_per_dim, int float_prec) {
        const auto& sizes = t->sizes();                 // std::vector<int64_t>
        const int D = static_cast<int>(sizes.size());
        std::vector<int64_t> strides(D, 1);
        for (int i = D - 2; i >= 0; --i) strides[i] = strides[i + 1] * sizes[i + 1];

        const T* data = t->data_ptr<T>();

        auto print_dim = [&](auto&& self, int dim, int64_t offset, int indent) -> void {
            std::cout << "[";
            if (dim == D - 1) {
                int64_t n = sizes[dim];
                int64_t show = std::min<int64_t>(n, max_per_dim);
                for (int64_t i = 0; i < show; ++i) {
                    if (i) std::cout << ", ";
                    if constexpr (std::is_floating_point<T>::value) {
                        std::cout << std::fixed << std::setprecision(float_prec) << data[offset + i];
                    } else {
                        std::cout << data[offset + i];
                    }
                }
                if (show < n) std::cout << ", ...";
                std::cout << "]";
                return;
            }
            int64_t n = sizes[dim];
            int64_t show = std::min<int64_t>(n, max_per_dim);
            for (int64_t i = 0; i < show; ++i) {
                if (i) std::cout << ",\n" << std::string(indent + 2, ' ');
                self(self, dim + 1, offset + i * strides[dim], indent + 2);
            }
            if (show < n) std::cout << ",\n" << std::string(indent + 2, ' ') << "...";
            std::cout << "]";
        };

        std::cout << "tensor(";
        if (D == 0) {
            if constexpr (std::is_floating_point<T>::value)
                std::cout << std::fixed << std::setprecision(float_prec) << *data;
            else
                std::cout << *data;
        } else {
            print_dim(print_dim, 0, 0, 0);
        }
        std::cout << ", dtype=" << dtype_name(t->scalar_type()) << ", shape=[";
        for (int i = 0; i < D; ++i) { if (i) std::cout << ","; std::cout << sizes[i]; }
        std::cout << "])\n";
    }

    inline void print_tensor_like_torch(const TensorPtr& t,
                                        int max_per_dim = 8,
                                        int float_precision = 4) {
        switch (t->scalar_type()) {
            case ScalarType::Float:  print_like_torch_impl<float>(t,  max_per_dim, float_precision); break;
            case ScalarType::Double: print_like_torch_impl<double>(t, max_per_dim, float_precision); break;
            case ScalarType::Long:   print_like_torch_impl<int64_t>(t, max_per_dim, 0); break;
            case ScalarType::Int:    print_like_torch_impl<int32_t>(t, max_per_dim, 0); break;
            case ScalarType::Short:  print_like_torch_impl<int16_t>(t, max_per_dim, 0); break;
            case ScalarType::Byte:   print_like_torch_impl<uint8_t>(t, max_per_dim, 0); break;
            case ScalarType::Char:   print_like_torch_impl<int8_t>(t, max_per_dim, 0); break;
            default: throw std::runtime_error("print_tensor_like_torch: unsupported dtype");
        }
    }

    // XXX: for improved performance a per-linktype kmax implementation will be needed
    // (e.g. for ADJACENT kmax=6).

    constexpr int LT_COUNT = EI(MMAI::Schema::V13::LinkType::_count);

    std::vector<int64_t> buildNBR(const std::vector<int64_t> &dst, int k_max) {
        auto nbr = std::vector<int64_t>{};  // zero-initialized
        nbr.reserve(165 * k_max);
        nbr.insert(nbr.begin(), nbr.capacity(), -1);

        // per-node fill position
        auto fill = std::array<int64_t, 165>{};

        for (std::size_t e = 0; e < dst.size(); ++e) {
            auto v = dst.at(e);

            if (v < 0 || v >= 165)
                THROW_FORMAT("dst contains node id out of range: ", v);

            auto p = fill.at(v);

            // TODO: ignore instead of throw
            // (requires tracking of indexes to drop from edge_src and edge_attr)
            if (p >= k_max)
                THROW_FORMAT("node %d exceeds K_max=%d", v % k_max);

            nbr.at(v*k_max + p) = e;
            fill.at(v) = p + 1;
        }

        return nbr;
    }

    executorch::extension::TensorPtr make_edge_index(const std::vector<int64_t>& src_index, const std::vector<int64_t>& dst_index) {
        const int64_t E = static_cast<int64_t>(src_index.size());

        if (dst_index.size() != static_cast<size_t>(E)) {
            throw std::runtime_error("src_index and dst_index must have the same length");
        }

        // Row-major layout: first row = src[0:E], second row = dst[0:E]
        std::vector<int64_t> data;
        data.reserve(2 * static_cast<size_t>(E));
        data.insert(data.end(), src_index.begin(), src_index.end());
        data.insert(data.end(), dst_index.begin(), dst_index.end());

        return executorch::extension::from_blob(data.data(), {2, static_cast<int>(E)});
    }

    std::vector<extension::TensorPtr> TorchModel::prepareInputsV13(
        const MMAI::Schema::IState * s,
        const MMAI::Schema::V13::ISupplementaryData* sup
    ) {
        // XXX: if needed, support for other versions may be added via conditionals
        if (version != 13)
            THROW_FORMAT("TorchModel: unsupported version: want: 13, have: %d", version);

        auto einds = std::vector<int64_t> {};
        auto eattrs = std::vector<float> {};
        auto enbrs = std::vector<int64_t> {};

        einds.reserve(LT_COUNT * 2 * e_max);   // [7, 2, 3300]
        eattrs.reserve(LT_COUNT * e_max);      // [7, 3300, 1]
        enbrs.reserve(LT_COUNT * 165 * k_max); // [7, 165, 20]

        int count = 0;

        for (const auto &[type, links] : sup->getAllLinks()) {
            // assert order
            if (EI(type) != count)
                THROW_FORMAT("TorchModel: unexpected link type: want: %d, have: %d", count % EI(type));

            const auto srcinds = links->getSrcIndex();
            const auto dstinds = links->getDstIndex();
            const auto attrs = links->getAttributes();
            const auto nbr = buildNBR(dstinds, k_max);

            count += 1;

            if (srcinds.size() > e_max)
                THROW_FORMAT("TorchModel: srcinds for LinkType(%d) exceeds e_max (%d): %d", EI(type) % e_max % srcinds.size());
            if (dstinds.size() > e_max)
                THROW_FORMAT("TorchModel: dstinds for LinkType(%d) exceeds e_max (%d): %d", EI(type) % e_max % dstinds.size());
            if (attrs.size() > e_max)
                THROW_FORMAT("TorchModel: attrs for LinkType(%d) exceeds e_max (%d): %d", EI(type) % e_max % attrs.size());

            einds.insert(einds.end(), srcinds.begin(), srcinds.end());
            einds.insert(einds.end(), e_max - srcinds.size(), 0);
            einds.insert(einds.end(), dstinds.begin(), dstinds.end());
            einds.insert(einds.end(), e_max - dstinds.size(), 0);

            eattrs.insert(eattrs.end(), attrs.begin(), attrs.end());
            eattrs.insert(eattrs.end(), e_max - attrs.size(), 0);

            enbrs.insert(enbrs.end(), nbr.begin(), nbr.end());
        }

        if (count != LT_COUNT)
            THROW_FORMAT("TorchModel: unexpected links count: want: %d, have: %d", LT_COUNT % count);

        auto state = s->getBattlefieldState();
        auto estate = std::vector<float>(state->size());
        std::copy(state->begin(), state->end(), estate.begin());

        auto t_state = executorch::extension::from_blob(estate.data(), {int(estate.size())}, aten::ScalarType::Float);
        auto t_einds = executorch::extension::from_blob(einds.data(), {LT_COUNT, 2, e_max}, aten::ScalarType::Long);
        auto t_eattrs = executorch::extension::from_blob(eattrs.data(), {LT_COUNT, e_max, 1}, aten::ScalarType::Float);
        auto t_enbrs = executorch::extension::from_blob(enbrs.data(), {LT_COUNT, 165, k_max}, aten::ScalarType::Long);

        // Must clone to copy the underlying data (originally owned by the std::vectors)
        return std::vector<extension::TensorPtr> {
            extension::clone_tensor_ptr(t_state),
            extension::clone_tensor_ptr(t_einds),
            extension::clone_tensor_ptr(t_eattrs),
            extension::clone_tensor_ptr(t_enbrs)
        };
    }

    aten::Tensor TorchModel::call(
        const std::string& method_name,
        const std::vector<runtime::EValue>& input,
        int numel,
        aten::ScalarType st
    ) {
        auto timer = ScopedTimer(method_name.c_str());
        auto res = mc->model.execute(method_name, input);

        std::string common = "TorchModel: " + method_name;
        std::string want = "";
        std::string have = "";

        if (!res.ok()) {
            THROW_FORMAT("TorchModel: %s: error code %d", method_name % EI(res.error()));
        }

        auto out = res->at(0);

        if (!out.isTensor()) {
            THROW_FORMAT("TorchModel: %s: not a tensor", method_name % EI(res.error()));
        }

        auto t = out.toTensor();  // Most exports return scalars as 0-D tensors with dtype int64

        if (t.numel() != numel) {
            THROW_FORMAT("TorchModel: %s: bad numel: want: %d, have: %d", method_name % numel % EI(t.numel()));
        }

        if (t.dtype() != st) {
            THROW_FORMAT("TorchModel: %s: bad dtype: want: %d, have: %d", method_name % EI(st) % EI(t.dtype()));
        }

        return t;
    }

    TorchModel::TorchModel(std::string path)
    : path(path)
    , mc(std::make_unique<ModelContainer>(path))
    {
        auto t_version = call("get_version", 1, aten::ScalarType::Long);
        version = static_cast<int>(t_version.const_data_ptr<int64_t>()[0]);

        auto t_e_max = call("get_e_max", 1, aten::ScalarType::Long);
        e_max = static_cast<int>(t_e_max.const_data_ptr<int64_t>()[0]);

        auto t_k_max = call("get_k_max", 1, aten::ScalarType::Long);
        k_max = static_cast<int>(t_k_max.const_data_ptr<int64_t>()[0]);

        auto t_side = call("get_side", 1, aten::ScalarType::Long);
        side = Schema::Side(static_cast<int>(t_side.const_data_ptr<int64_t>()[0]));
    }


    Schema::ModelType TorchModel::getType() {
        return Schema::ModelType::TORCH;
    };

    std::string TorchModel::getName() {
        return "MMAI_MODEL";
    };

    int TorchModel::getVersion() {
        return version;
    };

    Schema::Side TorchModel::getSide() {
        return side;
    };

    int TorchModel::getAction(const MMAI::Schema::IState * s) {
        auto any = s->getSupplementaryData();

        if (version != 13)
            THROW_FORMAT("TorchModel: unsupported model version: want: 13, have: %d", version);

        if (s->version() != 13)
            THROW_FORMAT("TorchModel: unsupported IState version: want: 13, have: %d", s->version());

        if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
        auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
        if(!err.empty())
            THROW_FORMAT("TorchModel: anycast failed: %s", err);

        auto sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

        if (sup->getIsBattleEnded())
            return MMAI::Schema::ACTION_RESET;

        auto inputs = prepareInputsV13(s, sup);
        auto values = std::vector<runtime::EValue>{};
        for (auto &t : inputs)
            values.push_back(t);

        // printf("--------------- 0:\n");
        // print_tensor_like_torch(inputs.at(0), 10, 6); // show up to 4 per dim, 6 decimals
        // printf("--------------- 1:\n");
        // print_tensor_like_torch(inputs.at(1), 10, 6); // show up to 4 per dim, 6 decimals
        // printf("--------------- 2:\n");
        // print_tensor_like_torch(inputs.at(2), 10, 6); // show up to 4 per dim, 6 decimals
        // printf("--------------- 3:\n");
        // print_tensor_like_torch(inputs.at(3), 10, 6); // show up to 4 per dim, 6 decimals

        auto output = call("predict", values, 1, aten::ScalarType::Long);

        int action = output.const_data_ptr<int64_t>()[0];
        std::cout << "Predicted action: " << action << "\n";

        // throw std::runtime_error("forced exit");
        return MMAI::Schema::Action(action);
    };

    double TorchModel::getValue(const MMAI::Schema::IState * s) {
        auto any = s->getSupplementaryData();

        if (version != 13)
            THROW_FORMAT("TorchModel: unsupported model version: want: 13, have: %d", version);

        if (s->version() != 13)
            THROW_FORMAT("TorchModel: unsupported IState version: want: 13, have: %d", s->version());

        if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
        auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
        if(!err.empty())
            THROW_FORMAT("TorchModel: anycast failed: %s", err);

        auto sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

        if (sup->getIsBattleEnded())
            return 0.0;

        auto inputs = prepareInputsV13(s, sup);
        auto values = std::vector<runtime::EValue>{};
        for (auto &t : inputs)
            values.push_back(t);

        auto output = call("predict", values, 1, aten::ScalarType::Float);
        auto value = output.const_data_ptr<float>()[0];
        std::cout << "Predicted value: " << value << "\n";

        return value;
    }
}
