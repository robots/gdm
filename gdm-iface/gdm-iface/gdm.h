#ifndef GDM_h_
#define GDM_h_


void gdm_init(void);
void gdm_periodic(void);
void gdm_set_mode(int mode);
int gdm_selftest(void);
void gdm_set_params(uint16_t timeout, uint16_t bitdly, uint8_t deb);

int gdm_send_start();
int gdm_send_pkt(uint8_t label, uint8_t *buf, uint8_t len);
int gdm_recv_byte(uint8_t *b, uint8_t *p);
int gdm_recv_pkt(uint8_t *label, uint8_t *buf, uint8_t *len);
#endif
