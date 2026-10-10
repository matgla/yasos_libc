/*
 *   Copyright (c) 2025 Mateusz Stadnik

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef __cplusplus
#if defined(__TINYC__) && !defined(YASLIBC_REAL_BOOL)
/* What every tcc-built program has been compiled with so far. Not _Bool, so a
   value converts by truncation rather than to 0/1 -- kept only because moving
   the whole rootfs over is a change of its own.  The kernel opts out
   (build/kernel_tcc.zig passes -DYASLIBC_REAL_BOOL): its C is written for
   _Bool, and pico-sdk's `bool dma_channel_is_busy() { return ctrl & BUSY; }`
   with BUSY = 1 << 26 truncated to 0 -- tcc then rightly deleted the SDIO
   driver's wait for the command response and the RP2350 never saw its card. */
#define bool unsigned char
#define true 1
#define false 0
#elif defined(__TINYC__) || !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
/* C23 makes bool, true and false keywords; before that they are these.  tcc's
   -std=c23 has the bool keyword but not true and false, so it always gets
   all three. */
#define bool _Bool
#define true 1
#define false 0
#endif
#endif

#define TRUE 1
#define FALSE 0
#define __bool_true_false_are_defined 1