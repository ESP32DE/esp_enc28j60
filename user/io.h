#ifndef IO_H
#define IO_H

#define ENCRESETGPIO  4
#define ENCINTGPIO    5
#define BTNGPIO       0

void ICACHE_FLASH_ATTR gpio16_output_set(uint8 value);

void ICACHE_FLASH_ATTR ioLed(int ena);
void ioInit(void);

#endif
