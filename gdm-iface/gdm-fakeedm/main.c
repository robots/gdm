#include "platform.h"

#include "systime.h"
#include "gpio.h"
#include "led.h"

#include "gdm.h"

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

	gdm_init();

	while (1) {
		systime_periodic();
		led_periodic();
		gdm_periodic();
	}
} 
