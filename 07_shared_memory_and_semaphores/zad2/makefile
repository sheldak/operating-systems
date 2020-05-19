CC = gcc -Wall -std=c11 -D_DEFAULT_SOURCE -D_GNU_SOURCE

compile:
		$(CC) -o main main.c -pthread -lrt
		$(CC) -o receiver receiver.c -pthread -lrt
		$(CC) -o packer packer.c -pthread -lrt
		$(CC) -o sender sender.c -pthread -lrt

clean:
		rm main
		rm receiver
		rm packer
		rm sender

run:
		./main 5 3 3