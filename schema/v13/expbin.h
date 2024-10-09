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

namespace MMAI::Schema::V13 {
    /*
     * Compile-time calculation of the min value (inclusive) for bin index `i`.
     */
    constexpr int ExpBinValueMin(int i, int vmax, int n, double slope) {
        double x = static_cast<double>(i) / n;
        return gcem::ceil((gcem::exp(slope * x) - 1.0) / (gcem::exp(slope) - 1.0) * vmax);
    }

    /*
     * Compile-time calculation of the max value (inclusive) for bin index `i`.
     */
    constexpr int ExpBinValueMax(int i, int vmax, int n, double slope) {
        double x = static_cast<double>(i + 1) / n;
        return gcem::ceil((gcem::exp(slope * x) - 1.0) / (gcem::exp(slope) - 1.0) * vmax) - 1;
    }

    // // Test:
    // constexpr auto binmin = ExpBinValueMin(0, 1000, 10, 6.5);
    // constexpr auto binmax = ExpBinValueMax(0, 1000, 10, 6.5);

    /*
     * XXX: This function is for debugging purposes (avoid it at runtime).
     *
     * Compile-time calculation of the bin index given:
     * - a value `v` < `vmax`
     * - the total number of bins `n`.
     *
     *
     * Test:
     * constexpr auto lb = ExpBin(50, 1000, 20, 6.5);
     */
    constexpr int ExpBin(int v, int vmax, int n, double slope) {
        if (v <= 0) return 0;
        if (v >= vmax) return n - 1;

        double ratio = static_cast<double>(v) / vmax;
        double scaled = gcem::log1p(ratio * (gcem::exp(slope) - 1.0)) / slope;
        int index = static_cast<int>(scaled * n);
        return gcem::min(index, n - 1);
    }

    /*
     * Compile-time detection for "dead" bins when encoding integer values
     * using exp binning (see example below).
     *
     * Example 1 (dead bin at index 1):
     * vmax=80, n=6, slope=6.5
     *  bin#0 bounds:   min: ceil(0)=0,     max: ceil(-0.76)=0  => holds v=0
     *  bin#1 bounds:   min: ceil(0.24)=1,  max: ceil(-0.07)=0  => holds ?
     *  bin#2 bounds:   min: ceil(0.93)=1,  max: ceil(1.98)=2   => holds v=1, v=2
     *  bin#3 bounds:   min: ceil(2.98)=3,  max: ceil(8.06)=9   => holds v=3..9
     *  bin#4 bounds:   min: ceil(9.06)=10, max: ceil(25.9)=26  => holds v=10..26
     *  bin#5 bounds:   min: ceil(26.9)=27, max: ceil(79.0)=79  => holds v=27..79
     *
     * Example 2 (no dead bins):
     * vmax=80, n=5, slope=6.5
     *  bin#0 bounds:   min: ceil(0)=0,     max: ceil(-0.68)=0  => holds v=0
     *  bin#1 bounds:   min: ceil(0.32)=1,  max: ceil(0.50)=1   => holds v=1
     *  bin#2 bounds:   min: ceil(1.50)=2,  max: ceil(4.83)=5   => holds v=2..5
     *  bin#3 bounds:   min: ceil(5.83)=6,  max: ceil(20.7)=21  => holds v=6..21
     *  bin#4 bounds:   min: ceil(21.7)=22, max: ceil(79.0)=79  => holds v=22..79
     *
     * Test:
     * constexpr auto db = FindDeadExpBin(80, 6, 6.5);
     */
    constexpr int FindDeadExpBin(int vmax, int n, double slope) {
        int prevmax = ExpBinValueMax(0, vmax, n, slope);
        for (int i = 1; i < n; ++i) {
            // The current bin's min value *must* be greater
            int curmin = ExpBinValueMin(i, vmax, n, slope);
            int curmax = ExpBinValueMax(i, vmax, n, slope);
            if (curmin > curmax || curmin <= prevmax)
                return i;
            prevmax = curmax;
        }
        return -1; // Not found
    }

    /*
     * Compile-time check if the first expbin is dedicated for the value `0`.
     *
     * Example 1 (no dedicated zero bin):
     * vmax=80, n=10, slope=3
     *  bin 0 bounds:   min: ceil(0)=0,     max: ceil(0.46)=1   => holds v=0, v=1
     *  bin 1 bounds:   min: ceil(1.46)=2,  max: ceil(2.45)=3   => holds v=2, v=3
     *  ...
     *
     * Example 1 (dedicated zero bin):
     * vmax=80, n=15, slope=3
     *  bin 0 bounds:   min: ceil(0)=0,     max: ceil(-0.07)=0  => holds v=0
     *  bin 1 bounds:   min: ceil(0.93)=1,  max: ceil(1.06)=2   => holds v=1, v=2
     *  ...
     *
     * Test:
     * constexpr auto zb = HasDedicatedZeroBin(1000, 13, 6.5);
     */
    constexpr bool HasDedicatedZeroExpBin(int vmax, int n, double slope) {
        int binmin = ExpBinValueMin(0, vmax, n, slope);
        int binmax = ExpBinValueMax(0, vmax, n, slope);

        return (binmin == 0 && binmax == 0);
    }

    /*
     * Compile-time calculation for the max number of exp-bins for
     * a given `vmax` and `slope` without "dead" bins.
     *
     * Return values:
     * * Number of bins `n > 0`
     * * Error `-1` if the given `vmax` and `slope` prevent the solution
     * * Error `-2` if the given `nmax` is too low (solution would be sub-optimal)
     *
     * Test:
     * constexpr auto maxlb = MaxExpBins(1000, 6.5);
     */
    constexpr int MaxExpBins(int vmax, double slope, int nmax = 50) {
        int res = -1;
        for (int n = nmax - 1; n > 0; --n) {
            if (FindDeadExpBin(vmax, n, slope) == -1) {
                res = (n == nmax - 1) ? -2 : n;
                break;
            }
        }

        if (res <= 0) throw "No solution for the given `vmax` and `slope`";
        return res;
    }

    /*
     * Compile-time calculation for the min number of exp-bins for
     * a given `vmax` and `slope` with a dedicated "zero" bin.
     *
     * Possible return values:
     * * Number of bins `n > 0`
     * * Error `-1` if there is no solution for the given `vmax` and `slope`
     *
     * Test:
     * constexpr auto minlb = MinExpBinsWithDedicatedZero(1000, 6.5);
     */
    constexpr int MinExpBinsWithDedicatedZero(int vmax, double slope, int nmax = 50) {
        int res = -1;
        for (int n = 1; n < nmax; ++n) {
            if (HasDedicatedZeroExpBin(vmax, n, slope)) {
                res = (n == nmax - 1) ? -2 : n;
                break;
            }
        }

        if (res <= 0)
            throw "No solution for the given `vmax` and `slope`";
        return res;
    }

    /*
     * Bash snippet to quickly preview the bin boundaries
     * for a given (vmax, n, slope):
     *
     *      python -c '
     *      import numpy as np, math, sys;
     *      vmax, n, slope = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
     *      x = np.linspace(0, 1, n + 1)
     *      bins = (np.exp(slope * x) - 1) / (np.exp(slope) - 1) * vmax
     *      print([math.ceil(b) for b in bins])
     *       ' 1000 10 6.5
     */
}
