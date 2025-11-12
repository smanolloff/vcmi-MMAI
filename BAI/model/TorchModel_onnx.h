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

#pragma once

#include <onnxruntime_cxx_api.h>   // from the onnx project

#include "schema/v13/types.h"
#include "schema/base.h"

namespace MMAI::BAI {

inline Ort::Env& ort_env() {
    static Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "app"};
    return env;
}

class TorchModel : public MMAI::Schema::IModel {
public:
    explicit TorchModel(std::string &path, float temperature, uint64_t seed);

    Schema::ModelType getType() override;
    std::string getName() override;
    int getVersion() override;
    Schema::Side getSide() override;
    int getAction(const MMAI::Schema::IState * s) override;
    double getValue(const MMAI::Schema::IState * s) override;
private:
    std::string path;
    float temperature;
    // c10::optional<at::Generator> rng;
    std::string name;
    int version;
    Schema::Side side;

    std::mt19937 rng;
    std::vector<std::vector<std::vector<int32_t>>> all_buckets;
    std::vector<std::vector<std::vector<int32_t>>> action_table;
    std::vector<Ort::AllocatedStringPtr> input_name_ptrs;
    std::vector<Ort::AllocatedStringPtr> output_name_ptrs;

    std::unique_ptr<Ort::Session> model = nullptr;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo meminfo;

    std::pair<std::vector<Ort::Value>, int> prepareInputsV13(
        const MMAI::Schema::IState * state,
        const MMAI::Schema::V13::ISupplementaryData* sup,
        int bucket = -1
    );

    template <typename T>
    Ort::Value toTensor(const std::string& name, std::vector<T>& vec, const std::vector<int64_t>& shape);

    template <typename T>
    std::vector<T> t2v(const std::string& name, const Ort::Value& tensor, int numel);

    std::vector<const char*> input_names;
    std::vector<const char*> output_names;

    // at::Tensor call(
    //     const std::string& method_name,
    //     const std::vector<c10::IValue>& input,
    //     int numel,
    //     int ndim,
    //     at::ScalarType st
    // );

    // at::Tensor call(const std::string &method_name, const c10::IValue& ev, int numel, int ndim, at::ScalarType st) {
    //     return call(method_name, std::vector<c10::IValue>{ev}, numel, ndim, st);
    // }

    // at::Tensor call(const std::string &method_name, int numel, int ndim, at::ScalarType st) {
    //     return call(method_name, std::vector<c10::IValue>{prepareDummyInput()}, numel, ndim, st);
    // }

    // template <typename T> T getScalar(const std::string &method_name, const std::vector<c10::IValue>& input);
    // template <typename T> T getScalar(const std::string &method_name) {
    //     return getScalar<T>(method_name, std::vector<c10::IValue>{prepareDummyInput()});
    // };
};

} // namespace MMAI::BAI
