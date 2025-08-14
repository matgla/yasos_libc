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

#pragma once

#include <stddef.h>
#include <sys/types.h>

#include <pwd.h>

#include <stdio.h>


struct passwd *getpwnam(const char *name) {
  printf("TODO: Implement getpwnam\n");
  return NULL; // Not implemented
}

struct passwd root_passwd = (struct passwd) {
  .pw_name = "root",
  .pw_passwd = "root",
  .pw_uid = 0,
  .pw_gid = 0,
  .pw_gecos = "root",
  .pw_dir = "/root",
  .pw_shell = "/bin/sh",
};


struct passwd *getpwuid(uid_t uid) {
  return &root_passwd; // Return root user for now
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t size,
               struct passwd **result) {
  printf("TODO: Implement getpwnam_r\n");
  return -1;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t size,
               struct passwd **result) {
  result = &root_passwd;
  return 0; 
}