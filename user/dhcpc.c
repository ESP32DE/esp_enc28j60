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
#include "esp8266.h"
#include "globals.h"
#include "config.h"
#include "stack.h"
#include "timer.h"
#include "dhcpc.h"
	
#ifdef USE_DHCP

struct dhcp_msg
{
	u8 op;              //1
	u8 htype;           //2
	u8 hlen;            //3
	u8 hops;            //4
	u8 xid[4];          //8
	u16  secs;          //12
	u16  flags;         //14
	u8 ciaddr[4];       //16
	u8 yiaddr[4];       //20
	u8 siaddr[4];       //24
	u8 giaddr[4];       //28
	u8 chaddr[16];      //44
	u8 sname[64];       //108
	u8 file[128];       //Up to here 236 Bytes
	u8 options[312];    // receive up to 312 bytes
};

struct dhcp_cache
{
	u8 type;
	u8 ovld;
	u8 router_ip[4];
	u8 dns1_ip  [4];
	u8 dns2_ip  [4];
	u8 netmask  [4];
	u8 lease    [4];
	u8 serv_id  [4];
};

//DHCP Message types for Option 53
#define DHCPDISCOVER   1     // client -> server
#define DHCPOFFER      2     // server -> client
#define DHCPREQUEST    3     // client -> server
#define DHCPDECLINE    4     // client -> server
#define DHCPACK        5     // server -> client
#define DHCPNAK        6     // server -> client
#define DHCPRELEASE    7     // client -> server
#define DHCPINFORM     8     // client -> server

u8 dhcp_state;
#define DHCP_STATE_IDLE             0
#define DHCP_STATE_DISCOVER_SENT    1
#define DHCP_STATE_OFFER_RCVD       2
#define DHCP_STATE_SEND_REQUEST     3
#define DHCP_STATE_REQUEST_SENT     4
#define DHCP_STATE_ACK_RCVD         5
#define DHCP_STATE_NAK_RCVD         6
#define DHCP_STATE_ERR              7
#define DHCP_STATE_FINISHED         8

struct dhcp_cache cache; 
volatile u32 dhcp_lease;
u8 timeout_cnt;

static ETSTimer dhcpCallTimer;

//----------------------------------------------------------------------------
//Init of DHCP client port
void ICACHE_FLASH_ATTR dhcp_init (void)
{
  u8 v;
	
  // Add function for DHCP packets
	add_udp_app (DHCP_CLIENT_PORT, (void(*)(u8,u8))dhcp_get);
	dhcp_state = DHCP_STATE_IDLE;
  timeout_cnt = 0;
  
  os_timer_disarm(&dhcpCallTimer);
	os_timer_setfn(&dhcpCallTimer, check_dhcp, NULL);
  os_timer_arm(&dhcpCallTimer, 5, 0);  
	return;
}
//----------------------------------------------------------------------------

