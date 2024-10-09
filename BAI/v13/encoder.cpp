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


#include "schema/v13/constants.h"
#include "schema/v13/types.h"

#include "BAI/v13/encoder.h"
#include "common.h"

namespace MMAI::BAI::V13 {
    using namespace Schema::V13;
    using BS = Schema::BattlefieldState;
    using clock = std::chrono::system_clock;

    static std::map<std::string, std::map<int, clock::time_point>> warns;

    #define COERCE(v, vfallback) (v == NULL_VALUE_UNENCODED) ? vfallback : v

    #define ADD_ZEROS_AND_RETURN(n, vec) \
            vec.insert(vec.end(), n, 0); \
            return;

    #define MAYBE_ADD_ZEROS_AND_RETURN(v, n, vec) \
        if (v <= 0) { ADD_ZEROS_AND_RETURN(n, vec) }

    #define MAYBE_ADD_MASKED_AND_RETURN(v, n, vec) \
        if (v == NULL_VALUE_UNENCODED) { \
            vec.insert(vec.end(), n, NULL_VALUE_ENCODED); \
            return; \
        }

    #define MAYBE_THROW_STRICT_ERROR(v) \
        if (v == NULL_VALUE_UNENCODED) { \
            throw std::runtime_error("NULL values are not allowed for strict encoding"); \
        }

    void Encoder::Encode(
        const char* attrname,
        const int a,
        const Encoding e,
        const int n,
        const int vmax,
        const double p,
        int v,
        BS &vec
    ) {
        if (e == Encoding::RAW) {
            vec.push_back(v);
            return;
        }

        if (v > vmax) {
            // THROW_FORMAT("Cannot encode value: %d (vmax=%d, a=%d, n=%d, e=%d)", v % vmax % EI(a) % n % EI(e));
            // Can happen (e.g. DMG_*_ACC_REL0 > 1 if there were resurrected stacks)

            // Warn at most once every 600s
            auto now = clock::now();
            auto warned_at = warns[attrname][EI(a)];

            // auto warned_at_ctime = clock::to_time_t(warned_at);
            // std::cout << "Warned at: " << std::ctime(&warned_at_ctime) << "\n";
            if (std::chrono::duration_cast<std::chrono::seconds>(now - warned_at) > std::chrono::seconds(600)) {
                printf("WARNING: v=%d (vmax=%d, a=%d, e=%d, n=%d, attrname=%s)\n", v, vmax, EI(a), EI(e), n, attrname);
                warns[attrname][EI(a)] = now;
            }
            v = vmax;
        }

        switch (e) {
        break; case Encoding::BINARY_EXPLICIT_NULL: EncodeBinaryExplicitNull(v, n, vec);
        break; case Encoding::BINARY_MASKING_NULL: EncodeBinaryMaskingNull(v, n, vec);
        break; case Encoding::BINARY_STRICT_NULL: EncodeBinaryStrictNull(v, n, vec);
        break; case Encoding::BINARY_ZERO_NULL: EncodeBinaryZeroNull(v, n, vec);
        break; case Encoding::EXPNORM_EXPLICIT_NULL: EncodeExpnormExplicitNull(v, vmax, p, vec);
        break; case Encoding::EXPNORM_MASKING_NULL: EncodeExpnormMaskingNull(v, vmax, p, vec);
        break; case Encoding::EXPNORM_STRICT_NULL: EncodeExpnormStrictNull(v, vmax, p, vec);
        break; case Encoding::EXPNORM_ZERO_NULL: EncodeExpnormZeroNull(v, vmax, p, vec);
        break; case Encoding::LINNORM_EXPLICIT_NULL: EncodeLinnormExplicitNull(v, vmax, vec);
        break; case Encoding::LINNORM_MASKING_NULL: EncodeLinnormMaskingNull(v, vmax, vec);
        break; case Encoding::LINNORM_STRICT_NULL: EncodeLinnormStrictNull(v, vmax, vec);
        break; case Encoding::LINNORM_ZERO_NULL: EncodeLinnormZeroNull(v, vmax, vec);
        break; case Encoding::CATEGORICAL_EXPLICIT_NULL: EncodeCategoricalExplicitNull(v, n, vec);
        break; case Encoding::CATEGORICAL_IMPLICIT_NULL: EncodeCategoricalImplicitNull(v, n, vec);
        break; case Encoding::CATEGORICAL_MASKING_NULL: EncodeCategoricalMaskingNull(v, n, vec);
        break; case Encoding::CATEGORICAL_STRICT_NULL: EncodeCategoricalStrictNull(v, n, vec);
        break; case Encoding::CATEGORICAL_ZERO_NULL: EncodeCategoricalZeroNull(v, n, vec);
        break; case Encoding::EXPBIN_EXPLICIT_NULL: EncodeExpbinExplicitNull(v, n, vmax, p, vec);
        break; case Encoding::EXPBIN_IMPLICIT_NULL: EncodeExpbinImplicitNull(v, n, vmax, p, vec);
        break; case Encoding::EXPBIN_MASKING_NULL: EncodeExpbinMaskingNull(v, n, vmax, p, vec);
        break; case Encoding::EXPBIN_STRICT_NULL: EncodeExpbinStrictNull(v, n, vmax, p, vec);
        break; case Encoding::EXPBIN_ZERO_NULL: EncodeExpbinZeroNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPBIN_EXPLICIT_NULL: EncodeAccumulatingExpbinExplicitNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPBIN_IMPLICIT_NULL: EncodeAccumulatingExpbinImplicitNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPBIN_MASKING_NULL: EncodeAccumulatingExpbinMaskingNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPBIN_STRICT_NULL: EncodeAccumulatingExpbinStrictNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPBIN_ZERO_NULL: EncodeAccumulatingExpbinZeroNull(v, n, vmax, p, vec);
        break; case Encoding::LINBIN_EXPLICIT_NULL: EncodeLinbinExplicitNull(v, n, vmax, p, vec);
        break; case Encoding::LINBIN_IMPLICIT_NULL: EncodeLinbinImplicitNull(v, n, vmax, p, vec);
        break; case Encoding::LINBIN_MASKING_NULL: EncodeLinbinMaskingNull(v, n, vmax, p, vec);
        break; case Encoding::LINBIN_STRICT_NULL: EncodeLinbinStrictNull(v, n, vmax, p, vec);
        break; case Encoding::LINBIN_ZERO_NULL: EncodeLinbinZeroNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_LINBIN_EXPLICIT_NULL: EncodeAccumulatingLinbinExplicitNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_LINBIN_IMPLICIT_NULL: EncodeAccumulatingLinbinImplicitNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_LINBIN_MASKING_NULL: EncodeAccumulatingLinbinMaskingNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_LINBIN_STRICT_NULL: EncodeAccumulatingLinbinStrictNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_LINBIN_ZERO_NULL: EncodeAccumulatingLinbinZeroNull(v, n, vmax, p, vec);
        break; case Encoding::ACCUMULATING_EXPLICIT_NULL: EncodeAccumulatingExplicitNull(v, n, vec);
        break; case Encoding::ACCUMULATING_IMPLICIT_NULL: EncodeAccumulatingImplicitNull(v, n, vec);
        break; case Encoding::ACCUMULATING_MASKING_NULL: EncodeAccumulatingMaskingNull(v, n, vec);
        break; case Encoding::ACCUMULATING_STRICT_NULL: EncodeAccumulatingStrictNull(v, n, vec);
        break; case Encoding::ACCUMULATING_ZERO_NULL: EncodeAccumulatingZeroNull(v, n, vec);
        break; default:
            THROW_FORMAT("Unexpected Encoding: %d", EI(e));
        }
    }

