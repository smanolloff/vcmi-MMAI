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

#include "schema/gcem/include/gcem.hpp"

namespace MMAI::Schema::V12 {
    /*
     * Compile-time calculation of the min value (inclusive) for bin index `i`.
     */
    constexpr int LinBinValueMin(int i, int vmax, int n) {
        double slope = static_cast<double>(vmax) / n;
        return gcem::ceil(i * slope);
    }

    /*
     * Compile-time calculation of the max value (inclusive) for bin index `i`.
     */
    constexpr int LinBinValueMax(int i, int vmax, int n) {
        double slope = static_cast<double>(vmax) / n;
        return gcem::ceil((i + 1) * slope) - 1;
    }

    constexpr auto binmin = LinBinValueMin(1, 100, 3);
    constexpr auto binmax = LinBinValueMax(1, 100, 3);

    /*
     * XXX: This function is for debugging purposes (avoid it at runtime).
     *
     * Compile-time calculation of the bin index given:
     * - a value `v` < `vmax`
     * - the total number of bins `n`.
     *
     *
     * Test:
     * constexpr auto lb = LinBin(5000, 1000, 20);
     */
    constexpr int LinBin(int v, int vmax, int n) {
        if (v <= 0) return 0;
        if (v >= vmax) return n - 1;
        double d = static_cast<double>(vmax) / n;
        int index = static_cast<int>(v / d);
        return index;
    }

    // Redundant (would return vmax < n)
    // constexpr int FindDeadLinBin(...)

    // Redundant (would return vmax <= n)
    // constexpr int HasDedicatedZeroLinBin(...)

    // Redundant (would return vmax)
    // constexpr int MaxExpBins(...)

    // Redundant (would return vmax)
    // constexpr int MinExpBinsWithDedicatedZero(...)

    /*
     * Bash snippet to quickly preview the bin boundaries
     * for a given (vmax, n):
     *
     *      python -c '
     *      import sys, math, numpy as np;
     *      vmax, n = int(sys.argv[1]), int(sys.argv[2])
     *      print([math.ceil(x) for x in np.linspace(0, vmax, n+1)])
     *      ' 100 3
     */
}
