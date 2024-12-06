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

#include "StdInc.h"

#include "battle/AccessibilityInfo.h"
#include "schema/v5/types.h"
#include "spells/CSpellHandler.h"
#include "vcmi/spells/Service.h"
#include "vcmi/spells/Spell.h"

#include "BAI/v5/hex.h"
#include "common.h"
#include "schema/v5/constants.h"

namespace MMAI::BAI::V5 {
    using A = Schema::V5::HexAttribute;
    using S = Schema::V5::HexState;
    using SA = Schema::V5::StackAttribute;

    constexpr HexStateMask S_PASSABLE = 1<<EI(HexState::PASSABLE);
    constexpr HexStateMask S_STOPPING = 1<<EI(HexState::STOPPING);
    constexpr HexStateMask S_DAMAGING_L = 1<<EI(HexState::DAMAGING_L);
    constexpr HexStateMask S_DAMAGING_R = 1<<EI(HexState::DAMAGING_R);
    constexpr HexStateMask S_DAMAGING_ALL = 1<<EI(HexState::DAMAGING_L) | 1<<EI(HexState::DAMAGING_R);

    // PrimaryActions <=> AMoveAction offset is 2
    constexpr int maskoffset = 2;
    static_assert(maskoffset + EI(AMoveAction::MOVE_ONLY) == EI(PrimaryAction::MOVE));
    static_assert(maskoffset + EI(AMoveAction::MOVE_AND_ATTACK_0) == EI(PrimaryAction::ATTACK_0));
    static_assert(maskoffset + EI(AMoveAction::MOVE_AND_ATTACK_9) == EI(PrimaryAction::ATTACK_9));
    static_assert(EI(AMoveAction::MOVE_ONLY) == 0);
    static_assert(EI(AMoveAction::MOVE_AND_ATTACK_9) == EI(AMoveAction::_count) - 1);

    // static
    PrimaryAction Hex::AMoveToPrimaryAction(AMoveAction ama) {
        return PrimaryAction(EI(ama) + maskoffset);
    }

    // static
    AMoveAction Hex::PrimaryToAMoveAction(PrimaryAction pa) {
        return AMoveAction(EI(pa) - maskoffset);
    }


    // static
    int Hex::CalcId(const BattleHex &bh) {
        ASSERT(bh.isAvailable(), "Hex unavailable: " + std::to_string(bh.hex));
        return bh.getX()-1 + bh.getY()*BF_XMAX;
    }

    // static
    std::pair<int, int> Hex::CalcXY(const BattleHex &bh) {
        return {bh.getX() - 1, bh.getY()};
    }

    Hex::Hex(
        const BattleHex &bhex_,
        const EAccessibility accessibility,
        const EGateState gatestate,
        const std::vector<std::shared_ptr<const CObstacleInstance>> &obstacles,
        const std::map<BattleHex, std::shared_ptr<Stack>> &hexstacks,
        const std::shared_ptr<ActiveStackInfo> &astackinfo
    ) : bhex(bhex_) {
        attrs.fill(NULL_VALUE_UNENCODED);

        auto [x, y] = CalcXY(bhex);
        auto it = hexstacks.find(bhex);
        stack = it == hexstacks.end() ? nullptr : it->second;

        setattr(A::Y_COORD, y);
        setattr(A::X_COORD, x);

        if (stack) {
            setattr(A::STACK_ID, stack->attr(SA::ID));
            setattr(A::STACK_SIDE, stack->attr(SA::SIDE));
        }

        if (astackinfo) {
            setStateMask(accessibility, obstacles, BattleSide(astackinfo->stack->attr(SA::SIDE)));
            setAmoveMask(astackinfo, hexstacks);
        } else {
            setStateMask(accessibility, obstacles, BattleSide::ATTACKER);
        }

        finalize();
    }

    Hex::Hex(const BattleHex &unavailable_bhex_) : bhex(unavailable_bhex_) {
        attrs.fill(NULL_VALUE_UNENCODED);
        auto [x, y] = CalcXY(bhex);
        stack = nullptr;
        setattr(A::Y_COORD, y);
        setattr(A::X_COORD, x);
        finalize();
    }

    const HexAttrs& Hex::getAttrs() const {
        return attrs;
    }

    int Hex::getAttr(HexAttribute a) const {
        return attr(a);
    }

    int Hex::attr(HexAttribute a) const { return attrs.at(EI(a)); };
    void Hex::setattr(HexAttribute a, int value) {
        attrs.at(EI(a)) = std::min(value, std::get<3>(HEX_ENCODING.at(EI(a))));
    };

    std::string Hex::name() const {
        // return boost::str(boost::format("(%d,%d)") % attr(A::Y_COORD) % attr(A::X_COORD));
        return "(" + std::to_string(attr(A::Y_COORD)) + "," + std::to_string(attr(A::X_COORD)) + ")";
    }

    void Hex::finalize() {
        attrs.at(EI(A::ACTION_MASK)) = amovemask.to_ulong();
        attrs.at(EI(A::STATE_MASK)) = statemask.to_ulong();
    }

    // private

