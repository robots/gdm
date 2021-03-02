#ifndef GPIO_h_
#define GPIO_h_

#include "platform.h"

enum gpio_pin_t {
	GPIO_Pin_0 = 0x0001,
	GPIO_Pin_1 = 0x0002,
	GPIO_Pin_2 = 0x0004,
	GPIO_Pin_3 = 0x0008,
	GPIO_Pin_4 = 0x0010,
	GPIO_Pin_5 = 0x0020,
	GPIO_Pin_6 = 0x0040,
	GPIO_Pin_7 = 0x0080,
	GPIO_Pin_8 = 0x0100,
	GPIO_Pin_9 = 0x0200,
	GPIO_Pin_10 = 0x0400,
	GPIO_Pin_11 = 0x0800,
	GPIO_Pin_12 = 0x1000,
	GPIO_Pin_13 = 0x2000,
	GPIO_Pin_14 = 0x4000,
	GPIO_Pin_15 = 0x8000,
};

enum gpio_mode_t
{
	GPIO_MODE_AIN = 0x0,
  GPIO_MODE_IN_FLOATING = 0x04,
  GPIO_MODE_IPD = 0x28,
  GPIO_MODE_IPU = 0x48,
  GPIO_MODE_OUT_OD = 0x14,
  GPIO_MODE_OUT_PP = 0x10,
  GPIO_MODE_AF_OD = 0x1C,
  GPIO_MODE_AF_PP = 0x18
};

enum gpio_speed_t
{ 
  GPIO_SPEED_LOW = 1,
  GPIO_SPEED_MED, 
  GPIO_SPEED_HIGH
};

enum gpio_state_t {
	GPIO_DEFAULT=0,
	GPIO_RESET,
	GPIO_SET,
};

struct gpio_init_table_t {
	GPIO_TypeDef* gpio;

	enum gpio_pin_t pin;
	enum gpio_mode_t mode;
	enum gpio_speed_t speed;
#if defined(STM32F4XX) || defined(STM32F042)
	GPIOPuPd_TypeDef pupd;
	GPIOOType_TypeDef otype;
	uint16_t af;
#endif
	enum gpio_state_t state;
};

void gpio_init(const struct gpio_init_table_t *t, int num);
uint8_t gpio_wait_state(const struct gpio_init_table_t *gpio, uint8_t state);
void gpio_set(const struct gpio_init_table_t *gpio, uint8_t state);
uint8_t gpio_get(const struct gpio_init_table_t *gpio);

#endif
