CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o matrix main.c
		$(CC) -o generator generator.c
		$(CC) -o test test.c

clean:
		rm matrix
		rm generator
		rm test
		rm matrices/tmp*

run:
		make compile

		./generator 5 16 16
		./matrix list.txt 15 3 common
# 		./matrix list.txt 2 3 separate
		./test in1.csv in2.csv out1.csv
