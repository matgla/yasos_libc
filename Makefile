CC ?= tcc 
CFLAGS = -std=c11 -Wall -Ilibs -gdwarf -fpic -pedantic -nostdlib -nostdinc -I. -I../../source/sys/include -I../tinycc/include 
LDFLAGS_STATIC = -nostdlib -L../tinycc 
LDFLAGS = -shared -fPIC -gdwarf ${LDFLAGS_STATIC} 

ifeq ($(CC), armv8m-tcc)
CFLAGS += -DYASLIBC_ARM_SVC_TRIGGER
LDFLAGS += -Wl,-image-base=0x0 -Wl,-section-alignment=0x4 -larmv8m-libtcc1.a 
endif

LDFLAGS_ELF = $(LDFLAGS) -Wl,-oformat=elf32-littlearm
ifeq ($(CC), armv8m-tcc)
LDFLAGS_ELF += -Wl,-oformat=elf32-littlearm
endif



SRCS = $(wildcard *.c) $(wildcard sys/*.c) $(wildcard arpa/*.c) arm/setjmp.S

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
	mkdir -p build/sys
	mkdir -p build/arpa

build/%.o: %.c prepare
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_SHARED): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SHARED).elf: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS_ELF)

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
	cp ../tinycc/include/*.h $(INCLUDEDIR)
	cp -r sys $(INCLUDEDIR)
	cp -r arpa $(INCLUDEDIR)
	cp -r net $(INCLUDEDIR)
	cp -r netinet $(INCLUDEDIR)

clean:
	rm -rf build

test: 
	$(MAKE) -C tests