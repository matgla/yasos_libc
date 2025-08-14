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

#include <mntent.h>

FILE *setmntent(const char *filename, const char *type) {
  printf("TODO: Implement setmntent\n");
  return NULL; // Not implemented
}

struct mntent *getmntent(FILE *stream) {
  printf("TODO: Implement getmntent\n");
  return NULL; // Not implemented
}

int addmntent(FILE *stream, const struct mntent *mnt) {
  printf("TODO: Implement addmntent\n");
  return -1; // Not implemented
}

int endmntent(FILE *streamp) {
  printf("TODO: Implement endmntent\n");
  return -1; // Not implemented
}

char *hasmntopt(const struct mntent *mnt, const char *opt) {
  printf("TODO: Implement hasmntopt\n");
  return NULL; // Not implemented
}