void ICACHE_FLASH_ATTR check_dhcp (void *arg) {
  u8 retVal;
  os_timer_disarm(&dhcpCallTimer);
  
  retVal = dhcp();
  if (retVal == DHCP_SUCCESS) {
    stack_updateIPs();
    stack_startEthTask();
    //user_main_setupContinued ();
  } else if (retVal == DHCP_CONTINUE) {
    os_timer_setfn(&dhcpCallTimer, check_dhcp, NULL);
    os_timer_arm(&dhcpCallTimer, 2, 0);
  } else {
      DHCP_DEBUG("DHCP fail, not updating IP's, continuing with default\r\n");
      // keep regular IP's 
      stack_startEthTask();
      //user_main_setupContinued ();
  } 
}
// Configure this client by DHCP
u8 ICACHE_FLASH_ATTR dhcp (void)
{ 
	if ( (dhcp_state == DHCP_STATE_FINISHED ) &&
       (dhcp_lease < 600) )
  {
      dhcp_state = DHCP_STATE_SEND_REQUEST;
  } 
	
  if ( dhcp_state != DHCP_STATE_FINISHED ) {
    if ( timeout_cnt > 3 )
    {
      dhcp_state = DHCP_STATE_ERR;
      DHCP_DEBUG("DHCP timeout\r\n");
      return DHCP_TIMEOUT;
    }
    switch (dhcp_state)
    {
      case DHCP_STATE_IDLE:
        dhcp_message(DHCPDISCOVER);        
        gp_timer = 5;
      break;
      case DHCP_STATE_DISCOVER_SENT:
        if (gp_timer == 0) 
        {
          dhcp_state = DHCP_STATE_IDLE;
          timeout_cnt++;
        }
      break;
      case DHCP_STATE_OFFER_RCVD:
        timeout_cnt = 0;
				dhcp_state = DHCP_STATE_SEND_REQUEST;
      break;
      case DHCP_STATE_SEND_REQUEST:
        gp_timer  = 5;
        dhcp_message(DHCPREQUEST);
      break;
      case DHCP_STATE_REQUEST_SENT:
        if (gp_timer == 0) 
        {
          dhcp_state = DHCP_STATE_SEND_REQUEST;
          timeout_cnt++;
        }
      break;
      case DHCP_STATE_ACK_RCVD:
        DHCP_DEBUG("LEASE %2x%2x%2x%2x\r\n", cache.lease[0],cache.lease[1],cache.lease[2],cache.lease[3]);
		
        dhcp_lease = (u32)cache.lease[0] << 24 | (u32)cache.lease[1] << 16 | (u32)cache.lease[2] <<  8 |(u32)cache.lease[3];
        
        #ifdef IPS_IN_UNION
          sysCfg.netmask.theint               = (*((u32*)&cache.netmask[0]));
          sysCfg.router_ip.theint             = (*((u32*)&cache.router_ip[0]));
          #ifdef USE_DNS
            sysCfg.dns_server_ip.theint       = (*((u32*)&cache.dns1_ip[0]));
          #endif
        #else
          (*((u32*)&sysCfg.netmask[0]))       = (*((u32*)&cache.netmask[0]));
          (*((u32*)&sysCfg.router_ip[0]))     = (*((u32*)&cache.router_ip[0]));
          #ifdef USE_DNS
          (*((u32*)&sysCfg.dns_server_ip[0])) = (*((u32*)&cache.dns1_ip[0]));
          #endif
        #endif
        dhcp_state = DHCP_STATE_FINISHED;
        DHCP_DEBUG("FROM DHCP: \r\n");
        #ifdef IPS_IN_UNION           
          DHCP_DEBUG("My IP: %d.%d.%d.%d\r\n",sysCfg.ethip.thech[0],sysCfg.ethip.thech[1],sysCfg.ethip.thech[2],sysCfg.ethip.thech[3]);
          DHCP_DEBUG("MASK %d.%d.%d.%d\r\n", sysCfg.netmask.thech[0]  , sysCfg.netmask.thech[1]  , sysCfg.netmask.thech[2]  , sysCfg.netmask.thech[3]);
          DHCP_DEBUG("Router IP: %d.%d.%d.%d\r\n",sysCfg.router_ip.thech[0],sysCfg.router_ip.thech[1],sysCfg.router_ip.thech[2],sysCfg.router_ip.thech[3]);
          #ifdef USE_DNS
            DHCP_DEBUG("DNS IP: %d.%d.%d.%d\r\n",sysCfg.dns_server_ip.thech[0],sysCfg.dns_server_ip.thech[1],sysCfg.dns_server_ip.thech[2],sysCfg.dns_server_ip.thech[3]);
          #endif
        #else
          DHCP_DEBUG("My IP: %d.%d.%d.%d\r\n",sysCfg.ethip[0], sysCfg.ethip[1], sysCfg.ethip[2], sysCfg.ethip[3]);
          DHCP_DEBUG("MASK %d.%d.%d.%d\r\n", sysCfg.netmask[0], sysCfg.netmask[1], sysCfg.netmask[2], sysCfg.netmask[3]);
          DHCP_DEBUG("Router IP: %d.%d.%d.%d\r\n",sysCfg.router_ip[0], sysCfg.router_ip[1], sysCfg.router_ip[2],sysCfg.router_ip[3]);
          #ifdef USE_DNS
            DHCP_DEBUG("DNS IP: %d.%d.%d.%d\r\n",sysCfg.dns_server_ip[0], sysCfg.dns_server_ip[1], sysCfg.dns_server_ip[2], sysCfg.dns_server_ip[3]);
          #endif
        #endif
        return DHCP_SUCCESS;
      break;
      case DHCP_STATE_NAK_RCVD:
        dhcp_state = DHCP_STATE_IDLE;
      break;
    }
    eth_get_data();
  }
  return DHCP_CONTINUE;
}

