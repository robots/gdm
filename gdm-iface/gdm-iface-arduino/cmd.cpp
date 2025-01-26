#include <string.h>
#include <Arduino.h>

#include<avr/wdt.h>

#include "fifo.h"
#include "crc32.h"

#include "gdm.h"
#include "cmd.h"

#define CMD_IN_TIMEOUT 300
#define CMD_PRIO_BUF_LEN 30

#define __REV(z) ((uint32_t)((z&0xFF)<<24|((z>>8)&0xFF)<<16|((z>>16)&0xFF)<<8|((z>>24)&0xFF)))
#define ARRAY_SIZE(x)      ((sizeof(x)/sizeof(x[0])))

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

unsigned long cmd_in_idx;
unsigned long cmd_in_state = CMD_IN_IDLE;

struct fifo_t cmd_prio_fifo;
byte cmd_prio_buf[CMD_PRIO_BUF_LEN];

struct {
	byte id;
	char params;
	int (*fnc)(void);
} cmd_commands[] = {
	{ CMD_ID,                   0, cmd_handle_id },
	{ CMD_PING,                -1, cmd_handle_ping },
	{ CMD_SELFTEST,             0, cmd_handle_selftest },
	{ CMD_TIMEOUT,              9, cmd_handle_timeout },
	{ CMD_RESET,                0, cmd_handle_reset },
	{ CMD_PWM,                  3, cmd_handle_pwm },
	{ CMD_MODE,                 2, cmd_handle_mode },
	{ CMD_SEND,                -1, cmd_handle_send },
	{ CMD_RECV,                 1, cmd_handle_recv },
};

static void gdm_rx_cb(byte iface, byte *data, unsigned int datalen);

void cmd_periodic(void)
{
  static unsigned long last_byte_time;
  static bool timing;
  
  if (millis() - last_byte_time > CMD_IN_TIMEOUT && cmd_in_state != CMD_IN_IDLE) {
    cmd_in_state = CMD_IN_IDLE;
  }
	if (Serial.available()) {
		byte b;
		b = Serial.read();
    last_byte_time = millis();
		cmd_parse(b);
	}

	// copy prio fifo to uart
	if (!FIFO_EMPTY(&cmd_prio_fifo)) {
		struct fifo_t *f = &cmd_prio_fifo;
		unsigned long rl;
		unsigned long totlen = 0;

		// hackhish access to fifo
		if (f->write > f->read) {
			rl = f->write - f->read;
			Serial.write(fifo_get_read_addr(f), (int)rl);
			totlen += rl;
		} else if (f->read > f->write) {
			rl = f->e_num - f->read;
			Serial.write(fifo_get_read_addr(f), (int)rl);
			totlen += rl;
			rl = f->write;
			Serial.write(f->buffer, (int)rl);
			totlen += rl;
		}

		fifo_read_done_count(f, totlen);

	}

	gdm_periodic();
}

void cmd_init(void)
{
	fifo_init(&cmd_prio_fifo, cmd_prio_buf, sizeof(byte), CMD_PRIO_BUF_LEN);

	gdm_init();

	gdm_set_rx_cb(&gdm_iface[0], gdm_rx_cb);
	gdm_set_rx_cb(&gdm_iface[1], gdm_rx_cb);
}

void cmd_parse(byte buf)
{
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
		cmd_in_pkt.crc = (unsigned long)buf;
		cmd_in_state = CMD_IN_CRC2;
	} else if (cmd_in_state == CMD_IN_CRC2) {
		cmd_in_pkt.crc |= (unsigned long)buf << 8;
		cmd_in_state = CMD_IN_CRC3;
	} else if (cmd_in_state == CMD_IN_CRC3) {
		cmd_in_pkt.crc |= (unsigned long)buf << 16;
		cmd_in_state = CMD_IN_CRC4;
	} else if (cmd_in_state == CMD_IN_CRC4) {
		cmd_in_pkt.crc |= (unsigned long)buf << 24;
		cmd_in_state = CMD_IN_IDLE;

		unsigned long c;

		c = xcrc32((byte *)&cmd_in_pkt.cmd, 1, 0xffffffff);
		c = xcrc32((byte *)&cmd_in_pkt.len, 2, c);
		c = xcrc32((byte *)cmd_in_pkt.data, cmd_in_pkt.len, c);
   
		if (cmd_in_pkt.crc == c) {
			// process command
			cmd_process();
		}
	}
}

static void gdm_rx_cb(byte iface, byte *data, unsigned int datalen)
{
	unsigned long c;
	byte sof = CMD_SOF;
	unsigned int len;

	byte cmd = CMD_PKT;
	cmd |= 0x80;

	len = 1+datalen; // iface + data

	c = xcrc32((byte *)&cmd, 1, 0xffffffff);
	c = xcrc32((byte *)&len, 2, c);
	c = xcrc32((byte *)&iface, 1, c);
	c = xcrc32((byte *)data, datalen, c);

	c = __REV(c);

	fifo_write_buf(&cmd_prio_fifo, (void *)&sof, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&cmd, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&len, 2);

	fifo_write_buf(&cmd_prio_fifo, &iface, 1);
	fifo_write_buf(&cmd_prio_fifo, data, datalen);

	fifo_write_buf(&cmd_prio_fifo, (void *)&c, 4);
}

