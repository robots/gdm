#ifndef BSP_SPI_h_
#define BSP_SPI_h_

#include "gpio.h"

extern const struct gpio_init_table_t spi_gpio[];
extern const struct gpio_init_table_t spi_cs_gpio[];

extern const int spi_gpio_cnt;
extern const int spi_cs_gpio_cnt;

#endif