//----------------------------------------------------------------------------
//Send DHCP messages as Broadcast
void ICACHE_FLASH_ATTR dhcp_message (u8 type)
{
  struct dhcp_msg *msg;
  u8   *options;
  
  for (u16 i=0; i < sizeof (struct dhcp_msg); i++) //clear eth_buffer to 0
  {
    eth_buffer[UDP_DATA_START+i] = 0;
  }
  
  msg = (struct dhcp_msg *)&eth_buffer[UDP_DATA_START];
  msg->op          = 1; // BOOTREQUEST
  msg->htype       = 1; // Ethernet
  msg->hlen        = 6; // Ethernet MAC
  #ifdef USE_SEPARATE_ENC_MAC
    msg->xid[0]      = MYMAC6; //use the MAC as the ID to be unique in the LAN
    msg->xid[1]      = MYMAC5;
    msg->xid[2]      = MYMAC4;
    msg->xid[3]      = MYMAC3;
    
    msg->chaddr[0]   = MYMAC1;
    msg->chaddr[1]   = MYMAC2;
    msg->chaddr[2]   = MYMAC3;
    msg->chaddr[3]   = MYMAC4;
    msg->chaddr[4]   = MYMAC5;
    msg->chaddr[5]   = MYMAC6;
  #else
    msg->xid[0]      = mymac[5]; 
    msg->xid[1]      = mymac[4];
    msg->xid[2]      = mymac[3];
    msg->xid[3]      = mymac[2];
    
    msg->chaddr[0]   = mymac[0];
    msg->chaddr[1]   = mymac[1];
    msg->chaddr[2]   = mymac[2];
    msg->chaddr[3]   = mymac[3];
    msg->chaddr[4]   = mymac[4];
    msg->chaddr[5]   = mymac[5];
	#endif
  msg->flags       = HTONS(0x8000);
  
  options = &msg->options[0];  //options
  *options++       = 99;       //magic cookie
  *options++       = 130;
  *options++       = 83;
  *options++       = 99;

  *options++       = 53;    // Option 53: DHCP message type DHCP Discover
  *options++       = 1;     // len = 1
  *options++       = type;  // 1 = DHCP Discover
  
  *options++       = 55;    // Option 55: parameter request list
  *options++       = 3;     // len = 3
  *options++       = 1;     // netmask
  *options++       = 3;     // router
  *options++       = 6;     // dns

  *options++       = 50;    // Option 54: requested IP
  *options++       = 4;     // len = 4
  #ifdef IPS_IN_UNION
    *options++       = sysCfg.ethip.thech[0];
    *options++       = sysCfg.ethip.thech[1];
    *options++       = sysCfg.ethip.thech[2];
    *options++       = sysCfg.ethip.thech[3];
  #else
    *options++       = sysCfg.ethip[0];
    *options++       = sysCfg.ethip[1];
    *options++       = sysCfg.ethip[2];
    *options++       = sysCfg.ethip[3];
  #endif

  switch (type)
  {
    case DHCPDISCOVER:
      dhcp_state = DHCP_STATE_DISCOVER_SENT;
      DHCP_DEBUG("DISCOVER sent\r\n");
    break;
    case DHCPREQUEST:
      *options++       = 54;    // Option 54: server ID
      *options++       = 4;     // len = 4
      *options++       = cache.serv_id[0];
      *options++       = cache.serv_id[1];
      *options++       = cache.serv_id[2];
      *options++       = cache.serv_id[3];
      dhcp_state = DHCP_STATE_REQUEST_SENT;
      DHCP_DEBUG("REQUEST sent\r\n");
    break;
    default:
      DHCP_DEBUG("Wrong DHCP msg type\r\n");
    break;
  }

  *options++       = 12;    // Option 12: host name
  *options++       = 9;     // len = 8
  *options++       = 'E';
  *options++       = 'S';
  *options++       = 'P';
  *options++       = '-';
  *options++       = 'E';
  *options++       = 'N';
  *options++       = 'C';
  *options++       = '0';
  *options++       = '0';
  
  *options         = 0xff;  //end option

  create_new_udp_packet(sizeof (struct dhcp_msg),DHCP_CLIENT_PORT,DHCP_SERVER_PORT,(u32)0xffffffff);
}
//----------------------------------------------------------------------------
//liest 4 bytes aus einem buffer und speichert in dem anderen
void ICACHE_FLASH_ATTR get4bytes (u8 *source, u8 *target)
{
  u8 i;
  
  for (i=0; i<4; i++)
  {
    *target++ = *source++;
  }
}
//----------------------------------------------------------------------------
//read all the options
//pointer to the variables and size from options to packet end
void ICACHE_FLASH_ATTR dhcp_parse_options (u8 *msg, struct dhcp_cache *c, u16 size)
{
  u16 ix;

  ix = 0;
  do
  {
    switch (msg[ix])
    {
      case 0: //Padding
      ix++;
      break;
      case 1: //Netmask
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->netmask[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 3: //router (gateway IP)
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->router_ip[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 6: //dns len = n * 4
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->dns1_ip[0]);
          ix += 4;
        }
        else
        if ( msg[ix] == 8 )
        {
          ix++;
          get4bytes(&msg[ix], &c->dns1_ip[0]);
          ix += 4;
          get4bytes(&msg[ix], &c->dns2_ip[0]);
          ix += 4;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 51: //lease time
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->lease[0]);
          ix += 4;
        }
        else
        {
          ix += msg[ix]+1;
        }
      break;
      case 52: //Options overload 
        ix++;
        if ( msg[ix] == 1 )   //size == 1
        {
          ix++;
          c->ovld   = msg[ix];
          ix++;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 53: //DHCP Type 
        ix++;
        if ( msg[ix] == 1 )   //size == 1
        {
          ix++;
          c->type = msg[ix];
          ix++;
        }
        else
        {
          ix += (msg[ix]+1);
        }
      break;
      case 54: //Server identifier
        ix++;
        if ( msg[ix] == 4 )
        {
          ix++;
          get4bytes(&msg[ix], &c->serv_id[0]);
          ix += 4;
        }
        else
        {
          ix += msg[ix]+1;
        }
      break;
      case 99:   //Magic cookie
        ix += 4;
      break;
      case 0xff: //end option
      break;
      default: 
        DHCP_DEBUG("Unknown Option: %2x\r\n", msg[ix]);
        ix++;
        ix += (msg[ix]+1);
      break;
    }
  }
  while ( (msg[ix] != 0xff) && (ix < size) ); 
}