    void Hex::setStateMask(
        const EAccessibility accessibility,
        const std::vector<std::shared_ptr<const CObstacleInstance>> &obstacles,
        BattleSide side
    ) {
        // First process obstacles
        // XXX: set only non-PASSABLE flags
        // (e.g. there may be a stack standing on the obstacle (firewall, moat))
        // so the PASSABLE mask bit will set later
        // XXX: moats are a weird obstacle:
        //      * if dispellable (Tower mines?) => type=SPELL_CREATED
        //      * otherwise => type=MOAT
        //      * their trigger ability is a spell, as it seems
        //        (which is not found in spells.json, neither is available as a SpellID constant)
        //
        //      Ref: Moat::placeObstacles()
        //           BattleEvaluator::goTowardsNearest() // var triggerAbility
        //

        for (auto &obstacle : obstacles) {
            switch (obstacle->obstacleType) {
            break; case CObstacleInstance::USUAL:
                   case CObstacleInstance::ABSOLUTE_OBSTACLE:
                statemask &= ~S_PASSABLE;
            break; case CObstacleInstance::MOAT:
                statemask |= (S_STOPPING | S_DAMAGING_ALL);
            break; case CObstacleInstance::SPELL_CREATED:
                // XXX: the public Obstacle / Spell API does not seem to expose
                //      any useful methods for checking if friendly creatures
                //      would get damaged by an obstacle.
                switch(SpellID(obstacle->ID)) {
                break; case SpellID::QUICKSAND:
                    statemask |= S_STOPPING;
                break; case SpellID::LAND_MINE:
                    auto casterside = dynamic_cast<const SpellCreatedObstacle *>(obstacle.get())->casterSide;
                    // XXX: in practice, there is no situation where enemy
                    //      mines are visible as the UI simply does not allow
                    //      to cast the spell in this case (e.g. if there is a
                    //      terrain-native stack in the enemy army).
                    statemask |= (side == casterside)
                        ? (side == BattleSide::DEFENDER ? S_DAMAGING_L : S_DAMAGING_R)
                        : (side == BattleSide::DEFENDER ? S_DAMAGING_R : S_DAMAGING_L);
                }
            break; default:
                THROW_FORMAT("Unexpected obstacle type: %d", EI(obstacle->obstacleType));
            }
        }

        switch(accessibility) {
        break; case EAccessibility::ACCESSIBLE:
            ASSERT(!stack, "accessibility is ACCESSIBLE, but a stack was found on hex");
            statemask |= S_PASSABLE;
        break; case EAccessibility::OBSTACLE:
            ASSERT(!stack, "accessibility is OBSTACLE, but a stack was found on hex");
            statemask &= ~S_PASSABLE;
        break; case EAccessibility::ALIVE_STACK:
            // XXX: stack can be NULL if it was left out of the observation
            // ASSERT(stack, "accessibility is ALIVE_STACK, but no stack was found on hex");
            statemask &= ~S_PASSABLE;
        break; case EAccessibility::DESTRUCTIBLE_WALL:
            // XXX: Destroyed walls become ACCESSIBLE.
            ASSERT(!stack, "accessibility is DESTRUCTIBLE_WALL, but a stack was found on hex");
            statemask &= ~S_PASSABLE;
        break; case EAccessibility::GATE:
            // See BattleProcessor::updateGateState() for gate states
            // See CBattleInfoCallback::getAccessibility() for accessibility on gate
            //
            // TL; DR:
            // -> GATE means closed, non-blocked gate
            // -> UNAVAILABLE means blocked
            // -> ACCESSIBLE otherwise (open, destroyed)
            //
            // Regardless of the gate state, we always set the GATE flag
            // purely based on the hex coordinates and not on the accessibility
            // => not setting GATE flag here
            //
            // However, in case of GATE accessibility, we still need
            // to set the PASSABLE flag accordingly.
            side == BattleSide::DEFENDER
                ? statemask.set(EI(S::PASSABLE))
                : statemask.reset(EI(S::PASSABLE));
        break; case EAccessibility::UNAVAILABLE:
            statemask &= ~S_PASSABLE;
        break; default:
            THROW_FORMAT("Unexpected hex accessibility for bhex %d: %d", bhex.hex % EI(accessibility));
        }

        // if (bhex == BattleHex::GATE_INNER || bhex == BattleHex::GATE_OUTER)
        //     statemask |= S_GATE;
    }

    void Hex::setAmoveMask(
        const std::shared_ptr<ActiveStackInfo> &astackinfo,
        const std::map<BattleHex, std::shared_ptr<Stack>> &hexstacks
    ) {
        auto astack = astackinfo->stack;

        // XXX: ReachabilityInfo::isReachable() must not be used as it
        //      returns true even if speed is insufficient => use distances.
        // NOTE: distances is 0 for the stack's main hex and 1 for its rear hex
        //       (100000 if it can't fit there)
        if (astackinfo->rinfo->distances.at(bhex) <= astack->attr(SA::SPEED)) {
            amovemask.set(EI(AMoveAction::MOVE_ONLY));
        } else {
            return; // astack can't even reach this hex, abort early
        }

        // astack can MOVE to hex => set AMOVE actions
        // ...unless it can shoot (we don't want melee attacks in that case)
        if (astackinfo->canshoot)
            return;

        for(auto nbhex : astack->cstack->getSurroundingHexes(bhex)) {
            auto it = hexstacks.find(nbhex);
            if (it == hexstacks.end())
                continue;

            auto &nstack = it->second;

            if (astack->attr(SA::SIDE) != nstack->attr(SA::SIDE))
                amovemask.set(EI(AMoveAction::MOVE_AND_ATTACK_0) + nstack->attr(SA::ID));
        }
    }
}
