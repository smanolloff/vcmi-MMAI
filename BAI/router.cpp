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
#include "filesystem/Filesystem.h"
#include "json/JsonUtils.h"
#include "VCMIDirs.h"

#include "schema/schema.h"

#include "common.h"
#include "AI/BattleAI/BattleAI.h"
#include "AI/StupidAI/StupidAI.h"
#include "BAI/model/TorchModel.h"
#include "BAI/model/ScriptedModel.h"
#include "BAI/router.h"
#include "BAI/base.h"
#include "scripted/summoner.h"

#include <ATen/core/ivalue.h>
#include <ATen/ops/from_blob.h>
#include <torch/csrc/jit/mobile/module.h>
#include <torch/csrc/jit/mobile/import.h>

namespace MMAI::BAI {
    class TorchJitContainer {
    public:
        TorchJitContainer(std::string path) : module(torch::jit::_load_for_mobile(path))
        {}

        c10::InferenceMode guard;
        torch::jit::mobile::Module module;
    };

    class TorchModel : public MMAI::Schema::IModel {
    private:
        std::string path;
        std::string name;
        int version;
        int sizeOneHex;
        int nactions;
        int actionOffset = 0;
        std::mutex m;
        std::unique_ptr<TorchJitContainer> tjc;
    public:
        TorchModel(std::string path)
        : path(path)
        , tjc(std::make_unique<TorchJitContainer>(path))
        {
            version = tjc->module.get_method("get_version")({}).toInt();

            switch(version) {
                break; case 1:
                    sizeOneHex = MMAI::Schema::V1::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                    nactions = MMAI::Schema::V1::N_ACTIONS;
                break; case 2:
                    sizeOneHex = MMAI::Schema::V2::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                    nactions = MMAI::Schema::V1::N_ACTIONS;
                break; case 3:
                    sizeOneHex = MMAI::Schema::V3::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                    nactions = MMAI::Schema::V3::N_ACTIONS;
                break; case 4:
                    sizeOneHex = MMAI::Schema::V4::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                    nactions = MMAI::Schema::V4::N_ACTIONS;
                break; default:
                    throw std::runtime_error("Unknown MMAI version: " + std::to_string(version));
            }

            // XXX: jit::mobile::Module has no toModule() attribute
            //      Maybe a call to .predict() with a dummy observation would work
            //
            // auto out_features = tjc->module.attr("actor", c10::IValue("method_not_found:actor")).toModule().attr("out_features").toInt();
            // switch(out_features) {
            // break; case 2311: actionOffset = 1;
            // break; case 2312: actionOffset = 0;
            // break; default:
            //     throw std::runtime_error("Expected 2311 or 2312 out_features for actor, got: " + std::to_string(out_features));
            // }
        }

        Schema::ModelType getType() {
            return Schema::ModelType::TORCH;
        };

        std::string getName() {
            return "MMAI_MODEL";
        };

        int getVersion() {
            return version;
        };

        int getAction(const MMAI::Schema::IState * s) {
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "1");
            auto any = s->getSupplementaryData();
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "2");
            auto ended = false;

