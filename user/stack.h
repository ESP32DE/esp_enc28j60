/*-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        18.05.2016
Description:    Header for stack.c

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

#ifndef _STACK_H
	#define _STACK_H

#include <string.h>
#include "enc28j60.h"
#include "user_config.h"
#include "timer.h"
#include "httpd.h"

#define MAX_APP_ENTRY 5

typedef struct __attribute__((packed))
{
	volatile u8 data_present  : 1;
	volatile u8 timer		      : 1;
	volatile u8 no_reset		  : 1;
}ethStruct;

extern ethStruct eth;

typedef struct __attribute__((packed))
{
	u16 port;		      // Local Port!
	void(*fp)(u8, u8);  	// Pointer to function to be executed
  struct espconn *espconn;
} TCP_PORT_ITEM;

typedef struct __attribute__((packed))
{
	u16 port;		      // Port
	void(*fp)(u8, u8);  	// Pointer to function to be executed
} UDP_PORT_ITEM;

extern TCP_PORT_ITEM TCP_PORT_TABLE[MAX_APP_ENTRY];
extern UDP_PORT_ITEM UDP_PORT_TABLE[MAX_APP_ENTRY];
	
#define HTONS(n) (u16)((((u16) (n)) << 8) | (((u16) (n)) >> 8))
#define HTONS32(x) ((x & 0xFF000000)>>24)+((x & 0x00FF0000)>>8)+((x & 0x0000FF00)<<8)+((x & 0x000000FF)<<24)

extern u8 myip[4];
extern u8 netmask[4];
extern u8 router_ip[4];
extern u8 broadcast_ip[4];

extern u16 IP_id_counter;

#define MAX_TCP_ENTRY 8
#define MAX_UDP_ENTRY 3
#define MAX_ARP_ENTRY 6
//#define MTU_SIZE 700
#define MTU_SIZE 1080

#define ARP_REPLY_LEN		60
#define ICMP_REPLY_LEN		98
#define ARP_REQUEST_LEN		42

extern u8 eth_buffer[MTU_SIZE+1];

#define TCP_TIME_OFF 		0xFF
#define TCP_MAX_ENTRY_TIME	3
#define MAX_TCP_PORT_OPEN_TIME 30 //30sec
#define MAX_TCP_ERRORCOUNT	5

#define ARP_MAX_ENTRY_TIME 100 //100sec.

#define MAX_WINDOWS_SIZE (MTU_SIZE-100)

typedef struct __attribute__((packed))
{
	volatile u8 arp_t_mac[6];
	volatile u32 arp_t_ip;
	volatile u16 arp_t_time;
} arp_table;

//FYI - Cant have attribute packed for this
typedef struct
{
	volatile u32 ip;
	volatile u16  src_port;
	volatile u16  dest_port;
	volatile u32 ack_counter;
	volatile u32 seq_counter;
	volatile u8 status;
	volatile u16  app_status;
	volatile u8 time;
	volatile u8 error_count;
	volatile u8 first_ack	:1;
  
  /* To copy the way the SDK does things with espconn,
    we need separate ones per port/entry 
    TODO: Would be better to malloc these... */
  esp_tcp tcp_data;
  struct espconn encconn;
} tcp_table;

typedef struct __attribute__((packed))
{
    u8 ip1[4];
    volatile u8 no;
    volatile u8 result;
}PING_STRUCT;

extern PING_STRUCT ping;
//----------------------------------------------------------------------------
//Prototypes
void stack_encInterrupt (void);
void stack_updateIPs (void);
sint8 stack_register_tcp_accept(struct espconn *espconn, u8 stack_func);

u16  htons(u16 val);
u32 htons32(u32 val);
u32 stack_init (void);
void check_packet (void);
void eth_get_data (void);

void new_eth_header (u8 *,u32);

char arp_entry_search (u32);
void arp_reply (void);
void arp_entry_add (void);
char arp_request (u32);

void make_ip_header (u8 *,u32);
void icmp_send (u32,u8,u8,u16,u16);
u16 ICACHE_FLASH_ATTR checksum (u8 *pointer,u16 result16,u32 result32);

void udp_socket_process(void);

