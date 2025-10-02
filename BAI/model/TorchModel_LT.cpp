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

#include <ATen/core/TensorBody.h>
#include <ATen/core/ivalue.h>
#include <ATen/ops/from_blob.h>
#include <c10/core/ScalarType.h>
#include <mutex>

#include "TorchModel_LT.h"
#include "schema/schema.h"

namespace MMAI::BAI {

constexpr int LT_COUNT = EI(MMAI::Schema::V13::LinkType::_count);

namespace {
    template<class... Args>
    [[noreturn]] inline void throwf(const std::string& fmt, Args&&... args) {
        boost::format f("TorchModel_LT: " + fmt);
        (void)std::initializer_list<int>{ ( (f % std::forward<Args>(args)), 0 )... };
        throw std::runtime_error(f.str());
    }

    struct ScopedTimer {
        const char* name;
        std::chrono::steady_clock::time_point t0;
        explicit ScopedTimer(const char* n) : name(n), t0(std::chrono::steady_clock::now()) {}
        ~ScopedTimer() {
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
            logAi->warn("%s: %lld ms", name, dt);
        }
    };

    std::array<std::vector<int64_t>, 165> buildNBR_unpadded(const std::vector<int64_t>& dst) {
        // Pass 1: validate and count degrees per node
        std::array<int, 165> deg{};
        for (size_t e = 0; e < dst.size(); ++e) {
            int v = static_cast<int>(dst[e]);
            if (v < 0 || v >= 165)
                throwf("dst contains node id out of range: ", v);
            ++deg[v];
        }

        std::array<std::vector<int64_t>, 165> nbr{};
        for (int v = 0; v < 165; ++v) nbr[v].reserve(deg[v]);
        for (size_t e = 0; e < dst.size(); ++e) {
            int v = static_cast<int>(dst[e]);
            nbr[v].push_back(static_cast<int64_t>(e));
        }

        return nbr;
    }

    struct IndexContainer {
        std::array<std::vector<int64_t>, 2> ei;
        std::vector<float> ea;
        std::array<std::vector<int64_t>, 165> nbrs;
    };

    struct BuildOutputs {
        int size_index = -1;                                // chosen index in all_sizes
        std::array<int64_t, LT_COUNT> emax{};               // chosen emax per link type
        std::array<int64_t, LT_COUNT> kmax{};               // chosen kmax per link type

        std::array<std::vector<int64_t>, 2> ei_flat;        // each length sum(emax)
        std::vector<float> ea_flat;                         // length sum(emax)
        std::array<std::vector<int64_t>, 165> nbrs_flat;    // each length sum(kmax)
    };

