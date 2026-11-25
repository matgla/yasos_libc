CC = gcc
# _DEFAULT_SOURCE exposes the host glibc POSIX/BSD extensions (e.g. P_tmpdir)
# that strict -std=c11 otherwise hides, matching what the yasos libc headers
# provide unconditionally.
CFLAGS = -Wall -pedantic -g -std=c11 -D_DEFAULT_SOURCE

# Standalone host reproducers: each has its own main() and #includes malloc.c
# directly, so they cannot be linked into the unified utest binary (duplicate
# main / allocator symbols). They are slow O(n^2)-audit stress drivers meant to
# be run on demand via `make stress`, not as part of the routine suite.
STANDALONE = malloc_overlap_repro malloc_overlap_devbackend malloc_bins_test malloc_replay

# libc's sources written in yasc (../Makefile)
YASC_SRCS = $(shell sed -n 's/^YASC_SRCS *:*= *//p' ../Makefile)
# Their tests (NAME_tests.c tests ../NAME.c) are yasc as well, which gcc cannot
# build: `make qemu` runs them on QEMU (below), in tcc's safe default.
YASC_TESTS = $(patsubst %.c,%_tests,$(YASC_SRCS))

SRCS = $(filter-out $(addsuffix .c,$(STANDALONE) $(YASC_TESTS)) qemu_shim.c qemu_suite.sh, $(wildcard *.c))
OBJS = $(patsubst %.c, build/%.o, $(SRCS))


TARGET = build/libc_tests
# The SUT library is built for the HOST (CC=gcc). It must not share the device
# build tree: a mixed ARM/x86 `../build` produces a libc that links but is
# subtly wrong, which cost a day chasing a phantom nestfunc-2 HardFault.
LIBC_BUILD = ../build-host
LIBC_STATIC = $(LIBC_BUILD)/libc.a
LIBC_TEST_STATIC = build/libc_sut.a
SYMBOLS_REDEFINES = symbols_redefines.txt
LIBC_SRCS = $(wildcard ../*.c) $(wildcard ../*.h)

# Rules
all: $(TARGET) test

build/%.o: %.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_STATIC): $(LIBC_SRCS)
	$(MAKE) -C .. CC=$(CC) BUILD=$(notdir $(LIBC_BUILD))

$(LIBC_TEST_STATIC): $(LIBC_STATIC)
	mkdir -p build
	cp $(LIBC_STATIC) $(LIBC_TEST_STATIC)
	./rename_symbols.sh $(LIBC_TEST_STATIC)
$(TARGET): $(OBJS) $(LIBC_TEST_STATIC)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TARGET)
	./$(TARGET)

build_tests: $(TARGET)

# Standalone allocator stress reproducers (slow; run on demand).
build/%: %.c ../malloc.c
	mkdir -p build
	$(CC) $(CFLAGS) -O2 -DPUNCH_MIN_PAGES=1 $< -o $@

# The bins test again at the device's sizes (4-byte pointers and long).
build/malloc_bins_test32: malloc_bins_test.c ../malloc.c
	mkdir -p build
	$(CC) $(CFLAGS) -O2 -m32 -DTILING_SLACK=8 -DPUNCH_MIN_PAGES=1 $< -o $@

stress: $(addprefix build/, $(STANDALONE)) build/malloc_bins_test32
	@for t in $(addprefix build/, $(STANDALONE)) build/malloc_bins_test32; do \
	  echo "------------ $$t ------------"; ./$$t || exit 1; \
	done

# ---- the tests on QEMU -------------------------------------------------------
# Every *_tests.c against libc built by tcc for the target, bare metal on QEMU
# (qemu_suite.sh says how), at every level the rootfs could build libc at --
# the only run of the yasc sources' tests (YASC_TESTS), and the 32-bit target's
# answers for the rest.  Needs the cross, arm-none-eabi-gcc and qemu-system-arm.
# `make qemu-O2` runs one level; logs in build/qemu/<level>/.
TINYCC ?= $(abspath ../../tinycc)
YASOS_SYS_INCLUDE ?= $(abspath ../../../source/sys/include)
# the cross: build_rootfs.sh leaves the ARM native tcc at the tree's root and
# the host cross in bin/, a plain `make cross` the cross at the root
QEMU_CC ?= $(firstword $(foreach c,$(TINYCC)/armv8m-tcc $(TINYCC)/bin/armv8m-tcc,$(shell "$(c)" -v >/dev/null 2>&1 && echo "$(c)")))
QEMU_MACHINE = mps2-an505
QEMU_HARNESS = $(TINYCC)/tests/ir_tests/qemu/$(QEMU_MACHINE)
QEMU_OPTS ?= O0 O1 O2 Os
QEMU_TIMEOUT ?= 60
# 0: the toolchain's newlib multilib (nothing to build); 1: the harness's own
QEMU_NEWLIB_BUILD ?= 0
QEMU_ARCH = -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -fno-pic -mno-sb-relative-got
QEMU_BUILD = $(abspath build/qemu)
# Left out on QEMU: they check libc against the host's glibc (the same input
# must give the same answer), which bare-metal newlib cannot stand in for --
# its strptime is another implementation, its struct tm has no tm_gmtoff or
# tm_zone, it has no regcomp -- and tz_tests also serves the host's
# /usr/share/zoneinfo.
QEMU_SKIP = tz_tests.c strptime_tests.c regex_tests.c
# Built against libc's headers: their types and constants must be libc's
# (struct rlimit, which newlib does not have; O_CREAT, which it numbers apart)
QEMU_LIBC_HEADERS = resource_tests.c stdio_tests.c

qemu: $(addprefix qemu-,$(QEMU_OPTS))

qemu-check-cc:
	@test -n "$(QEMU_CC)" || { echo "no host-runnable armv8m-tcc in $(TINYCC) or $(TINYCC)/bin: build_rootfs.sh or make -C $(TINYCC) cross"; exit 1; }

qemu-%: qemu-check-cc
	@QEMU_CC="$(QEMU_CC)" TINYCC="$(TINYCC)" YASOS_SYS_INCLUDE="$(YASOS_SYS_INCLUDE)" \
	  QEMU_BUILD="$(QEMU_BUILD)" QEMU_HARNESS="$(QEMU_HARNESS)" QEMU_MACHINE="$(QEMU_MACHINE)" \
	  QEMU_ARCH="$(QEMU_ARCH)" QEMU_TIMEOUT="$(QEMU_TIMEOUT)" QEMU_NEWLIB_BUILD="$(QEMU_NEWLIB_BUILD)" \
	  YASC_SRCS="$(YASC_SRCS)" QEMU_SKIP="$(QEMU_SKIP)" QEMU_LIBC_HEADERS="$(QEMU_LIBC_HEADERS)" \
	  sh ./qemu_suite.sh $*

.PHONY: qemu qemu-check-cc

clean:
	rm -rf build
	$(MAKE) -C .. clean
