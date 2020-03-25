CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c

clean:
		rm main

run:
		rm matrices/tmp*
		make compile

		./main .
		./main a
