#include "platform.h"

#include "systime.h"
#include "gpio.h"
#include "led.h"

#include "sniff.h"
#include "usb_uart_cdc.h"

int main(void)
{
	SystemInit();

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= (0x02 << 24); // only swj

	systime_init();
	led_init();

	usb_uart_cdc_init();

	led_set(0, LED_3BLINK);	
	sniff_init();
	
	while (1) {
		systime_periodic();
		led_periodic();
		sniff_periodic();
	}
} 
