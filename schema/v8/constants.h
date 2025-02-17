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

#include "schema/base.h"
#include "schema/v8/types.h"
#include "schema/v8/util.h"

namespace MMAI::Schema::V8 {
    constexpr int N_NONHEX_ACTIONS = 2;
    constexpr Action ACTION_RETREAT = 0;
    constexpr Action ACTION_WAIT = 1;
    constexpr int N_HEX_ACTIONS = EI(HexAction::_count);
    constexpr int N_ACTIONS = N_NONHEX_ACTIONS + 165 * N_HEX_ACTIONS;
    constexpr int STACK_ATTR_OFFSET = EI(HexAttribute::_count) - EI(StackAttribute::_count);


    // Control actions (not part of the regular action space)
    constexpr Action ACTION_UNSET = -666;
    constexpr Action ACTION_RESET = -1;
    constexpr Action ACTION_RENDER_ANSI = -2;

    // Value used when masking NULL values during encoding
    constexpr int NULL_VALUE_ENCODED = -1;
    constexpr int NULL_VALUE_UNENCODED = -1;

    // Convenience definitions which do not need to be exported
    namespace {
        inline constexpr auto AE = Encoding::ACCUMULATING_EXPLICIT_NULL;
        inline constexpr auto AI = Encoding::ACCUMULATING_IMPLICIT_NULL;
        inline constexpr auto AM = Encoding::ACCUMULATING_MASKING_NULL;
        inline constexpr auto AS = Encoding::ACCUMULATING_STRICT_NULL;
        inline constexpr auto AZ = Encoding::ACCUMULATING_ZERO_NULL;

        inline constexpr auto BE = Encoding::BINARY_EXPLICIT_NULL;
        inline constexpr auto BM = Encoding::BINARY_MASKING_NULL;
        inline constexpr auto BS = Encoding::BINARY_STRICT_NULL;
        inline constexpr auto BZ = Encoding::BINARY_ZERO_NULL;

        inline constexpr auto CE = Encoding::CATEGORICAL_EXPLICIT_NULL;
        inline constexpr auto CI = Encoding::CATEGORICAL_IMPLICIT_NULL;
        inline constexpr auto CM = Encoding::CATEGORICAL_MASKING_NULL;
        inline constexpr auto CS = Encoding::CATEGORICAL_STRICT_NULL;
        inline constexpr auto CZ = Encoding::CATEGORICAL_ZERO_NULL;

        inline constexpr auto EE = Encoding::EXPNORM_EXPLICIT_NULL;
        inline constexpr auto EM = Encoding::EXPNORM_MASKING_NULL;
        inline constexpr auto ES = Encoding::EXPNORM_STRICT_NULL;
        inline constexpr auto EZ = Encoding::EXPNORM_ZERO_NULL;

        inline constexpr auto LE = Encoding::LINNORM_EXPLICIT_NULL;
        inline constexpr auto LM = Encoding::LINNORM_MASKING_NULL;
        inline constexpr auto LS = Encoding::LINNORM_STRICT_NULL;
        inline constexpr auto LZ = Encoding::LINNORM_ZERO_NULL;

        using GA = GlobalAttribute;
        using PA = PlayerAttribute;
        using HA = HexAttribute;

        /*
         * The encoding schema `{a, e, n, vmax}`, where:
         * `a`=attribute, `e`=encoding, `n`=size, `vmax`=max_value.
         */
        using E4G = std::tuple<GlobalAttribute, Encoding, int, int>;
        using E4P = std::tuple<PlayerAttribute, Encoding, int, int>;
        using E4H = std::tuple<HexAttribute, Encoding, int, int>;
    }

    using GlobalEncoding = std::array<E4G, EI(GlobalAttribute::_count)>;
    using PlayerEncoding = std::array<E4P, EI(PlayerAttribute::_count)>;
    using HexEncoding = std::array<E4H, EI(HexAttribute::_count)>;

