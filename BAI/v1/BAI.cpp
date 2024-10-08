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
#include "lib/AI_Base.h"
#include "battle/BattleAction.h"

#include "common.h"
#include "schema/v1/types.h"
#include "BAI/v1/BAI.h"
#include "BAI/v1/supplementary_data.h"
#include "BAI/v1/action.h"
#include "BAI/v1/render.h"

namespace MMAI::BAI::V1 {
    Schema::Action BAI::getNonRenderAction() {
        // info("getNonRenderAciton called with result type: " + std::to_string(result->type));
        auto s = state.get();
        auto action = model->getAction(s);
        while (action == Schema::ACTION_RENDER_ANSI) {
            if (state->supdata->ansiRender.empty()) {
                state->supdata->ansiRender = renderANSI();
                state->supdata->type = Schema::V1::ISupplementaryData::Type::ANSI_RENDER;
            }

            // info("getNonRenderAciton (loop) called with result type: " + std::to_string(res.type));
            action = model->getAction(state.get());
        }
        state->supdata->ansiRender.clear();
        state->supdata->type = Schema::V1::ISupplementaryData::Type::REGULAR;
        return action;
    }

    std::unique_ptr<State> BAI::initState(const CPlayerBattleCallback* b) {
        return std::make_unique<State>(colorname, b);
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

    void BAI::activeStack(const BattleID &bid, const CStack *astack) {
        Base::activeStack(bid, astack);
        state->onActiveStack(astack);

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
            info("Got action: " + std::to_string(a) + " (" + state->action->name() + ")");
            auto ba = buildBattleAction();

            if (ba) {
                debug("Action is VALID: " + state->action->name());
                cb->battleMakeUnitAction(bid, *ba);
                break;
            } else {
                warn("Action is INVALID: " + state->action->name());
            }
        }
    }

