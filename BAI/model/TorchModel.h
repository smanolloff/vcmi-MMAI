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

#ifdef USING_EXECUTORCH
#include <executorch/extension/module/module.h>
#include <executorch/extension/tensor/tensor.h>
#endif

#include "schema/base.h"

namespace MMAI::BAI {

#ifdef USING_EXECUTORCH
    using namespace executorch;
#endif

    class TorchModel : public MMAI::Schema::IModel {
    public:
        TorchModel(std::string path);

        Schema::ModelType getType() override;
        std::string getName() override;
        int getVersion() override;
        int getAction(const MMAI::Schema::IState * s) override;
        double getValue(const MMAI::Schema::IState * s) override;
    private:
        std::string path;
        std::string name;
        int version;

#ifdef USING_EXECUTORCH
        class ModelContainer {
        public:
            ModelContainer(std::string path) : model(extension::Module(path)) {}
            extension::Module model;
        };

        std::unique_ptr<ModelContainer> mc;
        aten::Tensor prepareInput(const MMAI::Schema::IState * s);

        aten::Tensor call(
            const std::string& method_name,
            const std::vector<runtime::EValue>& input,
            int numel,
            aten::ScalarType st
        );

        inline aten::Tensor call(const std::string method_name, const runtime::EValue& ev, int numel, aten::ScalarType st) {
            return call(method_name, std::vector<runtime::EValue>{ev}, numel, st);
        }

        inline aten::Tensor call(const std::string method_name, int numel, aten::ScalarType st) {
            return call(method_name, std::vector<runtime::EValue>{}, numel, st);
        }
#endif
    };
}
