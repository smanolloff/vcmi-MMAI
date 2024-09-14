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

#include "schema/base.h"
#include "FallbackModel.h"

namespace MMAI::BAI {
    const std::vector<std::string> FALLBACKS = {"StupidAI", "BattleAI"};


    FallbackModel::FallbackModel(std::string keyword)
    : keyword(keyword)
    {
        auto it = std::find(FALLBACKS.begin(), FALLBACKS.end(), keyword);
        if (it == FALLBACKS.end()) {
            throw std::runtime_error("Unsupported fallback keyword: " + keyword);
        }
    }

    std::string FallbackModel::getName() {
        return keyword;
    };

    Schema::ModelType FallbackModel::getType() {
        return Schema::ModelType::SCRIPTED;
    };

    // The below methods should never be called on this object:
    // SCRIPTED models are dummy models which should not be used for anything
    // other than their getType() and getName() methods. Based on the return
    // value, the corresponding scripted bot (e.g. StupidAI) should be
    // used for the upcoming battle instead.

    int FallbackModel::getVersion() {
        warn("getVersion", -666);
        return -666;
    };

    int FallbackModel::getAction(const MMAI::Schema::IState * s) {
        warn("getAction", -666);
        return -666;
    };

    double FallbackModel::getValue(const MMAI::Schema::IState * s) {
        warn("getValue", -666);
        return -666;
    };

    void FallbackModel::warn(std::string m, int retval) {
        logAi->error("WARNING: method %s called on a FallbackModel object; returning %d\n", m.c_str(), retval);
    }
}
