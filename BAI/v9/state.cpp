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

#include "BAI/v9/encoder.h"
#include "BAI/v9/hexaction.h"
#include "BAI/v9/state.h"
#include "BAI/v9/supplementary_data.h"
#include "schema/v9/constants.h"
#include "schema/v9/types.h"

#include <memory>

namespace MMAI::BAI::V9 {
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
        int lv = 0;
        int lh = 0;
        int rv = 0;
        int rh = 0;
        for (auto &stack : battle->battleGetStacks()) {
            auto v = stack->getCount() * Stack::CalcValue(stack->unitType());
            auto h = stack->getAvailableHealth();
            // std::cout << "[" << EI(stack->unitSide()) << "] v=" << v << ", h=" << h << ", lv=" << lv << ", rv=" << rv << "\n";

            if (stack->unitSide() == BattleSide::ATTACKER) {
                lv += v;
                lh += h;
            } else {
                rv += v;
                rh += h;
            }
        }

        return {lv, lh, rv, rh};
    }

    State::State(const int version__, const std::string colorname_, const CPlayerBattleCallback* battle_)
    : version_(version__)
    , colorname(colorname_)
    , battle(battle_)
    , side(battle_->battleGetMySide())
    , nullstack(InitNullStack())
    {
        auto [lv, lh, rv, rh] = CalcGlobalStats(battle);
        lgstats = std::make_unique<GlobalStats>(lv, lh);
        rgstats = std::make_unique<GlobalStats>(rv, rh);
        battlefield = Battlefield::Create(battle_, nullptr, lgstats.get(), rgstats.get(), {}, false);

        bfstate.reserve(Schema::V9::BATTLEFIELD_STATE_SIZE);
        actmask.reserve(Schema::V9::N_ACTIONS);
    }

    void State::onActiveStack(const CStack* astack, CombatResult result) {
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

        auto [lv, lh, rv, rh] = CalcGlobalStats(battle);
        lgstats->valuePrev = lgstats->valueNow;
        lgstats->valueNow = lv;
        lgstats->hpPrev = lgstats->hpNow;
        lgstats->hpNow = lh;

        rgstats->valuePrev = rgstats->valueNow;
        rgstats->valueNow = rv;
        rgstats->hpPrev = rgstats->hpNow;
        rgstats->hpNow = rh;

        for (auto &al : attackLogs) {
            if (al->cattacker) {
                stacksStats[al->cattacker].dmgDealtNow += al->dmg;
                stacksStats[al->cattacker].dmgDealtTotal += al->dmg;
                stacksStats[al->cattacker].valueKilledNow += al->value;
                stacksStats[al->cattacker].valueKilledTotal += al->value;

                auto s = al->cattacker->unitSide() == BattleSide::LEFT_SIDE ? lgstats.get() : rgstats.get();
                s->dmgDealtNow += al->dmg;
                s->dmgDealtTotal += al->dmg;
                s->valueKilledNow += al->value;
                s->valueKilledTotal += al->value;
            }

            ASSERT(al->cdefender, "AttackLog cdefender is nullptr!");

            stacksStats[al->cdefender].dmgReceivedNow += al->dmg;
            stacksStats[al->cdefender].dmgReceivedTotal += al->dmg;
            stacksStats[al->cdefender].valueLostNow += al->value;
            stacksStats[al->cdefender].valueLostTotal += al->value;

            auto s = al->cdefender->unitSide() == BattleSide::LEFT_SIDE ? lgstats.get() : rgstats.get();
            s->dmgReceivedNow += al->dmg;
            s->dmgReceivedTotal += al->dmg;
            s->valueLostNow += al->value;
            s->valueLostTotal += al->value;
        }

        battlefield = Battlefield::Create(battle, astack, lgstats.get(), rgstats.get(), stacksStats, isMorale);
        isMorale = false;

        supdata = std::make_unique<SupplementaryData>(
            colorname,
            Side(side),
            lgstats.get(),
            rgstats.get(),
            battlefield.get(),
            attackLogs, // store the logs since last turn
            result
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

        encodeGlobal();
        encodePlayer(lgstats.get());
        encodePlayer(rgstats.get());

        for (auto &hexrow : *battlefield->hexes)
            for (auto &hex : hexrow)
                encodeHex(hex.get());

        // for (auto &link : battlefield->links)
        //     encodeLink(link);

        // printf("********** ENCODED %zu links\n", battlefield->links.size());
        verify();
    }

    void State::encodeGlobal() {
        Encoder::Encode(GA::BATTLE_SIDE, EI(battle->battleGetMySide()), bfstate);

        if (supdata->ended) {
            auto winner = EI(battle->battleGetMySide()) ? supdata->victory : !supdata->victory;
            Encoder::Encode(GA::BATTLE_WINNER, winner, bfstate);
        } else {
            Encoder::Encode(GA::BATTLE_WINNER, NULL_VALUE_UNENCODED, bfstate);
        }

        auto bf_valueNow = lgstats->valueNow + rgstats->valueNow;
        auto bf_valueStart = lgstats->valueStart + rgstats->valueStart;
        Encoder::Encode(GA::BFIELD_VALUE_NOW_REL0, 100 * bf_valueNow / bf_valueStart, bfstate);
    }

    void State::encodePlayer(const GlobalStats* s) {
        auto bf_valueNow = lgstats->valueNow + rgstats->valueNow;
        auto bf_valuePrev = lgstats->valuePrev + rgstats->valuePrev;
        auto bf_valueStart = lgstats->valueStart + rgstats->valueStart;

        // auto bf_hpNow = lgstats->hpNow + rgstats->hpNow;
        auto bf_hpPrev = lgstats->hpPrev + rgstats->hpPrev;
        auto bf_hpStart = lgstats->hpStart + rgstats->hpStart;

        Encoder::Encode(PA::ARMY_VALUE_NOW_REL,     100 * s->valueNow / bf_valueNow, bfstate);
        Encoder::Encode(PA::ARMY_VALUE_NOW_REL0,    100 * s->valueNow / bf_valueStart, bfstate);
        Encoder::Encode(PA::VALUE_KILLED_REL,       100 * s->valueKilledNow / bf_valuePrev, bfstate);
        Encoder::Encode(PA::VALUE_KILLED_ACC_REL0,  100 * s->valueKilledTotal / bf_valueStart, bfstate);
        Encoder::Encode(PA::VALUE_LOST_REL,         100 * s->valueLostNow / bf_valuePrev, bfstate);
        Encoder::Encode(PA::VALUE_LOST_ACC_REL0,    100 * s->valueLostTotal / bf_valueStart, bfstate);
        Encoder::Encode(PA::DMG_DEALT_REL,          100 * s->dmgDealtNow / bf_hpPrev, bfstate);
        Encoder::Encode(PA::DMG_DEALT_ACC_REL0,     100 * s->dmgDealtTotal / bf_hpStart, bfstate);
        Encoder::Encode(PA::DMG_RECEIVED_REL,       100 * s->dmgReceivedNow / bf_hpPrev, bfstate);
        Encoder::Encode(PA::DMG_RECEIVED_ACC_REL0,  100 * s->dmgReceivedTotal / bf_hpStart, bfstate);
    }

    void State::encodeHex(const Hex* hex) {
        // Battlefield state
        for (int i=0; i<EI(HA::_count); ++i)
            Encoder::Encode(HA(i), hex->attrs.at(i), bfstate);

        // Action mask
        for (int m=0; m<hex->actmask.size(); ++m)
            actmask.push_back(hex->actmask.test(m));
    }

    void State::encodeLink(const std::shared_ptr<Link> link) {
        Encoder::Encode(LA::SRC_HEX_ID, link->src->id, bfstate);
        Encoder::Encode(LA::DST_HEX_ID, link->dst->id, bfstate);
        Encoder::Encode(LA::VALUE, link->value, bfstate);
        Encoder::Encode(LA::TYPE, EI(link->type), bfstate);
    }

    void State::verify() {
        // auto linksSize = battlefield->links.size() * BATTLEFIELD_STATE_SIZE_ONE_LINK;
        // if (bfstate.size() != BATTLEFIELD_STATE_SIZE + linksSize) {
        //     THROW_FORMAT("unexpected bfstate.size(): %d != %d + %d", bfstate.size() % BATTLEFIELD_STATE_SIZE % linksSize);
        // }
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

            auto defender = std::find_if(stacks.begin(), stacks.end(), [&cdefender](std::shared_ptr<Stack> stack) {
                return cdefender == stack->cstack;
            });

            if (defender == stacks.end()) {
                logAi->info("defender cstack '%s' not found in stacks. Maybe it was just summoned/resurrected?", cdefender->getDescription());
            }

            auto attacker = std::find_if(stacks.begin(), stacks.end(), [&cattacker](std::shared_ptr<Stack> stack) {
                return cattacker == stack->cstack;
            });

            auto bf_valueNow = lgstats->valueNow + rgstats->valueNow;
            auto bf_hpNow = lgstats->hpNow + rgstats->hpNow;
            auto value = elem.killedAmount * Stack::CalcValue(cdefender->unitType());

            attackLogs.push_back(std::make_shared<AttackLog>(
                // XXX: attacker can be NULL when an effect does dmg (eg. Acid)
                // XXX: attacker or defender can be NULL if it did not exist
                //      when `stacks` was built (e.g. during our last turn),
                //      but the enemy just summonned/resurrected it
                attacker != stacks.end() ? *attacker : nullptr,
                defender != stacks.end() ? *defender : nullptr,
                cattacker,
                cdefender,
                elem.damageAmount,
                100 * elem.damageAmount / bf_hpNow,
                elem.killedAmount,
                value,
                100 * value / bf_valueNow
            ));
        }
    }

    void State::onBattleTriggerEffect(const BattleTriggerEffect &bte) {
        if (static_cast<BonusType>(bte.effect) != BonusType::MORALE)
            return;

        auto stack = battle->battleGetStackByID(bte.stackID, false);
        isMorale = stack->unitSide() == side;
    }

    void State::onBattleEnd(const BattleResult *br) {
        switch(br->winner) {
        break; case BattleSide::LEFT_SIDE:
            onActiveStack(nullptr, CombatResult::LEFT_WINS);
        break; case BattleSide::RIGHT_SIDE:
            onActiveStack(nullptr, CombatResult::RIGHT_WINS);
        break; default:
            onActiveStack(nullptr, CombatResult::DRAW);
        }
    }
};