    /*
     * Compile-time constructor for E4H and E4S tuples
     * https://stackoverflow.com/a/23784921
     */
    template<typename T>
    constexpr std::tuple<T, Encoding, int, int> E4(T a, Encoding e, int vmax) {
        switch(e) {
        // "0" is a value => vmax+1 values
        case AE: return {a, e, vmax+2, vmax};
        case AI: return {a, e, vmax+1, vmax};
        case AM: return {a, e, vmax+1, vmax};
        case AS: return {a, e, vmax+1, vmax};
        case AZ: return {a, e, vmax+1, vmax};

        // Log2(8)=3 (2^3), but if vmax=8 then 4 bits will be required
        // => Log2(9)=4
        case BE: return {a, e, static_cast<int>(Log2(vmax+1))+1, vmax};
        case BM: return {a, e, static_cast<int>(Log2(vmax+1)), vmax};
        case BS: return {a, e, static_cast<int>(Log2(vmax+1)), vmax};
        case BZ: return {a, e, static_cast<int>(Log2(vmax+1)), vmax};

        // "0" is a category => vmax+1 categories
        case CE: return {a, e, vmax+2, vmax};
        case CI: return {a, e, vmax+1, vmax};
        case CM: return {a, e, vmax+1, vmax};
        case CS: return {a, e, vmax+1, vmax};
        case CZ: return {a, e, vmax+1, vmax};

        case EE: return {a, e, 2, vmax};
        case EM: return {a, e, 1, vmax};
        case ES: return {a, e, 1, vmax};
        case EZ: return {a, e, 1, vmax};

        case LE: return {a, e, 2, vmax};
        case LM: return {a, e, 1, vmax};
        case LS: return {a, e, 1, vmax};
        case LZ: return {a, e, 1, vmax};
        default:
            throw std::runtime_error("Unexpected encoding: " + std::to_string(EI(e)));
        }
    }

    // Values above MAX are simply capped
    constexpr int STACK_VALUE_ONE_MAX = 180000;     // archangel ~50K, crystal dragon ~110K, azure dragon ~180K
    constexpr int STACK_QTY_MAX = 1500;
    constexpr int STACK_DMG_DEALT_MAX = 10000;  // 4x1096 has 98 Ghost dragons => ~4K dmg => 8K (+19 attack)
    constexpr int STACK_VALUE_KILLED_MAX = 100000;  // maxdmg(10K) / cerberus(25) = 400 kills * 200 value = 80K
    constexpr int STACK_HP_TOTAL_MAX = 30000;  // 98 Ghost dragons * 200HP = 20K
    constexpr int CREATURE_ID_MAX = 149;  // H3 core has creature IDs 0..149

    // NOTE: the generated maps use old AIValue() which is 4-6x LOWER
    //       than the one calculated by MMAI (in Stack::CalcValue())
    //       => a map with 100K pools corresponds to 400K..600K pools now
    // The biggest pools are:
    //   (1) 4x1096  => 500K pools (old)  => 3000K (new) => total = 6000K  (new)
    //   (2) 8x64    => 800K pools (old)  => 4800K (new) => total = 9600K  (new)
    //   (3) 8x64    => 1600K pools (old) => 9600K (new) => total = 19200K (new)
    // Since (1) is used for training while (2) and (3) are for evaluation, we
    // set max=10000K=10M (new) in order to:
    // - test higher-than-trained values via (2), but within limits,
    // - test higher-than-trained values via (3), but outside limits
    //
    // XXX: THIS IS NOW LEFT UNUSED (switched to relative values instead)
    // constexpr int ARMY_VALUE_MAX = 10 * 1000 * 1000; // 10M


    constexpr GlobalEncoding GLOBAL_ENCODING {
        E4(GA::BATTLE_SIDE,                 CS, 1),
        E4(GA::BATTLE_WINNER,               CE, 1),         // NULL means ongoing battle
        E4(GA::BFIELD_VALUE_NOW_REL0,       LS, 100),       // bfield_value_now             / bfield_value_at_start
    };

    constexpr PlayerEncoding PLAYER_ENCODING {
        E4(PA::ARMY_VALUE_NOW_REL,     LS, 100),       // army_value_now          / global_value_now
        E4(PA::ARMY_VALUE_NOW_REL0,    LS, 100),       // army_value_now          / global_value_at_start
        E4(PA::VALUE_KILLED_REL,       LS, 100),       // value_killed_this_turn  / global_value_last_turn
        E4(PA::VALUE_KILLED_ACC_REL0,  LS, 100),       // value_killed_lifetime   / global_value_at_start
        E4(PA::VALUE_LOST_REL,         LS, 100),       // value_lost_this_turn    / global_value_last_turn
        E4(PA::VALUE_LOST_ACC_REL0,    LS, 100),       // value_lost_lifetime     / global_value_at_start
        E4(PA::DMG_DEALT_REL,          LS, 100),       // dmg_dealt_this_turn     / global_hp_last_turn
        E4(PA::DMG_DEALT_ACC_REL0,     LS, 100),       // dmg_dealt_lifetime      / global_hp_at_start
        E4(PA::DMG_RECEIVED_REL,       LS, 100),       // dmg_received_this_turn  / global_hp_last_turn
        E4(PA::DMG_RECEIVED_ACC_REL0,  LS, 100),       // dmg_received_lifetime   / global_hp_at_start
    };

