CC=g++
CXXFLAGS=-g -Wall
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a
EXES=main peer-server

all: $(EXES)

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS)

main: ga.h ga-utils.h socket.hpp common.hpp

peer-server: peer-server.o remote.o protocol.o
	$(CC) -o $@ $@.o remote.o protocol.o -lpthread

peer-server.o: remote.hpp protocol.hpp
remote.o: remote.hpp

clean:
	rm *.o $(EXES)
