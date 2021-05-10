
// iface 1: PB12, PB13, PB14
// iface 2: PB8, PB15, PA8

#include "platform.h"
#include "systime.h"
#include "gpio.h"
#include "delay.h"
#include "led.h"

#include "gdm.h"

#define IO1_OUTPUT  GPIOB->CRH |= 0x3 << (4*4)
#define IO2_OUTPUT  GPIOB->CRH |= 0x3 << (4*5)
#define IO3_OUTPUT  GPIOB->CRH |= 0x3 << (4*6)

#define IO1_INPUT   GPIOB->CRH &= ~(0x3 << (4*4))
#define IO2_INPUT   GPIOB->CRH &= ~(0x3 << (4*5))
#define IO3_INPUT   GPIOB->CRH &= ~(0x3 << (4*6))

#define IO1_PIN    12
#define IO2_PIN    13
#define IO3_PIN    14

#define IOx_STATE(x) (!!(GPIOB->IDR & (1<<(x))))

#define IO1_SET_1  GPIOB->BSRR = (1 << 12); delay_us(gdm_bitdly);
#define IO1_SET_0  GPIOB->BRR = (1 << 12);  delay_us(gdm_bitdly);
#define IO2_SET_1  GPIOB->BSRR = (1 << 13); delay_us(gdm_bitdly);
#define IO2_SET_0  GPIOB->BRR = (1 << 13);  delay_us(gdm_bitdly);
#define IO3_SET_1  GPIOB->BSRR = (1 << 14); delay_us(gdm_bitdly);
#define IO3_SET_0  GPIOB->BRR = (1 << 14);  delay_us(gdm_bitdly);


int gdm_mode = 0;
uint16_t gdm_bitdly = 50;
uint16_t gdm_timeout = 1000;
uint8_t gdm_debounce = 3;

const struct gpio_init_table_t gdm_gpio[] = {
	{
		.gpio = GPIOB,
		.pin = GPIO_Pin_12,
		.mode = GPIO_MODE_OUT_OD,
		.speed = GPIO_SPEED_MED,
		.state = GPIO_SET,
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_Pin_13,
		.mode = GPIO_MODE_OUT_OD,
		.speed = GPIO_SPEED_MED,
		.state = GPIO_SET,
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_Pin_14,
		.mode = GPIO_MODE_OUT_OD,
		.speed = GPIO_SPEED_MED,
		.state = GPIO_SET,
	},
};

void gdm_init(void)
{
	gpio_init(gdm_gpio, ARRAY_SIZE(gdm_gpio));
	delay_init();

	IO1_INPUT;
	IO2_INPUT;
	IO3_INPUT;
	IO1_SET_1;
	IO2_SET_1;
	IO3_SET_1;
}

void gdm_periodic(void)
{
}

void gdm_set_params(uint16_t timeout, uint16_t bitdly, uint8_t deb)
{
	gdm_timeout = timeout;
	gdm_bitdly = bitdly;
	gdm_debounce = deb;
}

int gdm_selftest(void)
{
	int m = gdm_mode;

	gdm_set_mode(0);

	IO1_INPUT;
	IO2_INPUT;
	IO3_INPUT;

	IO1_OUTPUT;
	IO1_SET_0;
	if (IO2_INPUT == 0) {
		return 0x102;
	}
	if (IO3_INPUT == 0) {
		return 0x103;
	}

	IO1_SET_1;
	IO1_INPUT;

	IO2_OUTPUT;
	IO2_SET_0;
	if (IO1_INPUT == 0) {
		return 0x201;
	}
	if (IO3_INPUT == 0) {
		return 0x203;
	}

	IO2_SET_1;
	IO2_INPUT;
	

	IO3_OUTPUT;
	IO3_SET_0;
	if (IO1_INPUT == 0) {
		return 0x301;
	}
	if (IO2_INPUT == 0) {
		return 0x302;
	}

	IO3_SET_1;
	IO3_INPUT;

	gdm_set_mode(m);

	return 0;
}

void gdm_set_mode(int mode)
{
	if (gdm_mode == mode) {
		return;
	}

	gdm_mode = mode;

	if (mode) {
		// exti_enable(...)
	} else {
		// exti_disable(
	}
}

int gdm_can_read(void)
{
	return !IOx_STATE(IO2_PIN);
}

static int wait_state(int pin, uint16_t state)
{
	uint32_t t;
	uint32_t deb;

	t = systime_get();
	deb = 0;
	while(1) {
		delay_us(gdm_bitdly);
		if (IOx_STATE(pin) == state) {
			deb++;
			if (deb > gdm_debounce) {
				break;
			}
		} else {
			deb = 0;
		}
		if (systime_get()-t > gdm_timeout) {
			return 1;
		}
	}
	return 0;
}

// used for geodat to start comm
int gdm_send_start()
{
	int ret = 0;

	led_toggle(0);

	IO3_OUTPUT;
	IO3_SET_0;

	// wait for bus ready
	ret = wait_state(IO2_PIN, 0);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {
		// idle channels
		IO3_SET_1;

		// wait for bit read
		ret = wait_state(IO2_PIN, 1);
		if (ret) {
			ret = 2;
		}
	}

	IO3_INPUT;

	led_toggle(0);

	return ret;
}

