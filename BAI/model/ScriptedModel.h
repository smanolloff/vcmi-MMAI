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

#include "schema/base.h"

namespace MMAI::BAI {
    class ScriptedModel : public MMAI::Schema::IModel {
    public:
        ScriptedModel(std::string keyword);

        Schema::ModelType getType() override;
        std::string getName() override;
        int getVersion() override;
        int getAction(const MMAI::Schema::IState * s) override;
        double getValue(const MMAI::Schema::IState * s) override;
    private:
        const std::string keyword;
        void warn(std::string m, int retval);
    };
}
