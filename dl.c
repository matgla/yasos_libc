/**
 * dl.c
 *
 * Copyright (C) 2025 Mateusz Stadnik <matgla@live.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General
 * Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/syscall/dl.h>

/* dlerror()'s message: the last failure of dlopen, dlsym or dlclose, kept
   until dlerror() reads it (a call that works leaves it as it is). */
static char dl_message[192];
static bool dl_pending;

/* a dl call failed with errno: the message is "WHAT: NAME: cause" */
static void dl_failed(char *what, char *?name) {
  const char *cause = errno ? strerror(errno) : "failed";
  if (name)
    snprintf(dl_message, sizeof dl_message, "%s: %s: %s", what, name, cause);
  else
    snprintf(dl_message, sizeof dl_message, "%s: %s", what, cause);
  dl_pending = true;
}

/* the system call NUMBER with CONTEXT: errno is the kernel's on a failure,
   and as it was when the call works */
static int dl_call(int number, void *context) {
  const int saved = errno;
  errno = 0;
  const int rc = trigger_syscall(number, context);
  if (rc == 0)
    errno = saved;
  return rc;
}

void *?owned(dlclose) dlopen(char *?path, int mode) {
  void *result = nullptr;
  const dlopen_context context = {
      .path = path,
      .flags = mode,
      .result = &result,
  };

  if (dl_call(sys_dlopen, &context) != 0) {
    dl_failed("dlopen", path);
    return nullptr;
  }
  return unsafe(result);
}

inout void *?borrows(static, handle) dlsym(void *?handle, char *symbol_name) {
  void *?borrows(static) result = nullptr;
  const dlsym_context context = {
      .handle = handle,
      .symbol = symbol_name,
      .result = &result,
  };
  if (dl_call(sys_dlsym, &context) != 0) {
    dl_failed("dlsym", symbol_name);
    return nullptr;
  }
  return result;
}

int dlclose(drop void *owned handle) {
  /* the kernel takes the handle back, whatever it answers */
  const dlclose_context context = {
      .handle = move(handle),
  };
  const int rc = dl_call(sys_dlclose, &context);
  if (rc != 0)
    dl_failed("dlclose", nullptr);
  return rc;
}

const char *?borrows(static) dlerror(void) {
  if (!dl_pending)
    return nullptr;
  dl_pending = false;
  return dl_message;
}
