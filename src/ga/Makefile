CC=g++
CXXFLAGS=-g
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a

all: main server

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS)

main: ga.h ga-utils.h socket.hpp common.hpp

server: server.o remote.o

server.o: common.hpp remote.hpp
remote.o: remote.hpp

clean:
	rm *.o main server
