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

#include <mutex>

#include <torch/csrc/jit/mobile/import.h>
#include <torch/csrc/jit/mobile/module.h>

#include "schema/v13/types.h"
#include "schema/base.h"

namespace MMAI::BAI {

namespace tj = torch::jit;

class TorchModel : public MMAI::Schema::IModel {
public:
    explicit TorchModel(std::string &path);

    Schema::ModelType getType() override;
    std::string getName() override;
    int getVersion() override;
    Schema::Side getSide() override;
    int getAction(const MMAI::Schema::IState * s) override;
    double getValue(const MMAI::Schema::IState * s) override;
private:
    std::string path;
    std::string name;
    int version;
    Schema::Side side;
    std::mutex m;
    std::vector<std::vector<std::vector<int64_t>>> all_buckets;

    std::pair<std::vector<at::Tensor>, int> prepareInputsV13(
        const MMAI::Schema::IState * state,
        const MMAI::Schema::V13::ISupplementaryData* sup,
        int bucket = -1
    );

    at::Tensor call(
        const std::string& method_name,
        const std::vector<c10::IValue>& input,
        int numel,
        int ndim,
        at::ScalarType st
    );

    at::Tensor call(const std::string &method_name, const c10::IValue& ev, int numel, int ndim, at::ScalarType st) {
        return call(method_name, std::vector<c10::IValue>{ev}, numel, ndim, st);
    }

    at::Tensor call(const std::string &method_name, int numel, int ndim, at::ScalarType st) {
        return call(method_name, std::vector<c10::IValue>{}, numel, ndim, st);
    }

    template <typename T> T getScalar(const std::string &method_name, const std::vector<c10::IValue>& input);
    template <typename T> T getScalar(const std::string &method_name) {
        return getScalar<T>(method_name, std::vector<c10::IValue>{});
    };

    c10::InferenceMode guard;
    std::unique_ptr<tj::mobile::Module> model;
};

} // namespace MMAI::BAI
