/*
 Copyright (c) 2025 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "utest.h"

#include <stdio.h>
#include <syscalls.h>

#include "syscalls_stub.h"

int sut_puts(const char *str);
int sut_fflush(FILE *stream);

typedef struct write_call_context {
  int fd;
  const void *buf;
  size_t count;
  void *result;
} write_call_context;

static void *context;

static void process_syscall_write(int number, const void *args, void *result,
                                  optional_errno *err) {
  write_call_context *write = (write_call_context *)context;
  const write_context *write_args = (const write_context *)args;
  write->fd = write_args->fd;
  write->buf = write_args->buf;
  write->count = write_args->count;
  result = write->result;
}

static void reset_context_state() {
  context = NULL;
}

struct stdio_tests {};

UTEST_F_SETUP(stdio_tests) {
  reset_context_state();
}

UTEST_F_TEARDOWN(stdio_tests) {
}

UTEST_F(stdio_tests, puts) {
  write_call_context call;
  trigger_supervisor_call_fake.custom_fake = process_syscall_write;
  context = &call;
  ASSERT_GE(sut_puts("Hello, World!"), 0);
  ASSERT_EQ(call.fd, STDOUT_FILENO);
  ASSERT_EQ(call.count, 14);
  ASSERT_STREQ(call.buf, "Hello, World!\n");
}