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

#include <stdio.h>
#include <unistd.h>

static unsigned int tmpnam_counter;

static void append_hex(char *out, unsigned int value) {
  static const char hex_digits[] = "0123456789abcdef";
  int index;

  for (index = 0; index < 8; ++index) {
    out[7 - index] = hex_digits[value & 0x0f];
    value >>= 4;
  }
}

char *tmpnam(char *buffer) {
  static char internal_buffer[L_tmpnam];
  char *result = buffer != NULL ? buffer : internal_buffer;
  unsigned int sequence = tmpnam_counter;
  int attempt;

  for (attempt = 0; attempt < TMP_MAX; ++attempt) {
    unsigned int token = (unsigned int)getpid() ^ (sequence + (unsigned int)attempt);

    result[0] = '/';
    result[1] = 't';
    result[2] = 'm';
    result[3] = 'p';
    result[4] = '/';
    result[5] = 't';
    result[6] = 'm';
    result[7] = 'p';
    result[8] = '_';
    append_hex(result + 9, token);
    result[17] = '\0';

    if (access(result, F_OK) != 0) {
      tmpnam_counter = sequence + (unsigned int)attempt + 1;
      return result;
    }
  }

  return NULL;
}