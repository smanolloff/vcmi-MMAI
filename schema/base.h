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

#include <boost/core/demangle.hpp>
#include <boost/format.hpp>

#include <any>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

// Import + Export macro declarations
#ifdef VCMI_WINDOWS
#  ifdef VCMI_DLL_STATIC
#    define MMAI_IMPORT
#    define MMAI_EXPORT
#  elif defined(__GNUC__)
#    define MMAI_IMPORT __attribute__((dllimport))
#    define MMAI_EXPORT __attribute__((dllexport))
#  else
#    define MMAI_IMPORT __declspec(dllimport)
#    define MMAI_EXPORT __declspec(dllexport)
#  endif
#  define ELF_VISIBILITY
#else
#  ifdef __GNUC__
#    define MMAI_IMPORT	__attribute__ ((visibility("default")))
#    define MMAI_EXPORT __attribute__ ((visibility("default")))
#    define ELF_VISIBILITY __attribute__ ((visibility("default")))
#  endif
#endif

#ifdef MMAI_DLL
#  define MMAI_DLL_LINKAGE MMAI_EXPORT
#else
#  define MMAI_DLL_LINKAGE MMAI_IMPORT
#endif

#define MMAI_RESERVED_NAME_SUMMONER "MMAI_SCRIPT_SUMMONER"

namespace MMAI::Schema {
    #define EI(enum_value) static_cast<int>(enum_value)

    using Action = int;
    using BattlefieldState = std::vector<float>;
    using ActionMask = std::vector<bool>;
    using AttentionMask = std::vector<float>;

    // Same control actions for all versions
    constexpr Action ACTION_RETREAT = 0;
    constexpr Action ACTION_RESET = -1;
    constexpr Action ACTION_RENDER_ANSI = -2;

    class IState {
    public:
        virtual const ActionMask& getActionMask() const = 0;
        virtual const AttentionMask& getAttentionMask() const = 0;
        virtual const BattlefieldState& getBattlefieldState() const = 0;

        // Supplementary data may differ across versions => expose it as std::any
        // XXX: ensure the real data type has MMAI_DLL_LINKAGE to prevent std::any_cast errors
        virtual const std::any getSupplementaryData() const = 0;

        virtual int version() const = 0;
        virtual ~IState() = default;
    };

    // The CB functions above are bundled into Baggage struct
    // to be seamlessly transported through VCMI code as a std::any object,
    // then cast back to `Baggage` in the AI constructor.
    // Linkage needed due to ensure std::any_cast sees the proper symbol
    class IModel {
    public:
        virtual std::string getName() = 0;
        virtual int getVersion() = 0;
        virtual int getAction(const IState*) = 0;
        virtual double getValue(const IState*) = 0;

        virtual ~IModel() = default;
    };

    struct MMAI_DLL_LINKAGE Baggage {
        IModel* modelLeft;
        IModel* modelRight;

        // Models will be assigned based on player color
        // (used if training a single model on both sides)
        bool devMode = false;
    };

    inline std::string AnyCastError(const std::any any, const std::type_info &t) {
        if (!any.has_value()) {
            return "no value";
        } else if (any.type() != t) {
            return boost::str(
                boost::format("type mismatch: want: %s/%u, have: %s/%u") \
                % boost::core::demangle(t.name()) % t.hash_code() \
                % boost::core::demangle(any.type().name()) % any.type().hash_code()
            );
        } else {
            return "";
        }
    }
}
