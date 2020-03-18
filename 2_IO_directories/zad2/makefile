CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE

compile:
		$(CC) -o find main.c

clean:
		rm find

tests:
# 		make clean
		make compile

# 		./find .
# 		./find a/
		./find . -mtime 0 -atime 0 -maxdepth 3
# 		./find . -maxdepth 3
# 		./find a -mtime +20 -atime 30

