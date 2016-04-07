/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        04.04.2016
Description:    Simple Ethernet stack for the enc28j60 and an esp8266

  This code was modified for use with the ESP8266, based off an original AVR version by  
   Author:         Radig Ulrich mailto: mail@ulrichradig.de
   Version:        24.10.2007
   Description:    Ethernet Stack

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
#include "globals.h"
#include "stack.h"
#include "eeprom_sim.h"
#include <esp8266.h>
#include "debug.h"
#include "io.h"
#include "driver/uart.h"
#include "hextools.h"
#include "espconn.h"
#include "httpd.h"
#include "espmissingincludes.h"


TCP_PORT_ITEM TCP_PORT_TABLE[MAX_APP_ENTRY] = // Port-Tabelle
{
	{80,serveSimpleWeb},
	{0,0},
	{0,0} 
};

UDP_PORT_ITEM UDP_PORT_TABLE[MAX_APP_ENTRY] = // Port-Tabelle
{
	{0,0},
	{0,0},
	{0,0} 
};

u8 myip[4];
u8 netmask[4];
u8 router_ip[4];
u8 broadcast_ip[4];
u16 IP_id_counter = 0;
u16 htmlDataSize;

/* Ethernet packet buffers */
u8 eth_buffer[MTU_SIZE+1];

arp_table arp_entry[MAX_ARP_ENTRY];

//TCP Stack Size
//+1 so that a connection can be dismissed at full stack
tcp_table tcp_entry[MAX_TCP_ENTRY+1]; 

PING_STRUCT ping;
ethStruct eth;

//----------------------------------------------------------------------------
//Converts integer variables to network Byte order
u16 ICACHE_FLASH_ATTR htons(u16 val)
{
  return HTONS(val);
}
//----------------------------------------------------------------------------
//Converts integer variables to network Byte order
u32 ICACHE_FLASH_ATTR htons32(u32 val)
{
  return HTONS32(val);
}

//----------------------------------------------------------------------------
//Initialise Stack, pull IP's, and init ENC
void ICACHE_FLASH_ATTR stack_init (void)
{
	//Timer init
	timer_init();

	//IP, NETMASK und ROUTER_IP aus EEPROM auslesen	
	(*((u32*)&myip[0])) = get_eeprom_value(IP_EEPROM_STORE,MYIP);
	(*((u32*)&netmask[0])) = get_eeprom_value(NETMASK_EEPROM_STORE,NETMASK);
	(*((u32*)&router_ip[0])) = get_eeprom_value(ROUTER_IP_EEPROM_STORE,ROUTER_IP);
	
    #ifdef USE_DNS
      //Read DNS server IP from EEPROM
      (*((u32*)&dns_server_ip[0])) = get_eeprom_value(DNS_IP_EEPROM_STORE,DNS_IP);
    #endif
  
	//Calculate broadcast address
	(*((u32*)&broadcast_ip[0])) = (((*((u32*)&myip[0])) & (*((u32*)&netmask[0]))) | (~(*((u32*)&netmask[0]))));

	//MAC Adresse setzen
	mymac[0] = MYMAC1;
  mymac[1] = MYMAC2;
  mymac[2] = MYMAC3;
  mymac[3] = MYMAC4;
  mymac[4] = MYMAC5;
  mymac[5] = MYMAC6;
	
	/*NIC init*/
	DEBUG("\n\rETH init:");
	ETH_INIT();
	
	DEBUG("My IP: %u.%u.%u.%u\r\n\r\n",myip[0],myip[1],myip[2],myip[3]);
}

//----------------------------------------------------------------------------
//
u32 ICACHE_FLASH_ATTR get_eeprom_value (u16 eeprom_adresse,u32 default_value)
{
	
  u8 value[4];
	
	for (u8 count = 0; count<4;count++)
	{
		eeprom_busy_wait ();	
		value[count] = eeprom_read_byte((eeprom_adresse + count));
	}

	//If the EEPROM content empty
	if ((*((u32*)&value[0])) == 0xFFFFFFFF)
	{
    
		return(default_value);
    
	}
	return((*((u32*)&value[0])));
}

//----------------------------------------------------------------------------
//Management of TCP timer
void ICACHE_FLASH_ATTR tcp_timer_call (void)
{
	for (u8 index = 0;index<MAX_TCP_ENTRY;index++)
	{
		if (tcp_entry[index].time == 0)
		{
			if (tcp_entry[index].ip != 0)
			{
				tcp_entry[index].time = TCP_MAX_ENTRY_TIME;
				if ((tcp_entry[index].error_count++) > MAX_TCP_ERRORCOUNT)
				{
					DEBUG("Entry is removed MAX_ERROR STACK:%u\r\n",index);
					ETS_GPIO_INTR_DISABLE(); //ETH_INT_DISABLE;
					tcp_entry[index].status =  RST_FLAG | ACK_FLAG;
					create_new_tcp_packet(0,index);
					ETS_GPIO_INTR_ENABLE(); //ETH_INT_ENABLE;
					tcp_index_del(index);
				}
				else
				{
					DEBUG("Packet is retransmitted STACK:%u\r\n",index);
					find_and_start (index);
				}
			}
		}
		else
		{
			if (tcp_entry[index].time != TCP_TIME_OFF)
			{
				tcp_entry[index].time--;
			}
		}
	}
}

//----------------------------------------------------------------------------
//Managing the ARP timer
void ICACHE_FLASH_ATTR arp_timer_call (void)
{
	for (u8 a = 0;a<MAX_ARP_ENTRY;a++)
	{
		if (arp_entry[a].arp_t_time == 0)
		{
			for (u8 b = 0;b<6;b++)
			{
				arp_entry[a].arp_t_mac[b]= 0;
			}
			arp_entry[a].arp_t_ip = 0;
		}
		else
		{
			arp_entry[a].arp_t_time--;
		}
	}
}