    // all_sizes: S x LT_COUNT x 2, where [s][l] = {emax, kmax}
    BuildOutputs build_flattened(
        const std::array<IndexContainer, LT_COUNT>& containers,
        const std::vector<std::vector<std::vector<int64_t>>>& all_sizes,
        int bucket
    ) {
        BuildOutputs out{};

        // Required per-linktype capacities from data
        std::array<size_t, LT_COUNT> e_req{};
        std::array<size_t, LT_COUNT> k_req{};
        for (int l = 0; l < LT_COUNT; ++l) {
            e_req[l] = containers[l].ea.size();
            size_t km = 0;
            for (int v = 0; v < 165; ++v)
                km = std::max(km, containers[l].nbrs[v].size());
            k_req[l] = km;
        }

        // 1) Find smallest valid size index
        int chosen = -1;
        std::array<int64_t, LT_COUNT> emax{}, kmax{};
        for (int s = 0; s < static_cast<int>(all_sizes.size()); ++s) {
            const auto& sz = all_sizes[s];
            if (sz.size() != LT_COUNT) continue;  // skip malformed
            bool ok = true;
            for (int l = 0; l < LT_COUNT && ok; ++l) {
                if (sz[l].size() != 2) { ok = false; break; }
                int64_t emax_l = sz[l][0];
                int64_t kmax_l = sz[l][1];
                if (emax_l < static_cast<int64_t>(e_req[l]) ||
                    kmax_l < static_cast<int64_t>(k_req[l])) {
                    ok = false;
                }
            }
            ok = ok && (bucket == -1 || s == bucket);
            if (ok) {
                chosen = s;
                for (int l = 0; l < LT_COUNT; ++l) {
                    emax[l] = sz[l][0];
                    kmax[l] = sz[l][1];
                }
                break;
            }
        }

        // TODO: emit a warning and truncate edges instead (not straightforward)
        if (chosen < 0) {
            throw std::runtime_error("No size option in all_sizes satisfies the data requirements.");
        }

        logAi->debug("Size: %d", chosen);
        for (int i=0; i<LT_COUNT; ++i)
            logAi->debug("  %d: [%ld, %ld] -> [%lld, %lld]", i, e_req[i], k_req[i], all_sizes[chosen][i][0], all_sizes[chosen][i][1]);


        out.size_index = chosen;
        out.emax = emax;
        out.kmax = kmax;

        // Precompute sums
        const size_t sum_emax = std::accumulate(emax.begin(), emax.end(), 0);
        const size_t sum_kmax = std::accumulate(kmax.begin(), kmax.end(), 0);

        // 2) Build ei_flat and ea_flat (concat each layer's ei/ea, zero-padded to emax[l])
        out.ei_flat.at(0).clear();
        out.ei_flat.at(1).clear();
        out.ea_flat.clear();

        out.ei_flat.at(0).reserve(sum_emax);
        out.ei_flat.at(1).reserve(sum_emax);
        out.ea_flat.reserve(sum_emax);
        for (int l = 0; l < LT_COUNT; ++l) {
            const auto& ei = containers[l].ei;
            const auto& ea = containers[l].ea;

            out.ei_flat.at(0).insert(out.ei_flat.at(0).end(), ei.at(0).begin(), ei.at(0).end());
            out.ei_flat.at(1).insert(out.ei_flat.at(1).end(), ei.at(1).begin(), ei.at(1).end());
            out.ea_flat.insert(out.ea_flat.end(), ea.begin(), ea.end());

            size_t need = static_cast<size_t>(emax[l]) - ei.at(0).size();
            if (need > 0) out.ei_flat.at(0).insert(out.ei_flat.at(0).end(), need, 0);

            need = static_cast<size_t>(emax[l]) - ei.at(1).size();
            if (need > 0) out.ei_flat.at(1).insert(out.ei_flat.at(1).end(), need, 0);

            need = static_cast<size_t>(emax[l]) - ea.size();
            if (need > 0) out.ea_flat.insert(out.ea_flat.end(), need, 0.0f);
        }
        // Sanity
        if (out.ei_flat.at(0).size() != sum_emax)
            throwf("ei_flat.at(0) size mismatch: want: %d, have: %zu", sum_emax, out.ei_flat.at(0).size());
        if (out.ei_flat.at(1).size() != sum_emax)
            throwf("ei_flat.at(1) size mismatch: want: %d, have: %zu", sum_emax, out.ei_flat.at(1).size());
        if (out.ea_flat.size() != sum_emax)
            throwf("ea_flat size mismatch: want: %d, have: %zu", sum_emax, out.ea_flat.size());

        // 3) Build nbrs_flat per node: concat layer l, pad to kmax[l] with -1
        for (int v = 0; v < 165; ++v) {
            auto& dst = out.nbrs_flat[v];
            dst.clear();
            dst.reserve(sum_kmax);
            for (int l = 0; l < LT_COUNT; ++l) {
                const auto& src = containers[l].nbrs[v];
                dst.insert(dst.end(), src.begin(), src.end());
                const size_t need = static_cast<size_t>(kmax[l]) - src.size();
                if (need > 0) dst.insert(dst.end(), need, static_cast<int64_t>(-1));
            }
            // Optional sanity:
            if (dst.size() != sum_kmax) {
                throw std::runtime_error("nbrs_flat row size mismatch.");
            }
        }

        return out;
    }
}

template <typename T>
T TorchModel::getScalar(const std::string &method_name) {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, int>, "T must be float or int");
    at::ScalarType st;

    if constexpr (std::is_same_v<T, float>) {
        st = at::kFloat;
    } else { // T == int
        st = at::kLong;
    }

    auto t = call(method_name, 1, st);

    if (t.dim() != 1)
        throwf("getScalar: %s: bad dim(): want: 1, have: %ld", method_name, t.dim());

