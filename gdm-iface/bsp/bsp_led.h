#ifndef BSP_LED_h_
#define BSP_LED_h_

#include "gpio.h"

extern const struct gpio_init_table_t led_gpio[];

extern const uint8_t led_pol[];

extern const uint32_t led_gpio_cnt;

#endif
