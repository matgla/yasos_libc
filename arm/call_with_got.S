// call_with_got.S — Helper to call a function pointer with a different R9
// (GOT base register) context, then restore the caller's R9.
//
// void __call_with_got(void (*func)(void), void *got_base);
//   r0 = function pointer to call
//   r1 = GOT base (R9 value) to use during the call

.syntax unified
.thumb

.global __call_with_got
.thumb_func
__call_with_got:
    push {r4, lr}
    mov  r4, r9        // save current R9 (libc's GOT base)
    mov  r9, r1        // set R9 to caller's GOT base
    blx  r0            // call the function
    mov  r9, r4        // restore libc's GOT base
    pop  {r4, pc}
