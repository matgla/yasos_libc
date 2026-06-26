// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

/* It used to also pull in <fcntl.h> and <inttypes.h>, neither of whose symbols
   are used by this header or expected from <sys/types.h> -- a layering leak
   that forced every TU including <sys/types.h> (i.e. almost all of them) to
   open + read + tokenize two extra headers (fcntl.h alone is ~1.8 KB); those
   are dropped.  We keep:
     <stdint.h> -- fixed-width / intptr_t types used below;
     <stddef.h> -- size_t, which consumers expect <sys/types.h> to provide;
     <endian.h> -- BSD-style BYTE_ORDER/LITTLE_ENDIAN macros that third-party
                   code (e.g. mibench sha.c) relies on seeing transitively via
                   <sys/types.h>; removing it silently disables their byte-swap
                   paths, so it stays on purpose. */
#include <stdint.h>
#include <stddef.h>
#include <endian.h>

typedef uint32_t dev_t;
typedef uint16_t gid_t;
typedef uint16_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t uid_t;

typedef intptr_t ssize_t;
typedef long long time_t;
typedef long useconds_t;

/* POSIX types: these live here (not in <fcntl.h>) so any TU that includes
   <sys/types.h> gets them without dragging in the open()/fcntl() surface. */
typedef int32_t pid_t;
typedef signed long off_t;

typedef signed long ino_t;
typedef uint32_t uid32_t;
typedef uint32_t gid32_t;
typedef long fpos_t;

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned long long u_quad_t;

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
