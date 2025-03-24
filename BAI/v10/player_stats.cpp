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

#include "BAI/v10/global_stats.h"
#include "schema/v10/constants.h"
#include "BAI/v10/player_stats.h"

namespace MMAI::BAI::V10 {
    using GA = Schema::V10::GlobalAttribute;
    using A = Schema::V10::PlayerAttribute;

    static_assert(EI(Side::LEFT) == EI(BattleSide::LEFT_SIDE));
    static_assert(EI(Side::RIGHT) == EI(BattleSide::RIGHT_SIDE));

    PlayerStats::PlayerStats(GlobalStats* gstats, BattleSide side, int value, int hp) {
        setattr(A::BATTLE_SIDE, EI(side));
        update(gstats, value, hp)
    };

    void PlayerStats::update(GlobalStats *gstats, int value, int hp) {
        setattr(A::ARMY_VALUE_NOW_ABS, value);
        setattr(A::ARMY_VALUE_NOW_REL, 100 * value / gstats->attr(GA::BFIELD_VALUE_NOW_ABS));
        setattr(A::ARMY_VALUE_NOW_REL0, 100 * value / gstats->attr(GA::BFIELD_VALUE_NOW_ABS));
        setattr(A::VALUE_KILLED_ABS, 0);
        setattr(A::VALUE_KILLED_REL, 0);
        setattr(A::VALUE_KILLED_ACC_ABS, 0);
        setattr(A::VALUE_KILLED_ACC_REL0, 0);
        setattr(A::VALUE_LOST_ABS, 0);
        setattr(A::VALUE_LOST_REL, 0);
        setattr(A::VALUE_LOST_ACC_ABS, 0);
        setattr(A::VALUE_LOST_ACC_REL0, 0);
        setattr(A::DMG_DEALT_ABS, 0);
        setattr(A::DMG_DEALT_REL, 0);
        setattr(A::DMG_DEALT_ACC_ABS, 0);
        setattr(A::DMG_DEALT_ACC_REL0, 0);
        setattr(A::DMG_RECEIVED_ABS, 0);
        setattr(A::DMG_RECEIVED_REL, 0);
        setattr(A::DMG_RECEIVED_ACC_ABS, 0);
        setattr(A::DMG_RECEIVED_ACC_REL0, 0);
    };

    int PlayerStats::getAttr(PlayerAttribute a) const {
        return attr(a);
    }

    int PlayerStats::attr(PlayerAttribute a) const {
        return attrs.at(EI(a));
    };

    void PlayerStats::setattr(PlayerAttribute a, int value) {
        attrs.at(EI(a)) = std::min(value, std::get<3>(GLOBAL_ENCODING.at(EI(a))));
    };

    void PlayerStats::addattr(PlayerAttribute a, int value) {
        attrs.at(EI(a)) += value;
    };
}
