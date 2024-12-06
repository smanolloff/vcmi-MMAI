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

#include "BAI/v5/encoder.h"
#include "StdInc.h"

#include "battle/CBattleInfoEssentials.h"

#include "BAI/v5/action.h"
#include "BAI/v5/hex.h"
#include <stdexcept>

namespace MMAI::BAI::V5 {
    // static
    std::unique_ptr<Hex> Action::initHex(const Schema::Action &a, const Battlefield * bf) {
        auto pa = initPrimaryAction(a);
        auto hexid = (a >> 8) & 0xFF; // 2nd byte
        auto y = hexid / BF_XMAX;
        auto x = hexid % BF_XMAX;

        switch (pa) {
        case PrimaryAction::RETREAT:
        case PrimaryAction::WAIT:
            return nullptr;
        case PrimaryAction::MOVE:
            return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
        case PrimaryAction::ATTACK_0:
        case PrimaryAction::ATTACK_1:
        case PrimaryAction::ATTACK_2:
        case PrimaryAction::ATTACK_3:
        case PrimaryAction::ATTACK_4:
        case PrimaryAction::ATTACK_5:
        case PrimaryAction::ATTACK_6:
        case PrimaryAction::ATTACK_7:
        case PrimaryAction::ATTACK_8:
        case PrimaryAction::ATTACK_9: {
            // XXX: shooting action does NOT use a hex
            auto shooter = bf->astack->cstack->canShoot();
            auto blocked = bf->astack->flag(StackFlag::BLOCKED);

            if (shooter && !blocked)
                return nullptr;

            // create a new unique_ptr with a copy of Hex
            return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
        }
        default:
            throw std::runtime_error("Unexpected primary action: " + std::to_string(EI(pa)));
        }
    }

    // static
    std::shared_ptr<Stack> Action::initStack(const Schema::Action &a, const Battlefield * bf) {
        auto pa = initPrimaryAction(a);
        switch (pa) {
        case PrimaryAction::RETREAT:
        case PrimaryAction::WAIT:
        case PrimaryAction::MOVE:
            return nullptr;
        case PrimaryAction::ATTACK_0:
        case PrimaryAction::ATTACK_1:
        case PrimaryAction::ATTACK_2:
        case PrimaryAction::ATTACK_3:
        case PrimaryAction::ATTACK_4:
        case PrimaryAction::ATTACK_5:
        case PrimaryAction::ATTACK_6:
        case PrimaryAction::ATTACK_7:
        case PrimaryAction::ATTACK_8:
        case PrimaryAction::ATTACK_9: {
            auto id = EI(pa) - EI(PrimaryAction::ATTACK_0);
            auto aside = bf->astack->attr(SA::SIDE);
            return bf->stacks->at(!aside).at(id);
        }
        default:
            throw std::runtime_error("Unexpected primary action: " + std::to_string(EI(pa)));
        }
    }

    // static
    PrimaryAction Action::initPrimaryAction(const Schema::Action &a) {
        return PrimaryAction(a & 0xFF);  // 1st byte
    }

    Action::Action(const Schema::Action action_, const Battlefield * bf, const std::string color_)
        : action(action_)
        , color(color_)
        , primaryAction(initPrimaryAction(action_))
        , stack(initStack(action_, bf))
        , hex(initHex(action_, bf))
        , isDefend(primaryAction == PrimaryAction::MOVE && hex->bhex == bf->astack->cstack->getPosition())
        , isShooting(bf->astack->cstack->canShoot() && !bf->astack->flag(StackFlag::BLOCKED))
        {}

    std::string Action::name() const {
        switch (primaryAction) {
        case PrimaryAction::RETREAT:
            return "Retreat";
        case PrimaryAction::WAIT:
            return "Wait";
        case PrimaryAction::MOVE:
            return isDefend ? "Defend" : "Move to " + hex->name();
        case PrimaryAction::ATTACK_0:
        case PrimaryAction::ATTACK_1:
        case PrimaryAction::ATTACK_2:
        case PrimaryAction::ATTACK_3:
        case PrimaryAction::ATTACK_4:
        case PrimaryAction::ATTACK_5:
        case PrimaryAction::ATTACK_6:
        case PrimaryAction::ATTACK_7:
        case PrimaryAction::ATTACK_8:
        case PrimaryAction::ATTACK_9: {
            // red/blue
            // this is the attack TARGET => colors are reversed
            auto col = (color == "red") ? "\033[34m" : "\033[31m";
            auto nocol = "\033[0m";
            auto stackstr = boost::str(boost::format("%s#%c%s") % col % stack->getAlias() % nocol);

            return isShooting
                ? "Shoot at " + stackstr
                : "Move to " + hex->name() + " and attack " + stackstr;
        }
        default:
            throw std::runtime_error("Unexpected primary action: " + std::to_string(EI(primaryAction)));
        }
    }
}

