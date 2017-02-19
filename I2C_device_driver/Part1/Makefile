CFLAGS = -Wall
final: main.o EEPROM.o
	gcc -o final main.o EEPROM.o

main.o: main.c EEPROM.h
	gcc ${CFLAGS} -c main.c

EEPROM.o: EEPROM.c EEPROM.h
	gcc ${CFLAGS} -c EEPROM.c
	
clean:
	rm -rf *.o *.ko final
	
