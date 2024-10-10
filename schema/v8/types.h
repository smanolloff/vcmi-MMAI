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

namespace MMAI::Schema::V8 {
    enum class Encoding : int {
        /*
         * Represent `v` as `n` bits, where `bits[1..v+1]=1`.
         * If `v=-1` (a.k.a. "NULL"), only the bit at index 0 will be `1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,1,1,1,1]`
         * * `v=0`,  `n=5` => `[0,1,0,0,0]`
         * * `v=-1`, `n=5` => `[1,0,0,0,0]`
         */
        ACCUMULATING_EXPLICIT_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[0..v]=1`.
         * If `v=-1` (a.k.a. "NULL"), all bits will be `0`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,1,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[0,0,0,0,0]`
         */
        ACCUMULATING_IMPLICIT_NULL,
        /*
         * Represent `v` as `n` bits, where `bits[0..v]=1`.
         * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,1,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
         */
        ACCUMULATING_MASKING_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[0..v]=1`.
         * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,1,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => (error)
         */
        ACCUMULATING_STRICT_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[0..v]=1`.
         * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,1,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[1,0,0,0,0]`
         */
        ACCUMULATING_ZERO_NULL,

        /*
         * Represent `v<<1` as `n`-bit binary (unsigned, LSB at index 0).
         * If `v=-1` (a.k.a. "NULL"), only the bit at index 0 will be `1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,1,1,0,0]`
         * * `v=0`,  `n=5` => `[0,0,0,0,0]`
         * * `v=-1`, `n=5` => `[1,0,0,0,0]`
         */
        BINARY_EXPLICIT_NULL,

        /*
         * Represent `v` as an `n`-bit binary (LSB at index 0)
         * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,0,0,0]`
         * * `v=0`,  `n=5` => `[0,0,0,0,0]`
         * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
         */
        BINARY_MASKING_NULL,

        /*
         * Represent `v` as an `n`-bit binary (LSB at index 0)
         * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,0,0,0]`
         * * `v=0`,  `n=5` => `[0,0,0,0,0]`
         * * `v=-1`, `n=5` => (error)
         */
        BINARY_STRICT_NULL,

        /*
         * Represent `v` as `n`-bit binary (unsigned, LSB at index 0).
         * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[1,1,0,0,0]`
         * * `v=0`,  `n=5` => `[0,0,0,0,0]`
         * * `v=-1`, `n=5` => `[0,0,0,0,0]`
         */
        BINARY_ZERO_NULL,
        // XXX: BINARY_ZERO_NULL obsoletes BINARY_IMPLICIT_NULL

        /*
         * Represent `v` as `n` bits, where `bits[v+1]=1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,0,0,0,1]`
         * * `v=0`,  `n=5` => `[0,1,0,0,0]`
         * * `v=-1`, `n=5` => `[1,0,0,0,0]`
         */
        CATEGORICAL_EXPLICIT_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[v]=1`.
         * If `v=-1` (a.k.a. "NULL"), all bits will be `0`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,0,0,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[0,0,0,0,0]`
         */
        CATEGORICAL_IMPLICIT_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[v]=1`.
         * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,0,0,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
         */
        CATEGORICAL_MASKING_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[v]=1`.
         * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,0,0,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => (error)
         */
        CATEGORICAL_STRICT_NULL,

        /*
         * Represent `v` as `n` bits, where `bits[v]=1`.
         * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
         *
         * Examples:
         * * `v=3`,  `n=5` => `[0,0,0,1,0]`
         * * `v=0`,  `n=5` => `[1,0,0,0,0]`
         * * `v=-1`, `n=5` => `[1,0,0,0,0]`
         */
        CATEGORICAL_ZERO_NULL,

        /*
         * Normalize `v+1` exponentially with base `vmax`
         * and represent it as `[0, vnorm]`.
         * If `v=-1` (a.k.a. "NULL"), the result will be `[1, 0]
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `[0, 0.6]`
         * * `v=0`,  `vmax=10` => `[0, 0]`
         * * `v=-1`, `vmax=10` => `[1, 0]`
         */
        EXPNORM_EXPLICIT_NULL,

