all: test

test: test.c
	gcc -ggdb -Wall -o test test.c

objdump: test
	objdump -W test > test.objdump

clean:
	rm -fv test
