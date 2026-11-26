CC ?= gcc
CFLAGS = -Wall -Werror -Wno-nonnull-compare -Ilibs $(ROOTFS_DEBUG_CFLAGS) -fPIC -pedantic -nostdlib -nostdinc -Iinclude -I../../source/sys/include -I../tinycc/include
CFLAGS += $(ROOTFS_OPT_CFLAGS)
# libc's own sources are never fortified (sys/_fortify.h): they define the
# functions the fortified headers turn into macros
CFLAGS += -U_FORTIFY_SOURCE
# nor do they get the C23-style const-keeping strto* macros of <stdlib.h>,
# for the same reason
CFLAGS += -D__YL_LIBC_SOURCE
LDFLAGS_STATIC = -nostdlib $(ROOTFS_DEBUG_CFLAGS) -L../tinycc
LDFLAGS = -shared -fPIC ${LDFLAGS_STATIC}
LDFLAGS_ELF = $(LDFLAGS) -rdynamic
EXTERNAL_LIBS =
ifeq ($(CC), armv8m-tcc)
CFLAGS += -DYASLIBC_ARM_SVC_TRIGGER
# RELRO: keep .rodata pure-const (pointer-bearing const objects go to the
# writable data segment) so it can be shared XIP across processes.
CFLAGS += -share-rodata
LDFLAGS_STATIC += -Wl,-Ttext=0x0
LDFLAGS += -larmv8m-libtcc1.a
# What runs from a shared copy in SRAM (libs/tinycc/docs/hot_text.md).
CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--hot-text=hot_functions.txt
SRCS = arm/setjmp.S arm/vfork.S arm/call_with_got.S
LDFLAGS_ELF += -Wl,-oformat=elf32-littlearm -Wl,-Ttext=0x0
ARM_BUILD = y
EXTERNAL_LIBS += ../tinycc/armv8m-libtcc1.a
# The crt objects are linked into every program, not into libc.so, so their
# calls into libc (exit) are imports there. tcc decides which calls keep r9
# from the shared libraries installed when it compiles, and while libc itself
# is being built its own libc.so may not be installed yet: without this, exit
# is taken for a module-local call and every program link is refused.
CRT_CFLAGS = -fno-module-local-calls
# Each object's call graph and frames (<object>.ci, tinycc's -fcallgraph-info),
# gathered into build/libc.ci for tinycc's scripts/stack_usage.py: a
# program's worst stack path through libc.  Host-side data: not installed, so
# it never reaches a target image.  The objects are the same with and
# without it.  CALLGRAPH_INFO= turns it off.
CALLGRAPH_INFO ?= -fcallgraph-info=su
CFLAGS += $(CALLGRAPH_INFO)
else
CFLAGS += -Wno-pointer-arith -Wno-builtin-declaration-mismatch
LDFLAGS_STATIC += -Wl,--build-id=none -Wl,--no-eh-frame-hdr
endif


SRCS += $(wildcard *.c) $(wildcard sys/*.c) $(wildcard arpa/*.c)

# Sources written in yasc (`#pragma yasc`: slices, ownership): only tinycc
# builds them, so the host unit tests' gcc build leaves them out.
YASC_SRCS := libgen.c assert.c atoi.c byteswap.c complex.c ctype.c dirent.c dl.c fcntl.c
ifneq ($(ARM_BUILD),y)
SRCS := $(filter-out $(YASC_SRCS),$(SRCS))
endif

# Output directory. Overridable so a build for a *different* architecture does
# not land on top of this one: the host unit tests (tests/) invoke this Makefile
# with CC=gcc, and while that shared a single `build/` with the device rootfs
# build it silently mixed x86 objects into the ARM tree. The symptom is not a
# clean failure -- it is a rootfs that mostly works, which is far worse to
# debug than a link error.
BUILD ?= build


OBJS = $(patsubst %.c, $(BUILD)/%.o, $(SRCS))

# Rebuild everything when any header changes.
#
# Bluntly, rather than with -MMD: this Makefile is driven by two different
# compilers (gcc for the host unit tests, armv8m-tcc for the rootfs) and a
# whole-libc rebuild costs seconds, so precision buys nothing and portability
# costs nothing. Without it, a change to a *type* -- `time_t` narrowing to
# `long`, say -- leaves objects behind that still use the old layout, and the
# result is not a link error but a libc whose `localtime` reads eight bytes out
# of a four-byte field. That failure surfaces as wrong dates in `ls -l`, half a
# system away from the header that caused it.
# The header rule below is the first rule in the file; without this it would be
# the default goal, and a plain `make` would build one object and stop.
.DEFAULT_GOAL := all

# The public headers are include/<header>.  The ones written in yasc
# (include/yasc/<header>, one source) and the C headers tinycc writes from
# them (include/<header>, committed: gcc, clang and the kernel read them as
# they are).  `make headers` after editing a yasc one; CI runs
# `make headers-check`.  tinycc docs/yasc_c_headers.md.
YASC_HEADERS := $(patsubst include/yasc/%,%,$(wildcard include/yasc/*.h) $(wildcard include/yasc/*/*.h) $(wildcard include/yasc/*/*/*.h))
# a cross this host runs (after build_rootfs.sh, ../tinycc/armv8m-tcc is the native ARM one)
YASC_TCC ?= $(firstword $(foreach c,../tinycc/bin/armv8m-tcc ../tinycc/armv8m-tcc,$(shell "$(c)" -v >/dev/null 2>&1 && echo "$(c)")))

