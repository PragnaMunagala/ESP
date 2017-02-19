CFLAGS = -Wall

partb: main.o
	gcc -o partb main.o -lpthread

main.o: main.c
	gcc ${CFLAGS} -c main.c

clean:
	rm -rf *.o *.ko partb
