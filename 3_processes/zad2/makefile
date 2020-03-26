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

		./generator 5 35 35
		./matrix list.txt 30 10 common
# 		./matrix list.txt 2 3 separate
		./test in1.csv in2.csv out1.csv
		./test in3.csv in4.csv out2.csv
		./test in5.csv in6.csv out3.csv
		./test in7.csv in8.csv out4.csv
		./test in9.csv in10.csv out5.csv
		./test in11.csv in12.csv out6.csv
# 		./matrix list.txt 2 10 common