headers:
	@test -n "$(YASC_TCC)" || { echo "no armv8m-tcc in ../tinycc or ../tinycc/bin (YASC_TCC=)"; exit 1; }
	@for h in $(YASC_HEADERS); do $(YASC_TCC) -yasc-c-header include/yasc/$$h -o include/$$h || exit 1; done

headers-check:
	@test -n "$(YASC_TCC)" || { echo "no armv8m-tcc in ../tinycc or ../tinycc/bin (YASC_TCC=)"; exit 1; }
	@mkdir -p $(BUILD); fail=0; for h in $(YASC_HEADERS); do \
	  $(YASC_TCC) -yasc-c-header include/yasc/$$h > $(BUILD)/.header-check.h || exit 1; \
	  cmp -s $(BUILD)/.header-check.h include/$$h || { echo "include/$$h is not what include/yasc/$$h gives: make headers"; fail=1; }; \
	done; rm -f $(BUILD)/.header-check.h; exit $$fail

.PHONY: headers headers-check

HEADERS := $(wildcard include/*.h include/*/*.h include/*/*/*.h include/*/*/*/*.h) \
           $(wildcard internal/*.h arm/*.h libs/*.h)
$(filter %.o, $(OBJS)): $(HEADERS)

TARGET_SHARED = $(BUILD)/libc.so
TARGET_STATIC = $(BUILD)/libc.a
TARGET_CI = $(if $(and $(ARM_BUILD),$(CALLGRAPH_INFO)),$(BUILD)/libc.ci)

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

# Rules
all: $(TARGET_SHARED) $(TARGET_STATIC) $(TARGET_SHARED).elf $(TARGET_CI)

prepare:
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/arm
	mkdir -p $(BUILD)/sys
	mkdir -p $(BUILD)/arpa

$(BUILD)/%.o: %.c | prepare
	$(CC) $(CFLAGS) -c $< -o $@

# The yasc sources build in tcc's safe default (every check, unsafe C an
# error), not -ccompat: build_rootfs.sh hands every libc compile -ccompat and
# tcc has no option that takes it back, so it is filtered out here.  Same code
# as under -ccompat; only the checks are added.
$(patsubst %.c,$(BUILD)/%.o,$(YASC_SRCS)): $(BUILD)/%.o: %.c | prepare
	$(CC) $(filter-out -ccompat,$(CFLAGS)) -c $< -o $@

$(TARGET_SHARED): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SHARED).elf: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS_ELF)

$(TARGET_STATIC): $(OBJS)
	ar rcs $@ $^ $(EXTERNAL_LIBS)

# one `object NAME` section per member, as tinycc's armv8m-libtcc1.ci
$(BUILD)/libc.ci: $(OBJS)
	for o in $(patsubst %.o,%.ci,$(filter %.o,$(OBJS))); do m=$${o#$(BUILD)/}; echo "object $${m%.ci}.o"; cat $$o; done > $@

$(BUILD)/arm/crt1.o $(BUILD)/arm/crti.o $(BUILD)/arm/crtn.o: $(HEADERS)

$(BUILD)/arm/crt1.o: arm/crt1.c | prepare
	${CC} $(CFLAGS) $(CRT_CFLAGS) -c $< -o $@

$(BUILD)/arm/crti.o: arm/crti.c | prepare
	$(CC) $(CFLAGS) $(CRT_CFLAGS) -c $< -o $@

$(BUILD)/arm/crtn.o: arm/crtn.c | prepare
	$(CC) $(CFLAGS) $(CRT_CFLAGS) -c $< -o $@


install: $(TARGET_SHARED) $(TARGET_STATIC) $(BUILD)/arm/crt1.o $(BUILD)/arm/crti.o $(BUILD)/arm/crtn.o
	mkdir -p $(LIBDIR)
	mkdir -p $(INCLUDEDIR)
	cp $(TARGET_SHARED) $(LIBDIR)
	cp $(TARGET_STATIC) $(LIBDIR)
	cp $(BUILD)/arm/crt1.o $(LIBDIR)
	cp $(BUILD)/arm/crti.o $(LIBDIR)
	cp $(BUILD)/arm/crtn.o $(LIBDIR)
	cp -r include/. $(INCLUDEDIR)
	cp ../tinycc/include/*.h $(INCLUDEDIR)

clean:
	rm -rf build

test:
	$(MAKE) -C tests

build_tests:
	$(MAKE) -C tests build_tests
