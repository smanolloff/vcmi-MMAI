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
#include "schema/v12/expbin.h"
#include "schema/v12/types.h"
#include "schema/v12/util.h"

namespace MMAI::Schema::V12 {
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

        inline constexpr auto EBE = Encoding::EXPBIN_EXPLICIT_NULL;
        inline constexpr auto EBI = Encoding::EXPBIN_IMPLICIT_NULL;
        inline constexpr auto EBM = Encoding::EXPBIN_MASKING_NULL;
        inline constexpr auto EBS = Encoding::EXPBIN_STRICT_NULL;
        inline constexpr auto EBZ = Encoding::EXPBIN_ZERO_NULL;

        inline constexpr auto AEBE = Encoding::ACCUMULATING_EXPBIN_EXPLICIT_NULL;
        inline constexpr auto AEBI = Encoding::ACCUMULATING_EXPBIN_IMPLICIT_NULL;
        inline constexpr auto AEBM = Encoding::ACCUMULATING_EXPBIN_MASKING_NULL;
        inline constexpr auto AEBS = Encoding::ACCUMULATING_EXPBIN_STRICT_NULL;
        inline constexpr auto AEBZ = Encoding::ACCUMULATING_EXPBIN_ZERO_NULL;

        inline constexpr auto LBE = Encoding::LINBIN_EXPLICIT_NULL;
        inline constexpr auto LBI = Encoding::LINBIN_IMPLICIT_NULL;
        inline constexpr auto LBM = Encoding::LINBIN_MASKING_NULL;
        inline constexpr auto LBS = Encoding::LINBIN_STRICT_NULL;
        inline constexpr auto LBZ = Encoding::LINBIN_ZERO_NULL;

        inline constexpr auto ALBE = Encoding::ACCUMULATING_LINBIN_EXPLICIT_NULL;
        inline constexpr auto ALBI = Encoding::ACCUMULATING_LINBIN_IMPLICIT_NULL;
        inline constexpr auto ALBM = Encoding::ACCUMULATING_LINBIN_MASKING_NULL;
        inline constexpr auto ALBS = Encoding::ACCUMULATING_LINBIN_STRICT_NULL;
        inline constexpr auto ALBZ = Encoding::ACCUMULATING_LINBIN_ZERO_NULL;

        inline constexpr auto EE = Encoding::EXPNORM_EXPLICIT_NULL;
        inline constexpr auto EM = Encoding::EXPNORM_MASKING_NULL;
        inline constexpr auto ES = Encoding::EXPNORM_STRICT_NULL;
        inline constexpr auto EZ = Encoding::EXPNORM_ZERO_NULL;

        inline constexpr auto LE = Encoding::LINNORM_EXPLICIT_NULL;
        inline constexpr auto LM = Encoding::LINNORM_MASKING_NULL;
        inline constexpr auto LS = Encoding::LINNORM_STRICT_NULL;
        inline constexpr auto LZ = Encoding::LINNORM_ZERO_NULL;

        inline constexpr auto RAW = Encoding::RAW;

        using GA = GlobalAttribute;
        using PA = PlayerAttribute;
        using HA = HexAttribute;

