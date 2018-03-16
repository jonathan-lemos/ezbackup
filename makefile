NAME=ezbackup
CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c89 -D_XOPEN_SOURCE=500
CXX=clang
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11
LINKFLAGS=-lssl -lcrypto -lmenu -larchive -lncurses -lmega
DBGFLAGS=-g
CXXDBGFLAGS=-g
HEADERS=fileiterator maketar crypt readfile error checksum progressbar options checksumsort
CXXHEADERS=cloud/mega

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
CXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CXXDBGOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)+$(foreach cxxheader,$(CXXHEADERS),$(header).cpp.*);

release: main.o $(OBJECTS)
	$(CC) -o $(NAME)_unstripped main.o $(OBJECTS) $(CXXOBJECTS) $(CFLAGS) $(LINKFLAGS)
	cp $(NAME)_unstripped $(NAME)
	strip $(NAME)

debug: main.dbg.o $(DBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

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

%.cxx.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.cxx.dbg.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(CXXDBGFLAGS)

clean:
	rm -f *.o $(NAME) $(NAME)_unstripped $(CLEANOBJECTS) $(DBGOBJECTS) $(OBJECTS) main.c.* test.c.* vgcore.* test
