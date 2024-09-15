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

#include "json/JsonUtils.h"
#include <stdexcept>
#include <filesystem>
#include "AI/BattleAI/BattleAI.h"
#include "AI/StupidAI/StupidAI.h"
#include "BAI/model/TorchModel.h"
#include "BAI/model/ScriptedModel.h"
#include "BAI/router.h"
#include "BAI/base.h"
#include "CCallback.h"
#include "CConfigHandler.h"
#include "VCMIDirs.h"
#include "common.h"
#include "schema/base.h"
#include "scripted/summoner.h"

namespace MMAI::BAI {
    using ConfigStorage = std::map<std::string, std::string>;
    using ModelStorage = std::map<std::string, std::unique_ptr<TorchModel>>;

    static auto modelconfig = ConfigStorage();
    static auto models = ModelStorage();
    static std::unique_ptr<ScriptedModel> fallbackModel;
    static std::mutex modelmutex;

    static void InitModelConfigFromSettings() {
        auto lock = std::lock_guard(modelmutex);
        if (!modelconfig.empty()) return;

        auto node = JsonUtils::assembleFromFiles("config/ai/mmai/mmai-settings");
        for (auto &key : {"leftModel", "rightModel", "fallback"}) {
            if(node.Struct()[key].isString()) {
                modelconfig.insert({key, node.Struct()[key].String()});
            } else {
                logAi->warn("MMAI config contains invalid values: value for '%s' is not a string", key);
            }
        }
    }

    static void InitModelConfigFromBaggage(Schema::Baggage * baggage) {
        // Since ML client cannot load libtorch models, it sends TORCH_PATH
        // "models" whose getName() returns the path to the model to load
        auto lock = std::lock_guard(modelmutex);
        if (!modelconfig.empty()) return;

        if (baggage->modelLeft->getType() == Schema::ModelType::TORCH_PATH)
            modelconfig.insert({"leftModel", baggage->modelLeft->getName()});
        if (baggage->modelRight->getType() == Schema::ModelType::TORCH_PATH)
            modelconfig.insert({"rightModel", baggage->modelRight->getName()});
    }

    static Schema::IModel * GetModel(std::string key) {
        try {
            auto lock = std::lock_guard(modelmutex);
            auto it = models.find(key);

            if (it == models.end()) {
                auto it2 = modelconfig.find(key);
                if (it2 == modelconfig.end())
                    THROW_FORMAT("No such key in model config: %s", key);

                auto path = boost::filesystem::path(it2->second);

                if (path.is_relative()) {
                    logAi->debug("Model path '%s' is relative, will try to resolve it", path);
                    for (auto dirpath : VCMIDirs::get().dataPaths()) {
                        auto abspath = dirpath / "torchmodels" / path;
                        if (boost::filesystem::is_regular_file(abspath)) {
                            logAi->debug("Resolved from '%s'", dirpath.string());
                            path = abspath;
                            break;
                        }
                        logAi->debug("Could not resolve from '%s'", dirpath.string());
                    }
                }

                logAi->info("Loading %s model from %s", key, path.string());
                it = models.emplace(key, std::make_unique<TorchModel>(path.string())).first;
            } else {
                logAi->debug("Using previously loaded %s", key);
            }

            return it->second.get();
        } catch (std::exception & e) {
            logAi->error("Failed to load %s: %s", key, e.what());

            auto it2 = modelconfig.find("fallback");
            if (it2 == modelconfig.end() || it2->second.empty()) {
                logAi->error("Fallback model not configured, throwing...");
                throw;
            }

            auto lock = std::lock_guard(modelmutex);
            if (!fallbackModel)
                fallbackModel = std::make_unique<ScriptedModel>(it2->second);

            logAi->info("Will use fallback model: %s", fallbackModel->getName());
            return fallbackModel.get();
        }
    }

    Router::Router() {
        std::ostringstream oss;
        oss << this; // Store this memory address
        addrstr = oss.str();
        info("+++ constructor +++"); // log after addrstr is set
    }

    Router::~Router() {
        info("--- destructor ---");
    }

