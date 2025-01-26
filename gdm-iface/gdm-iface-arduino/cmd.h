#ifndef CMD_h_
#define CMD_h_

#define CMD_SOF 0xAA
#define CMD_BUF_SIZE 30

enum {
	CMD_ID = 0x01,
	CMD_PING = 0x02,
	CMD_SELFTEST = 0x03,
	CMD_RESET = 0x04,
	CMD_DEBUG = 0x05,
	CMD_SNIFPKT = 0x06,
	CMD_TIMEOUT = 0x07,

	CMD_PWM = 0x10,

	CMD_MODE = 0x20,
	CMD_SEND,
	CMD_RECV,


	CMD_PKT = 0x70,
};

struct cmd_pkt_t {
	byte sof; // = CMD_SOF;
	byte cmd;
	unsigned int len;
	byte data[CMD_BUF_SIZE];
	unsigned long crc;
};

void cmd_init(void);
void cmd_periodic(void);
void cmd_parse(byte buf);

#endif
