CC = gcc -Wall -std=c11 -ggdb

all: clean static shared

static:
	$(CC) -c library.c -o library.o
	ar rcs liblibrary.a library.o

shared:
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o liblibrary.so library.o

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