        /*
         * The encoding schema `{a, e, n, vmax, p}`, where:
         * a=attribute
         * e=encoding
         * n=size
         * vmax=max_value
         * p=param (encoding-specific)
         */
        using E5G = std::tuple<GlobalAttribute, Encoding, int, int, double>;
        using E5P = std::tuple<PlayerAttribute, Encoding, int, int, double>;
        using E5H = std::tuple<HexAttribute, Encoding, int, int, double>;
    }

    using GlobalEncoding = std::array<E5G, EI(GlobalAttribute::_count)>;
    using PlayerEncoding = std::array<E5P, EI(PlayerAttribute::_count)>;
    using HexEncoding = std::array<E5H, EI(HexAttribute::_count)>;

    /*
     * Compile-time constructor for E5H and E5S tuples
     * https://stackoverflow.com/a/23784921
     */
    template<typename T>
    constexpr std::tuple<T, Encoding, int, int, double> E5(T a, Encoding e, int vmax, double slope = -1, int bins = -1) {
        constexpr auto maybeCalcExpBins = [](int vmax, double slope, int bins) {
            if (bins > 0) return bins;
            return MaxExpBins(vmax, slope);
        };

        // slope needs to be int for this calculation
        constexpr auto calcLinBins = [](int vmax, int slope) {
            return (vmax % slope == 0) + vmax / slope;
        };

        switch(e) {
        // "0" is a value => vmax+1 values
        case AE: return {a, e, vmax+2, vmax, -1};
        case AI: return {a, e, vmax+1, vmax, -1};
        case AM: return {a, e, vmax+1, vmax, -1};
        case AS: return {a, e, vmax+1, vmax, -1};
        case AZ: return {a, e, vmax+1, vmax, -1};

        // Log2(8)=3 (2^3), but if vmax=8 then 4 bits will be required
        // => Log2(9)=4
        case BE: return {a, e, static_cast<int>(Log2(vmax+1))+1, vmax, -1};
        case BM: return {a, e, static_cast<int>(Log2(vmax+1)), vmax, -1};
        case BS: return {a, e, static_cast<int>(Log2(vmax+1)), vmax, -1};
        case BZ: return {a, e, static_cast<int>(Log2(vmax+1)), vmax, -1};

        // "0" is a category => vmax+1 categories
        case CE: return {a, e, vmax+2, vmax, -1};
        case CI: return {a, e, vmax+1, vmax, -1};
        case CM: return {a, e, vmax+1, vmax, -1};
        case CS: return {a, e, vmax+1, vmax, -1};
        case CZ: return {a, e, vmax+1, vmax, -1};

        // For ExpBins, the slope can be used with different numbers of bins
        // If `bins` is not provided, MaxExpBins calculation is used
        case EBE: return {a, e, maybeCalcExpBins(vmax, slope, bins)+1, vmax, slope};
        case EBI: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case EBM: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case EBS: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case EBZ: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};

        case AEBE: return {a, e, maybeCalcExpBins(vmax, slope, bins)+1, vmax, slope};
        case AEBI: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case AEBM: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case AEBS: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};
        case AEBZ: return {a, e, maybeCalcExpBins(vmax, slope, bins), vmax, slope};

        // For LinBins, the slope corresponds to exactly one number of bins
        // => always calculate `bins`
        case LBE: return {a, e, calcLinBins(vmax, slope)+1, vmax, slope};
        case LBI: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case LBM: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case LBS: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case LBZ: return {a, e, calcLinBins(vmax, slope), vmax, slope};

        case ALBE: return {a, e, calcLinBins(vmax, slope)+1, vmax, slope};
        case ALBI: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case ALBM: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case ALBS: return {a, e, calcLinBins(vmax, slope), vmax, slope};
        case ALBZ: return {a, e, calcLinBins(vmax, slope), vmax, slope};

        case EE: return {a, e, 2, vmax, -1};
        case EM: return {a, e, 1, vmax, -1};
        case ES: return {a, e, 1, vmax, -1};
        case EZ: return {a, e, 1, vmax, -1};

        case LE: return {a, e, 2, vmax, -1};
        case LM: return {a, e, 1, vmax, -1};
        case LS: return {a, e, 1, vmax, -1};
        case LZ: return {a, e, 1, vmax, -1};

        case RAW: return {a, e, 1, vmax, -1};
        default:
            throw std::runtime_error("Unexpected encoding: " + std::to_string(EI(e)));
        }
    }

    // 0-6 regular; 7=war machines; 8=other (summoned, commander, etc.)
    constexpr int STACK_SLOT_WARMACHINES = 7;
    constexpr int STACK_SLOT_SPECIAL = 8;

    // Values above MAX are simply capped
    constexpr int STACK_QUEUE_SIZE = 30;
    constexpr int CREATURE_ID_MAX = 149;  // H3 core has creature IDs 0..149
    constexpr int STACK_SLOT_MAX = 8;

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


    /*
    BASH code snipped for printing the bins. Args:
    vmax n slope

python -c '
import numpy as np, math, sys;
vmax, n, slope = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
x = np.linspace(0, 1, n + 1)
bins = (np.exp(slope * x) - 1) / (np.exp(slope) - 1) * vmax
print([math.ceil(b) for b in bins])
 ' 1500 30 6.5

    */

    constexpr auto BFIELD_VALUE_MAX = int(5e6);  // 4.2M max for 4x1024.vmap
    constexpr auto BFIELD_VALUE_NBINS = 50;
    constexpr auto BFIELD_VALUE_SLOPE = 7.5;
    // 10000000 50 7
    // [0, 1372, 2950, 4765, 6852, 9253, 12015, 15192, 18847, 23050, 27886,
    //  33448, 39846, 47205, 55670, 65407, 76608, 89491, 104311, 121358,
    //  ...
    //  4961259, 5708178, 6567339, 7555609, 8692390, 10000000]

    constexpr auto BFIELD_HP_MAX = int(100e3);  // 90k max for 4x1024.vmap
    constexpr auto BFIELD_HP_NBINS = 50;
    constexpr auto BFIELD_HP_SLOPE = 7.5;
    // 200000 50 7
    // [0, 28, 59, 96, 138, 186, 241, 304, 377, 461, 558, 669, 797, 945, 1114,
    //  1309, 1533, 1790, 2087, 2428, 2820, 3271, 3790, 4387, 5073, 5863, 6771,
    //  7816, 9018, 10401, 11991, 13820, 15925, 18345, 21129, 24332, 28015, 32253,
    //  37127, 42733, 49182, 56601, 65133, 74949, 86239, 99226, 114164, 131347,
    //  151113, 173848, 200000]

    constexpr GlobalEncoding GLOBAL_ENCODING {
        E5(GA::BATTLE_SIDE,                 CS, 1),
        E5(GA::BATTLE_SIDE_ACTIVE_PLAYER,   CE, 1),         // NULL means no battle
        E5(GA::BATTLE_WINNER,               CE, 1),         // NULL means ongoing battle
        E5(GA::BFIELD_VALUE_START_ABS,      LS, BFIELD_VALUE_MAX),
        // E5(GA::BFIELD_VALUE_START_ABS,      ES, BFIELD_VALUE_MAX),
        // E5(GA::BFIELD_VALUE_START_ABS_BINS, AEBS, BFIELD_VALUE_MAX, BFIELD_VALUE_SLOPE, BFIELD_VALUE_NBINS),
        E5(GA::BFIELD_VALUE_NOW_ABS,        LS, BFIELD_VALUE_MAX),
        // E5(GA::BFIELD_VALUE_NOW_ABS,        ES, BFIELD_VALUE_MAX),
        // E5(GA::BFIELD_VALUE_NOW_ABS_BINS,   AEBS, BFIELD_VALUE_MAX, BFIELD_VALUE_SLOPE, BFIELD_VALUE_NBINS),
        E5(GA::BFIELD_VALUE_NOW_REL0,       LS, 1000),      // bfield_value_now          / bfield_value_at_start
        E5(GA::BFIELD_HP_START_ABS,         LS, BFIELD_HP_MAX),
        // E5(GA::BFIELD_HP_START_ABS,         ES, BFIELD_HP_MAX),
        // E5(GA::BFIELD_HP_START_ABS_BINS,    AEBS, BFIELD_HP_MAX, BFIELD_HP_SLOPE, BFIELD_HP_NBINS),
        E5(GA::BFIELD_HP_NOW_ABS,           LS, BFIELD_HP_MAX),
        // E5(GA::BFIELD_HP_NOW_ABS,           ES, BFIELD_HP_MAX),
        // E5(GA::BFIELD_HP_NOW_ABS_BINS,      AEBS, BFIELD_HP_MAX, BFIELD_HP_SLOPE, BFIELD_HP_NBINS),
        E5(GA::BFIELD_HP_NOW_REL0,          LS, 1000),      // bfield_hp_now             / bfield_hp_at_start
        E5(GA::ACTION_MASK,                 BS, (1<<EI(GlobalAction::_count))-1)
    };

    constexpr auto VALUE_KILLED_NOW_MAX = int(800e3); // 4x1096 has 98 Ghost dragons => ~4K dmg => 8K (+22 attack)
    constexpr auto VALUE_KILLED_NOW_NBINS = 50;     // ^ vs. grand elf(15 HP) = 667 kills * 1.8k value = 1.2M
    constexpr auto VALUE_KILLED_NOW_SLOPE = 7.5;    // granularity at low values OK (1 imp = 213)
    // 2000000 50 6
    // [0, 634, 1349, 2154, 3062, 4086, 5241, 6543, 8010, 9665, 11531, 13635,
    //  16007, 18681, 21697, 25096, 28930, 33252, 38125, 43619, 49814, 56799,
    //  64674, 73553, 83564, 94852, 107579, 121929, 138108, 156350, 176917,
    //  200107, 226254, 255734, 288973, 326450, 368705, 416347, 470064, 530629,
    //  598916, 675909, 762719, 860597, 970954, 1095381, 1235673, 1393851,
    //  1572196, 1773279, 2000000]

    constexpr auto DMG_DEALT_NOW_MAX = int(5e3);
    constexpr auto DMG_DEALT_NOW_NBINS = 50;
    constexpr auto DMG_DEALT_NOW_SLOPE = 7.5;
    // 200000 50 7.5
    // [0, 18, 39, 63, 91, 124, 162, 206, 257, 317, 386, 466, 559, 668, 794,
    //  940, 1110, 1307, 1537, 1803, 2113, 2473, 2891, 3376, 3940, 4596, 5358,
    //  6242, 7271, 8465, 9853, 11465, 13338, 15515, 18043, 20981, 24395,
    //  28360, 32968, 38321, 44541, 51767, 60162, 69916, 81249, 94415, 109713,
    //  127486, 148135, 172127, 200000]

    constexpr PlayerEncoding PLAYER_ENCODING {
        E5(PA::BATTLE_SIDE,               CS, 1),
        E5(PA::ARMY_VALUE_NOW_ABS,        LS, BFIELD_VALUE_MAX),
        // E5(PA::ARMY_VALUE_NOW_ABS,        ES, BFIELD_VALUE_MAX),
        // E5(PA::ARMY_VALUE_NOW_ABS_BINS,   AEBS, BFIELD_VALUE_MAX, BFIELD_VALUE_SLOPE, BFIELD_VALUE_NBINS),
        E5(PA::ARMY_VALUE_NOW_REL,        LS, 1000),       // army_value_now          / global_value_now
        E5(PA::ARMY_VALUE_NOW_REL0  ,     LS, 1000),       // army_value_now          / global_value_at_start
        E5(PA::ARMY_HP_NOW_ABS,           LS, BFIELD_HP_MAX),
        // E5(PA::ARMY_HP_NOW_ABS,           ES, BFIELD_HP_MAX),
        // E5(PA::ARMY_HP_NOW_ABS_BINS,      AEBS, BFIELD_HP_MAX, BFIELD_HP_SLOPE, BFIELD_HP_NBINS),
        E5(PA::ARMY_HP_NOW_REL,           LS, 1000),       // army_hp_now             / global_hp_now
        E5(PA::ARMY_HP_NOW_REL0,          LS, 1000),       // army_hp_now             / global_hp_at_start
        E5(PA::VALUE_KILLED_NOW_ABS,      LS, VALUE_KILLED_NOW_MAX),
        // E5(PA::VALUE_KILLED_NOW_ABS,      ES, VALUE_KILLED_NOW_MAX),
        // E5(PA::VALUE_KILLED_NOW_ABS_BINS, AEBS, VALUE_KILLED_NOW_MAX, VALUE_KILLED_NOW_SLOPE, VALUE_KILLED_NOW_NBINS),
        E5(PA::VALUE_KILLED_NOW_REL,      LS, 1000),       // value_killed_this_turn  / global_value_last_turn
        E5(PA::VALUE_KILLED_ACC_ABS,      LS, BFIELD_VALUE_MAX),
        // E5(PA::VALUE_KILLED_ACC_ABS,      ES, BFIELD_VALUE_MAX),
        // E5(PA::VALUE_KILLED_ACC_ABS_BINS, AEBS, BFIELD_VALUE_MAX, BFIELD_VALUE_SLOPE, BFIELD_VALUE_NBINS),
        E5(PA::VALUE_KILLED_ACC_REL0,     LS, 1000),       // value_killed_lifetime   / global_value_at_start
        E5(PA::VALUE_LOST_NOW_ABS,        LS, VALUE_KILLED_NOW_MAX),
        // E5(PA::VALUE_LOST_NOW_ABS,        ES, VALUE_KILLED_NOW_MAX),
        // E5(PA::VALUE_LOST_NOW_ABS_BINS,   AEBS, VALUE_KILLED_NOW_MAX, VALUE_KILLED_NOW_SLOPE, VALUE_KILLED_NOW_NBINS),
        E5(PA::VALUE_LOST_NOW_REL,        LS, 1000),       // value_lost_this_turn    / global_value_last_turn
        E5(PA::VALUE_LOST_ACC_ABS,        LS, BFIELD_VALUE_MAX),
        // E5(PA::VALUE_LOST_ACC_ABS,        ES, BFIELD_VALUE_MAX),
        // E5(PA::VALUE_LOST_ACC_ABS_BINS,   AEBS, BFIELD_VALUE_MAX, BFIELD_VALUE_SLOPE, BFIELD_VALUE_NBINS),
        E5(PA::VALUE_LOST_ACC_REL0,       LS, 1000),       // value_lost_lifetime     / global_value_at_start
        E5(PA::DMG_DEALT_NOW_ABS,         LS, DMG_DEALT_NOW_MAX),
        // E5(PA::DMG_DEALT_NOW_ABS,         ES, DMG_DEALT_NOW_MAX),
        // E5(PA::DMG_DEALT_NOW_ABS_BINS,    AEBS, DMG_DEALT_NOW_MAX, DMG_DEALT_NOW_SLOPE, DMG_DEALT_NOW_NBINS),
        E5(PA::DMG_DEALT_NOW_REL,         LS, 1000),       // dmg_dealt_this_turn     / global_hp_last_turn
        E5(PA::DMG_DEALT_ACC_ABS,         LS, BFIELD_HP_MAX),
        // E5(PA::DMG_DEALT_ACC_ABS,         ES, BFIELD_HP_MAX),
        // E5(PA::DMG_DEALT_ACC_ABS_BINS,    AEBS, BFIELD_HP_MAX, BFIELD_HP_SLOPE, BFIELD_HP_NBINS),
        E5(PA::DMG_DEALT_ACC_REL0,        LS, 1000),       // dmg_dealt_lifetime      / global_hp_at_start
        E5(PA::DMG_RECEIVED_NOW_ABS,      LS, DMG_DEALT_NOW_MAX),
        // E5(PA::DMG_RECEIVED_NOW_ABS,      ES, DMG_DEALT_NOW_MAX),
        // E5(PA::DMG_RECEIVED_NOW_ABS_BINS, AEBS, DMG_DEALT_NOW_MAX, DMG_DEALT_NOW_SLOPE, DMG_DEALT_NOW_NBINS),
        E5(PA::DMG_RECEIVED_NOW_REL,      LS, 1000),       // dmg_received_this_turn  / global_hp_last_turn
        E5(PA::DMG_RECEIVED_ACC_ABS,      LS, BFIELD_HP_MAX),
        // E5(PA::DMG_RECEIVED_ACC_ABS,      ES, BFIELD_HP_MAX),
        // E5(PA::DMG_RECEIVED_ACC_ABS_BINS, AEBS, BFIELD_HP_MAX, BFIELD_HP_SLOPE, BFIELD_HP_NBINS),
        E5(PA::DMG_RECEIVED_ACC_REL0,     LS, 1000),       // dmg_received_lifetime   / global_hp_at_start
    };

    constexpr auto STACK_QTY_MAX = 1500;
    constexpr auto STACK_QTY_NBINS = 25;
    constexpr auto STACK_QTY_SLOPE = 6.5;
    // 1500 25 6.5
    // [0,   1,   2,   3,   5,   7,   9,   12,   16,
    //  22,  29,  38,  49,  65,  84,  110, 143,  186,
    //  242, 314, 408, 529, 687, 891, 1157, 1500]

    constexpr auto STACK_DMG_MAX = 80;
    constexpr auto STACK_DMG_NBINS = 15;
    constexpr auto STACK_DMG_SLOPE = 3.3;
    // 80 3.3 15
    // [0, 1, 2, 3, 5, 7, 9, 12, 15, 20, 25, 32, 40, 51, 64, 80]

    constexpr auto STACK_HP_MAX = 1000;
    constexpr auto STACK_HP_NBINS = 18;
    constexpr auto STACK_HP_SLOPE = 6;
    // 1000 18 6
    // [0, 1, 3, 5, 7, 11, 16, 24, 34, 48, 68, 95, 134, 187, 262, 367, 513, 716, 1000]

    constexpr auto STACK_VALUE_MAX = 80000;  // titan 55k, crystal dr. 113k, azure 180k...
    constexpr auto STACK_VALUE_NBINS = 20;
    constexpr auto STACK_VALUE_SLOPE = 6.5;
    // 60000 20 6.5
    // [0, 35, 83, 150, 242, 369, 545, 789, 1127, 1594, 2240, 3135,
    //  4373, 6087, 8459, 11743, 16287, 22576, 31280, 43327, 60000]


    constexpr HexEncoding HEX_ENCODING {
        E5(HA::Y_COORD,                 CS, 10),
        E5(HA::X_COORD,                 CS, 14),
        E5(HA::STATE_MASK,              BS, (1<<EI(HexState::_count))-1),
        E5(HA::ACTION_MASK,             BZ, (1<<EI(HexAction::_count))-1),
        E5(HA::IS_REAR,                 CE, 1),        // 1=this is the rear hex of a stack
        E5(HA::STACK_SIDE,              CE, 1),        // 0=attacker, 1=defender
        // E5(HA::STACK_CREATURE_ID,       CE, CREATURE_ID_MAX),
        E5(HA::STACK_SLOT,              CE, STACK_SLOT_MAX),
        E5(HA::STACK_QUANTITY,          LE, STACK_QTY_MAX),
        // E5(HA::STACK_QUANTITY,          EE, STACK_QTY_MAX, STACK_QTY_SLOPE),
        // E5(HA::STACK_QUANTITY_BINS,     AEBI, STACK_QTY_MAX, STACK_QTY_SLOPE, STACK_QTY_NBINS),
        E5(HA::STACK_ATTACK,            LE, 60),
        // E5(HA::STACK_ATTACK,            EE, 60),
        // E5(HA::STACK_ATTACK_BINS,       ALBI, 60, 5),
        E5(HA::STACK_DEFENSE,           LE, 60),        // azure dragon is 60 when defending
        // E5(HA::STACK_DEFENSE,           EE, 60),        // azure dragon is 60 when defending
        // E5(HA::STACK_DEFENSE_BINS,      ALBI, 60, 5),   // azure dragon is 60 when defending
        E5(HA::STACK_SHOTS,             EZ, 32),        // sharpshooter is 32
        E5(HA::STACK_DMG_MIN,           LE, STACK_DMG_MAX),
        // E5(HA::STACK_DMG_MIN,           EE, STACK_DMG_MAX, STACK_DMG_SLOPE),
        // E5(HA::STACK_DMG_MIN_BINS,      AEBI, STACK_DMG_MAX, STACK_DMG_SLOPE, STACK_DMG_NBINS),
        E5(HA::STACK_DMG_MAX,           LE, STACK_DMG_MAX),
        // E5(HA::STACK_DMG_MAX,           EE, STACK_DMG_MAX, STACK_DMG_SLOPE),
        // E5(HA::STACK_DMG_MAX_BINS,      AEBI, STACK_DMG_MAX, STACK_DMG_SLOPE, STACK_DMG_NBINS),
        E5(HA::STACK_HP,                LE, STACK_HP_MAX),
        // E5(HA::STACK_HP,                EE, STACK_HP_MAX, STACK_HP_SLOPE),
        // E5(HA::STACK_HP_BINS,           AEBI, STACK_HP_MAX, STACK_HP_SLOPE, STACK_HP_NBINS),
        E5(HA::STACK_HP_LEFT,           LE, STACK_HP_MAX),
        // E5(HA::STACK_HP_LEFT_REL,       LE, 1000),
        E5(HA::STACK_SPEED,             CE, 18),
        E5(HA::STACK_QUEUE,             BZ, (1<<STACK_QUEUE_SIZE)-1),       // 0..14, 0=active stack
        // H4(SSTACK_A::ESTIMATED_DMG,  NE, 1000),      // est. dmg by the active stack as a permillage of this stack's total HP
        E5(HA::STACK_VALUE_ONE,         LE, STACK_VALUE_MAX),
        // E5(HA::STACK_VALUE_ONE,         EE, STACK_VALUE_MAX, STACK_VALUE_SLOPE),
        // E5(HA::STACK_VALUE_ONE_BINS,    AEBI, STACK_VALUE_MAX, STACK_VALUE_SLOPE, STACK_VALUE_NBINS),
        E5(HA::STACK_FLAGS1,            BE, (1<<EI(StackFlag1::_count))-1),
        E5(HA::STACK_FLAGS2,            BE, (1<<EI(StackFlag2::_count))-1),

        E5(HA::STACK_VALUE_REL,             LE, 1000),
        E5(HA::STACK_VALUE_REL0,            LE, 1000),
        E5(HA::STACK_VALUE_KILLED_REL,      LE, 1000),
        E5(HA::STACK_VALUE_KILLED_ACC_REL0, LE, 1000),
        E5(HA::STACK_VALUE_LOST_REL,        LE, 1000),
        E5(HA::STACK_VALUE_LOST_ACC_REL0,   LE, 1000),
        E5(HA::STACK_DMG_DEALT_REL,         LE, 1000),
        E5(HA::STACK_DMG_DEALT_ACC_REL0,    LE, 1000),
        E5(HA::STACK_DMG_RECEIVED_REL,      LE, 1000),
        E5(HA::STACK_DMG_RECEIVED_ACC_REL0, LE, 1000),
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


    constexpr auto test_attr = HA::STACK_DMG_MIN;
    constexpr auto test_n = std::get<2>(HEX_ENCODING.at(EI(test_attr)));
    constexpr auto test_vmax = std::get<3>(HEX_ENCODING.at(EI(test_attr)));
    constexpr auto test_p = std::get<4>(HEX_ENCODING.at(EI(test_attr)));

    constexpr auto test_x = FindDeadExpBin(test_vmax, test_n-1, test_p);
    constexpr auto test_x1 = HasDedicatedZeroExpBin(test_vmax, test_n, test_p);
    constexpr auto test_mb = MaxExpBins(test_vmax, test_p);
    constexpr auto test_ebvmin = ExpBinValueMin(1, test_vmax, test_n - 1, test_p);
    constexpr auto test_ebvmax = ExpBinValueMax(1, test_vmax, test_n - 1, test_p);

    static_assert(MisconfiguredBinAttributeIndex(HEX_ENCODING) == -1, "Found misconfigured logbin/expbin element at this index");
    static_assert(DeadBinAttributeIndex(HEX_ENCODING) == -1, "Found dead bins at this index");

    // For STACK_VALUE_ONE, the first bin is basically zero since peasant has 75 value
    // => disable this check for that attr
    static_assert(
        NoDedicatedZeroBinAttributeIndex(HEX_ENCODING) == -1
        || NoDedicatedZeroBinAttributeIndex(HEX_ENCODING) == EI(HA::STACK_VALUE_ONE),
        "No dedicated zero-bin at this index"
    );


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
