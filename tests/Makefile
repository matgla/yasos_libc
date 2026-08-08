CC = gcc
# _DEFAULT_SOURCE exposes the host glibc POSIX/BSD extensions (e.g. P_tmpdir)
# that strict -std=c11 otherwise hides, matching what the yasos libc headers
# provide unconditionally.
CFLAGS = -Wall -pedantic -g -std=c11 -D_DEFAULT_SOURCE

# Standalone host reproducers: each has its own main() and #includes malloc.c
# directly, so they cannot be linked into the unified utest binary (duplicate
# main / allocator symbols). They are slow O(n^2)-audit stress drivers meant to
# be run on demand via `make stress`, not as part of the routine suite.
STANDALONE = malloc_overlap_repro malloc_overlap_devbackend

SRCS = $(filter-out $(addsuffix .c,$(STANDALONE)), $(wildcard *.c))
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
build/%: %.c
	mkdir -p build
	$(CC) $(CFLAGS) -O2 $< -o $@

stress: $(addprefix build/, $(STANDALONE))
	@for t in $(addprefix build/, $(STANDALONE)); do \
	  echo "------------ $$t ------------"; ./$$t || exit 1; \
	done

clean:
	rm -rf build
	$(MAKE) -C .. clean
