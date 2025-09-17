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

#include "ScriptedModel.h"
#include "schema/base.h"

namespace MMAI::BAI {
    const std::vector<std::string> FALLBACKS = {"StupidAI", "BattleAI"};


    ScriptedModel::ScriptedModel(std::string keyword)
    : keyword(keyword)
    {
        auto it = std::find(FALLBACKS.begin(), FALLBACKS.end(), keyword);
        if (it == FALLBACKS.end()) {
            throw std::runtime_error("Unsupported fallback keyword: " + keyword);
        }
    }

    std::string ScriptedModel::getName() {
        return keyword;
    }

    Schema::ModelType ScriptedModel::getType() {
        return Schema::ModelType::SCRIPTED;
    }

    Schema::Side ScriptedModel::getSide() {
        return Schema::Side::BOTH;
    }

    // The below methods should never be called on this object:
    // SCRIPTED models are dummy models which should not be used for anything
    // other than their getType() and getName() methods. Based on the return
    // value, the corresponding scripted bot (e.g. StupidAI) should be
    // used for the upcoming battle instead.

    int ScriptedModel::getVersion() {
        warn("getVersion", -666);
        return -666;
    };

    int ScriptedModel::getAction(const MMAI::Schema::IState * s) {
        warn("getAction", -666);
        return -666;
    };

    double ScriptedModel::getValue(const MMAI::Schema::IState * s) {
        warn("getValue", -666);
        return -666;
    };

    void ScriptedModel::warn(std::string m, int retval) {
        logAi->error("WARNING: method %s called on a ScriptedModel object; returning %d\n", m.c_str(), retval);
    }
}
