/*-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        18.05.2016
Description:    Mainly just global defines for debugging

-----------------------------------------------------------------------------------------*/
#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "user_config.h"
#include "types.h"

/* DEBUG UART DEFINES ---------------------------------------------------------
  Please keep in mind these slow down proceedings and can, in some cases 
  if too many debug outputs, cause watchdog resets from the SDK 
*/

	#define ENC_DEBUG(...)
	//#define ENC_DEBUG os_printf
  
  //#define SPI_DEBUG(...)
	#define SPI_DEBUG os_printf
  
  //#define CONFIG_DEBUG(...)
	#define CONFIG_DEBUG os_printf

  //#define TIMER_DEBUG os_printf
  #define TIMER_DEBUG(...)
  
  //#define STACK_DEBUG os_printf
  #define STACK_DEBUG(...)
  
  #define DHCP_DEBUG os_printf
	//#define DHCP_DEBUG(...)

  #define MAIN_DEBUG os_printf
	//#define MAIN_DEBUG(...)

  // #define CAPTDNS_DEBUG os_printf
	#define CAPTDNS_DEBUG(...)

  #define IPS_IN_UNION
  #define MQTT_QUEUE_SETBUF
  #define HAVE_COMMS
  #define ENC28J60
  #define USE_DHCP

  #define USE_DNS
  
  
  #ifdef MQTT_KEEPALIVE
    #define ENC_RESET_TIMEOUT (MQTT_KEEPALIVE+5)
  #else
    #define ENC_RESET_TIMEOUT (30)
  #endif
   
  
#endif /* _GLOBALS_H */
