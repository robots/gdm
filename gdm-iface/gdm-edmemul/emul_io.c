#include "platform.h"

#include "gpio.h"
#include "delay.h"

const struct gpio_init_table_t emul_io_gpio[] = {
	{
		.gpio = GPIOB,
		.pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14,
		.mode = GPIO_MODE_OUT_OD,
		.speed = GPIO_SPEED_MED,
		.state = GPIO_SET,
	},
};

struct emul_io_t {
	GPIO_TypeDef *gpio;
	uint32_t pin;
};

struct emul_io_t emul_io_port_0[] = {
	{ GPIOB, 12},
	{ GPIOB, 13},
	{ GPIOB, 14},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
};

struct emul_io_t emul_io_port_1[] = {
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
};

struct emul_io_t emul_io_port_2[] = {
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
};

struct emul_io_t emul_io_port_t[] = {
	{ NULL, 0},
	{ NULL, 0},
	{ NULL, 0},
};

void emul_io_init(void)
{
	gpio_init(emul_io_gpio, ARRAY_SIZE(emul_io_gpio));

}

uint8_t emul_io_px_read(struct emul_io_t *p)
{
	int i = 0;
	uint32_t mask = 1;
	uint8_t out = 0;

	while (i < 8) {
		if (p[i].gpio != NULL) {
			if (p[i].gpio->IDR & (1 << p[i].pin)) {
				out |= mask;
			}
		}

		i++;
		mask <<= 1;
	}

	return out;
}

uint8_t emul_io_bus_read()
{
	return emul_io_px_read(emul_io_port_0);
}

uint8_t emul_io_p1_read()
{
	return emul_io_px_read(emul_io_port_1);
}

uint8_t emul_io_p2_read()
{
	return emul_io_px_read(emul_io_port_2);
}

uint8_t emul_io_t_read(int t)
{
	struct emul_io_t *p;

	p = &emul_io_port_t[t];
	if (p->gpio == NULL){
		return 0;
	}

	if (p->gpio->IDR & (1 << p->pin)) {
		return 1;
	} else {
		return 0;
	}
}

void emul_io_px_write_pp(struct emul_io_t *p, uint8_t data)
{
	int i = 0;
	uint32_t mask = 1;

	while (i < 8) {
		if (p[i].gpio != NULL) {
		
			// set pin as output
			if (p[i].pin > 7) {
				p[i].gpio->CRH |= 0x3 << (4*(p[i].pin-8));
			} else {
				p[i].gpio->CRL |= 0x3 << (4*(p[i].pin));
			}

			if (data & mask) {
				p[i].gpio->BSRR = (1 << p[i].pin);
			} else {
				p[i].gpio->BRR = (1 << p[i].pin);
			}
		}

		i++;
		mask <<= 1;
	}
	
}

void emul_io_px_write(struct emul_io_t *p, uint8_t data)
{
	int i = 0;
	uint32_t mask = 1;

	while (i < 8) {
		if (p[i].gpio != NULL) {
			if (data & mask) {
				// set to 1
				p[i].gpio->BRR = (1 << p[i].pin);

				// set input
				if (p[i].pin > 7) {
					p[i].gpio->CRH &= ~(0x3 << (4*(p[i].pin-8)));
				} else {
					p[i].gpio->CRL &= ~(0x3 << (4*(p[i].pin)));
				}
			} else {
				// set output
				if (p[i].pin > 7) {
					p[i].gpio->CRH |= 0x3 << (4*(p[i].pin-8));
				} else {
					p[i].gpio->CRL |= 0x3 << (4*(p[i].pin));
				}

				// set to 0
				p[i].gpio->BRR = (1 << p[i].pin);
			}
		}

		i++;
		mask <<= 1;
	}
	
}

void emul_io_bus_write(uint8_t data)
{
	emul_io_px_write_pp(emul_io_port_0, data);
}

void emul_io_p1_write(uint8_t data)
{
	emul_io_px_write(emul_io_port_1, data);
}

void emul_io_p2_write(uint8_t data)
{
	emul_io_px_write(emul_io_port_2, data);
}

