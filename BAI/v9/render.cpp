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

#include "battle/AccessibilityInfo.h"
#include "battle/BattleAttackInfo.h"
#include "battle/CObstacleInstance.h"
#include "battle/IBattleInfoCallback.h"
#include "constants/EntityIdentifiers.h"
#include "mapObjects/CGTownInstance.h"
#include "vcmi/spells/Caster.h"

#include "BAI/v9/hex.h"
#include "BAI/v9/hexactmask.h"
#include "BAI/v9/render.h"
#include "schema/v9/constants.h"
#include "schema/v9/types.h"

namespace MMAI::BAI::V9 {
    using SA = StackAttribute;
    using SF = StackFlag;
    using LT = LinkType;

    std::string PadLeft(const std::string& input, size_t desiredLength, char paddingChar) {
        std::ostringstream ss;
        ss << std::right << std::setfill(paddingChar) << std::setw(desiredLength) << input;
        return ss.str();
    }

    std::string PadRight(const std::string& input, size_t desiredLength, char paddingChar) {
        std::ostringstream ss;
        ss << std::left << std::setfill(paddingChar) << std::setw(desiredLength) << input;
        return ss.str();
    }

    void Verify(const State* state) {
        auto battle = state->battle;
        auto hexes = Hexes();

        expect(battle, "no battle to verify");

        const CStack* astack = nullptr; // XXX: can remain nullptr (for terminal obs)
        auto l_CStacks = std::array<const CStack*, 7>{};
        auto r_CStacks = std::array<const CStack*, 7>{};
        auto l_CStacksSummons = std::vector<const CStack*>{};
        auto r_CStacksSummons = std::vector<const CStack*>{};
        auto l_CStacksMachines = std::vector<const CStack*>{};
        auto r_CStacksMachines = std::vector<const CStack*>{};
        auto hexstacks = std::array<const CStack*, 165> {};

        auto rinfos = std::map<const CStack*, ReachabilityInfo>{};
        auto allstacks = battle->battleGetStacks();
        std::sort(allstacks.begin(), allstacks.end(), [&](const CStack* a, const CStack* b) {
            return a->unitId() < b->unitId();
        });

        for (auto &cstack : allstacks) {
            if (cstack->unitId() == battle->battleActiveUnit()->unitId())
                astack = cstack;

            if (cstack->unitSlot() < 0) {
                if (cstack->unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER)
                    cstack->unitSide() == BattleSide::DEFENDER
                        ? r_CStacksSummons.push_back(cstack)
                        : l_CStacksSummons.push_back(cstack);
                else if (cstack->unitSlot() == SlotID::WAR_MACHINES_SLOT)
                    cstack->unitSide() == BattleSide::DEFENDER
                        ? r_CStacksMachines.push_back(cstack)
                        : l_CStacksMachines.push_back(cstack);
            } else {
                cstack->unitSide() == BattleSide::DEFENDER
                    ? r_CStacks.at(cstack->unitSlot()) = cstack
                    : l_CStacks.at(cstack->unitSlot()) = cstack;
            }

            rinfos.insert({cstack, battle->getReachability(cstack)});

            for (auto &bh : cstack->getHexes()) {
                if (!bh.isAvailable()) continue; // war machines rear hex, arrow towers
                expect(!hexstacks.at(Hex::CalcId(bh)), "hex occupied by multiple stacks?");
                hexstacks.at(Hex::CalcId(bh)) = cstack;
            }
        }

        auto l_CStacksAll = std::vector<const CStack*> {};
        l_CStacksAll.insert(l_CStacksAll.end(), l_CStacks.begin(), l_CStacks.end());
        l_CStacksAll.insert(l_CStacksAll.end(), l_CStacksSummons.begin(), l_CStacksSummons.end());
        l_CStacksAll.insert(l_CStacksAll.end(), l_CStacksMachines.begin(), l_CStacksMachines.end());

        auto r_CStacksAll = std::vector<const CStack*> {};
        r_CStacksAll.insert(r_CStacksAll.end(), r_CStacks.begin(), r_CStacks.end());
        r_CStacksAll.insert(r_CStacksAll.end(), r_CStacksSummons.begin(), r_CStacksSummons.end());
        r_CStacksAll.insert(r_CStacksAll.end(), r_CStacksMachines.begin(), r_CStacksMachines.end());

        auto l_CStacksExtra = std::vector<const CStack*>{};
        l_CStacksExtra.insert(l_CStacksExtra.end(), l_CStacksSummons.begin(), l_CStacksSummons.end());
        l_CStacksExtra.insert(l_CStacksExtra.end(), l_CStacksMachines.begin(), l_CStacksMachines.end());

        auto r_CStacksExtra = std::vector<const CStack*>{};
        r_CStacksExtra.insert(r_CStacksExtra.end(), r_CStacksSummons.begin(), r_CStacksSummons.end());
        r_CStacksExtra.insert(r_CStacksExtra.end(), r_CStacksMachines.begin(), r_CStacksMachines.end());

        auto getAllStacksForSide = [&](bool side) {
            return side ? r_CStacksAll : l_CStacksAll;
        };

        auto ended = state->supdata->ended;

        if (!astack)
            expect(ended, "astack is NULL, but ended is not true");
        else if (ended) {
            // at battle-end, activeStack is usually the ENEMY stack
            // XXX: this expect will incorrectly throw if we retreated as a regular action
            //      (in which case our stack will be active, but we would have lost the battle)
            // expect(state->supdata->victory == (astack->getOwner() == battle->battleGetMySide()), "state->supdata->victory is %d, but astack->side=%d and myside=%d", state->supdata->victory, astack->getOwner(), battle->battleGetMySide());

            // at battle-end, even regardless of the actual active stack,
            // battlefield->astack must be nullptr
            expect(!state->battlefield->astack, "ended, but battlefield->astack is not NULL");
            expect(state->supdata->getIsBattleEnded(), "ended, but state->supdata->getIsBattleEnded() is false");
        }

        // Return (attr == N/A), but after performing some checks
        auto isNA = [](int v, const CStack* stack, const char* attrname) {
            if (v == NULL_VALUE_UNENCODED) {
                expect(!stack, "%s: N/A but stack != nullptr", attrname);
                return true;
            }
            expect(stack, "%s: != N/A but stack = nullptr", attrname);
            return false;
        };

        auto checkReachable = [=](BattleHex bh, bool v, const CStack* stack) {
            auto distance = rinfos.at(stack).distances.at(bh.toInt());
            auto canreach = (stack->getMovementRange() >= distance);

            // XXX: if v=false, returns true when UNreachable
            //      if v=true returns true when reachable
            return v ? canreach : !canreach;
        };

        auto ensureReachability = [=](BattleHex bh, bool v, const CStack* stack, const char* attrname) {
            expect(checkReachable(bh, v, stack), "%s: (bhex=%d) reachability expected: %d", attrname, bh.toInt(), v);
        };

        auto ensureValueMatch = [](int have, int want, const char* attrname) {
            expect(have == want, "%s: have: %d, want: %d", attrname, have, want);
        };

        auto ensureFloatMatch = [](float have, float want, const char* attrname) {
            auto eps = 1e-6;
            expect(std::abs(have - want) < eps, "%s: have: %.6f, want: %.6f (eps=%.6f)", attrname, have, want, eps);
        };

        auto ensureStackNullOrMatch = [=](HexAttribute a, const CStack* cstack, int have, std::function<int()> wantfunc, const char* attrname) {
            auto vmax = std::get<3>(HEX_ENCODING.at(EI(a)));
            if (isNA(have, cstack, attrname))
                return;
            auto want = wantfunc();
            if (want > vmax) want = vmax;
            ensureValueMatch(have, want, attrname);
        };

        auto ensureMeleeability = [=](BattleHex bh, HexActMask mask, HexAction ha, const CStack* cstack, const char* attrname) {
            auto mv = mask.test(EI(ha));

            // if AMOVE is allowed, we must be able to reach hex
            // (no else -- we may still be able to reach it)
            if (mv == 1)
                ensureReachability(bh, 1, cstack, attrname);

            auto nbh = BattleHex{};

            switch (ha) {
            break; case HexAction::AMOVE_TR: nbh = bh.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false);
            break; case HexAction::AMOVE_R: nbh = bh.cloneInDirection(BattleHex::EDir::RIGHT, false);
            break; case HexAction::AMOVE_BR: nbh = bh.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false);
            break; case HexAction::AMOVE_BL: nbh = bh.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false);
            break; case HexAction::AMOVE_L: nbh = bh.cloneInDirection(BattleHex::EDir::LEFT, false);
            break; case HexAction::AMOVE_TL: nbh = bh.cloneInDirection(BattleHex::EDir::TOP_LEFT, false);
            break; default:
                THROW_FORMAT("Unexpected HexAction: %d", EI(ha));
              break;
            }

            auto estacks = getAllStacksForSide(!EI(cstack->unitSide()));
            auto it = std::find_if(estacks.begin(), estacks.end(), [&nbh](auto stack) {
                return stack && stack->coversPos(nbh);
            });

            auto estack = it == estacks.end() ? nullptr : *it;

            if (mv) {
                expect(estack, "%s: =1 (bhex %d, nbhex %d), but estack is nullptr", attrname, bh.toInt(), nbh.toInt());
                // must not pass "nbh" for defender position, as it could be its rear hex
                expect(cstack->isMeleeAttackPossible(cstack, estack, bh), "%s: =1 (bhex %d, nbhex %d), but VCMI says isMeleeAttackPossible=0", attrname, bh.toInt(), nbh.toInt());
            }

            //  else {
            //     if (it != estacks.end()) {
            //         auto estack = *it;
            //         // MASK may prohibit attack, but hex may still be reachable
            //         if (checkReachable(bh, 1, cstack))
            //             // MASK may prohibita this specific attack from a reachable hex
            //             // it does not mean any attack is impossible
            //             expect(!cstack->isMeleeAttackPossible(cstack, estack, bh), "%s: =0 (bhex %d, nbhex %d), but bhex is reachable and VCMI says isMeleeAttackPossible=1", attrname, bh.toInt(), nbh.toInt());
            //     }
            // }
        };

        // as opposed to ensureHexShootableOrNA, this hexattr works with a mask
        // values are 0 or 1 and this check requires a valid target
        auto ensureShootability = [=](BattleHex bh, int v, const CStack* cstack, const char* attrname) {
            auto canshoot = battle->battleCanShoot(cstack);
            auto estacks = getAllStacksForSide(!EI(cstack->unitSide()));
            auto it = std::find_if(estacks.begin(), estacks.end(), [&bh](auto estack) {
                return estack && estack->coversPos(bh);
            });

            auto estack = it == estacks.end() ? nullptr : *it;

            // XXX: the estack on `bh` might be "hidden" from the state
            //      in which case the mask for shooting will be 0 although
            //      there IS a stack to shoot on this hex
            if (v) {
                expect(estack, "%s: =%d, but estack is nullptr", attrname, bh.toInt());
                expect(canshoot, "%s: =%d but canshoot=%d", attrname, v, canshoot);
            } else {
                // stack must be unable to shoot
                // OR there must be no target at hex
                expect(!canshoot || !estack, "%s: =%d but canshoot=%d and estack is not null", attrname, v, canshoot);
            }
        };

        auto ensureCorrectMaskOrNA = [=](BattleHex bh, int v, const CStack* cstack, const CPlayerBattleCallback* battle, const char* attrname) {
            if (isNA(v, cstack, attrname)) return;

            auto basename = std::string(attrname);
            auto mask = HexActMask(v);

            ensureReachability(bh, mask.test(EI(HexAction::MOVE)), cstack, (basename + "{MOVE}").c_str());
            ensureShootability(bh, mask.test(EI(HexAction::SHOOT)), cstack, (basename + "{SHOOT}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_TR, cstack, (basename + "{AMOVE_TR}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_R, cstack, (basename + "{AMOVE_R}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_BR, cstack, (basename + "{AMOVE_BR}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_BL, cstack, (basename + "{AMOVE_BL}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_L, cstack, (basename + "{AMOVE_L}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_TL, cstack, (basename + "{AMOVE_TL}").c_str());
        };

        // XXX: good morale is NOT handled here for simplicity
        //      See comments in Battlefield::GetQueue how to handle it.
        auto tmp = std::vector<battle::Units>{};
        battle->battleGetTurnOrder(tmp, STACK_QUEUE_POS_MAX, 0);
        auto queue = std::vector<const battle::Unit*>{};
        for (const auto& units : tmp) {
            queue.insert(queue.end(), units.begin(), units.end());
        }


        auto alogs = state->supdata->getAttackLogs();

        for (int ihex=0; ihex < BF_SIZE; ihex++) {
            int x = ihex%15;
            int y = ihex/15;
            auto &hex = state->battlefield->hexes->at(y).at(x);
            auto bh = hex->bhex;
            expect(bh == BattleHex(x+1, y), "hex->bhex mismatch");

            auto ainfo = battle->getAccessibility();
            auto aa = ainfo.at(bh.toInt());

            for (int i=0; i < EI(HexAttribute::_count); i++) {
                auto attr = HexAttribute(i);
                auto v = hex->attrs.at(i);
                auto cstack = hexstacks.at(ihex);

                if (cstack) {
                    expect(!!hex->stack, "cstack is present, but hex->stack is nullptr");
                    expect(hex->stack->cstack == cstack, "hex->cstack != cstack");
                } else {
                    expect(!hex->stack, "cstack is nullptr, but hex->stack is present");
                }

                switch(attr) {
                break; case HA::Y_COORD:
                    expect(v == y, "HEX.Y_COORD: %d != %d", v, y);
                break; case HA::X_COORD:
                    expect(v == x, "HEX.X_COORD: %d != %d", v, x);
                break; case HA::STATE_MASK: {
                    auto obstacles = battle->battleGetAllObstaclesOnPos(bh, false);
                    auto anyobstacle = [&obstacles](std::function<bool(const CObstacleInstance*)> fn) {
                        return std::any_of(obstacles.begin(), obstacles.end(), [&fn](std::shared_ptr<const CObstacleInstance> obstacle) {
                            return fn(obstacle.get());
                        });
                    };

                    auto mask = HexStateMask(v);
                    BattleSide side = astack ? astack->unitSide() : BattleSide::ATTACKER; // XXX: Hex defaults to 0 if there is no astack

                    if (mask.test(EI(HexState::PASSABLE))) {
                        expect(
                            aa == EAccessibility::ACCESSIBLE || (EI(side) && aa == EAccessibility::GATE),
                            "HEX.STATE_MASK: PASSABLE bit is set, but accessibility is %d (side: %d)", EI(aa), side
                        );
                    } else {
                        if (aa == EAccessibility::OBSTACLE || aa == EAccessibility::ALIVE_STACK)
                            break;

                        switch (aa) {
                        break; case EAccessibility::ACCESSIBLE:
                            throw std::runtime_error("HEX.STATE_MASK: PASSABLE bit not set, but accessibility is ACCESSIBLE");
                        break; case EAccessibility::ALIVE_STACK:
                        break; case EAccessibility::OBSTACLE:
                        break; case EAccessibility::DESTRUCTIBLE_WALL:
                        break; case EAccessibility::GATE:
                        break; case EAccessibility::UNAVAILABLE:
                            // only Fort and Boat battles can have unavailable hexes
                            expect(
                                battle->battleGetFortifications().wallsHealth > 0 || battle->battleTerrainType() == TerrainId::WATER,
                                "Found UNAVAILABLE accessibility on non-fort, non-boat battlefield: tertype=%d", EI(battle->battleTerrainType())
                            );
                        break; case EAccessibility::SIDE_COLUMN:
                            // side hexes should are not included in the observation
                            throw std::runtime_error("HEX.STATE_MASK: SIDE_COLUMN accessibility found");
                        break; default:
                            throw std::runtime_error("Unexpected accessibility: " + std::to_string(EI(aa)));
                        }
                    }

                    if (mask.test(EI(HexState::STOPPING))) {
                        auto stopping = anyobstacle(std::mem_fn(&CObstacleInstance::stopsMovement));
                        expect(stopping, "HEX.STATE_MASK: STOPPING bit is set, but no obstacle stops movement");
                    }

                    if (mask.test(EI(HexState::DAMAGING_L))) {
                        auto damaging = anyobstacle([side](const CObstacleInstance* o) {
                            if (o->obstacleType == CObstacleInstance::MOAT) return true;
                            if (!o->triggersEffects()) return false;
                            auto s = SpellID(o->ID);
                            if (s == SpellID::FIRE_WALL) return true;
                            if (s != SpellID::LAND_MINE) return false;
                            auto so = dynamic_cast<const SpellCreatedObstacle *>(o);
                            return (side == so->casterSide) ? bool(side) : !bool(side);
                        });
                        expect(damaging, "HEX.STATE_MASK: DAMAGING bit is set, but no obstacle triggers a damaging effect");
                    }

                    if (mask.test(EI(HexState::DAMAGING_R))) {
                        auto damaging = anyobstacle([side](const CObstacleInstance* o) {
                            if (o->obstacleType == CObstacleInstance::MOAT) return true;
                            if (!o->triggersEffects()) return false;
                            auto s = SpellID(o->ID);
                            if (s == SpellID::FIRE_WALL) return true;
                            if (s != SpellID::LAND_MINE) return false;
                            auto so = dynamic_cast<const SpellCreatedObstacle *>(o);
                            return (side == so->casterSide) ? !bool(side) : bool(side);
                        });
                        expect(damaging, "HEX.STATE_MASK: DAMAGING bit is set, but no obstacle triggers a damaging effect");
                    }

                    // if (mask.test(EI(HexState::GATE))) {
                    //     expect(battle->battleGetSiegeLevel(), "HEX.STATE_MASK: GATE bit is set, but there is no fort");
                    //     expect(bh == BattleHex::GATE_INNER || bh == BattleHex::GATE_OUTER, "HEX.STATE_MASK: GATE bit is set on bhex#%d", bh.toInt());
                    // }
                }
                break; case HA::ACTION_MASK: {
                    if (ended) {
                        expect(v == 0, "HEX.ACTION_MASK: battle ended, but action mask is %d", v);
                    } else {
                        ensureCorrectMaskOrNA(bh, v, astack, battle, "HEX.ACTION_MASK");
                    }
                }
                break; case HA::IS_REAR: {
                    ensureValueMatch(v, cstack ? cstack->occupiedHex() == hex->bhex : 0, "HEX.STACK_FLAGS.IS_REAR");
                }
                break; case HA::STACK_SIDE:
                    ensureStackNullOrMatch(attr, cstack, v, [&] { return EI(cstack->unitSide()); }, "HA.STACK_SIDE");
                // break; case HA::STACK_CREATURE_ID:
                //     ensureStackValueMatch(a, v, cstack->unitType()->getId(), "HEX.STACK_CREATURE_ID");
                break; case HA::STACK_QUANTITY:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return std::round(STACK_QTY_MAX * float(cstack->getCount()) / STACK_QTY_MAX); }, "HEX.STACK_QUANTITY");
                break; case HA::STACK_ATTACK:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getAttack(false); }, "HEX.STACK_ATTACK");
                break; case HA::STACK_DEFENSE:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getDefense(false); }, "HEX.STACK_DEFENSE");
                break; case HA::STACK_SHOTS:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->shots.available(); }, "HEX.STACK_SHOTS");
                break; case HA::STACK_DMG_MIN:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getMinDamage(false); }, "HEX.STACK_DMG_MIN");
                break; case HA::STACK_DMG_MAX:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getMaxDamage(false); }, "HEX.STACK_DMG_MAX");
                break; case HA::STACK_HP:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getMaxHealth(); }, "HEX.STACK_HP");
                break; case HA::STACK_HP_LEFT:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getFirstHPleft(); }, "HEX.STACK_HP_LEFT");
                break; case HA::STACK_SPEED:
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return cstack->getMovementRange(); }, "HEX.STACK_SPEED");
                break; case HA::STACK_QUEUE_POS:
                    // at battle end, queue is messed up
                    // (the stack that dealt the killing blow is still "active", but not on 0 pos)
                    if (ended)
                        break;

                    if (v == 0)
                        expect(cstack == astack, "HEX.STACK_QUEUE_POS: =0 but is different from astack");
                    else
                        expect(cstack != astack, "HEX.STACK_QUEUE_POS: =%d but is same as astack", v);

                break; case HA::STACK_VALUE_ONE: {
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return Stack::CalcValue(cstack->unitType()); }, "HEX.STACK_VALUE_ONE");
                }
                break; case HA::STACK_VALUE_REL: {
                    int tot = 0;
                    for (auto &s : allstacks)
                        tot += s->getCount() * Stack::CalcValue(s->unitType());

                    // if (cstack) {
                    //     std::cout << "Stack type: ";
                    //     std::cout << cstack->unitType()->getNameSingularTextID();
                    //     auto vv = 100.0 * cstack->getCount() * Stack::CalcValue(cstack->unitType()) / tot;
                    //     std::cout << " Have: " << v << ", want: " << vv;
                    //     std::cout << "\n";
                    // }
                    ensureStackNullOrMatch(attr, cstack, v, [&]{ return 100 * cstack->getCount() * Stack::CalcValue(cstack->unitType()) / tot; }, "HEX.STACK_VALUE_REL");
                }

                // These require historical information
                // (CPlayerCallback does not provide such)
                break; case HA::STACK_VALUE_REL0:
                break; case HA::STACK_VALUE_KILLED_REL:
                break; case HA::STACK_VALUE_KILLED_ACC_REL0:
                break; case HA::STACK_VALUE_LOST_REL:
                break; case HA::STACK_VALUE_LOST_ACC_REL0:
                break; case HA::STACK_DMG_DEALT_REL:
                break; case HA::STACK_DMG_DEALT_ACC_REL0:
                break; case HA::STACK_DMG_RECEIVED_REL:
                break; case HA::STACK_DMG_RECEIVED_ACC_REL0:

                break; case HA::STACK_FLAGS: {
                    if (!isNA(v, cstack, "HEX.STACK_FLAGS")) {
                        for (int j=0; j<EI(StackFlag::_count); j++) {
                            auto f = StackFlag(j);
                            auto vf = hex->stack->flag(f);

                            switch(f) {
                            break; case SF::IS_ACTIVE:
                                // at battle end, queue is messed up
                                // (the stack that dealt the killing blow is still "active", but not on 0 pos)
                                if (ended)
                                    break;

                                if (vf == 0)
                                    expect(cstack != astack, "HEX.STACK_FLAGS.IS_ACTIVE: =0 but cstack == astack");
                                else
                                    expect(cstack == astack, "HEX.STACK_FLAGS.IS_ACTIVE: =%d but cstack != astack", vf);
                            break; case SF::CAN_WAIT:
                                ensureValueMatch(vf, cstack->willMove() && !cstack->waitedThisTurn, "HEX.STACK_FLAGS.CAN_WAIT");
                            break; case SF::WILL_ACT:
                                ensureValueMatch(vf, cstack->willMove(), "HEX.STACK_FLAGS.WILL_ACT");
                            break; case SF::CAN_RETALIATE:
                                // XXX: ableToRetaliate() calls CAmmo's (i.e. CRetaliations's) canUse() method
                                //      which takes into account relevant bonuses (e.g. NO_RETALIATION from expert Blind)
                                //      It does *NOT* take into account the attacker's BLOCKS_RETALIATION bonus
                                //      (if it did, what would be the correct CAN_RETALIATE value for friendly units?)
                                ensureValueMatch(vf, cstack->ableToRetaliate(), "HEX.STACK_FLAGS.CAN_RETALIATE");
                            break; case SF::SLEEPING:
                                cstack->unitType()->getId() == CreatureID::AMMO_CART
                                    ? ensureValueMatch(vf, false, "HEX.STACK_FLAGS.SLEEPING")
                                    : ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::NOT_ACTIVE), "HEX.STACK_FLAGS.SLEEPING");
                            break; case SF::BLOCKED:
                                ensureValueMatch(vf, cstack->canShoot() && battle->battleIsUnitBlocked(cstack), "HEX.STACK_FLAGS.TWO_HEX_ATTACK_BREATH");
                            break; case SF::BLOCKING: {
                                auto want = 0;
                                for (auto &adjstack : battle->battleAdjacentUnits(cstack)) {
                                    if (adjstack->unitSide() != cstack->unitSide() && adjstack->canShoot() && battle->battleIsUnitBlocked(adjstack)) {
                                        want = 1;
                                        break;
                                    }
                                }
                                ensureValueMatch(vf, bool(want), "HEX.STACK_FLAGS.BLOCKING");
                            }
                            break; case SF::IS_WIDE:
                                ensureValueMatch(vf, cstack->doubleWide(), "HEX.STACK_FLAGS.IS_WIDE");
                            break; case SF::FLYING:
                                ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::FLYING), "HEX.STACK_FLAGS.FLYING");
                            break; case SF::BLIND_LIKE_ATTACK: {
                                auto spell_after_attack_bonuses = BonusList();
                                auto bonuses = cstack->getAllBonuses(Selector::all, nullptr);
                                bonuses->getBonuses(spell_after_attack_bonuses, Selector::type()(BonusType::SPELL_AFTER_ATTACK), nullptr);
                                auto castchance = [&spell_after_attack_bonuses](std::vector<SpellID> spellids) {
                                    // TODO: what about BLOCK_MAGIC_ABOVE / BLOCK_ALL_MAGIC
                                    //       Isn't the chance always 0 then?
                                    int res = 0;

                                    auto sel = CSelector(Selector::subtype()(spellids.at(0)));
                                    for (auto it = spellids.begin()+1; it != spellids.end(); ++it)
                                        sel = sel.Or(Selector::subtype()(*it));

                                    auto selected = BonusList();
                                    spell_after_attack_bonuses.getBonuses(selected, sel);
                                    for (auto &bonus : selected)
                                        res += bonus->val;
                                    return res;
                                };

                                ensureValueMatch(vf, castchance({SpellID::BLIND, SpellID::STONE_GAZE, SpellID::PARALYZE}) > 0, "HEX.STACK_FLAGS.BLIND_LIKE_ATTACK");
                            }
                            break; case SF::ADDITIONAL_ATTACK:
                                ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::ADDITIONAL_ATTACK), "HEX.STACK_FLAGS.ADDITIONAL_ATTACK");
                            break; case SF::NO_MELEE_PENALTY:
                                ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::NO_MELEE_PENALTY), "HEX.STACK_FLAGS.NO_MELEE_PENALTY");
                            break; case SF::TWO_HEX_ATTACK_BREATH:
                                ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::TWO_HEX_ATTACK_BREATH), "HEX.STACK_FLAGS.TWO_HEX_ATTACK_BREATH");
                            break; case SF::BLOCKS_RETALIATION:
                                ensureValueMatch(vf, cstack->hasBonusOfType(BonusType::BLOCKS_RETALIATION), "HEX.STACK_FLAGS.BLOCKS_RETALIATION");
                            break; default:
                                THROW_FORMAT("Unexpected StackFlag: %d", EI(f));
                            }
                        }
                    }
                }
                break; default:
                    THROW_FORMAT("Unexpected HexAttribute: %d", EI(attr));
                }
            }
        }

        for (auto &link : state->battlefield->links) {
            expect(link->src && link->dst, "invalid link: null hexes");
            expect(link->src != link->dst, "invalid link: same hexes");
            auto src = link->src;
            auto dst = link->dst;

            switch (link->type) {
            break; case LT::ACTS_BEFORE: {
                expect(src->stack && dst->stack, "ACTS_BEFORE, but no stack(s)");
                auto it = std::find_if(queue.begin(), queue.end(), [&dst](const battle::Unit* u) {
                    return u->unitId() == dst->stack->cstack->unitId();
                });

                int dstpos = std::distance(queue.begin(), it);
                int want = 0;
                for (int i=0; i<dstpos; i++) {
                    auto u = queue.at(i);
                    if (u->unitId() == src->stack->cstack->unitId()) want++;
                }

                // edge case where blinded unit is not present in queue
                // (QPOS is fixed to last position in this case)
                // and the src stack was originally on the last position
                if (it == queue.end() && src->stack->cstack->unitId() == queue.at(queue.size() - 1)->unitId())
                    want -= 1;

                expect(link->value > 0, "ACTS_BEFORE, but value is 0");
                expect(want > 0, "ACTS_BEFORE, but want is 0");
                ensureFloatMatch(link->value, want, "ACTS_BEFORE");
            }
            break; case LT::ADJACENT: {
                expect(BattleHex::mutualPosition(src->bhex, dst->bhex) != BattleHex::EDir::NONE, "ADJACENT: not adjacent: %d and %d", src->bhex.toInt(), dst->bhex.toInt());
                ensureFloatMatch(link->value, 1, "ADJACENT");
            }
            break; case LT::BLOCKED_BY: {
                expect(src->stack && dst->stack, "BLOCKED_BY, but no stack(s)");
                expect(src->stack->attr(SA::SIDE) != dst->stack->attr(SA::SIDE), "BLOCKED_BY, but stacks are on same side");
                expect(src->stack->flag(SF::BLOCKED), "BLOCKED_BY, but src stack is not blocked");
                expect(dst->stack->flag(SF::BLOCKING), "BLOCKED_BY, but dst stack is not blocking");
                bool matched = false;
                for (auto u : battle->battleAdjacentUnits(src->stack->cstack)) {
                    if (u->unitId() == dst->stack->cstack->unitId()) {
                        matched = true;
                        break;
                    }
                }
                expect(matched, "BLOCKED_BY, but no adjacent enemy units found");
                ensureValueMatch(link->value, 1, "BLOCKED_BY");
            }
            break; case LT::MELEE_DMG_REL: {
                expect(src->stack && dst->stack, "MELEE_DMG_REL, but no stack(s)");
                expect(src->stack->attr(SA::SIDE) != dst->stack->attr(SA::SIDE), "MELEE_DMG_REL, but stacks are on same side");
                auto bai = BattleAttackInfo(src->stack->cstack, dst->stack->cstack, 0, false);
                auto dmg = battle->battleEstimateDamage(bai);
                auto avgdmg = 0.5 * (dmg.damage.min + dmg.damage.max);
                auto rel = avgdmg / dst->stack->cstack->getAvailableHealth();
                ensureFloatMatch(link->value, rel, "MELEE_DMG_REL");
            }
            break; case LT::RETAL_DMG_REL: {
                expect(src->stack && dst->stack, "RETAL_DMG_REL, but no stack(s)");
                expect(src->stack->attr(SA::SIDE) != dst->stack->attr(SA::SIDE), "RETAL_DMG_REL, but stacks are on same side");

                DamageEstimation est;
                auto bai = BattleAttackInfo(src->stack->cstack, dst->stack->cstack, 0, false);
                battle->battleEstimateDamage(bai, &est);
                auto avgdmg = 0.5 * (est.damage.min + est.damage.max);
                auto rel = avgdmg / src->stack->cstack->getAvailableHealth();
                ensureFloatMatch(link->value, rel, "RETAL_DMG_REL");
            }
            break; case LT::RANGED_DMG_REL: {
                expect(src->stack && dst->stack, "RANGED_DMG_REL, but no stack(s)");
                expect(src->stack->attr(SA::SIDE) != dst->stack->attr(SA::SIDE), "RANGED_DMG_REL, but stacks are on same side");
                expect(src->stack->cstack->canShoot(), "RANGED_DMG_REL, but src stack.canShoot() returned false");
                auto bai = BattleAttackInfo(src->stack->cstack, dst->stack->cstack, 0, true);
                auto dmg = battle->battleEstimateDamage(bai);
                auto avgdmg = 0.5 * (dmg.damage.min + dmg.damage.max);
                auto rel = avgdmg / dst->stack->cstack->getAvailableHealth();
                ensureFloatMatch(link->value, rel, "RANGED_DMG_REL");
            }
            break; case LT::RANGED_MOD: {
                expect(src->stack != nullptr, "RANGED_DMG_MOD, but no src stack");
                expect(src->stack->cstack->canShoot(), "RANGED_DMG_MOD, but src stack.canShoot() returned false");

                auto adj = BattleHex::mutualPosition(src->bhex, dst->bhex) != BattleHex::EDir::NONE;
                auto own = src->stack->cstack->coversPos(dst->bhex);
                auto freeshooting = src->stack->cstack->hasBonusOfType(BonusType::FREE_SHOOTING);

                expect(!own, "RANGED_DMG_MOD, but own hex is not shootable");
                expect(!adj || freeshooting, "RANGED_DMG_MOD, but neighbouring hexes are not shootable");

                float want = 1;

                if (battle->battleHasDistancePenalty(src->stack->cstack, src->bhex, dst->bhex))
                    want *= 0.5;

                if (battle->battleHasWallPenalty(src->stack->cstack, src->bhex, dst->bhex))
                    want *= 0.5;

                ensureFloatMatch(link->value, want, "RANGED_MOD");
            }
            break; case LT::REACH: {
                expect(src->stack != nullptr, "REACH, but no src stack");
                // std::cout << Render(state, state->action.get());
                expect(dst->statemask.test(EI(HexState::PASSABLE)) || src->stack == dst->stack, "REACH from %d, but dst hex at %d is not PASSABLE and not part of the same stack", src->id, dst->id);

                auto dist = rinfos.at(src->stack->cstack).distances.at(dst->bhex.toInt());
                auto speed = src->stack->cstack->getMovementRange();
                expect(dist <= speed, "REACH, but distance %d > speed %d", dist, speed);
                ensureFloatMatch(link->value, 1, "REACH");
            }
            break; case LT::REAR_HEX: {
                expect(src->stack != nullptr, "REAR_HEX, but no src stack");
                expect(src->stack->cstack->doubleWide(), "REAR_HEX, but src stack is not double-wide");
                expect(src->stack->cstack->occupiedHex() == dst->bhex, "REAR_HEX, occupied hex %d != dst %d", src->stack->cstack->occupiedHex().toInt(), dst->bhex.toInt());
                ensureFloatMatch(link->value, 1, "REAR_HEX");
            }
            break; default:
                THROW_FORMAT("Unexpected LinkType: %d", EI(link->type));
                break;
            }
        }

        // Mask is undefined at battle end
        if (ended)
            return;
    }

    // This intentionally uses the IState interface to ensure that
    // the schema is properly exposing all needed informaton
    std::string Render(const Schema::IState* istate, const Action *action) {
        // auto bfstate = istate->getBattlefieldState();
        auto supdata_ = istate->getSupplementaryData();
        expect(supdata_.has_value(), "supdata_ holds no value");
        expect(supdata_.type() == typeid(const ISupplementaryData*), "supdata_ of unexpected type");
        auto supdata = std::any_cast<const ISupplementaryData*>(supdata_);
        expect(supdata, "supdata holds a nullptr");
        auto lgstats = supdata->getGlobalStatsLeft();
        auto rgstats = supdata->getGlobalStatsRight();
        auto mystats = EI(supdata->getSide()) ? supdata->getGlobalStatsRight() : supdata->getGlobalStatsLeft();
        auto hexes = supdata->getHexes();
        auto color = supdata->getColor();

        const IStack* astack = nullptr;

        // find an active hex (i.e. with active stack on it)
        for (auto &row : hexes) {
            for (auto &hex : row) {
                auto stack = hex->getStack();
                if (stack && stack->getFlag(SF::IS_ACTIVE)) {
                    expect(!astack || astack == stack, "two active stacks found");
                    astack = stack;
                }
            }
        }

        if (!astack && !supdata->getIsBattleEnded())
            logAi->error("could not find an active stack (battle has not ended).");

        std::string nocol = "\033[0m";
        std::string redcol = "\033[31m"; // red
        std::string bluecol = "\033[34m"; // blue
        std::string darkcol = "\033[90m";
        std::string activemod = "\033[107m\033[7m"; // bold+reversed
        std::string ukncol = "\033[7m"; // white

        std::vector<std::stringstream> lines;

        //
        // 1. Add logs table:
        //
        // #1 attacks #5 for 16 dmg (1 killed)
        // #5 attacks #1 for 4 dmg (0 killed)
        // ...
        //
        for (auto &alog : supdata->getAttackLogs()) {
            auto row = std::stringstream();
            auto attcol = ukncol;
            auto attalias = '?';
            auto defcol = ukncol;
            auto defalias = '?';

            if (alog->getAttacker()) {
                attcol = (alog->getAttacker()->getAttr(SA::SIDE) == 0) ? redcol : bluecol;
                attalias = alog->getAttacker()->getAlias();
            }

            if (alog->getDefender()) {
                defcol = (alog->getDefender()->getAttr(SA::SIDE) == 0) ? redcol : bluecol;
                defalias = alog->getDefender()->getAlias();
            }

            row << attcol << "#" << attalias << nocol;
            row << " attacks ";
            row << defcol << "#" << defalias << nocol;
            row << " for " << alog->getDamageDealt() << " dmg";
            row << " (kills: " << alog->getUnitsKilled() << ", value: " << alog->getValueKilled() << " / " << alog->getValueKilledPercent() << "%)";

            lines.push_back(std::move(row));
        }

        //
        // 2. Build ASCII table
        //    (+populate aliveStacks var)
        //    NOTE: the contents below look mis-aligned in some editors.
        //          In (my) terminal, it all looks correct though.
        //
        //   ▕₁▕₂▕₃▕₄▕₅▕₆▕₇▕₈▕₉▕₀▕₁▕₂▕₃▕₄▕₅▕
        //  ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃
        // ¹┨  1 ◌ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ 1 ┠¹
        // ²┨ ◌ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠²
        // ³┨  ◌ ○ ○ ○ ○ ○ ○ ◌ ▦ ▦ ◌ ◌ ◌ ◌ ◌ ┠³
        // ⁴┨ ◌ ○ ○ ○ ○ ○ ○ ○ ▦ ▦ ▦ ◌ ◌ ◌ ◌  ┠⁴
        // ⁵┨  2 ◌ ○ ○ ▦ ▦ ◌ ○ ◌ ◌ ◌ ◌ ◌ ◌ 2 ┠⁵
        // ⁶┨ ◌ ○ ○ ○ ▦ ▦ ◌ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌  ┠⁶
        // ⁷┨  3 3 ○ ○ ○ ▦ ◌ ○ ○ ◌ ◌ ▦ ◌ ◌ 3 ┠⁷
        // ⁸┨ ◌ ○ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ▦ ▦ ◌ ◌  ┠⁸
        // ⁹┨  ◌ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ┠⁹
        // ⁰┨ ◌ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠⁰
        // ¹┨  4 ◌ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ 4 ┠¹
        //  ┃▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁┃
        //   ▕¹▕²▕³▕⁴▕⁵▕⁶▕⁷▕⁸▕⁹▕⁰▕¹▕²▕³▕⁴▕⁵▕
        //

        // s=special; can be any number, slot is always 7 (SPECIAL), visualized A,B,C...

        auto tablestartrow = lines.size();

        lines.emplace_back() << "    ₀▏₁▏₂▏₃▏₄▏₅▏₆▏₇▏₈▏₉▏₀▏₁▏₂▏₃▏₄";
        lines.emplace_back() << " ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃ ";

        static std::array<std::string, 10> nummap{"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"};

        bool addspace = true;

        auto seenstacks = std::map<const IStack*, IHex*>{};
        bool divlines = true;

        // y even "▏"
        // y odd "▕"

        for (int y=0; y < BF_YMAX; y++) {
            for (int x=0; x < BF_XMAX; x++) {
                auto sym = std::string("?");
                auto &hex = hexes.at(y).at(x);
                auto stack = hex->getStack();

                auto &row = (x == 0)
                    ? (lines.emplace_back() << nummap.at(y%10) << "┨" << (y % 2 == 0 ? " " : ""))
                    : lines.back();

                if (addspace) {
                    if (divlines && (x != 0)) {
                        row << darkcol << (y % 2 == 0 ? "▏" : "▕") << nocol;
                    } else {
                        row << " ";
                    }
                }

                addspace = true;

                auto smask = HexStateMask(hex->getAttr(HA::STATE_MASK));
                auto col = nocol;

                // First put symbols based on hex state.
                // If there's a stack on this hex, symbol will be overriden.
                HexStateMask mpass = 1<<EI(HexState::PASSABLE);
                HexStateMask mstop = 1<<EI(HexState::STOPPING);
                HexStateMask mdmgl = 1<<EI(HexState::DAMAGING_L);
                HexStateMask mdmgr = 1<<EI(HexState::DAMAGING_R);
                HexStateMask mdefault = 0; // or mother :)

                std::vector<std::tuple<std::string, std::string, HexStateMask>> symbols {
                    {"⨻", bluecol, mpass|mstop|mdmgl},
                    {"⨻", redcol, mpass|mstop|mdmgr},
                    {"✶", bluecol, mpass|mdmgl},
                    {"✶", redcol, mpass|mdmgr},
                    {"△", nocol, mpass|mstop},
                    {"○", nocol, mpass}, // changed to "◌" if unreachable
                    {"◼",  nocol, mdefault}
                };

                for (auto &tuple : symbols) {
                    auto &[s, c, m] = tuple;
                    if ((smask & m) == m) {
                        sym = s;
                        col = c;
                        break;
                    }
                }

                auto amask = HexActMask(hex->getAttr(HA::ACTION_MASK));
                if (col == nocol && !amask.test(EI(HexAction::MOVE))) { // || supdata->getIsBattleEnded()
                    col = darkcol;
                    sym = sym == "○" ? "◌" : sym;
                }

                if (stack) {
                    auto seen = seenstacks.find(stack) != seenstacks.end();
                    auto &[_, _e, n, _vmax] = HEX_ENCODING.at(EI(HA::STACK_FLAGS));
                    auto flags = std::bitset<n>(stack->getAttr(SA::FLAGS));
                    sym = std::string(1, stack->getAlias());
                    col = stack->getAttr(SA::SIDE) ? bluecol : redcol;

                    if (stack->getAttr(SA::QUEUE_POS) == 0) // && !supdata->getIsBattleEnded()
                        col += activemod;

                    if (flags.test(EI(SF::IS_WIDE)) && !seen) {
                        if (stack->getAttr(SA::SIDE) == 0) {
                            sym += "↠";
                            addspace = false;
                        } else if(stack->getAttr(SA::SIDE) == 1 && hex->getAttr(HA::X_COORD) < 14) {
                            sym += "↞";
                            addspace = false;
                        }
                    }

                    if (!seen)
                        seenstacks.insert({stack, hex});
                }

                row << (col + sym + nocol);

                if (x == BF_XMAX-1) {
                    row << (y % 2 == 0 ? " " : "  ") << "┠" << nummap.at(y % 10);
                }
            }
        }

        lines.emplace_back() << " ┃▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁┃";
        lines.emplace_back() << "   ⁰▕¹▕²▕³▕⁴▕⁵▕⁶▕⁷▕⁸▕⁹▕⁰▕¹▕²▕³▕⁴";

        //
        // 3. Add side table stuff
        //
        //   ▕₁▕₂▕₃▕₄▕₅▕₆▕₇▕₈▕₉▕₀▕₁▕₂▕₃▕₄▕₅▕
        //  ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃         Player: RED
        // ₁┨  ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ┠₁    Last action:
        // ₂┨ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠₂      DMG dealt: 0
        // ₃┨  1 ○ ○ ○ ○ ○ ◌ ◌ ▦ ▦ ◌ ◌ ◌ ◌ 1 ┠₃   Units killed: 0
        // ...

        for (int i=0; i<=lines.size(); i++) {
            std::string name;
            std::string value;
            auto side = EI(supdata->getSide());

            switch(i) {
            break; case 1:
                name = "Player";
                if (supdata->getIsBattleEnded())
                    value = "";
                else
                    value = side ? bluecol + "BLUE" + nocol : redcol + "RED" + nocol;
            break; case 2:
                name = "Last action";
                value = action ? action->name() + " [" + std::to_string(action->action) + "]" : "";
            break; case 3: name = "DMG dealt"; value = boost::str(boost::format("%d (%d since start)") % mystats->getDmgDealtNow() % mystats->getDmgDealtTotal());
            break; case 4: name = "DMG received"; value = boost::str(boost::format("%d (%d since start)") % mystats->getDmgReceivedNow() % mystats->getDmgReceivedTotal());
            break; case 5: name = "Value killed"; value = boost::str(boost::format("%d (%d since start)") % mystats->getValueKilledNow() % mystats->getValueKilledTotal());
            break; case 6: name = "Value lost"; value = boost::str(boost::format("%d (%d since start)") % mystats->getValueLostNow() % mystats->getValueLostTotal());
            break; case 7: {
                // XXX: if there's a draw, this text will be incorrect
                auto restext = supdata->getIsVictorious()
                    ? (side ? bluecol + "BLUE WINS" : redcol + "RED WINS")
                    : (side ? redcol + "RED WINS" : bluecol + "BLUE WINS" );

                name = "Battle result"; value = supdata->getIsBattleEnded() ? (restext + nocol) : "";
            }
            break; case 8: name = "Army value (L)"; value = boost::str(boost::format("%d (%.0f%% of current BF value)") % lgstats->getValueNow() % (100.0 * lgstats->getValueNow() / (lgstats->getValueNow() + rgstats->getValueNow())));
            break; case 9: name = "Army value (R)"; value = boost::str(boost::format("%d (%.0f%% of current BF value)") % rgstats->getValueNow() % (100.0 * rgstats->getValueNow() / (lgstats->getValueNow() + rgstats->getValueNow())));
            break; default:
                continue;
            }

            lines.at(tablestartrow + i) << PadLeft(name, 15, ' ') << ": " << value;
        }

        lines.emplace_back() << "";

        //
        // 5. Add stacks table:
        //
        //          Stack # |   0   1   2   3   4   5   6   A   B   C   0   1   2   3   4   5   6   A   B   C
        // -----------------+--------------------------------------------------------------------------------
        //              Qty |   0  34   0   0   0   0   0   0   0   0   0  17   0   0   0   0   0   0   0   0
        //           Attack |   0   8   0   0   0   0   0   0   0   0   0   6   0   0   0   0   0   0   0   0
        //    ...10 more... | ...
        // -----------------+--------------------------------------------------------------------------------
        //
        // table with 24 columns (1 header, 3 dividers, 10 stacks per side)
        // Each row represents a separate attribute

        using RowDef = std::tuple<StackAttribute, std::string>;

        // max to show
        constexpr int max_stacks_per_side = 10;

        // All cell text is aligned right
        auto colwidths = std::array<int, 4 + 2*max_stacks_per_side>{};
        colwidths.fill(4); // default col width
        colwidths.at(0) = 16; // header col

        // Divider column indexes
        auto divcolids = {1, max_stacks_per_side+2, 2*max_stacks_per_side+3};

        for (int i : divcolids)
            colwidths.at(i) = 2; // divider col

        // {Attribute, name, colwidth}
        const auto rowdefs = std::vector<RowDef> {
            RowDef{SA::FLAGS, "Stack #"},  // stack alias (1..7, S or M)
            RowDef{SA::SIDE, ""},  // divider row
            RowDef{SA::QUANTITY, "Qty"},
            RowDef{SA::ATTACK, "Attack"},
            RowDef{SA::DEFENSE, "Defense"},
            RowDef{SA::SHOTS, "Shots"},
            RowDef{SA::DMG_MIN, "Dmg (min)"},
            RowDef{SA::DMG_MAX, "Dmg (max)"},
            RowDef{SA::HP, "HP"},
            RowDef{SA::HP_LEFT, "HP left"},
            RowDef{SA::SPEED, "Speed"},
            RowDef{SA::QUEUE_POS, "Queue"},
            RowDef{SA::VALUE_ONE, "Value (one)"},
            RowDef{SA::VALUE_REL, "Value (%)"},
            // RowDef{SA::ESTIMATED_DMG, "Est. DMG%"},
            RowDef{SA::FLAGS, "State"},  // "WAR" = CAN_WAIT, WILL_ACT, CAN_RETAL
            RowDef{SA::FLAGS, "Attack mods"}, // "DB" = Double, Blinding
            // 2 values per column to avoid too long table
            RowDef{SA::FLAGS, "Blocked/ing"},
            RowDef{SA::FLAGS, "Fly/Sleep"},
            RowDef{SA::FLAGS, "NoRetal/NoMelee"},
            RowDef{SA::FLAGS, "Wide/Breath"},
            RowDef{SA::SIDE, ""},  // divider row
        };

        // Table with nrows and ncells, each cell a 3-element tuple
        // cell: color, width, txt
        using TableCell = std::tuple<std::string, int, std::string>;
        using TableRow = std::array<TableCell, colwidths.size()>;

        auto table = std::vector<TableRow> {};

        auto divrow = TableRow{};
        for (int i=0; i<colwidths.size(); i++)
            divrow[i] = {nocol, colwidths.at(i), std::string(colwidths.at(i), '-')};

        for (int i : divcolids)
            divrow.at(i) = {nocol, colwidths.at(i), std::string(colwidths.at(i)-1, '-') + "+"};

        int specialcounter = 0;

        // Attribute rows
        for (auto& [a, aname] : rowdefs) {
            if (a == SA::SIDE) { // divider row
                table.push_back(divrow);
                continue;
            }

            auto row = TableRow{};

            // Header col
            row.at(0) = {nocol, colwidths.at(0), aname};

            // Div cols
            for (int i : {1, 2+max_stacks_per_side, int(colwidths.size()-1)})
                row.at(i) = {nocol, colwidths.at(i), "|"};

            // Stack cols
            for (auto side : {0, 1}) {
                auto sidestacks = std::array<std::pair<const IStack*, IHex*>, max_stacks_per_side>{};
                auto extracounter = 0;
                for (const auto& [stack, hex] : seenstacks) {
                    if (stack->getAttr(SA::SIDE) == side) {
                        int slot = stack->getAlias() >= '0' && stack->getAlias() <= '6'
                            ? stack->getAlias() - '0'
                            : 7 + extracounter;

                        if (slot < max_stacks_per_side)
                            sidestacks.at(slot) = {stack, hex};

                        if (slot >= 7) extracounter += 1;
                    }
                }

                for (int i=0; i<sidestacks.size(); ++i) {
                    auto &[stack, hex] = sidestacks.at(i);
                    auto color = nocol;
                    std::string value = "";

                    if (stack) {
                        auto &[_, _e, n, _vmax] = HEX_ENCODING.at(EI(HA::STACK_FLAGS));
                        auto flags = std::bitset<n>(stack->getAttr(SA::FLAGS));

                        color = stack->getAttr(SA::SIDE) ? bluecol : redcol;
                        // } else if (a == SA::ACTSTATE) {
                        //     switch(StackActState(stack->getAttr(a))) {
                        //     break; case StackActState::READY: value = "R";
                        //     break; case StackActState::WAITING: value = "W";
                        //     break; case StackActState::DONE: value = "D";
                        //     break; default:
                        //         throw std::runtime_error("Unexpected actstate:: " + std::to_string(EI(stack->getAttr(a))));
                        //     }
                        if (a == SA::VALUE_ONE && stack->getAttr(a) >= 1000) {
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(1) << (stack->getAttr(a) / 1000.0);
                            value = oss.str();
                            value[value.size()-2] = 'k';
                            if (value.rfind("K0") == (value.size() - 2))
                                value.resize(value.size() - 1);
                        } else if (a == SA::FLAGS) {
                            auto fmt = boost::format("%d/%d");

// Y_COORD, "State"},  // "WAR" = CAN_WAIT, WILL_ACT, CAN_RETAL
// Y_COORD, "Attack type"}, // "DB" = Double strike, Blind-like
// Y_COORD, "Blocked/ing"},
// Y_COORD, "Fly/Sleep"},
// Y_COORD, "No Retal/Melee"},
// Y_COORD, "Wide/Breath"},
                            switch(specialcounter) {
                            break; case 0: value = std::string(1, stack->getAlias());
                            break; case 1: {
                                value = std::string("");
                                value += flags.test(EI(SF::CAN_WAIT)) ? "W" : " ";
                                value += flags.test(EI(SF::WILL_ACT)) ? "A" : " ";
                                value += flags.test(EI(SF::CAN_RETALIATE)) ? "R" : " ";
                            }
                            break; case 2: {
                                value = std::string("");
                                value += flags.test(EI(SF::ADDITIONAL_ATTACK)) ? "D" : " ";
                                value += flags.test(EI(SF::BLIND_LIKE_ATTACK)) ? "B" : " ";
                            }
                            break; case 3: value = boost::str(fmt % flags.test(EI(SF::BLOCKED)) % flags.test(EI(SF::BLOCKING)));
                            break; case 4: value = boost::str(fmt % flags.test(EI(SF::FLYING)) % flags.test(EI(SF::SLEEPING)));
                            break; case 5: value = boost::str(fmt % flags.test(EI(SF::BLOCKS_RETALIATION)) % flags.test(EI(SF::NO_MELEE_PENALTY)));
                            break; case 6: value = boost::str(fmt % flags.test(EI(SF::IS_WIDE)) % flags.test(EI(SF::TWO_HEX_ATTACK_BREATH)));
                            break; default:
                                THROW_FORMAT("Unexpected specialcounter: %d", specialcounter);
                            }
                        } else {
                            value = std::to_string(stack->getAttr(a));
                        }

                        if (stack->getAttr(SA::QUEUE_POS) == 0 && !supdata->getIsBattleEnded()) color += activemod;
                    }

                    auto colid = 2 + i + side + (max_stacks_per_side*side);
                    row.at(colid) = {color, colwidths.at(colid), value};
                }
            }

            if (a == SA::FLAGS)
                ++specialcounter;

            table.push_back(row);
        }

        for (auto &r : table) {
            auto line = std::stringstream();
            for (auto& [color, width, txt] : r)
                line << color << PadLeft(txt, width, ' ') << nocol;

            lines.push_back(std::move(line));
        }

        //
        // 7. Join rows into a single string
        //
        std::string res = lines[0].str();
        for (int i=1; i<lines.size(); i++)
            res += "\n" + lines[i].str();

        return res;
    }
}
