all: main

main: flush_reload.c
	gcc -static -Os flush_reload.c -c
test: test.c
	gcc -static -Os test.c flush_reload.o -o test