void tcp_entry_add (u8 *);
void tcp_socket_process(void);
char tcp_entry_search (u32 ,u16);
void tcp_Port_close (u8);
void tcp_port_open (u32, u16, u16);
void tcp_index_del (u8);
void create_new_tcp_packet(u16,u8);
void create_new_udp_packet(	u16,u16,u16,u32);

void find_and_start (u8 index);
void tcp_timer_call (void);
void arp_timer_call (void);
s8 add_tcp_app (u16, void(*fp1)(u8, u8), struct espconn *espconn);
void kill_tcp_app (u16 port);
void add_udp_app (u16, void(*fp1)(u8, u8));
void kill_udp_app (u16 port);
void change_port_tcp_app (u16, u16);
void pinging (void);

#define ETHER_OFFSET			0x00
#define ETHER_MACDEST			(ETHER_OFFSET)
#define ETHER_MACSRC			(ETHER_MACDEST+6)
#define ETHER_PKTYPE			(ETHER_MACSRC+6)

#define ARP_OFFSET				0x0E
#define ARP_HWTYPE        (ARP_OFFSET)
#define ARP_PRTYPE		    (ARP_HWTYPE+2)
#define ARP_HWLEN	        (ARP_PRTYPE+2)
#define ARP_PRLEN         (ARP_HWLEN+1)
#define ARP_OP            (ARP_PRLEN+1)
#define ARP_SHADDR        (ARP_OP+2)
#define ARP_SIPADDR       (ARP_SHADDR+6)
#define ARP_THADDR        (ARP_SIPADDR+4)
#define ARP_TIPADDR       (ARP_THADDR+6)

#define IP_OFFSET				  0x0E
#define IP_OFS_VER_LEN    (IP_OFFSET)
#define IP_OFS_TOS        (IP_OFS_VER_LEN+1)
#define IP_OFS_PKTLEN     (IP_OFS_TOS+1)
#define IP_OFS_ID         (IP_OFS_PKTLEN+2)
#define IP_OFS_FRAG_OFS   (IP_OFS_ID+2)
#define IP_OFS_TTL        (IP_OFS_FRAG_OFS+2)
#define IP_OFS_PROTO      (IP_OFS_TTL+1)
#define IP_OFS_HDR_CHK    (IP_OFS_PROTO+1)
#define IP_OFS_SRCADDR    (IP_OFS_HDR_CHK+2)
#define IP_OFS_DESTADDR   (IP_OFS_SRCADDR+4)

#define ICMP_OFFSET				0x22
#define ICMP_OFS_TYPE     (ICMP_OFFSET)
#define ICMP_OFS_CODE     (ICMP_OFS_TYPE+1)
#define ICMP_OFS_CHKSUM   (ICMP_OFS_CODE+1)
#define ICMP_OFS_ID       (ICMP_OFS_CHKSUM+2)
#define ICMP_OFS_SEQNO    (ICMP_OFS_ID+2)

#define ICMP_DATA				0x2A

#define TCP_OFFSET				  0x22
#define TCP_OFS_SCR_PORT    (TCP_OFFSET)
#define TCP_OFS_DST_PORT    (TCP_OFS_SCR_PORT+2)
#define TCP_OFS_SEQ_NUM     (TCP_OFS_DST_PORT+2)
#define TCP_OFS_ACK_NUM     (TCP_OFS_SEQ_NUM+4)
#define TCP_OFS_HDR_LEN     (TCP_OFS_ACK_NUM+4)
#define TCP_OFS_HDR_FLAGS   (TCP_OFS_HDR_LEN+1)
#define TCP_OFS_WINDOW      (TCP_OFS_HDR_FLAGS+1)
#define TCP_OFS_CHKSUM      (TCP_OFS_WINDOW+2)
#define TCP_OFS_URGENT_PTR  (TCP_OFS_CHKSUM+2)

#define UDP_OFFSET				0x22

extern arp_table arp_entry[MAX_ARP_ENTRY];
extern tcp_table tcp_entry[MAX_TCP_ENTRY+1];

//IP Protocol Types
#define	PROT_ICMP				0x01	//zeigt an die Nutzlasten enthalten das ICMP Prot
#define	PROT_TCP				0x06	//zeigt an die Nutzlasten enthalten das TCP Prot.
#define	PROT_UDP				0x11	//zeigt an die Nutzlasten enthalten das UDP Prot.	

