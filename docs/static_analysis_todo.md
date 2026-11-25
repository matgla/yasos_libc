# Static analysis: what libc is missing

TODO list (2026-11-20, brought up to date the same day after the pointer
attributes landed, and again after assert, the safe APIs and the remaining
view results and handles). tcc can check calls into libc only when the headers say
what each function expects. The compiler side is in
`libs/tinycc/docs/static_analysis_todo.md`; most items here depend on one there,
and that dependency is named in brackets.

Rules:

- GCC attributes go through the `__yl_*` macros of `sys/_attrs.h`
  (`__yl_malloc`, `__yl_dealloc(f, n)`, `__yl_nonnull(...)`,
  `__yl_returns_nonnull`); a macro expands to nothing on a compiler without
  the attribute. Attributes tcc does not check yet (`format`,
  `warn_unused_result`, `nodiscard`, `alloc_size`, `access`, `deprecated`)
  are reported by its `-Wignored-attributes` (the safe default) outside
  system headers, so they get a macro gated by `__has_attribute` (tcc answers
  it only for what it implements, `decl/attribute.c`), never a raw spelling.
  `sys/cdefs.h`'s `__printflike` / `__scanflike` are gated that way (0fd98e2).
- yasc qualifiers cannot be written in a regular-C header: its yasc words are
  identifiers there, also in the expansion of its macros. What only yasc can
  say goes into `sys/yasc/<header>.h`, which the header includes before its
  own prototypes; under `__YASC__` that file says `#pragma yasc` (yasc to its
  end, the includer stays C) and declares the functions again. tcc keeps the
  first declaration's qualifiers (nonnull it merges from all of them). Every
  pointer there says `view` or `owned`, `maybe` where null is allowed: a
  parameter without `maybe` is nonnull to the flow checks, so the yasc
  declarations carry no `__yl_nonnull`. Don't
  redeclare there a function that carries `__yl_malloc` / `__yl_dealloc`, nor
  a deallocator: the first declaration would drop the attribute's effect.
  Regular C (`-ccompat`, gcc, host builds) sees exactly the C prototypes:
  libc's objects stay byte-identical.
