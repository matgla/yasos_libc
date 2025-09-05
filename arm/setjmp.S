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

.global setjmp
.thumb_func
setjmp:
    mov r1, sp
    stmia r0, {r1, r4-r11, lr}
    movs r0, #0
    bx lr

.global longjmp
.thumb_func
longjmp:
    ldmia r0, {r2, r4-r11, lr}
    mov sp, r2
    movs r0, r1
    bx lr

.global siglongjmp
.thumb_func
siglongjmp:
    ldmia r0, {r2, r4-r11, lr}
    mov sp, r2
    movs r0, r1
    bx lr


.global sigsetjmp
.thumb_func
sigsetjmp:
    mov r1, sp
    stmia r0, {r1, r4-r11, lr}
    movs r0, #0
    bx lr




