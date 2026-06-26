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

float _Complex conjf(float _Complex value) {
  float_complex_parts result;

  result.value = value;
  result.parts.imag = -result.parts.imag;
  return result.value;
}

double _Complex conj(double _Complex value) {
  double_complex_parts result;

  result.value = value;
  result.parts.imag = -result.parts.imag;
  return result.value;
}

long double _Complex conjl(long double _Complex value) {
  long_double_complex_parts result;

  result.value = value;
  result.parts.imag = -result.parts.imag;
  return result.value;
}

float crealf(float _Complex value) {
  float_complex_parts parts;

  parts.value = value;
  return parts.parts.real;
}

double creal(double _Complex value) {
  double_complex_parts parts;

  parts.value = value;
  return parts.parts.real;
}

long double creall(long double _Complex value) {
  long_double_complex_parts parts;

  parts.value = value;
  return parts.parts.real;
}

float cimagf(float _Complex value) {
  float_complex_parts parts;

  parts.value = value;
  return parts.parts.imag;
}

double cimag(double _Complex value) {
  double_complex_parts parts;

  parts.value = value;
  return parts.parts.imag;
}

long double cimagl(long double _Complex value) {
  long_double_complex_parts parts;

  parts.value = value;
  return parts.parts.imag;
}