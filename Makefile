CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fwrapv

# Set to /usr/local to install in the system directories
PREFIX = $(HOME)/prefix


# ---------------------------------------------------------

NAME = termio

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
	DYLIB_FLAGS = -shared -fPIC
endif


.PHONY: all clean install remake reinstall dynamiclib staticlib

all: dynamiclib staticlib

clean:
	rm -f *.$(DYLIB_EXT) *.a *.o

install: all
	install $(NAME).$(DYLIB_EXT) $(PREFIX)/lib
	install $(NAME).a $(PREFIX)/lib
	install $(NAME).h $(PREFIX)/include

remake: clean all

reinstall: clean install

dynamiclib: $(NAME).$(DYLIB_EXT)

staticlib: $(NAME).a


%.o: %.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

%.$(DYLIB_EXT): $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) -o $@ $^

%.a: $(OBJECT_FILES)
	ar -cr $@ $^