//Defines für IF Abfrage
#define IF_MYIP 				(ip->IP_Destaddr==*((u32*)&myip[0]))
#define IP_UDP_PACKET 			(ip->IP_Proto == PROT_UDP)
#define IP_TCP_PACKET 			(ip->IP_Proto == PROT_TCP)
#define IP_ICMP_PACKET 			(ip->IP_Proto == PROT_ICMP)

//Defines für if Abfrage
#define ETHERNET_IP_DATAGRAMM	ethernet->EnetPacketType == 0x0008
#define ETHERNET_ARP_DATAGRAMM 	ethernet->EnetPacketType == 0x0608

#define IP_VERS_LEN 			20
#define TCP_HDR_LEN 			20
#define UDP_HDR_LEN				8
#define ETH_HDR_LEN 			14
#define TCP_DATA_START			(IP_VERS_LEN + TCP_HDR_LEN + ETH_HDR_LEN)
#define UDP_DATA_START			(IP_VERS_LEN + UDP_HDR_LEN + ETH_HDR_LEN)
#define UDP_DATA_END_VAR        (ETH_HDR_LEN + ((eth_buffer[IP_PKTLEN]<<8)+eth_buffer[IP_PKTLEN+1]) - UDP_HDR_LEN + 8)

#define	TCP_HDRFLAGS_FIX		0x2E
#define TCP_DATA_START_VAR		(ETH_HDR_LEN + IP_VERS_LEN + ((eth_buffer[TCP_HDRFLAGS_FIX] & 0xF0) >>2))

#define	IP_PKTLEN				0x10
#define TCP_DATA_END_VAR 		(ETH_HDR_LEN + IP_VERS_LEN + ((eth_buffer[IP_PKTLEN]<<8)+eth_buffer[IP_PKTLEN+1]) - ((eth_buffer[TCP_HDRFLAGS_FIX] & 0xF0) >>2))

//TCP Flags
#define  FIN_FLAG				0x01	//Datenübertragung beendet (finished)
#define  SYN_FLAG				0x02	//Verbindungs aufbauen (synchronize)
#define  RST_FLAG				0x04	//Verbindung neu initialisieren (reset)
#define  PSH_FLAG				0x08	//Datenübergabe an Anwendung beschleunigen (push)
#define  ACK_FLAG				0x10	//Datenübertragung bestätigen (acknowledged)

//----------------------------------------------------------------------------
//Aufbau eines Ethernetheader
//
//
//
//
typedef struct __attribute__((packed)) {
	u8 EnetPacketDest[6];	//MAC Zieladresse 6 Byte
	u8 EnetPacketSrc[6];	//MAC Quelladresse 6 Byte
	u16 EnetPacketType;		//Nutzlast 0x0800=IP Datagramm;0x0806 = ARP
} Ethernet_Header;

//----------------------------------------------------------------------------
//Aufbau eines ARP Header
//	
//	2 BYTE Hardware Typ				|	2 BYTE Protokoll Typ	
//	1 BYTE Länge Hardwareadresse	|	1 BYTE Länge Protokolladresse
//	2 BYTE Operation
//	6 BYTE MAC Adresse Absender		|	4 BYTE IP Adresse Absender
//	6 BYTE MAC Adresse Empfänger	|	4 BYTE IP Adresse Empfänger	
typedef struct __attribute__((packed)) {
		u16 ARP_HWType;		//Hardware Typ enthält den Code für Ethernet oder andere Link Layer
		u16 ARP_PRType;		//Protokoll Typ enthält den Code für IP o. anderes Übertragungsprotokoll
		u8 ARP_HWLen;		//Länge der Hardwareadresse enthält 6 für 6 Byte lange MAC Adressen
		u8 ARP_PRLen;		//Länge der Protocolladresse enthält 4 für 4 Byte lange IP Adressen
		u16 ARP_Op;			//Enthält Code der signalisiert ob es sich um eine Anfrage o. Antwort handelt
		u8 ARP_SHAddr[6];	//Enthält die MAC Adresse des Anfragenden  
		u32 ARP_SIPAddr;    //Enthält die IP Adresse des Absenders
		u8 ARP_THAddr[6];	//MAC Adresse Ziel, ist in diesem Fall 6 * 00,da die Adresse erst noch herausgefunden werden soll (ARP Request)
		u32 ARP_TIPAddr;    //IP Adresse enthält die IP Adresse zu der die Kommunikation aufgebaut werden soll 
} ARP_Header;

