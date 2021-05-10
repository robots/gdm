#ifndef USB_UART_CDC_h_
#define USB_UART_CDC_h_

#include "cmd.h"
#include "fifo.h"

extern struct fifo_t usb_rx1_fifo;
extern struct fifo_t usb_rx2_fifo;

void usb_uart_cdc_init(void);

void usb_uart_cdc1_pkt(struct cmd_pkt_t *pkt);
void usb_uart_cdc1_send(const char *buf, uint32_t len);

void usb_uart_cdc2_send(const char *buf, uint32_t len);
#endif