    constexpr HexEncoding HEX_ENCODING {
        E4(HA::Y_COORD,                 CS, 10),
        E4(HA::X_COORD,                 CS, 14),
        E4(HA::STATE_MASK,              BS, (1<<EI(HexState::_count))-1),
        E4(HA::ACTION_MASK,             BZ, (1<<EI(HexAction::_count))-1),
        E4(HA::IS_REAR,                 CE, 1),        // 1=this is the rear hex of a stack
        E4(HA::STACK_SIDE,              CE, 1),        // 0=attacker, 1=defender
        // E4(HA::STACK_CREATURE_ID,       CE, CREATURE_ID_MAX),
        E4(HA::STACK_QUANTITY,          EE, STACK_QTY_MAX),
        E4(HA::STACK_ATTACK,            EE, 80),
        E4(HA::STACK_DEFENSE,           EE, 80),       // azure dragon is 60 when defending
        E4(HA::STACK_SHOTS,             EE, 32),       // sharpshooter is 32
        E4(HA::STACK_DMG_MIN,           EE, 100),      // azure dragon is 80
        E4(HA::STACK_DMG_MAX,           EE, 100),      // azure dragon is 80
        E4(HA::STACK_HP,                EE, 1300),     // azure dragon + all artifacts is 1254
        E4(HA::STACK_HP_LEFT,           EE, 1300),
        E4(HA::STACK_SPEED,             EE, 30),       // at 19=full reach; max is... 37?
        E4(HA::STACK_QUEUE_POS,         EE, 15),       // 0..14, 0=active stack
        // H4(SSTACK_A::ESTIMATED_DMG,  NE, 100),      // est. dmg by the active stack as a percentage of this stack's total HP
        E4(HA::STACK_VALUE_ONE,         EE, STACK_VALUE_ONE_MAX),
        E4(HA::STACK_FLAGS,             BE, (1<<EI(StackFlag::_count))-1),

        E4(HA::STACK_VALUE_REL,             LE, 100),
        E4(HA::STACK_VALUE_REL0,            LE, 100),
        E4(HA::STACK_VALUE_KILLED_REL,      LE, 100),
        E4(HA::STACK_VALUE_KILLED_ACC_REL0, LE, 100),
        E4(HA::STACK_VALUE_LOST_REL,        LE, 100),
        E4(HA::STACK_VALUE_LOST_ACC_REL0,   LE, 100),
        E4(HA::STACK_DMG_DEALT_REL,         LE, 100),
        E4(HA::STACK_DMG_DEALT_ACC_REL0,    LE, 100),
        E4(HA::STACK_DMG_RECEIVED_REL,      LE, 100),
        E4(HA::STACK_DMG_RECEIVED_ACC_REL0, LE, 100),
    };

    // Dedining encodings for each attribute by hand is error-prone
    // The below compile-time asserts are essential.
    static_assert(UninitializedEncodingAttributes(GLOBAL_ENCODING) == 0, "Found uninitialized elements");
    static_assert(UninitializedEncodingAttributes(PLAYER_ENCODING) == 0, "Found uninitialized elements");
    static_assert(UninitializedEncodingAttributes(HEX_ENCODING) == 0, "Found uninitialized elements");
    static_assert(DisarrayedEncodingAttributeIndex(GLOBAL_ENCODING) == -1, "Found wrong element at this index");
    static_assert(DisarrayedEncodingAttributeIndex(PLAYER_ENCODING) == -1, "Found wrong element at this index");
    static_assert(DisarrayedEncodingAttributeIndex(HEX_ENCODING) == -1, "Found wrong element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(GLOBAL_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(PLAYER_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(HEX_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(GLOBAL_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(PLAYER_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(HEX_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");

    constexpr int BATTLEFIELD_STATE_SIZE_GLOBAL = EncodedSize(GLOBAL_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ONE_PLAYER = EncodedSize(PLAYER_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ONE_HEX = EncodedSize(HEX_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ALL_HEXES = 165 * BATTLEFIELD_STATE_SIZE_ONE_HEX;
    constexpr int BATTLEFIELD_STATE_SIZE = \
        BATTLEFIELD_STATE_SIZE_GLOBAL + \
        BATTLEFIELD_STATE_SIZE_ONE_PLAYER + \
        BATTLEFIELD_STATE_SIZE_ONE_PLAYER + \
        BATTLEFIELD_STATE_SIZE_ALL_HEXES;
}
