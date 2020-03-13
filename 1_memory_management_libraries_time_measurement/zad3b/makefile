CC = gcc -Wall -std=c11 -ggdb
Opt ?= 0
all: static shared dynamic

static:
	make clean
	$(CC) -c library.c
	ar rcs liblibrary.a library.o
	$(CC) -o test main.c liblibrary.a -I . -O$(Opt)
	make tests

shared:
	make clean
	$(CC) -fPIC -c library.c
	$(CC) -shared -fPIC -o liblibrary.so library.o
	$(CC) main.c -o test -L. -llibrary -Wl,-rpath=`pwd` -O$(Opt)
	make tests

dynamic:
	make clean
	$(CC) -fPIC -c library.c -o library.o
	$(CC) -Wl,-rpath=. -fPIC -shared -o liblibrary.so library.o
	$(CC) -fPIC -L. -o test main_dynamic.c -ldl -D DYNAMIC -O$(Opt)
	make tests

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -f test
	rm -f tmp*.txt

tests:
	@echo --- Small ---
	./test create_table 3 compare_pairs a.txt:b.txt c.txt:d.txt e.txt:f.txt remove_block 0 remove_block 1 remove_operation 2 0 remove_block 2
	./test create_table 5 compare_pairs a.txt:a.txt b.txt:b.txt c.txt:c.txt d.txt:d.txt f.txt:f.txt remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4

	@echo --- Medium ---
	./test create_table 8 compare_pairs m_a.txt:m_b.txt m_b.txt:m_c.txt m_c.txt:m_d.txt m_d.txt:m_e.txt m_a.txt:m_c.txt m_a.txt:m_d.txt m_a.txt:m_e.txt m_c.txt:m_e.txt remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 remove_block 6 remove_block 7

	@echo --- Large ---
	./test create_table 12 compare_pairs m_a.txt:m_b.txt m_b.txt:m_c.txt m_c.txt:m_d.txt m_d.txt:m_e.txt m_e.txt:m_f.txt m_a.txt:m_c.txt m_a.txt:m_d.txt m_a.txt:m_e.txt m_a.txt:m_f.txt m_b.txt:m_d.txt m_b.txt:m_e.txt m_b.txt:m_f.txt m_c.txt:m_e.txt remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 remove_block 6 remove_block 7 remove_block 8 remove_block 9 remove_block 10 remove_block 11
	./test create_table 15 compare_pairs m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt m_g.txt:m_h.txt remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5
