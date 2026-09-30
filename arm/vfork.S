// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// The FP save below is compiled in only when this library is built for a part
// that HAS an FPU (tcc defines __ARM_FP for any -mfpu other than soft). The
// runtime CONTROL.FPCA test cannot stand in for that check: on an FPU-less core
// -- a Cortex-M23, or QEMU's mps3-an524 Cortex-M33 model, which has no FPU at
// all -- FPCA reads 0, which is exactly the branch that runs the vpush, and the
// first vfork() then dies with a NOCP UsageFault (CFSR=0x00080000).
//
// r3 carries "the frame has {s0-s31} above {r4-r11,lr}" to the kernel
// (process_vfork_child in source/arch/armv8-m/context_switch.S), so leaving it
// at 0 here is what keeps the two sides agreeing on the frame layout.
#ifdef __ARM_FP
// Enable FP instruction encodings (vpush/vpop) regardless of the -mfpu the
// library is built with; whether the FP register file is actually saved is
// decided at runtime below via CONTROL.FPCA.
.fpu fpv5-sp-d16
#endif

.global vfork
.thumb_func
vfork:
    mrs r0, control
    tst r0, #4
    mov r3, #0
#ifdef __ARM_FP
    bne vfork_omit_fpu
    mov r3, #1
    vpush {s0-s31}
vfork_omit_fpu:
#endif
    push {r4-r11, lr}
    mov r2, sp
    push {r0}

    mov r0, lr
    mov r1, r11
    bl _vfork_process
    pop {r1}
    pop {r4-r11, lr}
#ifdef __ARM_FP
    cmp r1, #1
    bne vfork_omit_load_fpu
    vpop {s0-s31}
vfork_omit_load_fpu:
#endif
    push {r0}
    mov r0, #1 // unblock context switch
    svc #0
    pop {r0}
    bx lr
