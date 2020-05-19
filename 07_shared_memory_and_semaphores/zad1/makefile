CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c
		$(CC) -o receiver receiver.c
		$(CC) -o packer packer.c
		$(CC) -o sender sender.c

clean:
		rm main
		rm receiver
		rm packer
		rm sender

run:
		./main 5 3 3