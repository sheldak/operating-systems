CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c
		$(CC) -o producer producer.c
		$(CC) -o consumer consumer.c

clean:
		rm main
		rm producer
		rm consumer
		rm pipe

run:
		make compile

		./main 20