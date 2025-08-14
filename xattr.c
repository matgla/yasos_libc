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

#include <sys/xattr.h>

#include <stdio.h>

ssize_t getxattr(const char *path, const char *name, void *value, size_t size) {
  printf("TODO: Implement getxattr\n");
  return -1; // Not implemented
}

ssize_t lgetxattr(const char *path, const char *name, void *value,
                  size_t size) {
  printf("TODO: Implement lgetxattr\n");
  return -1; // Not implemented
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size) {
  printf("TODO: Implement fgetxattr\n");
  return -1; // Not implemented
}

ssize_t listxattr(const char *path, char *list, size_t size) {
  printf("TODO: Implement listxattr\n");
  return -1; // Not implemented
}

ssize_t llistxattr(const char *path, char *list, size_t size) {
  printf("TODO: Implement llistxattr\n");
  return -1; // Not implemented
}

ssize_t flistxattr(int fd, char *list, size_t size) {
  printf("TODO: Implement flistxattr\n");
  return -1; // Not implemented
}

int setxattr(const char *path, const char *name, const void *value, size_t size,
             int flags) {
  printf("TODO: Implement setxattr\n");
  return -1; // Not implemented
}

int lsetxattr(const char *path, const char *name, const void *value,
              size_t size, int flags) {
  printf("TODO: Implement lsetxattr\n");
  return -1; // Not implemented
}

int fsetxattr(int fd, const char *name, const void *value, size_t size,
              int flags) {
  printf("TODO: Implement fsetxattr\n");
  return -1; // Not implemented
}