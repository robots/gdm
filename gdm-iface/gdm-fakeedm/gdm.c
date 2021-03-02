
#include "platform.h"
#include "systime.h"
#include "gpio.h"
#include "delay.h"
#include "led.h"

#include "gdm.h"

#define IO1_PIN    12
#define IO2_PIN    13
#define IO3_PIN    14
#define INT_PIN    15
#define REQ_PIN    11

#define IOxh_OUTPUT_pp(x)  GPIOB->CRH = (GPIOB->CRH & ~(0xf << (4*(x-8)))) | (0x3 << (4*(x-8)))
#define IOxh_OUTPUT_od(x)  GPIOB->CRH = (GPIOB->CRH & ~(0xf << (4*(x-8)))) | (0x7 << (4*(x-8)))
#define IOxh_INPUT(x)      GPIOB->CRH = (GPIOB->CRH & ~(0xf << (4*(x-8)))) | (0x4 << (4*(x-8)))

#define IO1_OUTPUT     IOxh_OUTPUT_od(IO1_PIN)
#define IO2_OUTPUT     IOxh_OUTPUT_od(IO2_PIN)
#define IO3_OUTPUT     IOxh_OUTPUT_od(IO3_PIN)

#define IO1_INPUT      IOxh_INPUT(IO1_PIN)
#define IO2_INPUT      IOxh_INPUT(IO2_PIN)
#define IO3_INPUT      IOxh_INPUT(IO3_PIN)

#define IOx_STATE(x)   (!!(GPIOB->IDR & (1<<(x))))
#define IO_DELAY 150

#define IO1_SET_1      GPIOB->BSRR = (1 << IO1_PIN);  delay_us(IO_DELAY);
#define IO1_SET_0      GPIOB->BRR  = (1 << IO1_PIN);  delay_us(IO_DELAY);
#define IO2_SET_1      GPIOB->BSRR = (1 << IO2_PIN);  delay_us(IO_DELAY);
#define IO2_SET_0      GPIOB->BRR  = (1 << IO2_PIN);  delay_us(IO_DELAY);
#define IO3_SET_1      GPIOB->BSRR = (1 << IO3_PIN);  delay_us(IO_DELAY);
#define IO3_SET_0      GPIOB->BRR  = (1 << IO3_PIN);  delay_us(IO_DELAY);


#define IOx_OUTPUT(x)  GPIOB->CRH |= 0x3 << (4*(x-8))
#define IOx_INPUT(x)   GPIOB->CRH &= ~(0x3 << (4*(x-8)))

#define IOx_SET_1(x)   GPIOB->BSRR = (1 << (x));  delay_us(IO_DELAY);
#define IOx_SET_0(x)   GPIOB->BRR  = (1 << (x));  delay_us(IO_DELAY);


// 50*100ms
#define GDM_TIMEOUT_VAL 1000
#define GDM_DEB_CYCLES 10


#define MAX_SAVE 100
struct {
	int ret;
	uint8_t lab;
	uint8_t len;
	uint8_t data[4];
	uint32_t ts;
} save_label[100];
int save_label_idx = 0;

uint8_t ram[256];

void gdm_init(void)
{
	delay_init();

	IO1_OUTPUT;
	IO2_OUTPUT;
	IO3_OUTPUT;
	IO1_SET_1;
	IO2_SET_1;
	IO3_SET_1;
	IO1_INPUT;
	IO2_INPUT;
	IO3_INPUT;
	IOxh_INPUT(REQ_PIN);
	IOxh_OUTPUT_pp(INT_PIN);
	IOx_SET_0(INT_PIN);

	memset(ram, 0, sizeof(ram));
	memset(save_label, 0, sizeof(save_label));

	// wait for gdm to be turned on
	while(IOx_STATE(IO3_PIN) == 0);
	delay_ms(20);

}

