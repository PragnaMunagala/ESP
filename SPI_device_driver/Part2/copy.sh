make clean
make
scp main.c root@<IP address>:/home/root/part2
scp led_dev.ko root@<IP address>:/home/root/part2
scp pulse_dev.ko root@<IP address>:/home/root/part2
scp board.sh root@<IP address>:/home/root/part2

