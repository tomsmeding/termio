CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fwrapv -fPIC

# Set to /usr/local to install in the system directories
PREFIX = $(HOME)/prefix


# ---------------------------------------------------------

ifeq ($(PREFIX),)
	PREFIX = /usr/local
endif

SRC_FILES = $(wildcard *.c)
HEADER_FILES = $(wildcard *.h)
OBJECT_FILES = $(patsubst %.c,%.o,$(SRC_FILES))

UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	DYLIB_EXT = dylib
	DYLIB_FLAGS = -dynamiclib
else
	DYLIB_EXT = so
	DYLIB_FLAGS = -shared
endif


# Don't remove intermediate files
.SECONDARY:


.PHONY: all clean install uninstall remake reinstall dynamiclib staticlib

all: dynamiclib staticlib

clean:
	rm -f *.$(DYLIB_EXT) *.a *.o

install: all
	install libtermio.$(DYLIB_EXT) $(PREFIX)/lib
	install libtermio.a $(PREFIX)/lib
	install termio.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/libtermio.$(DYLIB_EXT)
	rm -f $(PREFIX)/lib/libtermio.a
	rm -f $(PREFIX)/include/termio.h

remake: clean all

reinstall: clean install

dynamiclib: libtermio.$(DYLIB_EXT)

staticlib: libtermio.a


%.o: %.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

%.$(DYLIB_EXT): $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) -o $@ $^

%.a: $(OBJECT_FILES)
	ar -cr $@ $^
