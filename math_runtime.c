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

#include <float.h>
#include <limits.h>

// Built here rather than taken from <math.h>: this file is part of the libc
// that provides math.h, and the complex helpers must not depend on it.
#define INFINITY_F (1.0f / 0.0f)
#define INFINITY_D (1.0 / 0.0)

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

typedef union {
  float _Complex value;
  struct {
    float real;
    float imag;
  } parts;
} float_complex_parts;

static float fabsf_local(float value) {
  return value < 0.0f ? -value : value;
}

// Smith's algorithm is only accurate while the intermediate ratio and
// denominator stay inside the exponent range.  Where they do not, the naive
// form silently underflows a whole component to zero -- for
// (0x1.0b1600p-133 + 0x1.5e1c28p+54i) / (-0x1.cdec8cp-119 + 0x1.1e72ccp+32i)
// the ratio is 2^-151, below the smallest subnormal float, so the imaginary
// part came out 0 instead of -0x1.f89220p-129.  gcc.c-torture's cdivchk{f,d,ld}
// exist to catch exactly that, and all four of their vectors failed.
//
// The scaling below is libgcc2.c's: pre-scale the operands away from the ends
// of the range, and when the ratio itself is subnormal, reassociate so the
// division happens before the multiplication instead of after.  The NaN
// recovery afterwards restores the infinities and zeros that the scaled form
// computes as NaN+iNaN (nonzero/zero, infinite/finite, finite/infinite).
#define RBIG_F (FLT_MAX / 2)
#define RMIN_F FLT_MIN
#define RMIN2_F FLT_EPSILON
#define RMINSCAL_F (1 / FLT_EPSILON)
#define RMAX2_F (RBIG_F * RMIN2_F)

typedef union {
  float value;
  unsigned int bits;
} float_bits;

static int isnan_f(float value) {
  return value != value;
}

static int isinf_f(float value) {
  float_bits repr;

  repr.value = value;
  return (repr.bits & 0x7fffffffu) == 0x7f800000u;
}

static int isfinite_f(float value) {
  float_bits repr;

  repr.value = value;
  return (repr.bits & 0x7f800000u) != 0x7f800000u;
}

static float copysign_f(float magnitude, float sign) {
  float_bits m;
  float_bits s;

  m.value = magnitude;
  s.value = sign;
  m.bits = (m.bits & 0x7fffffffu) | (s.bits & 0x80000000u);
  return m.value;
}

float _Complex __divsc3(float a_real, float a_imag, float b_real,
                        float b_imag) {
  float a = a_real;
  float b = a_imag;
  float c = b_real;
  float d = b_imag;
  float ratio;
  float denom;
  float x;
  float y;
  float_complex_parts result;

  if (fabsf_local(c) < fabsf_local(d)) {
    // Halve first when the denominator is near the top of the range, so the
    // scale-up below cannot overflow it.
    if (fabsf_local(d) >= RBIG_F) {
      a = a / 2;
      b = b / 2;
      c = c / 2;
      d = d / 2;
    }
    if (fabsf_local(d) < RMIN2_F) {
      a = a * RMINSCAL_F;
      b = b * RMINSCAL_F;
      c = c * RMINSCAL_F;
      d = d * RMINSCAL_F;
    } else if (((fabsf_local(a) < RMIN_F) && (fabsf_local(b) < RMAX2_F) &&
                (fabsf_local(d) < RMAX2_F)) ||
               ((fabsf_local(b) < RMIN_F) && (fabsf_local(a) < RMAX2_F) &&
                (fabsf_local(d) < RMAX2_F))) {
      a = a * RMINSCAL_F;
      b = b * RMINSCAL_F;
      c = c * RMINSCAL_F;
      d = d * RMINSCAL_F;
    }
    ratio = c / d;
    denom = (c * ratio) + d;
    if (fabsf_local(ratio) > RMIN_F) {
      x = ((a * ratio) + b) / denom;
      y = ((b * ratio) - a) / denom;
    } else {
      x = ((c * (a / d)) + b) / denom;
      y = ((c * (b / d)) - a) / denom;
    }
  } else {
    if (fabsf_local(c) >= RBIG_F) {
      a = a / 2;
      b = b / 2;
      c = c / 2;
      d = d / 2;
    }
    if (fabsf_local(c) < RMIN2_F) {
      a = a * RMINSCAL_F;
      b = b * RMINSCAL_F;
      c = c * RMINSCAL_F;
      d = d * RMINSCAL_F;
    } else if (((fabsf_local(a) < RMIN_F) && (fabsf_local(b) < RMAX2_F) &&
                (fabsf_local(c) < RMAX2_F)) ||
               ((fabsf_local(b) < RMIN_F) && (fabsf_local(a) < RMAX2_F) &&
                (fabsf_local(c) < RMAX2_F))) {
      a = a * RMINSCAL_F;
      b = b * RMINSCAL_F;
      c = c * RMINSCAL_F;
      d = d * RMINSCAL_F;
    }
    ratio = d / c;
    denom = (d * ratio) + c;
    if (fabsf_local(ratio) > RMIN_F) {
      x = ((b * ratio) + a) / denom;
      y = (b - (a * ratio)) / denom;
    } else {
      x = (a + (d * (b / c))) / denom;
      y = (b - (d * (a / c))) / denom;
    }
  }

  if (isnan_f(x) && isnan_f(y)) {
    if (c == 0.0f && d == 0.0f && (!isnan_f(a) || !isnan_f(b))) {
      x = copysign_f(INFINITY_F, c) * a;
      y = copysign_f(INFINITY_F, c) * b;
    } else if ((isinf_f(a) || isinf_f(b)) && isfinite_f(c) && isfinite_f(d)) {
      a = copysign_f(isinf_f(a) ? 1.0f : 0.0f, a);
      b = copysign_f(isinf_f(b) ? 1.0f : 0.0f, b);
      x = INFINITY_F * (a * c + b * d);
      y = INFINITY_F * (b * c - a * d);
    } else if ((isinf_f(c) || isinf_f(d)) && isfinite_f(a) && isfinite_f(b)) {
      c = copysign_f(isinf_f(c) ? 1.0f : 0.0f, c);
      d = copysign_f(isinf_f(d) ? 1.0f : 0.0f, d);
      x = 0.0f * (a * c + b * d);
      y = 0.0f * (b * c - a * d);
    }
  }

  result.parts.real = x;
  result.parts.imag = y;
  return result.value;
}

