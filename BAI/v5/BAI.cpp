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

#include "battle/AccessibilityInfo.h"
#include "battle/BattleAction.h"
#include "battle/CBattleInfoEssentials.h"
#include "lib/AI_Base.h"

#include "BAI/v5/BAI.h"
#include "BAI/v5/action.h"
#include "BAI/v5/render.h"
#include "BAI/v5/supplementary_data.h"
#include "common.h"
#include "schema/v5/constants.h"
#include "schema/v5/types.h"
#include <stdexcept>

namespace MMAI::BAI::V5 {
    Schema::Action BAI::getNonRenderAction() {
        // info("getNonRenderAciton called with result type: " + std::to_string(result->type));
        auto s = state.get();
        auto action = model->getAction(s);
        debug("Got action: " + std::to_string(action));
        while (action == Schema::ACTION_RENDER_ANSI) {
            if (state->supdata->ansiRender.empty()) {
                state->supdata->ansiRender = renderANSI();
                state->supdata->type = Schema::V5::ISupplementaryData::Type::ANSI_RENDER;
            }

            // info("getNonRenderAciton (loop) called with result type: " + std::to_string(res.type));
            action = model->getAction(state.get());
        }
        state->supdata->ansiRender.clear();
        state->supdata->type = Schema::V5::ISupplementaryData::Type::REGULAR;
        return action;
    }

    std::unique_ptr<State> BAI::initState(const CPlayerBattleCallback* b) {
        return std::make_unique<State>(version, colorname, b);
    }

    void BAI::battleStart(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) {
        Base::battleStart(bid, army1, army2, tile, hero1, hero2, side, replayAllowed);
        battle = cb->getBattle(bid);
        state = initState(battle.get());
    }

    // XXX: battleEnd() is NOT called by CPlayerInterface (i.e. GUI)
    //      However, it's called by AAI (i.e. headless) and that's all we want
    //      since the terminal result is needed only during training.
    void BAI::battleEnd(const BattleID &bid, const BattleResult *br, QueryID queryID) {
        Base::battleEnd(bid, br, queryID);
        state->onBattleEnd(br);

        // Check if battle ended normally or was forced via a RETREAT action
        if (state->action == nullptr) {
            // no previous action means battle ended giving us a turn (OK)
            // Happens if the enemy immediately retreats (we won)
            // or if the enemy one-shots us (we lost)
            info("Battle ended without giving us a turn: nothing to do");
        } else if (state->action->action == Schema::ACTION_RETREAT) {
            if (resetting) {
                // this is an intended restart (i.e. converted ACTION_RESTART)
                info("Battle ended due to ACTION_RESET: nothing to do");
            } else {
                // this is real retreat
                info("Battle ended due to ACTION_RETREAT: reporting terminal state, expecting ACTION_RESET");
                auto a = getNonRenderAction();
                ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
            }
        } else {
            info("Battle ended normally: reporting terminal state, expecting ACTION_RESET");
            auto a = getNonRenderAction();
            ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
        }

        // BAI is destroyed after this call
        debug("Leaving battleEnd, embracing death");
    }

    void BAI::battleStacksAttacked(const BattleID &bid, const std::vector<BattleStackAttacked> &bsa, bool ranged) {
        Base::battleStacksAttacked(bid, bsa, ranged);
        state->onBattleStacksAttacked(bsa);
    }

    void BAI::battleTriggerEffect(const BattleID &bid, const BattleTriggerEffect &bte) {
        Base::battleTriggerEffect(bid, bte);
        state->onBattleTriggerEffect(bte);
    }

    void BAI::yourTacticPhase(const BattleID &bid, int distance) {
        Base::yourTacticPhase(bid, distance);
        cb->battleMakeTacticAction(bid, BattleAction::makeEndOFTacticPhase(battle->battleGetTacticsSide()));
    }