            switch(version) {
                break; case 1:
                       case 2:
                    ended = std::any_cast<const MMAI::Schema::V1::ISupplementaryData*>(any)->getIsBattleEnded();
                break; case 3:
                    ended = std::any_cast<const MMAI::Schema::V3::ISupplementaryData*>(any)->getIsBattleEnded();
                break; case 4:
                    ended = std::any_cast<const MMAI::Schema::V4::ISupplementaryData*>(any)->getIsBattleEnded();
                break; default:
                    throw std::runtime_error("Unknown MMAI version: " + std::to_string(version));
            }

            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "3");
            if (ended)
                return MMAI::Schema::ACTION_RESET;

            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "4");
            auto &src = s->getBattlefieldState();
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "5");
            auto dst = MMAI::Schema::BattlefieldState{};
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "6");
            dst.resize(src.size());
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "7");
            std::copy(src.begin(), src.end(), dst.begin());
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "8");

            auto obs = at::from_blob(dst.data(), {static_cast<long long>(dst.size())}, at::kFloat);
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "9");

            // yields no performance benefit over (safer) copy approach:
            // auto obs = torch::from_blob(const_cast<float*>(s->getBattlefieldState().data()), {11, 15, sizeOneHex}, torch::kFloat);

            auto intmask = std::vector<int>{};
            intmask.reserve(nactions);
            auto &boolmask = s->getActionMask();
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "10");

            int i = 0;
            for (auto it = boolmask.begin() + actionOffset; it != boolmask.end(); ++it) {
                // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "boolmask i=%d", i);
                i++;
                intmask.push_back(static_cast<int>(*it));
            }
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "11");

            intmask.at(0) = 0;  // prevent retreats for now

            auto mask = at::from_blob(intmask.data(), {static_cast<long>(intmask.size())}, at::kInt).to(at::kBool);
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "12");

            // auto mask_accessor = mask.accessor<bool,1>();
            // for (int i = 0; i < mask_accessor.size(0); ++i)
            //     printf("mask[%d]=%d\n", i, mask_accessor[i]);

            std::unique_lock lock(m);
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "13");

            auto method = tjc->module.get_method("predict");
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "14");
            auto inputs = std::vector<c10::IValue>{obs, mask};
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "15");
            auto res = method(inputs).toInt() + actionOffset;
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "16");

            logAi->debug("AI action prediction: %d\n", int(res));

            // Also esitmate value
            auto vmethod = tjc->module.get_method("get_value");
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "17");
            auto vinputs = std::vector<c10::IValue>{obs};
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "18");
            auto vres = vmethod(vinputs).toDouble();
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "19");
            logAi->debug("AI value estimation: %f\n", vres);

            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "20");
            return MMAI::Schema::Action(res);
        };

        double getValue(const MMAI::Schema::IState * s) {
            auto &src = s->getBattlefieldState();
            auto dst = MMAI::Schema::BattlefieldState{};
            dst.reserve(dst.size());
            std::copy(src.begin(), src.end(), dst.begin());

            std::unique_lock lock(m);

            auto obs = at::from_blob(dst.data(), {11, 15, sizeOneHex}, at::kFloat);

            auto method = tjc->module.get_method("get_value");
            auto inputs = std::vector<c10::IValue>{obs};
            auto res = method(inputs).toDouble();
            return res;
        }
    };


    using ConfigStorage = std::map<std::string, std::string>;
    using ModelStorage = std::map<std::string, std::unique_ptr<TorchModel>>;

    static auto modelconfig = ConfigStorage();
    static auto models = ModelStorage();
    static std::unique_ptr<ScriptedModel> fallbackModel;
    static std::mutex modelmutex;

    static void InitModelConfigFromSettings() {
        auto lock = std::lock_guard(modelmutex);
        if (!modelconfig.empty()) return;

        auto cfg = JsonUtils::assembleFromFiles("MMAI/CONFIG/mmai-settings.json").Struct();

        for (auto &key : {"attacker", "defender", "fallback"}) {
            if(cfg[key].isString()) {
                modelconfig.insert({key, cfg[key].String()});
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
            modelconfig.insert({"attacker", baggage->modelLeft->getName()});
        if (baggage->modelRight->getType() == Schema::ModelType::TORCH_PATH)
            modelconfig.insert({"defender", baggage->modelRight->getName()});
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

                logAi->info("Loading %s model from %s", key, fullpathstr);

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

            info("Baggage decoded");
            #ifndef ENABLE_ML
            throw std::runtime_error("ENABLE_ML IS UNDEFINED, but baggage was given!");
            #endif
        } else {
            info("No baggage given");
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
        logAi->error("TEST ERROR LOG 1");
        logAi->error("TEST ERROR LOG 2: cb->getPlayerID()->hasValue(): " + std::to_string(cb->getPlayerID()->hasValue()));
        logAi->error("TEST ERROR LOG 3: baggage is nullptr? " + std::to_string(!!baggage));

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
                // For example, attacker is an injected model (being trained),
                // while defender is nullptr which stands for "load a pre-trained
                // model as usual"
                model = GetModel(side == BattleSide::ATTACKER ? "attacker" : "defender");
            }
        } else {
            InitModelConfigFromSettings();
            model = GetModel(side == BattleSide::ATTACKER ? "attacker" : "defender");
        }

        ASSERT(model, "failed to build model");

        // printf("(side=%d) hero0: %s, hero1: %s\n", EI(side), hero1->nameCustomTextId.c_str(), hero2->nameCustomTextId.c_str());

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
