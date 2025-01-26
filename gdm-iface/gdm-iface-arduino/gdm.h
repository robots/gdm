#ifndef GDM_h_
#define GDM_h_

#include "fifo.h"
#include <Arduino.h>

#define GDM_PACKET_DATA_LEN 20

struct gdm_packet_t;

typedef void(*gdm_rx_cb_t)(byte i, byte *pkt, unsigned int len);

enum {
	GDM_IFACE_0,
	GDM_IFACE_1,
};

struct gdm_packet_t {
	byte label;
	byte len;
	byte data[GDM_PACKET_DATA_LEN];
};

struct gdm_iface_t {
	size_t i;
	byte io1_pin;
	byte io2_pin;
	byte io3_pin;

	char mode;

	unsigned int bitdly;
	unsigned int timeout;
	byte debounce;

	struct gdm_packet_t rx_pkt;
	struct gdm_packet_t tx_pkt;

	gdm_rx_cb_t rx_cb;
};

extern struct gdm_iface_t gdm_iface[2];

void gdm_init(void);
void gdm_periodic(void);
void gdm_set_rx_cb(struct gdm_iface_t *iface, gdm_rx_cb_t cb);
void gdm_set_params(struct gdm_iface_t *iface, unsigned int timeout, unsigned int bitdly, byte deb);
char gdm_selftest(struct gdm_iface_t *iface);
void gdm_set_mode(struct gdm_iface_t *iface, int mode);

char gdm_send_start(struct gdm_iface_t *iface);

char gdm_send_byte(struct gdm_iface_t *iface, byte b, byte *p);
char gdm_recv_byte(struct gdm_iface_t *iface, byte *b, byte *p);

long gdm_send_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt);
long gdm_recv_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt);

#endif
