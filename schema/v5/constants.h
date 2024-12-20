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
#include "schema/v5/types.h"
#include "schema/v5/util.h"

namespace MMAI::Schema::V5 {
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

        using MA = MiscAttribute;
        using HA = HexAttribute;
        using SA = StackAttribute;

        /*
         * The encoding schema `{a, e, n, vmax}`, where:
         * `a`=attribute, `e`=encoding, `n`=size, `vmax`=max_value.
         */
        using E4M = std::tuple<MiscAttribute, Encoding, int, int>;
        using E4H = std::tuple<HexAttribute, Encoding, int, int>;
        using E4S = std::tuple<StackAttribute, Encoding, int, int>;
    }

    using MiscEncoding = std::array<E4M, EI(MiscAttribute::_count)>;
    using HexEncoding = std::array<E4H, EI(HexAttribute::_count)>;
    using StackEncoding = std::array<E4S, EI(StackAttribute::_count)>;

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

    constexpr int STACK_QTY_MAX = 5000;
    constexpr int STACK_VALUE_MAX = 80000;  // archangel 9K, crystal dragon 39K, azure dragon 79K
    constexpr int ARMY_VALUE_MAX = 5000000;
    constexpr int CREATURE_ID_MAX = 149;  // H3 core has creature IDs 0..149

    constexpr int MAX_STACKS_PER_SIDE = std::tuple_size<Stacks::value_type>::value;

    constexpr MiscEncoding MISC_ENCODING {
        E4(MA::PRIMARY_ACTION_MASK,      BS, (1<<EI(PrimaryAction::_count))-1),
        E4(MA::SHOOTING,                 CS, 1),
        E4(MA::INITIAL_ARMY_VALUE_LEFT,  ES, ARMY_VALUE_MAX),
        E4(MA::INITIAL_ARMY_VALUE_RIGHT, ES, ARMY_VALUE_MAX),
        E4(MA::CURRENT_ARMY_VALUE_LEFT,  ES, ARMY_VALUE_MAX),
        E4(MA::CURRENT_ARMY_VALUE_RIGHT, ES, ARMY_VALUE_MAX),
    };

    constexpr HexEncoding HEX_ENCODING {
        E4(HA::Y_COORD,      CS, 10),
        E4(HA::X_COORD,      CS, 14),
        E4(HA::STATE_MASK,   BS, (1<<EI(HexState::_count))-1),
        E4(HA::STACK_ID,     CE, MAX_STACKS_PER_SIDE-1),
        E4(HA::STACK_SIDE,   CE, 1),
        E4(HA::ACTION_MASK,  BZ, (1<<EI(AMoveAction::_count))-1),
    };

    constexpr StackEncoding STACK_ENCODING {
        E4(SA::ID,                        CE, MAX_STACKS_PER_SIDE-1),
        E4(SA::SIDE,                      CE, 1),        // 0=attacker, 1=defender
        E4(SA::Y_COORD,                   CE, 10),
        E4(SA::X_COORD,                   CE, 14),
        E4(SA::IS_ACTIVE,                 CE, 1),
        E4(SA::CREATURE_ID,               CE, CREATURE_ID_MAX),
        E4(SA::QUANTITY,                  EE, STACK_QTY_MAX),
        E4(SA::ATTACK,                    EE, 80),
        E4(SA::DEFENSE,                   EE, 80),       // azure dragon is 60 when defending
        E4(SA::SHOTS,                     EE, 32),       // sharpshooter is 32
        E4(SA::DMG_MIN,                   EE, 100),      // azure dragon is 80
        E4(SA::DMG_MAX,                   EE, 100),      // azure dragon is 80
        E4(SA::HP,                        EE, 1300),     // azure dragon + all artifacts is 1254
        E4(SA::HP_LEFT,                   EE, 1300),
        E4(SA::SPEED,                     EE, 30),       // at 19=full reach; max is... 37?
        E4(SA::QUEUE_POS,                 EE, 15),       // 0..14, 0=active stack
        // E4(SA::ESTIMATED_DMG,             NE, 100),      // est. dmg by the active stack as a percentage of this stack's total HP
        E4(SA::AI_VALUE,                  EE, STACK_VALUE_MAX),
        E4(SA::FLAGS,                     BE, (1<<EI(StackFlag::_count))-1),
    };

    // Dedining encodings for each attribute by hand is error-prone
    // The below compile-time asserts are essential.
    static_assert(UninitializedEncodingAttributes(MISC_ENCODING) == 0, "Found uninitialized elements");
    static_assert(UninitializedEncodingAttributes(HEX_ENCODING) == 0, "Found uninitialized elements");
    static_assert(UninitializedEncodingAttributes(STACK_ENCODING) == 0, "Found uninitialized elements");
    static_assert(DisarrayedEncodingAttributeIndex(MISC_ENCODING) == -1, "Found wrong element at this index");
    static_assert(DisarrayedEncodingAttributeIndex(HEX_ENCODING) == -1, "Found wrong element at this index");
    static_assert(DisarrayedEncodingAttributeIndex(STACK_ENCODING) == -1, "Found wrong element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(MISC_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(HEX_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeIndex(STACK_ENCODING) == -1, "Found miscalculated binary vmax element at this index");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(MISC_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(HEX_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");
    static_assert(MiscalculatedBinaryAttributeUnusedValues(STACK_ENCODING) == 0, "Number of unused values in the binary attribute is not 0");

    constexpr int BATTLEFIELD_STATE_SIZE_MISC = EncodedSize(MISC_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ONE_STACK = EncodedSize(STACK_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ALL_STACKS = 2 * MAX_STACKS_PER_SIDE * BATTLEFIELD_STATE_SIZE_ONE_STACK;
    constexpr int BATTLEFIELD_STATE_SIZE_ONE_HEX = EncodedSize(HEX_ENCODING);
    constexpr int BATTLEFIELD_STATE_SIZE_ALL_HEXES = 165 * BATTLEFIELD_STATE_SIZE_ONE_HEX;
    constexpr int BATTLEFIELD_STATE_SIZE =
        BATTLEFIELD_STATE_SIZE_MISC +
        BATTLEFIELD_STATE_SIZE_ALL_STACKS +
        BATTLEFIELD_STATE_SIZE_ALL_HEXES;

    constexpr int N_ACTIONS = EI(PrimaryAction::_count) + 165 * EI(AMoveAction::_count);
}
