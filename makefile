# makefile -- builds the project
#
# Copyright (c) 2018 Jonathan Lemos
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.

NAME=ezbackup
VERSION=0.2\ beta
CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -pedantic -std=c89 -D_XOPEN_SOURCE=500 -DPROG_NAME=\"$(NAME)\" -DPROG_VERSION=\"$(VERSION)\"
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11 -DPROG_NAME=\"$(NAME)\" -DPROG_VERSION=\"$(VERSION)\"
LINKFLAGS=-lssl -lcrypto -lmenu -larchive -lncurses -lmega -lstdc++ -ledit
DBGFLAGS=-g -rdynamic
CXXDBGFLAGS=-g -rdynamic
RELEASEFLAGS=-O3
CXXRELEASEFLAGS=-O3
DIRECTORIES=. cloud crypt
# HEADERS=fileiterator maketar crypt readfile error checksum progressbar options checksumsort
HEADERS=$(foreach directory,$(DIRECTORIES),$(shell ls $(directory) | grep .*\\.c$$ | sed 's|\(.*\)\.c$$|$(directory)/\1|g;s|.*main||g'))
CXXHEADERS=$(foreach directory,$(DIRECTORIES),$(shell ls $(directory) | grep .*\\.cpp$$ | sed 's|\(.*\)\.cpp$$|$(directory)/\1|g'))
# TESTS=tests/test_checksum tests/test_crypt tests/test_error tests/test_fileiterator tests/test_maketar tests/test_options tests/test_progressbar
TESTS=$(foreach test,$(shell ls tests | grep .*\\.c$$ | sed 's/\.c//g;s/test_base//g'),tests/$(test))
CXXTESTS=$(foreach test,$(shell ls tests | grep .*\\.cpp$$ | sed 's/\.cpp//g;s/test_base//g'),tests/$(test))

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
CXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CXXDBGOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.dbg.o)
TESTOBJECTS=$(foreach test,$(TESTS),$(test).dbg.o)
TESTCXXOBJECTS=$(foreach cxxtest,$(CXXTESTS),$(cxxtest).cxx.dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)
CLEANCXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(header).cpp.*)
TESTEXECS=$(foreach test,$(TESTS),$(test).x)

release: main.o $(OBJECTS) $(CXXOBJECTS)
	$(CC) -o $(NAME) main.o $(OBJECTS) $(CXXOBJECTS) $(CFLAGS) $(LINKFLAGS) $(RELEASEFLAGS)

debug: main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

test: $(TESTEXECS) $(TESTOBJECTS) $(DBGOBJECTS) $(CXXDBGOBJECTS) tests/test_base.dbg.o
	@echo "Made all tests"

%.x: %.dbg.o tests/test_base.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(CC) -o $@ $< tests/test_base.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(RELEASEFLAGS)

%.dbg.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DBGFLAGS)

%.cxx.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(RELEASEFLAGS)

%.cxx.dbg.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(CXXDBGFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(NAME) $(CLEANOBJECTS) $(CLEANCXXOBJECTS) main.c.* vgcore.* $(TESTOBJECTS) $(TESTCXXOBJECTS) tests/*.o cloud/*.o $(TESTEXECS)

.PHONY: linecount
linecount:
	wc -l makefile readme.txt $(foreach dir,$(DIRECTORIES),$(dir)/*.c $(dir)/*.cpp) tests/*.c tests/*.cpp

.PHONY: linecount_notests
linecount_notests:
	wc -l makefile readme.txt $(foreach dir,$(DIRECTORIES),$(dir)/*.c $(dir)/*.cpp)
