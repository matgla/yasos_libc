CC ?= gcc
CFLAGS = -Wall -Werror -Wno-nonnull-compare -Ilibs $(ROOTFS_DEBUG_CFLAGS) -fPIC -pedantic -nostdlib -nostdinc -I. -I../../source/sys/include -I../tinycc/include
CFLAGS += $(ROOTFS_OPT_CFLAGS)
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
SRCS = arm/setjmp.S arm/vfork.S arm/call_with_got.S
LDFLAGS_ELF += -Wl,-oformat=elf32-littlearm -Wl,-Ttext=0x0
ARM_BUILD = y
EXTERNAL_LIBS += ../tinycc/armv8m-libtcc1.a
else
CFLAGS += -Wno-pointer-arith -Wno-builtin-declaration-mismatch
LDFLAGS_STATIC += -Wl,--build-id=none -Wl,--no-eh-frame-hdr
endif


SRCS += $(wildcard *.c) $(wildcard sys/*.c) $(wildcard arpa/*.c)

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
HEADERS := $(wildcard *.h) $(wildcard sys/*.h) $(wildcard arpa/*.h) \
           $(wildcard arm/*.h) $(wildcard libs/*.h)
$(filter %.o, $(OBJS)): $(HEADERS)

TARGET_SHARED = $(BUILD)/libc.so
TARGET_STATIC = $(BUILD)/libc.a

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

# Rules
all: $(TARGET_SHARED) $(TARGET_STATIC) $(TARGET_SHARED).elf

prepare:
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/arm
	mkdir -p $(BUILD)/sys
	mkdir -p $(BUILD)/arpa

$(BUILD)/%.o: %.c | prepare
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_SHARED): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SHARED).elf: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS_ELF)

$(TARGET_STATIC): $(OBJS)
	ar rcs $@ $^ $(EXTERNAL_LIBS)

$(BUILD)/arm/crt1.o $(BUILD)/arm/crti.o $(BUILD)/arm/crtn.o: $(HEADERS)

$(BUILD)/arm/crt1.o: arm/crt1.c | prepare
	${CC} $(CFLAGS) -c $< -o $@

$(BUILD)/arm/crti.o: arm/crti.c | prepare
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/arm/crtn.o: arm/crtn.c | prepare
	$(CC) $(CFLAGS) -c $< -o $@


install: $(TARGET_SHARED) $(TARGET_STATIC) $(BUILD)/arm/crt1.o $(BUILD)/arm/crti.o $(BUILD)/arm/crtn.o
	mkdir -p $(LIBDIR)
	mkdir -p $(INCLUDEDIR)
	cp $(TARGET_SHARED) $(LIBDIR)
	cp $(TARGET_STATIC) $(LIBDIR)
	cp $(BUILD)/arm/crt1.o $(LIBDIR)
	cp $(BUILD)/arm/crti.o $(LIBDIR)
	cp $(BUILD)/arm/crtn.o $(LIBDIR)
	cp *.h $(INCLUDEDIR)
	cp ../tinycc/include/*.h $(INCLUDEDIR)
	cp -r sys $(INCLUDEDIR)
	cp -r arpa $(INCLUDEDIR)
	cp -r net $(INCLUDEDIR)
	cp -r netinet $(INCLUDEDIR)

clean:
	rm -rf build

test:
	$(MAKE) -C tests

build_tests:
	$(MAKE) -C tests build_tests
