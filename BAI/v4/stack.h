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

#include "BAI/v4/hexactmask.h"
#include "battle/IBattleInfoCallback.h"
#include "schema/v4/types.h"

namespace MMAI::BAI::V4 {
    using namespace Schema::V4;
    using Queue = std::vector<uint32_t>; // item=unit id

    /**
     * A wrapper around CStack
     */
    class Stack : public Schema::V4::IStack {
    public:
        Stack(const CStack* cstack, int index, Queue &q, bool blocked, bool blocking, DamageEstimation estdmg);

        // IStack impl
        const StackAttrs& getAttrs() const override;
        int getAttr(StackAttribute a) const override;
        char getAlias() const override;
        char alias;

        const CStack* const cstack;
        StackAttrs attrs = {};

        std::string name() const;
        int attr(StackAttribute a) const;
        int shots;
    private:
        void setattr(StackAttribute a, int value);
        void addattr(StackAttribute a, int value);
        void finalize();
    };
}
