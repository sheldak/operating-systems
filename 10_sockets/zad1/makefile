CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o server server.c -lpthread
		$(CC) -o client client.c -lpthread

clean:
		rm server
		rm client

runserver:
		./server 1234 HOME

run:
		./client one unix HOME
# 		./client two inet 127.0.0.1 1234
