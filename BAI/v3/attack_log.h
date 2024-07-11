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

#pragma once

#include "lib/AI_Base.h"
#include "schema/v3/types.h"
#include "./stack.h"

namespace MMAI::BAI::V3 {
    class AttackLog : public Schema::V3::IAttackLog {
    public:
        AttackLog(
            std::shared_ptr<Stack> attacker_, std::shared_ptr<Stack> defender_,
            int dmg_, int units_, int value_
        ) : attacker(attacker_), defender(defender_), dmg(dmg_), units(units_), value(value_) {}

        // IAttackLog impl
        Stack* getAttacker() const override { return attacker.get(); }
        Stack* getDefender() const override { return defender.get(); }
        int getDamageDealt() const override { return dmg; }
        int getUnitsKilled() const override { return units; }
        int getValueKilled() const override { return value; }


        /*
         * attacker dealing dmg might be our friendly fire
         * If we look at Attacker POV, we would count our friendly fire as "dmg dealt"
         * So we look at Defender POV, so our friendly fire is counted as "dmg received"
         * This means that if the enemy does friendly fire dmg,
         *  we would count it as our dmg dealt - that is OK (we have "tricked" the enemy!)
         * => store only defender slot
         */

        const std::shared_ptr<Stack> attacker;  // XXX: can be nullptr if dmg is not from creature
        const std::shared_ptr<Stack> defender;
        const int dmg;
        const int units;

        // NOTE: "value" is hard-coded in original H3 and can be found online:
        // https://heroes.thelazy.net/index.php/List_of_creatures
        const int value;
    };
}
