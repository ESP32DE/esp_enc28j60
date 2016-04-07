/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        04.04.2016
Description:    user_main() - setup snippets

  The ENC28J60 functions are modified for use with the ESP8266, based off an original AVR 
  version by:
   Author:         Radig Ulrich mailto: mail@ulrichradig.de
   Version:        24.10.2007
  
  The HTTPD functionality was expanded to use the ENC28J60 with te ESP, but originally from 
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

-----------------------------------------------------------------------------------------*/
#include <esp8266.h>
#include "espmissingincludes.h"
#include "globals.h"
#include "auth.h"
#include "espfs.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "debug.h"
#include "timer.h"
#include "eeprom_sim.h"
#include "dhcpc.h"

static ETSTimer ethLoopTimer;
extern u32 my1secTime;


#ifdef ENC28J60
static void ICACHE_FLASH_ATTR ethLoopCb (void *arg) {
 
	static u32 time_old = ENC_RESET_TIMEOUT;
	
  os_timer_disarm(&ethLoopTimer);
	os_timer_setfn(&ethLoopTimer, ethLoopCb, NULL);
  
	eth_get_data();

	//Ethernet OK?
	if(my1secTime > time_old)
	{
		if(eth.no_reset)
		{
			eth.no_reset = 0;
		}
		else
		{
      INFO("RST ");
			ETS_GPIO_INTR_DISABLE();
			enc_init();
			enc28j60_led_blink (0);
			ETS_GPIO_INTR_ENABLE();
		}
		time_old = my1secTime+ENC_RESET_TIMEOUT;
	}
  
  os_timer_arm(&ethLoopTimer, 2, 0);	
}  
#endif


/* Sets up the GPIO, and the interrupt function call from the ENC */
void ICACHE_FLASH_ATTR ioInit() {
  uint32 gpio_status;   
  
  #ifdef ENC28J60
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);    // enc reset
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);    // enc interrupt 
  
  GPIO_DIS_OUTPUT(ENCINTGPIO);
  PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
  ETS_GPIO_INTR_DISABLE();
  ETS_GPIO_INTR_ATTACH(stack_encInterrupt,ENCINTGPIO);
  
  GPIO_REG_WRITE(GPIO_STATUS_W1TS_ADDRESS, BIT(ENCINTGPIO));
  gpio_pin_intr_state_set(GPIO_ID_PIN(ENCINTGPIO),GPIO_PIN_INTR_NEGEDGE);
  gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
  ETS_GPIO_INTR_ENABLE();
  #endif
  INFO("end io init\r\n");
}

void ICACHE_FLASH_ATTR user_main_setupContinued (void) {  
  // 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
  // where image is written in flash that is defined in Makefile.
  espFsInit((void*)(0x40200000 + ESPFS_POS));
  
  httpdInit(builtInUrls, 80);
 
  #ifdef SHOW_HEAP_USE
    os_timer_disarm(&prHeapTimer);
    os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
    os_timer_arm(&prHeapTimer, 3000, 1);
  #endif
  
  #ifdef ENC28J60
    // this is like a while loop - but this is the closest we can get to that on the esp
    os_timer_disarm(&ethLoopTimer);
    os_timer_setfn(&ethLoopTimer, ethLoopCb, NULL);
    os_timer_arm(&ethLoopTimer, 2, 0);
    enc28j60_led_blink (1);
  #endif
 
  os_printf("\nReady\n");
}

// Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) { 
	uart_init(BIT_RATE_115200, BIT_RATE_115200);	
   
  #ifdef ENC28J60
    /* For testing switch off wifi, just focussing on ENC */
    wifi_station_set_auto_connect(FALSE);
    
    //Application inits
    INFO("\nEEPROM SIM INIT\n");
    eeprom_sim_init();
    
    INFO("\nSTACK INIT\n");	
    stack_init();
    
    INFO("\nIO INIT\n");
    ioInit();
    
    #ifdef USE_DHCP
      /* DHCP will negotiate, and once done call user_main_setupContinued() */
      INFO("\nDHCP INIT\n");
      dhcp_init();      
    #else
      INFO("\nSETUP CONT.\n");
      user_main_setupContinued();
    #endif 	
}
