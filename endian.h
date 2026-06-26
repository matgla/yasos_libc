// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
// Please check the LICENSE file for copying conditions.

#pragma once

/* Byte-order macros (glibc/newlib compatible).  YasOS targets are
 * little-endian ARM only. */

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __PDP_ENDIAN 3412

#define __BYTE_ORDER __LITTLE_ENDIAN

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#define PDP_ENDIAN __PDP_ENDIAN
#define BYTE_ORDER __BYTE_ORDER
