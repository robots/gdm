#include "platform.h"

#include "systime.h"
#include "gpio.h"
#include "led.h"

#include "cmd.h"
#include "ub.h"
#include "usb_uart_cdc.h"

const struct gpio_init_table_t main_gpio[] = {
	{
		.gpio = GPIOA,
		.pin = 0xffff,
		.mode = GPIO_MODE_AIN,
	},
	{
		.gpio = GPIOB,
		.pin = 0xffff,
		.mode = GPIO_MODE_AIN,
	},
	{
		.gpio = GPIOC,
		.pin = 0xffff,
		.mode = GPIO_MODE_AIN,
	},
};

int main(void)
{
	SystemInit();

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= (0x02 << 24); // only swj

	gpio_init(main_gpio, ARRAY_SIZE(main_gpio));

	systime_init();
	led_init();

	usb_uart_cdc_init();

	cmd_init();
	ub_init();

	led_set(0, LED_3BLINK);	
	
	while (1) {
		systime_periodic();
		led_periodic();
		cmd_periodic();
		ub_periodic();
	}
} 