    void Router::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AICombatOptions aiCombatOptions_) {
        info("*** initBattleInterface ***");
        env = ENV;
        cb = CB;

        colorname = cb->getPlayerID()->toString();
        aiCombatOptions = aiCombatOptions_;

        // During training, baggage is used for injecting the model-in-training
        // (which acts as a bridge to vcmi-gym)
        auto &any = aiCombatOptions.other;
        if (any.has_value()) {
            auto &t = typeid(Schema::Baggage*);
            ASSERT(any.type() == t, boost::str(
                boost::format("Bad std::any payload type for aiCombatOptions.other: want: %s/%u, have: %s/%u") \
                % boost::core::demangle(t.name()) % t.hash_code() \
                % boost::core::demangle(any.type().name()) % any.type().hash_code()
            ));
            baggage = std::any_cast<Schema::Baggage*>(aiCombatOptions.other);
        }

        bai.reset();
    }

    /*
     * Delegated methods
     */

    void Router::actionFinished(const BattleID &bid, const BattleAction &action) {
        bai->actionFinished(bid, action);
    }

    void Router::actionStarted(const BattleID &bid, const BattleAction &action) {
        bai->actionStarted(bid, action);
    }

    void Router::activeStack(const BattleID &bid, const CStack * astack) {
        bai->activeStack(bid, astack);
    }

    void Router::battleAttack(const BattleID &bid, const BattleAttack *ba) {
        bai->battleAttack(bid, ba);
    }

    void Router::battleCatapultAttacked(const BattleID &bid, const CatapultAttack &ca) {
        bai->battleCatapultAttacked(bid, ca);
    }

    void Router::battleEnd(const BattleID &bid, const BattleResult *br, QueryID queryID) {
        bai->battleEnd(bid, br, queryID);
    }

    void Router::battleGateStateChanged(const BattleID &bid, const EGateState state) {
        bai->battleGateStateChanged(bid, state);
    };

    void Router::battleLogMessage(const BattleID &bid, const std::vector<MetaString> &lines) {
        bai->battleLogMessage(bid, lines);
    };

    void Router::battleNewRound(const BattleID &bid) {
        bai->battleNewRound(bid);
    }

    void Router::battleNewRoundFirst(const BattleID &bid) {
        bai->battleNewRoundFirst(bid);
    }

    void Router::battleObstaclesChanged(const BattleID &bid, const std::vector<ObstacleChanges> &obstacles) {
        bai->battleObstaclesChanged(bid, obstacles);
    };

    void Router::battleSpellCast(const BattleID &bid, const BattleSpellCast *sc) {
        bai->battleSpellCast(bid, sc);
    }

    void Router::battleStackMoved(const BattleID &bid, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport) {
        bai->battleStackMoved(bid, stack, dest, distance, teleport);
    }

    void Router::battleStacksAttacked(const BattleID &bid, const std::vector<BattleStackAttacked> &bsa, bool ranged) {
        bai->battleStacksAttacked(bid, bsa, ranged);
    }

    void Router::battleStacksEffectsSet(const BattleID &bid, const SetStackEffect &sse) {
        bai->battleStacksEffectsSet(bid, sse);
    }

    void Router::battleStart(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) {
        Schema::IModel * model;

        if (baggage) {
            // During training, the model object is provided via the baggage
            // This is a special model (a bridge between VCMI and vcmi-gym).
            // Additionally, the `cb->getPlayerID` is used instead of `side`
            // to accomodate for the "side swapping" training feature.
            // XXX: dev mode assumes there are no neutral players in battle
            ASSERT(baggage, "baggage is nullptr");
            ASSERT(cb->getPlayerID()->hasValue(), "cb->getPlayerID() has no value");
            model = cb->getPlayerID()->num ? baggage->modelRight : baggage->modelLeft;
            ASSERT(model, "model is nullptr");
            InitModelConfigFromBaggage(baggage);
            if (model->getType() == Schema::ModelType::TORCH_PATH) {
                // If the baggage does not carry a model, it means we must load one from file
                // This occurs when training is done vs. another pre-trained model
                // For example, leftModel is an injected model (being trained),
                // while rightModel is nullptr which stands for "load a pre-trained
                // model as usual"
                model = GetModel(side == BattleSide::ATTACKER ? "leftModel" : "rightModel");
            }
        } else {
            InitModelConfigFromSettings();
            model = GetModel(side == BattleSide::ATTACKER ? "leftModel" : "rightModel");
        }

        ASSERT(model, "failed to build model");

        switch (model->getType()) {
        case Schema::ModelType::SCRIPTED:
            if (model->getName() == "StupidAI") {
                bai = CDynLibHandler::getNewBattleAI("StupidAI");
                bai->initBattleInterface(env, cb, aiCombatOptions);
            } else if (model->getName() == "BattleAI") {
                bai = CDynLibHandler::getNewBattleAI("BattleAI");
                bai->initBattleInterface(env, cb, aiCombatOptions);
            } else if (model->getName() == MMAI_RESERVED_NAME_SUMMONER) {
                bai = std::make_shared<Scripted::Summoner>();
                bai->initBattleInterface(env, cb, aiCombatOptions);
            } else {
                THROW_FORMAT("Unexpected scripted model name: %s", model->getName());
            }
            break;
        case Schema::ModelType::TORCH:
        case Schema::ModelType::USER:
            // XXX: must not call initBattleInterface here
            bai = Base::Create(model, env, cb);
            break;

        // TORCH_PATH models should have been replaced by TORCH models
        case Schema::ModelType::TORCH_PATH:
        default:
            THROW_FORMAT("Unexpected model type: %d", EI(model->getType()));
        }

        bai->battleStart(bid, army1, army2, tile, hero1, hero2, side, replayAllowed);
    }

    void Router::battleTriggerEffect(const BattleID &bid, const BattleTriggerEffect &bte) {
        bai->battleTriggerEffect(bid, bte);
    }

    void Router::battleUnitsChanged(const BattleID &bid, const std::vector<UnitChanges> &units) {
        bai->battleUnitsChanged(bid, units);
    }

    void Router::yourTacticPhase(const BattleID &bid, int distance) {
        bai->yourTacticPhase(bid, distance);
    }

    /*
     * private
     */

    void Router::error(const std::string &text) const { log(ELogLevel::ERROR, text); }
    void Router::warn(const std::string &text) const { log(ELogLevel::WARN, text); }
    void Router::info(const std::string &text) const { log(ELogLevel::INFO, text); }
    void Router::debug(const std::string &text) const { log(ELogLevel::DEBUG, text); }
    void Router::trace(const std::string &text) const { log(ELogLevel::TRACE, text); }
    void Router::log(ELogLevel::ELogLevel level, const std::string &text) const {
        if (logAi->getLevel() <= level) logAi->debug("Router-%s [%s] %s", addrstr, colorname, text);
    }
}
