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

#include "StdInc.h"

#include "TorchModel.h"
#include "executorch/extension/tensor/tensor_ptr.h"
#include "executorch/runtime/core/exec_aten/exec_aten.h"
#include "schema/schema.h"

namespace MMAI::BAI {
    // XXX: for improved performance a per-linktype kmax implementation will be needed
    // (e.g. for ADJACENT kmax=6).

    constexpr int K = 20;
    constexpr int LT_COUNT = EI(MMAI::Schema::V13::LinkType::_count);
    using NBR = std::array<std::array<int64_t, K>, 165>;

    NBR buildNBR(const std::vector<int64_t> &dst) {
        auto nbr = NBR{};
        for (auto& row : nbr) row.fill(-1);

        // per-node fill position
        auto fill = std::array<int64_t, 165>{};

        for (std::size_t e = 0; e < dst.size(); ++e) {
            int64_t v = dst[e];
            if (v < 0 || v >= static_cast<int64_t>(165)) {
                throw std::out_of_range("dst contains node id out of range: " + std::to_string(v));
            }
            int64_t p = fill[static_cast<std::size_t>(v)];

            // TODO: ignore instead of throw
            // (requires tracking of indexes to drop from edge_src and edge_attr)
            if (p >= static_cast<int64_t>(K)) {
                throw std::runtime_error(
                    "node " + std::to_string(v) + " exceeds K_max=" + std::to_string(K));
            }
            nbr[static_cast<std::size_t>(v)][static_cast<std::size_t>(p)] = static_cast<int64_t>(e);
            fill[static_cast<std::size_t>(v)] = p + 1;
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

    std::vector<runtime::EValue> TorchModel::prepareInputs(const MMAI::Schema::IState * s) {
        // XXX: if needed, support for other versions may be added via conditionals
        if (version != 13) {
            THROW_FORMAT("TorchModel: unsupported version: want: 13, have: %d", version);
        }

        auto &any = s->getSupplementaryData();
        if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
        auto &t = typeid(const MMAI::Schema::V13::ISupplementaryData*);
        auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));

        if(!err.empty()) {
            THROW_FORMAT("TorchModel: anycast failed: %s", err);
        }

        auto sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(s->getSupplementaryData());
        auto nbrs = std::array<NBR, EI(MMAI::Schema::V13::LinkType::_count)>{};
        auto srcs = std::array<std::vector<int64_t>, LT_COUNT>{};
        auto dsts = std::array<std::vector<int64_t>, LT_COUNT>{};
        auto attrs = std::array<std::vector<float>, LT_COUNT>{};

        for (auto &nbr : nbrs) {
            for (auto &hex : nbr) {
                hex.fill(-1);
            }
        }

        for (const auto &[type, links] : sup->getAllLinks()) {
            const auto srcinds = links->getSrcIndex();
            const auto dstinds = links->getDstIndex();
            const auto attr = links->getAttributes();

            srcs.at(EI(type)) = srcinds;
            dsts.at(EI(type)) = dstinds;
            attrs.at(EI(type)) = attr;
            nbrs.at(EI(type)) = buildNBR(dstinds);
        }

        // if auto, then type is executorch::runtime::etensor::Tensor
        // ...but it seems interchangeable with runtime::EValue ???

        // auto input = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});
        // runtime::EValue ev = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});

        // std::vector<runtime::EValue>{ev}
        auto inputs = std::vector<runtime::EValue> {};
        auto srcState = s->getBattlefieldState();
        auto dstState = std::vector<float>();
        dstState.resize(srcState->size());
        std::copy(srcState->begin(), srcState->end(), dstState.begin());

        // Signature:
        // (x, edge_ind1, edge_attr1, edge_nbr1, edge_ind2, edge_attr2, edge_nbr2, ...)

        // from_blob creates a non-owning tensor (no copy)
        // make_tensor_ptr creates an owning tensor (copies data)
        // auto x = executorch::extension::from_blob(dstState.data(), {static_cast<int>(dstState.size())});
        inputs.push_back(executorch::extension::make_tensor_ptr({static_cast<int>(dstState.size())}, std::move(dstState), {}, {}, aten::ScalarType::Float));

        for (int i=0; i<LT_COUNT; ++i) {
            // Convert src & dst to a single edgeIndex of shape {2, E}
            int E = srcs.at(i).size();
            int E1 = dsts.at(i).size();
            if (E != E1) THROW_FORMAT("TorchModel: src/dst index mismatch for LT=%d: %d != %d", i % E % E1);
            std::vector<int64_t> ind;
            ind.reserve(2 * static_cast<size_t>(E));
            ind.insert(ind.end(), srcs.at(i).begin(), srcs.at(i).end());
            ind.insert(ind.end(), dsts.at(i).begin(), dsts.at(i).end());

            // Convert nbr to a tensor of shape {165, K}
            std::vector<int64_t> nbr;
            nbr.reserve(165 * K);
            for (const auto &row : nbrs.at(i))
                nbr.insert(nbr.end(), row.begin(), row.end());
            if(nbr.size() == 165*K) THROW_FORMAT("TorchModel: nbr size mismatch: want: %d, have: %d", (165*K) % EI(nbr.size()));

            inputs.push_back(executorch::extension::make_tensor_ptr({2, E}, std::move(ind), {}, {}, aten::ScalarType::Long));
            inputs.push_back(executorch::extension::make_tensor_ptr({E}, std::move(attrs.at(i)), {}, {}, aten::ScalarType::Float));
            inputs.push_back(executorch::extension::make_tensor_ptr({165, K}, std::move(nbr), {}, {}, aten::ScalarType::Long));
        }
    }

    aten::Tensor TorchModel::call(
        const std::string& method_name,
        const std::vector<runtime::EValue>& input,
        int numel,
        aten::ScalarType st
    ) {
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
        auto t = call("get_version", 1, aten::ScalarType::Long);
        version = static_cast<int>(t.const_data_ptr<int64_t>()[0]);
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

    int TorchModel::getAction(const MMAI::Schema::IState * s) {
        auto any = s->getSupplementaryData();

        if (std::any_cast<const MMAI::Schema::V12::ISupplementaryData*>(any)->getIsBattleEnded())
            return MMAI::Schema::ACTION_RESET;

        // auto src = s->getBattlefieldState();
        // auto dst = std::vector<float>();
        // dst.resize(src->size());
        // std::copy(src->begin(), src->end(), dst.begin());
        // auto input = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});
        auto inputs = prepareInputs(s);
        auto output = call("predict", inputs, 1, aten::ScalarType::Long);

        int action = output.const_data_ptr<int64_t>()[0];
        std::cout << "Predicted action: " << action << "\n";
        return MMAI::Schema::Action(action);
    };

    double TorchModel::getValue(const MMAI::Schema::IState * s) {
        auto any = s->getSupplementaryData();

        if (std::any_cast<const MMAI::Schema::V12::ISupplementaryData*>(any)->getIsBattleEnded())
            return 0.0;

        // auto src = s->getBattlefieldState();
        // auto dst = std::vector<float>();
        // dst.resize(src->size());
        // std::copy(src->begin(), src->end(), dst.begin());
        // auto input = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});
        auto inputs = prepareInputs(s);
        auto output = call("get_value", inputs, 1, aten::ScalarType::Float);

        auto value = output.const_data_ptr<float>()[0];
        std::cout << "Predicted value: " << value << "\n";
        return value;
    }
}
