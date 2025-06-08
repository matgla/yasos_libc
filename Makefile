CC ?= tcc 
CFLAGS = -std=c11 -Wall -Ilibs -gdwarf -fpic -pedantic -nostdlib -nostdinc -I. -I../../source/sys/include 
LDFLAGS_STATIC = -nostdlib -L../tinycc 
LDFLAGS = -shared -fPIC -gdwarf ${LDFLAGS_STATIC} 

ifeq ($(CC), armv8m-tcc)
CFLAGS += -DYASLIBC_ARM_SVC_TRIGGER
LDFLAGS += -Wl,-image-base=0x0 -Wl,-section-alignment=0x4 -larmv8m-libtcc1.a 
endif

SRCS = $(wildcard *.c)

OBJS = $(patsubst %.c, build/%.o, $(SRCS))

TARGET_SHARED = build/libc.so
TARGET_STATIC = build/libc.a

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

# Rules
all: $(TARGET_SHARED) $(TARGET_STATIC) $(TARGET_SHARED).elf

prepare: 
	mkdir -p build
	mkdir -p build/arm

build/%.o: %.c prepare
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_SHARED): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) -Wl,-oformat=yaff

$(TARGET_SHARED).elf: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) -Wl,-oformat=elf32-littlearm

$(TARGET_STATIC): $(OBJS)
	ar rcs $@ $^ ../tinycc/armv8m-libtcc1.a

build/arm/crt1.o: arm/crt1.c prepare
	${CC} $(CFLAGS) -c $< -o $@

build/arm/crti.o: arm/crti.c prepare
	$(CC) $(CFLAGS) -c $< -o $@

build/arm/crtn.o: arm/crtn.c prepare
	$(CC) $(CFLAGS) -c $< -o $@


install: $(TARGET_SHARED) $(TARGET_STATIC) build/arm/crt1.o build/arm/crti.o build/arm/crtn.o
	mkdir -p $(LIBDIR)
	cp $(TARGET_SHARED) $(LIBDIR)
	cp $(TARGET_STATIC) $(LIBDIR)
	cp build/arm/crt1.o $(LIBDIR)
	cp build/arm/crti.o $(LIBDIR)
	cp build/arm/crtn.o $(LIBDIR)

	cp *.h $(INCLUDEDIR)
	cp -r sys $(INCLUDEDIR)

clean:
	rm -rf build