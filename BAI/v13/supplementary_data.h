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

#include "BAI/v13/attack_log.h"
#include "BAI/v13/battlefield.h"
#include "BAI/v13/player_stats.h"
#include "BAI/v13/global_stats.h"
// #include "BAI/v13/util.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13 {
    // match sides for convenience when determining winner (see `victory`)
    static_assert(EI(CombatResult::LEFT_WINS) == EI(Side::LEFT));
    static_assert(EI(CombatResult::RIGHT_WINS) == EI(Side::RIGHT));

    class SupplementaryData : public Schema::V13::ISupplementaryData {
    public:
        SupplementaryData() = delete;

        // Called on activeStack (complete battlefield info)
        SupplementaryData(
            std::string colorname_,
            Side side_,
            const GlobalStats* gstats_,
            const PlayerStats* lpstats_,
            const PlayerStats* rpstats_,
            const Battlefield* battlefield_,
            const std::vector<std::shared_ptr<AttackLog>> attackLogs_,
            std::vector<std::tuple<Schema::Action, std::shared_ptr<Schema::ActionMask>, std::shared_ptr<Schema::BattlefieldState>>> transitions_,
            CombatResult result
        ) : colorname(colorname_),
            side(side_),
            gstats(gstats_),
            lpstats(lpstats_),
            rpstats(rpstats_),
            battlefield(battlefield_),
            attackLogs(attackLogs_),
            transitions(transitions_),
            ended(result != CombatResult::NONE),
            victory(EI(result) == EI(side))
        {};

        // impl ISupplementaryData
        Type getType() const override { return type; };
        Side getSide() const override { return side; };
        std::string getColor() const override { return colorname; };
        ErrorCode getErrorCode() const override { return errcode; }; // TODO

        bool getIsBattleEnded() const override { return ended; };
        bool getIsVictorious() const override { return victory; };

        const Schema::V13::Stacks getStacks() const override;
        const Schema::V13::Hexes getHexes() const override;
        const Schema::V13::AllLinks getAllLinks() const override;
        const Schema::V13::AttackLogs getAttackLogs() const override;
        const Schema::V13::StateTransitions getStateTransitions() const override;
        const Schema::V13::IGlobalStats* getGlobalStats() const override { return gstats; }
        const Schema::V13::IPlayerStats* getLeftPlayerStats() const override { return lpstats; }
        const Schema::V13::IPlayerStats* getRightPlayerStats() const override { return rpstats; }
        const std::string getAnsiRender() const override { return ansiRender; }

        const std::string colorname;
        const Side side;
        const Battlefield* const battlefield;
        const GlobalStats* const gstats;
        const PlayerStats* const lpstats;
        const PlayerStats* const rpstats;
        const std::vector<std::shared_ptr<AttackLog>> attackLogs;
        const bool ended = false;
        const bool victory = false;
        const std::vector<std::tuple<Schema::Action, std::shared_ptr<Schema::ActionMask>, std::shared_ptr<Schema::BattlefieldState>>> transitions;

        // Optionally modified (during activeStack if action was invalid)
        ErrorCode errcode = ErrorCode::OK;

        // Optionally modified (during activeStack if action was RENDER)
        Type type = Type::REGULAR;
        std::string ansiRender = "";
    };
}
