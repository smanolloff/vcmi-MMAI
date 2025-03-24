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

#include "schema/v10/constants.h"
#include "BAI/v10/global_stats.h"

namespace MMAI::BAI::V10 {
    using A = Schema::V10::GlobalAttribute;

    static_assert(EI(Side::LEFT) == EI(BattleSide::LEFT_SIDE));
    static_assert(EI(Side::RIGHT) == EI(BattleSide::RIGHT_SIDE));

    GlobalStats::GlobalStats(BattleSide side, int value, int hp) {
        setattr(A::BATTLE_SIDE, EI(side));
        setattr(A::BATTLE_WINNER, NULL_VALUE_UNENCODED);  // no winner
        setattr(A::BFIELD_VALUE_START_ABS, value);
        setattr(A::BFIELD_VALUE_NOW_ABS, value);
        setattr(A::BFIELD_VALUE_NOW_REL0, 100);
    }

    int GlobalStats::getAttr(GlobalAttribute a) const {
        return attr(a);
    }

    int GlobalStats::attr(GlobalAttribute a) const {
        return attrs.at(EI(a));
    };

    void GlobalStats::setattr(GlobalAttribute a, int value) {
        attrs.at(EI(a)) = std::min(value, std::get<3>(GLOBAL_ENCODING.at(EI(a))));
    };

}
