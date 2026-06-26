// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <limits.h>

#include "../../libm/include/math.h"
#include "utest.h"

typedef union {
  double _Complex value;
  struct {
    double real;
    double imag;
  } parts;
} double_complex_parts;

typedef union {
  double value;
  unsigned long long bits;
} double_bits;

typedef union {
  float value;
  unsigned int bits;
} float_bits;

long double sut_fmaxl(long double left, long double right);
int sut_ilogb(double value);
int sut_ilogbf(float value);
int sut_ilogbl(long double value);
double _Complex sut___divdc3(double a_real, double a_imag, double b_real,
                             double b_imag);

static double fabs_test(double value) {
  return value < 0.0 ? -value : value;
}

static double make_double(unsigned long long bits) {
  double_bits repr;

  repr.bits = bits;
  return repr.value;
}

static float make_float(unsigned int bits) {
  float_bits repr;

  repr.bits = bits;
  return repr.value;
}

UTEST(math_runtime_tests, fmaxl_picks_larger_value) {
  ASSERT_EQ(4.0L, sut_fmaxl(4.0L, 3.0L));
  ASSERT_EQ(7.5L, sut_fmaxl(-1.0L, 7.5L));
  ASSERT_EQ(0.0L, sut_fmaxl(-0.0L, 0.0L));
}

UTEST(math_runtime_tests, ilogbl_returns_unbiased_exponent) {
  ASSERT_EQ(0, sut_ilogbl(1.0L));
  ASSERT_EQ(5, sut_ilogbl(32.0L));
  ASSERT_EQ(-3, sut_ilogbl(0.125L));
  ASSERT_EQ(FP_ILOGB0, sut_ilogbl(0.0L));
}

UTEST(math_runtime_tests, ilogb_and_ilogbf_follow_ilogbl_for_finite_values) {
  ASSERT_EQ(3, sut_ilogb(8.0));
  ASSERT_EQ(-4, sut_ilogb(0.0625));
  ASSERT_EQ(4, sut_ilogbf(16.0f));
  ASSERT_EQ(-2, sut_ilogbf(0.25f));
}

UTEST(math_runtime_tests, ilogb_family_handles_zero_nan_and_infinity) {
  const double infinity = make_double(0x7ff0000000000000ULL);
  const double nan = make_double(0x7ff8000000000000ULL);
  const float infinityf = make_float(0x7f800000U);
  const float nanf = make_float(0x7fc00000U);

  ASSERT_EQ(FP_ILOGB0, sut_ilogb(0.0));
  ASSERT_EQ(FP_ILOGB0, sut_ilogbf(0.0f));
  ASSERT_EQ(FP_ILOGB0, sut_ilogbl(0.0L));

  ASSERT_EQ(INT_MAX, sut_ilogb(infinity));
  ASSERT_EQ(INT_MAX, sut_ilogbf(infinityf));
  ASSERT_EQ(INT_MAX, sut_ilogbl((long double)infinity));

  ASSERT_EQ(FP_ILOGBNAN, sut_ilogb(nan));
  ASSERT_EQ(FP_ILOGBNAN, sut_ilogbf(nanf));
  ASSERT_EQ(FP_ILOGBNAN, sut_ilogbl((long double)nan));
}

UTEST(math_runtime_tests, divdc3_divides_complex_doubles) {
  double_complex_parts result;

  result.value = sut___divdc3(1.0, 2.0, 3.0, 4.0);

  ASSERT_LT(fabs_test(result.parts.real - 0.44), 1e-12);
  ASSERT_LT(fabs_test(result.parts.imag - 0.08), 1e-12);
}