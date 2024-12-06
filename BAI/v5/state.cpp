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

#include "BAI/v5/hex.h"
#include "StdInc.h"

#include "battle/CPlayerBattleCallback.h"
#include "battle/IBattleState.h"
#include "networkPacks/PacksForClientBattle.h"

#include "BAI/v5/encoder.h"
#include "BAI/v5/state.h"
#include "BAI/v5/supplementary_data.h"
#include "schema/v5/constants.h"
#include "schema/v5/types.h"

#include <memory>

namespace MMAI::BAI::V5 {
    // static
    std::vector<float> State::InitNullStack() {
        auto res = std::vector<float> {};
        res.reserve(BATTLEFIELD_STATE_SIZE_ONE_STACK);
        for (int i=0; i<EI(StackAttribute::_count); ++i)
            Encoder::Encode(StackAttribute(i), NULL_VALUE_UNENCODED, res);
        ASSERT(res.size() == res.capacity(), "incorrectly initialized nullstack");
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
        bfstate.reserve(Schema::V5::BATTLEFIELD_STATE_SIZE);
        actmask.reserve(Schema::V5::N_ACTIONS);
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

        // XXX: Order is important:
        //   1. Misc
        //   2. Stacks
        //   3. Hexes

        auto shooting = battle->battleCanShoot(astack);

        encodeMisc(shooting, astack);
        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE_MISC, "[1] unexpected bfstate.size(): " + std::to_string(bfstate.size()));

        for (auto i=0; i < battlefield->stacks->size(); i++) {
            auto &sidestacks = battlefield->stacks->at(i);

            static_assert(EI(Side::LEFT) == EI(BattleSide::ATTACKER));
            if (shooting && i == EI(astack->unitSide()))
                shooting = false;

            for (auto &stack : sidestacks)
                encodeStack(stack.get(), shooting);
        }

        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE_MISC + BATTLEFIELD_STATE_SIZE_ALL_STACKS, "[2] unexpected bfstate.size(): " + std::to_string(bfstate.size()));

        for (auto &hexrow : *battlefield->hexes)
            for (auto &hex : hexrow)
                encodeHex(hex.get());

        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE, "[3] unexpected bfstate.size(): " + std::to_string(bfstate.size()));

        verify();
    }

    void State::encodeMisc(bool shooting, const CStack* astack) {
        // mask must come first in misc encoding to ensure
        // correct indexing based on PrimaryAction enum
        static_assert(EI(MA::PRIMARY_ACTION_MASK) == 0);
        static_assert(std::get<1>(MISC_ENCODING[EI(MA::PRIMARY_ACTION_MASK)]) == Encoding::BINARY_STRICT_NULL);

        // Insert the 11 primary actions now, update some of them later
        actmask.insert(actmask.end(), EI(PrimaryAction::_count), false);
        actmask.at(EI(PrimaryAction::RETREAT)) = 1;
        actmask.at(EI(PrimaryAction::WAIT)) = battlefield->astack && !battlefield->astack->cstack->waitedThisTurn;
        actmask.at(EI(PrimaryAction::MOVE)) = !!astack;
        bfstate.insert(bfstate.end(), actmask.begin(), actmask.end());
        // the ATTACK_* actions are updated later (in encodeStack and encodeHex)

        Encoder::Encode(MiscAttribute::SHOOTING, shooting, bfstate);
        Encoder::Encode(MiscAttribute::INITIAL_ARMY_VALUE_LEFT, supdata->misc->initialArmyValueLeft, bfstate);
        Encoder::Encode(MiscAttribute::INITIAL_ARMY_VALUE_RIGHT, supdata->misc->initialArmyValueRight, bfstate);
        Encoder::Encode(MiscAttribute::CURRENT_ARMY_VALUE_LEFT, supdata->misc->currentArmyValueLeft, bfstate);
        Encoder::Encode(MiscAttribute::CURRENT_ARMY_VALUE_RIGHT, supdata->misc->currentArmyValueRight, bfstate);
    }

    void State::encodeStack(Stack* stack, bool shootable) {
        if (!stack) {
            bfstate.insert(bfstate.end(), nullstack.begin(), nullstack.end());
            return;
        }

        if (shootable) {
            auto i = EI(PrimaryAction::ATTACK_0) + stack->attr(SA::ID);
            actmask.at(i) = true;
            bfstate.at(i) = 1;
        }

        for (int i=0; i<EI(StackAttribute::_count); ++i)
            Encoder::Encode(StackAttribute(i), stack->attrs.at(i), bfstate);
    }

    void State::encodeHex(Hex* hex) {
        // Battlefield state
        for (int i=0; i<EI(HexAttribute::_count); ++i)
            Encoder::Encode(HexAttribute(i), hex->attrs.at(i), bfstate);

        // Action mask
        for (int m=0; m < EI(AMoveAction::_count); ++m) {
            // 1. Set primary (nonhex) action mask:
            //    if the hex allows an action
            //    => the corresponding primary action is enabled as well
            auto v = hex->amovemask.test(m);
            auto i = EI(Hex::AMoveToPrimaryAction(AMoveAction(m)));
            if(!actmask.at(i) and v) {
                actmask.at(i) = true;
                bfstate.at(i) = 1;
            }

            // 2. Add the hex-specific action mask
            actmask.push_back(v);
        }

        // Attention mask
        // (not used)
    }

    void State::verify() {
        ASSERT(bfstate.size() == BATTLEFIELD_STATE_SIZE, "unexpected bfstate.size(): " + std::to_string(bfstate.size()));
        ASSERT(actmask.size() == N_ACTIONS, "unexpected actmask.size(): " + std::to_string(actmask.size()));
        // ASSERT(attnmask.size() == 165*165, "unexpected attnmask.size(): " + std::to_string(attnmask.size()));
    }

    void State::onBattleStacksAttacked(const std::vector<BattleStackAttacked> &bsa) {
        for(auto & elem : bsa) {
            auto cdefender = battle->battleGetStackByID(elem.stackAttacked, false);
            auto cattacker = battle->battleGetStackByID(elem.attackerID, false);

            ASSERT(cdefender, "defender cannot be NULL");
            // logAi->debug("Attack: %s -> %s (%d dmg, %d died)", attacker->getName(), defender->getName(), elem.damageAmount, elem.killedAmount);

            // there may have been no room for defender in the obs
            auto defender = battlefield->stackmapping.find(cdefender) != battlefield->stackmapping.end()
                ? battlefield->stackmapping.at(cdefender)
                : nullptr;

            auto attacker = battlefield->stackmapping.find(cattacker) != battlefield->stackmapping.end()
                ? battlefield->stackmapping.at(cattacker)
                : nullptr;

            attackLogs.push_back(std::make_shared<AttackLog>(
                // XXX: attacker can be NULL when an effect does dmg (eg. Acid)
                // XXX: attacker and/or defender can be NULL they can't fit in obs
                attacker,
                defender,
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
