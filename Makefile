# xbar build system

PREFIX  = /usr
CC      = clang
CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra -Wunused -Wwrite-strings `pkg-config --cflags x11`
LDFLAGS = `pkg-config --libs x11`
EXE     = xbar

OBJS    = xbar.o mod_say.o

.PHONY: clean install

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $(EXE) $(OBJS)

clean:
	rm -f *.o $(EXE)

install:
	install -m 755 $(EXE) $(PREFIX)/bin

.c.o:
	$(CC) $(CFLAGS) -c $<

include .depend
