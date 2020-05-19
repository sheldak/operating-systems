CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o server server.c -lrt
		$(CC) -o client client.c -lrt

clean:
		rm server
		rm client

runserver:
		./server

runclient:
		./client