//----------------------------------------------------------------------------
// Add TCP PORT / application in application list - returns port_index
s8 ICACHE_FLASH_ATTR add_tcp_app (u16 port, void(*fp1)(u8))
{
	u8 port_index = 0;
	//Search free entry in the application list
	while (TCP_PORT_TABLE[port_index].port)
	{ 
		port_index++;
	}
	if (port_index >= MAX_APP_ENTRY)
	{
		DEBUG("TCP too many applications have been launched\r\n");
		return -1;
	}
	DEBUG("TCP application is entered in list: Entry %u, port = %u\r\n",port_index, port);
	TCP_PORT_TABLE[port_index].port = port;
	TCP_PORT_TABLE[port_index].fp = *fp1;
	return port_index;
}

//----------------------------------------------------------------------------
//Change the TCP PORT / application in application list
void ICACHE_FLASH_ATTR change_port_tcp_app (u16 port_old, u16 port_new)
{
	u8 port_index = 0;
	//Search for free entry in the application list
	while (TCP_PORT_TABLE[port_index].port && TCP_PORT_TABLE[port_index].port != port_old)
	{ 
		port_index++;
	}
	if (port_index >= MAX_APP_ENTRY)
	{
		DEBUG("( Port Change ) Port not found\r\n");
		return;
	}
	DEBUG("TCP application Change Port : Entry %i\r\n",port_index);
	TCP_PORT_TABLE[port_index].port = port_new;
	return;
}

//----------------------------------------------------------------------------
//Add UDP port/application to list
void ICACHE_FLASH_ATTR add_udp_app (u16 port, void(*fp1)(u8))
{
	u8 port_index = 0;
	//Search
	while (UDP_PORT_TABLE[port_index].port)
	{ 
		port_index++;
	}
	if (port_index >= MAX_APP_ENTRY)
	{
		DEBUG("Too many UDP application were launched\r\n");
		return;
	}
	DEBUG("UDP Application is registered in List: Entry %u\r\n",port_index);
	UDP_PORT_TABLE[port_index].port = port;
	UDP_PORT_TABLE[port_index].fp = *fp1;
	return;
}

//----------------------------------------------------------------------------
//Clears UDP application from the application list
void ICACHE_FLASH_ATTR kill_udp_app (u16 port)
{
    u8 i;

    for (i = 0; i < MAX_APP_ENTRY; i++)
    {
        if ( UDP_PORT_TABLE[i].port == port )
        {
            UDP_PORT_TABLE[i].port = 0;
        }
    }
    return;
}

//----------------------------------------------------------------------------
//Interrupt from the ENC
void ICACHE_FLASH_ATTR stack_encInterrupt (void)
{
  u32 gpio_status;
  ETS_GPIO_INTR_DISABLE();
  
  gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
  
	//DEBUG("In interrupt\r\n");
  eth.data_present = 1;
	eth.no_reset = 1;
  
  gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);  
}

//----------------------------------------------------------------------------
//PORT DONE - ETH get data
void ICACHE_FLASH_ATTR eth_get_data (void)
{ 
	
	if(eth.timer)
	{
		tcp_timer_call();
		arp_timer_call();
		eth.timer = 0;
	}	
	if(eth.data_present)
	{
		while(!GPIO_INPUT_GET(ENCINTGPIO))
		{	
			u16 packet_length;
				  
			packet_length = ETH_PACKET_RECEIVE(MTU_SIZE,eth_buffer);
			/*Wenn ein Packet angekommen ist, ist packet_lenght =! 0*/
			if(packet_length > 0)
			{
        /*u16 len;
        s8 first, second;
        */
				eth_buffer[packet_length+1] = 0;
       
        /*DEBUG("Check packet...\r\n\r\n");        
        for (len = 0; len <packet_length; len++) {          
          hextools_hex2ascii(eth_buffer[len], &first, &second);
          uart0_write_char(first);
          uart0_write_char(second);
        }
        DEBUG("\r\n\r\nDone packet...\r\n");
        */
        
				check_packet();
			}
		}
		eth.data_present = 0;
		ETS_GPIO_INTR_ENABLE();
	}
	return;
}

