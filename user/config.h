/* config.h

-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        18.05.2016
Description:    Config holding structure adapted for the enc28j60 and an esp8266

  This code was modified from TuanPM's mqtt project
   Author:         Tuan PM <tuanpm at live dot com>
   Version:        2014-2015

-----------------------------------------------------------------------------------------
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef USER_CONFIG_H_
  #define USER_CONFIG_H_
  #include "os_type.h"
  #include "globals.h"
  
  #define CFG_HOLDER	0x00FF55A4	/* Change this value to load default configurations */
  #define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */
  
  #ifdef MQTT_USER_CONFIG
    #include "user_config.h"
  #endif

  /* Conversion from IP to u32 */
	#define IP(a,b,c,d) ((u32)(d)<<24)+((u32)(c)<<16)+((u32)(b)<<8)+a

	/* IP's */
  #define DHCPIP	  0
  #define MYIP		  IP(192,168,0,222)
  #define ROUTER_IP	IP(192,168,0,1)	
  #define NETMASK		IP(255,255,255,0)
  #define DNSIP	    ROUTER_IP
	
  /* Dummy MAC Address for ENC - ENC specific
    Only actually used if USE_SEPARATE_ENC_MAC is defined */
  #define MYMAC1	0x00
  #define MYMAC2	0x11
  #define MYMAC3	0x22
  #define MYMAC4	0x33	
  #define MYMAC5	0x44
  #define MYMAC6	0x55
 
  
  typedef struct{
    uint32_t cfg_holder;
    
    /* ENC28J60 ethernet IP address's - placed here to keep alignment */
    #ifdef IPS_IN_UNION
      union {
          uint8_t thech[4];
          uint32_t theint;
      } setipaddr;      
      union {
        uint8_t thech[4];
        uint32_t theint;
      } ethip;
      union {
        uint8_t thech[4];
        uint32_t theint;
      } netmask;
      union {
        uint8_t thech[4];
        uint32_t theint;
      } router_ip;
      union {
        uint8_t thech[4];
        uint32_t theint;
      } dns_server_ip;   
    #else
      uint8_t setipaddr[4];
      uint8_t ethip[4];
      uint8_t netmask[4];
      uint8_t router_ip[4];
      uint8_t dns_server_ip[4];
    #endif
    
    uint8_t device_id[16];

    uint8_t sta_ssid[64];
    uint8_t sta_pwd[64];
    uint32_t sta_type;

    uint8_t mqtt_host[64];
    uint32_t mqtt_port;
    uint8_t mqtt_user[32];
    uint8_t mqtt_pass[32];
    uint32_t mqtt_keepalive;
    uint8_t security;
  } SYSCFG;

  typedef struct {
      uint8 flag;
      uint8 pad[3];
  } SAVE_FLAG;

  void ICACHE_FLASH_ATTR CFG_Save();
  void ICACHE_FLASH_ATTR CFG_Load();

  extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
