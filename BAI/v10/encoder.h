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

#include "schema/base.h"
#include "schema/v10/types.h"

namespace MMAI::BAI::V10 {
    using GlobalAttribute = Schema::V10::GlobalAttribute;
    using PlayerAttribute = Schema::V10::PlayerAttribute;
    using HexAttribute = Schema::V10::HexAttribute;
    using LinkAttribute = Schema::V10::LinkAttribute;
    using BS = Schema::BattlefieldState;

    class Encoder {
    public:
        static void Encode(const LinkAttribute a, const int v, BS &vec);
        static void Encode(const HexAttribute a, const int v, BS &vec);
        static void Encode(const PlayerAttribute a, const int v, BS &vec);
        static void Encode(const GlobalAttribute a, const int v, BS &vec);
        static void Encode(const char* attrtype, const int a, const Schema::V10::Encoding e, const int n, int v, const int vmax, BS &vec);

        static void EncodeAccumulatingExplicitNull(const int v, const int n, BS &vec);
        static void EncodeAccumulatingImplicitNull(const int v, const int n, BS &vec);
        static void EncodeAccumulatingMaskingNull(const int v, const int n, BS &vec);
        static void EncodeAccumulatingStrictNull(const int v, const int n, BS &vec);
        static void EncodeAccumulatingZeroNull(const int v, const int n, BS &vec);

        static void EncodeBinaryExplicitNull(const int v, const int n, BS &vec);
        static void EncodeBinaryMaskingNull(const int v, const int n, BS &vec);
        static void EncodeBinaryStrictNull(const int v, const int n, BS &vec);
        static void EncodeBinaryZeroNull(const int v, const int n, BS &vec);

        static void EncodeCategoricalExplicitNull(const int v, const int n, BS &vec);
        static void EncodeCategoricalImplicitNull(const int v, const int n, BS &vec);
        static void EncodeCategoricalMaskingNull(const int v, const int n, BS &vec);
        static void EncodeCategoricalStrictNull(const int v, const int n, BS &vec);
        static void EncodeCategoricalZeroNull(const int v, const int n, BS &vec);

        static void EncodeExpnormExplicitNull(const int v, const int vmax, BS &vec);
        static void EncodeExpnormMaskingNull(const int v, const int vmax, BS &vec);
        static void EncodeExpnormStrictNull(const int v, const int vmax, BS &vec);
        static void EncodeExpnormZeroNull(const int v, const int vmax, BS &vec);

        static void EncodeLinnormExplicitNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormMaskingNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormStrictNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormZeroNull(const int v, const int vmax, BS &vec);

        static float CalcExpnorm(const int v, const int vmax);
        static float CalcLinnorm(const int v, const int vmax);
    private:
        static void EncodeAccumulating(const int v, const int n, BS &vec);
        static void EncodeBinary(const int v, const int n, BS &vec);
        static void EncodeCategorical(const int v, const int n, BS &vec);
        static void EncodeExpnorm(const int v, const int vmax, BS &vec);
        static void EncodeLinnorm(const int v, const int vmax, BS &vec);
    };
}
