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
#include <stddef.h>
#include <stdio.h>

#include <sys/syscall.h>

void *dlopen(const char *path, int mode) {
  void *result;
  printf("dlopen: %s %d\n", path, mode);
  const dlopen_context context = {
      .path = path,
      .flags = mode,
      .result = &result,
  };

  trigger_syscall(sys_dlopen, &context);
  printf("dlopen result: %p\n", result);
  return result;
}

void *dlsym(void *handle, const char *symbol_name) {
  void *result;
  printf("dlsym: %p %s\n", handle, symbol_name);
  const dlsym_context context = {
      .handle = handle,
      .symbol = symbol_name,
      .result = &result,
  };
  trigger_syscall(sys_dlsym, &context);
  printf("dlsym result: %p\n", result);
  return result;
}

int dlclose(void *handle) {
  const dlclose_context context = {
      .handle = handle,
  };
  return trigger_syscall(sys_dlclose, &context);
}

char *dlerror(void) {
  return "";
}
