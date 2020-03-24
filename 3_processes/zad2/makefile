CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o matrix main.c

clean:
		rm matrix

run:
		make compile

		./matrix list.txt 2 3 common
# 		./matrix list.txt 2 3 separate
