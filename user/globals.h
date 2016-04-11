#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "types.h"
#include "driver/uart.h"

  #define ENC28J60
  #ifdef ENC28J60 
    #define USE_DHCP
  #endif

  //#define USE_DNS
  
  #define ENC_RESET_TIMEOUT 10
  

#endif /* _GLOBALS_H */
