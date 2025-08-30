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

#include "networkPacks/PacksForClientBattle.h"
#include "networkPacks/SetStackEffect.h"
#include "spells/CSpellHandler.h"

#include "BAI/v3/BAI.h"
#include "BAI/v8/BAI.h"
#include "BAI/v9/BAI.h"
#include "BAI/v10/BAI.h"
#include "BAI/v11/BAI.h"
#include "BAI/v12/BAI.h"
#include "BAI/v13/BAI.h"
#include "base.h"

namespace MMAI::BAI {
    // static
    std::shared_ptr<Base> Base::Create(
        Schema::IModel* model,
        const std::shared_ptr<Environment> env,
        const std::shared_ptr<CBattleCallback> cb
    ) {
        std::shared_ptr<Base> res;
        auto version = model->getVersion();

        switch (version) {
        break; case 3:
            res = std::make_shared<V3::BAI>(model, version, env, cb);
        break; case 8:
            res = std::make_shared<V8::BAI>(model, version, env, cb);
        break; case 9:
            res = std::make_shared<V9::BAI>(model, version, env, cb);
        break; case 10:
            res = std::make_shared<V10::BAI>(model, version, env, cb);
        break; case 11:
            res = std::make_shared<V11::BAI>(model, version, env, cb);
        break; case 12:
            res = std::make_shared<V12::BAI>(model, version, env, cb);
        break; case 13:
            res = std::make_shared<V13::BAI>(model, version, env, cb);
        break; default:
            throw std::runtime_error("Unsupported schema version: " + std::to_string(version));
        }

        res->init();
        return res;
    }

    Base::Base(
        Schema::IModel* model,
        const int version,
        const std::shared_ptr<Environment> env,
        const std::shared_ptr<CBattleCallback> cb
    ) : model(model)
      , version(version)
      , env(env)
      , cb(cb)
      , name("BAI-v" + std::to_string(version))
      , colorname(cb->getPlayerID()->toString())
    {
        std::ostringstream oss;
        oss << this; // Store this memory address
        addrstr = oss.str();

        const char* envvar = std::getenv("MMAI_VERBOSE");
        verbose = envvar != nullptr && strcmp(envvar, "1") == 0;
    }

    /*
     * These methods MUST be overridden by derived BAI (e.g. BAI::V1)
     * Their base implementation is is for logging purposes only.
     */

    void Base::activeStack(const BattleID &bid, const CStack *astack) {
        debug("*** activeStack ***");
        trace("activeStack called for " + astack->nodeName());
    };

    void Base::yourTacticPhase(const BattleID &bid, int distance) {
        debug("*** yourTacticPhase ***");
    };

    /*
     * These methods MAY be overriden by derived BAI (e.g. BAI::V1)
     * Their implementation here is a no-op.
     */

    void Base::init() {
        debug("*** init ***");
    }

    void Base::actionFinished(const BattleID &bid, const BattleAction &action) {
        debug("*** actionFinished ***");
    }

    void Base::actionStarted(const BattleID &bid, const BattleAction &action) {
        debug("*** actionStarted ***");
    }

    void Base::battleAttack(const BattleID &bid, const BattleAttack *ba) {
        debug("*** battleAttack ***");
    }

    void Base::battleCatapultAttacked(const BattleID &bid, const CatapultAttack & ca) {
        debug("*** battleCatapultAttacked ***");
    }

    void Base::battleEnd(const BattleID &bid, const BattleResult *br, QueryID queryID) {
        debug("*** battleEnd ***");
    }

    void Base::battleGateStateChanged(const BattleID &bid, const EGateState state) {
        debug("*** battleGateStateChanged ***");
        trace("New gate state: %d", EI(state));
    }

    void Base::battleLogMessage(const BattleID &bid, const std::vector<MetaString> &lines) {
        debug("*** battleLogMessage ***");
        if (verbose) {
            std::string res = "Messages:";
            for(const auto & line : lines) {
                std::string formatted = line.toString();
                boost::algorithm::trim(formatted);
                res = res + "\n\t* " + formatted;
            }
            std::cout << "MMAI_VERBOSE: " << res << "\n";
        }
    }

    void Base::battleNewRound(const BattleID &bid) {
        debug("*** battleNewRound ***");
    }

    void Base::battleNewRoundFirst(const BattleID &bid) {
        debug("*** battleNewRoundFirst ***");
    }

    void Base::battleObstaclesChanged(const BattleID &bid, const std::vector<ObstacleChanges> &obstacles) {
        debug("*** battleObstaclesChanged ***");
    }

