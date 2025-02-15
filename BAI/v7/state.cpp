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

#include "BAI/v7/encoder.h"
#include "BAI/v7/hexaction.h"
#include "BAI/v7/state.h"
#include "BAI/v7/supplementary_data.h"
#include "schema/v7/constants.h"
#include "schema/v7/types.h"

#include <memory>

namespace MMAI::BAI::V7 {
    using SA = StackAttribute;

    static_assert(EI(HexAttribute::STACK_SIDE)           == EI(StackAttribute::SIDE) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_QUANTITY)       == EI(StackAttribute::QUANTITY) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_ATTACK)         == EI(StackAttribute::ATTACK) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_DEFENSE)        == EI(StackAttribute::DEFENSE) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_SHOTS)          == EI(StackAttribute::SHOTS) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_DMG_MIN)        == EI(StackAttribute::DMG_MIN) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_DMG_MAX)        == EI(StackAttribute::DMG_MAX) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_HP)             == EI(StackAttribute::HP) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_HP_LEFT)        == EI(StackAttribute::HP_LEFT) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_SPEED)          == EI(StackAttribute::SPEED) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_QUEUE_POS)      == EI(StackAttribute::QUEUE_POS) + STACK_ATTR_OFFSET);
    // static_assert(EI(HexAttribute::STACK_ESTIMATED_DMG)  == EI(StackAttribute::ESTIMATED_DMG) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_AI_VALUE)       == EI(StackAttribute::AI_VALUE) + STACK_ATTR_OFFSET);
    static_assert(EI(HexAttribute::STACK_FLAGS)          == EI(StackAttribute::FLAGS) + STACK_ATTR_OFFSET);

    // static
    std::vector<float> State::InitNullStack() {
        auto res = std::vector<float> {};
        for (int i=0; i<EI(StackAttribute::_count); ++i)
            Encoder::Encode(HexAttribute(STACK_ATTR_OFFSET + i), NULL_VALUE_UNENCODED, res);
        return res;
    };

    State::State(const int version__, const std::string colorname_, const CPlayerBattleCallback* battle_)
    : version_(version__)
    , colorname(colorname_)
    , battle(battle_)
    , side(battle_->battleGetMySide())
    , initialArmyValues(GeneralInfo::CalcTotalArmyValues(battle_))
    , nullstack(InitNullStack())
    , battlefield(Battlefield::Create(battle_, nullptr, initialArmyValues, false))
    {
        bfstate.reserve(Schema::V7::BATTLEFIELD_STATE_SIZE);
        actmask.reserve(Schema::V7::N_ACTIONS);
        // attnmask.reserve(165 * 165);
    }

    void State::onActiveStack(const CStack* astack) {
        int dmgReceived = 0;
        int unitsLost = 0;
        int valueLost = 0;
        int dmgDealt = 0;
        int unitsKilled = 0;
        int valueKilled = 0;

        for (auto &al : attackLogs) {
            // note: in VCMI there is no excess dmg if stack is killed

            // XXX: defender can be nullptr if excluded from obs
            //      In this case, we must ignore the log as well as we can't
            //      tell if this was damage received or dealt
            //      (even if attacker is friendly, the defender might still be
            //       friendly, i.e. dragon breath friendly fire)
            if (al->defender == nullptr)
                continue;

            if (al->defender->attr(SA::SIDE) == EI(side)) {
                dmgReceived += al->dmg;
                unitsLost += al->units;
                valueLost += al->value;
            } else {
                dmgDealt += al->dmg;
                unitsKilled += al->units;
                valueKilled += al->value;
            }
        }

        battlefield = Battlefield::Create(battle, astack, initialArmyValues, isMorale);
        isMorale = false;

        supdata = std::make_unique<SupplementaryData>(
            colorname,
            Side(side),
            dmgDealt,
            dmgReceived,
            unitsLost,
            unitsKilled,
            valueLost,
            valueKilled,
            battlefield.get(),
            attackLogs  // store the logs since last turn
        );

        attackLogs.clear(); // accumulate new logs until next turn
        bfstate.clear();
        actmask.clear();
        // attnmask.clear();

        for (int i=0; i<EI(NonHexAction::count); i++) {
            switch (NonHexAction(i)) {
            // TODO: handle cases where retreat is not allowed (shackles of war, no hero, etc.)
            break; case NonHexAction::RETREAT: actmask.push_back(true);
            break; case NonHexAction::WAIT: actmask.push_back(battlefield->astack && !battlefield->astack->cstack->waitedThisTurn);
            break; default:
                THROW_FORMAT("Unexpected NonHexAction: %d", i);
            }
        }

        for (auto &hexrow : *battlefield->hexes)
            for (auto &hex : hexrow)
                encodeHex(hex.get());

        verify();
    }

    void State::encodeHex(Hex* hex) {
        // Battlefield state
        for (int i=0; i<EI(HexAttribute::_count); ++i)
            Encoder::Encode(HexAttribute(i), hex->attrs.at(i), bfstate);

        // Action mask
        for (int m=0; m<hex->actmask.size(); ++m)
            actmask.push_back(hex->actmask.test(m));

        // // Attention mask
        // // (not used)
        // if (hex->cstack) {
        //     auto side = hex->attrs.at(EI(HexAttribute::STACK_SIDE));
        //     auto slot = hex->attrs.at(EI(HexAttribute::STACK_SLOT));
        //     for (auto &hexrow1 : bf.hexes) {
        //         for (auto &hex1 : hexrow1) {
        //             if (hex1.hexactmasks.at(side).at(slot).any()) {
        //                 attnmask.push_back(0);
        //             } else {
        //                 attnmask.push_back(std::numeric_limits<float>::lowest());
        //             }
        //         }
        //     }
        // } else {
        //     attnmask.insert(attnmask.end(), 165, std::numeric_limits<float>::lowest());
        // }
    }

    void State::verify() {
        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE, "unexpected bfstate.size(): " + std::to_string(bfstate.size()));
        ASSERT(actmask.size() == N_ACTIONS, "unexpected actmask.size(): " + std::to_string(actmask.size()));
        // ASSERT(attnmask.size() == 165*165, "unexpected attnmask.size(): " + std::to_string(attnmask.size()));
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
                elem.killedAmount * cdefender->unitType()->getAIValue()
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