        /*
         * Normalize `v+1` exponentially with base `vmax`.
         * If `v=0`, en error will be thrown.
         * If `v=-1` (a.k.a. "NULL"), it will not be normalized.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.6`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => `-1`
         */
        EXPNORM_MASKING_NULL,

        /*
         * Normalize `v+1` exponentially with base `vmax`.
         * If `v=0` or `v=-1` (a.k.a. "NULL"), an error will be thrown.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.6`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => (error)
         */
        EXPNORM_STRICT_NULL,

        /*
         * Normalize `v+1` exponentially with base `vmax`.
         * If `v=0`, en error will be thrown.
         * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.6`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => `0`
         */
        EXPNORM_ZERO_NULL,
        // XXX: NORMALIZED_ZERO_NULL obsoletes NORMALIZED_IMPLICIT_NULL

        /*
         * Normalize `v` linearly in the range `(0, vmax)`.
         * and represent it as `[0, vnorm]`.
         * If `v=-1` (a.k.a. "NULL"), the result will be `[1, 0]
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `[0, 0.3]`
         * * `v=0`,  `vmax=10` => `[0, 0]`
         * * `v=-1`, `vmax=10` => `[1, 0]`
         */
        LINNORM_EXPLICIT_NULL,

        /*
         * Normalize `v` linearly in the range `(0, vmax)`.
         * If `v=-1` (a.k.a. "NULL"), it will not be normalized.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.3`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => `-1`
         */
        LINNORM_MASKING_NULL,

        /*
         * Normalize `v` linearly in the range `(0, vmax)`.
         * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.3`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => (error)
         */
        LINNORM_STRICT_NULL,

