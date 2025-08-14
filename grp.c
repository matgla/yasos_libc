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

#include <grp.h>

#include <stddef.h>

#include <stdio.h>

struct group *getgrgid(gid_t gid) {
  printf("TODO: Implement getgrgid\n");
  return NULL; // Not implemented
}

int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize,
               struct group **result) {
  printf("TODO: Implement getgrgid_r\n");
  return -1; // Not implemented
}

struct group *getgrnam(const char *name) {
  printf("TODO: Implement getgrnam\n");
  return NULL; // Not implemented
}

int getgrnam_r(const char *name, struct group *grp, char *buf, size_t size,
               struct group **result) {
  printf("TODO: Implement getgrnam_r\n");
  return -1; // Not implemented
}

int initgroups(const char *user, gid_t group) {
  printf("TODO: Implement initgroups\n");
  return -1; // Not implemented
}