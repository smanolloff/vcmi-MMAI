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

#include "battle/CPlayerBattleCallback.h"
#include "networkPacks/PacksForClientBattle.h"

#include "BAI/v8/encoder.h"
#include "BAI/v8/hexaction.h"
#include "BAI/v8/state.h"
#include "BAI/v8/supplementary_data.h"
#include "schema/v8/constants.h"
#include "schema/v8/types.h"

#include <memory>

namespace MMAI::BAI::V8 {
    using HA = HexAttribute;
    using SA = StackAttribute;

    //
    // Prevent human errors caused by the Stack / Hex attr overlap
    //
    static_assert(EI(HA::STACK_SIDE)                    == EI(SA::SIDE) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_QUANTITY)                == EI(SA::QUANTITY) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_ATTACK)                  == EI(SA::ATTACK) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DEFENSE)                 == EI(SA::DEFENSE) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_SHOTS)                   == EI(SA::SHOTS) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_MIN)                 == EI(SA::DMG_MIN) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_MAX)                 == EI(SA::DMG_MAX) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_HP)                      == EI(SA::HP) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_HP_LEFT)                 == EI(SA::HP_LEFT) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_SPEED)                   == EI(SA::SPEED) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_QUEUE_POS)               == EI(SA::QUEUE_POS) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_ONE)               == EI(SA::VALUE_ONE) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_FLAGS)                   == EI(SA::FLAGS) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_REL)               == EI(SA::VALUE_REL) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_REL0)              == EI(SA::VALUE_REL0) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_KILLED_REL)        == EI(SA::VALUE_KILLED_REL) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_KILLED_ACC_REL0)   == EI(SA::VALUE_KILLED_ACC_REL0) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_LOST_REL)          == EI(SA::VALUE_LOST_REL) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_VALUE_LOST_ACC_REL0)     == EI(SA::VALUE_LOST_ACC_REL0) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_DEALT_REL)           == EI(SA::DMG_DEALT_REL) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_DEALT_ACC_REL0)      == EI(SA::DMG_DEALT_ACC_REL0) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_RECEIVED_REL)        == EI(SA::DMG_RECEIVED_REL) + STACK_ATTR_OFFSET);
    static_assert(EI(HA::STACK_DMG_RECEIVED_ACC_REL0)   == EI(SA::DMG_RECEIVED_ACC_REL0) + STACK_ATTR_OFFSET);

    // static
    std::vector<float> State::InitNullStack() {
        auto res = std::vector<float> {};
        for (int i=0; i<EI(StackAttribute::_count); ++i)
            Encoder::Encode(HA(STACK_ATTR_OFFSET + i), NULL_VALUE_UNENCODED, res);
        return res;
    };

    std::tuple<int, int, int, int> CalcGlobalStats(const CPlayerBattleCallback *battle) {
        int vl = 0;
        int vr = 0;
        int hl = 0;
        int hr = 0;
        for (auto &stack : battle->battleGetStacks()) {
            auto v = stack->getCount() * Stack::CalcValue(stack->unitType());
            auto h = stack->getAvailableHealth();
            // std::cout << "[" << EI(stack->unitSide()) << "] v=" << v << ", h=" << h << ", vl=" << vl << ", vr=" << vr << "\n";

            if (stack->unitSide() == BattleSide::ATTACKER) {
                vl += v;
                hl += h;
            } else {
                vr += v;
                hr += h;
            }
        }

        return {vl, vr, hl, hr};
    }

    State::State(const int version__, const std::string colorname_, const CPlayerBattleCallback* battle_)
    : version_(version__)
    , colorname(colorname_)
    , battle(battle_)
    , side(battle_->battleGetMySide())
    , nullstack(InitNullStack())
    {
        auto [vl, vr, hl, hr] = CalcGlobalStats(battle);
        lgstats = std::make_unique<GlobalStats>(vl, hl);
        rgstats = std::make_unique<GlobalStats>(vr, hr);
        battlefield = Battlefield::Create(battle_, nullptr, lgstats.get(), rgstats.get(), {}, false);

        bfstate.reserve(Schema::V8::BATTLEFIELD_STATE_SIZE);
        actmask.reserve(Schema::V8::N_ACTIONS);
    }

    void State::onActiveStack(const CStack* astack) {
        lgstats->dmgDealtNow = 0;
        rgstats->dmgDealtNow = 0;
        lgstats->dmgReceivedNow = 0;
        rgstats->dmgReceivedNow = 0;
        lgstats->valueKilledNow = 0;
        rgstats->valueKilledNow = 0;
        lgstats->valueLostNow = 0;
        rgstats->valueLostNow = 0;

        for (auto &[cstack, ss] : stacksStats) {
            ss.dmgDealtNow = 0;
            ss.dmgReceivedNow = 0;
            ss.valueKilledNow = 0;
            ss.valueLostNow = 0;
        }

        auto [vl, vr, hl, hr] = CalcGlobalStats(battle);
        lgstats->valueNow = vl;
        rgstats->valueNow = vr;
        lgstats->hpNow = hl;
        rgstats->hpNow = hr;

        for (auto &al : attackLogs) {
            if (al->attacker) {
                stacksStats[al->attacker->cstack].dmgDealtNow += al->dmg;
                stacksStats[al->attacker->cstack].dmgDealtTotal += al->dmg;
                stacksStats[al->attacker->cstack].valueKilledNow += al->value;
                stacksStats[al->attacker->cstack].valueKilledTotal += al->value;
                if (al->attacker->attr(SA::SIDE) == EI(BattleSide::ATTACKER)) {
                    lgstats->dmgDealtNow += al->dmg;
                    lgstats->dmgDealtTotal += al->dmg;
                    lgstats->valueKilledNow += al->value;
                    lgstats->valueKilledTotal += al->value;
                } else {
                    rgstats->dmgDealtNow += al->dmg;
                    rgstats->dmgDealtTotal += al->dmg;
                    rgstats->valueKilledNow += al->value;
                    rgstats->valueKilledTotal += al->value;
                }
            }

            ASSERT(al->defender, "AttackLog defender is nullptr!");

            stacksStats[al->defender->cstack].dmgReceivedNow += al->dmg;
            stacksStats[al->defender->cstack].dmgReceivedTotal += al->dmg;
            stacksStats[al->defender->cstack].valueLostNow += al->value;
            stacksStats[al->defender->cstack].valueLostTotal += al->value;

            if (al->defender->attr(SA::SIDE) == EI(BattleSide::ATTACKER)) {
                lgstats->dmgReceivedNow += al->dmg;
                lgstats->dmgReceivedTotal += al->dmg;
                lgstats->valueLostNow += al->value;
                lgstats->valueLostTotal += al->value;
            } else {
                rgstats->dmgDealtNow += al->dmg;
                rgstats->dmgDealtTotal += al->dmg;
                rgstats->valueKilledNow += al->value;
                rgstats->valueKilledTotal += al->value;
            }

        }

        battlefield = Battlefield::Create(battle, astack, lgstats.get(), rgstats.get(), stacksStats, isMorale);
        isMorale = false;

        supdata = std::make_unique<SupplementaryData>(
            colorname,
            Side(side),
            lgstats.get(),
            rgstats.get(),
            battlefield.get(),
            attackLogs  // store the logs since last turn
        );

        attackLogs.clear(); // accumulate new logs until next turn
        bfstate.clear();
        actmask.clear();

        for (int i=0; i<EI(NonHexAction::count); i++) {
            switch (NonHexAction(i)) {
            // TODO: handle cases where retreat is not allowed (shackles of war, no hero, etc.)
            break; case NonHexAction::RETREAT: actmask.push_back(true);
            break; case NonHexAction::WAIT: actmask.push_back(battlefield->astack && !battlefield->astack->cstack->waitedThisTurn);
            break; default:
                THROW_FORMAT("Unexpected NonHexAction: %d", i);
            }
        }

        encodeMisc();

        for (auto &hexrow : *battlefield->hexes)
            for (auto &hex : hexrow)
                encodeHex(hex.get());

        verify();
    }

    void State::encodeMisc() {
        Encoder::Encode(MA::BATTLE_SIDE, EI(battle->battleGetMySide()), bfstate);

        if (supdata->ended) {
            auto winner = EI(battle->battleGetMySide()) ? supdata->victory : !supdata->victory;
            Encoder::Encode(MA::BATTLE_WINNER, winner, bfstate);
        } else {
            Encoder::Encode(MA::BATTLE_WINNER, NULL_VALUE_UNENCODED, bfstate);
        }

        auto l = lgstats.get();
        auto r = rgstats.get();
        auto bf_valueNow = l->valueNow + r->valueNow;
        auto bf_valueStart = l->valueStart + rgstats->valueStart;

        Encoder::Encode(MA::BFIELD_VALUE_NOW_REL0,       bf_valueNow / bf_valueStart, bfstate);
        Encoder::Encode(MA::ARMY_VALUE_L_NOW_REL,        l->valueNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::ARMY_VALUE_R_NOW_REL,        r->valueNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::ARMY_VALUE_L_NOW_REL0,       l->valueNow / bf_valueStart, bfstate);
        Encoder::Encode(MA::ARMY_VALUE_R_NOW_REL0,       r->valueNow / bf_valueStart, bfstate);
        Encoder::Encode(MA::VALUE_KILLED_LEFT_REL,       l->valueKilledNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::VALUE_KILLED_RIGHT_REL,      r->valueKilledNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::VALUE_KILLED_LEFT_ACC_REL0,  l->valueKilledTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::VALUE_KILLED_RIGHT_ACC_REL0, r->valueKilledTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::VALUE_LOST_LEFT_REL,         l->valueLostNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::VALUE_LOST_RIGHT_REL,        r->valueLostNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::VALUE_LOST_LEFT_ACC_REL0,    l->valueLostTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::VALUE_LOST_RIGHT_ACC_REL0,   r->valueLostTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::DMG_DEALT_LEFT_REL,          l->dmgDealtNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::DMG_DEALT_RIGHT_REL,         r->dmgDealtNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::DMG_DEALT_LEFT_ACC_REL0,     l->dmgDealtTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::DMG_DEALT_RIGHT_ACC_REL0,    r->dmgDealtTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::DMG_TAKEN_LEFT_REL,          l->dmgReceivedNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::DMG_TAKEN_RIGHT_REL,         r->dmgReceivedNow / bf_valueNow, bfstate);
        Encoder::Encode(MA::DMG_TAKEN_LEFT_ACC_REL0,     l->dmgReceivedTotal / bf_valueStart, bfstate);
        Encoder::Encode(MA::DMG_TAKEN_RIGHT_ACC_REL0,    r->dmgReceivedTotal / bf_valueStart, bfstate);
    }

    void State::encodeHex(Hex* hex) {
        // Battlefield state
        for (int i=0; i<EI(HA::_count); ++i)
            Encoder::Encode(HA(i), hex->attrs.at(i), bfstate);

        // Action mask
        for (int m=0; m<hex->actmask.size(); ++m)
            actmask.push_back(hex->actmask.test(m));
    }

    void State::verify() {
        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE, "unexpected bfstate.size(): " + std::to_string(bfstate.size()));
        ASSERT(actmask.size() == N_ACTIONS, "unexpected actmask.size(): " + std::to_string(actmask.size()));
    }

    void State::onBattleStacksAttacked(const std::vector<BattleStackAttacked> &bsa) {
        auto stacks = battlefield->stacks;

        for(auto & elem : bsa) {
            auto cdefender = battle->battleGetStackByID(elem.stackAttacked, false);
            auto cattacker = battle->battleGetStackByID(elem.attackerID, false);

            ASSERT(cdefender, "defender cannot be NULL");
            // logAi->debug("Attack: %s -> %s (%d dmg, %d died)", attacker->getName(), defender->getName(), elem.damageAmount, elem.killedAmount);

            // there may have been no room for defender in the obs
            auto defender = std::find_if(stacks.begin(), stacks.end(), [&cdefender](std::shared_ptr<Stack> stack) {
                return cdefender == stack->cstack;
            });

            auto attacker = std::find_if(stacks.begin(), stacks.end(), [&cattacker](std::shared_ptr<Stack> stack) {
                return cattacker == stack->cstack;
            });

            attackLogs.push_back(std::make_shared<AttackLog>(
                // XXX: attacker can be NULL when an effect does dmg (eg. Acid)
                // XXX: attacker and/or defender can be NULL they can't fit in obs
                attacker != stacks.end() ? *attacker : nullptr,
                defender != stacks.end() ? *defender : nullptr,
                elem.damageAmount,
                elem.killedAmount,
                elem.killedAmount * Stack::CalcValue(cdefender->unitType())
            ));
        }
    }

    void State::onBattleTriggerEffect(const BattleTriggerEffect &bte) {
        if (static_cast<BonusType>(bte.effect) != BonusType::MORALE)
            return;

        auto stack = battle->battleGetStackByID(bte.stackID);
        isMorale = stack->unitSide() == side;
    }

    void State::onBattleEnd(const BattleResult *br) {
        onActiveStack(nullptr);
        supdata->ended = true;
        supdata->victory = (br->winner == battle->battleGetMySide());
    }
};
