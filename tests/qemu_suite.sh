#!/bin/sh
# qemu_suite.sh LEVEL (O0, O1, O2, Os): libc's unit tests on QEMU, run by
# `make qemu` (tests/Makefile sets the environment below).
#
# The whole libc is compiled by tcc at -LEVEL as the rootfs builds it (-ccompat,
# the yasc sources in tcc's safe default), but bare metal (-fno-pic) and without
# YASLIBC_ARM_SVC_TRIGGER: trigger_supervisor_call stays external, and
# syscalls_stub.c's fake answers it, as in the host suite.  Every symbol libc
# defines is renamed sut_* in every object (definitions and references), and
# every *_tests.c is linked with it into one newlib program by tinycc's
# mps2-an505 harness.  Output and logs: $QEMU_BUILD/LEVEL/.
#
# Environment: QEMU_CC TINYCC YASOS_SYS_INCLUDE QEMU_BUILD QEMU_HARNESS
# QEMU_MACHINE QEMU_ARCH QEMU_TIMEOUT QEMU_NEWLIB_BUILD YASC_SRCS, QEMU_SKIP (tests
# left out) and QEMU_LIBC_HEADERS (tests built against libc's headers), as
# file names.

set -e
o=$1
here=$(cd "$(dirname "$0")" && pwd)
libc=$(dirname "$here")
out=$QEMU_BUILD/$o
mkdir -p "$out/libc"

cflags="-Wall -Werror -Wno-nonnull-compare -I$libc/libs -pedantic -nostdlib -nostdinc -I$libc/include \
-I$YASOS_SYS_INCLUDE -I$TINYCC/include -U_FORTIFY_SOURCE -D__YL_LIBC_SOURCE -ffunction-sections $QEMU_ARCH -$o"

: >"$out/libc.log"

# compile WHAT TCC-ARGS...: one tcc compile, its output in libc.log.  On a
# failure the compile runs again with stderr on the terminal when it is one,
# so tcc's -fdiagnostics-color=auto colours it and shows the caret (the log
# stays plain); otherwise the log is printed.
compile() {
  what=$1
  shift
  "$QEMU_CC" "$@" >>"$out/libc.log" 2>&1 && return 0
  if [ -t 2 ]; then
    "$QEMU_CC" "$@" || true
  else
    cat "$out/libc.log"
  fi
  echo "qemu libc tests -$o: $what did not build"
  exit 1
}

objs=
for src in "$libc"/*.c "$libc"/sys/*.c "$libc"/arpa/*.c; do
  rel=${src#"$libc"/}
  obj=$out/libc/$(echo "$rel" | tr / _ | sed 's/\.c$/.o/')
  case " $YASC_SRCS " in
    *" $rel "*) compat= ;;
    *) compat=-ccompat ;;
  esac
  compile "libc ($rel)" $compat $cflags -c "$src" -o "$obj"
  objs="$objs $obj"
done

arm-none-eabi-nm -g --defined-only $objs | awk 'NF == 3 { print $3 " sut_" $3 }' | sort -u >"$out/redefine.txt"
for obj in $objs; do
  arm-none-eabi-objcopy --redefine-syms="$out/redefine.txt" "$obj"
done
rm -f "$out/libc_sut.a"
arm-none-eabi-ar rcs "$out/libc_sut.a" $objs

# The tests build against newlib's headers, plus the few of libc's that newlib
# does not have: sys/_attrs.h (include/assert.h and the yasc/ headers reach for it) and
# sys/mman.h (the tests' "host" mmap, qemu_shim.c) -- from this tree, not the
# copies the cross finds installed in the rootfs.
mkdir -p "$out/inc/sys"
for h in sys/_attrs.h sys/mman.h; do
  ln -sfn "$libc/include/$h" "$out/inc/$h"
done

harness_cflags="-$o -D_DEFAULT_SOURCE -DLIBC_TESTS_QEMU -I$out/inc"

# The tests of libc's yasc sources (NAME_tests.c for NAME.c in YASC_SRCS) are
# yasc too: tcc's safe default (no -ccompat), every warning an error -- with
# the harness's own flags otherwise, which it prints (no "Entering directory"
# lines: a make -C above puts w in MAKEFLAGS, and they would become files).
yasc_cflags=$(make -s --no-print-directory -C "$QEMU_HARNESS" CC="$QEMU_CC" USE_NEWLIB_BUILD="$QEMU_NEWLIB_BUILD" \
  CCOMPAT= EXTRA_CFLAGS="$harness_cflags -Wall -Werror" \
  --eval 'print-cflags: ; @echo $(CFLAGS) $(TCC_FLAGS)' print-cflags)

# Tests whose types must be libc's (struct rlimit: newlib has none) build
# against libc's headers instead, here; the rest go to the harness.
tests=
own=
for t in "$here"/*_tests.c; do
  b=$(basename "$t")
  case " $QEMU_SKIP " in
    *" $b "*) continue ;;
  esac
  case " $YASC_SRCS " in
    *" ${b%_tests.c}.c "*)
      compile "$b (yasc)" $yasc_cflags -c "$t" -o "$out/${b%.c}.o"
      own="$own $out/${b%.c}.o"
      continue
      ;;
  esac
  case " $QEMU_LIBC_HEADERS " in
    *" $b "*)
      compile "$b" -ccompat -w -DLIBC_TESTS_QEMU -nostdlib -nostdinc -I"$libc/include" -I"$YASOS_SYS_INCLUDE" -I"$TINYCC/include" \
        -ffunction-sections $QEMU_ARCH -$o -c "$t" -o "$out/${b%.c}.o"
      own="$own $out/${b%.c}.o"
      ;;
    *) tests="$tests $t" ;;
  esac
done

rm -f "$out/libc_tests.elf"
if ! make -s -C "$QEMU_HARNESS" CC="$QEMU_CC" USE_NEWLIB_BUILD="$QEMU_NEWLIB_BUILD" OUTPUT="$out" \
  TARGET="$out/libc_tests.elf" \
  TEST_FILES="$tests $here/libc_tests_main.c $here/syscalls_stub.c $here/qemu_shim.c" \
  EXTRA_CFLAGS="$harness_cflags -w" EXTRA_OBJS="$own $out/libc_sut.a" >"$out/build.log" 2>&1; then
  grep -v '^GC:' "$out/build.log"
  echo "qemu libc tests -$o: the test program did not build"
  exit 1
fi

# utest's output streams as it runs (gtest-style [ RUN ] / [ OK ] lines), and
# is kept in run.log
echo "qemu libc tests -$o: running"
[ -z "$QEMU_SKIP" ] || echo "qemu libc tests -$o: skipped $QEMU_SKIP"
timeout "$QEMU_TIMEOUT" qemu-system-arm -machine "$QEMU_MACHINE" -nographic -semihosting \
  -kernel "$out/libc_tests.elf" </dev/null 2>&1 | tee "$out/run.log" || true
if grep -q "PASSED" "$out/run.log" && ! grep -q "FAILED" "$out/run.log"; then
  echo "qemu libc tests -$o: $(grep 'PASSED' "$out/run.log" | tail -1)"
else
  echo "qemu libc tests -$o: FAILED (log: $out/run.log)"
  exit 1
fi
