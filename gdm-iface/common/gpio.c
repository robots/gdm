
#include "platform.h"

#include "gpio.h"


void gpio_init(const struct gpio_init_table_t *t, int num)
{
	int i;
#if defined(STM32F4XX) || defined(STM32F0XX)
	uint32_t reg;
	for (i = 0; i < num; i++) {
		uint16_t pos = 0;

		gpio_set(&t[i], t[i].state);

		for (pos = 0; pos < 16; pos++) {
			if (!(t[i].pin & (1 << pos))) {
				continue;
			}

			if (t[i].af) {
				uint32_t idx = ((uint32_t)(pos & (uint32_t)0x07U) * 4U);
				reg = t[i].gpio->AFR[pos >> 3];
				reg &= ~((uint32_t)0xFU << idx) ;
				reg |= ((uint32_t)(t[i].af) << idx);
				t[i].gpio->AFR[pos >> 3] = reg;
			}

			if ((t[i].mode == GPIO_Mode_OUT) || (t[i].mode == GPIO_Mode_AF)) {
				reg = t[i].gpio->OSPEEDR;
				reg &= ~(3 << (pos * 2));
				reg |= (t[i].speed << (pos * 2));
				t[i].gpio->OSPEEDR = reg;

				reg = t[i].gpio->OTYPER;
				reg &= ~(1 << pos);
				reg |= (t[i].otype << pos);
				t[i].gpio->OTYPER = (uint16_t)reg;
			}

			reg = t[i].gpio->MODER;
			reg &= ~(3 << (pos * 2));
			reg |= (t[i].mode << (pos * 2));
			t[i].gpio->MODER = reg;

			reg = t[i].gpio->PUPDR;
			reg &= ~(3 << (pos * 2));
			reg |= (t[i].pupd << (pos * 2));
			t[i].gpio->PUPDR = reg;
		}
	}
#else
	for (i = 0; i < num; i++) {
		uint16_t pos = 0;
		uint32_t mode;

		mode = t[i].mode & 0xf;
		if (t[i].mode & 0x10) {
			mode |= t[i].speed;
		}
			
		if (!(t[i].mode == GPIO_MODE_AIN || t[i].mode == GPIO_MODE_IN_FLOATING || t[i].mode == GPIO_MODE_IPD || t[i].mode == GPIO_MODE_IPU)) {
			gpio_set(&t[i], t[i].state);
		}

		if (t[i].pin & 0xff) {
			uint32_t cr;
			cr = t[i].gpio->CRL;
			
			for (pos = 0; pos < 8; pos++) {
				if (!(t[i].pin & (1 << pos))) {
					continue;
				}

				uint32_t pinmask = 0x0f << (4*pos);

				cr &= ~pinmask;
				cr |= mode << (4*pos);

				if (t[i].mode == GPIO_MODE_IPD) {
					t[i].gpio->BRR = 0x01 << pos;
				} else if (t[i].mode == GPIO_MODE_IPU) {
					t[i].gpio->BSRR = 0x01 << pos;
				}
			}

			t[i].gpio->CRL = cr;
		}

		if (t[i].pin & 0xff00) {
			uint32_t cr;
			cr = t[i].gpio->CRH;
			
			for (pos = 0; pos < 8; pos++) {
				if (!(t[i].pin & (1 << (pos+8)))) {
					continue;
				}

				uint32_t pinmask = 0x0f << (4*pos);

				cr &= ~pinmask;
				cr |= mode << (4*pos);

				if (t[i].mode == GPIO_MODE_IPD) {
					t[i].gpio->BRR = 0x01 << pos;
				} else if (t[i].mode == GPIO_MODE_IPU) {
					t[i].gpio->BSRR = 0x01 << pos;
				}
			}

			t[i].gpio->CRH = cr;
		}
	}
#endif
}

uint8_t gpio_wait_state(const struct gpio_init_table_t *gpio, uint8_t state)
{
	volatile uint32_t time = 0xFFFFFF;

	while (--time && ((!!(gpio->gpio->IDR & gpio->pin)) != (!!state)));

	return (time > 0)?0:1;
}

void gpio_set(const struct gpio_init_table_t *gpio, uint8_t state)
{
	switch (state) {
		case GPIO_SET:
#if defined(STM32F4XX)
			gpio->gpio->BSRRL = gpio->pin;
#else
			gpio->gpio->BSRR = gpio->pin;
#endif
			break;
		case GPIO_RESET:
#if defined(STM32F4XX)
			gpio->gpio->BSRRH = gpio->pin;
#else
			gpio->gpio->BRR = gpio->pin;
#endif
			break;
		default:
			break;
	}
}

uint8_t gpio_get(const struct gpio_init_table_t *gpio)
{
	return !!(gpio->gpio->IDR & gpio->pin);
}
