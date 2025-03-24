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

#include "BAI/v7/general_info.h"
#include "BAI/v7/hex.h"
#include "BAI/v7/stack.h"
#include "common.h"

namespace MMAI::BAI::V7 {
    using Stacks = std::vector<std::shared_ptr<Stack>>;
    using Hexes = std::array<std::array<std::unique_ptr<Hex>, BF_XMAX>, BF_YMAX>;
    using XY = std::pair<int, int>;
    using DirHex = std::vector<std::pair<BattleHex::EDir, BattleHex>>;

    class Battlefield {
    public:
        static std::shared_ptr<const Battlefield> Create(
            const CPlayerBattleCallback* battle,
            const CStack* astack_,
            std::pair<int, int> initialArmyValues,
            bool isMorale
        );

        Battlefield(
            std::shared_ptr<GeneralInfo> info,
            std::shared_ptr<Hexes> hexes,
            Stacks stacks,
            const Stack* astack
        );

        const std::shared_ptr<GeneralInfo> info;
        const std::shared_ptr<Hexes> hexes;
        const Stacks stacks;
        const Stack* const astack;     // XXX: nullptr on battle start/end, or if army stacks > MAX_STACKS_PER_SIDE
    private:
        static Stacks InitStacks(
            const CPlayerBattleCallback* battle,
            const CStack* astack,
            bool isMorale
        );

        static std::tuple<std::shared_ptr<Hexes>, Stack*> InitHexes(
            const CPlayerBattleCallback* battle,
            const CStack* acstack,
            const Stacks stacks
        );

        static Queue GetQueue(const CPlayerBattleCallback* battle, const CStack* astack, bool isMorale);
        static int BaseMeleeMod(const CPlayerBattleCallback* battle, const CStack* cstack);
        static int BaseRangeMod(const CPlayerBattleCallback* battle, const CStack* cstack);
    };
}
