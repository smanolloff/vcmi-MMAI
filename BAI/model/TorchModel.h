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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <executorch/extension/data_loader/file_data_loader.h>
#include <executorch/extension/memory_allocator/malloc_memory_allocator.h>
#include <executorch/extension/tensor/tensor.h>
#include <executorch/runtime/core/evalue.h>
#include <executorch/runtime/core/portable_type/scalar_type.h>
#include <executorch/runtime/executor/program.h>

#include "schema/v13/types.h"
#include "schema/base.h"

namespace MMAI::BAI {
namespace et_ext = executorch::extension;
namespace et_run = executorch::runtime;
using EValue = executorch::runtime::EValue;
using Tensor = executorch::runtime::etensor::Tensor;
using ScalarType = executorch::runtime::etensor::ScalarType;

class TorchModel : public MMAI::Schema::IModel {
public:
    explicit TorchModel(std::string &path);

    Schema::ModelType getType() override;
    std::string getName() override;
    int getVersion() override;
    Schema::Side getSide() override;
    int getAction(const MMAI::Schema::IState * s) override;
    double getValue(const MMAI::Schema::IState * s) override;
private:
    std::string path;
    std::string name;
    int version;
    Schema::Side side;

    // https://github.com/pytorch/executorch/blob/v0.7.0/docs/source/running-a-model-cpp-tutorial.md

    std::unique_ptr<et_ext::FileDataLoader> loader;
    std::unique_ptr<et_ext::MallocMemoryAllocator> memory_allocator;
    std::unique_ptr<et_ext::MallocMemoryAllocator> temp_allocator;
    std::unique_ptr<executorch::runtime::Program> program;

    struct MethodHolder {
        std::vector<std::vector<uint8_t>> planned_buffers;
        std::vector<et_run::Span<uint8_t>> planned_spans;
        std::unique_ptr<et_run::HierarchicalAllocator> planned_memory;
        std::unique_ptr<et_run::MemoryManager> memory_manager;
        std::unique_ptr<et_run::Method> method;
        std::vector<EValue> inputs;
    };

    std::unordered_map<std::string, MethodHolder> methods;

    void maybeLoadMethod(const std::string& method_name);

    // 3D tensor as a vector
    std::vector<std::vector<std::vector<int64_t>>> all_sizes;

    std::pair<std::vector<executorch::extension::TensorPtr>, int> prepareInputsV13(
        const MMAI::Schema::IState * state,
        const MMAI::Schema::V13::ISupplementaryData* sup,
        int bucket = -1
    );

    Tensor call(
        const std::string& method_name,
        const std::vector<EValue>& input,
        int numel,
        ScalarType st = ScalarType(-1)  // magic for "don't check"
    );

    Tensor call(const std::string &method_name, const EValue& ev, int numel, ScalarType st) {
        return call(method_name, std::vector<EValue>{ev}, numel, st);
    }

    Tensor call(const std::string &method_name, int numel, ScalarType st) {
        return call(method_name, std::vector<EValue>{}, numel, st);
    }
};

} // namespace MMAI::BAI
