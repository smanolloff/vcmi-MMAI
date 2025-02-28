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

namespace MMAI::BAI::V9 {
    class GlobalStats : public Schema::V9::IGlobalStats {
    public:
        int getValueStart() const override { return valueStart; }
        int getValueNow() const override { return valuePrev; }
        int getValuePrev() const override { return valueNow; }
        int getHPStart() const override { return  hpStart; }
        int getHPPrev() const override { return hpPrev; }
        int getHPNow() const override { return hpNow; }

        int getDmgDealtNow() const override { return dmgDealtNow; }
        int getDmgDealtTotal() const override { return dmgDealtTotal; }
        int getDmgReceivedNow() const override { return dmgReceivedNow; }
        int getDmgReceivedTotal() const override { return dmgReceivedTotal; }
        int getValueKilledNow() const override { return valueKilledNow; }
        int getValueKilledTotal() const override { return valueKilledTotal; }
        int getValueLostNow() const override { return valueLostNow; }
        int getValueLostTotal() const override { return valueLostTotal; }

        const int valueStart;
        int valuePrev = 0;
        int valueNow = 0;

        const int hpStart;
        int hpPrev = 0;
        int hpNow = 0;

        int dmgDealtNow = 0;
        int dmgDealtTotal = 0;
        int dmgReceivedNow = 0;
        int dmgReceivedTotal = 0;
        int valueKilledNow = 0;
        int valueKilledTotal = 0;
        int valueLostNow = 0;
        int valueLostTotal = 0;

        GlobalStats(int totalValue, int totalHP)
        : valueStart(totalValue)
        , valuePrev(totalValue)
        , valueNow(totalValue)
        , hpStart(totalHP)
        , hpPrev(totalHP)
        , hpNow(totalHP)
        {}
    };
}
