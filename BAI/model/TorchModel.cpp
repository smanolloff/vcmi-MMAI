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

#include "TorchModel.h"
#include "schema/schema.h"

namespace MMAI::BAI {
    aten::Tensor TorchModel::call(
        const std::string& method_name,
        const std::vector<runtime::EValue>& input,
        int numel,
        aten::ScalarType st
    ) {
        auto res = model.execute(method_name, input);

        std::string common = "TorchModel: " + method_name;
        std::string want = "";
        std::string have = "";

        if (!res.ok()) {
            THROW_FORMAT("TorchModel: %s: error code %d", method_name % EI(res.error()));
        }

        auto out = res->at(0);

        if (!out.isTensor()) {
            THROW_FORMAT("TorchModel: %s: not a tensor", method_name % EI(res.error()));
        }

        auto t = out.toTensor();  // Most exports return scalars as 0-D tensors with dtype int64

        if (t.numel() != numel) {
            THROW_FORMAT("TorchModel: %s: bad numel: want: %d, have: %d", method_name % numel % EI(t.numel()));
        }

        if (t.dtype() != st) {
            THROW_FORMAT("TorchModel: %s: bad dtype: want: %d, have: %d", method_name % EI(st) % EI(t.dtype()));
        }

        return t;
    }

    TorchModel::TorchModel(std::string path)
    : path(path)
    , model(extension::Module(path))
    {
        version = 12;
        auto t = call("get_version", 1, aten::ScalarType::Long);
        version = static_cast<int>(t.const_data_ptr<int64_t>()[0]);
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
        auto any = s->getSupplementaryData();

        if (std::any_cast<const MMAI::Schema::V12::ISupplementaryData*>(any)->getIsBattleEnded())
            return MMAI::Schema::ACTION_RESET;

        auto src = s->getBattlefieldState();
        auto dst = std::vector<float>();
        dst.resize(src->size());
        std::copy(src->begin(), src->end(), dst.begin());
        auto input = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});
        auto output = call("predict", input, 1, aten::ScalarType::Long);

        int action = output.const_data_ptr<int64_t>()[0];
        std::cout << "Predicted action: " << action << "\n";
        return MMAI::Schema::Action(action);
    };

    double TorchModel::getValue(const MMAI::Schema::IState * s) {
        auto any = s->getSupplementaryData();

        if (std::any_cast<const MMAI::Schema::V12::ISupplementaryData*>(any)->getIsBattleEnded())
            return 0.0;

        auto src = s->getBattlefieldState();
        auto dst = std::vector<float>();
        dst.resize(src->size());
        std::copy(src->begin(), src->end(), dst.begin());
        auto input = executorch::extension::from_blob(dst.data(), {static_cast<int>(dst.size())});
        auto output = call("get_value", input, 1, aten::ScalarType::Float);

        auto value = output.const_data_ptr<float>()[0];
        std::cout << "Predicted value: " << value << "\n";
        return value;
    }
}
