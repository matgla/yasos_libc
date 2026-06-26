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

#include "utest.h"

typedef union {
  float _Complex value;
  struct {
    float real;
    float imag;
  } parts;
} float_complex_parts;

typedef union {
  double _Complex value;
  struct {
    double real;
    double imag;
  } parts;
} double_complex_parts;

typedef union {
  long double _Complex value;
  struct {
    long double real;
    long double imag;
  } parts;
} long_double_complex_parts;

float _Complex sut_conjf(float _Complex value);
double _Complex sut_conj(double _Complex value);
long double _Complex sut_conjl(long double _Complex value);
float sut_crealf(float _Complex value);
double sut_creal(double _Complex value);
long double sut_creall(long double _Complex value);
float sut_cimagf(float _Complex value);
double sut_cimag(double _Complex value);
long double sut_cimagl(long double _Complex value);

UTEST(complex_tests, conj_returns_complex_conjugate) {
  float_complex_parts float_value;
  double_complex_parts double_value;
  long_double_complex_parts long_double_value;

  float_value.value = sut_conjf(1.5f + 2.25fi);
  ASSERT_EQ(1.5f, float_value.parts.real);
  ASSERT_EQ(-2.25f, float_value.parts.imag);

  double_value.value = sut_conj(3.0 + 4.5i);
  ASSERT_EQ(3.0, double_value.parts.real);
  ASSERT_EQ(-4.5, double_value.parts.imag);

  long_double_value.value = sut_conjl(5.0L + 6.75iL);
  ASSERT_EQ(5.0L, long_double_value.parts.real);
  ASSERT_EQ(-6.75L, long_double_value.parts.imag);
}

UTEST(complex_tests, real_and_imag_extract_components) {
  ASSERT_EQ(7.5f, sut_crealf(7.5f + 8.5fi));
  ASSERT_EQ(8.5f, sut_cimagf(7.5f + 8.5fi));

  ASSERT_EQ(9.25, sut_creal(9.25 + 10.75i));
  ASSERT_EQ(10.75, sut_cimag(9.25 + 10.75i));

  ASSERT_EQ(11.125L, sut_creall(11.125L + 12.875iL));
  ASSERT_EQ(12.875L, sut_cimagl(11.125L + 12.875iL));
}