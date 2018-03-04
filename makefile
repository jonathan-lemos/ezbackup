NAME=cbackup
CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c89 -larchive -lssl -lcrypto -D_XOPEN_SOURCE=500 -lmenu -lncurses
DBGFLAGS=-g -da -Q
OBJECTS=fileiterator.o maketar.o crypt.o readfile.o evperror.o checksum.o progressbar.o options.o
DBGOBJECTS=fileiterator.dbg.o maketar.dbg.o crypt.dbg.o readfile.dbg.o evperror.dbg.o checksum.dbg.o progressbar.dbg.o options.dbg.o

all: main.o $(OBJECTS)
	$(CC) -o $(NAME) main.o $(OBJECTS) $(CFLAGS)

debug: main.dbg.o $(DBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CFLAGS) $(DBGFLAGS)

test: $(DBGOBJECTS)
	$(CC) -o test test.c $(DBGOBJECTS) $(CFLAGS) $(DBGFLAGS)

main.o: main.c
	$(CC) -c -o main.o main.c $(CFLAGS)

main.dbg.o: main.c
	$(CC) -c -o main.dbg.o main.c $(CFLAGS) $(DBGFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.dbg.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DBGFLAGS)

clean:
	rm -f *.o $(NAME) maketar.c.* main.c.* fileiterator.c.* test.c.* test crypt.c.* readfile.c.* evperror.c.* checksum.c.* vgcore.* progressbar.c.* options.c.*
