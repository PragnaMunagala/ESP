CFLAGS = -Wall
final: main.o spi_gpio_module.o
	gcc -o final main.o spi_gpio_module.o -lpthread

main.o: main.c spi_gpio_module.h
	gcc ${CFLAGS} -c main.c

spi_gpio_module.o: spi_gpio_module.c spi_gpio_module.h
	gcc ${CFLAGS} -c spi_gpio_module.c
	
clean:
	rm -rf *.o *.ko final
