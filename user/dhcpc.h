/*----------------------------------------------------------------------------
 Author:         Mark F (Cicero-MF) mark@cdelec.co.za
 Remarks:        
 Version:        04.04.2016
 Description:    DHCP client for the enc28j60 and an esp8266
 
 This code was adapted for use with the ESP8266, and based off Michael Kliebers version. 
   Author:         Michael Kleiber
   Remarks:        
   known Problems: none
   Version:        29.11.2008
   Description:    DHCP Client

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

#ifdef USE_DHCP
#ifndef _DHCPCLIENT_H
	#define _DHCPCLIENT_H
  #include <stdio.h>
  #include "globals.h"

	#define DHCP_CLIENT_PORT		  68
	#define DHCP_SERVER_PORT		  67
  
  /* Flags for where we are in the DHCP negotiation process */
  #define DHCP_SUCCESS  0
  #define DHCP_CONTINUE 1
  #define DHCP_TIMEOUT  2

  extern volatile u32 dhcp_lease;
  extern volatile u8 dhcp_timer;

  void dhcp_init     (void);
  void dhcp_message  (u8 type);
  void dhcp_get      (u8 index, u8 port_index);
  
  u8 ICACHE_FLASH_ATTR dhcp (void);
  void ICACHE_FLASH_ATTR check_dhcp (void *arg);
  void ICACHE_FLASH_ATTR user_main_setupContinued (void);

#endif //_DHCPCLIENT_H
#endif //USE_DHCP

