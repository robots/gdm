#include "gdm.h"

#define MIN(x,y)           ((x)>(y)?(y):(x))
#define ARRAY_SIZE(x)      ((sizeof(x)/sizeof(x[0])))

enum {
	IO1_PIN,
	IO2_PIN,
	IO3_PIN,
};

struct gdm_iface_t gdm_iface[] = {
	{
		.i = 0,
		.io1_pin = 2, // TX / GDM_DAT
		.io2_pin = 4, // RX / GDM_ACK
		.io3_pin = 7, // CLK / GDM_CLK

		.mode = 0,
		.bitdly = 50,
		.timeout = 1000,
		.debounce = 3,
	},
	{
		.i = 1,
    .io1_pin = 8, // TX / GDM_DAT
    .io2_pin = 12, // RX / GDM_ACK
    .io3_pin = 13, // CLK / GDM_CLK
    
		.mode = 0,
		.bitdly = 50,
		.timeout = 1000,
		.debounce = 3,
	},
};

static char wait_state(struct gdm_iface_t *iface, byte pin, byte state);
static char gdm_read_request(struct gdm_iface_t *iface);
static char gdm_ack_read_request(struct gdm_iface_t *iface);

static void pin_input(byte pin)
{
	pinMode(pin, INPUT_PULLUP);
}

static void pin_output(byte pin)
{
	pinMode(pin, OUTPUT);
}

static void pin_set(unsigned long pin, byte state, unsigned int bitdly)
{
	if (state) {
		pinMode(pin, INPUT_PULLUP);
    digitalWrite(pin, HIGH);
	} else {
    digitalWrite(pin, LOW);
		pinMode(pin, OUTPUT);
	}

	delayMicroseconds(bitdly);
}

static byte pin_state(byte pin)
{
	return digitalRead(pin);
}

static void gdm_init_iface(struct gdm_iface_t *iface)
{
	pin_input(iface->io1_pin);
	pin_input(iface->io2_pin);
	pin_input(iface->io3_pin);

	pin_set(iface->io1_pin, 1, iface->bitdly);
	pin_set(iface->io2_pin, 1, iface->bitdly);
	pin_set(iface->io3_pin, 1, iface->bitdly);
}

void gdm_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(gdm_iface); i ++) {
		gdm_iface[i].i = i;
		gdm_init_iface(&gdm_iface[i]);
	}
}

void gdm_periodic(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(gdm_iface); i++ ) {
		struct gdm_iface_t *iface = &gdm_iface[i];
		if (iface->mode == 1) {
			if (gdm_read_request(iface)) {
				
				// first we need to ack this request from board
				gdm_ack_read_request(iface);

				struct gdm_packet_t *pkt = &iface->rx_pkt;
				unsigned int pktlen;

				// angle board sends data in multi-packets
				// first is 0xff with 1 byte
				// if byte is 0x20 - 0x40 - it means byte-0x20 packets are following
				// if byte is 0x50 - 0x60 - it means error on angle board side.
				pkt->len = GDM_PACKET_DATA_LEN;
				gdm_recv_pkt(iface, pkt);
				pktlen = 2 + pkt->len;

				if (pkt->label == 0xff) {
					byte v = pkt->data[0];

					if ((0x1f < v) && (v < 0x40)) {
						// this is multipacket, get packet count
						v -= 0x20;

						// get pointer right after the 0xff packet
						uintptr_t ptr = (uintptr_t)pkt;
						ptr += pkt->len + 2;

						for (size_t n = 0; n < v; n++) {
							struct gdm_packet_t p;	
							gdm_recv_pkt(iface, &p);
							
							// copy packet
							memcpy((void *)ptr, &p, 2 + p.len);

							// advance pointer, and pkt length
							ptr += p.len + 2;
							pktlen += p.len + 2;
						}
					} else if ((0x50 < v) && (v < 0x60)) {
						// error, just send this packet to host, no other packet is coming
					}
				}

				if (iface->rx_cb) {
					iface->rx_cb(i, (byte *)pkt, pktlen);
				}
			}
		}
	}
}

void gdm_set_rx_cb(struct gdm_iface_t *iface, gdm_rx_cb_t cb)
{
	iface->rx_cb = cb;
}

