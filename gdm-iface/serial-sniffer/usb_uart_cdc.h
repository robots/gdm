#ifndef USB_UART_CDC_h_
#define USB_UART_CDC_h_

#include "fifo.h"

extern struct fifo_t usb_rx_fifo;

void usb_uart_cdc_init(void);
void usb_uart_cdc_send(const char *buf, uint32_t len);
#endif
