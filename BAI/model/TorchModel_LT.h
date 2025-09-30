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
    class TorchModel : public MMAI::Schema::IModel {
    public:
        TorchModel(std::string &path);

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

        std::pair<std::vector<at::Tensor>, int> prepareInputsV13(
            const MMAI::Schema::IState * state,
            const MMAI::Schema::V13::ISupplementaryData* sup,
            int bucket = -1
        );

        std::vector<std::vector<std::vector<int64_t>>> all_buckets;

        class TorchJitContainer {
        public:
            TorchJitContainer(std::string path) : module(torch::jit::_load_for_mobile(path)) {}
            c10::InferenceMode guard;
            torch::jit::mobile::Module module;
        };

        std::unique_ptr<TorchJitContainer> tjc;
    };
}