    std::shared_ptr<BattleAction> BAI::buildBattleAction() {
        ASSERT(state->battlefield, "Cannot build battle action if state->battlefield is missing");
        auto action = state->action.get();
        auto bf = state->battlefield.get();
        auto astack = bf->astack;
        auto apos = astack->getPosition();

        auto [x, y] = Hex::CalcXY(apos);
        auto &hex = bf->hexes.at(y).at(x);
        std::shared_ptr<BattleAction> res = nullptr;

        // std::cout << renderANSI() << "\n";
        // XXX: if the asserts below fail, it may be due to morale
        //      (all battles must be fought on cursed ground with no morale)
        ASSERT(hex->attr(HexAttribute::STACK_QUEUE_POS) == 0, "expected 0 queue pos");
        ASSERT(hex->attr(HexAttribute::STACK_IS_ACTIVE) == 1, "expected active=1");

        if (!state->action->hex) {
            switch(NonHexAction(state->action->action)) {
            break; case NonHexAction::RETREAT:
                res = std::make_shared<BattleAction>(BattleAction::makeRetreat(battle->battleGetMySide()));
            break; case NonHexAction::WAIT:
                ASSERT(!astack->waitedThisTurn, "stack already waited this turn");
                res = std::make_shared<BattleAction>(BattleAction::makeWait(astack));
            break; default:
                THROW_FORMAT("Unexpected non-hex action: %d", state->action->action);
            }

            return res;
        }

        // With action masking, invalid actions should never occur
        // However, for manual playing/testing, it's bad to raise exceptions
        // => return errcode (Gym env will raise an exception if errcode > 0)
        auto &bhex = action->hex->bhex;
        auto &cstack = action->hex->cstack;
        auto mask = HexActMask(action->hex->attr(HexAttribute::HEX_ACTION_MASK_FOR_ACT_STACK));
        if (mask.test(EI(action->hexaction))) {
            // Action is VALID
            // XXX: Do minimal asserts to prevent bugs with nullptr deref
            //      Server will log any attempted invalid actions otherwise
            switch(action->hexaction) {
            case HexAction::MOVE: {
                auto ba = (bhex.hex == astack->getPosition().hex)
                    ? BattleAction::makeDefend(astack)
                    : BattleAction::makeMove(astack, bhex);
                res = std::make_shared<BattleAction>(ba);
            }
            break;
            case HexAction::SHOOT:
                ASSERT(cstack, "no target to shoot");
                res = std::make_shared<BattleAction>(BattleAction::makeShotAttack(astack, cstack));
            break;
            case HexAction::AMOVE_TR:
            case HexAction::AMOVE_R:
            case HexAction::AMOVE_BR:
            case HexAction::AMOVE_BL:
            case HexAction::AMOVE_L:
            case HexAction::AMOVE_TL: {
                auto &edir = AMOVE_TO_EDIR.at(action->hexaction);
                auto nbh = bhex.cloneInDirection(edir, false); // neighbouring bhex
                ASSERT(nbh.isAvailable(), "mask allowed attack to an unavailable hex #" + std::to_string(nbh.hex));
                auto estack = battle->battleGetStackByPos(nbh);
                ASSERT(estack, "no enemy stack for melee attack");
                res = std::make_shared<BattleAction>(BattleAction::makeMeleeAttack(astack, nbh, bhex));
            }
            break;
            case HexAction::AMOVE_2TR:
            case HexAction::AMOVE_2R:
            case HexAction::AMOVE_2BR:
            case HexAction::AMOVE_2BL:
            case HexAction::AMOVE_2L:
            case HexAction::AMOVE_2TL: {
                ASSERT(astack->doubleWide(), "got AMOVE_2 action for a single-hex stack");
                auto &edir = AMOVE_TO_EDIR.at(action->hexaction);
                auto obh = astack->occupiedHex(bhex);
                auto nbh = obh.cloneInDirection(edir, false); // neighbouring bhex
                ASSERT(nbh.isAvailable(), "mask allowed attack to an unavailable hex #" + std::to_string(nbh.hex));
                auto estack = battle->battleGetStackByPos(nbh);
                ASSERT(estack, "no enemy stack for melee attack");
                res = std::make_shared<BattleAction>(BattleAction::makeMeleeAttack(astack, nbh, bhex));
            }
            break;
            default:
                THROW_FORMAT("Unexpected hexaction: %d", EI(action->hexaction));
            }

            return res;
        }

        // Action is INVALID

        // XXX:
        // mask prevents certain actions, but during TESTING
        // those actions may be taken anyway.
        //
        // IF we are here, it means the mask disallows that action
        //
        // => *throw* errors here only if the mask SHOULD HAVE ALLOWED it
        //    and *set* regular, non-throw errors otherwise
        //
        auto rinfo = battle->getReachability(astack);
        auto ainfo = battle->getAccessibility();

        switch(state->action->hexaction) {
            case HexAction::AMOVE_TR:
            case HexAction::AMOVE_R:
            case HexAction::AMOVE_BR:
            case HexAction::AMOVE_BL:
            case HexAction::AMOVE_L:
            case HexAction::AMOVE_TL:
            case HexAction::AMOVE_2TR:
            case HexAction::AMOVE_2R:
            case HexAction::AMOVE_2BR:
            case HexAction::AMOVE_2BL:
            case HexAction::AMOVE_2L:
            case HexAction::AMOVE_2TL:
            case HexAction::MOVE: {
                auto a = ainfo.at(action->hex->bhex);
                if (a == EAccessibility::OBSTACLE) {
                    auto hs = hex->getState();
                    ASSERT(hs == HexState::OBSTACLE, "incorrect hex state -- expected OBSTACLE, got: " + std::to_string(EI(hs)) + debugInfo(action, astack, nullptr));
                    state->supdata->errcode = ErrorCode::HEX_BLOCKED;
                    error("Action error: HEX_BLOCKED");
                    break;
                } else if (a == EAccessibility::ALIVE_STACK) {
                    auto bh = action->hex->bhex;
                    if (bh.hex == astack->getPosition().hex) {
                        // means we want to defend (moving to self)
                        // or attack from same hex we're currently at
                        // this should always be allowed
                        ASSERT(false, "mask prevented (A)MOVE to own hex" + debugInfo(action, astack, nullptr));
                    } else if (bh.hex == astack->occupiedHex().hex) {
                        ASSERT(rinfo.distances.at(bh) == ReachabilityInfo::INFINITE_DIST, "mask prevented (A)MOVE to self-occupied hex" + debugInfo(action, astack, nullptr));
                        // means we can't fit on our own back hex
                    }

                    // means we try to move onto another stack
                    state->supdata->errcode = ErrorCode::HEX_BLOCKED;
                    error("Action error: HEX_BLOCKED");
                    break;
                }

                // only remaining is ACCESSIBLE
                expect(a == EAccessibility::ACCESSIBLE, "accessibility should've been ACCESSIBLE, was: %d", a);

                auto nbh = BattleHex{};

                if (action->hexaction < HexAction::AMOVE_2TR) {
                    auto edir = AMOVE_TO_EDIR.at(action->hexaction);
                    nbh = bhex.cloneInDirection(edir, false);
                } else {
                    if (!astack->doubleWide()) {
                        state->supdata->errcode = ErrorCode::INVALID_DIR;
                        error("Action error: INVALID_DIR");
                        break;
                    }

                    auto edir = AMOVE_TO_EDIR.at(action->hexaction);
                    nbh = astack->occupiedHex().cloneInDirection(edir, false);
                }

                if (!nbh.isAvailable()) {
                    state->supdata->errcode = ErrorCode::HEX_MELEE_NA;
                    error("Action error: HEX_MELEE_NA");
                    break;
                }

                auto estack = battle->battleGetStackByPos(nbh);

                if (!estack) {
                    state->supdata->errcode = ErrorCode::STACK_NA;
                    error("Action error: STACK_NA");
                    break;
                }

                if (estack->unitSide() == astack->unitSide()) {
                    state->supdata->errcode = ErrorCode::FRIENDLY_FIRE;
                    error("Action error: FRIENDLY_FIRE");
                    break;
                }
            }
            break;
            case HexAction::SHOOT:
                if (!cstack) {
                    state->supdata->errcode = ErrorCode::STACK_NA;
                    error("Action error: STACK_NA");
                    break;
                } else if (cstack->unitSide() == astack->unitSide()) {
                    state->supdata->errcode = ErrorCode::FRIENDLY_FIRE;
                    error("Action error: FRIENDLY_FIRE");
                    break;
                } else {
                    ASSERT(!battle->battleCanShoot(astack, bhex), "mask prevented SHOOT at a shootable bhex " + action->hex->name());
                    state->supdata->errcode = ErrorCode::CANNOT_SHOOT;
                    error("Action error: CANNOT_SHOOT");
                    break;
                }
            break;
            default:
                THROW_FORMAT("Unexpected hexaction: %d", EI(action->hexaction));
            }

        ASSERT(state->supdata->errcode != ErrorCode::OK, "Could not identify why the action is invalid" + debugInfo(action, astack, nullptr));

        return res;
    }

