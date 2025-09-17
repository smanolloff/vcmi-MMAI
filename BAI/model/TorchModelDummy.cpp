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

#include <memory>
#include <stdexcept>

#include "TorchModel.h"
#include "schema/base.h"
#include "schema/schema.h"

/*
 * Dummy implementation of TorchModel:
 * Used if MMAI_LIBTORCH_PATH is not set during compilation
 * (see notes in CMakeLists.txt)
 */

namespace MMAI::BAI {
    TorchModel::TorchModel(std::string path) {
        throw std::runtime_error(
            "MMAI was compiled without executorch support and cannot load \"MMAI_MODEL\" files."
        );
    }

    Schema::ModelType TorchModel::getType() { return MMAI::Schema::ModelType::TORCH; };
    std::string TorchModel::getName() { return ""; };
    int TorchModel::getVersion() { return 0; }
    Schema::Side TorchModel::getSide() { return Schema::Side(0); }
    int TorchModel::getAction(const MMAI::Schema::IState * s) { return 0; }
    double TorchModel::getValue(const MMAI::Schema::IState * s) { return 0; }
}