//----------------------------------------------------------------------------
//PORT DONE - Check Packet and call Stack for TCP or UDP
void ICACHE_FLASH_ATTR check_packet (void)
{
  Ethernet_Header *ethernet;    
  IP_Header       *ip;         
  ICMP_Header     *icmp;       

  ethernet = (Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
  ip       = (IP_Header       *)&eth_buffer[IP_OFFSET];
  icmp     = (ICMP_Header     *)&eth_buffer[ICMP_OFFSET];
  
  //ARP or not?
  if(ethernet->EnetPacketType == HTONS(0x0806) ) {
    arp_reply(); // check arp packet request/reply
  } else {
    // if IP
    if( ethernet->EnetPacketType == HTONS(0x0800) ) {         
      //DEBUG("if IP, %u\r\n",ip->IP_Destaddr);
      // if my IP address 
      if( ip->IP_Destaddr == *((u32*)&myip[0]) ) {
        //DEBUG("if my IP\r\n");
        arp_entry_add(); 
        if(ip->IP_Proto == PROT_ICMP) {
          switch ( icmp->ICMP_Type ) {
            case (8): //Ping reqest
              DEBUG("ping request\r\n");
              icmp_send(ip->IP_Srcaddr,0,0,icmp->ICMP_SeqNum,icmp->ICMP_Id); 
              break;

            case (0): //Ping reply
              DEBUG("ping reply\r\n");
              if ((*((u32*)&ping.ip1[0])) == ip->IP_Srcaddr) {
                ping.result |= 0x01;
              }
              DEBUG("%u",    (ip->IP_Srcaddr&0x000000FF)     );
              DEBUG(".%u",  ((ip->IP_Srcaddr&0x0000FF00)>>8 ));
              DEBUG(".%u",  ((ip->IP_Srcaddr&0x00FF0000)>>16));
              DEBUG(".%u :",((ip->IP_Srcaddr&0xFF000000)>>24));
              break;
          }
          return;
        } else {
          if( ip->IP_Proto == PROT_TCP ) tcp_socket_process();
          if( ip->IP_Proto == PROT_UDP ) udp_socket_process();
        }
      } else {        
        // if broadcast
        if (ip->IP_Destaddr == (u32)0xffffffff || ip->IP_Destaddr == *((u32*)&broadcast_ip[0]) ) {
          //DEBUG("Broadcast address\r\n");
          if( ip->IP_Proto == PROT_UDP ) { 
            //DEBUG("Calling UDP app\r\n");
            udp_socket_process();
          }
        }
      }
    }
  }
  return;
}

//----------------------------------------------------------------------------
//PORT DONE - creates an ARP - entry if not yet available 
void ICACHE_FLASH_ATTR arp_entry_add (void)
{
    DEBUG("ARP entry add\r\n");
    Ethernet_Header *ethernet;
    ARP_Header      *arp;
    IP_Header       *ip;
      
    ethernet = (Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
    arp      = (ARP_Header      *)&eth_buffer[ARP_OFFSET];
    ip       = (IP_Header       *)&eth_buffer[IP_OFFSET];
        
    DEBUG("ARP entry add\r\n");
    
    //Entry already exists ?
    for (u8 b = 0; b<MAX_ARP_ENTRY; b++)
    {
        if( ethernet->EnetPacketType == HTONS(0x0806) ) //If ARP
        {
           
          if(arp_entry[b].arp_t_ip == arp->ARP_SIPAddr)
          {
          // Time refresh
          for(u8 a = 0; a < 6; a++) {
            arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a];
          }
          arp_entry[b].arp_t_ip   = arp->ARP_SIPAddr;
          arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
          return;
          }
        } 
        
        if( ethernet->EnetPacketType == HTONS(0x0800) ) //If IP
        {
          if(arp_entry[b].arp_t_ip == ip->IP_Srcaddr)
          {
            //Time refresh
            for(u8 a = 0; a < 6; a++) {
              arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a];
            }
            arp_entry[b].arp_t_ip   = ip->IP_Srcaddr;
            arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
            return;
          }
        }
    }
  
    //Find entry
    for (u8 b = 0; b<MAX_ARP_ENTRY; b++)
    {
        if(arp_entry[b].arp_t_ip == 0)
        {
            if( ethernet->EnetPacketType == HTONS(0x0806) ) //if ARP
            {
                for(u8 a = 0; a < 6; a++)
                {
                    arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a]; 
                }
                arp_entry[b].arp_t_ip   = arp->ARP_SIPAddr;
                arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
            if( ethernet->EnetPacketType == HTONS(0x0800) ) //if IP
            {
                for(u8 a = 0; a < 6; a++)
                {
                    arp_entry[b].arp_t_mac[a] = ethernet->EnetPacketSrc[a]; 
                }
                arp_entry[b].arp_t_ip   = ip->IP_Srcaddr;
                arp_entry[b].arp_t_time = ARP_MAX_ENTRY_TIME;
                return;
            }
            DEBUG("No ARP or IP packet!\r\n");
            return;
        }
    }    
    DEBUG("ARP entry table full!\r\n");
    return;
}

//----------------------------------------------------------------------------
//PORT DONE - This routine search by IP ARP entry
char ICACHE_FLASH_ATTR arp_entry_search (u32 dest_ip)
{
	for (u8 b = 0;b<MAX_ARP_ENTRY;b++)
	{
		if(arp_entry[b].arp_t_ip == dest_ip)
		{
			return(b);
		}
	}
	return (MAX_ARP_ENTRY);
}

//----------------------------------------------------------------------------
//PORT DONE - This routine creates a new ethernet header 
void ICACHE_FLASH_ATTR new_eth_header (u8 *buffer,u32 dest_ip)
{
  u8 b;
  u8 a;
  
  /* FIXME: this hackMAC is the gateways MAC, 
          this it should get from its own ARP request, fix that */
  u8 hackMAC[6] = {0x64, 0xB5, 0xDE, 0x3A, 0xA5, 0x1E};
  
	Ethernet_Header *ethernet;
	ethernet = (Ethernet_Header *)&buffer[ETHER_OFFSET];
  	
	b = arp_entry_search (dest_ip);
	if (b != MAX_ARP_ENTRY) //found entry if not equal
	{
		for(u8 a = 0; a < 6; a++)
		{			
			ethernet->EnetPacketDest[a] = arp_entry[b].arp_t_mac[a];			
			ethernet->EnetPacketSrc[a] = mymac[a];
		}
		return;
	}
    
	DEBUG("ARP entry is not found*\r\n");
	for(a = 0; a < 6; a++)
	{	
      ethernet->EnetPacketDest[a] = 0xFF;
    
		//My MAC address is written in the source address
		ethernet->EnetPacketSrc[a] = mymac[a];
	}
	return;

}

//----------------------------------------------------------------------------
//PORT DONE - This routine responds to an ARP packet
void ICACHE_FLASH_ATTR arp_reply (void)
{
    u8 b;
    u8 a;
    u16 len;
    char first, second;
    
    Ethernet_Header *ethernet;
    ARP_Header      *arp;
    
    arp      = (ARP_Header      *)&eth_buffer[ARP_OFFSET];
    ethernet = (Ethernet_Header *)&eth_buffer[ETHER_OFFSET];
    
    //DEBUG("arp reply\r\n"); 

    if( arp->ARP_HWType  == HTONS(0x0001)  &&             // Hardware Typ:   Ethernet
        arp->ARP_PRType  == HTONS(0x0800)  &&             // Protocol Typ:  IP
        arp->ARP_HWLen   == 0x06           &&             
        arp->ARP_PRLen   == 0x04           &&             
        arp->ARP_TIPAddr == *((u32*)&myip[0])) // For us?
    {
        if (arp->ARP_Op == HTONS(0x0001) )                  // Request?
        {
            arp_entry_add(); 
            new_eth_header (eth_buffer, arp->ARP_SIPAddr); // Creates new ethernet header
            ethernet->EnetPacketType = HTONS(0x0806);      // 0x0800=IP Datagramm;0x0806 = ARP
      
            b = arp_entry_search (arp->ARP_SIPAddr);
            if (b < MAX_ARP_ENTRY)                        
            {
              for(a = 0; a < 6; a++)
              {
                  arp->ARP_THAddr[a] = arp_entry[b].arp_t_mac[a];
                  arp->ARP_SHAddr[a] = mymac[a];
              }              
            }
            else
            {
                DEBUG("ARP Entry not found\r\n");        
            }
            
	  
            arp->ARP_Op      = HTONS(0x0002);                   // ARP op = ECHO  
            arp->ARP_TIPAddr = arp->ARP_SIPAddr;                // ARP Target IP Adresse 
            arp->ARP_SIPAddr = *((u32 *)&myip[0]);   // Meine IP Adresse = ARP Source
                        
            //DEBUG("Sending packet...\r\n\r\n");        
            /*for (len = 0; len <ARP_REPLY_LEN; len++) {          
              hextools_hex2ascii(eth_buffer[len], &first, &second);
              uart0_write_char(first);
              uart0_write_char(second);
            }
            //DEBUG("\r\n\r\nDone send packet...\r\n");
            */
            DEBUG("Sending ARP reply\r\n"); 
            ETH_PACKET_SEND(ARP_REPLY_LEN,eth_buffer);          // ARP Reply senden...
            eth.no_reset = 1;
            return;
        }

        if ( arp->ARP_Op == HTONS(0x0002) )                    // REPLY from another client
        {
            arp_entry_add();
            DEBUG("ARP REPLY RECEIVED!\r\n");
        }
    }
    return;
}

