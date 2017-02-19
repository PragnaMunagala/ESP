CFLAGS = -Wall
parta: main.o
	gcc -o parta main.o -lpthread

main.o: main.c
	gcc ${CFLAGS} -c main.c
	
clean:
	rm -rf *.o *.ko parta
