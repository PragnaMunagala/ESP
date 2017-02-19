CFLAGS = -Wall
	
partd: main.o
	gcc -o partd main.o -lpthread

main.o: main.c
	gcc ${CFLAGS} -c main.c
	
clean:
	rm -rf *.o *.ko partd 
