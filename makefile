# Makefile for ezbackup
# Copyright (C) 2018 Jonathan Lemos
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


NAME=ezbackup
CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c89 -D_XOPEN_SOURCE=500
CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11
LINKFLAGS=-lssl -lcrypto -lmenu -larchive -lncurses -lmega -lstdc++ -lreadline
DBGFLAGS=-g
CXXDBGFLAGS=-g
RELEASEFLAGS=-O3
CXXRELEASEFLAGS=-O3
HEADERS=fileiterator maketar crypt readfile error checksum progressbar options checksumsort
CXXHEADERS=cloud/mega

SOURCEFILES=$(foreach header,$(HEADERS),$(header).c)
OBJECTS=$(foreach header,$(HEADERS),$(header).o)
CXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.o)
DBGOBJECTS=$(foreach header,$(HEADERS),$(header).dbg.o)
CXXDBGOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(cxxheader).cxx.dbg.o)
CLEANOBJECTS=$(foreach header,$(HEADERS),$(header).c.*)
CLEANCXXOBJECTS=$(foreach cxxheader,$(CXXHEADERS),$(header).cpp.*)

release: main.o $(OBJECTS) $(CXXOBJECTS)
	$(CC) -o $(NAME) main.o $(OBJECTS) $(CXXOBJECTS) $(CFLAGS) $(LINKFLAGS) $(RELEASEFLAGS)

debug: main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(CC) -o $(NAME) main.dbg.o $(DBGOBJECTS) $(CXXDBGOBJECTS) $(CFLAGS) $(DBGFLAGS) $(LINKFLAGS)

test: test.c $(DBGOBJECTS) $(CXXDBGOBJECTS)
	$(CC) -o test test.c $(DBGOBJECTS) $(CFLAGS) $(CXXDBGOBJECTS) $(DBGFLAGS) $(LINKFLAGS)

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