//----------------------------------------------------------------------------
//FIXME: Use this to request the gateway MAC, get rid of hackMAC
/*
  char ICACHE_FLASH_ATTR arp_request (u32 dest_ip)
  {
      u8 buffer[ARP_REQUEST_LEN];
      u8 index = 0;
      u8 index_tmp;
      u8 count;
      u32 a;
      u32 dest_ip_store;

      Ethernet_Header *ethernet;
      ARP_Header *arp;

      DEBUG("arp request\r\n");  
      ethernet = (Ethernet_Header *)&buffer[ETHER_OFFSET];
      arp      = (ARP_Header      *)&buffer[ARP_OFFSET];

      dest_ip_store = dest_ip;

      if ( (dest_ip & (*((u32 *)&netmask[0])))==
         ((*((u32 *)&myip[0]))&(*((u32 *)&netmask[0]))) )
      {
          DEBUG("MY NETWORK!\r\n");
      }
      else
      {
          DEBUG("ROUTING!\r\n");
          dest_ip = (*((u32 *)&router_ip[0]));
      }

      ethernet->EnetPacketType = HTONS(0x0806);          // Nutzlast 0x0800=IP Datagramm;0x0806 = ARP
    
      new_eth_header (buffer,dest_ip);
    
      arp->ARP_SIPAddr = *((u32 *)&myip[0]);   // MyIP = ARP Source IP
      arp->ARP_TIPAddr = dest_ip;                         // Dest IP 
    
      for(count = 0; count < 6; count++)
      {
          arp->ARP_SHAddr[count] = mymac[count];
          arp->ARP_THAddr[count] = 0;
      }
    
      arp->ARP_HWType = HTONS(0x0001);
      arp->ARP_PRType = HTONS(0x0800);
      arp->ARP_HWLen  = 0x06;
      arp->ARP_PRLen  = 0x04;
      arp->ARP_Op     = HTONS(0x0001);

      ETH_PACKET_SEND(ARP_REQUEST_LEN, buffer);        //send....
    eth.no_reset = 1;
    
      for(count = 0; count<20; count++)
      {
          index_tmp = arp_entry_search(dest_ip_store);
          index = arp_entry_search(dest_ip);
          if (index < MAX_ARP_ENTRY || index_tmp < MAX_ARP_ENTRY)
          {
              DEBUG("ARP EINTRAG GEFUNDEN!\r\n");
              if (index_tmp < MAX_ARP_ENTRY) return(1);//OK
              arp_entry[index].arp_t_ip = dest_ip_store;
              return(1);//OK
          }
          os_delay_us(1000);
          eth_get_data();
          DEBUG("**KEINEN ARP EINTRAG GEFUNDEN**\r\n");
      }
      return(0);//keine Antwort
  }
*/


//----------------------------------------------------------------------------
//PORT DONE - This routine creates a new ICMP Packet
void ICACHE_FLASH_ATTR icmp_send (u32 dest_ip, u8 icmp_type, 
                u8 icmp_code, u16 icmp_sn, 
                u16 icmp_id)
{
  u16 result16;  //Checksum
  IP_Header   *ip;
  ICMP_Header *icmp;
  
  
  ip   = (IP_Header   *)&eth_buffer[IP_OFFSET];
  icmp = (ICMP_Header *)&eth_buffer[ICMP_OFFSET];

  DEBUG("icmp_send()\r\n");  
  
  //This is an Echo Reply Packet
  icmp->ICMP_Type   = icmp_type;
  icmp->ICMP_Code   = icmp_code;
  icmp->ICMP_Id     = icmp_id;
  icmp->ICMP_SeqNum = icmp_sn;
  icmp->ICMP_Cksum  = 0;
  ip->IP_Pktlen     = HTONS(0x0054);   // 0x54 = 84 
  ip->IP_Proto      = PROT_ICMP;
  make_ip_header (eth_buffer,dest_ip);

  //Berechnung der ICMP Header länge
  result16 = htons(ip->IP_Pktlen);
  result16 = result16 - ((ip->IP_Vers_Len & 0x0F) << 2);

  //pointer wird auf das erste Paket im ICMP Header gesetzt
  //jetzt wird die Checksumme berechnet
  result16 = checksum (&icmp->ICMP_Type, result16, 0);
 
  //schreibt Checksumme ins Packet
  icmp->ICMP_Cksum = htons(result16);
   
  //Sendet das erzeugte ICMP Packet 
  ETH_PACKET_SEND(ICMP_REPLY_LEN,eth_buffer);
  eth.no_reset = 1;
}

