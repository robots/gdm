#ifndef UART1_h_
#define UART1_h_

typedef void (*uart1_rx_cb_t)(char *, uint32_t);

extern struct fifo_t uart1_rx_fifo;

void uart1_init(void);
void uart1_set(uint32_t br, uint8_t bits, uint8_t stop, uint8_t parity);
void uart1_set_rx_cb(uart1_rx_cb_t cb);
void uart1_disable_int(void);
void uart1_enable_int(void);
void uart1_apply(void);
void uart1_send(const char *buf, uint32_t len);

#endif