    if constexpr (std::is_same_v<T, float>) {
        if (t.scalar_type() != at::kFloat)
            throwf("getScalar: %s: bad dtype: want: %d, have: %d", method_name, EI(at::kFloat), EI(t.scalar_type()));
        return t.item<float>();
    } else { // T == int
        if (t.scalar_type() != at::kLong)
            throwf("getScalar: %s: bad dtype: want: %d, have: %d", method_name, EI(at::kLong), EI(t.scalar_type()));

        int64_t v64 = t.item<int64_t>();
        if (v64 < std::numeric_limits<int>::min() || v64 > std::numeric_limits<int>::max())
            throwf("getScalar: %s: out of range for int: %ld", method_name, v64);
        return static_cast<int>(v64);
    }
}

std::pair<std::vector<at::Tensor>, int> TorchModel::prepareInputsV13(
    const MMAI::Schema::IState * s,
    const MMAI::Schema::V13::ISupplementaryData* sup,
    int bucket
) {
    // XXX: if needed, support for other versions may be added via conditionals
    if (version != 13)
        throwf("unsupported version: want: 13, have: %d", version);

    auto containers = std::array<IndexContainer, LT_COUNT> {};

    int count = 0;

    for (const auto &[type, links] : sup->getAllLinks()) {
        // assert order
        if (EI(type) != count)
            throwf("unexpected link type: want: %d, have: %d", count, EI(type));

        auto &c = containers.at(count);

        const auto srcinds = links->getSrcIndex();
        const auto dstinds = links->getDstIndex();
        const auto attrs = links->getAttributes();

        auto nlinks = srcinds.size();

        if (dstinds.size() != nlinks)
            throwf("unexpected dstinds.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, dstinds.size());

        if (attrs.size() != nlinks)
            throwf("unexpected attrs.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, attrs.size());

        // c.e_max = nlinks;
        // c.k_max = k_max;

        c.ei.at(0).reserve(nlinks);
        c.ei.at(1).reserve(nlinks);
        c.ei.at(0).insert(c.ei.at(0).end(), srcinds.begin(), srcinds.end());
        c.ei.at(1).insert(c.ei.at(1).end(), dstinds.begin(), dstinds.end());

        c.ea.reserve(nlinks);
        c.ea.insert(c.ea.end(), attrs.begin(), attrs.end());

        c.nbrs = buildNBR_unpadded(dstinds);

        ++count;
    }

    if (count != LT_COUNT)
        throwf("unexpected links count: want: %d, have: %d", LT_COUNT, count);

    auto build = build_flattened(containers, all_buckets, bucket);

    const auto *state = s->getBattlefieldState();
    auto estate = std::vector<float>(state->size());
    std::copy(state->begin(), state->end(), estate.begin());

    int sum_e = build.ei_flat.at(0).size();
    int sum_k = build.nbrs_flat.at(0).size();

    if (build.ei_flat.at(0).size() != sum_e)
        throwf("unexpected build.ei_flat.at(0).size(): want: %d, have: %d", sum_e, build.ei_flat.at(0).size());
    if (build.ei_flat.at(1).size() != sum_e)
        throwf("unexpected build.ei_flat.at(1).size(): want: %d, have: %d", sum_e, build.ei_flat.at(1).size());
    if (build.ea_flat.size() != sum_e)
        throwf("unexpected build.ea_flat.size(): want: %d, have: %d", sum_e, build.ea_flat.size());
    for (int i=0; i<165; ++i) {
        if (build.nbrs_flat.at(i).size() != sum_k) {
            throwf("unexpected build.nbrs_flat.at(%d).size(): want: %d, have: %d", i, sum_k, build.nbrs_flat.at(i).size());
        }
    }

    auto einds = std::vector<int64_t> {};
    einds.reserve(2*sum_e);
    for (auto &eind : build.ei_flat)
        einds.insert(einds.end(), eind.begin(), eind.end());

    auto nbrs = std::vector<int64_t> {};
    nbrs.reserve(165*sum_k);
    for (auto &nbr : build.nbrs_flat)
        nbrs.insert(nbrs.end(), nbr.begin(), nbr.end());

    // auto t_state = et_ext::from_blob(estate.data(), {int(estate.size())}, ScalarType::Float);
    auto t_state = at::from_blob(estate.data(), {int(estate.size())}, at::kFloat);
    auto t_ei_flat = at::from_blob(einds.data(), {2, sum_e}, at::kLong);
    auto t_ea_flat = at::from_blob(build.ea_flat.data(), {sum_e, 1}, at::kFloat);
    auto t_nbrs_flat = at::from_blob(nbrs.data(), {165, sum_k}, at::kLong);

    auto tensors = std::vector<at::Tensor> {
        t_state.clone(),
        t_ei_flat.clone(),
        t_ea_flat.clone(),
        t_nbrs_flat.clone()
    };

    return {tensors, build.size_index};
}


at::Tensor TorchModel::call(
    const std::string& method_name,
    const std::vector<c10::IValue>& input,
    int resNumel,
    at::ScalarType st
) {
    std::unique_lock lock(m);
    auto raw = model->get_method(method_name)(input);

    if (!raw.isTensor())
        throwf("call: %s: not a tensor", method_name);

    at::Tensor t = raw.toTensor().contiguous();

    if (t.dim() != 1)
        throwf("call: %s: bad dim(): want: 1, have: %ld", method_name, t.dim());

    if (t.scalar_type() != st)
        throwf("call: %s: bad dtype: want: %d, have: %d", method_name, EI(st), EI(t.scalar_type()));

    if (resNumel && t.numel() != 1)
        throwf("call: %s: bad numel: want: %d, have: %ld", method_name, resNumel, t.numel());

    return t;
}

TorchModel::TorchModel(std::string &path)
: path(path)
, model(std::make_unique<tj::mobile::Module>(tj::_load_for_mobile(path)))
{
    version = getScalar<int>("get_side");
    side = Schema::Side(getScalar<int>("get_side"));

    if (version != 13)
        throwf("unsupported model version: want: 13, have: %d", version);

    auto t_buckets = call("get_all_sizes", 0, at::kLong);
    std::vector<int64_t> flat_buckets(t_buckets.numel());
    std::memcpy(flat_buckets.data(), t_buckets.data_ptr<int64_t>(), flat_buckets.size() * sizeof(int64_t));

    auto dims = t_buckets.sizes();              // [D0, D1, D2]

    if (dims.size() != 3)
        throwf("dims.size(): expected 3, got: %zu", dims.size());

    auto d0 = dims[0];
    auto d1 = dims[1];
    auto d2 = dims[2];

    all_buckets = std::vector<std::vector<std::vector<int64_t>>>(
        d0, std::vector<std::vector<int64_t>>(
            d1, std::vector<int64_t>(d2)
    ));

    int64_t idx = 0;
    for (int64_t i = 0; i < d0; ++i) {
        for (int64_t j = 0; j < d1; ++j) {
            for (int64_t k = 0; k < d2; ++k, ++idx) {
                all_buckets[i][j][k] = flat_buckets[idx];
            }
        }
    }
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
    auto timer = ScopedTimer("getAction");
    auto any = s->getSupplementaryData();

    if (s->version() != version)
        throwf("getAction: unsupported IState version: want: %d, have: %d", version, s->version());

    if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
    auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
    if(!err.empty())
        throwf("getAction: anycast failed: %s", err);

    const auto *sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

    if (sup->getIsBattleEnded())
        return MMAI::Schema::ACTION_RESET;

    auto [inputs, size_idx] = prepareInputsV13(s, sup);
    auto values = std::vector<c10::IValue>{};

    for (auto &t : inputs) {
        values.push_back(t);
    }

    auto method = "predict" + std::to_string(size_idx);
    auto action = getScalar<int>(method);
    logAi->debug("AI action prediction: %d\n", action);

    // Also esitmate value
    // XXX: this value is useless (seems pretty random)
    /*
    auto vmethod = tjc->module.get_method("get_value");
    auto vinputs = std::vector<c10::IValue>{obs};
    auto vres = vmethod(vinputs).toDouble();
    logAi->debug("AI value estimation: %f\n", vres);
    */

    return static_cast<MMAI::Schema::Action>(action);
};

double TorchModel::getValue(const MMAI::Schema::IState * s) {
    auto any = s->getSupplementaryData();

    if (s->version() != version)
        throwf("getValue: unsupported IState version: want: %d, have: %d", version, s->version());

    if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
    auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
    if(!err.empty())
        throwf("getValue: anycast failed: %s", err);

    const auto *sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

    if (sup->getIsBattleEnded())
        return 0.0;

    auto [inputs, size_idx] = prepareInputsV13(s, sup);
    auto values = std::vector<c10::IValue>{};
    for (auto &t : inputs) {
        values.push_back(t);
    }

    auto method = "get_value" + std::to_string(size_idx);
    auto value = getScalar<float>(method);
    logAi->debug("AI value prediction: %f\n", value);

    return value;
}

} // namespace MMAI::BAI
