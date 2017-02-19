CFLAGS = -Wall

partc: main.o
	gcc -o partc main.o -lpthread

main.o: main.c
	gcc ${CFLAGS} -c main.c
	
clean:
	rm -rf *.o *.ko partc 
