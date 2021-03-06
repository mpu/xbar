# xbar build system

PREFIX  = /usr
CC      = clang
CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra -Wunused -Wwrite-strings `pkg-config --cflags x11`
LDFLAGS = `pkg-config --libs x11`
EXE     = xbar

OBJS    = xbar.o mod_say.o mod_time.o mod_cpu.o mod_bat.o mod_cmd.o mod_read.o

.PHONY: clean install depend

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $(EXE) $(OBJS)

config.h:
	cp config.h.def config.h

clean:
	rm -f *.o $(EXE)

install:
	install -m 755 $(EXE) $(PREFIX)/bin

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: config.h
	$(CC) -MM *.c > .depend

include .depend
