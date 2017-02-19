final: main.o sharedQueue.o
	gcc -o final main.o sharedQueue.o -lpthread -lm

main.o: main.c sharedQueue.h
	gcc -c main.c

sharedQueue.o: sharedQueue.c sharedQueue.h
	gcc -c sharedQueue.c
	
clean:
	rm -rf *.o final
	
	

