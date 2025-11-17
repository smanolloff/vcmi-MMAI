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

// #include <ATen/core/TensorBody.h>
// #include <ATen/core/ivalue.h>
// #include <ATen/ops/from_blob.h>
// #include <ATen/ops/tensor.h>
#include <ATen/ATen.h>
#include <ATen/core/Generator.h>
#include <c10/core/ScalarType.h>
#include <mutex>
#include <tuple>
#include <limits>
#include <utility>

#include "TorchModel_LT.h"
#include "schema/schema.h"
#include "vstd/CLoggerBase.h"

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
        std::string name;
        std::chrono::steady_clock::time_point t0;
        explicit ScopedTimer(const std::string& n) : name(n), t0(std::chrono::steady_clock::now()) {}
        ~ScopedTimer() {
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
            logAi->info("%s: %lld ms", name, dt);
        }
    };

    struct SampleResult {
        int index;
        double prob;      // softmax probability of the chosen index
        bool fallback;    // true if choice came from the policy fallback
    };

    inline SampleResult sample_masked_logits(
        const at::Tensor& logits_1d,     // shape [K], float
        const at::Tensor& mask_1d,       // shape [K], bool
        bool throw_if_empty,
        float temperature = 1.0,
        const c10::optional<at::Generator>& gen = c10::nullopt
    ) {
        if (logits_1d.size(0) != mask_1d.size(0)) throwf("size mismatch");
        if (temperature < 0.0) throwf("negative temperature");

        // Count valid options
        auto n_valid = mask_1d.sum().item<int>();
        if (n_valid == 0) {
            if (throw_if_empty)
                throwf("No valid options available for act0");
            return {0, 0.0, true}; // fallback index
        }

        // Promote to float32 for stability if needed
        auto logits = logits_1d.toType(at::kFloat);
        auto mask = mask_1d.toType(at::kBool);

        auto neginf = at::full_like(logits, -INFINITY);
        auto masked_logits = at::where(mask, logits, neginf);

        int64_t idx_chosen;
        double p_chosen;

        if (temperature > 1e8) {
            // Uniform over valid
            auto uniform = at::zeros_like(logits);
            uniform.masked_fill_(mask, 1.0 / static_cast<double>(n_valid));
            auto sampled = at::multinomial(uniform, 1, false, gen);
            idx_chosen = sampled.item<int64_t>();
            p_chosen = uniform.index({idx_chosen}).item<double>();
        } else if (temperature < 1e-8) {
            // Argmax among valid
            const auto& vals = masked_logits; // already has -inf on invalids
            auto max_idx = std::get<1>(vals.max(-1, false)).item<int64_t>();
            idx_chosen = max_idx;
            p_chosen = 1.0;
        } else {
            // Standard softmax(logits / T)
            auto scaled = masked_logits / temperature;
            // stability: subtract max over valid slice
            auto m = std::get<0>(scaled.max(-1, false));
            auto stabilized = scaled - m;
            auto probs = at::softmax(stabilized, -1);
            if(!at::isfinite(probs).all().item<bool>())
                throwf("non-finite probabilities");
            auto sampled = at::multinomial(probs, 1, false, gen);
            idx_chosen = sampled.item<int64_t>();
            p_chosen = probs.index({idx_chosen}).item<double>();
        }

        return {static_cast<int>(idx_chosen), p_chosen, false};
    }

    struct TripletSample {
        int act0;  // over 4 actions
        int hex1;  // over 165 hexes
        int hex2;  // over 165 hexes
        double confidence;  // joint prob
    };

    inline TripletSample sample_triplet(
        const at::Tensor& act0_logits,   // [1, 4]
        const at::Tensor& hex1_logits,   // [1, 165]
        const at::Tensor& hex2_logits,   // [1, 165]
        const at::Tensor& mask_act0,     // [1, 4] bool
        const at::Tensor& mask_hex1,     // [1, 4, 165] bool
        const at::Tensor& mask_hex2,
        float temperature,
        const c10::optional<at::Generator>& gen = c10::nullopt
    ) {   // [1, 4, 165, 165] bool

        // Squeeze batch dimension
        auto a0_log = act0_logits.squeeze(0);
        auto h1_log = hex1_logits.squeeze(0);
        auto h2_log = hex2_logits.squeeze(0);

        auto m_a0 = mask_act0.squeeze(0).toType(at::kBool);
        auto m_h1 = mask_hex1.squeeze(0).toType(at::kBool); // [4,165]
        auto m_h2 = mask_hex2.squeeze(0).toType(at::kBool); // [4,165,165]

        // Sample act0
        auto act0 = sample_masked_logits(a0_log, m_a0, true, temperature, gen);

        // Slice masks for hex1 and hex2 based on act0 and hex1
        // Sample hex1
        auto m_h1_for_act0 = m_h1.index({act0.index, at::indexing::Slice()}); // [165]
        auto hex1 = sample_masked_logits(h1_log, m_h1_for_act0, false, temperature, gen);

        // Sample hex2
        auto m_h2_for_pair = m_h2.index({act0.index, hex1.index, at::indexing::Slice()}); // [165]
        auto hex2 = sample_masked_logits(h2_log, m_h2_for_pair, false, temperature, gen);

        // joint
        // printf("fback: %d     / %d     / %d\n", 0, hex1.fallback, hex2.fallback);
        // printf("probs: %.3f / %.3f / %.3f\n", act0.prob, hex1.prob, hex2.prob);
        double confidence = act0.prob * (hex1.fallback ? 1.0 : hex1.prob) * (hex2.fallback ? 1.0 : hex2.prob);

        return {act0.index, hex1.index, hex2.index, confidence};
    }


    std::array<std::vector<int32_t>, 165> buildNBR_unpadded(const std::vector<int64_t>& dst) {
        // Pass 1: validate and count degrees per node
        std::array<int, 165> deg{};
        for (size_t e = 0; e < dst.size(); ++e) {
            int v = static_cast<int>(dst[e]);
            if (v < 0 || v >= 165)
                throwf("dst contains node id out of range: ", v);
            ++deg[v];
        }

        std::array<std::vector<int32_t>, 165> nbr{};
        for (int v = 0; v < 165; ++v) nbr[v].reserve(deg[v]);
        for (size_t e = 0; e < dst.size(); ++e) {
            int v = static_cast<int>(dst[e]);
            nbr[v].push_back(static_cast<int32_t>(e));
        }

        return nbr;
    }

    struct IndexContainer {
        std::array<std::vector<int32_t>, 2> ei;
        std::vector<float> ea;
        std::array<std::vector<int32_t>, 165> nbrs;
    };

    struct BuildOutputs {
        int size_index = -1;                                // chosen index in all_sizes
        std::array<int32_t, LT_COUNT> emax{};               // chosen emax per link type
        std::array<int32_t, LT_COUNT> kmax{};               // chosen kmax per link type

        std::array<std::vector<int32_t>, 2> ei_flat;        // each length sum(emax)
        std::vector<float> ea_flat;                         // length sum(emax)
        std::array<std::vector<int32_t>, 165> nbrs_flat;    // each length sum(kmax)
    };

    // all_sizes: S x LT_COUNT x 2, where [s][l] = {emax, kmax}
    BuildOutputs build_flattened(
        const std::array<IndexContainer, LT_COUNT>& containers,
        const std::vector<std::vector<std::vector<int32_t>>>& all_sizes,
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
        std::array<int32_t, LT_COUNT> emax{}, kmax{};
        for (int s = 0; s < static_cast<int>(all_sizes.size()); ++s) {
            const auto& sz = all_sizes[s];
            if (sz.size() != LT_COUNT) continue;  // skip malformed
            bool ok = true;
            for (int l = 0; l < LT_COUNT && ok; ++l) {
                if (sz[l].size() != 2) { ok = false; break; }
                int32_t emax_l = sz[l][0];
                int32_t kmax_l = sz[l][1];
                if (emax_l < static_cast<int32_t>(e_req[l]) ||
                    kmax_l < static_cast<int32_t>(k_req[l])) {
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
                if (need > 0) dst.insert(dst.end(), need, static_cast<int32_t>(-1));
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
T TorchModel::getScalar(const std::string &method_name, const std::vector<c10::IValue>& input) {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, int>, "T must be float or int");
    at::ScalarType st;

    if constexpr (std::is_same_v<T, float>) {
        st = at::kFloat;
    } else { // T == int
        st = at::kInt;
    }

    auto t = call(method_name, input, 1, 1, st);

    if (t.dim() != 1)
        throwf("getScalar: %s: bad dim(): want: 1, have: %ld", method_name, t.dim());

    if constexpr (std::is_same_v<T, float>) {
        if (t.scalar_type() != at::kFloat)
            throwf("getScalar: %s: bad dtype: want: %d, have: %d", method_name, EI(at::kFloat), EI(t.scalar_type()));
        return t.item<float>();
    } else { // T == int
        if (t.scalar_type() != at::kInt)
            throwf("getScalar: %s: bad dtype: want: %d, have: %d", method_name, EI(at::kInt), EI(t.scalar_type()));

        return t.item<int32_t>();
    }
}

at::Tensor TorchModel::prepareDummyInput() {
    return at::tensor({0}, at::kInt);
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

    auto einds = std::vector<int32_t> {};
    einds.reserve(2*sum_e);
    for (auto &eind : build.ei_flat)
        einds.insert(einds.end(), eind.begin(), eind.end());

    auto nbrs = std::vector<int32_t> {};
    nbrs.reserve(165*sum_k);
    for (auto &nbr : build.nbrs_flat)
        nbrs.insert(nbrs.end(), nbr.begin(), nbr.end());

    // auto t_state = et_ext::from_blob(estate.data(), {int(estate.size())}, ScalarType::Float);
    auto t_state = at::from_blob(estate.data(), {int(estate.size())}, at::kFloat);
    auto t_ei_flat = at::from_blob(einds.data(), {2, sum_e}, at::kInt);
    auto t_ea_flat = at::from_blob(build.ea_flat.data(), {sum_e, 1}, at::kFloat);
    auto t_nbrs_flat = at::from_blob(nbrs.data(), {165, sum_k}, at::kInt);

    auto tensors = std::vector<at::Tensor> {
        t_state.clone(),
        t_ei_flat.clone(),
        t_ea_flat.clone(),
        t_nbrs_flat.clone()
    };

    return {tensors, build.size_index};
}

at::Tensor TorchModel::toTensor(
    const std::string& name,
    const c10::IValue& ivalue,
    int ndim,
    int numel,
    at::ScalarType st
) {
    if (!ivalue.isTensor())
        throwf("toTensor: %s: not a tensor", name);

    at::Tensor t = ivalue.toTensor().contiguous();

    if (t.dim() != ndim)
        throwf("toTensor: %s: bad dim(): want: %d, have: %ld", name, ndim, t.dim());

    if (t.scalar_type() != st)
        throwf("toTensor: %s: bad dtype: want: %d, have: %d", name, EI(st), EI(t.scalar_type()));

    if (numel && t.numel() != numel)
        throwf("toTensor: %s: bad numel: want: %d, have: %ld", name, numel, t.numel());

    return t;
}

at::Tensor TorchModel::call(
    const std::string& method_name,
    const std::vector<c10::IValue>& input,
    int numel,
    int ndim,
    at::ScalarType st
) {
    auto tag = "call: " + method_name;
    auto timer = ScopedTimer(tag);
    std::unique_lock lock(m);
    logAi->debug("%s...", tag);
    auto raw = model->get_method(method_name)(input);
    return toTensor(tag, raw, ndim, numel, st);
}

TorchModel::TorchModel(std::string &path, float temperature, uint64_t seed)
: path(path)
, temperature(temperature)
, model(std::make_unique<tj::mobile::Module>(tj::_load_for_mobile(path)))
{
    version = getScalar<int>("get_version");
    side = Schema::Side(getScalar<int>("get_side"));

    logAi->info("MMAI params: seed=%1%, temperature=%2%, model=%3%", seed, temperature, path);

    if (seed > 0) {
        rng = at::make_generator<at::CPUGeneratorImpl>();
        rng->set_current_seed(seed);
    }

    if (version != 13)
        throwf("unsupported model version: want: 13, have: %d", version);

    //
    // buckets
    //

    {
        auto t_buckets = call("get_all_sizes", 0, 3, at::kInt);
        std::vector<int32_t> flat_buckets(t_buckets.numel());
        std::memcpy(flat_buckets.data(), t_buckets.data_ptr<int32_t>(), flat_buckets.size() * sizeof(int32_t));

        auto dims = t_buckets.sizes();              // [D0, D1, D2]

        if (dims.size() != 3)
            throwf("dims.size(): expected 3, got: %zu", dims.size());

        auto d0 = dims[0];
        auto d1 = dims[1];
        auto d2 = dims[2];

        all_buckets = std::vector<std::vector<std::vector<int32_t>>>(
            d0, std::vector<std::vector<int32_t>>(
                d1, std::vector<int32_t>(d2)
        ));

        int32_t idx = 0;
        for (int32_t i = 0; i < d0; ++i) {
            for (int32_t j = 0; j < d1; ++j) {
                for (int32_t k = 0; k < d2; ++k, ++idx) {
                    all_buckets[i][j][k] = flat_buckets[idx];
                }
            }
        }
    }

    //
    // action_table
    //

    {
        auto t_table = call("get_action_table", 4*165*165, 3, at::kInt);
        std::vector<int32_t> flat_table(t_table.numel());
        std::memcpy(flat_table.data(), t_table.data_ptr<int32_t>(), flat_table.size() * sizeof(int32_t));

        auto dims = t_table.sizes();              // [D0, D1, D2]

        if (dims.size() != 3)
            throwf("dims.size(): expected 3, got: %zu", dims.size());

        auto d0 = dims[0];
        auto d1 = dims[1];
        auto d2 = dims[2];

        action_table = std::vector<std::vector<std::vector<int32_t>>>(
            d0, std::vector<std::vector<int32_t>>(
                d1, std::vector<int32_t>(d2)
        ));

        int32_t idx = 0;
        for (int32_t i = 0; i < d0; ++i) {
            for (int32_t j = 0; j < d1; ++j) {
                for (int32_t k = 0; k < d2; ++k, ++idx) {
                    action_table[i][j][k] = flat_table[idx];
                }
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

    auto method_name = "predict_with_logits" + std::to_string(size_idx);
    auto raw = model->get_method(method_name)(values);

    if (!raw.isTuple())
        throwf("call: %s: not a tensor", method_name);

    auto tuple = raw.toTuple();
    const auto& elems = tuple->elements();

    if (elems.size() != 10)
        throwf("call: %s: expected 10 outputs in the tuple", method_name);

    auto t_action = toTensor("predict_with_logits: t_action", elems[0], 1, 1, at::kInt);

    // deterministic action (useful for debugging)
    int action = t_action.item<int>();

    auto t_act0_logits = toTensor("predict_with_logits: t_act0_logits",  elems[1], 2, 4,         at::kFloat); // [1, 4]
    auto t_hex1_logits = toTensor("predict_with_logits: t_hex1_logits",  elems[2], 2, 165,       at::kFloat); // [1, 165]
    auto t_hex2_logits = toTensor("predict_with_logits: t_hex2_logits",  elems[3], 2, 165,       at::kFloat); // [1, 165]
    auto t_mask_act0   = toTensor("predict_with_logits: mask_act0",      elems[4], 2, 4,         at::kInt);   // [1, 4]
    auto t_mask_hex1   = toTensor("predict_with_logits: mask_hex1",      elems[5], 3, 4*165,     at::kInt);   // [1, 4, 165]
    auto t_mask_hex2   = toTensor("predict_with_logits: mask_hex2",      elems[6], 4, 4*165*165, at::kInt);   // [1, 4, 165, 165]
    auto t_act0        = toTensor("predict_with_logits: t_act0",         elems[7], 1, 1,         at::kInt);   // [1]
    auto t_hex1        = toTensor("predict_with_logits: t_hex1",         elems[8], 1, 1,         at::kInt);   // [1]
    auto t_hex2        = toTensor("predict_with_logits: t_hex2",         elems[9], 1, 1,         at::kInt);   // [1]

    auto sample = sample_triplet(
        t_act0_logits,  // [1, 4]
        t_hex1_logits,  // [1, 165]
        t_hex2_logits,  // [1, 165]
        t_mask_act0,    // [1, 4]
        t_mask_hex1,    // [1, 4, 165]
        t_mask_hex2,    // [1, 4, 165, 165]
        temperature,
        rng
    );

    auto s_action = action_table.at(sample.act0).at(sample.hex1).at(sample.hex2);

    if (s_action != action)
        logAi->debug("Sampled a non-greedy action: %d != %d", s_action, action);

    timer.name = boost::str(boost::format("MMAI action: %d (confidence=%.2f)") % action % sample.confidence);
    return static_cast<MMAI::Schema::Action>(s_action);
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
    logAi->debug("AI value prediction: %f", value);

    return value;
}

} // namespace MMAI::BAI
