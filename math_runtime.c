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

#ifndef FP_ILOGB0
#define FP_ILOGB0 (-INT_MAX)
#endif

#ifndef FP_ILOGBNAN
#define FP_ILOGBNAN INT_MAX
#endif

typedef union {
  double value;
  unsigned long long bits;
} double_bits;

typedef union {
  double _Complex value;
  struct {
    double real;
    double imag;
  } parts;
} double_complex_parts;

static long double fabs_local(long double value) {
  return value < 0.0L ? -value : value;
}

static int isnan_local(long double value) {
  return value != value;
}

static int signbit_local(long double value) {
  double_bits repr;

  repr.value = (double)value;
  return (int)(repr.bits >> 63);
}

long double fmaxl(long double left, long double right) {
  if (isnan_local(left))
    return right;
  if (isnan_local(right))
    return left;
  if (left > right)
    return left;
  if (right > left)
    return right;
  if (left == 0.0L && right == 0.0L)
    return signbit_local(left) ? right : left;
  return left;
}

int ilogbl(long double value) {
  double_bits repr;
  unsigned long long fraction;
  int exponent;

  repr.value = (double)value;
  exponent = (int)((repr.bits >> 52) & 0x7ffu);
  fraction = repr.bits & 0x000fffffffffffffull;

  if (exponent == 0) {
    int shift = 0;

    if (fraction == 0)
      return FP_ILOGB0;
    while ((fraction & (1ULL << 51)) == 0) {
      fraction <<= 1;
      shift++;
    }
    return -1022 - shift;
  }

  if (exponent == 0x7ff)
    return fraction == 0 ? INT_MAX : FP_ILOGBNAN;

  return exponent - 1023;
}

int ilogb(double value) {
  return ilogbl((long double)value);
}

int ilogbf(float value) {
  return ilogbl((long double)value);
}

double _Complex __divdc3(double a_real, double a_imag, double b_real,
                        double b_imag) {
  double ratio;
  double denom;
  double_complex_parts result;

  if (fabs_local(b_real) < fabs_local(b_imag)) {
    ratio = b_real / b_imag;
    denom = b_real * ratio + b_imag;
    result.parts.real = (a_real * ratio + a_imag) / denom;
    result.parts.imag = (a_imag * ratio - a_real) / denom;
  } else {
    ratio = b_imag / b_real;
    denom = b_imag * ratio + b_real;
    result.parts.real = (a_imag * ratio + a_real) / denom;
    result.parts.imag = (a_imag - a_real * ratio) / denom;
  }

  return result.value;
}