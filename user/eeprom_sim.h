#ifndef _EEPROM_SIM_H
#define _EEPROM_SIM_H

#define EEPROM_SIM_BUF_LEN (200+512+10+300)
void eeprom_write_block(u8 *src, u16 dst, u16 n);
void eeprom_busy_wait(void);
void eeprom_sim_init (void);
u8 eeprom_read_byte (u16 ptr);
void eeprom_write_byte (u16 ptr, u8 val);
void eeprom_read_block(u8 *dst, u16 src, u16 n);
void ICACHE_FLASH_ATTR save_ip_addresses(void);
void ICACHE_FLASH_ATTR read_ip_addresses (void);
#endif /* _EEPROM_SIM_H */
