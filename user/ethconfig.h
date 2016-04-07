/*----------------------------------------------------------------------------
 Author:         Mark F (Cicero-MF) mark@cdelec.co.za
 Remarks:        
 Version:        06.03.2016
 Description:    Config file for Ethernet parameters

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

#ifndef _ETHCONFIG_H_
	#define _ETHCONFIG_H_	

	/* Conversion from IP to u32 */
	#define IP(a,b,c,d) ((u32)(d)<<24)+((u32)(c)<<16)+((u32)(b)<<8)+a

	/* IP's */
	#define MYIP		IP(192,168,0,200)
    #define ROUTER_IP	IP(192,168,0,1)	
    #define NETMASK		IP(255,255,255,0)
		
	/* MAC Address for ENC - ENC specific 
    TODO:16/03/2016:Allow to pull MAC from ESP - if user wants either or operation
      in terms of wifi or ethernet (saves cost and hassle of MAC programming)*/
	#define MYMAC1	0x00
	#define MYMAC2	0x11
	#define MYMAC3	0x22
	#define MYMAC4	0x33	
	#define MYMAC5	0x44
	#define MYMAC6	0x55
    
#endif //_ETHCONFIG_H_


