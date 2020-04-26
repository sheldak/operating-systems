CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c -lrt
		$(CC) -o receiver receiver.c -lrt
		$(CC) -o packer packer.c -lrt
		$(CC) -o sender sender.c -lrt

clean:
		rm main
		rm receiver
		rm packer
		rm sender

run:
		./main 1 1 1