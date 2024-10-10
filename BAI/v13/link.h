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

#include "schema/v13/types.h"
#include "BAI/v13/hex.h"

namespace MMAI::BAI::V13 {
    using LinkType = Schema::V13::LinkType;

    class Link : public Schema::V13::ILink {
    public:
        Link(
            const Hex *src,
            const Hex *dst,
            bool neighbour,
            bool reachable,
            float rangedMod,
            float rangedDmgFrac,
            float meleeDmgFrac,
            float retalDmgFrac
        ) : src(src)
          , dst(dst)
          , neighbour(neighbour)
          , reachable(reachable)
          , rangedMod(rangedMod)
          , rangedDmgFrac(rangedDmgFrac)
          , meleeDmgFrac(meleeDmgFrac)
          , retalDmgFrac(retalDmgFrac)
        {};

        const Hex* const src;
        const Hex* const dst;
        const bool neighbour;
        const bool reachable;
        const float rangedMod;
        const float rangedDmgFrac;
        const float meleeDmgFrac;
        const float retalDmgFrac;

        const IHex* getSrc() const override { return src; }
        const IHex* getDst() const override { return dst; }
        bool isNeighbour() const override { return neighbour; };
        bool isReachable() const override { return reachable; };
        float getRangedMod() const override { return rangedMod; };
        float getRangedDmgFrac() const override { return rangedDmgFrac; };
        float getMeleeDmgFrac() const override { return meleeDmgFrac; };
        float getRetalDmgFrac() const override { return retalDmgFrac; };
    };
}
