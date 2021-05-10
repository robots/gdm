#include "platform.h"

#include <string.h>

#include "fifo.h"
#include "crc32.h"
#include "systime.h"
#include "timer.h"

#include "usb_uart_cdc.h"

#include "gdm.h"
#include "pwm.h"
#include "cmd.h"

#define CMD_IN_TIMEOUT 300
#define CMD_PRIO_BUF_LEN 300

enum {
	CMD_IN_IDLE,
	CMD_IN_CMD,
	CMD_IN_LEN1,
	CMD_IN_LEN2,
	CMD_IN_DATA,
	CMD_IN_CRC1,
	CMD_IN_CRC2,
	CMD_IN_CRC3,
	CMD_IN_CRC4,
};

static void cmd_process(void);
static void cmd_timer_handler(void);

static int cmd_handle_id(void);
static int cmd_handle_ping(void);
static int cmd_handle_selftest(void);
static int cmd_handle_timeout(void);
static int cmd_handle_reset(void);
static int cmd_handle_pwm(void);
static int cmd_handle_mode(void);
static int cmd_handle_send(void);
static int cmd_handle_recv(void);


struct cmd_pkt_t cmd_out_pkt;
struct cmd_pkt_t cmd_in_pkt;

uint32_t cmd_in_idx;
uint32_t cmd_in_state = CMD_IN_IDLE;

struct fifo_t cmd_prio_fifo;
uint8_t cmd_prio_buf[CMD_PRIO_BUF_LEN];

struct {
	uint8_t id;
	int8_t params;
	int (*fnc)(void);
} cmd_commands[] = {
	{ CMD_ID,                   0, cmd_handle_id },
	{ CMD_PING,                -1, cmd_handle_ping },
	{ CMD_SELFTEST,             0, cmd_handle_selftest },
	{ CMD_TIMEOUT,              8, cmd_handle_timeout },
	{ CMD_RESET,                0, cmd_handle_reset },
	{ CMD_PWM,                  3, cmd_handle_pwm },
	{ CMD_MODE,                 1, cmd_handle_mode },
	{ CMD_SEND,                -1, cmd_handle_send },
	{ CMD_RECV,                 0, cmd_handle_recv },
};

void cmd_periodic(void)
{
	if (!FIFO_EMPTY(&usb_rx1_fifo)) {
		uint8_t *b;
		b = fifo_get_read_addr(&usb_rx1_fifo);

		cmd_parse(*b);

		fifo_read_done(&usb_rx1_fifo);
	}

	// copy prio fifo to uart
	if (!FIFO_EMPTY(&cmd_prio_fifo)) {
		struct fifo_t *f = &cmd_prio_fifo;
		uint32_t rl;
		uint32_t totlen = 0;

		// hackhish access to fifo
		if (f->write > f->read) {
			rl = f->write - f->read;
			usb_uart_cdc1_send(fifo_get_read_addr(f), rl);
			totlen += rl;
		} else if (f->read > f->write) {
			rl = f->e_num - f->read;
			usb_uart_cdc1_send(fifo_get_read_addr(f), rl);
			totlen += rl;
			rl = f->write;
			usb_uart_cdc1_send(f->buffer, rl);
			totlen += rl;
		}

		fifo_read_done_count(f, totlen);

	}

	gdm_periodic();
}

void cmd_init(void)
{
	fifo_init(&cmd_prio_fifo, cmd_prio_buf, sizeof(uint8_t), CMD_PRIO_BUF_LEN);

	timer_init(TIMER_TIM3);
	timer_set_handler(TIMER_TIM3, cmd_timer_handler);

	pwm_init();
	gdm_init();
}

