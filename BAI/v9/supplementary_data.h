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

#include "BAI/v9/attack_log.h"
#include "BAI/v9/battlefield.h"
#include "BAI/v9/global_stats.h"
// #include "BAI/v9/util.h"
#include "schema/v9/types.h"

namespace MMAI::BAI::V9 {
    // match sides for convenience when determining winner (see `victory`)
    static_assert(EI(CombatResult::LEFT_WINS) == EI(Side::LEFT));
    static_assert(EI(CombatResult::RIGHT_WINS) == EI(Side::RIGHT));

    class SupplementaryData : public Schema::V9::ISupplementaryData {
    public:
        SupplementaryData() = delete;

        // Called on activeStack (complete battlefield info)
        SupplementaryData(
            std::string colorname_,
            Side side_,
            const GlobalStats* lgstats_,
            const GlobalStats* rgstats_,
            const Battlefield* battlefield_,
            std::vector<std::shared_ptr<AttackLog>> attackLogs_,
            CombatResult result
        ) : colorname(colorname_),
            side(side_),
            lgstats(lgstats_),
            rgstats(rgstats_),
            battlefield(battlefield_),
            attackLogs(attackLogs_),
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

        const Schema::V9::Stacks getStacks() const override;
        const Schema::V9::Hexes getHexes() const override;
        const Schema::V9::Links getLinks() const override;
        const Schema::V9::AttackLogs getAttackLogs() const override;
        const Schema::V9::IGlobalStats* getGlobalStatsLeft() const override { return lgstats; }
        const Schema::V9::IGlobalStats* getGlobalStatsRight() const override { return rgstats; }
        const std::string getAnsiRender() const override { return ansiRender; }

        const std::string colorname;
        const Side side;
        const Battlefield* const battlefield;
        const GlobalStats* const lgstats;
        const GlobalStats* const rgstats;
        const std::vector<std::shared_ptr<AttackLog>> attackLogs;
        const bool ended = false;
        const bool victory = false;

        // Optionally modified (during activeStack if action was invalid)
        ErrorCode errcode = ErrorCode::OK;

        // Optionally modified (during activeStack if action was RENDER)
        Type type = Type::REGULAR;
        std::string ansiRender = "";
    };
}
