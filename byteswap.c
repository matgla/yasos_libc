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

#include <byteswap.h>

uint16_t bswap_16(uint16_t x) {
  return (x << 8) | (x >> 8);
}

uint32_t bswap_32(uint32_t x) {
  return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |
         ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}

uint64_t bswap_64(uint64_t x) {
  return ((x & 0x00000000000000FFULL) << 56) |
         ((x & 0x000000000000FF00ULL) << 40) |
         ((x & 0x00000000FF000000ULL) << 24) |
         ((x & 0x0000FF0000000000ULL) << 8) |
         ((x & 0x00FF000000000000ULL) >> 8) |
         ((x & 0xFF00000000000000ULL) >> 24) |
         ((x & 0x0000FF00000000FFULL) >> 40) |
         ((x & 0xFFFFFFFFFFFFFFFFULL) >> 56);
}