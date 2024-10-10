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

#include "battle/CBattleInfoEssentials.h"

#include "BAI/v9/action.h"
#include "BAI/v9/hex.h"
#include "BAI/v9/hexaction.h"

namespace MMAI::BAI::V9 {
    // static
    std::unique_ptr<Hex> Action::initHex(const Schema::Action &a, const Battlefield * bf) {
        // Control actions (<0) should never reach here
        ASSERT(a >= 0 && a < N_ACTIONS, "Invalid action: " + std::to_string(a));

        auto i = a - EI(NonHexAction::count);

        if (i < 0) return nullptr;

        i = i / EI(HexAction::_count);
        auto y = i / BF_XMAX;
        auto x = i % BF_XMAX;

        // create a new unique_ptr with a copy of Hex
        return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
    }

    // static
    std::unique_ptr<Hex> Action::initAMoveTargetHex(const Schema::Action &a, const Battlefield * bf) {
        auto hex = initHex(a, bf);
        if (!hex) return nullptr;

        auto ha = initHexAction(a, bf);
        if (EI(ha) == -1) return nullptr;

        if (ha == HexAction::MOVE || ha == HexAction::SHOOT)
            return nullptr;
            // throw std::runtime_error("MOVE and SHOOT are not AMOVE actions");


        auto &bh = hex->bhex;

        auto edir = AMOVE_TO_EDIR.at(EI(ha));
        auto nbh = bh.cloneInDirection(edir);

        ASSERT(nbh.isAvailable(), "unavailable AMOVE target hex #" + std::to_string(nbh.toInt()));

        auto [x, y] = Hex::CalcXY(nbh);

        // create a new unique_ptr with a copy of Hex
        return std::make_unique<Hex>(*bf->hexes->at(y).at(x));
    }

    // static
    HexAction Action::initHexAction(const Schema::Action &a, const Battlefield * bf) {
        if(a < EI(NonHexAction::count)) return HexAction(-1); // a is not about a hex
        return HexAction((a-EI(NonHexAction::count)) % EI(HexAction::_count));
    }

    Action::Action(const Schema::Action action_, const Battlefield * bf, const std::string color_)
        : action(action_)
        , hex(initHex(action_, bf))
        , aMoveTargetHex(initAMoveTargetHex(action_, bf))
        , hexaction(initHexAction(action_, bf))
        , color(color_) {}

    std::string Action::name() const {
        if (action == ACTION_RETREAT)
            return "Retreat";
        else if (action == ACTION_WAIT)
            return "Wait";

        ASSERT(hex, "hex is null");

        auto ha = HexAction((action - EI(NonHexAction::count)) % EI(HexAction::_count));
        auto res = std::string{};
        std::shared_ptr<const Stack> stack = nullptr;
        std::string stackstr;

        if (ha == HexAction::SHOOT || ha == HexAction::MOVE) {
            stack = hex->stack;
        } else if (aMoveTargetHex) {
            stack = aMoveTargetHex->stack;
        }

        if (stack) {
            std::string targetcolor = "\033[31m";  // red
            if (color == "red") targetcolor = "\033[34m"; // blue
            stackstr = targetcolor + "#" + std::string(1, stack->getAlias()) + "\033[0m";
        } else {
            std::string targetcolor = "\033[7m";  // white
            stackstr = targetcolor + "#?" + "\033[0m";
        }

        switch (HexAction(ha)) {
        break; case HexAction::MOVE:
            res = (stack && hex->bhex == stack->cstack->getPosition() ? "Defend on " : "Move to ") + hex->name();
        break; case HexAction::AMOVE_TL:  res = "Attack " + stackstr + " from " + hex->name() + " /top-left/";
        break; case HexAction::AMOVE_TR:  res = "Attack " + stackstr + " from " + hex->name() + " /top-right/";
        break; case HexAction::AMOVE_R:   res = "Attack " + stackstr + " from " + hex->name() + " /right/";
        break; case HexAction::AMOVE_BR:  res = "Attack " + stackstr + " from " + hex->name() + " /bottom-right/";
        break; case HexAction::AMOVE_BL:  res = "Attack " + stackstr + " from " + hex->name() + " /bottom-left/";
        break; case HexAction::AMOVE_L:   res = "Attack " + stackstr + " from " + hex->name() + " /left/";
        break; case HexAction::SHOOT:     res = "Attack " + stackstr + " " + hex->name() + " (ranged)";
        break; default:
            THROW_FORMAT("Unexpected hexaction: %d", EI(ha));
        }

        return res;
    }

}

