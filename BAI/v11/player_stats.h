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

#include "battle/BattleSide.h"
#include "schema/v11/types.h"
#include "BAI/v11/global_stats.h"

namespace MMAI::BAI::V11 {
    using namespace Schema::V11;

    class PlayerStats : public IPlayerStats {
    public:
        PlayerStats(BattleSide side, int value, int hp);

        int getAttr(PlayerAttribute a) const override;
        int attr(PlayerAttribute a) const;
        void setattr(PlayerAttribute a, int value);
        void addattr(PlayerAttribute a, int value);
        void update(const GlobalStats* gstats, int value, int hp, int dmgDealt, int dmgReceived, int valueKilled, int valueLost);
        PlayerAttrs attrs = {};
    };
}
