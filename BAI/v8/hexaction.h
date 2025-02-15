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

#include "battle/BattleHex.h"

#include "common.h"
#include "schema/v8/constants.h"
#include "schema/v8/types.h"

// There is a cyclic dependency if those are placed in action.h:
// action.h -> battlefield.h -> hex.h -> actmask.h -> action.h
namespace MMAI::BAI::V8 {
    enum class NonHexAction : int {
        RETREAT,
        WAIT,
        count
    };

    using HexAction = Schema::V8::HexAction;

    static_assert(EI(HexAction::AMOVE_TR) == 0);
    static_assert(EI(HexAction::AMOVE_R) == 1);
    static_assert(EI(HexAction::AMOVE_BR) == 2);
    static_assert(EI(HexAction::AMOVE_BL) == 3);
    static_assert(EI(HexAction::AMOVE_L) == 4);
    static_assert(EI(HexAction::AMOVE_TL) == 5);

    constexpr auto AMOVE_TO_EDIR = std::array<BattleHex::EDir, 12> {
        BattleHex::TOP_RIGHT,
        BattleHex::RIGHT,
        BattleHex::BOTTOM_RIGHT,
        BattleHex::BOTTOM_LEFT,
        BattleHex::LEFT,
        BattleHex::TOP_LEFT,
    };

    static_assert(EI(NonHexAction::count) == Schema::V8::N_NONHEX_ACTIONS);
    static_assert(EI(HexAction::_count) == Schema::V8::N_HEX_ACTIONS);

    constexpr int N_ACTIONS = EI(NonHexAction::count) + EI(HexAction::_count)*BF_SIZE;
}
