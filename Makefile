CC=g++ --std=c++0x -O2
HEADERS=arith.h

all: codec

codec: codec.o
	$(CC) -o codec codec.o

codec.o: codec.cpp $(HEADERS)
	$(CC) -c codec.cpp

clean:
	rm -f codec codec.o