//----------------------------------------------------------------------------
//PORT DONE - This routine create a checksum
u16 ICACHE_FLASH_ATTR checksum (u8 *pointer,u16 result16,u32 result32)
{
	u16 result16_1 = 0x0000;
	u8 DataH;
	u8 DataL;
  
  //Jetzt werden alle Packete in einer While Schleife addiert
	while(result16 > 1)
	{
		//schreibt Inhalt Pointer nach DATAH danach inc Pointer
		DataH=*pointer++;

		//schreibt Inhalt Pointer nach DATAL danach inc Pointer
		DataL=*pointer++;

		//erzeugt Int aus Data L und Data H
		result16_1 = ((DataH << 8)+DataL);
		//Addiert packet mit vorherigen
		result32 = result32 + result16_1;
		//decrimiert Länge von TCP Headerschleife um 2
		result16 -=2;
	}

	//Ist der Wert result16 ungerade ist DataL = 0
	if(result16 > 0)
	{
		//schreibt Inhalt Pointer nach DATAH danach inc Pointer
		DataH=*pointer;
		//erzeugt Int aus Data L ist 0 (ist nicht in der Berechnung) und Data H
		result16_1 = (DataH << 8);
		//Addiert packet mit vorherigen
		result32 = result32 + result16_1;
	}
	
	//Komplementbildung (addiert Long INT_H Byte mit Long INT L Byte)
	result32 = ((result32 & 0x0000FFFF)+ ((result32 & 0xFFFF0000) >> 16));
	result32 = ((result32 & 0x0000FFFF)+ ((result32 & 0xFFFF0000) >> 16));	
	result16 =~(result32 & 0x0000FFFF);
	
	return (result16);
}

//----------------------------------------------------------------------------
//PORT DONE - This routine creates an IP packet
void ICACHE_FLASH_ATTR make_ip_header (u8 *buffer,u32 dest_ip)
{
    u16 result16;  //Checksum
    Ethernet_Header *ethernet;
    IP_Header       *ip;

    ethernet = (Ethernet_Header *)&buffer[ETHER_OFFSET];
    ip       = (IP_Header       *)&eth_buffer[IP_OFFSET];
    
    //DEBUG("make ip header\r\n");  
    new_eth_header (buffer, dest_ip);         //Erzeugt einen neuen Ethernetheader

    ethernet->EnetPacketType = HTONS(0x0800); //Payload 0x0800=IP
    IP_id_counter++;

    ip->IP_Frag_Offset = 0x0040;  //don't fragment
    ip->IP_ttl         = 128;      //max. hops
    ip->IP_Id          = htons(IP_id_counter);
    ip->IP_Vers_Len    = 0x45;  //4 BIT Die Versionsnummer von IP, 
    ip->IP_Tos         = 0;
    ip->IP_Destaddr     = dest_ip;
    ip->IP_Srcaddr     = *((u32 *)&myip[0]);
    ip->IP_Hdr_Cksum   = 0;
  
    //Berechnung der IP Header länge  
    result16 = (ip->IP_Vers_Len & 0x0F) << 2;

    //jetzt wird die Checksumme berechnet
    result16 = checksum (&ip->IP_Vers_Len, result16, 0);

    //schreibt Checksumme ins Packet
    ip->IP_Hdr_Cksum = htons(result16);
    return;
}

//----------------------------------------------------------------------------
//Diese Routine verwaltet TCP-Einträge
void ICACHE_FLASH_ATTR tcp_entry_add (u8 *buffer)
{
    u32 result32;

    TCP_Header *tcp;
    IP_Header  *ip;

    tcp = (TCP_Header *)&buffer[TCP_OFFSET];
    ip  = (IP_Header  *)&buffer[IP_OFFSET];
  
    //Entry already exists?
    for (u8 index = 0;index<(MAX_TCP_ENTRY);index++)
    {
      if( (tcp_entry[index].ip       == ip->IP_Srcaddr  ) &&
          (tcp_entry[index].src_port == tcp->TCP_SrcPort)    )
      {
        //Record found Time refresh
        tcp_entry[index].ack_counter = tcp->TCP_Acknum;
        tcp_entry[index].seq_counter = tcp->TCP_Seqnum;
        tcp_entry[index].status      = tcp->TCP_HdrFlags;
        if ( tcp_entry[index].time != TCP_TIME_OFF )
        {
            tcp_entry[index].time = TCP_MAX_ENTRY_TIME;
        }
        result32 = htons(ip->IP_Pktlen) - IP_VERS_LEN - ((tcp->TCP_Hdrlen& 0xF0) >>2);
        result32 = result32 + htons32(tcp_entry[index].seq_counter);
        tcp_entry[index].seq_counter = htons32(result32);
  
        DEBUG("TCP Entry found %u\r\n",index);
        return;
      }
    }
  
    //Find outdoor entry
    for (u8 index = 0;index<(MAX_TCP_ENTRY);index++)
    {
        if(tcp_entry[index].ip == 0)
        {
            tcp_entry[index].ip          = ip->IP_Srcaddr;
            tcp_entry[index].src_port    = tcp->TCP_SrcPort;
            tcp_entry[index].dest_port   = tcp->TCP_DestPort;
            tcp_entry[index].ack_counter = tcp->TCP_Acknum;
            tcp_entry[index].seq_counter = tcp->TCP_Seqnum;
            tcp_entry[index].status      = tcp->TCP_HdrFlags;
            tcp_entry[index].app_status  = 0;
            tcp_entry[index].time        = TCP_MAX_ENTRY_TIME;
            tcp_entry[index].error_count = 0;
            tcp_entry[index].first_ack   = 0;
            DEBUG("TCP NewListing %u\r\n",index);
            return;  
        }
    }
    //Entry could not be included
    DEBUG("Server Busy (NO MORE CONNECTIONS)!\r\n");
    return;
}

//----------------------------------------------------------------------------
//Diese Routine sucht den etntry eintrag
char ICACHE_FLASH_ATTR tcp_entry_search (u32 dest_ip,u16 SrcPort)
{
	for (u8 index = 0;index<MAX_TCP_ENTRY;index++)
	{
    DEBUG("Search %u, ip = %u, scrPort = %u\r\n",index, tcp_entry[index].ip, tcp_entry[index].src_port);
		if(	tcp_entry[index].ip == dest_ip &&
			tcp_entry[index].src_port == SrcPort)
		{
			return(index);
		}
	}
	return (MAX_TCP_ENTRY);
}

