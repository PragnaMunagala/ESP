rmmod spidev
rmmod led_dev.ko
rmmod pulse_dev.ko
insmod led_dev.ko
insmod pulse_dev.ko
gcc -o main main.c -lpthread
./main