    std::string BAI::debugInfo(Action *action, const CStack* astack, BattleHex* nbh) {
        auto info = std::stringstream();
        info << "\n*** DEBUG INFO ***\n";
        info << "action: " << action->name() << " [" << action->action << "]\n";
        info << "action->hex->bhex.hex = " << action->hex->bhex.hex << "\n";

        auto ainfo = battle->getAccessibility();
        auto rinfo = battle->getReachability(astack);

        info << "ainfo[bhex]=" << EI(ainfo.at(action->hex->bhex.hex)) << "\n";
        info << "rinfo.distances[bhex] <= astack->getMovementRange(): " << (rinfo.distances[action->hex->bhex.hex] <= astack->getMovementRange()) << "\n";

        info << "action->hex->name = " << action->hex->name() << "\n";

        for (int i=0; i < action->hex->attrs.size(); i++)
            info << "action->hex->attrs[" << i << "] = " << EI(action->hex->attrs[i]) << "\n";

        info << "action->hex->hexactmask = ";
        info << HexActMask(action->hex->attr(HexAttribute::HEX_ACTION_MASK_FOR_ACT_STACK)).to_string();
        info << "\n";

        auto cstack = action->hex->cstack;
        if (cstack) {
            info << "cstack->getPosition().hex=" << cstack->getPosition().hex << "\n";
            info << "cstack->slot=" << cstack->unitSlot() << "\n";
            info << "cstack->doubleWide=" << cstack->doubleWide() << "\n";
            info << "cb->battleCanShoot(cstack)=" << battle->battleCanShoot(cstack) << "\n";
        } else {
            info << "cstack: (nullptr)\n";
        }

        info << "astack->getPosition().hex=" << astack->getPosition().hex << "\n";
        info << "astack->slot=" << astack->unitSlot() << "\n";
        info << "astack->doubleWide=" << astack->doubleWide() << "\n";
        info << "cb->battleCanShoot(astack)=" << battle->battleCanShoot(astack) << "\n";

        if (nbh) {
            info << "nbh->hex=" << nbh->hex << "\n";
            info << "ainfo[nbh]=" << EI(ainfo.at(*nbh)) << "\n";
            info << "rinfo.distances[nbh] <= astack->getMovementRange(): " << (rinfo.distances[*nbh] <= astack->getMovementRange()) << "\n";

            if (cstack)
                info << "astack->isMeleeAttackPossible(...)=" << astack->isMeleeAttackPossible(astack, cstack, *nbh) << "\n";
        }

        info << "\nACTION TRACE:\n";
        for (const auto& a : allactions)
            info << a << ",";

        info << "\nRENDER:\n";
        info << renderANSI();

        return info.str();
    }

    std::string BAI::renderANSI() {
        Verify(state.get());
        return Render(state.get(), state->action.get());
    }
}
