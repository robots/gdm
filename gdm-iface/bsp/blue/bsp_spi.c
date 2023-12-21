#include "platform.h"

#include "gpio.h"
#include "bsp_spi.h"

const struct gpio_init_table_t spi_gpio[] = {
	{ // SCK 
		.gpio = GPIOA,
		.pin = GPIO_Pin_5,
		.mode = GPIO_Mode_AF_PP,
		.speed = GPIO_Speed_50MHz,
	},
	{ // MOSI
		.gpio = GPIOA,
		.pin = GPIO_Pin_7,
		.mode = GPIO_Mode_AF_PP,
		.speed = GPIO_Speed_50MHz,
	},
	{ // MISO
		.gpio = GPIOA,
		.pin = GPIO_Pin_6,
		.mode = GPIO_Mode_IN_FLOATING,
		.speed = GPIO_Speed_50MHz,
	},
};

const struct gpio_init_table_t spi_cs_gpio[] = {
	{ // CS0 
		.gpio = GPIOA,
		.pin = GPIO_Pin_4,
		.mode = GPIO_Mode_Out_PP,
		.speed = GPIO_Speed_50MHz,
		.state = GPIO_SET,
	},
};

const int spi_gpio_cnt = ARRAY_SIZE(spi_gpio);
const int spi_cs_gpio_cnt = ARRAY_SIZE(spi_cs_gpio);

