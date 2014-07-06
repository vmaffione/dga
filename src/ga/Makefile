CC=g++
CXXFLAGS=-g
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a
EXES=main manager proxy

all: $(EXES)

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS)

main: ga.h ga-utils.h socket.hpp common.hpp

manager: manager.o remote.o protocol.o

proxy: proxy.o remote.o protocol.o
	$(CC) -o $@ $@.o remote.o protocol.o -lpthread

manager.o: remote.hpp protocol.hpp
proxy.o: remote.hpp protocol.hpp
remote.o: remote.hpp

clean:
	rm *.o $(EXES)
