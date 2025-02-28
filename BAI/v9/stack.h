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

#include "CStack.h"
#include "battle/IBattleInfoCallback.h"

#include "BAI/v9/global_stats.h"
#include "battle/ReachabilityInfo.h"
#include "schema/v9/types.h"

namespace MMAI::BAI::V9 {
    using namespace Schema::V9;
    using Queue = std::vector<uint32_t>; // item=unit id

    /**
     * A wrapper around CStack
     */
    class Stack : public Schema::V9::IStack {
    public:
        static int CalcValue(const CCreature* creature);

        struct Stats {
            int dmgDealtNow = 0;
            int dmgDealtTotal = 0;
            int dmgReceivedNow = 0;
            int dmgReceivedTotal = 0;
            int valueKilledNow = 0;
            int valueKilledTotal = 0;
            int valueLostNow = 0;
            int valueLostTotal = 0;
        };

        Stack(
            const CStack* cstack,
            Queue &q,
            const GlobalStats* lgstats,
            const GlobalStats* rgstats,
            const Stats stats,
            const ReachabilityInfo rinfo,
            bool blocked,
            bool blocking,
            DamageEstimation estdmg
        );

        // IStack impl
        const StackAttrs& getAttrs() const override;
        int getAttr(StackAttribute a) const override;
        int getFlag(StackFlag sf) const override;
        char getAlias() const override;
        char alias;

        const CStack* const cstack;
        const ReachabilityInfo rinfo;
        StackAttrs attrs = {};
        StackFlags flags = 0;   //

        int attr(StackAttribute a) const;
        bool flag(StackFlag f) const;
        int shots;
    private:
        void setflag(StackFlag f);
        void setattr(StackAttribute a, int value);
        void addattr(StackAttribute a, int value);
        void finalize();
    };
}