    std::shared_ptr<BattleAction> BAI::maybeBuildAutoAction(const CStack *astack) {
        if (astack->creatureId() == CreatureID::FIRST_AID_TENT) {
            const CStack* target = nullptr;
            auto maxdmg = 0;
            for (auto stack : battle->battleGetStacks(CBattleInfoEssentials::ONLY_MINE)) {
                auto dmg = stack->getMaxHealth() - stack->getFirstHPleft();
                if(dmg <= maxdmg) continue;
                maxdmg = dmg;
                target = stack;
            }

            if (target) {
                return std::make_shared<BattleAction>(BattleAction::makeHeal(astack, target));
            }
        } else if (astack->creatureId() == CreatureID::CATAPULT) {
            if (!astack->canShoot())
                // out of ammo
                return std::make_shared<BattleAction>(BattleAction::makeDefend(astack));  // out of ammo (arrow towers have 99 shots)

            auto ba = std::make_shared<BattleAction>();
            ba->side = astack->unitSide();
            ba->stackNumber = astack->unitId();
            ba->actionType = EActionType::CATAPULT;

            if(battle->battleGetGateState() == EGateState::CLOSED) {
                ba->aimToHex(battle->wallPartToBattleHex(EWallPart::GATE));
                return ba;
            }

            using WP = EWallPart;
            auto wallparts = {
                WP::KEEP, WP::BOTTOM_TOWER, WP::UPPER_TOWER,
                WP::BELOW_GATE, WP::OVER_GATE, WP::BOTTOM_WALL, WP::UPPER_WALL
            };

            for (auto wp : wallparts) {
                using WS = EWallState;
                auto ws = battle->battleGetWallState(wp);
                if(ws == WS::REINFORCED || ws == WS::INTACT || ws == WS::DAMAGED) {
                    ba->aimToHex(battle->wallPartToBattleHex(wp));
                    return ba;
                }
            }

            // no walls left
            return std::make_shared<BattleAction>(BattleAction::makeDefend(astack));  // out of ammo (arrow towers have 99 shots)
        } else if (astack->creatureId() == CreatureID::ARROW_TOWERS) {
            if (!astack->canShoot())
                // out of ammo (arrow towers have 99 shots)
                return std::make_shared<BattleAction>(BattleAction::makeDefend(astack));

            auto allstacks = battle->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY);
            auto target = std::max_element(allstacks.begin(), allstacks.end(), [](const CStack* a, const CStack* b) {
                return a->unitType()->getAIValue() < b->unitType()->getAIValue();
            });

            ASSERT(target != allstacks.end(), "Could not find an enemy stack to attack. Falling back to a defend.");
            return std::make_shared<BattleAction>(BattleAction::makeShotAttack(astack, *target));
        }

