# makefile -- builds the project
#
# Copyright (c) 2018 Jonathan Lemos
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.

NAME=ezbackup
CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c89 -D_XOPEN_SOURCE=500
CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11
LINKFLAGS=-lssl -lcrypto -lmenu -larchive -lncurses -lmega -lstdc++
DBGFLAGS=-g
CXXDBGFLAGS=-g
RELEASEFLAGS=-O3
CXXRELEASEFLAGS=-O3
HEADERS=fileiterator maketar crypt readfile error checksum progressbar options checksumsort
CXXHEADERS=cloud/mega
TESTS=tests/test_checksum.c
CXXTESTS=
TESTFLAGS=-rdynamic

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
CXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CXXDBGOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.dbg.o)
TESTOBJECTS=$(foreach test,$(TESTS),$(test).dbg.o)
TESTCXXOBJECTS=$(foreach cxxtest,$(CXXTESTS),$(cxxtest).cxx.dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)
CLEANCXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(header).cpp.*)

release: main.o $(OBJECTS) $(CXXOBJECTS)
	$(CC) -o $(NAME) main.o $(OBJECTS) $(CXXOBJECTS) $(CFLAGS) $(LINKFLAGS) $(RELEASEFLAGS)

debug: main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

test: tests/test_base.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(foreach test,$(test),$(CC) -o $(test) $(TESTOBJECTS) tests/test_base.dbg.o $(DBGOBJECTS) $(CFLAGS) $(CXXDBGOBJECTS) $(DBGFLAGS) $(LINKFLAGS) $(TESTFLAGS))

main.o: main.c
	$(CC) -c -o main.o main.c $(CFLAGS) $(RELEASEFLAGS)

main.dbg.o: main.c
	$(CC) -c -o main.dbg.o main.c $(CFLAGS) $(DBGFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(RELEASEFLAGS)

%.dbg.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DBGFLAGS)

%.cxx.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(RELEASEFLAGS)

%.cxx.dbg.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(CXXDBGFLAGS)

clean:
	rm -f *.o $(NAME) $(NAME)_unstripped $(CLEANOBJECTS) $(CLEANCXXOBJECTS) $(OBJECTS) $(CXXOBJECTS) $(DBGOBJECTS) $(CXXDBGOBJECTS) main.c.* test.c.* vgcore.* test
