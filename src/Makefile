CC=g++
CXXFLAGS=-g -Wall
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a peer-server.o remote.o protocol.o ga.o ga-utils.o
EXES=main

all: $(EXES)

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS) -lpthread

main.o: ga.h ga-utils.h socket.hpp peer-server.hpp

peer-server.o: remote.hpp protocol.hpp

remote.o: remote.hpp

ga.o: ga.h ga-utils.h

ga-utils.o: ga-utils.h

clean:
	rm *.o $(EXES)
