#pragma yasc
/**
 * dlfcn.h
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

#include <sys/_attrs.h>

#define RTLD_GLOBAL 0x00100
#define RTLD_LOCAL 0
#define RTLD_LAZY 0x00001
#define RTLD_NOW 0x00002
#define RTLD_DEFAULT ((void *)0)


/* dlclose first: dlopen names it as its deallocator (sys/_attrs.h); every
   handle dlopen returns is given back with one dlclose. */
int dlclose(drop void *owned handle);
void *?owned(dlclose) dlopen(char *?path, int mode);

/* null when the symbol is not found; HANDLE may be RTLD_DEFAULT (null) */
inout void *?borrows(static, handle) dlsym(void *?handle, char *name) __yl_nonnull(2);
/* null when no error happened since the last call; the text is libc's: C
   sees POSIX's `char *`, which it must not write through either */
const char *?@(borrows(static), c_api(char *)) dlerror(void);
