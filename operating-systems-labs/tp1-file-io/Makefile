
main.o: options.o digest.o
	gcc options.o digest.o main.c -o hash -lssl -lcrypto

options.o: options.h options.c
	gcc -c -o options.o options.c -Wall

digest.o: digest.h digest.c
	gcc -c -o digest.o digest.c -Wall

