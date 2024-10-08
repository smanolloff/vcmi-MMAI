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

#include "../base.h"
#include "../v1/types.h"
#include "../v1/constants.h"
#include "../v1/util.h"

namespace MMAI::Schema::V2 {
    using A = V1::HexAttribute;
    using E = V1::Encoding;

    // Hex encoding schema
    constexpr V1::HexEncoding HEX_ENCODING {
        ToE4(A::PERCENT_CUR_TO_START_TOTAL_VALUE,   E::FLOATING,          100),     // percentage of all units still alive (global value - same for each hex)
        ToE4(A::HEX_Y_COORD,                        E::FLOATING,          11),      //
        ToE4(A::HEX_X_COORD,                        E::FLOATING,          15),      //
        ToE4(A::HEX_STATE,                          E::FLOATING,          3),       // see HexState
        ToE4(A::HEX_ACTION_MASK_FOR_ACT_STACK,      E::FLOATING,          16384),   // see HexAction
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_0,      E::FLOATING,          16384),   // 16384=14 bits
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_1,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_2,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_3,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_4,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_5,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_L_STACK_6,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_0,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_1,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_2,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_3,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_4,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_5,      E::FLOATING,          16384),   //
        ToE4(A::HEX_ACTION_MASK_FOR_R_STACK_6,      E::FLOATING,          16384),   //
        ToE4(A::HEX_MELEEABLE_BY_ACT_STACK,         E::FLOATING,          3),       // can active stack melee attack hex? (0=no, 1=half, 2=full dmg)
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_0,         E::FLOATING,          3),       // can left-side stack0 melee attack hex? (0=no, 1=half, 2=full dmg)
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_1,         E::FLOATING,          3),       // XXX: MELEEABLE hex does NOT mean there's a stack there (could even be an obstacle)
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_2,         E::FLOATING,          3),       //      It's all about whether the stack can reach a NEARBY hex
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_3,         E::FLOATING,          3),       //      Should it be false for obstacles?
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_4,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_5,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_L_STACK_6,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_0,         E::FLOATING,          3),       // can right-side stack0 melee attack hex? XXX: see note above
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_1,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_2,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_3,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_4,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_5,         E::FLOATING,          3),       //
        ToE4(A::HEX_MELEEABLE_BY_R_STACK_6,         E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_ACT_STACK,  E::FLOATING,          3),       // 0=n/a, 1=half_dmg (far), 2=full_dmg (near, or no range penalty)
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_0,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_1,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_2,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_3,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_4,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_5,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_L_STACK_6,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_0,  E::FLOATING,          3),       // Same as above, but for right-side shooters
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_1,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_2,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_3,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_4,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_5,  E::FLOATING,          3),       //
        ToE4(A::HEX_SHOOT_DISTANCE_FROM_R_STACK_6,  E::FLOATING,          3),       //
        ToE4(A::HEX_MELEE_DISTANCE_FROM_ACT_STACK,  E::FLOATING,          3),       // see MeleeDistance
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_0,  E::FLOATING,          3),       // 0=n/a, 1=FAR, 2=NEAR (wide R attackers only; "~" marks their tail hex)
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_1,  E::FLOATING,          3),       // . . . . . . . . . . . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_2,  E::FLOATING,          3),       //  . . 2 2 . . . . . 1 . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_3,  E::FLOATING,          3),       // . . 2 X 2 . . . X ~ 1 . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_4,  E::FLOATING,          3),       //  . . 2 2 . . . . . 1 . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_5,  E::FLOATING,          3),       // . . . . . . . . . . . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_L_STACK_6,  E::FLOATING,          3),       //
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_0,  E::FLOATING,          3),       // Same as above, but for right-side stacks (i.e. 2 is for wide L attackers only)
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_1,  E::FLOATING,          3),       // . . . . . . . . . . . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_2,  E::FLOATING,          3),       //  . . 2 2 . . . . 1 . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_3,  E::FLOATING,          3),       // . . 2 X 2 . . . 1 ~ X . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_4,  E::FLOATING,          3),       //  . . 2 2 . . . . 1 . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_5,  E::FLOATING,          3),       // . . . . . . . . . . . . .
        ToE4(A::HEX_MELEE_DISTANCE_FROM_R_STACK_6,  E::FLOATING,          3),       //
        ToE4(A::STACK_QUANTITY,                     E::FLOATING,          1023),    // (NUMERIC_SQRT) max for with n=31 (32^2=1024)
        ToE4(A::STACK_ATTACK,                       E::FLOATING,          63),      // (NUMERIC_SQRT) max for with n=7 (8^2=64)
        ToE4(A::STACK_DEFENSE,                      E::FLOATING,          63),      // (NUMERIC_SQRT) max for with n=7 (8^2=64) - crystal dragon is 48 when defending
        ToE4(A::STACK_SHOTS,                        E::FLOATING,          35),      // (NUMERIC_SQRT) max for with n=5 (6^2=36) - sharpshooter is 32
        ToE4(A::STACK_DMG_MIN,                      E::FLOATING,          80),      // (NUMERIC_SQRT) max for with n=8 (9^2=81)
        ToE4(A::STACK_DMG_MAX,                      E::FLOATING,          80),      // (NUMERIC_SQRT) max for with n=8 (9^2=81)
        ToE4(A::STACK_HP,                           E::FLOATING,          840),     // (NUMERIC_SQRT) max for with n=28 (29^2=841) - crystal dragon is 800
        ToE4(A::STACK_HP_LEFT,                      E::FLOATING,          840),     // (NUMERIC_SQRT) max for with n=28 (29^2=841) - crystal dragon is 800
        ToE4(A::STACK_SPEED,                        E::FLOATING,          23),      // phoenix is 22
        ToE4(A::STACK_WAITED,                       E::FLOATING,          2),       //
        ToE4(A::STACK_QUEUE_POS,                    E::FLOATING,          15),      // 0..14, 0=active stack
        ToE4(A::STACK_RETALIATIONS_LEFT,            E::FLOATING,          3),       // inf is truncated to 2 (royal griffin)
        ToE4(A::STACK_SIDE,                         E::FLOATING,          2),       // 0=attacker, 1=defender
        ToE4(A::STACK_SLOT,                         E::FLOATING,          7),       // 0..6
        ToE4(A::STACK_CREATURE_TYPE,                E::FLOATING,          145),     // 0..144 (incl.)
        ToE4(A::STACK_AI_VALUE_TENTH,               E::FLOATING,          3968),    // max for n=62 (63^2=3969) - crystal dragon is 3933
        ToE4(A::STACK_IS_ACTIVE,                    E::FLOATING,          2),       //
        ToE4(A::STACK_IS_WIDE,                      E::FLOATING,          2),       // is this a two-hex stack?
        ToE4(A::STACK_FLYING,                       E::FLOATING,          2),       //
        ToE4(A::STACK_NO_MELEE_PENALTY,             E::FLOATING,          2),       //
        ToE4(A::STACK_TWO_HEX_ATTACK_BREATH,        E::FLOATING,          2),       //
        ToE4(A::STACK_BLOCKS_RETALIATION,           E::FLOATING,          2),       //
        ToE4(A::STACK_DEFENSIVE_STANCE,             E::FLOATING,          2),       //
    };

    /*
     * Compile-time check for uninitialized elements `HEX_ENCODING`.
     * The index of the uninitialized element is returned.
     */
    constexpr int UninitializedHexEncodingElements() {
        for (int i = 0; i < EI(V1::HexAttribute::_count); i++) {
            if (HEX_ENCODING.at(i) == V1::E4{}) return i;
        }
        return -1;
    }
    static_assert(UninitializedHexEncodingElements() == -1, "Uninitialized element at this index in HEX_ENCODING");

    /*
     * Compile-time check for elements in `HEX_ENCODING` which are
     * out-of-order compared to the `Attribute` enum values.
     * The index at which the order is violated is returned.
     */
    constexpr int DisarrayedHexEncodingAttributes() {
        if (UninitializedHexEncodingElements() != -1) return -1;

        for (int i = 0; i < EI(V1::HexAttribute::_count); i++)
            if (std::get<0>(HEX_ENCODING.at(i)) != V1::HexAttribute(i)) return i;
        return -1;
    }
    static_assert(DisarrayedHexEncodingAttributes() == -1, "HexAttribute out of order at this index in HEX_ENCODING");

    /*
     * Compile-time calculation for the encoded size of one hex
     */
    constexpr int BattlefieldStateSizeOneHex() {
        int ret = 0;
        for (int i = 0; i < EI(V1::HexAttribute::_count); i++)
            ret += std::get<2>(HEX_ENCODING[i]);
        return ret;
    }

    constexpr int BATTLEFIELD_STATE_SIZE_ONE_HEX = BattlefieldStateSizeOneHex();
    constexpr int BATTLEFIELD_STATE_SIZE = 165 * BATTLEFIELD_STATE_SIZE_ONE_HEX;
}
