CFLAGS = -Wall
final: main.o
	gcc -o part1 main.o -lpthread

main.o: main.c
	gcc ${CFLAGS} -c main.c
	
clean:
	rm -rf *.o *.ko part1