void gdm_periodic(void)
{
	static int retsend;
	static int req_countdown = 0;
	uint8_t label;
	uint8_t len = 20;
	uint8_t buf[20];

	// station wants to talk
	if (IOx_STATE(IO3_PIN) == 0) {
		len = 20;

		memset(buf, 0xaa, len);
		// receive data
		int ret = gdm_recv_pkt(&label, buf, &len);

		//save for analysis
		if (save_label_idx < MAX_SAVE) {
			save_label[save_label_idx].ret = ret;
			save_label[save_label_idx].lab = label;
			save_label[save_label_idx].ts = systime_get();
			save_label[save_label_idx].len = len;
			save_label[save_label_idx].data[0] = buf[0];
			save_label[save_label_idx].data[1] = buf[1];
			save_label[save_label_idx].data[2] = buf[2];
			save_label[save_label_idx].data[3] = buf[3];
			
			save_label_idx++;
		}

		if (ret == 0) {
			if (len+label < 256) {
				memcpy(&ram[label], buf, len);
			}
		}
	}

	// measurement request
	if (IOx_STATE(REQ_PIN) == 0) {
		req_countdown = 1000;
	}

	if (req_countdown > 0) {
		req_countdown--;
		// is measurement done ?
		if (req_countdown == 1) {

			// trigger interrupt
			IOx_SET_1(INT_PIN);
			delay_us(10);
			IOx_SET_0(INT_PIN);
			delay_us(10);

			// send 1234.000 meters result
			buf[0] = 0x00;
			buf[1] = 0x40;
			buf[2] = 0x23;
			buf[3] = 0x01;
			buf[4] = 0x00;
			retsend = gdm_send_pkt(0x12, buf, 5);

			return;

		}
	}
}

static int wait_state(int pin, uint16_t state)
{
	uint32_t t;
	uint32_t deb;

	t = systime_get();
	deb = 0;
	while(1) {
		delay_us(IO_DELAY);
		if (IOx_STATE(pin) == state) {
			deb++;
			if (deb > GDM_DEB_CYCLES) {
				break;
			}
		} else {
			deb = 0;
		}
		if (systime_get()-t > GDM_TIMEOUT_VAL) {
			return 1;
		}
	}
	return 0;
}

static int send_bit(uint8_t b, uint8_t *p)
{
	int ret = 0;

	led_toggle(0);

	IO1_OUTPUT;
	IO3_OUTPUT;

	// wait for bus ready
	ret = wait_state(IO2_PIN, 0);
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
		ret = wait_state(IO2_PIN, 1);
		if (ret) {
			ret = 2;
		}
	}

	// idle channels
	IO3_SET_1;
	IO3_INPUT;
	IO1_SET_1;
	IO1_INPUT;

	led_toggle(0);

	return ret;
}

static int recv_bit(uint8_t *b, uint8_t *par)
{
	int ret = 0;

	led_toggle(0);

	ret = wait_state(IO3_PIN, 0);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {
		delay_us(10);

		// read bit
		if (IOx_STATE(IO1_PIN)) {
			*b |= 1;
		} else {
			*b &= ~1;
		}

		// tell other side bit was read
		IO2_OUTPUT;
		IO2_SET_0;

		// wait for signal to go high
		ret = wait_state(IO3_PIN, 1);

		// idle output channel
		IO2_SET_1;

		if (ret == 0) {
			*par ^= *b & 1;
		}
	}

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
	uint8_t bit;
	int i=0;

	for (i = 0; i < 8; i++) {
		bit = 0;
		ret = recv_bit(&bit, p);
		if (ret) {
			ret = 0x10*i+ret;
			break;
		}

		*b = (*b >> 1);
		if (bit & 1) {
			*b |= 0x80;
		}
	}

	return ret;

}

static int send_ack(uint8_t *ack, uint8_t *p)
{
	int ret = 0;
	uint8_t b;
	// dummy parity
	uint8_t pp;

//	*p = *p ^ 1;
	ret = send_bit(*p, &pp);
	if (ret != 0) {
		return 0x10 + ret;
	}

//	ret = send_bit(*p, &pp);
//return 0x4000;
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