void gdm_set_params(struct gdm_iface_t *iface, unsigned int timeout, unsigned int bitdly, byte deb)
{
	iface->timeout = timeout;
	iface->bitdly = bitdly;
	iface->debounce = deb;
}

char gdm_selftest(struct gdm_iface_t *iface)
{
	char m = iface->mode;

	gdm_set_mode(iface, 0);

	pin_set(iface->io1_pin, 0, iface->bitdly);
	if (pin_state(iface->io2_pin) == 0) {
		return 0x102;
	}
	if (pin_state(iface->io3_pin) == 0) {
		return 0x103;
	}

	pin_set(iface->io1_pin, 1, iface->bitdly);

	pin_set(iface->io2_pin, 0, iface->bitdly);
	if (pin_state(iface->io1_pin) == 0) {
		return 0x201;
	}
	if (pin_state(iface->io3_pin) == 0) {
		return 0x203;
	}

	pin_set(iface->io2_pin, 1, iface->bitdly);

	pin_set(iface->io3_pin, 0, iface->bitdly);
	if (pin_state(iface->io1_pin) == 0) {
		return 0x301;
	}
	if (pin_state(iface->io2_pin) == 0) {
		return 0x302;
	}

	pin_set(iface->io3_pin, 1, iface->bitdly);

	gdm_set_mode(iface, m);

	return 0;
}

void gdm_set_mode(struct gdm_iface_t *iface, int mode)
{
	if (iface->mode == mode) {
		return;
	}

	iface->mode = mode;
}

static char wait_state(struct gdm_iface_t *iface, byte pin, byte state)
{
	unsigned long t;
	unsigned long deb;
	byte ipin = iface->io1_pin;

	if (pin == IO2_PIN) {
		ipin = iface->io2_pin;
	} else if (pin == IO3_PIN) {
		ipin = iface->io3_pin;
	}

	t = millis();
	deb = 0;
	while(1) {
		delayMicroseconds(iface->bitdly);
		if (pin_state(ipin) == state) {
			deb++;
			if (deb > iface->debounce) {
				break;
			}
		} else {
			deb = 0;
		}
		if (millis() - t > iface->timeout) {
			return 1;
		}
	}
	return 0;
}

// used for geodat to start comm
// not tested
char gdm_send_start(struct gdm_iface_t *iface)
{
	char ret = 0;

	pin_set(iface->io3_pin, 0, iface->bitdly);

	// wait for bus ready
	ret = wait_state(iface, IO2_PIN, 0);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {
		// idle channels
		pin_set(iface->io3_pin, 1, iface->bitdly);

		// wait for bit read
		ret = wait_state(iface, IO2_PIN, 1);
		if (ret) {
			ret = 2;
		}
	}

	return ret;
}

static char gdm_read_request(struct gdm_iface_t *iface)
{
	// if IO1 is pulled low, board want's us to read
	return !pin_state(iface->io1_pin);
}

static char gdm_ack_read_request(struct gdm_iface_t *iface)
{
	char ret;

	// IO1 is low
	
	// IO2 to output, and low
	pin_set(iface->io2_pin, 0, iface->bitdly);

	// wait for board to pull IO3 down
	ret = wait_state(iface, IO3_PIN, 0);
	if (ret) {
		return 1;
	}

	// release IO2
	pin_set(iface->io2_pin, 1, iface->bitdly);

	// wait for board to release IO3
	ret = wait_state(iface, IO3_PIN, 1);
	if (ret) {
		return 2;
	}

	// and done!
	return 0;
}


static char send_bit(struct gdm_iface_t *iface, byte b, byte *p)
{
	char ret = 0;

	// wait for bus ready
	ret = wait_state(iface, IO2_PIN, 1);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {

		// setup bit
		*p ^= b&1;
		if (b&1) {
			pin_set(iface->io1_pin, 1, iface->bitdly);
		} else {
			pin_set(iface->io1_pin, 0, iface->bitdly);
		}

		// tell other side bit is ready
		pin_set(iface->io3_pin, 0, iface->bitdly);

		// wait for bit read
		ret = wait_state(iface, IO2_PIN, 0);
		if (ret) {
			ret = 2;
		}
	}

	// idle channels
	pin_set(iface->io3_pin, 1, iface->bitdly);
	pin_set(iface->io1_pin, 1, iface->bitdly);

	return ret;
}

