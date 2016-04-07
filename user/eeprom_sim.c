/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        04.04.2016
Description:    EEPROM simulator - still todo, right now just pretends a ram buffer is it.

-----------------------------------------------------------------------------------------
License:
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.
  This program is distributed in the hope that it will be useful, but

  WITHOUT ANY WARRANTY;

  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51
  Franklin St, Fifth Floor, Boston, MA 02110, USA

  http://www.gnu.de/gpl-ger.html
-----------------------------------------------------------------------------------------*/

#include <esp8266.h>
#include "globals.h"
#include "stack.h"
#include "debug.h"
#include "eeprom_sim.h"

u8 eepromBuf[EEPROM_SIM_BUF_LEN];

void ICACHE_FLASH_ATTR eeprom_sim_init (void) {  
	u16 i;
  for (i=0; i<EEPROM_SIM_BUF_LEN; i++) {
    eepromBuf[i] = 0xff;
  }  
  INFO("eeprom_sim init\r\n");
}

void ICACHE_FLASH_ATTR eeprom_write_byte (u16 ptr, u8 val) {
  if ((u16)ptr > EEPROM_SIM_BUF_LEN) {
    INFO("EEPROM WRITE OUT OF BOUNDS!, ptr=%u\r\n",ptr);
    return;
  }
  
  *(eepromBuf+ptr) = val;  
}

u8 ICACHE_FLASH_ATTR eeprom_read_byte (u16 ptr) {
  if ((u16)ptr > EEPROM_SIM_BUF_LEN) {
    INFO("EEPROM READ OUT OF BOUNDS!, ptr=%u\r\n",ptr);
    return 0xFF;
  }
  
  return *(eepromBuf+ptr);  
}

void ICACHE_FLASH_ATTR eeprom_write_block(u8 *src, u16 dst, u16 n) {
  u16 i;
  
  for (i=0; i<n; i++) {
    *(eepromBuf+dst+i) = *(u8 *)(src+i);
  }
}

void ICACHE_FLASH_ATTR eeprom_read_block(u8 *dst, u16 src, u16 n) {
  u16 i;
  
  for (i=0; i<n; i++) {
    *(u8 *)(dst+i) = *(eepromBuf+src+i);
  }
}

void ICACHE_FLASH_ATTR eeprom_busy_wait (void) {};

//save all ip-datas
void ICACHE_FLASH_ATTR save_ip_addresses(void)
{
	for (unsigned char count = 0; count<4; count++)
	{
		eeprom_busy_wait ();
		eeprom_write_byte((IP_EEPROM_STORE + count),myip[count]);
	}
	for (unsigned char count = 0; count<4; count++)
	{
		eeprom_busy_wait ();
		eeprom_write_byte((NETMASK_EEPROM_STORE + count),netmask[count]);
	}
	for (unsigned char count = 0; count<4; count++)
	{
		eeprom_busy_wait ();
		eeprom_write_byte((ROUTER_IP_EEPROM_STORE + count),router_ip[count]);
	}
	#ifdef USE_DNS
	for (unsigned char count = 0; count<4; count++)
	{
		eeprom_busy_wait ();
		eeprom_write_byte((DNS_IP_EEPROM_STORE + count),dns_server_ip[count]);
	}
	#endif //USE_DNS
}

//------------------------------------------------------------------------------
//Read all IP-Datas
void ICACHE_FLASH_ATTR read_ip_addresses (void)
{
	(*((unsigned long*)&myip[0]))      = get_eeprom_value(IP_EEPROM_STORE,MYIP);
	(*((unsigned long*)&netmask[0]))   = get_eeprom_value(NETMASK_EEPROM_STORE,NETMASK);
	(*((unsigned long*)&router_ip[0])) = get_eeprom_value(ROUTER_IP_EEPROM_STORE,ROUTER_IP);
	//Broadcast-Adresse berechnen
	(*((unsigned long*)&broadcast_ip[0])) = (((*((unsigned long*)&myip[0])) & (*((unsigned long*)&netmask[0]))) | (~(*((unsigned long*)&netmask[0]))));
   
	#ifdef USE_DNS
	//DNS-Server IP aus EEPROM auslesen
	(*((unsigned long*)&dns_server_ip[0])) = get_eeprom_value(DNS_IP_EEPROM_STORE,DNS_IP);
	#endif

}


