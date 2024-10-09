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

#include "battle/CBattleInfoEssentials.h"
#include "battle/CPlayerBattleCallback.h"
#include "networkPacks/PacksForClientBattle.h"

#include "BAI/v13/action.h"
#include "BAI/v13/attack_log.h"
#include "BAI/v13/battlefield.h"
#include "BAI/v13/player_stats.h"
#include "BAI/v13/global_stats.h"
#include "BAI/v13/supplementary_data.h"
#include "schema/base.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13 {
    using BS = Schema::BattlefieldState;

    static const auto DUMMY_ATTNMASK = Schema::AttentionMask();

    class State : public Schema::IState {
    public:

        // IState impl
        const Schema::ActionMask* getActionMask() const override { return &actmask; };
        const Schema::AttentionMask* getAttentionMask() const override { return &DUMMY_ATTNMASK; }
        const Schema::BattlefieldState* getBattlefieldState() const override { return &bfstate; }
        const std::any getSupplementaryData() const override {
            return static_cast<const MMAI::Schema::V13::ISupplementaryData*>(supdata.get());
        }
        int version() const override { return version_; }

        State() = delete;
        State(const int version_, const std::string colorname, const CPlayerBattleCallback* battle_);

        void onActiveStack(const CStack* astack, CombatResult result = CombatResult::NONE, bool recording = false, bool fastpath = false);
        void onBattleStacksAttacked(const std::vector<BattleStackAttacked> &bsa);
        void onBattleTriggerEffect(const BattleTriggerEffect &bte);
        void onActionStarted(const BattleAction &action);
        void _onActionStarted(const BattleAction &action);
        void onActionFinished(const BattleAction &action);
        void onBattleEnd(const BattleResult *br);

        // Subsequent versions may override this if they only change
        // the data type of encoded values (i.e. have their own HEX_ENCODING)
        void encodeGlobal(CombatResult result);
        void encodePlayer(const PlayerStats* pstats);
        void encodeHex(const Hex* hex);
        void encodeLink(const std::shared_ptr<Link> link);
        void verify();

        const int version_;
        Schema::BattlefieldState bfstate = {};
        Schema::ActionMask actmask = {};
        std::unique_ptr<SupplementaryData> supdata = nullptr;
        std::vector<std::shared_ptr<AttackLog>> attackLogs = {};
        std::vector<std::shared_ptr<AttackLog>> persistentAttackLogs = {};
        std::vector<std::tuple<Schema::Action, std::shared_ptr<Schema::ActionMask>, std::shared_ptr<Schema::BattlefieldState>>> transitions = {};
        std::unique_ptr<Action> action = nullptr;
        std::unique_ptr<GlobalStats> gstats = nullptr;
        std::unique_ptr<PlayerStats> lpstats = nullptr;
        std::unique_ptr<PlayerStats> rpstats = nullptr;
        std::map<const CStack*, Stack::Stats> sstats;
        const std::pair<int, int> initialArmyValues;
        const std::string colorname;
        const CPlayerBattleCallback* const battle;
        const BattleSide side;
        std::shared_ptr<const Battlefield> battlefield;
        bool isMorale = false;

        int previousAction = -1;
        int startedAction = -1;
        const CStack* actingStack = nullptr;

        static std::vector<float> InitNullStack();
        const std::vector<float> nullstack;
    };
}