static char recv_bit(struct gdm_iface_t *iface, byte *b, byte *par)
{
	char ret = 0;

	// tell other side we are ready to read
	pin_set(iface->io2_pin, 0, iface->bitdly);

	ret = wait_state(iface, IO3_PIN, 0);
	if (ret) {
		ret = 1;
	}

	if (ret == 0) {
		// read bit
		if (pin_state(iface->io1_pin)) {
			*b |= 1;
		} else {
			*b |= 0;
		}

		// acknowledge
		pin_set(iface->io2_pin, 1, iface->bitdly);

		// wait for signal to go high
		ret = wait_state(iface, IO3_PIN, 1);

		if (ret == 0) {
			*par ^= *b & 1;
		}
	}

	// idle output channel
	pin_set(iface->io2_pin, 1, iface->bitdly);

	return ret;
}

char gdm_send_byte(struct gdm_iface_t *iface, byte b, byte *p)
{
	char ret = 0;

	for (int i = 0; i < 8; i++) {
		ret = send_bit(iface, b & 1, p);
		if (ret) {
			ret = 0x10*i+ret;
			break;
		}
		b >>= 1;
	}

	return ret;
}

char gdm_recv_byte(struct gdm_iface_t *iface, byte *b, byte *p)
{
	char ret = 0;

	*b = 0;
	for (int i = 0; i < 8; i++) {
		ret = recv_bit(iface, b, p);
		if (ret) {
			ret = 0x10*i+ret;
			break;
		}
		*b = (*b >> 1) | (*b << 7);
	}

	return ret;
}

static char send_ack(struct gdm_iface_t *iface, byte *ack, byte *p)
{
	char ret = 0;
	byte b;
	byte pp; // dummy parity

	ret = send_bit(iface, *p, &pp);
	if (ret != 0) {
		return 0x10 + ret;
	}

	ret = recv_bit(iface, ack, &pp);
	if (ret != 0) {
		return 0x20 + ret;
	}

	ret = recv_bit(iface, &b, &pp);
	if (ret != 0) {
		return 0x30 + ret;
	}

	*p = b & 1;

	return 0;
}

static char recv_ack(struct gdm_iface_t *iface, byte ack, byte *p)
{
	char ret = 0;
	byte b;
	byte pp;

	// do dummy read, parity will update
	ret = recv_bit(iface, &b, p);
	if (ret != 0) {
		return 0x10 + ret;
	}

	// 1 means continue transmission
	ret = send_bit(iface, ack, &pp);
	if (ret != 0) {
		return 0x20 + ret;
	}

	*p ^= 1;

	ret = send_bit(iface, *p, &pp);
	if (ret != 0) {
		return 0x30 + ret;
	}
	return 0;
}

long gdm_send_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt)
{
	byte ack = 1;
	byte par = 0;
	long ret = 0;
	unsigned long i;

	ret = gdm_send_byte(iface, pkt->label, &par);
	if (ret != 0) return 0x100+ret;

	ret = gdm_send_byte(iface, pkt->len, &par);
	if (ret != 0) return 0x200+ret;

	for (i = 0; i < pkt->len; i++) {
		ret = gdm_send_byte(iface, pkt->data[i], &par);
		if (ret != 0) return 0x300+ret;

		ret = send_ack(iface, &ack, &par);
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

long gdm_recv_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt)
{
	byte ack = 1;
	byte par = 0;
	long ret = 0;
	unsigned long i;
	byte lenin;

	ret = gdm_recv_byte(iface, &pkt->label, &par);
	if (ret != 0) return 0x100+ret;

	ret = gdm_recv_byte(iface, &lenin, &par);
	if (ret != 0) return 0x200+ret;

	lenin = MIN(GDM_PACKET_DATA_LEN, lenin);
	pkt->len = lenin;
	for (i = 0; i < lenin; i++) {
		ret = gdm_recv_byte(iface, &pkt->data[i], &par);
		if (ret != 0) return 0x300+ret;

		ret = recv_ack(iface, ack, &par);
		if (ret != 0) return 0x400+ret;

		par = 0;
	}

	return 0;
}
