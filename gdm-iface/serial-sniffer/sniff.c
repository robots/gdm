#include "platform.h"

#include "usb.h"
#include "usb_cdc.h"

#include "usb_uart_cdc.h"
#include "uart1.h"
#include "uart3.h"

#include "hptim.h"
#include "util.h"

#include "sniff.h"

#define TRANSCATION_TIMEOUT (1000*1000)

struct {
	uint32_t baudrate;
	uint8_t stop;
	uint8_t parity;
	uint8_t databits;
} sniff_uart_cfg = {
    .baudrate = 9600,
    .stop     = USB_CDC_1_STOP_BITS,
    .parity   = USB_CDC_NO_PARITY,
    .databits = 8,
};

const char sniff_console_endc[] = "\033[0m";
const char *sniff_console_colors[] = 
{
	"", // 0 is not uart
	"\033[91m", // uart1 - red
	"\033[94m", // timestamp - blue
	"\033[92m", // uart3 - green
};


volatile int sniff_last_uart = 0;
uint32_t sniff_last_ts = 0;

static void sniff_rx1_cb(char *b, uint32_t l);
static void sniff_rx3_cb(char *b, uint32_t l);

void sniff_init(void)
{
	hptim_init();

	uart1_init();
	uart1_set_rx_cb(sniff_rx1_cb);

	uart3_init();
	uart3_set_rx_cb(sniff_rx3_cb);

	// todo: make settable runtime...
	uart1_set(sniff_uart_cfg.baudrate, sniff_uart_cfg.databits, sniff_uart_cfg.stop, sniff_uart_cfg.parity);
	uart3_set(sniff_uart_cfg.baudrate, sniff_uart_cfg.databits, sniff_uart_cfg.stop, sniff_uart_cfg.parity);
}

void sniff_periodic(void)
{

}

static void sniff_print_hdr(int uart)
{
	char temp[100];

	uint32_t delta = hptim_get() - sniff_last_ts;
	sniff_last_ts = hptim_get();

	if ((sniff_last_uart != uart) || (delta > TRANSCATION_TIMEOUT)) {
		// timestamp on new line
		usb_uart_cdc_send("\n\r", 2);

		// timestamp is in blue
		uint32_t l = strlen(sniff_console_colors[2]);
		usb_uart_cdc_send(sniff_console_colors[2], l);

		// convert timestamp to yyy.xxx
		uint32_t ll = ts_to_str(delta, temp);
		usb_uart_cdc_send(temp, ll);

		// add "ms:"
		usb_uart_cdc_send("ms:", 3);

		// add new line after timestamp
		usb_uart_cdc_send("\n\r", 2);
	}

/*
	if (sniff_last_uart != uart) {
		// uart text on next line
		usb_uart_cdc_send("\n\r", 2);
	}
*/
	sniff_last_uart = uart;

	// change color for uart data
	uint32_t l = strlen(sniff_console_colors[uart]);
	usb_uart_cdc_send(sniff_console_colors[uart], l);
}

static void sniff_rx1_cb(char *b, uint32_t l)
{
	sniff_print_hdr(1);

	if (b[0] == '\r') {
		usb_uart_cdc_send("\n", 1); 
	}

	usb_uart_cdc_send(b, l);
}

static void sniff_rx3_cb(char *b, uint32_t l)
{
	sniff_print_hdr(3);

	if (b[0] == '\r') {
		usb_uart_cdc_send("\n", 1); 
	}

	usb_uart_cdc_send(b, l);
}
