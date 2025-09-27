#include <algorithm>
#include <executorch/extension/tensor/tensor_ptr.h>
#include <stdexcept>
#include <executorch/runtime/platform/runtime.h>

#include "TorchModel.h"

namespace MMAI::BAI {

using TensorPtr = executorch::extension::TensorPtr;

constexpr int LT_COUNT = EI(MMAI::Schema::V13::LinkType::_count);

namespace {
    template<class... Args>
    [[noreturn]] inline void throwf(const std::string& fmt, Args&&... args) {
        boost::format f("TorchModel: " + fmt);
        (void)std::initializer_list<int>{ ( (f % std::forward<Args>(args)), 0 )... };
        throw std::runtime_error(f.str());
    }

    struct ScopedTimer {
        const char* name;
        std::chrono::steady_clock::time_point t0;
        explicit ScopedTimer(const char* n) : name(n), t0(std::chrono::steady_clock::now()) {}
        ~ScopedTimer() {
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
            logAi->debug("%s: %lld ms", name, dt);
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
        const std::vector<std::vector<std::vector<int64_t>>>& all_sizes)
    {
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
            throwf("ei_flat.at(0) size mismatch: want: %d, have: %d", sum_emax, out.ei_flat.at(0).size());
        if (out.ei_flat.at(1).size() != sum_emax)
            throwf("ei_flat.at(1) size mismatch: want: %d, have: %d", sum_emax, out.ei_flat.at(1).size());
        if (out.ea_flat.size() != sum_emax)
            throwf("ea_flat size mismatch: want: %d, have: %d", sum_emax, out.ea_flat.size());

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

TorchModel::TorchModel(std::string &path)
: path(path) {
    auto loaderRes = executorch::extension::FileDataLoader::from(path.c_str());
    if (!loaderRes.ok())
        throwf("loader error code: %d", static_cast<int>(loaderRes.error()));

    loader = std::make_unique<executorch::extension::FileDataLoader>(std::move(*loaderRes));

    memory_allocator = std::make_unique<executorch::extension::MallocMemoryAllocator>();
    temp_allocator = std::make_unique<executorch::extension::MallocMemoryAllocator>();

    executorch::runtime::runtime_init();

    auto programRes = executorch::runtime::Program::load(loader.get());
    if (!programRes.ok())
        throwf("program error code: %d", static_cast<int>(programRes.error()));
    program = std::make_unique<executorch::runtime::Program>(std::move(*programRes));

    auto t_version = call("get_version", 1, ScalarType::Long);
    version = static_cast<int>(t_version.const_data_ptr<int64_t>()[0]);

    auto t_side = call("get_side", 1, ScalarType::Long);
    side = Schema::Side(static_cast<int>(t_side.const_data_ptr<int64_t>()[0]));

    auto t_all_sizes = call("get_all_sizes", 0, ScalarType::Long);

    // Convert 3-D tensor to vector<vector<int64>>
    auto ndim = t_all_sizes.dim();
    if (ndim != 3)
        throwf("t_all_sizes: bad ndim: want: %d, have: %d", 3, EI(t_all_sizes.dim()));

    auto sz = t_all_sizes.sizes();
    int d0 = sz[0];
    int d1 = sz[1];
    int d2 = sz[2];

    auto st = t_all_sizes.strides();
    int s0 = st[0];
    int s1 = st[1];
    int s2 = st[2];

    // last time strides=1 means vector is contiguous (which is what we want)
    // XXX: turns out this is not contiguous
    // if (s2 != 1)
    //     throwf("t_all_sizes: bad strides on last dim: want: 1, have: %d", s2);

    if (d1 != LT_COUNT)
        throwf("t_all_sizes: bad size(1): want: %d, have: %d", LT_COUNT, d1);

    if (d2 != 2)
        throwf("t_all_sizes: bad size(2): want: %d, have: %d", 2, d2);

    // print_tensor_like_torch(std::make_shared<executorch::runtime::etensor::Tensor>(t_all_sizes));

    const int64_t* baseptr = t_all_sizes.const_data_ptr<int64_t>(); // or equivalent getter
    all_sizes.resize(d0);
    for (int i0=0; i0<d0; ++i0) {
        all_sizes.at(i0).resize(d1);
        for (int i1=0; i1<d1; ++i1) {
            all_sizes.at(i0).at(i1).resize(d2);
            const int64_t* ptr = baseptr + i0 * s0 + i1 * s1;

            if (s2 == 1) {
                // Fast path: last dimension contiguous
                std::memcpy(all_sizes.at(i0).at(i1).data(), ptr, sizeof(int64_t) * static_cast<size_t>(d2));
            } else {
                // Generic path: strided copy along last dimension
                for (int64_t k = 0; k < d2; ++k) {
                    all_sizes.at(i0).at(i1).at(k) = ptr[k * s2];
                }
            }
        }
    }
}

void TorchModel::maybeLoadMethod(const std::string& method_name) {
    if (methods.count(method_name))
        return;

    MethodHolder mh;

    auto methodMetaRes = program->method_meta(method_name.c_str());
    if (!methodMetaRes.ok())
        throwf("method_meta: error code: %d", static_cast<int>(methodMetaRes.error()));

    auto method_metadata = methodMetaRes.get();
    const auto planned_buffers_count = method_metadata.num_memory_planned_buffers();
    mh.planned_buffers.reserve(planned_buffers_count);
    mh.planned_spans.reserve(planned_buffers_count);

    for (auto index = 0; index < planned_buffers_count; ++index) {
        const auto buffer_size = method_metadata.memory_planned_buffer_size(index).get();
        mh.planned_buffers.emplace_back(buffer_size);
        mh.planned_spans.emplace_back(mh.planned_buffers.back().data(), buffer_size);
    }

    mh.planned_memory = std::make_unique<executorch::runtime::HierarchicalAllocator>(
        executorch::runtime::Span(mh.planned_spans.data(), mh.planned_spans.size())
    );

    mh.memory_manager = std::make_unique<executorch::runtime::MemoryManager>(memory_allocator.get(), mh.planned_memory.get(), temp_allocator.get());

    auto methodRes = program->load_method(method_name.c_str(), mh.memory_manager.get());
    if (!methodRes.ok())
        throwf("load_method: error code: %d", static_cast<int>(methodRes.error()));

    mh.method = std::make_unique<executorch::runtime::Method>(std::move(*methodRes));
    mh.inputs.resize(mh.method->inputs_size());
    methods.emplace(method_name, std::move(mh));
}

Tensor TorchModel::call(
    const std::string& method_name,
    const std::vector<EValue>& input,
    int resNumel,
    ScalarType st
) {
    maybeLoadMethod(method_name);
    auto& method = methods.at(method_name).method;
    auto& inputs = methods.at(method_name).inputs;

    if (input.size() != inputs.size())
        throwf("call: %s: input size: %zu does not match method input size: %zu", method_name, input.size(), inputs.size());

    for (auto i = 0; i < input.size(); ++i) {
        inputs[i] = input[i];
    }

    auto setRes = method->set_inputs(executorch::aten::ArrayRef<EValue>(inputs.data(), inputs.size()));
    if (setRes != et_run::Error::Ok)
        throwf("set_inputs: %s: error code: %d", method_name, static_cast<int>(setRes));

    auto execRes = method->execute();
    if (setRes != et_run::Error::Ok)
        throwf("execute: %s: error code: %d", method_name, static_cast<int>(execRes));

    const auto outputs_size = method->outputs_size();
    auto outputs = std::vector<EValue>(outputs_size);

    // Copy data to outputs
    auto getoutRes = method->get_outputs(outputs.data(), outputs_size);
    if (getoutRes != et_run::Error::Ok)
        throwf("execute: %s: error code: %d", method_name, static_cast<int>(execRes));

    if (outputs.size() != 1)
        throwf("call: %s: outputs.size(): want: 1, have: %zu", method_name, outputs.size());

    auto out = outputs.at(0);

    if (!out.isTensor())
        throwf("call: %s: not a tensor", method_name);

    auto t = out.toTensor();

    if (resNumel && t.numel() != resNumel)
        throwf("call: %s: bad resNumel: want: %d, have: %d", method_name, resNumel, EI(t.numel()));

    if (t.dtype() != st)
        throwf("call: %s: bad dtype: want: %d, have: %d", method_name, EI(st), EI(t.dtype()));

    return t;
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

std::pair<std::vector<TensorPtr>, int> TorchModel::prepareInputsV13(
    const MMAI::Schema::IState * s,
    const MMAI::Schema::V13::ISupplementaryData* sup
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
            throwf("unexpected dstinds.size() for LinkType(%d): want: %d, have: %d", nlinks, dstinds.size());

        if (attrs.size() != nlinks)
            throwf("unexpected attrs.size() for LinkType(%d): want: %d, have: %d", nlinks, attrs.size());

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
        throwf("unexpected links count: want: %d, have: %d", LT_COUNT % count);

    auto build = build_flattened(containers, all_sizes);

    const auto *state = s->getBattlefieldState();
    auto estate = std::vector<float>(state->size());
    std::copy(state->begin(), state->end(), estate.begin());

    int sum_e = build.ei_flat.at(0).size();
    int sum_k = build.nbrs_flat.at(0).size();

    if (build.ei_flat.at(0).size() != sum_e)
        throwf("unexpected build.ei_flat.at(0).size(): want: %d, have: %d", sum_e % build.ei_flat.at(0).size());
    if (build.ei_flat.at(1).size() != sum_e)
        throwf("unexpected build.ei_flat.at(1).size(): want: %d, have: %d", sum_e % build.ei_flat.at(1).size());
    if (build.ea_flat.size() != sum_e)
        throwf("unexpected build.ea_flat.size(): want: %d, have: %d", sum_e % build.ea_flat.size());
    for (int i=0; i<165; ++i) {
        if (build.nbrs_flat.at(i).size() != sum_k) {
            throwf("unexpected build.nbrs_flat.at(%d).size(): want: %d, have: %d", i % sum_k % build.nbrs_flat.at(i).size());
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

    auto t_state = et_ext::from_blob(estate.data(), {int(estate.size())}, ScalarType::Float);
    auto t_ei_flat = et_ext::from_blob(einds.data(), {2, sum_e}, ScalarType::Long);
    auto t_ea_flat = et_ext::from_blob(build.ea_flat.data(), {sum_e, 1}, ScalarType::Float);
    auto t_nbrs_flat = et_ext::from_blob(nbrs.data(), {165, sum_k}, ScalarType::Long);

    auto tensors = std::vector<TensorPtr> {
        et_ext::clone_tensor_ptr(t_state),
        et_ext::clone_tensor_ptr(t_ei_flat),
        et_ext::clone_tensor_ptr(t_ea_flat),
        et_ext::clone_tensor_ptr(t_nbrs_flat)
    };

    return {tensors, build.size_index};
}

int TorchModel::getAction(const MMAI::Schema::IState * s) {
    auto timer = ScopedTimer("getAction");
    auto any = s->getSupplementaryData();

    if (version != 13)
        throwf("unsupported model version: want: 13, have: %d", version);

    if (s->version() != 13)
        throwf("unsupported IState version: want: 13, have: %d", s->version());

    if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
    auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
    if(!err.empty())
        throwf("anycast failed: %s", err);

    const auto *sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

    if (sup->getIsBattleEnded())
        return MMAI::Schema::ACTION_RESET;

    auto [inputs, size_idx] = prepareInputsV13(s, sup);

    auto values = std::vector<EValue>{};

    // int i=0;
    for (auto &t : inputs) {
        values.push_back(t);
        // printf("Input %d\n", i++);
        // print_tensor_like_torch(t);
    }

    // printf("--------------- 0:\n");
    // print_tensor_like_torch(inputs.at(0), 10, 6); // show up to 4 per dim, 6 decimals
    // printf("--------------- 1:\n");
    // print_tensor_like_torch(inputs.at(1), 10, 6); // show up to 4 per dim, 6 decimals
    // printf("--------------- 2:\n");
    // print_tensor_like_torch(inputs.at(2), 10, 6); // show up to 4 per dim, 6 decimals
    // printf("--------------- 3:\n");
    // print_tensor_like_torch(inputs.at(3), 10, 6); // show up to 4 per dim, 6 decimals

    auto output = call("predict" + std::to_string(size_idx), values, 1, ScalarType::Long);

    int action = output.const_data_ptr<int64_t>()[0];

    // throw std::runtime_error("forced exit");
    return MMAI::Schema::Action(action);
};

double TorchModel::getValue(const MMAI::Schema::IState * s) {
    auto any = s->getSupplementaryData();

    if (version != 13)
        throwf("unsupported model version: want: 13, have: %d", version);

    if (s->version() != 13)
        throwf("unsupported IState version: want: 13, have: %d", s->version());

    if(!any.has_value()) throw std::runtime_error("extractSupplementaryData: supdata is empty");
    auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData*));
    if(!err.empty())
        throwf("anycast failed: %s", err);

    const auto *sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData*>(any);

    if (sup->getIsBattleEnded())
        return 0.0;

    auto [inputs, size_idx] = prepareInputsV13(s, sup);
    auto values = std::vector<EValue>{};
    for (auto &t : inputs)
        values.push_back(t);

    auto output = call("predict" + std::to_string(size_idx), values, 1, ScalarType::Float);
    auto value = output.const_data_ptr<float>()[0];

    return value;
}

} // namespace MMAI::BAI
