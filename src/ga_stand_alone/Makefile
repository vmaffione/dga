CC=g++
BIN=test
OBJS=main.o stoc1.cpp randomaelf64.a

all: test

$(BIN): $(OBJS)
	$(CC) -o test $(OBJS)

main.o: pga.h pga_error.h pga_utils.h

clean:
	-rm *.o $(BIN)
