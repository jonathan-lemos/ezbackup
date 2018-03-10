NAME=cbackup
CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c89 -D_XOPEN_SOURCE=500
LINKFLAGS=-lssl -lcrypto -lmenu -larchive -lncurses
DBGFLAGS=-g
HEADERS=fileiterator maketar crypt readfile error checksum progressbar options checksumsort

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)

release: main.o $(OBJECTS)
	$(CC) -o $(NAME) main.o $(OBJECTS) $(CFLAGS) $(LINKFLAGS)
	strip $(NAME)

debug: main.dbg.o $(DBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

test: test.c $(DBGOBJECTS)
	$(CC) -o test test.c $(DBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

main.o: main.c
	$(CC) -c -o main.o main.c $(CFLAGS)

main.dbg.o: main.c
	$(CC) -c -o main.dbg.o main.c $(CFLAGS) $(DBGFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.dbg.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DBGFLAGS)

clean:
	rm -f *.o $(NAME) $(CLEANOBJECTS) $(DBGOBJECTS) $(OBJECTS) main.c.* test.c.* vgcore.* test
