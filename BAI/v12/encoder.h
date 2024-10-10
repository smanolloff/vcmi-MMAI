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
#include "schema/v12/types.h"

namespace MMAI::BAI::V12 {
    using GlobalAttribute = Schema::V12::GlobalAttribute;
    using PlayerAttribute = Schema::V12::PlayerAttribute;
    using HexAttribute = Schema::V12::HexAttribute;
    using BS = Schema::BattlefieldState;

    class Encoder {
    public:
        static void Encode(const HexAttribute a, const int v, BS &vec);
        static void Encode(const PlayerAttribute a, const int v, BS &vec);
        static void Encode(const GlobalAttribute a, const int v, BS &vec);
        static void Encode(
            const char* attrtype,
            const int a,
            const Schema::V12::Encoding e,
            const int n,
            const int vmax,
            const double p,
            int v,
            BS &vec
        );

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

        static void EncodeExpbinExplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeExpbinImplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeExpbinMaskingNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeExpbinStrictNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeExpbinZeroNull(const int v, const int n, const int vmax, double slope, BS &vec);

        static void EncodeAccumulatingExpbinExplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingExpbinImplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingExpbinMaskingNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingExpbinStrictNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingExpbinZeroNull(const int v, const int n, const int vmax, double slope, BS &vec);

        static void EncodeLinbinExplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeLinbinImplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeLinbinMaskingNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeLinbinStrictNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeLinbinZeroNull(const int v, const int n, const int vmax, double slope, BS &vec);

        static void EncodeAccumulatingLinbinExplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingLinbinImplicitNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingLinbinMaskingNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingLinbinStrictNull(const int v, const int n, const int vmax, double slope, BS &vec);
        static void EncodeAccumulatingLinbinZeroNull(const int v, const int n, const int vmax, double slope, BS &vec);

        static void EncodeExpnormExplicitNull(const int v, const int vmax, double slope, BS &vec);
        static void EncodeExpnormMaskingNull(const int v, const int vmax, double slope, BS &vec);
        static void EncodeExpnormStrictNull(const int v, const int vmax, double slope, BS &vec);
        static void EncodeExpnormZeroNull(const int v, const int vmax, double slope, BS &vec);

        static void EncodeLinnormExplicitNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormMaskingNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormStrictNull(const int v, const int vmax, BS &vec);
        static void EncodeLinnormZeroNull(const int v, const int vmax, BS &vec);

        static float CalcExpnorm(const int v, const int vmax, double slope);
        static float CalcLinnorm(const int v, const int vmax);
    private:
        static void EncodeAccumulating(const int v, const int n, BS &vec);
        static void EncodeBinary(const int v, const int n, BS &vec);
        static void EncodeCategorical(const int v, const int n, BS &vec);
        static void EncodeExpbin(const int v, const int n, const int vmax, const double slope, BS &vec);
        static void EncodeAccumulatingExpbin(const int v, const int n, const int vmax, const double slope, BS &vec);
        static void EncodeLinbin(const int v, const int n, const int vmax, const double slope, BS &vec);
        static void EncodeAccumulatingLinbin(const int v, const int n, const int vmax, const double slope, BS &vec);
        static void EncodeExpnorm(const int v, const int vmax, double slope, BS &vec);
        static void EncodeLinnorm(const int v, const int vmax, BS &vec);
    };
}