void cmd_parse(uint8_t buf)
{
	timer_timeout(TIMER_TIM3, CMD_IN_TIMEOUT * 1000);

	if (cmd_in_state == CMD_IN_IDLE) {
		if (buf == CMD_SOF) {
			// found sof
			cmd_in_state = CMD_IN_CMD;
			cmd_in_idx = 0;
			cmd_in_pkt.crc = 0;
			cmd_in_pkt.len = 0;
			//memset(&cmd_in_pkt, 0, sizeof(struct cmd_pkt_t));
		}
	} else if (cmd_in_state == CMD_IN_CMD) {
		cmd_in_pkt.cmd = buf;
		cmd_in_state = CMD_IN_LEN1;
	} else if (cmd_in_state == CMD_IN_LEN1) {
		cmd_in_pkt.len |= buf;
		cmd_in_state = CMD_IN_LEN2;
	} else if (cmd_in_state == CMD_IN_LEN2) {
		cmd_in_pkt.len |= buf << 8;
				
		if (cmd_in_pkt.len >= CMD_BUF_SIZE) {
			// error, longer than buffer
			cmd_in_state = CMD_IN_IDLE;
		} else if (cmd_in_pkt.len == 0) {
			cmd_in_state = CMD_IN_CRC1;
		} else {
			cmd_in_state = CMD_IN_DATA;
		}
	} else if (cmd_in_state == CMD_IN_DATA) {
		cmd_in_pkt.data[cmd_in_idx++] = buf;

		if (cmd_in_idx >= cmd_in_pkt.len) {
			cmd_in_state = CMD_IN_CRC1;
		}
	} else if (cmd_in_state == CMD_IN_CRC1) {
		cmd_in_pkt.crc = buf;
		cmd_in_state = CMD_IN_CRC2;
	} else if (cmd_in_state == CMD_IN_CRC2) {
		cmd_in_pkt.crc |= buf << 8;
		cmd_in_state = CMD_IN_CRC3;
	} else if (cmd_in_state == CMD_IN_CRC3) {
		cmd_in_pkt.crc |= buf << 16;
		cmd_in_state = CMD_IN_CRC4;
	} else if (cmd_in_state == CMD_IN_CRC4) {
		cmd_in_pkt.crc |= buf << 24;
		cmd_in_state = CMD_IN_IDLE;

		timer_abort(TIMER_TIM3);

		uint32_t c;

		c = xcrc32((uint8_t *)&cmd_in_pkt.cmd, 1, 0xffffffff);
		c = xcrc32((uint8_t *)&cmd_in_pkt.len, 2, c);
		c = xcrc32((uint8_t *)cmd_in_pkt.data, cmd_in_pkt.len, c);

		if (cmd_in_pkt.crc == c) {
			// process command
			cmd_process();
		}
	}
}

static void cmd_timer_handler(void)
{
	cmd_in_state = CMD_IN_IDLE;
}

void cmd_queue(uint8_t cmd, uint8_t *data, uint16_t len)
{
	uint32_t c;
	uint8_t sof = CMD_SOF;

	cmd |= 0x80;
	
	c = xcrc32((uint8_t *)&cmd, 1, 0xffffffff);
	c = xcrc32((uint8_t *)&len, 2, c);
	c = xcrc32((uint8_t *)data, len, c);

	c = __REV(c);

	fifo_write_buf(&cmd_prio_fifo, (void *)&sof, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&cmd, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&len, 2);
	fifo_write_buf(&cmd_prio_fifo, data, len);
	fifo_write_buf(&cmd_prio_fifo, (void *)&c, 4);
}

void cmd_send(void)
{
	uint32_t c;

	cmd_out_pkt.sof = CMD_SOF;
	cmd_out_pkt.cmd |= 0x80;
	
	c = xcrc32((uint8_t *)&cmd_out_pkt.cmd, 1, 0xffffffff);
	c = xcrc32((uint8_t *)&cmd_out_pkt.len, 2, c);
	c = xcrc32((uint8_t *)cmd_out_pkt.data, cmd_out_pkt.len, c);

	cmd_out_pkt.crc = __REV(c);
	
	usb_uart_cdc1_pkt(&cmd_out_pkt);

	memset(&cmd_out_pkt, 0, sizeof(struct cmd_pkt_t));
}