    void Encoder::Encode(const HexAttribute a, const int v, BS &vec) {
        auto &[_, e, n, vmax, p] = HEX_ENCODING.at(EI(a));
        Encode("HexAttribute", EI(a), e, n, vmax, p, v, vec);
    }

    void Encoder::Encode(const PlayerAttribute a, const int v, BS &vec) {
        auto &[_, e, n, vmax, p] = PLAYER_ENCODING.at(EI(a));
        Encode("PlayerAttribute", EI(a), e, n, vmax, p, v, vec);
    }

    void Encoder::Encode(const GlobalAttribute a, const int v, BS &vec) {
        auto &[_, e, n, vmax, p] = GLOBAL_ENCODING.at(EI(a));
        Encode("GlobalAttribute", EI(a), e, n, vmax, p, v, vec);
    }

    //
    // ACCUMULATING
    //
    void Encoder::EncodeAccumulatingExplicitNull(const int v, const int n, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        };
        vec.push_back(0);
        EncodeAccumulating(v, n-1, vec);
    }

    void Encoder::EncodeAccumulatingImplicitNull(const int v, const int n, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }
        EncodeAccumulating(v, n, vec);
    }

    void Encoder::EncodeAccumulatingMaskingNull(const int v, const int n, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeAccumulating(v, n, vec);
    }

    void Encoder::EncodeAccumulatingStrictNull(const int v, const int n, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeAccumulating(v, n, vec);
    }

    void Encoder::EncodeAccumulatingZeroNull(const int v, const int n, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        EncodeAccumulating(v, n, vec);
    }

    void Encoder::EncodeAccumulating(const int v, const int n, BS &vec) {
        vec.insert(vec.end(), v+1, 1);
        vec.insert(vec.end(), n-v-1, 0);
    }

    //
    // BINARY
    //

    void Encoder::EncodeBinaryExplicitNull(const int v, const int n, BS &vec) {
        vec.push_back(v == NULL_VALUE_UNENCODED);
        EncodeBinary(v, n-1, vec);
    }

    void Encoder::EncodeBinaryMaskingNull(const int v, const int n, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeBinary(v, n, vec);
    }

    void Encoder::EncodeBinaryStrictNull(const int v, const int n, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeBinary(v, n, vec);
    }

    void Encoder::EncodeBinaryZeroNull(const int v, const int n, BS &vec) {
        EncodeBinary(v, n, vec);
    }

    void Encoder::EncodeBinary(const int v, const int n, BS &vec) {
        MAYBE_ADD_ZEROS_AND_RETURN(v, n, vec);

        int vtmp = v;
        for (int i=0; i < n; ++i) {
            vec.push_back(vtmp % 2);
            vtmp /= 2;
        }
    }

    //
    // CATEGORICAL
    //

    void Encoder::EncodeCategoricalExplicitNull(const int v, const int n, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(v == NULL_VALUE_UNENCODED);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        vec.push_back(0);
        EncodeCategorical(v, n-1, vec);
    }

    void Encoder::EncodeCategoricalImplicitNull(const int v, const int n, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }

        EncodeCategorical(v, n, vec);
    }

    void Encoder::EncodeCategoricalMaskingNull(const int v, const int n, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeCategorical(v, n, vec);
    }

    void Encoder::EncodeCategoricalStrictNull(const int v, const int n, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeCategorical(v, n, vec);
    }

    void Encoder::EncodeCategoricalZeroNull(const int v, const int n, BS &vec) {
        EncodeCategorical(v, n, vec);
    }

    void Encoder::EncodeCategorical(const int v, const int n, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }

        for (int i=0; i < n; ++i) {
            if (i == v) {
                vec.push_back(1);
                ADD_ZEROS_AND_RETURN(n-i-1, vec);
            } else {
                vec.push_back(0);
            }
        }
    }

    //
    // EXPBIN
    //

    void Encoder::EncodeExpbinExplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(v == NULL_VALUE_UNENCODED);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        vec.push_back(0);
        EncodeExpbin(v, n-1, vmax, slope, vec);
    }

    void Encoder::EncodeExpbinImplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }

        EncodeExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeExpbinMaskingNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeExpbinStrictNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeExpbinZeroNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        EncodeExpbin(v, n, vmax, slope, vec);
    }


    /*
    BASH code snipped for printing the bins. Args:
    vmax n slope

python -c '
import numpy as np, math, sys;
vmax, n, slope = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
x = np.linspace(0, 1, n + 1)
bins = (np.exp(slope * x) - 1) / (np.exp(slope) - 1) * vmax
print([math.ceil(b) for b in bins])
 ' 1500 30 6.5

    See also CalcExpnorm() for visualisation example.
    */

    void Encoder::EncodeExpbin(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }

        double ratio = static_cast<double>(v) / vmax;
        double scaled = std::log1p(ratio * (std::exp(slope) - 1.0)) / slope;
        int index = std::min(static_cast<int>(scaled * n), n - 1);

        for (int i=0; i < n; ++i) {
            if (i == index) {
                vec.push_back(1);
                ADD_ZEROS_AND_RETURN(n-i-1, vec);
            } else {
                vec.push_back(0);
            }
        }
    }

    void Encoder::EncodeAccumulatingExpbinExplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(v == NULL_VALUE_UNENCODED);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        vec.push_back(0);
        EncodeAccumulatingExpbin(v, n-1, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingExpbinImplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }

        EncodeAccumulatingExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingExpbinMaskingNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeAccumulatingExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingExpbinStrictNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeAccumulatingExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingExpbinZeroNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        EncodeAccumulatingExpbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingExpbin(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }

        double ratio = static_cast<double>(v) / vmax;
        double scaled = std::log1p(ratio * (std::exp(slope) - 1.0)) / slope;
        int index = static_cast<int>(scaled * n);

        for (int i=0; i < n; ++i) {
            if (i == index) {
                vec.push_back(1);
                ADD_ZEROS_AND_RETURN(n-i-1, vec);
            } else {
                vec.push_back(1);
            }
        }
    }

    //
    // LINBIN
    //

    void Encoder::EncodeLinbinExplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(v == NULL_VALUE_UNENCODED);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        vec.push_back(0);
        EncodeLinbin(v, n-1, vmax, slope, vec);
    }

    void Encoder::EncodeLinbinImplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }

        EncodeLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeLinbinMaskingNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeLinbinStrictNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeLinbinZeroNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        EncodeLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeLinbin(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }

        int index = std::min(static_cast<int>(v / slope), n - 1);

        for (int i=0; i < n; ++i) {
            if (i == index) {
                vec.push_back(1);
                ADD_ZEROS_AND_RETURN(n-i-1, vec);
            } else {
                vec.push_back(0);
            }
        }
    }

    void Encoder::EncodeAccumulatingLinbinExplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(v == NULL_VALUE_UNENCODED);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }
        vec.push_back(0);
        EncodeAccumulatingLinbin(v, n-1, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingLinbinImplicitNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            ADD_ZEROS_AND_RETURN(n, vec);
        }

        EncodeAccumulatingLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingLinbinMaskingNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_ADD_MASKED_AND_RETURN(v, n, vec);
        EncodeAccumulatingLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingLinbinStrictNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeAccumulatingLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingLinbinZeroNull(const int v, const int n, const int vmax, const double slope, BS &vec) {
        EncodeAccumulatingLinbin(v, n, vmax, slope, vec);
    }

    void Encoder::EncodeAccumulatingLinbin(const int v, const int n, const int vmax, const double slope, BS &vec) {
        if (v <= 0) {
            vec.push_back(1);
            ADD_ZEROS_AND_RETURN(n-1, vec);
        }

        int index = static_cast<int>(v / slope);

        for (int i=0; i < n; ++i) {
            if (i == index) {
                vec.push_back(1);
                ADD_ZEROS_AND_RETURN(n-i-1, vec);
            } else {
                vec.push_back(1);
            }
        }
    }

    //
    // EXPNORM
    //

    void Encoder::EncodeExpnormExplicitNull(const int v, const int vmax, double slope, BS &vec) {
        vec.push_back(v == NULL_VALUE_UNENCODED);
        EncodeExpnorm(v, vmax, slope, vec);
    }

    void Encoder::EncodeExpnormMaskingNull(const int v, const int vmax, double slope, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(NULL_VALUE_ENCODED);
            return;
        }
        EncodeExpnorm(v, vmax, slope, vec);
    }

    void Encoder::EncodeExpnormStrictNull(const int v, const int vmax, double slope, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeExpnorm(v, vmax, slope, vec);
    }

    void Encoder::EncodeExpnormZeroNull(const int v, const int vmax, double slope, BS &vec) {
        EncodeExpnorm(v, vmax, slope, vec);
    }

    void Encoder::EncodeExpnorm(const int v, const int vmax, double slope, BS &vec) {
        if (v <= 0) {
            vec.push_back(0);
            return;
        }

        vec.push_back(CalcExpnorm(v, vmax, slope));
    }

    // Visualise on https://www.desmos.com/calculator:
    // ln(1 + (x/M) * (exp(S)-1))/S
    // Add slider "S" (slope) and "M" (vmax).
    // Play with the sliders to see the nonlinearity (use M=1 for best view)
    // XXX: slope cannot be 0
    float Encoder::CalcExpnorm(const int v, const int vmax, const double slope) {
        double ratio = static_cast<double>(v) / vmax;
        return std::log1p(ratio * (std::exp(slope) - 1.0)) / (slope + 1e-6);
    }

    //
    // LINNORM
    //

    void Encoder::EncodeLinnormExplicitNull(const int v, const int vmax, BS &vec) {
        vec.push_back(v == NULL_VALUE_UNENCODED);
        EncodeLinnorm(v, vmax, vec);
    }

    void Encoder::EncodeLinnormMaskingNull(const int v, const int vmax, BS &vec) {
        if (v == NULL_VALUE_UNENCODED) {
            vec.push_back(NULL_VALUE_ENCODED);
            return;
        }
        EncodeLinnorm(v, vmax, vec);
    }

    void Encoder::EncodeLinnormStrictNull(const int v, const int vmax, BS &vec) {
        MAYBE_THROW_STRICT_ERROR(v);
        EncodeLinnorm(v, vmax, vec);
    }

    void Encoder::EncodeLinnormZeroNull(const int v, const int vmax, BS &vec) {
        EncodeLinnorm(v, vmax, vec);
    }

    void Encoder::EncodeLinnorm(const int v, const int vmax, BS &vec) {
        if (v <= 0) {
            vec.push_back(0);
            return;
        }

        // XXX: this is a simplified version for 0..1 norm
        vec.push_back(CalcLinnorm(v, vmax));
    }

    float Encoder::CalcLinnorm(const int v, const int vmax) {
        return static_cast<float>(v) / static_cast<float>(vmax);
    }
}
