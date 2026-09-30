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

/* Every field is the same width, as on Linux. That is not tidiness: portable
 * code walks this struct in steps of sizeof(sysname) -- toybox's uname does
 * exactly that for -a -- so fields of different sizes print the wrong bytes.
 * The kernel fills it in (sys_uname), NUL-terminated and truncated to fit. */
#define _UTSNAME_LENGTH 65

struct utsname {
  char sysname[_UTSNAME_LENGTH];  /* Operating system name: "YasOS" */
  char nodename[_UTSNAME_LENGTH]; /* This machine's name; the board, since
                                     there is no network to be named on */
  char release[_UTSNAME_LENGTH];  /* Operating system release */
  char version[_UTSNAME_LENGTH];  /* When this kernel's image was built */
  char machine[_UTSNAME_LENGTH];  /* CPU architecture, e.g. "armv8-m" */
};

int uname(struct utsname *buf);