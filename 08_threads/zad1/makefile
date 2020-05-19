CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c -lpthread

clean:
		rm main

run:
		./main 1 sign brain.pgm histogram.txt
		./main 2 sign brain.pgm histogram.txt
		./main 4 sign brain.pgm histogram.txt
		./main 8 sign brain.pgm histogram.txt
		./main 1 block brain.pgm histogram.txt
		./main 2 block brain.pgm histogram.txt
		./main 4 block brain.pgm histogram.txt
		./main 8 block brain.pgm histogram.txt
		./main 1 interleaved brain.pgm histogram.txt
		./main 2 interleaved brain.pgm histogram.txt
		./main 4 interleaved brain.pgm histogram.txt
		./main 8 interleaved brain.pgm histogram.txt