//----------------------------------------------------------------------------
//Aufbau eines IP Datagramms (B=BIT)
//	
//4B Version	|4B Headergr.	|8B Tos	|16B Gesamtlänge in Bytes	
//16B Identifikation			|3B Schalter	|13B Fragmentierungsposition
//8B Time to Live	|8B Protokoll	|16B Header Prüfsumme 
//32B IP Quelladresse
//32B IB Zieladresse
typedef struct __attribute__((packed)) {
	u8	IP_Vers_Len;	//4 BIT Die Versionsnummer von IP, 
									//meistens also 4 + 4Bit Headergröße 	
	u8	IP_Tos;			//Type of Service
	u16	IP_Pktlen;		//16 Bit Komplette Läng des IP Datagrams in Bytes
	u16	IP_Id;			//ID of the package for fragmentation 
									//reassembling
	u16	IP_Frag_Offset;	//wird benutzt um ein fragmentiertes 
									//IP Packet wieder korrekt zusammenzusetzen
	u8	IP_ttl;			//8 Bit Time to Live die lebenszeit eines Paket
	u8	IP_Proto;		//Zeigt das höherschichtige Protokoll an 
									//(TCP, UDP, ICMP)
	u16	IP_Hdr_Cksum;	//Checksumme des IP Headers
	u32	IP_Srcaddr;		//32 Bit IP Quelladresse
	u32	IP_Destaddr;	//32 Bit IP Zieladresse
} IP_Header;

//----------------------------------------------------------------------------
//Aufbau einer ICMP Nachricht
//	
//8 BIT Typ	|8 BIT Code	|16 BIT Prüfsumme	
//Je nach Typ			|Nachricht unterschiedlich
//Testdaten
//
//8 BIT Typ = 0 Echo Reply oder 8 Echo request	
typedef struct __attribute__((packed)) {
		u8     	ICMP_Type;		//8 bit typ identifiziert Aufgabe der Nachricht 
											//0 = Echo Reply 8 = Echo Request
		u8     	ICMP_Code;		//8 Bit Code enthält Detailangaben zu bestimmten Typen
		u16     	ICMP_Cksum;		//16 Bit Prüfsumme enthält die CRC Prüfsumme
		u16     	ICMP_Id;		//2 byte Identifizierung
		u16     	ICMP_SeqNum;	//Sequenznummer
} ICMP_Header;


//----------------------------------------------------------------------------
//TCP Header Layout
//
//
typedef struct __attribute__((packed)) {
	u16 	TCP_SrcPort;	//der Quellport für die Verbindung (Socket)
	u16 	TCP_DestPort;	//der Zielport für die Verbindung (Socket)
	u32	TCP_Seqnum;	//numerierter Bytestrom
	u32	TCP_Acknum;	//numerierter Bytestrom (Gegenrichtung)
	u8  TCP_Hdrlen;
	u8 	TCP_HdrFlags;//4Bit Länge des Headers in 32 Bit schritten rest Flags
	u16	TCP_Window;	//Fenstergröße max. 64K Byte
	u16	TCP_Chksum;	//Enthält eine Prüfsumme über Header und Datenbereich
	u16	TCP_UrgentPtr;	//16 Bit für dringende Übertragung
} TCP_Header;

//----------------------------------------------------------------------------
//UDP Header Layout
//
//
typedef struct __attribute__((packed)) {
	u16 	udp_SrcPort;	//der Quellport für die Verbindung (Socket)
	u16 	udp_DestPort;	//der Zielport für die Verbindung (Socket)
	u16   udp_Hdrlen;
	u16	udp_Chksum;	//Enthält eine Prüfsumme über Header und Datenbereich
} UDP_Header;

/* HTTDP specific functions */
void ICACHE_FLASH_ATTR serveHTTPD (u8 index, u8 port_index);
sint8 ICACHE_FLASH_ATTR stack_sendData(struct espconn *conn, uint8_t *dataIn, u16 data_length);
void ICACHE_FLASH_ATTR stack_startEthTask (void);
sint8 ICACHE_FLASH_ATTR stack_connDisconnect(struct espconn *conn);


#endif // #define _STACK_H