//----------------------------------------------------------------------------
// Evaluates message out by DHCP server
// DHCP packets: 20 Bytes IP Header, 8 Bytes UDP_Header,
// DHCP fixed fields 236 Bytes, min 312 Bytes options -> 576 Bytes min.

void ICACHE_FLASH_ATTR dhcp_get (u8 index, u8 port_index)
{
  struct dhcp_msg  *msg;
  IP_Header *ip;
  u8 *p;
  u16 i;

  ip  = (IP_Header *)&eth_buffer[IP_OFFSET];
  DHCP_DEBUG("In DHCP get\r\n");  
  
  if ( htons(ip->IP_Pktlen) > MTU_SIZE )
  {
    DHCP_DEBUG("DHCP too big, discarded\r\n");
    return;
  }

  p = &cache.type; //clear the cache
  for (i=0; i<sizeof(cache); i++)
  {
    p[i] = 0;
  }

  // set pointer of DHCP message to beginning of UDP data
  msg = (struct dhcp_msg *)&eth_buffer[UDP_DATA_START];
  #ifdef USE_SEPARATE_ENC_MAC
    //check the id
    if ( (msg->xid[0] != MYMAC6) ||
         (msg->xid[1] != MYMAC5) ||
         (msg->xid[2] != MYMAC4) ||
         (msg->xid[3] != MYMAC3)    )
    {
      DHCP_DEBUG("Wrong DHCP ID, discarded\r\n");
      return;
    }
  #else
    //check the id
    if ( (msg->xid[0] != mymac[5]) ||
         (msg->xid[1] != mymac[4]) ||
         (msg->xid[2] != mymac[3]) ||
         (msg->xid[3] != mymac[2])    )
    {
      DHCP_DEBUG("Wrong DHCP ID, discarded\r\n");
      return;
    }
	#endif


  dhcp_parse_options(&msg->options[0], &cache, (htons(ip->IP_Pktlen)-264) );
  // check if file field or sname field are overloaded (option 52)
  switch (cache.ovld) 
  {
    case 0:  // no overload, do nothing
    break;
    case 1:  // the file field contains options
      dhcp_parse_options(&msg->file[0], &cache, 128);
    break;
    case 2:  // the sname field contains options
      dhcp_parse_options(&msg->sname[0], &cache, 64);
    break;
    case 3:  // the file and the sname field contain options
      dhcp_parse_options(&msg->file[0], &cache, 128);
      dhcp_parse_options(&msg->sname[0], &cache, 64);
    break;
    default: // must not occur
      DHCP_DEBUG("Option 52 Error\r\n");
    break;
  }

  switch (cache.type)
  {
    case DHCPOFFER:
      // this will be our IP address
      #ifdef IPS_IN_UNION    
        memcpy(sysCfg.ethip.thech, msg->yiaddr ,4);
      #else
        (*((u32*)&sysCfg.ethip[0])) = (*((u32*)&msg->yiaddr[0]));
      #endif
      
      DHCP_DEBUG("** DHCP OFFER RECVD! **\r\n");
      
      dhcp_state = DHCP_STATE_OFFER_RCVD;
    break;
    case DHCPACK:
      DHCP_DEBUG("** DHCP ACK RECVD! **\r\n");
      dhcp_state = DHCP_STATE_ACK_RCVD;
    break;
    case DHCPNAK:
      DHCP_DEBUG("** DHCP NAK RECVD! **\r\n");
      dhcp_state = DHCP_STATE_NAK_RCVD;
    break;
  }
}

#endif