static void cmd_process(void)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(cmd_commands); i++) {
		if (cmd_commands[i].id == cmd_in_pkt.cmd) {

			if (cmd_commands[i].params >= 0) {
				if (cmd_commands[i].params != (cmd_in_pkt.len)) {
					break;
				}
			}

			int ret = cmd_commands[i].fnc();

			if (ret == 1) {
				cmd_send();
			} else if (ret == 2) {
				cmd_out_pkt.len = 0;
				cmd_out_pkt.cmd = cmd_in_pkt.cmd;
				cmd_send();
			}

			return;
		}
	}
}

static int cmd_handle_id(void)
{
	cmd_out_pkt.cmd = CMD_ID;
	cmd_out_pkt.data[0] = 'G';
	cmd_out_pkt.data[1] = 'D';
	cmd_out_pkt.data[2] = 'M';
	cmd_out_pkt.data[3] = 'I';
	cmd_out_pkt.data[4] = 'f';
	cmd_out_pkt.data[5] = '1';
	cmd_out_pkt.len = 6;

	return 1;
}

static int cmd_handle_ping(void)
{
	cmd_out_pkt.len = cmd_in_pkt.len;
	cmd_out_pkt.cmd = CMD_PING;
	memcpy((void *)&cmd_out_pkt.data[0], (void *)&cmd_in_pkt.data[0], cmd_in_pkt.len);

	return 1;
}

static int cmd_handle_selftest(void)
{
	int ret;

	cmd_out_pkt.len = 4;
	cmd_out_pkt.cmd = CMD_SELFTEST;

	ret = gdm_selftest();

	memcpy((void *)&cmd_out_pkt.data[0], (void *)&ret, 4);

	return 1;
}

static int cmd_handle_reset(void)
{
	cmd_out_pkt.cmd = CMD_RESET;
	cmd_send();
	
	systime_delay(100);

	NVIC_SystemReset();

	return 0;
}

static int cmd_handle_timeout(void)
{
	uint16_t timeout;
	uint16_t bitdly;
	uint8_t deb;

	memcpy(&timeout, &cmd_in_pkt.data[0], 2);
	memcpy(&bitdly, &cmd_in_pkt.data[2], 2);
	memcpy(&deb, &cmd_in_pkt.data[4], 1);

	gdm_set_params(timeout, bitdly, deb);

	return 2;
}

static int cmd_handle_pwm(void)
{
	uint16_t pwm;
	uint8_t chan;

	memcpy(&chan, &cmd_in_pkt.data[0], 1);
	memcpy(&pwm, &cmd_in_pkt.data[1], 2);

	pwm_set(chan, pwm);

	return 2;
}

static int cmd_handle_mode(void)
{
	gdm_set_mode(cmd_in_pkt.data[0]);
	return 1;
}

static int cmd_handle_send(void)
{
	uint8_t label;
	uint8_t l;
	uint8_t *dat;
	int ret;

	label = cmd_in_pkt.data[0];
	l = cmd_in_pkt.len - 1;
	dat = &cmd_in_pkt.data[1];

	ret = gdm_send_pkt(label, dat, l); 

	cmd_out_pkt.cmd = CMD_SEND;
	cmd_out_pkt.len = 4;

	memcpy(&cmd_out_pkt.data[0], &ret, 4);

	return 1;
}

static int cmd_handle_recv(void)
{
	uint8_t label;
	uint8_t l;
	uint8_t *dat;
	int ret;

	l = CMD_BUF_SIZE-5;
	dat = &cmd_out_pkt.data[5];

	ret = gdm_recv_pkt(&label, dat, &l); 

	cmd_out_pkt.cmd = CMD_RECV;
	cmd_out_pkt.len = l+5;
	memcpy(&cmd_out_pkt.data[0], &ret, 4);
	cmd_out_pkt.data[4] = label;

	return 1;
}
