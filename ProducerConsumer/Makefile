process: process.o my402list.o threads.o 
	gcc -o process -g process.o my402list.o threads.o -pthread -lm 
process.o: process.c defs.h
	gcc -g -c -Wall process.c
	
threads.o: threads.c defs.h
	gcc -g -c -Wall threads.c -D_POSIX_PTHREAD_SEMANTICS

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c 
	
clean:
	rm -f *.o process
 
