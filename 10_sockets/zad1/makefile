CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o server server.c -lpthread
		$(CC) -o client client.c -lpthread

clean:
		rm server
		rm client

runserver:
		./server 1234 HOME

run_one:
		./client one unix HOME

run_two:
		./client two inet 127.0.0.1 1234

run_three:
		./client three unix HOME

run_four:
		./client four inet 127.0.0.1 1234