    void Base::battleSpellCast(const BattleID &bid, const BattleSpellCast *sc) {
        debug("*** battleSpellCast ***");
        if (verbose) {
            std::string res = "Spellcast info:";
            auto battle = cb->getBattle(bid);
            auto caster = battle->battleGetStackByID(sc->casterStack, false);

            res += "\n\t* spell: " + sc->spellID.toSpell()->identifier;
            res += "\n\t* castByHero=" + std::to_string(sc->castByHero);
            res += "\n\t* casterStack=" + (caster ? caster->getDescription() : "");
            res += "\n\t* activeCast=" + std::to_string(sc->activeCast);
            res += "\n\t* side=" + std::to_string(EI(sc->side));
            res += "\n\t* tile=" + std::to_string(sc->tile.toInt());

            res += "\n\t* affected:";
            for (auto &cid : sc->affectedCres)
                res += "\n\t  > " + battle->battleGetStackByID(cid, false)->getDescription();

            res += "\n\t* resisted:";
            for (auto &cid : sc->resistedCres)
                res += "\n\t  > " + battle->battleGetStackByID(cid, false)->getDescription();

            res += "\n\t* reflected:";
            for (auto &cid : sc->reflectedCres)
                res += "\n\t  > " + battle->battleGetStackByID(cid, false)->getDescription();

            std::cout << "MMAI_VERBOSE: " << res << "\n";
        }
    }

    void Base::battleStackMoved(const BattleID &bid, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport) {
        debug("*** battleStackMoved ***");
        if (verbose) {
            auto battle = cb->getBattle(bid);
            std::string fmt = "Movement info:";

            fmt += "\n\t* stack description=%s";
            fmt += "\n\t* stack owner=%s";
            fmt += "\n\t* dest[0]=%d (Hex#%d, y=%d, x=%d)";
            fmt += "\n\t* distance=%d";
            fmt += "\n\t* teleport=%d";

            auto bh0 = dest.at(dest.size() - 1);
            auto hexid0 = bh0.getX()-1 + bh0.getY()*BF_XMAX;
            auto x0 = bh0.getX() - 1;
            auto y0 = bh0.getY();

            auto res = boost::format(fmt)
                % stack->getDescription()
                % stack->getOwner().toString()
                % bh0 % hexid0 % y0 % x0
                % distance
                % teleport;

            std::cout << "MMAI_VERBOSE: " << boost::str(res) << "\n";
        }
    }

    void Base::battleStacksAttacked(const BattleID &bid, const std::vector<BattleStackAttacked> &bsa, bool ranged) {
        debug("*** battleStacksAttacked ***");
    }

    void Base::battleStacksEffectsSet(const BattleID &bid, const SetStackEffect & sse) {
        debug("*** battleStacksEffectsSet ***");
        if (verbose) {
            auto battle = cb->getBattle(bid);

            std::string res = "Effects set:";

            for (auto &[unitid, bonuses] : sse.toAdd) {
                auto cstack = battle->battleGetStackByID(unitid);
                res += "\n\t* stack=" + (cstack ? cstack->getDescription() : "");
                for (auto &bonus : bonuses) {
                    res += "\n\t  > add bonus=" + bonus.description.toString();
                }
            }

            for (auto &[unitid, bonuses] : sse.toRemove) {
                auto cstack = battle->battleGetStackByID(unitid);
                res += "\n\t* stack=" + (cstack ? cstack->getDescription() : "");
                for (auto &bonus : bonuses) {
                    res += "\n\t  > remove bonus=" + bonus.description.toString();
                }
            }

            for (auto &[unitid, bonuses] : sse.toUpdate) {
                auto cstack = battle->battleGetStackByID(unitid);
                res += "\n\t* stack=" + (cstack ? cstack->getDescription() : "");
                for (auto &bonus : bonuses) {
                    res += "\n\t  > update bonus=" + bonus.description.toString();
                }
            }

            std::cout << "MMAI_VERBOSE: " << res << "\n";
        };
    }

    void Base::battleStart(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) {
        debug("*** battleStart ***");
    }

    // XXX: positive morale triggers an effect
    //      negative morale just skips turn
    void Base::battleTriggerEffect(const BattleID &bid, const BattleTriggerEffect & bte) {
        debug("*** battleTriggerEffect ***");
        if (verbose) {
            auto battle = cb->getBattle(bid);
            auto cstack = battle->battleGetStackByID(bte.stackID);
            std::string res = "Effect triggered:";
            res += "\n\t* bonus id=" + std::to_string(bte.effect);
            res += "\n\t* bonus value=" + std::to_string(bte.val);
            res += "\n\t* stack=" + (cstack ? cstack->getDescription() : "");
            std::cout << "MMAI_VERBOSE: " << res << "\n";
        }
    }

    void Base::battleUnitsChanged(const BattleID &bid, const std::vector<UnitChanges> &changes) {
        debug("*** battleUnitsChanged ***");
        if (verbose) {
            std::string res = "Changes:";
            for(const auto & change : changes) {
                res += "\n\t* operation=" + std::to_string(EI(change.operation));
                res += "\n\t* healthDelta=" + std::to_string(change.healthDelta);
            }
            std::cout << "MMAI_VERBOSE: " << res << "\n";
        }
    }
}