//----------------------------------------------------------------------------
//This routine manages the UDP ports
void ICACHE_FLASH_ATTR udp_socket_process(void)
{
	u8 port_index = 0;	
	UDP_Header *udp;
	u8 len;
    
	udp = (UDP_Header *)&eth_buffer[UDP_OFFSET];

  /*DEBUG("Check udp packet...\r\n\r\n");
        for (len = 0; len <76; len++) {
        	s8 first, second;
          hextools_hex2ascii(eth_buffer[len], &first, &second);
          uart0_write_char(first);
          uart0_write_char(second);
        }
       DEBUG("\r\n\r\nDone packet...\r\n");*/
        
	//Perform UDP Port with Port Dest application list
  //DEBUG("UDP Dest Port = %02x, %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n", udp->udp_DestPort);
  
  //DEBUG("UDP Dest Port = %d\r\n", (htons(udp->udp_DestPort)));
	while (UDP_PORT_TABLE[port_index].port && UDP_PORT_TABLE[port_index].port!=(htons(udp->udp_DestPort)))
	{ 
		port_index++;
	}
	
	// If index is too big , then quit any existing application for the Port
	if (!UDP_PORT_TABLE[port_index].port)
	{ 
		//No existing application found (END)
		//DEBUG("UDP No app found!\r\n");
		return;
	}

	//zugehörige Anwendung ausführen
	UDP_PORT_TABLE[port_index].fp(0); 
	return;
}

//----------------------------------------------------------------------------
//Diese Routine Erzeugt ein neues UDP Packet
void ICACHE_FLASH_ATTR create_new_udp_packet( u16  data_length,
                            u16  src_port,
                            u16  dest_port,
                            u32 dest_ip)
{
  u16  result16;
  u32 result32;

  UDP_Header *udp;
  IP_Header  *ip;
  
  udp = (UDP_Header *)&eth_buffer[UDP_OFFSET];
  ip  = (IP_Header  *)&eth_buffer[IP_OFFSET];

  //DEBUG("create_new_udp_packet()\r\n");  
  
  udp->udp_SrcPort  = htons(src_port);
  udp->udp_DestPort = htons(dest_port);

  data_length     += UDP_HDR_LEN;                //UDP Packetlength
  udp->udp_Hdrlen = htons(data_length);

  data_length     += IP_VERS_LEN;                //IP Headerlänge + UDP Headerlänge
  ip->IP_Pktlen = htons(data_length);
  data_length += ETH_HDR_LEN;
  ip->IP_Proto = PROT_UDP;
  make_ip_header (eth_buffer,dest_ip);

  udp->udp_Chksum = 0;

  //Berechnet Headerlänge und Addiert Pseudoheaderlänge 2XIP = 8
  result16 = htons(ip->IP_Pktlen) + 8;
  result16 = result16 - ((ip->IP_Vers_Len & 0x0F) << 2);
  result32 = result16 + 0x09;

  //Routine berechnet die Checksumme
  result16 = checksum ((&ip->IP_Vers_Len+12), result16, result32);
  udp->udp_Chksum = htons(result16);

  ETH_PACKET_SEND(data_length,eth_buffer); //send...
  eth.no_reset = 1;
  return;
}

