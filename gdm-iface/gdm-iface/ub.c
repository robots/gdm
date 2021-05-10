#include "platform.h"
#include "usb.h"
#include "usb_cdc.h"

#include "usb_uart_cdc.h"
#include "uart3.h"
#include "ub.h"

struct usb_cdc_line_coding ub_cdc_line = {
    .dwDTERate          = 9600,
    .bCharFormat        = USB_CDC_1_STOP_BITS,
    .bParityType        = USB_CDC_NO_PARITY,
    .bDataBits          = 8,
};

static void ub_rx_cb(char *b, uint32_t l);

void ub_init(void)
{
	uart3_init();
	uart3_set_rx_cb(ub_rx_cb);
	ub_apply_setting();
}

void ub_periodic(void)
{
	if (!FIFO_EMPTY(&usb_rx2_fifo)) {
		char *b = fifo_get_read_addr(&usb_rx2_fifo);
		uint32_t l = fifo_get_read_count_cont(&usb_rx2_fifo);

		uart3_send(b, l);

		fifo_read_done_count(&usb_rx2_fifo, l);
	}
/*
	if (!FIFO_EMPTY(&uart3_rx_fifo)) {
		char *b = fifo_get_read_addr(&uart3_rx_fifo);
		uint32_t l = fifo_get_read_count_cont(&uart3_rx_fifo);

		usb_uart_cdc2_send(b, l);

		fifo_read_done_count(&uart3_rx_fifo, l);
	}
*/
}

void ub_apply_setting(void)
{
	uart3_set(ub_cdc_line.dwDTERate, ub_cdc_line.bDataBits, ub_cdc_line.bCharFormat, ub_cdc_line.bParityType); 
}

static void ub_rx_cb(char *b, uint32_t l)
{
	usb_uart_cdc2_send(b, l);
}
