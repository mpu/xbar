# xbar build system

PREFIX  = /usr
CC      = clang
CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra -Wunused -Wwrite-strings `pkg-config --cflags x11`
LDFLAGS = `pkg-config --libs x11` -pthreads
EXE     = xbar

OBJS    = xbar.o mod_say.o mod_time.o mod_cpu.o

.PHONY: clean install depend

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $(EXE) $(OBJS)

clean:
	rm -f *.o $(EXE)

install:
	install -m 755 $(EXE) $(PREFIX)/bin

.c.o:
	$(CC) $(CFLAGS) -c $<

depend:
	$(CC) -MM *.c > .depend

include .depend
