CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o server server.c
		$(CC) -o client client.c

clean:
		rm server
		rm client

runserver:
		./server

runclient:
		./client