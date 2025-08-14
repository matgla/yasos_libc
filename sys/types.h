// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>

typedef uint32_t dev_t;
typedef uint16_t gid_t;
typedef uint16_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t uid_t;

// typedef long ssize_t;
typedef long long time_t;
typedef long useconds_t;

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
