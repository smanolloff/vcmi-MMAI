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

#include <ATen/core/ivalue.h>
#include <ATen/ops/from_blob.h>
#include <torch/csrc/jit/mobile/module.h>

#include "TorchModel.h"
#include "schema/base.h"
#include "schema/schema.h"

namespace MMAI::BAI {
    TorchModel::TorchModel(std::string path)
    : path(path)
    , tjc(std::make_unique<TorchJitContainer>(path))
    {
        version = tjc->module.get_method("get_version")({}).toInt();

        switch(version) {
            break; case 1:
                sizeOneHex = MMAI::Schema::V1::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                nactions = MMAI::Schema::V1::N_ACTIONS;
            break; case 2:
                sizeOneHex = MMAI::Schema::V2::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                nactions = MMAI::Schema::V1::N_ACTIONS;
            break; case 3:
                sizeOneHex = MMAI::Schema::V3::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                nactions = MMAI::Schema::V3::N_ACTIONS;
            break; case 4:
                sizeOneHex = MMAI::Schema::V4::BATTLEFIELD_STATE_SIZE_ONE_HEX;
                nactions = MMAI::Schema::V4::N_ACTIONS;
            break; default:
                throw std::runtime_error("Unknown MMAI version: " + std::to_string(version));
        }

        // XXX: jit::mobile::Module has no toModule() attribute
        //      Maybe a call to .predict() with a dummy observation would work
        //
        // auto out_features = tjc->module.attr("actor", c10::IValue("method_not_found:actor")).toModule().attr("out_features").toInt();
        // switch(out_features) {
        // break; case 2311: actionOffset = 1;
        // break; case 2312: actionOffset = 0;
        // break; default:
        //     throw std::runtime_error("Expected 2311 or 2312 out_features for actor, got: " + std::to_string(out_features));
        // }
    }

    Schema::ModelType TorchModel::getType() {
        return Schema::ModelType::TORCH;
    };

    std::string TorchModel::getName() {
        return "MMAI_MODEL";
    };

    int TorchModel::getVersion() {
        return version;
    };

    int TorchModel::getAction(const MMAI::Schema::IState * s) {
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "1");
        auto any = s->getSupplementaryData();
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "2");
        auto ended = false;

        switch(version) {
            break; case 1:
                   case 2:
                ended = std::any_cast<const MMAI::Schema::V1::ISupplementaryData*>(any)->getIsBattleEnded();
            break; case 3:
                ended = std::any_cast<const MMAI::Schema::V3::ISupplementaryData*>(any)->getIsBattleEnded();
            break; case 4:
                ended = std::any_cast<const MMAI::Schema::V4::ISupplementaryData*>(any)->getIsBattleEnded();
            break; default:
                throw std::runtime_error("Unknown MMAI version: " + std::to_string(version));
        }

        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "3");
        if (ended)
            return MMAI::Schema::ACTION_RESET;

        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "4");
        auto &src = s->getBattlefieldState();
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "5");
        auto dst = MMAI::Schema::BattlefieldState{};
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "6");
        dst.resize(src.size());
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "7");
        std::copy(src.begin(), src.end(), dst.begin());
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "8");

        auto obs = at::from_blob(dst.data(), {static_cast<long long>(dst.size())}, at::kFloat);
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "9");

        // yields no performance benefit over (safer) copy approach:
        // auto obs = torch::from_blob(const_cast<float*>(s->getBattlefieldState().data()), {11, 15, sizeOneHex}, torch::kFloat);

        auto intmask = std::vector<int>{};
        intmask.reserve(nactions);
        auto &boolmask = s->getActionMask();
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "10");

        int i = 0;
        for (auto it = boolmask.begin() + actionOffset; it != boolmask.end(); ++it) {
            // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "boolmask i=%d", i);
            i++;
            intmask.push_back(static_cast<int>(*it));
        }
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "11");

        intmask.at(0) = 0;  // prevent retreats for now

        auto mask = at::from_blob(intmask.data(), {static_cast<long>(intmask.size())}, at::kInt).to(at::kBool);
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "12");

        // auto mask_accessor = mask.accessor<bool,1>();
        // for (int i = 0; i < mask_accessor.size(0); ++i)
        //     printf("mask[%d]=%d\n", i, mask_accessor[i]);

        std::unique_lock lock(m);
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "13");

        auto method = tjc->module.get_method("predict");
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "14");
        auto inputs = std::vector<c10::IValue>{obs, mask};
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "15");
        auto res = method(inputs).toInt() + actionOffset;
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "16");

        logAi->debug("AI action prediction: %d\n", int(res));

        // Also esitmate value
        auto vmethod = tjc->module.get_method("get_value");
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "17");
        auto vinputs = std::vector<c10::IValue>{obs};
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "18");
        auto vres = vmethod(vinputs).toDouble();
        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "19");
        logAi->debug("AI value estimation: %f\n", vres);

        // __android_log_print(ANDROID_LOG_ERROR, "TORCHTEST", "20");
        return MMAI::Schema::Action(res);
    };

    double TorchModel::getValue(const MMAI::Schema::IState * s) {
        auto &src = s->getBattlefieldState();
        auto dst = MMAI::Schema::BattlefieldState{};
        dst.reserve(dst.size());
        std::copy(src.begin(), src.end(), dst.begin());

        std::unique_lock lock(m);

        auto obs = at::from_blob(dst.data(), {11, 15, sizeOneHex}, at::kFloat);

        auto method = tjc->module.get_method("get_value");
        auto inputs = std::vector<c10::IValue>{obs};
        auto res = method(inputs).toDouble();
        return res;
    }
}
