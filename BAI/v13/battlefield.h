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

#include "battle/CPlayerBattleCallback.h"

#include "BAI/v13/hex.h"
#include "BAI/v13/links.h"
#include "BAI/v13/stack.h"
#include "common.h"

namespace MMAI::BAI::V13 {
    using Stacks = std::vector<std::shared_ptr<Stack>>;
    using Hexes = std::array<std::array<std::unique_ptr<Hex>, BF_XMAX>, BF_YMAX>;
    using AllLinks = std::map<LinkType, std::shared_ptr<Links>>;

    using XY = std::pair<int, int>;

    class Battlefield {
    public:
        static std::shared_ptr<const Battlefield> Create(
            const CPlayerBattleCallback* battle,
            const CStack* astack,
            const GlobalStats* ogstats,
            const GlobalStats* gstats,
            const std::map<const CStack*, Stack::Stats> stacksStats,
            bool isMorale
        );

        Battlefield(
            const std::shared_ptr<Hexes> hexes,
            const Stacks stacks,
            const AllLinks allLinks,
            const Stack* astack
        );

        const std::shared_ptr<Hexes> hexes;
        const Stacks stacks;
        const AllLinks allLinks;
        const Stack* const astack;     // XXX: nullptr on battle start/end, or if army stacks > MAX_STACKS_PER_SIDE
    private:
        static std::tuple<Stacks, Queue> InitStacks(
            const CPlayerBattleCallback* battle,
            const CStack* astack,
            const GlobalStats* ogstats,
            const GlobalStats* gstats,
            const std::map<const CStack*, Stack::Stats> stacksStats,
            bool isMorale
        );

        static std::tuple<std::shared_ptr<Hexes>, Stack*> InitHexes(
            const CPlayerBattleCallback* battle,
            const CStack* acstack,
            const Stacks stacks
        );

        static AllLinks InitAllLinks(
            const CPlayerBattleCallback* battle,
            const Stacks stacks,
            const Queue &queue,
            const std::shared_ptr<Hexes>
        );

        static void LinkTwoHexes(
            AllLinks &allLinks,
            const CPlayerBattleCallback* battle,
            const Stacks &stacks,
            const Queue &queue,
            const Hex* src,
            const Hex* dst
        );

        static Queue GetQueue(const CPlayerBattleCallback* battle, const CStack* astack, bool isMorale);
    };
}
