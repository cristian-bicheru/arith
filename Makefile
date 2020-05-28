CC=g++ --std=c++11 -O2
HEADERS=arith.h huffman.h

all: codec

codec: codec.o
	$(CC) -o codec codec.o

codec.o: codec.cpp $(HEADERS)
	$(CC) -c codec.cpp

clean:
	rm -f codec codec.o
