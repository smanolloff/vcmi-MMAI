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

#include "BAI/v13/global_stats.h"
#include "battle/ReachabilityInfo.h"
#include "schema/v13/constants.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13 {
    using namespace Schema::V13;
    using Queue = std::vector<uint32_t>; // item=unit id
    using BitQueue = std::bitset<STACK_QUEUE_SIZE>;

    static_assert(1<<STACK_QUEUE_SIZE < std::numeric_limits<int>::max(), "BitQueue must be convertible to int");


    /**
     * A wrapper around CStack
     */
    class Stack : public Schema::V13::IStack {
    public:
        static int CalcValue(const CCreature* creature);

        // not the quantum version :)
        static std::pair<BitQueue, int> QBits(const CStack*, const Queue&);

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
            const GlobalStats* ogstats,
            const GlobalStats* gstats,
            const Stats stats,
            const ReachabilityInfo rinfo,
            bool blocked,
            bool blocking,
            DamageEstimation estdmg
        );

        // IStack impl
        const StackAttrs& getAttrs() const override;
        int getAttr(StackAttribute a) const override;
        int getFlag(StackFlag1 sf) const override;
        int getFlag(StackFlag2 sf) const override;
        char getAlias() const override;
        char alias;

        const CStack* const cstack;
        const ReachabilityInfo rinfo;
        StackAttrs attrs = {};
        StackFlags1 flags1 = 0;   //
        StackFlags2 flags2 = 0;   //

        int attr(StackAttribute a) const;
        bool flag(StackFlag1 f) const;
        bool flag(StackFlag2 f) const;
        int shots;
        int qposFirst;
    private:
        void setflag(StackFlag1 f);
        void setflag(StackFlag2 f);
        void setattr(StackAttribute a, int value);
        void addattr(StackAttribute a, int value);
        void finalize();
    };
}
