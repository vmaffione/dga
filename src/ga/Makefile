CC=g++
CXXFLAGS=-g
ARCHBITS=$(shell getconf LONG_BIT)
OBJS=main.o socket.o stoc1.o randomaelf$(ARCHBITS).a

main: $(OBJS)
	$(CC) -o $@ $(OBJS)

main: ga.h ga-utils.h socket.hpp

clean:
	rm *.o main