/*
void cmd_queue(byte cmd, byte *data, unsigned int len)
{
	unsigned long c;
	byte sof = CMD_SOF;

	cmd |= 0x80;
	
	c = xcrc32((byte *)&cmd, 1, 0xffffffff);
	c = xcrc32((byte *)&len, 2, c);
	c = xcrc32((byte *)data, len, c);

	c = __REV(c);

	fifo_write_buf(&cmd_prio_fifo, (void *)&sof, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&cmd, 1);
	fifo_write_buf(&cmd_prio_fifo, (void *)&len, 2);
	fifo_write_buf(&cmd_prio_fifo, data, len);
	fifo_write_buf(&cmd_prio_fifo, (void *)&c, 4);
}
*/

void cmd_send(void)
{
	unsigned long c;

	cmd_out_pkt.sof = CMD_SOF;
	cmd_out_pkt.cmd |= 0x80;
	
	c = xcrc32((byte *)&cmd_out_pkt.cmd, 1, 0xffffffff);
	c = xcrc32((byte *)&cmd_out_pkt.len, 2, c);
	c = xcrc32((byte *)cmd_out_pkt.data, cmd_out_pkt.len, c);

	cmd_out_pkt.crc = __REV(c);
	
	Serial.write((byte *)&cmd_out_pkt, cmd_out_pkt.len + 4);
  Serial.write((byte *)&cmd_out_pkt.crc, 4);

	memset(&cmd_out_pkt, 0, sizeof(struct cmd_pkt_t));
}

static void cmd_process(void)
{
	for (unsigned long i = 0; i < ARRAY_SIZE(cmd_commands); i++) {
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
	cmd_out_pkt.data[5] = '2';
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

	ret = gdm_selftest(&gdm_iface[0]);

	if (ret == 0) {
		ret = gdm_selftest(&gdm_iface[1]);
		if (ret != 0) {
			ret = 0x1000 + ret;
		}
	}

	memcpy((void *)&cmd_out_pkt.data[0], (void *)&ret, 4);

	return 1;
}

static int cmd_handle_reset(void)
{
	cmd_out_pkt.cmd = CMD_RESET;
	cmd_send();
	
	delay(100);

	wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}

	return 0;
}

static int cmd_handle_timeout(void)
{
	struct gdm_iface_t *iface;

	unsigned int timeout;
	unsigned int bitdly;
	byte deb;

	if (cmd_in_pkt.data[0] == 0) {
		iface = &gdm_iface[0];
	} else {
		iface = &gdm_iface[1];
	}

	memcpy(&timeout, &cmd_in_pkt.data[1], 2);
	memcpy(&bitdly, &cmd_in_pkt.data[3], 2);
	memcpy(&deb, &cmd_in_pkt.data[5], 1);

	gdm_set_params(iface, timeout, bitdly, deb);

	return 2;
}

static int cmd_handle_pwm(void)
{
	return 2;
}

static int cmd_handle_mode(void)
{
	struct gdm_iface_t *iface;

	if (cmd_in_pkt.data[0] == 0) {
		iface = &gdm_iface[0];
	} else {
		iface = &gdm_iface[1];
	}

	gdm_set_mode(iface, cmd_in_pkt.data[1]);
	return 1;
}

static int cmd_handle_send(void)
{
	long ret;
	byte x = cmd_in_pkt.data[0];
	if (x > ARRAY_SIZE(gdm_iface)) x = 0;
	
	struct gdm_packet_t *pkt = &gdm_iface[x].tx_pkt;

	pkt->label = cmd_in_pkt.data[1];
	pkt->len = cmd_in_pkt.len - 2;
	memcpy(pkt->data, &cmd_in_pkt.data[2], pkt->len);

	ret = gdm_send_pkt(&gdm_iface[x], pkt); 

	cmd_out_pkt.cmd = CMD_SEND;
	cmd_out_pkt.len = 4;

	memcpy(&cmd_out_pkt.data[0], &ret, 4);

	return 1;
}

static int cmd_handle_recv(void)
{
	long ret;
	byte x = cmd_in_pkt.data[0];
	if (x > ARRAY_SIZE(gdm_iface)) x = 0;

	struct gdm_packet_t *pkt = &gdm_iface[x].rx_pkt;
	
	pkt->len = CMD_BUF_SIZE-5;

	ret = gdm_recv_pkt(&gdm_iface[x], pkt); 

	cmd_out_pkt.cmd = CMD_RECV;
	cmd_out_pkt.len = pkt->len + 5;
	memcpy(&cmd_out_pkt.data[0], &ret, 4);
	cmd_out_pkt.data[4] = pkt->label;
	memcpy(&cmd_out_pkt.data[5], pkt->data, pkt->len);

	return 1;
}