static int send_bit(uint8_t b, uint8_t *p)
{
	int ret = 0;

	led_toggle(0);

	IO1_OUTPUT;
	IO3_OUTPUT;

	// wait for bus ready
	ret = wait_state(IO2_PIN, 1);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {

		// setup bit
		*p ^= b&1;
		if (b&1) {
			IO1_SET_1;
		} else {
			IO1_SET_0;
		}

		// tell other side bit is ready
		IO3_SET_0;

		// wait for bit read
		ret = wait_state(IO2_PIN, 0);
		if (ret) {
			ret = 2;
		}
	}

	// idle channels
	IO3_SET_1;
	IO1_SET_1;

	IO1_INPUT;
	IO3_INPUT;

	led_toggle(0);

	return ret;
}

static int recv_bit(uint8_t *b, uint8_t *par)
{
	int ret = 0;

	led_toggle(0);

	// tell other side bit was read
	IO2_OUTPUT;
	IO2_SET_0;

	ret = wait_state(IO3_PIN, 0);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {
		// read bit
		if (IOx_STATE(IO1_PIN)) {
			*b |= 1;
		} else {
			*b |= 0;
		}

		// idle output channel
		IO2_SET_1;
		IO2_INPUT;

		// wait for signal to go high
		ret = wait_state(IO3_PIN, 1);

		if (ret == 0) {
			*par ^= *b & 1;
		}
	}

	// idle output channel
	IO2_SET_1;
	IO2_INPUT;

	led_toggle(0);

	return ret;
}

static int send_byte(uint8_t b, uint8_t *p)
{
	int ret = 0;

	for (int i = 0; i < 8; i++) {
		ret = send_bit(b & 1, p);
		if (ret) {
			ret = 0x10*i+ret;
			break;
		}
		b >>= 1;
	}

	return ret;
}

int gdm_recv_byte(uint8_t *b, uint8_t *p)
{
	int ret = 0;

	*b = 0;
	for (int i = 0; i < 8; i++) {
		ret = recv_bit(b, p);
		if (ret) {
			ret = 0x10*i+ret;
			break;
		}
		*b = (*b >> 1) | (*b << 7);
	}

	return ret;
}

static int send_ack(uint8_t *ack, uint8_t *p)
{
	int ret = 0;
	uint8_t b;
	uint8_t pp; // dummy parity

	ret = send_bit(*p, &pp);
	if (ret != 0) {
		return 0x10 + ret;
	}

	ret = recv_bit(ack, &pp);
	if (ret != 0) {
		return 0x20 + ret;
	}

	ret = recv_bit(&b, &pp);
	if (ret != 0) {
		return 0x30 + ret;
	}

	*p = b & 1;

	return 0;
}

static int recv_ack(uint8_t ack, uint8_t *p)
{
	int ret = 0;
	uint8_t b;
	uint8_t pp;

	// do dummy read, parity will update
	ret = recv_bit(&b, p);
	if (ret != 0) {
		return 0x10 + ret;
	}

	// 1 means continue transmission
	ret = send_bit(ack, &pp);
	if (ret != 0) {
		return 0x20 + ret;
	}

	*p ^= 1;

	ret = send_bit(*p, &pp);
	if (ret != 0) {
		return 0x30 + ret;
	}
	return 0;
}

int gdm_send_pkt(uint8_t label, uint8_t *buf, uint8_t len)
{
	uint8_t ack = 1;
	uint8_t par = 0;
	int ret = 0;
	uint32_t i;

	if (gdm_mode) {
		return 1;
	}

	ret = send_byte(label, &par);
	if (ret != 0) return 0x100+ret;

	ret = send_byte(len, &par);
	if (ret != 0) return 0x200+ret;

	for (i = 0; i < len; i++) {
		ret = send_byte(buf[i], &par);
		if (ret != 0) return 0x300+ret;

		ret = send_ack(&ack, &par);
		if (ret != 0) return 0x400+ret;

		if (par == 0) {
			return 0x500+i;
		}

		par = 0;

		if (ack == 0) {
			return 0x600+i;
		}
	}

	return 0;
}

int gdm_recv_pkt(uint8_t *label, uint8_t *buf, uint8_t *len)
{
	uint8_t ack = 1;
	uint8_t par = 0;
	int ret = 0;
	uint32_t i;
	uint8_t lenin;

	if (gdm_mode) {
		return 1;
	}

	ret = gdm_recv_byte(label, &par);
	if (ret != 0) return 0x100+ret;

	ret = gdm_recv_byte(&lenin, &par);
	if (ret != 0) return 0x200+ret;

	lenin = MIN(*len, lenin);
	*len = lenin;
	for (i = 0; i < lenin; i++) {
		ret = gdm_recv_byte(&buf[i], &par);
		if (ret != 0) return 0x300+ret;

		ret = recv_ack(ack, &par);
		if (ret != 0) return 0x400+ret;

		par = 0;
	}

	return 0;
}
