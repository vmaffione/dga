CC=g++
CXXFLAGS=-g -Wall -D DBG=1
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o stoc1.o randomaelf$(ARCHBITS).a peer-server.o remote.o protocol.o ga.o ga-utils.o
EXES=main

all: $(EXES)

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS) -lpthread

main.o: ga.hpp ga-utils.hpp peer-server.hpp

peer-server.o: remote.hpp protocol.hpp

remote.o: remote.hpp

ga.o: ga.hpp ga-utils.hpp

ga-utils.o: ga-utils.hpp

clean:
	rm *.o $(EXES)
