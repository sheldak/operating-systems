CC = gcc -Wall -std=c11

compile:
		$(CC) -o main main.c

clean:
		rm main
		rm tmp*.txt

run:
		make compile

		./main generate tmp.txt 10000 1 copy tmp.txt tmpSys.txt 10000 1 sys copy tmp.txt tmpLib.txt 10000 1 lib sort tmpSys.txt 10000 1 sys sort tmpLib.txt 10000 1 lib
# 		./main generate tmp.txt 30000 1 copy tmp.txt tmpSys.txt 30000 1 sys copy tmp.txt tmpLib.txt 30000 1 lib sort tmpSys.txt 30000 1 sys sort tmpLib.txt 30000 1 lib
# 		./main generate tmp.txt 10000 4 copy tmp.txt tmpSys.txt 10000 4 sys copy tmp.txt tmpLib.txt 10000 4 lib sort tmpSys.txt 10000 4 sys sort tmpLib.txt 10000 4 lib
# 		./main generate tmp.txt 30000 4 copy tmp.txt tmpSys.txt 30000 4 sys copy tmp.txt tmpLib.txt 30000 4 lib sort tmpSys.txt 30000 4 sys sort tmpLib.txt 30000 4 lib
# 		./main generate tmp.txt 10000 512 copy tmp.txt tmpSys.txt 10000 512 sys copy tmp.txt tmpLib.txt 10000 512 lib sort tmpSys.txt 10000 512 sys sort tmpLib.txt 10000 512 lib
# 		./main generate tmp.txt 40000 512 copy tmp.txt tmpSys.txt 40000 512 sys copy tmp.txt tmpLib.txt 40000 512 lib sort tmpSys.txt 40000 512 sys sort tmpLib.txt 40000 512 lib
# 		./main generate tmp.txt 10000 1024 copy tmp.txt tmpSys.txt 10000 1024 sys copy tmp.txt tmpLib.txt 10000 1024 lib sort tmpSys.txt 10000 1024 sys sort tmpLib.txt 10000 1024 lib
# 		./main generate tmp.txt 40000 1024 copy tmp.txt tmpSys.txt 40000 1024 sys copy tmp.txt tmpLib.txt 40000 1024 lib sort tmpSys.txt 40000 1024 sys sort tmpLib.txt 40000 1024 lib
# 		./main generate tmp.txt 10000 2048 copy tmp.txt tmpSys.txt 10000 2048 sys copy tmp.txt tmpLib.txt 10000 2048 lib sort tmpSys.txt 10000 2048 sys sort tmpLib.txt 10000 2048 lib
# 		./main generate tmp.txt 20000 2048 copy tmp.txt tmpSys.txt 20000 2048 sys copy tmp.txt tmpLib.txt 20000 2048 lib sort tmpSys.txt 20000 2048 sys sort tmpLib.txt 20000 2048 lib
# 		./main generate tmp.txt 5000 4096 copy tmp.txt tmpSys.txt 5000 4096 sys copy tmp.txt tmpLib.txt 5000 4096 lib sort tmpSys.txt 5000 4096 sys sort tmpLib.txt 5000 4096 lib
# 		./main generate tmp.txt 10000 4096 copy tmp.txt tmpSys.txt 10000 4096 sys copy tmp.txt tmpLib.txt 10000 4096 lib sort tmpSys.txt 10000 4096 sys sort tmpLib.txt 10000 4096 lib
# 		./main generate tmp.txt 2500 8192 copy tmp.txt tmpSys.txt 2500 8192 sys copy tmp.txt tmpLib.txt 2500 8192 lib sort tmpSys.txt 2500 8192 sys sort tmpLib.txt 2500 8192 lib
# 		./main generate tmp.txt 5000 8192 copy tmp.txt tmpSys.txt 5000 8192 sys copy tmp.txt tmpLib.txt 5000 8192 lib sort tmpSys.txt 5000 8192 sys sort tmpLib.txt 5000 8192 lib