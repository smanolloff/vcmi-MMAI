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

namespace MMAI::Schema::V5 {
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

    enum class HexState : int {
        // IMPASSABLE,         // obstacle/stack/gate(closed,attacker)
        PASSABLE,       // empty/mine/firewall/gate(open)/gate(closed,defender), ...
        STOPPING,       // moat/quicksand
        DAMAGING_L,     // moat/mine/firewall
        DAMAGING_R,     // moat/mine/firewall
        // GATE,               // XXX: redundant? Always set during siege (regardless of gate state)
        _count
    };

    enum class PrimaryAction : int {
        RETREAT,
        WAIT,

        // Hex is required
        MOVE,

        // Hex is optional (not required if stack can shoot)
        ATTACK_0,   // = attack (melee or ranged) at stack 0 / 10
        ATTACK_1,   // = attack (melee or ranged) at stack 1 / 11
        ATTACK_2,   // ...
        ATTACK_3,   //
        ATTACK_4,   //
        ATTACK_5,   //
        ATTACK_6,   //
        ATTACK_7,   //
        ATTACK_8,   //
        ATTACK_9,   //
        _count
    };

    // RETREAT action is version-agnostic (always 0)
    static_assert(EI(PrimaryAction::RETREAT) == ACTION_RETREAT);

    // Hex-specific actions (each hex's statemask is based on these)
    enum class AMoveAction : int {
        MOVE_ONLY,
        MOVE_AND_ATTACK_0,
        MOVE_AND_ATTACK_1,
        MOVE_AND_ATTACK_2,
        MOVE_AND_ATTACK_3,
        MOVE_AND_ATTACK_4,
        MOVE_AND_ATTACK_5,
        MOVE_AND_ATTACK_6,
        MOVE_AND_ATTACK_7,
        MOVE_AND_ATTACK_8,
        MOVE_AND_ATTACK_9,
        _count
    };

    enum class MiscAttribute : int {
        PRIMARY_ACTION_MASK,
        SHOOTING,
        INITIAL_ARMY_VALUE_LEFT,
        INITIAL_ARMY_VALUE_RIGHT,
        CURRENT_ARMY_VALUE_LEFT,
        CURRENT_ARMY_VALUE_RIGHT,
        _count
    };

    enum class StackAttribute : int {
        ID,
        SIDE,
        Y_COORD,
        X_COORD,
        IS_ACTIVE,
        CREATURE_ID,
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
        AI_VALUE,
        FLAGS,

        _count
    };

    enum class StackFlag : int {
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

    // For description on each attribute, see the comments for HEX_ENCODING
    enum class HexAttribute : int {
        Y_COORD,
        X_COORD,
        STATE_MASK,
        STACK_ID,
        STACK_SIDE,
        ACTION_MASK,
        _count
    };

    enum class ErrorCode : int {
        OK,
        INVALID,
        _count
    };

    class IMisc {
    public:
        virtual int getInitialArmyValueLeft() const = 0;
        virtual int getInitialArmyValueRight() const = 0;
        virtual int getCurrentArmyValueLeft() const = 0;
        virtual int getCurrentArmyValueRight() const = 0;
        virtual ~IMisc() = default;
    };

    using HexAttrs = std::array<int, static_cast<int>(HexAttribute::_count)>;
    using StackAttrs = std::array<int, static_cast<int>(StackAttribute::_count)>;
    using StackFlags = std::bitset<EI(StackFlag::_count)>;

    class IStack {
    public:
        virtual const StackAttrs& getAttrs() const = 0;
        virtual int getAttr(StackAttribute) const = 0;
        virtual char getAlias() const = 0;
        virtual ~IStack() = default;
    };


    class IHex {
    public:
        virtual const HexAttrs& getAttrs() const = 0;
        virtual int getAttr(HexAttribute) const = 0;
        virtual ~IHex() = default;
    };

    class IAttackLog {
    public:
        virtual IStack* getAttacker() const = 0;
        virtual IStack* getDefender() const = 0;
        virtual int getDamageDealt() const = 0;
        virtual int getUnitsKilled() const = 0;
        virtual int getValueKilled() const = 0;
        virtual ~IAttackLog() = default;
    };

    using AttackLogs = std::vector<IAttackLog*>;
    using Hexes = std::array<std::array<IHex*, 15>, 11>;
    using Stacks = std::array<std::array<IStack*, 10>, 2>;

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
        virtual int getDmgDealt() const = 0;
        virtual int getDmgReceived() const = 0;
        virtual int getUnitsLost() const = 0;
        virtual int getUnitsKilled() const = 0;
        virtual int getValueLost() const = 0;
        virtual int getValueKilled() const = 0;
        virtual bool getIsBattleEnded() const = 0;
        virtual bool getIsVictorious() const = 0;
        virtual const IMisc* getMisc() const = 0;
        virtual const Hexes getHexes() const = 0;
        virtual const Stacks getStacks() const = 0;
        virtual const AttackLogs getAttackLogs() const = 0;
        virtual const std::string getAnsiRender() const = 0;
        virtual ~ISupplementaryData() = default;
    };
}
