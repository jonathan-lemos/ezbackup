NAME=cbackup
CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c89 -larchive -lssl -lcrypto -D_XOPEN_SOURCE=500 -lmenu -lncurses
DBGFLAGS=-g -da
HEADERS=fileiterator maketar crypt readfile evperror checksum progressbar options

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)

release: main.o $(OBJECTS)
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
	rm -f *.o $(NAME) $(CLEANOBJECTS) main.c.* test.c.* vgcore.*
