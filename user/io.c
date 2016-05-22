
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "globals.h"
#include "io.h"
#include "stack.h"

#define LEDGPIO 2
#define BTNGPIO 0

static ETSTimer resetBtntimer;

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better mactos
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;
	} else {
		if (resetCnt>=6) { //3 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(0x3); //reset to AP+STA mode
			os_printf("Reset to AP mode. Restarting system...\n");
			system_restart();
		}
		resetCnt=0;
	}
}

void ICACHE_FLASH_ATTR gpio16_output_conf(void) {
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                 (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

  WRITE_PERI_REG(RTC_GPIO_CONF,
                 (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
}

void ICACHE_FLASH_ATTR gpio16_output_set(uint8 value) {
  WRITE_PERI_REG(RTC_GPIO_OUT,
                 (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(value & 1));
}

void ioInit() {
	uint32 gpio_status;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTNGPIO));
	os_timer_disarm(&resetBtntimer);
	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
	os_timer_arm(&resetBtntimer, 500, 1);
  
  /* map GPIO16 as an I/O pin - for resetting the ESP
  gpio_status = READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc;
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF, gpio_status | 0x00000001);
  gpio_status = READ_PERI_REG(RTC_GPIO_CONF) & 0xfffffffe;
  WRITE_PERI_REG(RTC_GPIO_CONF, gpio_status | 0x00000000);*/
  gpio16_output_set(1);
  gpio16_output_conf();
  gpio16_output_set(1);
  
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
}

