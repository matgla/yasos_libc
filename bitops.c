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
#include <strings.h>

static int first_set_bit(unsigned long long value) {
  int position = 1;

  while (value != 0) {
    if ((value & 1ULL) != 0)
      return position;
    value >>= 1;
    position++;
  }

  return 0;
}

static int leading_zero_bits(unsigned long long value, int bit_count) {
  int count = 0;
  unsigned long long mask;

  if (value == 0)
    return bit_count;

  mask = 1ULL << (bit_count - 1);
  while ((value & mask) == 0) {
    count++;
    mask >>= 1;
  }

  return count;
}

static int trailing_zero_bits(unsigned long long value, int bit_count) {
  int count = 0;

  if (value == 0)
    return bit_count;

  while ((value & 1ULL) == 0) {
    count++;
    value >>= 1;
  }

  return count;
}

static int popcount_bits(unsigned long long value) {
  int count = 0;

  while (value != 0) {
    count += (int)(value & 1ULL);
    value >>= 1;
  }

  return count;
}

int ffs(int value) {
  return first_set_bit((unsigned int)value);
}

int ffsl(long value) {
  return first_set_bit((unsigned long)value);
}

int ffsll(long long value) {
  return first_set_bit((unsigned long long)value);
}

int __clzsi2(unsigned int value) {
  return leading_zero_bits(value, sizeof(value) * CHAR_BIT);
}

int __ctzsi2(unsigned int value) {
  return trailing_zero_bits(value, sizeof(value) * CHAR_BIT);
}

int __paritysi2(unsigned int value) {
  return popcount_bits(value) & 1;
}

int __popcountsi2(unsigned int value) {
  return popcount_bits(value);
}

int __clzdi2(unsigned long long value) {
  return leading_zero_bits(value, sizeof(value) * CHAR_BIT);
}

int __ctzdi2(unsigned long long value) {
  return trailing_zero_bits(value, sizeof(value) * CHAR_BIT);
}

int __popcountdi2(unsigned long long value) {
  return popcount_bits(value);
}

int __paritydi2(unsigned long long value) {
  return popcount_bits(value) & 1;
}