        /*
         * Normalize `v` linearly in the range `(0, vmax)`.
         * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
         *
         * Examples:
         * * `v=3`,  `vmax=10` => `0.6`
         * * `v=0`,  `vmax=10` => `0`
         * * `v=-1`, `vmax=10` => `0`
         */
        LINNORM_ZERO_NULL,
        // XXX: NORMALIZED_ZERO_NULL obsoletes NORMALIZED_IMPLICIT_NULL
    };

    enum class CombatResult {
        LEFT_WINS,
        RIGHT_WINS,
        DRAW,
        NONE,

        _count
    };

    enum class StackActState : int {
        READY,          // will act this turn, not waited
        WAITING,        // will act this turn, already waited
        DONE,           // will not act this turn
        _count
    };

    enum class HexState : int {
        // IMPASSABLE,         // obstacle/stack/gate(closed,attacker)
        PASSABLE,       // empty/mine/firewall/gate(open)/gate(closed,defender), ...
        STOPPING,       // moat/quicksand
        DAMAGING_L,     // moat/mine/firewall
        DAMAGING_R,     // moat/mine/firewall
        // GATE,               // XXX: redundant? Always set during siege (regardless of gate state)
        _count
    };

    enum class HexAction : int {
        AMOVE_TR,   // = Move to (*) + attack at hex 0..11:
        AMOVE_R,    //  . . . . . . . . . 5 0 . . . .
        AMOVE_BR,   // . 1-hex:  . . . . 4 * 1 . . .
        AMOVE_BL,   //  . . . . . . . . . 3 2 . . . .
        AMOVE_L,    // . . . . . . . . . . . . . . .
        AMOVE_TL,   //  . . . . . . . . . . . . . . .
        MOVE,       // = Move to (defend if current hex)
        SHOOT,      // = shoot at
        _count
    };

    enum class GlobalAttribute : int {
        BATTLE_SIDE,                 // 0=left, 1=right
        BATTLE_WINNER,               // 0=left, 1=right (NA = battle not finished)
        BFIELD_VALUE_NOW_REL0,       // global_value_now / global_value_at_start

        _count
    };

    enum class PlayerAttribute : int {
        ARMY_VALUE_NOW_REL,         // side_army_value_now          / global_value_now
        ARMY_VALUE_NOW_REL0,        // side_army_value_now          / global_value_at_start
        VALUE_KILLED_REL,           // left_value_killed_this_turn  / global_value_last_turn
        VALUE_KILLED_ACC_REL0,      // left_value_killed_lifetime   / global_value_at_start
        VALUE_LOST_REL,             // left_value_lost_this_turn    / global_value_last_turn
        VALUE_LOST_ACC_REL0,        // left_value_lost_lifetime     / global_value_at_start
        DMG_DEALT_REL,              // left_dmg_dealt_this_turn     / global_hp_last_turn
        DMG_DEALT_ACC_REL0,         // left_dmg_dealt_lifetime      / global_hp_at_start
        DMG_RECEIVED_REL,           // left_dmg_taken_this_turn     / global_hp_last_turn
        DMG_RECEIVED_ACC_REL0,      // left_dmg_taken_lifetime      / global_hp_at_start

        _count
    };

    // For description on each attribute, see the comments for HEX_ENCODING
    enum class HexAttribute : int {
        Y_COORD,
        X_COORD,
        STATE_MASK,
        ACTION_MASK,
        IS_REAR,  // is this hex the rear hex of a stack
        STACK_SIDE,
        // STACK_CREATURE_ID,
        STACK_QUANTITY,
        STACK_ATTACK,
        STACK_DEFENSE,
        STACK_SHOTS,
        STACK_DMG_MIN,
        STACK_DMG_MAX,
        STACK_HP,
        STACK_HP_LEFT,
        STACK_SPEED,
        STACK_QUEUE_POS,
        // STACK_ESTIMATED_DMG,
        STACK_VALUE_ONE,
        STACK_FLAGS,

        STACK_VALUE_REL,
        STACK_VALUE_REL0,
        STACK_VALUE_KILLED_REL,
        STACK_VALUE_KILLED_ACC_REL0,
        STACK_VALUE_LOST_REL,
        STACK_VALUE_LOST_ACC_REL0,
        STACK_DMG_DEALT_REL,
        STACK_DMG_DEALT_ACC_REL0,
        STACK_DMG_RECEIVED_REL,
        STACK_DMG_RECEIVED_ACC_REL0,

        _count
    };

    enum class StackAttribute : int {
        SIDE,
        QUANTITY,
        ATTACK,
        DEFENSE,
        SHOTS,
        DMG_MIN,
        DMG_MAX,
        HP,
        HP_LEFT,
        SPEED,
        QUEUE_POS,
        // ESTIMATED_DMG,
        VALUE_ONE,
        FLAGS,


        // RELATIVE values
        VALUE_REL,               // stack_value_now          / global_value_now
        VALUE_REL0,              // stack_value_now          / global_value_at_start
        VALUE_KILLED_REL,        // value_killed_this_turn   / global_value_last_turn
        VALUE_KILLED_ACC_REL0,   // value_killed_lifetime    / global_value_at_start
        VALUE_LOST_REL,          // value_lost_this_turn     / global_value_last_turn
        VALUE_LOST_ACC_REL0,     // value_lost_lifetime      / global_value_at_start
        DMG_DEALT_REL,           // dmg_dealt_this_turn      / global_hp_last_turn
        DMG_DEALT_ACC_REL0,      // dmg_dealt_lifetime       / global_hp_at_start
        DMG_RECEIVED_REL,        // dmg_received_this_turn   / global_hp_last_turn
        DMG_RECEIVED_ACC_REL0,   // dmg_received_lifetime    / global_hp_at_start
        _count
    };

    enum class StackFlag : int {
        IS_ACTIVE,
        WILL_ACT,
        CAN_WAIT,
        CAN_RETALIATE,
        SLEEPING,
        BLOCKED,
        BLOCKING,
        IS_WIDE,
        FLYING,
        BLIND_LIKE_ATTACK,          // 1=20% chance (unicorns, medusas, basilisks, scorpicores)
        ADDITIONAL_ATTACK,          /*val: number of additional attacks to perform*/
        NO_MELEE_PENALTY,
        TWO_HEX_ATTACK_BREATH,      /*eg. dragons*/
        BLOCKS_RETALIATION,         /*eg. naga*/

        _count
    };

    enum class ErrorCode : int {
        OK,
        ALREADY_WAITED,
        MOVE_SELF,
        HEX_UNREACHABLE,
        HEX_BLOCKED,
        HEX_MELEE_NA,
        STACK_NA,
        STACK_DEAD,
        STACK_INVALID,
        CANNOT_SHOOT,
        FRIENDLY_FIRE,
        INVALID_DIR,
    };

    class IGlobalStats {
    public:

        virtual int getValueStart() const = 0;
        virtual int getValuePrev() const = 0;
        virtual int getValueNow() const = 0;
        virtual int getHPStart() const = 0;
        virtual int getHPPrev() const = 0;
        virtual int getHPNow() const = 0;

        virtual int getDmgDealtNow() const = 0;
        virtual int getDmgDealtTotal() const = 0;
        virtual int getDmgReceivedNow() const = 0;
        virtual int getDmgReceivedTotal() const = 0;
        virtual int getValueKilledNow() const = 0;
        virtual int getValueKilledTotal() const = 0;
        virtual int getValueLostNow() const = 0;
        virtual int getValueLostTotal() const = 0;

        virtual ~IGlobalStats() = default;
    };

    using HexAttrs = std::array<int, static_cast<int>(HexAttribute::_count)>;
    using StackAttrs = std::array<int, static_cast<int>(StackAttribute::_count)>;
    using StackFlags = std::bitset<EI(StackFlag::_count)>;

    class IStack {
    public:
        virtual const StackAttrs& getAttrs() const = 0;
        virtual int getAttr(StackAttribute) const = 0;
        virtual int getFlag(StackFlag) const = 0;
        virtual char getAlias() const = 0;
        virtual ~IStack() = default;
    };

    class IHex {
    public:
        virtual const HexAttrs& getAttrs() const = 0;
        virtual int getAttr(HexAttribute) const = 0;
        virtual const IStack* getStack() const = 0;
        virtual ~IHex() = default;
    };

    class IAttackLog {
    public:
        // NOTE: each of those can be nullptr if cstack was just resurrected/summoned
        virtual IStack* getAttacker() const = 0;
        virtual IStack* getDefender() const = 0;

        virtual int getDamageDealt() const = 0;
        virtual int getDamageDealtPercent() const = 0;
        virtual int getUnitsKilled() const = 0;
        virtual int getValueKilled() const = 0;
        virtual int getValueKilledPercent() const = 0;
        virtual ~IAttackLog() = default;
    };

    using AttackLogs = std::vector<IAttackLog*>;
    using Hexes = std::array<std::array<IHex*, 15>, 11>;
    using Stacks = std::vector<IStack*>;

    enum class Side : int {LEFT, RIGHT}; // corresponds to BattleSide::Type

    // This is returned as std::any by IState
    // => MMAI_DLL_LINKAGE is needed to ensure std::any_cast sees the same symbol
    class MMAI_DLL_LINKAGE ISupplementaryData {
    public:
        enum class Type : int {REGULAR, ANSI_RENDER};

        virtual Type getType() const = 0;
        virtual Side getSide() const = 0;
        virtual std::string getColor() const = 0;
        virtual ErrorCode getErrorCode() const = 0;
        virtual bool getIsBattleEnded() const = 0;
        virtual bool getIsVictorious() const = 0;
        virtual const IGlobalStats* getGlobalStatsLeft() const = 0;
        virtual const IGlobalStats* getGlobalStatsRight() const = 0;
        virtual const Hexes getHexes() const = 0;
        virtual const Stacks getStacks() const = 0;
        virtual const AttackLogs getAttackLogs() const = 0;
        virtual const std::string getAnsiRender() const = 0;
        virtual ~ISupplementaryData() = default;
    };
}
