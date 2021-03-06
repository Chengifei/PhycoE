/* Copyright 2017-2018 by Yifei Zheng
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This header defines a few commonly used routines in many math headers.
 * NOTE: this header shall NOT include other /math headers.
 * The guarantee is made so that all the functions defined is basic and common
 * enough for other math functions.
 */

#ifndef MATH_COMMON_HPP
#define MATH_COMMON_HPP
#include <cmath>
#include <limits>
#include <algorithm>
#include "../env.hpp"
#ifdef CSF_AVX
#include <immintrin.h>
#endif

struct zu_wrapper {
#ifdef CSF_NEED_ZERO_UPPER
    ~zu_wrapper() {
        _mm256_zeroupper();
    }
#endif
};

inline double _raw_sqrt(double d) {
    zu_wrapper _Z;
    double ret;
    __asm__("vsqrtsd\t%0, %1, %1":"=x"(ret):"x"(d));
    return ret;
}

template <typename T>
bool _fp_almost_equal(T A, T B, T max_abs_diff, T max_rel_diff) {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    T diff = fabs(A - B);
    if (diff <= max_abs_diff)
        return true;
    A = fabs(A);
    B = fabs(B);
    if (diff <= std::max(A, B) * max_rel_diff)
        return true;
    return false;
}

template <typename T>
bool _fp_almost_equal_abs_only(T A, T B, T max_abs_diff) {
    return fabs(A - B) <= max_abs_diff;
}

inline bool
double_equal(double A, double B,
             double abs_tol = std::numeric_limits<double>::epsilon(),
             double rel_tol = std::numeric_limits<double>::epsilon()) {
    return _fp_almost_equal(A, B, abs_tol, rel_tol);
}

inline bool
double_equal_abs_only(double A, double B,
                      double abs_tol = std::numeric_limits<double>::epsilon()) {
    return _fp_almost_equal_abs_only(A, B, abs_tol);
}

template <typename T>
constexpr int signum(T t) {
    if (t > 0)
        return 1;
    else if (t == 0)
        return 0;
    else
        return -1;
}

template <typename T>
constexpr int fsignum(T t) {
  	T abs = fabs(t);
    if (abs <= std::numeric_limits<T>::epsilon())
        return 0;
    else if (t == abs)
        return 1;
    return -1;
}

constexpr double take_step(double d, double dir) {
    return d + fsignum(dir) * std::max(fabs(d), 1.0) * 0x1p-20;
}
#endif

