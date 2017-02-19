#define SLAVE_ADDRESS 0x51
#define PAGE_SIZE 64
#define PAGES 512

/* Function declarations for various operations on EEPROM */
int EEPROM_init();
int EEPROM_read(void *buf, int count);
int EEPROM_write(void *buf, int count);
int EEPROM_set(int new_position);
int EEPROM_erase();
