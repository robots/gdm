#ifndef UART3_h_
#define UART3_h_

typedef void (*uart3_rx_cb_t)(char *, uint32_t);

extern struct fifo_t uart3_rx_fifo;

void uart3_init(void);
void uart3_set(uint32_t br, uint8_t bits, uint8_t stop, uint8_t parity);
void uart3_set_rx_cb(uart3_rx_cb_t cb);
void uart3_disable_int(void);
void uart3_enable_int(void);
void uart3_apply(void);
void uart3_send(const char *buf, uint32_t len);

#endif