// The double form of the scaling above; see __divsc3 for why it is needed.
#define RBIG_D (DBL_MAX / 2)
#define RMIN_D DBL_MIN
#define RMIN2_D DBL_EPSILON
#define RMINSCAL_D (1 / DBL_EPSILON)
#define RMAX2_D (RBIG_D * RMIN2_D)

static double fabs_d(double value) {
  return value < 0.0 ? -value : value;
}

static int isnan_d(double value) {
  return value != value;
}

static int isinf_d(double value) {
  double_bits repr;

  repr.value = value;
  return (repr.bits & 0x7fffffffffffffffull) == 0x7ff0000000000000ull;
}

static int isfinite_d(double value) {
  double_bits repr;

  repr.value = value;
  return (repr.bits & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

static double copysign_d(double magnitude, double sign) {
  double_bits m;
  double_bits s;

  m.value = magnitude;
  s.value = sign;
  m.bits = (m.bits & 0x7fffffffffffffffull) | (s.bits & 0x8000000000000000ull);
  return m.value;
}

double _Complex __divdc3(double a_real, double a_imag, double b_real,
                        double b_imag) {
  double a = a_real;
  double b = a_imag;
  double c = b_real;
  double d = b_imag;
  double ratio;
  double denom;
  double x;
  double y;
  double_complex_parts result;

  if (fabs_d(c) < fabs_d(d)) {
    if (fabs_d(d) >= RBIG_D) {
      a = a / 2;
      b = b / 2;
      c = c / 2;
      d = d / 2;
    }
    if (fabs_d(d) < RMIN2_D) {
      a = a * RMINSCAL_D;
      b = b * RMINSCAL_D;
      c = c * RMINSCAL_D;
      d = d * RMINSCAL_D;
    } else if (((fabs_d(a) < RMIN_D) && (fabs_d(b) < RMAX2_D) &&
                (fabs_d(d) < RMAX2_D)) ||
               ((fabs_d(b) < RMIN_D) && (fabs_d(a) < RMAX2_D) &&
                (fabs_d(d) < RMAX2_D))) {
      a = a * RMINSCAL_D;
      b = b * RMINSCAL_D;
      c = c * RMINSCAL_D;
      d = d * RMINSCAL_D;
    }
    ratio = c / d;
    denom = (c * ratio) + d;
    if (fabs_d(ratio) > RMIN_D) {
      x = ((a * ratio) + b) / denom;
      y = ((b * ratio) - a) / denom;
    } else {
      x = ((c * (a / d)) + b) / denom;
      y = ((c * (b / d)) - a) / denom;
    }
  } else {
    if (fabs_d(c) >= RBIG_D) {
      a = a / 2;
      b = b / 2;
      c = c / 2;
      d = d / 2;
    }
    if (fabs_d(c) < RMIN2_D) {
      a = a * RMINSCAL_D;
      b = b * RMINSCAL_D;
      c = c * RMINSCAL_D;
      d = d * RMINSCAL_D;
    } else if (((fabs_d(a) < RMIN_D) && (fabs_d(b) < RMAX2_D) &&
                (fabs_d(c) < RMAX2_D)) ||
               ((fabs_d(b) < RMIN_D) && (fabs_d(a) < RMAX2_D) &&
                (fabs_d(c) < RMAX2_D))) {
      a = a * RMINSCAL_D;
      b = b * RMINSCAL_D;
      c = c * RMINSCAL_D;
      d = d * RMINSCAL_D;
    }
    ratio = d / c;
    denom = (d * ratio) + c;
    if (fabs_d(ratio) > RMIN_D) {
      x = ((b * ratio) + a) / denom;
      y = (b - (a * ratio)) / denom;
    } else {
      x = (a + (d * (b / c))) / denom;
      y = (b - (d * (a / c))) / denom;
    }
  }

  if (isnan_d(x) && isnan_d(y)) {
    if (c == 0.0 && d == 0.0 && (!isnan_d(a) || !isnan_d(b))) {
      x = copysign_d(INFINITY_D, c) * a;
      y = copysign_d(INFINITY_D, c) * b;
    } else if ((isinf_d(a) || isinf_d(b)) && isfinite_d(c) && isfinite_d(d)) {
      a = copysign_d(isinf_d(a) ? 1.0 : 0.0, a);
      b = copysign_d(isinf_d(b) ? 1.0 : 0.0, b);
      x = INFINITY_D * (a * c + b * d);
      y = INFINITY_D * (b * c - a * d);
    } else if ((isinf_d(c) || isinf_d(d)) && isfinite_d(a) && isfinite_d(b)) {
      c = copysign_d(isinf_d(c) ? 1.0 : 0.0, c);
      d = copysign_d(isinf_d(d) ? 1.0 : 0.0, d);
      x = 0.0 * (a * c + b * d);
      y = 0.0 * (b * c - a * d);
    }
  }

  result.parts.real = x;
  result.parts.imag = y;
  return result.value;
}