- nonnull only where C or POSIX leave null undefined -- not where it is
  defined (free, fflush, setvbuf's buffer, time, perror, system, getcwd,
  realpath's buffer and path, setenv/unsetenv (EINVAL), getline's lineptr
  and n (EINVAL), freopen's path), not where libc gives it a meaning
  (fchmodat/fchownat: a null path is the descriptor), and not on the
  length-bounded functions (memcpy, strncmp, read, ...: null with a zero
  length, defined from C2y).
- An annotation that is wrong is worse than none: each one gets a test that
  compiles a correct and a wrong call (`tests/annotations/`).

## Already there

- [x] `noreturn` on `exit`, `abort`, `_exit`, `longjmp`, `siglongjmp`.
- [x] `calloc` checks `n * m` for overflow (`malloc.c`).
- [x] `MALLOC_DEBUG` (compile-time only): poisons freed blocks with 0xDE and
  reports a poison violation when a block is reused.
- [x] `__chk_fail()` (`bounds.c`), with no `*_chk` functions behind it yet.
- [x] `MEMTST` allocation tracking, on x86_64 only.
- [x] **The annotation header** `sys/_attrs.h` (0ce2dc5).
- [x] **Allocators and their deallocators** (0ce2dc5, 2e3d79f):
  `malloc`, `calloc`, `aligned_alloc`, `memalign` (`malloc` +
  `malloc(free, 1)`), `realloc` (`malloc(free, 1)` only), `strdup`/`strndup`
  (`malloc`); `fopen`/`fdopen`/`tmpfile`/`fmemopen` -> `fclose`, `popen` ->
  `pclose`, `opendir`/`fdopendir` -> `closedir`, `setmntent` -> `endmntent`,
  `dlopen` -> `dlclose`. In yasc a result is `owned maybe` and the
  deallocator's argument `owned maybe drop`: leaks, double close, use after
  close and discarded results warn (-Wlifetime).
- [x] **`nonnull`** (0ce2dc5, 980648a; -Wnonnull in tcc, gcc, clang):
  - the whole-string `str*` functions, `strdup`, `strtok`/`strtok_r`'s
    delimiters, `strcasecmp` (string.h and strings.h);
  - stdio: every `FILE *` but the deallocators and `fflush`, the formats of
    the printf/scanf families, `fputs`/`puts`, `fgets`, `fopen`/`popen`/
    `fdopen`/`fmemopen`'s names and modes, `freopen`'s mode and stream,
    `rename`/`renameat`/`renameat2`, `remove`, `getline`/`getdelim`'s stream;
  - stdlib: `ato*`, `strto*`'s string, `getenv`, `putenv`, `mkstemp`,
    `mkdtemp`, `itoa`, `qsort`/`bsearch`'s comparison and `bsearch`'s key,
    `atexit`, `on_exit`;
  - paths: `open`/`creat`/`openat`, the `stat` family and its buffer,
    `access`, `unlink(at)`, `chdir`, `rmdir`, `readlink(at)`, `(l)chown`,
    `link(at)`, `symlink(at)`, `chroot`, `faccessat`, `exec*`, `mkdir(at)`,
    `mknod(at)`, `mkfifo`, `chmod`, `pipe`/`pipe2`;
  - time.h's `time_t` / `struct tm` pointers, `strftime`'s format,
    `strptime`, `nanosleep`'s request; `readdir`, `getmntent`/`addmntent`/
    `hasmntopt`, `dlsym`'s name, `getpwnam`, `getgrnam`, `initgroups`.
- [x] **`maybe` results** (e0b04d4, `sys/yasc/`), `view maybe`: `strchr`,
  `strrchr`, `strstr`, `strpbrk`, `memchr` (and `__memchr_c`), `memrchr`,
  `strtok`, `strtok_r`, `getenv`, `bsearch`, `mkdtemp`, `fgets`, `tmpnam`,
  `readdir`, `localtime(_r)`, `gmtime(_r)`, `ctime(_r)`, `strptime`,
  `getpwnam`, `getpwuid`, `getgrnam`, `getgrgid`, `setlocale`, `dlsym`,
  `dlerror`, `getmntent`, `hasmntopt`. In yasc a result stored in a pointer
  that is not maybe, or dereferenced before a test, warns; so does a maybe
  value for one of their non-maybe parameters.
  - [x] (fixed in tinycc 2bac02645a: the helpers take the declared result
    type and the call's -Wlifetime record; check.sh runs both ways)
    tcc's string builtins (`gen/builtin/string.c`) redirect `strchr`,
    `strrchr`, `strstr` and `strpbrk` to `__tcc_*` helpers with a result type
    of their own, which drops `maybe` (only `-fno-builtin` keeps it), and give
    `strchr` the converted argument's type: `char *p = strchr(s, c)` with a
    plain `char *s` reports "assignment discards qualifiers" (-Wall; an error
    in the safe default -- 38 of the ratchet's errors, all acutest.h:901).
    [tcc: keep the declared result type; take `const` from the argument as
    written]

## P0

- [x] **A real `assert()`** (2026-11-20; it was `((void)0)` even without
  `NDEBUG`): without `NDEBUG` a false expression calls
  `__assert_fail(expr, file, line, func)` (`assert.c`, noreturn), which writes
  "FILE:LINE: FUNC: Assertion \`EXPR' failed." with write(2) -- no stdio -- and
  `abort()`s. The macro is variadic (C23: a comma in a compound literal is one
  argument) and defined again at each inclusion from the `NDEBUG` in force
  there; `static_assert` is `_Static_assert` before C23 and the keyword after.
  tcc reads `assert(p)` as a null test (tests/annotations). libc's objects do
  not change: TRE, its only user, defines `NDEBUG` in `tre.h`.
  - [ ] Programs get live asserts now -- no rootfs build passes `NDEBUG`
    (tinycc only with `RELEASE=yes`): GNU make has 43 (its sources compile
    with the new header), ctags' gnulib `fnmatch` 5 and its build-time packcc
    more; toybox and the other apps none outside comments. Run the QEMU
    smoke suite (make, ctags) after the rootfs picks this up; a failure there
    is a latent bug the no-op hid.

## P1: header annotations

- [ ] **`format`**: `printf`, `fprintf`, `sprintf`, `snprintf`, `dprintf`,
  `vprintf` (`format(printf, n, 0)`) and the other `v*printf`, `syslog`/`vsyslog`,
  the `scanf` family, `strftime` (`format(strftime, 3, 0)`), and `err`/`warn` if
  added. A `__yl_format` macro gated by `__has_attribute(__format__)`.
  [tcc `-Wformat`]
- [ ] **nonnull, what is left**:
  - `returns_nonnull` on `strcpy`/`strcat`/`memcpy`/`stpcpy` (they return
    their argument): tcc reads `returns_nonnull` only on a malloc result
    today, and yasc already reads those results as unchecked.
  - [x] (fixed in tinycc 9d3df34968: every declaration's nonnull marks merge)
    a nonnull of an earlier declaration is lost when the function is
    declared again or defined (tcc keeps the first declaration's type but does
    not merge the attribute): libc's `fchown` calling `fchownat(fd, 0, ...)`
    after its definition is reported by gcc, not by tcc; strings.h and
    string.h both say `strcasecmp`'s. [tcc]
  - [ ] a maybe value for a `nonnull` parameter of a regular-C declaration is
    not reported (only a null constant); the `sys/yasc/` functions get it from
    their non-maybe parameters. [tcc: nonnull = not maybe in the flow check]
- [ ] **`warn_unused_result` / `[[nodiscard]]`**: `realloc`, `mkstemp`, `dup`,
  `pipe`, `fork`, `setuid`, `chdir`, `read`, `write`, `fgets`, `fread`, and
  `scanf` results (the conversion count); `malloc`/`calloc`/`strdup`/`fopen`/
  `fdopen` are covered in yasc (a discarded owned result warns).
  [tcc `-Wunused-result`]
- [ ] **`alloc_size`**: `malloc` (`alloc_size(1)`), `calloc` (`(1, 2)`),
  `realloc` (`(2)`), `aligned_alloc`, `memalign`. [tcc
  `__builtin_object_size`]
- [ ] **`deprecated`** in yasc only -- in the `sys/yasc/` files, so regular C
  never sees it:
  - `tmpnam` (race; use `mkstemp`)
  - `sprintf`/`vsprintf` (use `snprintf`)
  - `strcpy`/`strcat` (use `strlcpy`/`strlcat`, below)
  - `strtok` (use `strtok_r`, already present)
  - `atoi`/`atol` (no error report; use `strtol`)

  [tcc `-Wdeprecated-declarations`]
- [ ] **C23 qualifier-preserving generics**: `strchr`, `strrchr`, `strstr`,
  `strpbrk`, `memchr`, `bsearch` return `const T *` for a `const T *`
  argument. With discarded-qualifiers an error, this is what keeps a `const`
  from being cast away by a libc call. tcc's builtin redirect half-does it
  (above, wrongly for a non-const argument); a header `_Generic` would be
  regular C and drop the yasc declaration's `maybe`. [tcc builtin fix first]

## P1: yasc ownership signatures (`sys/yasc/`)

A pointer declared in regular C is *unchecked* from yasc (no diagnostics;
tinycc `docs/yasc_modules.md`, "Legacy C"), so a libc function is checked
only as far as its attributes or its `sys/yasc/` declaration say.

- [x] Wrong deallocator (`free(f)` on a FILE): tcc maps the existing
  `malloc(fclose, 1)` (`__yl_dealloc`) to `owned(fclose)`, so libc needed
  nothing: "'f' is released by fclose, not free" (2026-11-20).
- [x] `realloc`: `void * owned(free) maybe takes(v) realloc(...)` in
  `sys/yasc/stdlib.h`; `fdopen` takes its fd the same way (`sys/yasc/stdio.h`)
  (2026-11-20).
- [x] (2026-11-20) Results that point into an argument: `borrows(s)` on
  `strchr`, `strrchr`, `strstr`, `strpbrk`, `memchr`, `memrchr`, `strtok_r`,
  `fgets`, `strptime`, `mkdtemp`, `hasmntopt`; `borrows(dst/buf/s)` on
  `strcpy`, `strcat`, `stpcpy`, `asctime_r`, `itoa`, `inet_ntop` (maybe),
  `memset_explicit`; `borrows(static, path)` on `dirname`, `basename` (now
  POSIX: `basename("/usr/")` is "usr", `basename(NULL)` "."; it was "" and
  null); `borrows(static)` on `getenv`, `localtime`/`gmtime`, `ctime`,
  `getpw*`/`getgr*`, `setlocale`, `dlerror`, `tmpnam(NULL)`, `strerror`,
  `asctime`, `nl_langinfo(_l)`, `gai_strerror` (a fixed message now, its stub
  returned null), `getlogin` (maybe). The elision would read a `view` result
  with one `view` parameter as borrowing from it, which is wrong for
  `getenv(name)` and `localtime(t)`, too wide for `asctime_r` and the string
  copies (borrowing from the source too), and one with none (`strerror`,
  `dlerror`) is an error. Checked by returning each result from a
  `borrows(static)` function (HIT when it points into an argument) and from
  one that borrows the right parameter (clean).
  - Not done: `memcpy`/`memmove`/`memset`/`mempcpy`/`memccpy`, `strncpy`/
    `stpncpy`/`strncat`, the `wmem*`: length-bounded, so their destination
    is `view maybe` and the result would be maybe too -- noise for a result
    nearly nobody reads. `getcwd`/`realpath`: below.
  - [ ] tcc follows `borrows(...)` through a call's result for escapes (the
    HIT lines above) but not yet in the use-after-free flow check:
    `v = strchr(b, 'x'); free(b); *v` is reported only when `v = b`. [tcc,
    "then `borrows` through calls"]
- [x] File descriptors (2026-11-20): `int owned(close) maybe` from open,
  creat, openat, dup, socket, accept (with `listen`, new ENOSYS stubs like the
  other socket calls; the listening fd is `int view`), mkstemp,
  inotify_init/inotify_init1; `int close(int drop owned(close) maybe fd)` (an
  invalid one is fine, as free(NULL)); read/write and inotify_add_watch take
  `int view fd`. `dup2(int view fd, int fd2)` returns a plain int: the
  result is fd2, a number the caller picked and already accounts for (nearly
  always 0, 1 or 2, which no variable owns), so an owned result would report
  every `dup2(p[1], 1)` as a leak.
  - Still to do: `pipe`/`pipe2`/`socketpair` (an array of owned handles:
    owned out-parameters first; until then `close(p[0])` on pipe's plain ints
    warns "an int that owns nothing is stored in an owned handle"). `dup3`
    and `accept4` do not exist here. `fcntl(F_DUPFD)` returns a handle only
    for that command. `fileno` would be an `int view` borrowing its FILE --
    untried on int handles.
- [x] Allocation through out-parameters (2026-11-20): `getline`/`getdelim`
  (`char * owned(free) maybe * view lineptr`, in/out: still the caller's after
  -1), `posix_memalign` (`out void * owned(free) maybe * view`, written on 0
  only), `getaddrinfo` (`out struct addrinfo * owned(freeaddrinfo) maybe *
  view res`) with `freeaddrinfo` its drop.  tcc follows `&v` passed to such a
  parameter and reads a status result's test (0 / >= 0 success).
  - Done later the same day: `regcomp`/`regexec`/`regfree` (`regex_t
    owned(regfree)`, an object owned as a whole: sys/yasc/regex.h) and
    `pipe`/`pipe2` (`out int owned(close) maybe * view fds`: both written on
    0, followed in a local `int owned(close) maybe p[2]`).
  - Still to do: `asprintf`/`vasprintf` and `scandir` do not exist here;
    `socketpair` does not either.
- [ ] Out parameters that are only written: `strtol`/`strtod`'s `endptr`
  (`char * view * out maybe`), `pipe(int fds[2])`, `localtime_r`'s result,
  `stat`'s buffer -- useful once callers' `-Wmaybe-uninitialized` reads `out`
  at a call. [tcc]
- [ ] Not annotated on purpose: `freopen` (returns its argument; the result
  of `freopen(path, mode, stdin)` is dropped), `realpath`/`getcwd`/`tmpnam`
  with a caller buffer (owned only when the buffer is null), `mmap` (fails
  with `MAP_FAILED`, not null), `newlocale` (`locale_t` is not a pointer).

## P1: missing safe APIs

- [x] `strlcpy` / `strlcat` (POSIX 2024, `string_safe.c`, 2026-11-20):
  nonnull `src`, `dst` may be null with size 0; yasc: `src` a view, `dst` a
  view maybe.
- [x] `memset_explicit` (C23) and `explicit_bzero` (2026-11-20): memset and
  an empty asm that reads the memory. tcc knows `memset` by name (frame DFE,
  `ir/frame_dfe.c`) and drops one into a local not read again at -O1 and up;
  it does not know these, so the call stays (checked in the disassembly; gcc
  -O2 the same). yasc: `void * borrows(s) maybe`, `s` a view maybe.
- [x] `reallocarray` (2026-11-20): null with ENOMEM when `n * m` overflows,
  `p` untouched. C: `malloc(free, 1)` as realloc; yasc:
  `void * owned(free) maybe takes(p)`.
- [ ] `asprintf` / `vasprintf` (POSIX 2024) -- with owned out-parameters.
- [x] `ckd_add` / `ckd_sub` / `ckd_mul` in `<stdckdint.h>` (2026-11-20): on
  `__builtin_{add,sub,mul}_overflow`. tcc implements them but its
  `__has_builtin` answers no on purpose (tccpp.c, a code-size choice for
  zig.h), so the header tests `__TINYC__`/`__GNUC__`, and other compilers
  get an `#error`. Host tests run gcc's builtins; tcc's are its own tests'.

Each new one gets its attributes (and a `sys/yasc/` declaration where it
returns a pointer that may be null) when it is added; host tests in
`tests/safe_api_tests.c`.

## P2: runtime hardening

- [ ] **`_FORTIFY_SOURCE`**: `__memcpy_chk`, `__memmove_chk`, `__memset_chk`,
  `__strcpy_chk`, `__strcat_chk`, `__sprintf_chk`, `__snprintf_chk`,
  `__vsnprintf_chk`, `__read_chk` and `__fgets_chk`, all calling `__chk_fail`.
  Header wrappers use `__builtin_object_size(dst, 1)`. Level 3 uses
  `__builtin_dynamic_object_size`. [tcc object size builtins]
- [ ] **Stack protector runtime**: `__stack_chk_guard`, seeded at startup from
  the RP2350 TRNG or the kernel (an `AT_RANDOM`-style auxv entry), and
  `__stack_chk_fail` (prints and `abort()`s). [tcc `-fstack-protector-strong`]
- [ ] **`%n` in `printf`**: refuse it (abort) unless the format is a literal,
  or always in safe mode.
- [ ] **malloc checks at runtime, not only at build time**:
  - a header magic check in `free` that is always on (double free, and
    freeing a non-heap pointer), which is cheap
  - `MALLOC_CHECK_`-style environment switches for poison and redzones
  - guard bytes after each block, checked on `free`
  - port `MEMTST`'s leak report at `exit` to ARM/YasOS
- [x] **UBSan handlers**: `__ubsan_handle_*` (print site and `abort`) for
  tcc's handler mode. Trap mode needs nothing. [tcc `-fsanitize=undefined`]
  Done 2026-11-21 (`ubsan.c`): clang's ABI and messages for add/sub/mul/
  negate/divrem overflow, shift_out_of_bounds, type_mismatch_v1,
  vla_bound_not_positive, out_of_bounds; each place reported once by the
  plain (recovering) handlers, the `_abort` variants abort. Tested through
  tinycc's copy (`tests/ir_tests/ubsan_runtime.c`, test_sanitize.py on QEMU).
- [ ] **`bcheck` hooks**: if tcc's `-b` is ported, libc must route
  `malloc`/`free`/`mem*`/`str*` through it.

## P2: analysing libc itself

- [ ] **Host test build under sanitizers**: `tests/` with gcc/clang
  `-fsanitize=address,undefined` in CI. Use `build-host`; the `fff.h` stubs
  are already host-friendly. Note that gcc may now drop a defensive null test
  for a nonnull parameter (`getenv`'s `!name`, `stat`'s): the build already
  passes `-Wno-nonnull-compare`.
- [ ] **`gcc -fanalyzer`** and **`clang --analyze`** over the host build.
  Triage once, then gate on new findings only.
- [ ] **valgrind memcheck** over the host tests (≤16 concurrent jobs).
- [ ] **Compile libc with tcc in safe mode** (drop `-ccompat` for libc first).
  libc is where the ownership APIs are defined, so it should be the first
  yasc-native code. The ratchet counts it (`libs/libc`: 89 files, 71 stop at
  their first error, nearly all "plain pointer in yasc").
- [ ] **Fuzzing (low priority)**: `sscanf`, `strtod`, `printf`, `regcomp`/`regexec`
  (TRE), `strptime`, `fnmatch`, the TZif parser. Run on the host build with libFuzzer.

## How the annotations are checked

- `libc.a` with `armv8m-tcc -ccompat -O2 -Wall -Werror` (the rootfs flags);
  its objects must not change with a header-only annotation.
- `make -C tests` (gcc host build, `-Werror`): 148 cases.
- A file including every annotated header, `-fsyntax-only` with
  `arm-none-eabi-gcc -Wall -Wextra -Wpedantic` and `clang -Wall -Wextra`:
  the same diagnostics before and after.
- `tests/annotations/check.sh [armv8m-tcc]`: tcc `-std=yasc -Wall` on a file
  with one wrong use per annotation (each line marked HIT must warn, no other
  line may), on one with the correct uses (silent), and `-ccompat -Wall` on
  the C equivalent (silent but for the null constants). Add a line there
  with each new annotation.
- yasos `scripts/safe_mode_ratchet.py`: must not grow. As long as a file
  stops at its first error (a plain pointer, in nearly every counted file)
  the annotations rarely move it.
