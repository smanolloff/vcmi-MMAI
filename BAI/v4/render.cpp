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
#include "battle/CObstacleInstance.h"
#include "bonuses/Bonus.h"
#include "bonuses/BonusCustomTypes.h"
#include "bonuses/BonusEnum.h"
#include "bonuses/BonusList.h"
#include "bonuses/BonusSelector.h"
#include "constants/EntityIdentifiers.h"
#include "mapObjects/CGTownInstance.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "spells/CSpellHandler.h"
#include "vcmi/spells/Caster.h"

#include "schema/v4/constants.h"
#include "schema/v4/types.h"

#include "./render.h"
#include "./hex.h"

namespace MMAI::BAI::V4 {
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

        // XXX: special containers for "reserved" unit slots:
        // summoned creatures (-3), war machines (-4), arrow towers (-5)

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

        if (!astack) // draw?
            expect(ended, "astack is NULL, but ended is not true");
        else if (ended) {
            // at battle-end, activeStack is usually the ENEMY stack
            // XXX: this expect will incorrectly throw if we retreated as a regular action
            //      (in which case our stack will be active, but we would have lost the battle)
            // expect(state->supdata->victory == (astack->getOwner() == battle->battleGetMySide()), "state->supdata->victory is %d, but astack->side=%d and myside=%d", state->supdata->victory, astack->getOwner(), battle->battleGetMySide());

            // at battle-end, even regardless of the actual active stack,
            // battlefield->astack must be nullptr
            expect(!state->battlefield->astack, "ended, but battlefield->astack is not NULL");
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

        //
        // XXX: if v=0, returns true when UNreachable
        //      if v=1, returns true when reachable
        //
        auto checkReachable = [=](BattleHex bh, int v, const CStack* stack) {
            auto distance = rinfos.at(stack).distances.at(bh);
            auto canreach = (stack->getMovementRange() >= distance);
            if (v == 0)
                return !canreach;
            else if (v == 1)
                return canreach;
            else
                THROW_FORMAT("Unexpected v: %d", v);
        };

        auto ensureReachability = [=](BattleHex bh, int v, const CStack* stack, const char* attrname) {
            expect(checkReachable(bh, v, stack), "%s: (bhex=%d) reachability expected: %d", attrname, bh.hex, v);
        };

        auto ensureReachabilityOrNA = [=](BattleHex bh, int v, const CStack* stack, const char* attrname) {
            if (isNA(v, stack, attrname)) return;
            ensureReachability(bh, v, stack, attrname);
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

            // find id of `estack`
            // can't use std::distance, as estacks can contain nullptrs
            // for regular slots (0..6)
            int eindex = -1;
            for (auto &stack : estacks) {
                if (stack && estack) ++eindex;
                if (stack == estack) break;
            }

            // XXX: the estack on `bh` might be "hidden" from the state
            //      in which case the mask for shooting will be 0 although
            //      there IS a stack to shoot on this hex
            if (v) {
                expect(canshoot && (eindex < MAX_STACKS_PER_SIDE), "%s: =%d but canshoot=%d and eindex=%d", attrname, v, canshoot, eindex);
            } else {
                // stack must be unable to shoot
                // OR there must be no target at hex
                // OR the target at hex must be "hidden"
                expect(!canshoot || eindex == -1 || eindex >= MAX_STACKS_PER_SIDE, "%s: =%d but canshoot=%d and eindex=%d", attrname, v, canshoot, eindex);
            }
        };

        auto getHexesForFixedTarget = [rinfos](BattleHex bh, const CStack* stack){
            auto nbhs = std::vector<BattleHex>{
                bh.cloneInDirection(BattleHex::BOTTOM_LEFT, false),
                bh.cloneInDirection(BattleHex::TOP_LEFT, false),
                bh.cloneInDirection(BattleHex::TOP_RIGHT, false),
                bh.cloneInDirection(BattleHex::BOTTOM_RIGHT, false),
            };

            if (stack->doubleWide()) {
                if (stack->unitSide() == BattleSide::ATTACKER) {
                    // attacker's "back" hex is to-the-left
                    nbhs.push_back(bh.cloneInDirection(BattleHex::LEFT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::RIGHT, false).cloneInDirection(BattleHex::TOP_RIGHT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::RIGHT, false).cloneInDirection(BattleHex::RIGHT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::RIGHT, false).cloneInDirection(BattleHex::BOTTOM_RIGHT, false));
                } else {
                    // attacker's "back" hex is to-the-right
                    nbhs.push_back(bh.cloneInDirection(BattleHex::RIGHT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::LEFT, false).cloneInDirection(BattleHex::BOTTOM_LEFT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::LEFT, false).cloneInDirection(BattleHex::LEFT, false));
                    nbhs.push_back(bh.cloneInDirection(BattleHex::LEFT, false).cloneInDirection(BattleHex::TOP_LEFT, false));
                }
            } else {
                nbhs.push_back(bh.cloneInDirection(BattleHex::LEFT, false));
                nbhs.push_back(bh.cloneInDirection(BattleHex::RIGHT, false));
            }

            auto res = std::vector<int>{};

            for (auto &nbh : nbhs)
                if (nbh.isAvailable() && rinfos.at(stack).distances.at(nbh) <= stack->getMovementRange())
                    res.push_back(nbh);

            return res;
        };

        auto ensureValueMatch = [=](int have, int want, const char* attrname) {
            expect(have == want, "%s: have: %d, want: %d", attrname, have, want);
        };

        auto ensureStackValueMatch = [=](StackAttribute a, int have, int want, const char* attrname) {
            auto vmax = std::get<3>(STACK_ENCODING.at(EI(a)));
            if (want > vmax) want = vmax;
            ensureValueMatch(have, want, attrname);
        };

        auto ensureMeleeability = [=](BattleHex bh, HexActMask mask, HexAction ha, const CStack* cstack, const char* attrname) {
            auto mv = mask.test(EI(ha));

            // if AMOVE is allowed, we must be able to reach hex
            // (no else -- we may still be able to reach it)
            if (mv == 1)
                ensureReachability(bh, 1, cstack, attrname);

            auto r_nbh = bh.cloneInDirection(BattleHex::EDir::RIGHT, false);
            auto l_nbh = bh.cloneInDirection(BattleHex::EDir::LEFT, false);
            auto nbh = BattleHex{};

            switch (ha) {
            break; case HexAction::AMOVE_TR: nbh = bh.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false);
            break; case HexAction::AMOVE_R: nbh = r_nbh;
            break; case HexAction::AMOVE_BR: nbh = bh.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false);
            break; case HexAction::AMOVE_BL: nbh = bh.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false);
            break; case HexAction::AMOVE_L: nbh = l_nbh;
            break; case HexAction::AMOVE_TL: nbh = bh.cloneInDirection(BattleHex::EDir::TOP_LEFT, false);
            break; case HexAction::AMOVE_2TR: nbh = r_nbh.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false);
            break; case HexAction::AMOVE_2R: nbh = r_nbh.cloneInDirection(BattleHex::EDir::RIGHT, false);
            break; case HexAction::AMOVE_2BR: nbh = r_nbh.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false);
            break; case HexAction::AMOVE_2BL: nbh = l_nbh.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false);
            break; case HexAction::AMOVE_2L: nbh = l_nbh.cloneInDirection(BattleHex::EDir::LEFT, false);
            break; case HexAction::AMOVE_2TL: nbh = l_nbh.cloneInDirection(BattleHex::EDir::TOP_LEFT, false);
            break; default:
                THROW_FORMAT("Unexpected HexAction: %d", EI(ha));
              break;
            }

            auto estacks = getAllStacksForSide(!EI(cstack->unitSide()));
            auto it = std::find_if(estacks.begin(), estacks.end(), [&nbh](auto stack) {
                return stack && stack->coversPos(nbh);
            });

            auto estack = it == estacks.end() ? nullptr : *it;

            // find id of `estack`
            // can't use std::distance, as estacks can contain nullptrs
            // for regular slots (0..6)
            int eindex = -1;
            for (auto &stack : estacks) {
                if (stack && estack) ++eindex;
                if (stack == estack) break;
            }

            if (mv) {
                expect(eindex < MAX_STACKS_PER_SIDE, "%s: =%d (bhex %d, nbhex %d), but nbhex stack has eindex=%d", attrname, bh.hex, nbh.hex, eindex);
                // must not pass "nbh" for defender position, as it could be its rear hex
                expect(cstack->isMeleeAttackPossible(cstack, estack, bh), "%s: =1 (bhex %d, nbhex %d), but VCMI says isMeleeAttackPossible=0", attrname, bh.hex, nbh.hex);
            }
            //  else {
            //     if (it != estacks.end()) {
            //         auto estack = *it;

            //         // MASK may prohibit attack, but hex may still be reachable
            //         if (checkReachable(bh, 1, cstack))
            //             // MASK may prohibita this specific attack from a reachable hex
            //             // it does not mean any attack is impossible
            //             expect(!cstack->isMeleeAttackPossible(cstack, estack, bh), "%s: =0 (bhex %d, nbhex %d), but bhex is reachable and VCMI says isMeleeAttackPossible=1", attrname, bh.hex, nbh.hex);
            //     }
            // }
        };

        auto ensureCorrectMaskOrNA = [=](BattleHex bh, int v, const CStack* cstack, const char* attrname) {
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
            ensureMeleeability(bh, mask, HexAction::AMOVE_2TR, cstack, (basename + "{AMOVE_2TR}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_2R, cstack, (basename + "{AMOVE_2R}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_2BR, cstack, (basename + "{AMOVE_2BR}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_2BL, cstack, (basename + "{AMOVE_2BL}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_2L, cstack, (basename + "{AMOVE_2L}").c_str());
            ensureMeleeability(bh, mask, HexAction::AMOVE_2TL, cstack, (basename + "{AMOVE_2TL}").c_str());
        };

        // ihex = 0, 1, 2, .. 164
        // ibase = 0, 15, 30, ... 2460
        for (int ihex=0; ihex < BF_SIZE; ihex++) {
            int x = ihex%15;
            int y = ihex/15;
            auto &hex = state->battlefield->hexes->at(y).at(x);
            auto bh = hex->bhex;
            expect(bh == BattleHex(x+1, y), "hex->bhex mismatch");

            auto ainfo = battle->getAccessibility();
            auto aa = ainfo.at(bh);

            for (int i=0; i < EI(HexAttribute::_count); i++) {
                auto attr = HexAttribute(i);
                auto v = hex->attrs.at(i);
                auto cstack = hexstacks.at(ihex);

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

                        if (battle->battleGetFortifications().wallsHealth == 0)
                            throw std::runtime_error("HEX.STATE_MASK: PASSABLE bit not set, but accessibility is neither OBSTACLE nor ALIVE_STACK (no walls)");

                        expect(aa == EAccessibility::DESTRUCTIBLE_WALL || aa == EAccessibility::GATE || aa == EAccessibility::UNAVAILABLE,
                            "HEX.STATE_MASK: PASSABLE bit not set (siege), but accessibility is %d", EI(aa));
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
                    //     expect(bh == BattleHex::GATE_INNER || bh == BattleHex::GATE_OUTER, "HEX.STATE_MASK: GATE bit is set on bhex#%d", bh.hex);
                    // }
                }
                break; case HA::ACTION_MASK: {
                    if(v == NULL_VALUE_UNENCODED) {
                        // NULL action mask happens if there are too many stacks
                        // and the active stack is one of the ignored stacks
                        // XXX: cstack may be null here (this may be a blank hex)
                        auto stacks = battle->battleGetMySide() == BattleSide::DEFENDER ? r_CStacksAll : l_CStacksAll;
                        expect(stacks.size() > MAX_STACKS_PER_SIDE, "HEX.ACTION_MASK: is null and hex->stack is null, but n_stacks is %d", stacks.size());
                    } else if (ended) {
                        expect(v == 0, "HEX.ACTION_MASK: battle ended, but action mask is %d", v);
                    } else {
                        ensureCorrectMaskOrNA(bh, v, astack, "HEX.ACTION_MASK");
                    }
                }
                break; case HA::STACK_ID: {
                    // XXX: isNA may fail for ignored stacks (4th+ extra stacks)
                    //      If cstack is nullptr => v MUST be -1
                    //      If cstack != nullptr => v MAY be -1 (for ignored stacks)
                    if (cstack == nullptr) {
                        expect(v == NULL_VALUE_UNENCODED, "HEX.STACK_ID: v=%d, but cstack is nullptr", v);
                        break;
                    }

                    auto side = cstack->unitSide();
                    auto slot = cstack->unitSlot();

                    if (slot >= 0) {
                        // regular stack -- STACK_ID is 0..6 (or 10..16 for R side)
                        auto id = side == BattleSide::DEFENDER ? slot + MAX_STACKS_PER_SIDE : slot;
                        expect(v == id, "HEX.STACK_ID: =%d, but cstack->unitSlot()=%d and side=%d", v, slot, side);
                        break;
                    }

                    // special (extra) stack (summoned creature or war machine)
                    expect(
                        slot == SlotID::WAR_MACHINES_SLOT || slot == SlotID::SUMMONED_SLOT_PLACEHOLDER,
                        "unexpected unitSlot: %d", slot
                    );

                    auto extras = side == BattleSide::DEFENDER ? r_CStacksExtra : l_CStacksExtra;
                    auto it = std::find(extras.begin(), extras.end(), cstack);
                    if (it == extras.end()) {
                        THROW_FORMAT("HEX.STACK_ID: cstack is an extra stack (unitSlot()=%d), but did not find it among the extra stacks", slot);
                    }

                    //  A special stack has STACK_ID of either:
                    //  a) 7..9 (or 17..19 for R side)
                    //  b) 0..6 (or 10..16 for R side)
                    //  c) -1 (too many other stacks in the army)
                    //
                    // Case a) is for when cstack is within the first 3 "extra" stacks.
                    // Case b) is for when cstack is 4th or later "extra" stack,
                    //          but there are free IDs 0..6 which it could use.
                    // Case c) is same as b), but no free IDs 0..6 -- stack is ignored.
                    //

                    // First see if the extra stack a "designated" extra STACK_ID
                    // Check a)
                    int extraSeqNumber = std::distance(extras.begin(), it);
                    if (extraSeqNumber < MAX_STACKS_PER_SIDE - 7) {
                        // stack is within the first 3 "extra" stacks
                        // => it will be assigned STACK_ID of 7..9 (or 17..19 for R side)
                        int idForSide = 7 + extraSeqNumber;
                        int globalID = side == BattleSide::DEFENDER ? 10 + idForSide : idForSide;
                        expect(v == globalID, "HEX.STACK_ID: v=%d, but expected %d (extraSeqNumber=%d)", v, globalID, extraSeqNumber);
                        break;
                    }

                    // The extra stack is at least 4th or later in line
                    // => Check b)
                    auto regulars = cstack->unitSide() == BattleSide::DEFENDER ? r_CStacks : l_CStacks;
                    auto regularIds = std::vector<int> {};
                    for (auto s : regulars) if (s) regularIds.push_back(s->unitSlot());

                    auto nFreeRegularIds = 7 - regularIds.size();
                    // 4th extra stack would take first free regular id, 5th - the second, etc.
                    auto nFreeRegularIdsNeeded = (extraSeqNumber+1) - (MAX_STACKS_PER_SIDE - 7);
                    if (nFreeRegularIds >= nFreeRegularIdsNeeded) {
                        // the extra stack was assigned a regular ID
                        auto idForSide = v % MAX_STACKS_PER_SIDE;

                        expect(idForSide < 7,
                            "HEX.STACK_ID: v=%d, i.e. idForSide=%d, but expected idForSide < 7 (extraSeqNumber=%d, nFreeRegularIds=%d)",
                            v, idForSide, extraSeqNumber, nFreeRegularIds
                        );

                        // the ID our extra stack uses must NOT be used by a regular stack
                        auto cstackAtID = regulars.at(idForSide);
                        expect(cstackAtID == nullptr,
                            "HEX.STACK_ID: v=%d, i.e. idForSide=%d, but there's a stack at this regular id",
                            v, idForSide
                        );

                        break;
                    }

                    // There are no free regular ids to use
                    // => Check c)
                    expect(v == NULL_VALUE_UNENCODED,
                        "HEX.STACK_ID: v=%d, but expected NULL (extraSeqNumber=%d, nFreeRegularIds=%d)",
                        v, extraSeqNumber, nFreeRegularIds
                    );
                }
                break; default:
                    THROW_FORMAT("Unexpected HexAttribute: %d", EI(attr));
                }
            }
        }

        for (int side : {0, 1}) {
            for (int istack=0; istack < MAX_STACKS_PER_SIDE; istack++) {
                auto stack = state->battlefield->stacks->at(side).at(istack);

                if (!stack)
                    continue;

                const CStack *cstack = stack->cstack;

                expect(cstack, "cstack is nullptr, but stack is present");
                expect(stack->cstack, "stack->cstack is nullptr, but cstack is present");

                auto [x, y] = Hex::CalcXY(cstack->getPosition());
                auto bonuses = cstack->getAllBonuses(Selector::all, nullptr);

                // See docs/modders/Bonus/Bonus_Types.md
                auto spell_after_attack_bonuses = BonusList();
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

                // XXX: alternative (simpler?) implementation
                // TODO: compare performance
                // auto castchance = [&spell_after_attack_bonuses](std::vector<SpellIDBase::Type> spellids) {
                //     int res = 0;
                //     for (auto &bonus : spell_after_attack_bonuses) {
                //         auto st = bonus->subtype;
                //         for (auto sid : spellids) {
                //             if (st == sid) res += bonus->val;
                //         }
                //     }
                //     return res;
                // };

                // See docs/modders/Game_Identifiers.md
                auto hate_bonuses = BonusList();
                bonuses->getBonuses(hate_bonuses, Selector::type()(BonusType::HATE), nullptr);

                auto find_bonus = [&bonuses](BonusType t) {
                    return bonuses->getFirst(Selector::type()(t));
                };

                // auto find_bonus2 = [&bonuses](BonusType t1, BonusSubtypeID t2) {
                //     return bonuses->getFirst(Selector::typeSubtype(t1, t2));
                // };

                auto has_bonus = [&find_bonus](BonusType t) {
                    return !!find_bonus(t);
                };

                // auto has_bonus2 = [&find_bonus2](BonusType t1, BonusSubtypeID t2) {
                //     return !!find_bonus2(t1, t2);
                // };

                auto get_bonus_duration = [&find_bonus](BonusType t) {
                    auto b = find_bonus(t);
                    if (!b)
                        return 0;

                    if (b->duration & BonusDuration::PERMANENT)
                        return 100;
                    if (b->duration & BonusDuration::N_TURNS)
                        return static_cast<int>(b->turnsRemain);

                    return 0;
                };


                // Now validate all attributes...
                for (int i=0; i < EI(SA::_count); i++) {
                    auto a = SA(i);
                    auto v = stack->attr(a);
                    auto [_, e, n, vmax] = STACK_ENCODING.at(i);
                    (void)_; // Suppress unused variable warning

                    int want = -666;

                    switch(a) {
                    break; case SA::ID:
                    break; case SA::Y_COORD:
                        expect(v == y, "STACK.Y_COORD: %d != %d", v, y);
                    break; case SA::X_COORD:
                        expect(v == x, "STACK.X_COORD: %d != %d", v, x);
                    break; case SA::SIDE:
                        ensureStackValueMatch(a, v, EI(cstack->unitSide()), "STACK.SIDE");
                    break; case SA::QUANTITY:
                        ensureStackValueMatch(a, v, std::round(STACK_QTY_MAX * std::tanh(float(cstack->getCount()) / STACK_QTY_MAX)), "STACK.AI_VALUE");
                    break; case SA::ATTACK:
                        ensureStackValueMatch(a, v, cstack->getAttack(false), "STACK.ATTACK");
                    break; case SA::DEFENSE:
                        ensureStackValueMatch(a, v, cstack->getDefense(false), "STACK.DEFENSE");
                    break; case SA::SHOTS:
                        ensureStackValueMatch(a, v, cstack->shots.available(), "STACK.SHOTS");
                    break; case SA::DMG_MIN:
                        ensureStackValueMatch(a, v, cstack->getMinDamage(false), "STACK.DMG_MIN");
                    break; case SA::DMG_MAX:
                        ensureStackValueMatch(a, v, cstack->getMaxDamage(false), "STACK.DMG_MAX");
                    break; case SA::HP:
                        ensureStackValueMatch(a, v, cstack->getMaxHealth(), "STACK.HP");
                    break; case SA::HP_LEFT:
                        ensureStackValueMatch(a, v, cstack->getFirstHPleft(), "STACK.HP_LEFT");
                    break; case SA::SPEED:
                        ensureStackValueMatch(a, v, cstack->getMovementRange(), "STACK.SPEED");
                    break; case SA::ACTSTATE:
                        if (cstack->willMove()) {
                            if (cstack->waited()) {
                                ensureStackValueMatch(a, v, EI(StackActState::WAITING), "STACK.ACTSTATE");
                            } else {
                                ensureStackValueMatch(a, v, EI(StackActState::READY), "STACK.ACTSTATE");
                            }
                        } else {
                            ensureStackValueMatch(a, v, EI(StackActState::DONE), "STACK.ACTSTATE");
                        }
                    break; case SA::SLEEPING:
                        if (cstack->unitType()->getId() == CreatureID::AMMO_CART)
                            ensureStackValueMatch(a, v, 0, "STACK.SLEEPING");
                        else
                            ensureStackValueMatch(a, v, get_bonus_duration(BonusType::NOT_ACTIVE), "STACK.SLEEPING");
                    break; case SA::QUEUE_POS:
                        // at battle end, queue is messed up
                        // (the stack that dealt the killing blow is still "active", but not on 0 pos)
                        if (ended)
                            break;

                        if (v == 0)
                            expect(cstack == astack, "STACK.QUEUE_POS: =0 but is different from astack");
                        else
                            expect(cstack != astack, "STACK.QUEUE_POS: =%d but is same as astack", v);
                    break; case SA::RETALIATIONS_LEFT:
                        // not verifying unlimited retals, just check 0
                        // XXX: if stack is inactive (e.g. blinded), retals left may go negative
                        if (v <= 0)
                            expect(!cstack->ableToRetaliate(), "STACK.RETALIATIONS_LEFT: =%d but ableToRetaliate=true", v);
                        else
                            expect(cstack->ableToRetaliate(), "STACK.RETALIATIONS_LEFT: =%d but ableToRetaliate=false", v);
                    break; case SA::IS_WIDE:
                        ensureStackValueMatch(a, v, cstack->occupiedHex().isAvailable(), "STACK.IS_WIDE");
                    break; case SA::AI_VALUE:
                        ensureStackValueMatch(a, v, std::round(STACK_VALUE_MAX * std::tanh(float(cstack->unitType()->getAIValue()) / STACK_VALUE_MAX)), "STACK.AI_VALUE");
                    break; case SA::BLIND_LIKE_ATTACK:
                        want = castchance({SpellID::BLIND, SpellID::STONE_GAZE, SpellID::PARALYZE});
                        ensureStackValueMatch(a, v, want, "STACK.BLIND_LIKE_ATTACK");
                    break; case SA::FLYING:
                        want = has_bonus(BonusType::FLYING);
                        ensureStackValueMatch(a, v, want, "STACK.FLYING");
                    break; case SA::ADDITIONAL_ATTACK:
                        want = has_bonus(BonusType::ADDITIONAL_ATTACK);
                        ensureStackValueMatch(a, v, want, "STACK.ADDITIONAL_ATTACK");
                    break; case SA::NO_MELEE_PENALTY:
                        want = has_bonus(BonusType::NO_MELEE_PENALTY);
                        ensureStackValueMatch(a, v, want, "STACK.NO_MELEE_PENALTY");
                    break; case SA::TWO_HEX_ATTACK_BREATH:
                        want = has_bonus(BonusType::TWO_HEX_ATTACK_BREATH);
                        ensureStackValueMatch(a, v, want, "STACK.TWO_HEX_ATTACK_BREATH");
                    break; case SA::BLOCKED:
                        want = cstack->canShoot() && battle->battleIsUnitBlocked(cstack);
                        ensureStackValueMatch(a, v, want, "STACK.BLOCKED");
                    break; case SA::BLOCKING:
                        want = 0;
                        for (auto &adjstack : battle->battleAdjacentUnits(cstack)) {
                            if (adjstack->unitSide() != cstack->unitSide() && adjstack->canShoot() && battle->battleIsUnitBlocked(adjstack)) {
                                want = 1;
                                break;
                            }
                        }
                        ensureStackValueMatch(a, v, want, "STACK.BLOCKING");
                    break; case SA::ESTIMATED_DMG:
                        if (ended)
                            break;

                        if (!astack || astack->unitSide() == cstack->unitSide()) {
                           want = 0;
                        } else {
                            auto dmgrange = battle->battleEstimateDamage(astack, cstack, 0, nullptr);
                            auto dmgAsHPFrac = 0.5*(dmgrange.damage.max + dmgrange.damage.min) / cstack->getAvailableHealth();
                            auto dmgAsHPPercent = static_cast<int>(std::round(100 * dmgAsHPFrac));
                            want = std::clamp<int>(dmgAsHPPercent, 0, 100);
                        }
                        ensureStackValueMatch(a, v, want, "STACK.ESTIMATED_DMG");
                    break; case SA::BLOCKS_RETALIATION:
                        want = has_bonus(BonusType::BLOCKS_RETALIATION);
                        ensureStackValueMatch(a, v, want, "STACK.BLOCKS_RETALIATION");
                    break; default:
                        THROW_FORMAT("Unexpected StackAttribute: %d", EI(a));
                    }
                }
            }
        }
        // TODO: validate global state
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
        auto misc = supdata->getMisc();
        auto hexes = supdata->getHexes();
        auto allstacks = supdata->getStacks();
        auto color = supdata->getColor();

        IStack* astack = nullptr;
        auto idstacks = std::array<IStack*, MAX_STACKS>{};

        // find an active hex (i.e. with active stack on it)
        for (auto &sidestacks : allstacks) {
            for (auto &stack : sidestacks) {
                if (stack) {
                    idstacks.at(stack->getAttr(SA::ID)) = stack;
                    if (stack->getAttr(SA::QUEUE_POS) == 0) {
                        // can be same stack (2-hex)
                        expect(astack != stack, "two active stacks found");
                        astack = stack;
                    }
                }
            }
        }

        if (!astack)
            logAi->warn("could not find an active stack. Are there more than %d stacks in this army?", MAX_STACKS_PER_SIDE);

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
            row << " (kills: " << alog->getUnitsKilled() << ", value: " << alog->getValueKilled() << ")";

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

        //     ₀▏₁▏₂▏₃▏₄▏₅▏₆▏₇▏₈▏₉▏₀▏₁▏₂▏₃▏₄
        //  ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃          Player: RED
        // ₀┨  0▏0▏○▏○▏✶▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○ ┠₀    Last action:
        // ₁┨ ○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○  ┠₁      DMG dealt: 0
        // ₂┨  1▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○ ┠₂   Units killed: 0
        // ₃┨ ✶▕○▕▦▕▦▕▦▕▦▕▦▕○▕○▕○▕○▕○▕○▕○▕○  ┠₃   Value killed: 0
        // ₄┨  2▏2▏○▏○▏○▏○▏▦▏▦▏▦▏○▏○▏○▏○▏○▏○ ┠₄   DMG received: 0
        // ₅┨ 3▕3▕○▕○▕○▕○▕○▕○▕○▕▦▕○▕○▕○▕3▕3  ┠₅     Units lost: 0
        // ₆┨  4▏4▏○▏○▏○▏○▏○▏○▏✶▏▦▏▦▏○▏○▏○▏○ ┠₆     Value lost: 0
        // ₇┨ ○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕▦▕○▕○▕○  ┠₇  Battle result:
        // ₈┨  5▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○ ┠₈
        // ₉┨ ○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○▕○  ┠₉
        // ₀┨  6▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏○▏✶▏○ ┠₀
        //  ┃▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁┃
        //   ▕⁰▕¹▕²▕³▕⁴▕⁵▕⁶▕⁷▕⁸▕⁹▕⁰▕¹▕²▕³▕⁴▕


        // s=special; can be any number, slot is always 7 (SPECIAL), visualized A,B,C...

        static_assert(MAX_STACKS == 20, "code assumes 20 stacks");

        auto tablestartrow = lines.size();

        lines.emplace_back() << "    ₀▏₁▏₂▏₃▏₄▏₅▏₆▏₇▏₈▏₉▏₀▏₁▏₂▏₃▏₄";
        lines.emplace_back() << " ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃ ";

        static std::array<std::string, 10> nummap{"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"};

        bool addspace = true;

        auto seenstacks = std::bitset<MAX_STACKS>(0);
        bool divlines = true;

        // y even "▏"
        // y odd "▕"

        for (int y=0; y < BF_YMAX; y++) {
            for (int x=0; x < BF_XMAX; x++) {
                auto sym = std::string("?");
                auto &hex = hexes.at(y).at(x);

                IStack* stack = nullptr;

                if (hex->getAttr(HA::STACK_ID) != NULL_VALUE_UNENCODED) {
                    stack = idstacks.at(hex->getAttr(HA::STACK_ID));
                    expect(stack, "stack with ID=%d not found", hex->getAttr(HA::STACK_ID));
                }

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

                if (hex->getAttr(HA::STACK_ID) >= 0) {
                    auto seen = seenstacks.test(stack->getAttr(SA::ID));
                    sym = std::string(1, stack->getAlias());
                    col = stack->getAttr(SA::SIDE) ? bluecol : redcol;

                    if (stack->getAttr(SA::QUEUE_POS) == 0) // && !supdata->getIsBattleEnded()
                        col += activemod;

                    if (stack->getAttr(SA::IS_WIDE) > 0 && !seen) {
                        if (stack->getAttr(SA::SIDE) == 0) {
                            sym += "↠";
                            addspace = false;
                        } else if(stack->getAttr(SA::SIDE) == 1 && hex->getAttr(HA::X_COORD) < 14) {
                            sym += "↞";
                            addspace = false;
                        }
                    }

                    seenstacks.set(stack->getAttr(SA::ID));
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
            break; case 3: name = "DMG dealt"; value = std::to_string(supdata->getDmgDealt());
            break; case 4: name = "Units killed"; value = std::to_string(supdata->getUnitsKilled());
            break; case 5: name = "Value killed"; value = std::to_string(supdata->getValueKilled());
            break; case 6: name = "DMG received"; value = std::to_string(supdata->getDmgReceived());
            break; case 7: name = "Units lost"; value = std::to_string(supdata->getUnitsLost());
            break; case 8: name = "Value lost"; value = std::to_string(supdata->getValueLost());
            break; case 9: {
                // XXX: if there's a draw, this text will be incorrect
                auto restext = supdata->getIsVictorious()
                    ? (side ? bluecol + "BLUE WINS" : redcol + "RED WINS")
                    : (side ? redcol + "RED WINS" : bluecol + "BLUE WINS" );

                name = "Battle result"; value = supdata->getIsBattleEnded() ? (restext + nocol) : "";
            }
            break; case 10: name = "Army value (L)"; value = boost::str(boost::format("%d (%d remaining)") % misc->getInitialArmyValueLeft() % misc->getCurrentArmyValueLeft());
            break; case 11: name = "Army value (R)"; value = boost::str(boost::format("%d (%d remaining)") % misc->getInitialArmyValueRight() % misc->getCurrentArmyValueRight());
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

        // All cell text is aligned right
        auto colwidths = std::array<int, 4 + MAX_STACKS>{};
        colwidths.fill(4); // default col width
        colwidths.at(0) = 16; // header col

        // Divider column indexes
        auto divcolids = {1, MAX_STACKS_PER_SIDE+2, MAX_STACKS+3};

        for (int i : divcolids)
            colwidths.at(i) = 2; // divider col

        // {Attribute, name, colwidth}
        const auto rowdefs = std::vector<RowDef> {
            RowDef{SA::ID, "Stack #"},
            RowDef{SA::X_COORD, ""},  // divider row
            RowDef{SA::QUANTITY, "Qty"},
            RowDef{SA::ATTACK, "Attack"},
            RowDef{SA::DEFENSE, "Defense"},
            RowDef{SA::SHOTS, "Shots"},
            RowDef{SA::DMG_MIN, "Dmg (min)"},
            RowDef{SA::DMG_MAX, "Dmg (max)"},
            RowDef{SA::HP, "HP"},
            RowDef{SA::HP_LEFT, "HP left"},
            RowDef{SA::SPEED, "Speed"},
            RowDef{SA::ACTSTATE, "Act state"},
            RowDef{SA::QUEUE_POS, "Queue"},
            RowDef{SA::RETALIATIONS_LEFT, "Ret. left"},
            RowDef{SA::ESTIMATED_DMG, "Est. DMG%"},
            RowDef{SA::AI_VALUE, "Value"},
            RowDef{SA::BLIND_LIKE_ATTACK, "Blind* chance"},
            // 2 values per column to avoid too long table
            RowDef{SA::Y_COORD, "Attacks/Retals"},
            RowDef{SA::Y_COORD, "Blocked/ing"},
            RowDef{SA::Y_COORD, "Fly/Sleep"},
            RowDef{SA::Y_COORD, "No Retal/Melee"},
            RowDef{SA::Y_COORD, "Wide/Breath"},
            RowDef{SA::X_COORD, ""},  // divider row
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
            if (a == SA::X_COORD) { // divider row
                table.push_back(divrow);
                continue;
            }

            auto row = TableRow{};

            // Header col
            row.at(0) = {nocol, colwidths.at(0), aname};

            // Div cols
            for (int i : {1, 2+MAX_STACKS_PER_SIDE, int(colwidths.size()-1)})
                row.at(i) = {nocol, colwidths.at(i), "|"};

            // Stack cols
            for (auto side : {0, 1}) {
                auto &sidestacks = allstacks.at(side);
                for (int i=0; i<sidestacks.size(); ++i) {
                    auto stack = sidestacks.at(i);
                    auto color = nocol;
                    std::string value = "";

                    if (stack) {
                        color = stack->getAttr(SA::SIDE) ? bluecol : redcol;
                        if (a == SA::ID) {
                            value = std::string(1, stack->getAlias());
                        } else if (a == SA::ACTSTATE) {
                            switch(StackActState(stack->getAttr(a))) {
                            break; case StackActState::READY: value = "R";
                            break; case StackActState::WAITING: value = "W";
                            break; case StackActState::DONE: value = "D";
                            break; default:
                                throw std::runtime_error("Unexpected actstate:: " + std::to_string(EI(stack->getAttr(a))));
                            }
                        } else if (a == SA::AI_VALUE && stack->getAttr(a) >= 1000) {
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(1) << (stack->getAttr(a) / 1000.0);
                            value = oss.str();
                            value[value.size()-2] = 'k';
                            if (value.rfind("K0") == (value.size() - 2))
                                value.resize(value.size() - 1);
                        } else if (a == SA::Y_COORD) {
                            auto fmt = boost::format("%d/%d");
                            switch(specialcounter) {
                            break; case 0: value = boost::str(fmt % (stack->getAttr(SA::ADDITIONAL_ATTACK) ? 2 : 1) % stack->getAttr(SA::RETALIATIONS_LEFT));
                            break; case 1: value = boost::str(fmt % stack->getAttr(SA::BLOCKED) % stack->getAttr(SA::BLOCKING));
                            break; case 2: value = boost::str(fmt % stack->getAttr(SA::FLYING) % stack->getAttr(SA::SLEEPING));
                            break; case 3: value = boost::str(fmt % stack->getAttr(SA::BLOCKS_RETALIATION) % stack->getAttr(SA::NO_MELEE_PENALTY));
                            break; case 4: value = boost::str(fmt % stack->getAttr(SA::IS_WIDE) % stack->getAttr(SA::TWO_HEX_ATTACK_BREATH));
                            break; default:
                                THROW_FORMAT("Unexpected specialcounter: %d", specialcounter);
                            }
                        } else {
                            value = std::to_string(stack->getAttr(a));
                        }

                        if (stack->getAttr(SA::QUEUE_POS) == 0 && !supdata->getIsBattleEnded()) color += activemod;
                    }

                    auto colid = 2 + i + side + (MAX_STACKS_PER_SIDE*side);
                    row.at(colid) = {color, colwidths.at(colid), value};
                }
            }

            if (a == SA::Y_COORD)
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