        return nullptr;
    }

    void BAI::activeStack(const BattleID &bid, const CStack *astack) {
        Base::activeStack(bid, astack);

        auto ba = maybeBuildAutoAction(astack);

        if (ba) {
            info("Making automatic action with stack %s", astack->getDescription());
            cb->battleMakeUnitAction(bid, *ba);
            return;
        }

        state->onActiveStack(astack);

        if (state->battlefield->astack == nullptr) {
            warn("The current stack is not part of the state. This can happen "
                    "if there are more than %d alive stacks in the army. "
                    "Falling back to a wait/defend action.", MAX_STACKS_PER_SIDE);
            auto fa = astack->waitedThisTurn ? BattleAction::makeDefend(astack) : BattleAction::makeWait(astack);
            cb->battleMakeUnitAction(bid, fa);
            return;
        }

        while(true) {
            auto a = getNonRenderAction();
            allactions.push_back(a);

            if (a == Schema::ACTION_RESET) {
                // XXX: retreat is always allowed for ML, limited by action mask only
                info("Received ACTION_RESET, converting to ACTION_RETREAT in order to reset battle");
                a = Schema::ACTION_RETREAT;
                resetting = true;
            }

            state->action = std::make_unique<Action>(a, state->battlefield.get(), colorname);
            info("Got action: %d (%s)", a, state->action->name());

            try {
                auto ba = buildBattleAction();

                if (ba) {
                    debug("Action is VALID: " + state->action->name());
                    cb->battleMakeUnitAction(bid, *ba);
                    break;
                } else {
                    warn("Action is INVALID: " + state->action->name());
                }
            } catch (const std::exception& e) {
                std::cout << Render(state.get(), state->action.get()) << "\n";
                std::cout << "FATAL ERROR: " << e.what() << "\n";
                throw;
            }
        }
    }

    std::shared_ptr<BattleAction> BAI::buildBattleAction() {
        ASSERT(state->battlefield, "Cannot build battle action if state->battlefield is missing");
        auto action = state->action.get();
        auto bf = state->battlefield.get();
        auto acstack = bf->astack->cstack;
        auto mask = state->actmask;

        // auto [x, y] = Hex::CalcXY(acstack->getPosition());
        // auto &hex = bf->hexes->at(y).at(x);
        std::shared_ptr<BattleAction> res = nullptr;

        ASSERT(action, "action is nullptr");

        auto pa = action->primaryAction;
        if (!state->actmask.at(EI(pa))) {
            // XXX: A shooter attempting to melee results in a confusing message:
            //      "Shoot at #0: PrimaryAction 3 not allowed by mask"
            //      This is because ATTACK_0 is interpreted as a ranged attack
            error(action->name() + ": PrimaryAction " + std::to_string(EI(pa)) + " not allowed by mask");
            state->supdata->errcode = ErrorCode::INVALID;
            return nullptr;
        }

        auto ama = Hex::PrimaryToAMoveAction(action->primaryAction);
        if (action->hex && !(action->hex->amovemask.test(EI(ama)))) {
            error(action->name() + ": AMoveAction " + std::to_string(EI(ama)) + " not allowed by mask");
            state->supdata->errcode = ErrorCode::INVALID;
            return nullptr;
        }

        switch(action->primaryAction) {
        case PrimaryAction::RETREAT:
            return std::make_shared<BattleAction>(BattleAction::makeRetreat(battle->battleGetMySide()));
        case PrimaryAction::WAIT:
            return std::make_shared<BattleAction>(BattleAction::makeWait(acstack));
        case PrimaryAction::MOVE:
            return std::make_shared<BattleAction>(action->isDefend
                ? BattleAction::makeDefend(acstack)
                : BattleAction::makeMove(acstack, action->hex->bhex)
            );
        case PrimaryAction::ATTACK_0:
        case PrimaryAction::ATTACK_1:
        case PrimaryAction::ATTACK_2:
        case PrimaryAction::ATTACK_3:
        case PrimaryAction::ATTACK_4:
        case PrimaryAction::ATTACK_5:
        case PrimaryAction::ATTACK_6:
        case PrimaryAction::ATTACK_7:
        case PrimaryAction::ATTACK_8:
        case PrimaryAction::ATTACK_9: {
            if (battle->battleCanShoot(acstack)) {
                // shooting => target hex is not needed
                ASSERT(!action->hex, action->name() + ", but astack can shoot");
                return std::make_shared<BattleAction>(BattleAction::makeShotAttack(acstack, action->stack->cstack));
            }

            ASSERT(action->hex, action->name() + ", but astack can only melee");
            auto id = EI(action->primaryAction) - EI(PrimaryAction::ATTACK_0);
            auto aside = bf->astack->attr(StackAttribute::SIDE);
            auto estack = bf->stacks->at(!aside).at(id);
            ASSERT(estack, action->name() + ", but no enemy stack with id=" + std::to_string(id));

            auto hextargets = CStack::meleeAttackHexes(acstack, estack->cstack, action->hex->bhex);
            ASSERT(!hextargets.empty(), action->name() + ", but melee attack is impossible against stack with id=" + std::to_string(id));

            return std::make_shared<BattleAction>(
                BattleAction::makeMeleeAttack(acstack, hextargets.at(0), action->hex->bhex)
            );
        }
        default:
            THROW_FORMAT("Unexpected primary action: %d", EI(state->action->primaryAction));
        }

        // should never be here
        throw std::runtime_error("Internal action error");
    }

    std::string BAI::renderANSI() {
        Verify(state.get());
        return Render(state.get(), state->action.get());
    }
}