//----------------------------------------------------------------------------
//This routine manages the TCP ports
void ICACHE_FLASH_ATTR tcp_socket_process(void)
{
	u8 index = 0;
	u8 port_index = 0;
	u32 result32 = 0;

	TCP_Header *tcp;
	tcp = (TCP_Header *)&eth_buffer[TCP_OFFSET];

	IP_Header *ip; 	
	ip = (IP_Header *)&eth_buffer[IP_OFFSET];

	// Perform TCP Port with Port Dest application list
	while (TCP_PORT_TABLE[port_index].port && TCP_PORT_TABLE[port_index].port!=(htons(tcp->TCP_DestPort)))
	{ 
		port_index++;
	}
	
	// If index is too large , then quit any existing application for Port
  // Starts from a client what ? Will a client application to open a port ?
	if (!TCP_PORT_TABLE[port_index].port)
	{ 
		//No existing application available! (END)
		DEBUG("TCP No application found!\r\n");
		return;
	}

  DEBUG("Received Flags= ");	
  if(tcp->TCP_HdrFlags & SYN_FLAG)
	{	
		DEBUG("SYN ");	
	}
  if(tcp->TCP_HdrFlags & ACK_FLAG)
	{	
		DEBUG("ACK ");	
	}
  if(tcp->TCP_HdrFlags & RST_FLAG)
	{	
		DEBUG("RST ");	
	}
  if(tcp->TCP_HdrFlags & PSH_FLAG)
	{	
		DEBUG("PSH ");	
	}
  if(tcp->TCP_HdrFlags & FIN_FLAG)
	{	
		DEBUG("FIN ");	
	}
  DEBUG("\r\n");	
  
	//Server opens port
	if((tcp->TCP_HdrFlags & SYN_FLAG) && (tcp->TCP_HdrFlags & ACK_FLAG))
	{	
		DEBUG("SYN ACK received\r\n");
    
    // Takes on entry as it is a client - are applying for the port
		tcp_entry_add (eth_buffer);
		// Was the listing successful?
		index = tcp_entry_search (ip->IP_Srcaddr,tcp->TCP_SrcPort);
		if (index >= MAX_TCP_ENTRY) //Found entry if not equal
		{
			DEBUG("TCP entry not successful!\r\n");
			return;
		}
	
		//tcp_entry[index].time = MAX_TCP_PORT_OPEN_TIME;
		DEBUG("TCP port has been opened by the server STACK:%u\r\n",index);
		result32 = htons32(tcp_entry[index].seq_counter) + 1;
		tcp_entry[index].seq_counter = htons32(result32);
		tcp_entry[index].status =  ACK_FLAG;
    DEBUG("Sending ACK\r\n");
		create_new_tcp_packet(0,index);
		//Server port has been opened app . can now send data !
		tcp_entry[index].app_status = 1;
    
		return;
	}
	
	//Not intended for connection application
	if(tcp->TCP_HdrFlags == SYN_FLAG)
	{
		//Takes on entry as it is a server - are applying for the port
		tcp_entry_add (eth_buffer);
		//Was the listing successful?
		index = tcp_entry_search (ip->IP_Srcaddr,tcp->TCP_SrcPort);
		if (index >= MAX_TCP_ENTRY) //Found entry if not equal
		{
			DEBUG("TCP entry not successful!\r\n");
			return;
		}
	
		DEBUG("TCP New SERVER Conn:STACK:%u\r\n",index);
		
		tcp_entry[index].status =  ACK_FLAG | SYN_FLAG;
		create_new_tcp_packet(0,index);
		return;
	}

	//Find Packet entry in the TCP stack !
	index = tcp_entry_search (ip->IP_Srcaddr,tcp->TCP_SrcPort);
	
	if (index >= MAX_TCP_ENTRY) //Entry not found
	{
		DEBUG("TCP entry not found\r\n");
		tcp_entry_add (eth_buffer);
		if(tcp->TCP_HdrFlags & FIN_FLAG || tcp->TCP_HdrFlags & RST_FLAG)
		{	
			result32 = htons32(tcp_entry[index].seq_counter) + 1;
			tcp_entry[index].seq_counter = htons32(result32);
			
			if (tcp_entry[index].status & FIN_FLAG)
			{
				tcp_entry[index].status = ACK_FLAG;
				create_new_tcp_packet(0,index);
			}
			tcp_index_del(index);
			DEBUG("Deleted TCP stack entry! STACK:%u\r\n",index);
			return;
		}
		return;
	}


	//Refresh the entry
	tcp_entry_add (eth_buffer);
	
	//Host wants to end connection !
	if(tcp_entry[index].status & FIN_FLAG || tcp_entry[index].status & RST_FLAG)
	{	
		result32 = htons32(tcp_entry[index].seq_counter) + 1;
		tcp_entry[index].seq_counter = htons32(result32);
		
		if (tcp_entry[index].status & FIN_FLAG)
		{
			// Announce the end of the application !
			TCP_PORT_TABLE[port_index].fp(index);

			tcp_entry[index].status = ACK_FLAG;
			create_new_tcp_packet(0,index);
		}
		tcp_index_del(index);
		DEBUG("Deleted TCP stack entry! STACK:%u\r\n",index);
		return;
	}
	
	// Data for application PSH flag set ?
	if((tcp_entry[index].status & PSH_FLAG) && 
		(tcp_entry[index].status & ACK_FLAG))
	{
		// Run associated application
		if(tcp_entry[index].app_status < 0xFFFE) tcp_entry[index].app_status++;	
		//tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
		tcp_entry[index].status =  ACK_FLAG;
		TCP_PORT_TABLE[port_index].fp(index); 
		return;
	}
	
	//No data received packet was co nfirmed for application
  //z.B . after the connection ( SYN packet )
	if((tcp_entry[index].status & ACK_FLAG) && (tcp_entry[index].first_ack == 0))
	{
    DEBUG("First ACK\r\n");
		//No further action
		tcp_entry[index].first_ack = 1;
    
		return;
	}
	
	//Acknowledgment of receipt of a signal sent from the application package ( END )
	if((tcp_entry[index].status & ACK_FLAG) && (tcp_entry[index].first_ack == 1))
	{
		//ACK for Connection release
		if(tcp_entry[index].app_status == 0xFFFF)
		{
			return;
		}
    
		// Run associated application
		tcp_entry[index].status =  ACK_FLAG;
		if(tcp_entry[index].app_status < 0xFFFE) tcp_entry[index].app_status++;
		TCP_PORT_TABLE[port_index].fp(index); 
		return;
	}
	return;
}

void ICACHE_FLASH_ATTR serveSimpleWeb (u8 index) { 
  // for the first one, call httpdWiredrec
  // then once sent, and an ack received, call httpdSentCb
  // once finished, send FIN flag
  // ack FIN flag as well in here
  
  u16 dat_p;  
  
  DEBUG("appS==%u\r\n", tcp_entry[index].app_status);
  if (tcp_entry[index].app_status == 1) {
    DEBUG("WCon+Rec\r\n");
    /* If this is the first time we're in here, setup the handle for httpd connection */
    tcp_entry[index].i = httpdWiredConnect();
    dat_p=TCP_DATA_END_VAR - TCP_DATA_START_VAR;
    httpdWiredRec((char *)&(eth_buffer[TCP_DATA_START]), dat_p, tcp_entry[index].i);
    if (htmlDataSize) {       
      tcp_entry[index].status = ACK_FLAG;
      create_new_tcp_packet(htmlDataSize,index);         
    } 
    htmlDataSize = 0; 
  }
  
  if (tcp_entry[index].app_status > 1) {
    if(tcp_entry[index].status & ACK_FLAG) {
      /* Means we've sent the intial stuff, and have a handle we hope - so call the sent callback */
      httpdWiredSentCb(tcp_entry[index].i);
      if (htmlDataSize) {
        tcp_entry[index].status = ACK_FLAG;
        create_new_tcp_packet(htmlDataSize,index);
      } else {
        /* If there's no data, lets send a FIN flag */
        tcp_entry[index].status = ACK_FLAG | FIN_FLAG;          
        create_new_tcp_packet(0,index);
        
        /* Try doing nothing... */
      }        
      htmlDataSize = 0;        
    }      
  } 
}

void ICACHE_FLASH_ATTR stack_saveHtmlPacket(uint8_t *dataIn, u16 data_length) {
  os_memcpy(&eth_buffer[TCP_DATA_START], dataIn, data_length);
  htmlDataSize = data_length;
}

