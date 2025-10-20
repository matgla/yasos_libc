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

#include <sys/utsname.h>

#include <string.h>

#include <stdio.h>

int uname(struct utsname *buf) {
  const char *default_sysname = "yasos";
  const char *default_nodename = "localhost";
  const char *default_release = "0.0.1";
  const char *default_version = "0";
  const char *default_machine = "arm";

  memcpy(buf->sysname, default_sysname, strlen(default_sysname) + 1);
  memcpy(buf->nodename, default_nodename, strlen(default_nodename) + 1);
  memcpy(buf->release, default_release, strlen(default_release) + 1);
  memcpy(buf->version, default_version, strlen(default_version) + 1);
  memcpy(buf->machine, default_machine, strlen(default_machine) + 1);

  return -1; // Not implemented
}
