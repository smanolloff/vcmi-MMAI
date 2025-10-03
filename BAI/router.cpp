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
#include "callback/CBattleCallback.h"
#include "callback/CDynLibHandler.h"
#include "filesystem/Filesystem.h"
#include "json/JsonUtils.h"
#include "VCMIDirs.h"

#include "AI/BattleAI/BattleAI.h"
#include "AI/StupidAI/StupidAI.h"
#include "BAI/base.h"
#include "BAI/model/ScriptedModel.h"
#include "BAI/model/TorchModel.h"
#include "BAI/router.h"
#include "common.h"
#include "schema/schema.h"

namespace MMAI::BAI {
    using ConfigStorage = std::map<std::string, std::string>;
    using ModelStorage = std::map<std::string, std::unique_ptr<TorchModel>>;

    #if defined(USING_EXECUTORCH)
    static auto modelExt = ".pte";
    #elif defined(USING_LIBTORCH)
    static auto modelExt = ".ptl";
    #endif

    static auto modelconfig = ConfigStorage();
    static auto models = ModelStorage();
    static std::unique_ptr<ScriptedModel> fallbackModel;
    static std::mutex modelmutex;

    static void InitModelConfigFromSettings() {
        auto lock = std::lock_guard(modelmutex);
        if (!modelconfig.empty()) return;

        auto cfg = JsonUtils::assembleFromFiles("MMAI/CONFIG/mmai-settings.json").Struct();

        for (const auto &key : {"attacker", "defender", "fallback"}) {
            if(cfg[key].isString()) {
                std::string value = cfg[key].String();
                if (std::string(key) != "fallback")
                    value = "MMAI/models/" + value + modelExt;

                modelconfig.insert({key, value});
            } else {
                logAi->warn("MMAI config contains invalid values: value for '%s' is not a string", key);
            }
        }
    }

    static Schema::IModel * GetModel(std::string key) {
        try {
            auto lock = std::lock_guard(modelmutex);
            auto it = models.find(key);

            if (it == models.end()) {
                auto it2 = modelconfig.find(key);
                if (it2 == modelconfig.end())
                    THROW_FORMAT("No such key in model config: %s", key);

                logAi->debug("Found value for key %s: %s", key, it2->second);

                auto rpath = ResourcePath(it2->second);
                auto loaders = CResourceHandler::get()->getResourcesWithName(rpath);

                if (loaders.size() != 1)
                    logAi->warn("Expected 1 loader, found %d for %s", static_cast<int>(loaders.size()), rpath.getName());

                auto fullpath = loaders.at(0)->getResourceName(rpath);
                ASSERT(fullpath.has_value(), "could not obtain path for resource " + rpath.getName());
                auto fullpathstr = fullpath.value().string();

                logAi->info("Loading Torch %s model from %s", key, fullpathstr);
                it = models.emplace(key, std::make_unique<TorchModel>(fullpathstr)).first;
            } else {
                logAi->debug("Using previously loaded %s", key);
            }

            return it->second.get();
        } catch (std::exception & e) {
            logAi->error("Failed to load %s: %s", key, e.what());

            #ifdef ENABLE_MMAI_STRICT_LOAD
            throw;
            #endif

            // TODO: how to log a message in user interface?

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
        cb->waitTillRealize = wasWaitingForRealize;
    }

    void Router::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) {
        info("*** initBattleInterface ***");
        env = ENV;
        cb = CB;
        colorname = cb->getPlayerID()->toString();
        wasWaitingForRealize = cb->waitTillRealize;

        cb->waitTillRealize = false;
        bai.reset();
    }

    void Router::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences _) {
        initBattleInterface(ENV, CB);
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

    void Router::battleStackMoved(const BattleID &bid, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport) {
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
        InitModelConfigFromSettings();
        auto modelkey = side == BattleSide::ATTACKER ? "attacker" : "defender";
        model = GetModel(modelkey);

        auto modelside = model->getSide();
        auto realside = Schema::Side(EI(side));

        if (modelside != realside && modelside != Schema::Side::BOTH) {
            logAi->warn("The loaded '%s' model was not trained to play as %s", modelkey, modelkey);
        }

        // printf("(side=%d) hero0: %s, hero1: %s\n", EI(side), hero1->nameCustomTextId.c_str(), hero2->nameCustomTextId.c_str());

        switch (model->getType()) {
        case Schema::ModelType::SCRIPTED:
            if (model->getName() == "StupidAI") {
                bai = CDynLibHandler::getNewBattleAI("StupidAI");
                bai->initBattleInterface(env, cb);
            } else if (model->getName() == "BattleAI") {
                bai = CDynLibHandler::getNewBattleAI("BattleAI");
                bai->initBattleInterface(env, cb);
            } else {
                THROW_FORMAT("Unexpected scripted model name: %s", model->getName());
            }
            break;
        case Schema::ModelType::TORCH:
            // XXX: must not call initBattleInterface here
            bai = Base::Create(model, env, cb);
            break;

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
        if (logAi->getEffectiveLevel() <= level) logAi->debug("Router-%s [%s] %s", addrstr, colorname, text);
    }
}
