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

/* yasc's reading of the results that may be null (sys/_attrs.h) */
#include <sys/yasc/dlfcn.h>

/* dlclose first: dlopen names it as its deallocator (sys/_attrs.h); every
   handle dlopen returns is given back with one dlclose. */
int dlclose(void *);
void *dlopen(const char *, int) __yl_dealloc(dlclose, 1);
void *dlsym(void *, const char *) __yl_nonnull(2);
char *dlerror(void);
