CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c
		$(CC) -o sender sender.c
		$(CC) -o catcher catcher.c

clean:
		rm main
		rm sender
		rm catcher

run:
		make compile

		./main 1000 kill
		./main 1000 sigqueue
		./main 1000 sigrt