CC=g++
CXXFLAGS=-g
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a

all: main server

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS)

main: ga.h ga-utils.h socket.hpp

server: server.o

clean:
	rm *.o main server
