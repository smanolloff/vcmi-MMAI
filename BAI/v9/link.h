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

#include "schema/v9/types.h"
#include "BAI/v9/hex.h"

namespace MMAI::BAI::V9 {
    using LinkType = Schema::V9::LinkType;

    class Link : public Schema::V9::ILink {
    public:
        Link(LinkType t, const Hex *src, const Hex *dst, float v)
        : type(t)
        , src(src)
        , dst(dst)
        , value(v)
        {};

        const LinkType type;
        const Hex* const src;
        const Hex* const dst;
        const float value;

        LinkType getType() const override { return type; };
        const IHex* getSrc() const override { return src; }
        const IHex* getDst() const override { return dst; }
        float getValue() const override { return value; };
    };
}
