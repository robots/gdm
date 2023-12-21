#ifndef GDM_h_
#define GDM_h_

#include "platform.h"
#include "fifo.h"
#include "gpio.h"

#define GDM_PACKET_DATA_LEN 255

struct gdm_packet_t;

typedef void(*gdm_rx_cb_t)(uint8_t i, uint8_t *pkt, uint16_t len);

enum {
	GDM_IFACE_0,
	GDM_IFACE_1,
};

struct gdm_packet_t {
	uint8_t label;
	uint8_t len;
	uint8_t data[GDM_PACKET_DATA_LEN];
};

struct gdm_iface_t {
	size_t i;
	GPIO_TypeDef *io1_gpio;
	uint32_t io1_pin;
	GPIO_TypeDef *io2_gpio;
	uint32_t io2_pin;
	GPIO_TypeDef *io3_gpio;
	uint32_t io3_pin;

	int mode;

	uint16_t bitdly;
	uint16_t timeout;
	uint8_t debounce;

	struct gdm_packet_t rx_pkt;
	struct gdm_packet_t tx_pkt;

	gdm_rx_cb_t rx_cb;
};

extern struct gdm_iface_t gdm_iface[2];

void gdm_init(void);
void gdm_periodic(void);
void gdm_set_rx_cb(struct gdm_iface_t *iface, gdm_rx_cb_t cb);
void gdm_set_params(struct gdm_iface_t *iface, uint16_t timeout, uint16_t bitdly, uint8_t deb);
int gdm_selftest(struct gdm_iface_t *iface);
void gdm_set_mode(struct gdm_iface_t *iface, int mode);

int gdm_send_start(struct gdm_iface_t *iface);

int gdm_send_byte(struct gdm_iface_t *iface, uint8_t b, uint8_t *p);
int gdm_recv_byte(struct gdm_iface_t *iface, uint8_t *b, uint8_t *p);

int gdm_send_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt);
int gdm_recv_pkt(struct gdm_iface_t *iface, struct gdm_packet_t *pkt);

#endif
