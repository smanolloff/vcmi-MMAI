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

#include "schema/v11/constants.h"
#include "BAI/v11/global_stats.h"

namespace MMAI::BAI::V11 {
    using A = Schema::V11::GlobalAttribute;

    static_assert(EI(Side::LEFT) == EI(BattleSide::LEFT_SIDE));
    static_assert(EI(Side::RIGHT) == EI(BattleSide::RIGHT_SIDE));

    GlobalStats::GlobalStats(BattleSide side, int value, int hp) {
        // Fill with NA to guard against "forgotten" attrs
        // (all attrs are strict so encoder will throw if NAs are found)
        attrs.fill(NULL_VALUE_UNENCODED);

        static_assert(EI(A::_count) == 10);

        setattr(A::BATTLE_WINNER, NULL_VALUE_UNENCODED);
        setattr(A::BATTLE_SIDE, EI(side));
        setattr(A::BATTLE_SIDE_ACTIVE_PLAYER, NULL_VALUE_UNENCODED);
        setattr(A::BFIELD_VALUE_START_ABS, value);
        setattr(A::BFIELD_VALUE_NOW_ABS, value);
        setattr(A::BFIELD_VALUE_NOW_REL0, 1000);
        setattr(A::BFIELD_HP_START_ABS, hp);
        setattr(A::BFIELD_HP_NOW_ABS, hp);
        setattr(A::BFIELD_HP_NOW_REL0, 1000);
        setattr(A::ACTION_MASK, 0);
    }

    static_assert(EI(GlobalAction::_count) == 2);  // RETREAT, WAIT

    void GlobalStats::update(BattleSide side, CombatResult res, int value, int hp, bool canWait) {
        (res == CombatResult::NONE)
            ? setattr(A::BATTLE_WINNER, NULL_VALUE_UNENCODED)
            : setattr(A::BATTLE_WINNER, EI(res));

        (side == BattleSide::NONE)
            ? setattr(A::BATTLE_SIDE_ACTIVE_PLAYER, NULL_VALUE_UNENCODED)
            : setattr(A::BATTLE_SIDE_ACTIVE_PLAYER, EI(side));

        // ll (long long) ensures long is 64-bit even on 32-bit systems
        setattr(A::BFIELD_VALUE_NOW_ABS, value);
        setattr(A::BFIELD_VALUE_NOW_REL0, 1000ll * value / attr(A::BFIELD_VALUE_START_ABS));
        setattr(A::BFIELD_HP_NOW_ABS, hp);
        setattr(A::BFIELD_HP_NOW_REL0, 1000ll * hp / attr(A::BFIELD_HP_START_ABS));

        canWait
            ? actmask.set(EI(GlobalAction::WAIT))
            : actmask.reset(EI(GlobalAction::WAIT));

        setattr(A::ACTION_MASK, actmask.to_ulong());
    }

    int GlobalStats::getAttr(GlobalAttribute a) const {
        return attr(a);
    }

    int GlobalStats::attr(GlobalAttribute a) const {
        return attrs.at(EI(a));
    };

    void GlobalStats::setattr(GlobalAttribute a, int value) {
        attrs.at(EI(a)) = value;
    };
}
