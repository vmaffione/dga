CC=g++
CXXFLAGS=-g
ARCHBITS=$(shell getconf LONG_BIT)
MAIN_OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a
EXES=main manager proxy

all: $(EXES)

main: $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS)

main: ga.h ga-utils.h socket.hpp common.hpp

manager: manager.o remote.o

proxy: proxy.o remote.o

manager.o: common.hpp remote.hpp member.hpp
proxy.o: common.hpp remote.hpp member.hpp
remote.o: remote.hpp

clean:
	rm *.o $(EXES)
