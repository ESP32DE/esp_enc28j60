/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za
Remarks:        
Version:        04.04.2016
Description:    Ethernet device driver for the enc28j60 and an esp8266

  This code was modified for use with the ESP8266, based off a version by Radig Ulrich  
     Author:         Radig Ulrich mailto: mail@ulrichradig.de
     Remarks:
     known Problems: none
     Version:        24.10.2007
     Description:    Timer Routinen

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
#ifndef _TIMER_H
	#define _TIMER_H

	#include "stack.h"

	#define RESET_TIME 86400

  #ifdef USE_DHCP
    extern volatile u8 gp_timer;
  #endif
	void timer_init (void);
	 
#endif //_TIMER_H

