/*
 Copyright (c) 2026 Mateusz Stadnik <matgla@live.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#pragma once

/* reboot(2) commands, with Linux's values so programs written for it build
 * unchanged. Every command writes out what the mounted filesystems hold back
 * first, as sync(2) does; the caller does not have to. Halt and power-off stop
 * every core until the next reset: QEMU exits, an RP2350 powers its cores down
 * (halt) or switches off everything but its always-on domain (power-off).
 * None of the three returns on success. */
#define RB_AUTOBOOT 0x01234567
#define RB_HALT_SYSTEM 0xcdef0123
#define RB_POWER_OFF 0x4321fedc
/* Ctrl-Alt-Del: accepted and ignored, there is no such key to trap. The other
 * Linux commands (kexec, suspend) are EINVAL. */
#define RB_ENABLE_CAD 0x89abcdef
#define RB_DISABLE_CAD 0
#define RB_KEXEC 0x45584543
#define RB_SW_SUSPEND 0xd000fce2

int reboot(int cmd);