//----------------------------------------------------------------------------
//This routine creates a new TCP Packet
void ICACHE_FLASH_ATTR create_new_tcp_packet(u16 data_length,u8 index)
{
  u16  result16;
  u32 result32;
  u16  bufferlen;
  
  TCP_Header *tcp;
  IP_Header  *ip;

  tcp = (TCP_Header *)&eth_buffer[TCP_OFFSET];
  ip  = (IP_Header  *)&eth_buffer[IP_OFFSET];

  tcp->TCP_SrcPort   = tcp_entry[index].dest_port;
  tcp->TCP_DestPort  = tcp_entry[index].src_port;
  tcp->TCP_UrgentPtr = 0;
  tcp->TCP_Window    = htons(MAX_WINDOWS_SIZE);
  tcp->TCP_Hdrlen    = 0x50;

  DEBUG("TCP Src/DestPort %u, %u\r\n", tcp->TCP_SrcPort,tcp->TCP_DestPort );

  result32 = htons32(tcp_entry[index].seq_counter); 

  tcp->TCP_HdrFlags = tcp_entry[index].status;
  
  DEBUG("Sending Flags= ");	
  if(tcp->TCP_HdrFlags & SYN_FLAG)
	{	
		DEBUG("SYN ");	
	}
  if(tcp->TCP_HdrFlags & ACK_FLAG)
	{	
		DEBUG("ACK ");	
	}
  if(tcp->TCP_HdrFlags & RST_FLAG)
	{	
		DEBUG("RST ");	
	}
  if(tcp->TCP_HdrFlags & PSH_FLAG)
	{	
		DEBUG("PSH ");	
	}
  if(tcp->TCP_HdrFlags & FIN_FLAG)
	{	
		DEBUG("FIN ");	
	}
  DEBUG("\r\n");
  
  //Connection is established
  if(tcp_entry[index].status & SYN_FLAG)
  {
      result32++;
      // MSS-Option (siehe RFC 879) wil.
      eth_buffer[TCP_DATA_START]   = 2;
      eth_buffer[TCP_DATA_START+1] = 4;
      eth_buffer[TCP_DATA_START+2] = (MAX_WINDOWS_SIZE >> 8) & 0xff;
      eth_buffer[TCP_DATA_START+3] = MAX_WINDOWS_SIZE & 0xff;
      data_length                  = 0x04;
      tcp->TCP_Hdrlen              = 0x60;
  }
  
  tcp->TCP_Acknum = htons32(result32);
  tcp->TCP_Seqnum = tcp_entry[index].ack_counter;

  bufferlen = IP_VERS_LEN + TCP_HDR_LEN + data_length;    //IP Headerlänge + TCP Headerlänge
  ip->IP_Pktlen = htons(bufferlen);                      //Hier wird erstmal der IP Header neu erstellt
  bufferlen += ETH_HDR_LEN;
  ip->IP_Proto = PROT_TCP;
  make_ip_header (eth_buffer,tcp_entry[index].ip);

  tcp->TCP_Chksum = 0;

  //Berechnet Headerlänge und Addiert Pseudoheaderlänge 2XIP = 8
  result16 = htons(ip->IP_Pktlen) + 8;
  result16 = result16 - ((ip->IP_Vers_Len & 0x0F) << 2);
  result32 = result16 - 2;

  //Checksum
  result16 = checksum ((&ip->IP_Vers_Len+12), result16, result32);
  tcp->TCP_Chksum = htons(result16);

  /*DEBUG("Sending packet...\r\n\r\n");        
    for (len = 0; len <bufferlen; len++) {          
      hextools_hex2ascii(eth_buffer[len], &first, &second);
      uart0_write_char(first);
      uart0_write_char(second);
    }
    DEBUG("\r\n\r\nDone packet...\r\n");
   */
   
  //Send the TCP packet
  ETH_PACKET_SEND(bufferlen,eth_buffer);
  eth.no_reset = 1;

  //for Retransmission
  tcp_entry[index].status = 0;
  return;
}

//----------------------------------------------------------------------------
//Diese Routine schließt einen offenen TCP-Port
void ICACHE_FLASH_ATTR tcp_Port_close (u8 index)
{
	DEBUG("Port is closed in TCP stack STACK:%u\r\n",index);
	tcp_entry[index].app_status = 0xFFFF;
	tcp_entry[index].status =  ACK_FLAG | FIN_FLAG;
	create_new_tcp_packet(0,index);
	return;
}

//----------------------------------------------------------------------------
//Diese Routine findet die Anwendung anhand des TCP Ports
void ICACHE_FLASH_ATTR find_and_start (u8 index)
{
    u8 port_index = 0;

    //Port mit Anwendung in der Liste suchen
    while ( (TCP_PORT_TABLE[port_index].port!=(htons(tcp_entry[index].dest_port))) &&
          (port_index < MAX_APP_ENTRY)                                                 )
    { 
        port_index++;
    }
    if (port_index >= MAX_APP_ENTRY) return;
  
    //zugehörige Anwendung ausführen (Senden wiederholen)
    TCP_PORT_TABLE[port_index].fp(index);
    return;
}

//----------------------------------------------------------------------------
//This routine opens a TCP port
void ICACHE_FLASH_ATTR tcp_port_open (u32 dest_ip,u16 port_dst,u16 port_src)
{
	u8 index;
    
	ETS_GPIO_INTR_DISABLE();
		
	//Find entry
	for (index = 0;index<MAX_TCP_ENTRY;index++)
	{
		if(tcp_entry[index].ip == 0)
		{
			tcp_index_del(index);
			tcp_entry[index].ip = dest_ip;
			tcp_entry[index].src_port = port_dst;
			tcp_entry[index].dest_port = port_src;
			tcp_entry[index].ack_counter = 1234;
			tcp_entry[index].seq_counter = 2345;
			tcp_entry[index].time = MAX_TCP_PORT_OPEN_TIME;
			DEBUG("TCP Open New Listing %u\r\n",index);
			break;
		}
	}
	if (index >= MAX_TCP_ENTRY)
	{
		//Entry could not be included
		DEBUG("Busy (NO MORE CONNECTIONS)!\r\n");
	}
	
	tcp_entry[index].status =  SYN_FLAG;
	create_new_tcp_packet(0,index);
	ETS_GPIO_INTR_ENABLE();
	return;
}

//----------------------------------------------------------------------------
//Diese Routine löscht einen Eintrag
void ICACHE_FLASH_ATTR tcp_index_del (u8 index)
{
	if (index<MAX_TCP_ENTRY + 1)
	{
		tcp_entry[index].ip = 0;
		tcp_entry[index].src_port = 0;
		tcp_entry[index].dest_port = 0;
		tcp_entry[index].ack_counter = 0;
		tcp_entry[index].seq_counter = 0;
		tcp_entry[index].status = 0;
		tcp_entry[index].app_status = 0;
		tcp_entry[index].time = 0;
		tcp_entry[index].first_ack = 0;
	}
	return;
}

//----------------------------------------------------------------------------
//End of file: stack.c







