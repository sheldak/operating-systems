CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o find main.c

clean:
		rm find

tests:
		make compile

		./find . -mtime +2 -atime -1 -maxdepth 3
		./find . -mtime +2 -atime -1 -maxdepth 3 nftw
		./find a -maxdepth 0
		./find a -maxdepth 0 nftw
		./find a -maxdepth 1
		./find a -maxdepth 1 nftw
		./find a/b -mtime 1
		./find a/c.txt nftw
		./find a -atime 1

