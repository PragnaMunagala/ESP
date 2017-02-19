CFLAGS = -Wall
final: main.o sig_lib.o
	gcc -o part2 main.o sig_lib.o -lpthread

main.o: main.c sig_lib.h
	gcc ${CFLAGS} -c main.c

sig_lib.o: sig_lib.c sig_lib.h
	gcc ${CFLAGS} -c sig_lib.c
	
clean:
	rm -rf *.o *.ko part2
