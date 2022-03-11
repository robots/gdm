#include "platform.h"

#include <string.h>

#include "gpio.h"
#include "fifo.h"

#include "bsp_uart.h"

#include "uart1.h"

#define UART_BUFFER 256


uart1_rx_cb_t uart1_rx_cb;
struct fifo_t uart1_tx_fifo;
uint8_t uart1_tx_buf[UART_BUFFER];

// dma stuff
volatile uint8_t uart1_sending = 0;
volatile uint32_t uart1_sending_size;

void uart1_set(uint32_t br, uint8_t bits, uint8_t stop, uint8_t parity)
{
	USART1->CR1 &= ~USART_CR1_UE;

	if (bits == 8) {
		USART1->CR1 &= ~USART_CR1_M;
	} else if (bits == 9) {
		USART1->CR1 |= USART_CR1_M;
	} else {
		return;
	}
	
	USART1->CR2 &= ~(3<<12);
	if (stop == 0) { // USB_CDC_1_STOP_BITS
		USART1->CR2 |= 0 << 12;
	} else if (stop == 1) { // USB_CDC_1_5_STOP_BITS
		USART1->CR2 |= 3 << 12;
	} else if (stop == 2) { // USB_CDC_2_STOP_BITS
		USART1->CR2 |= 2 << 12;
	} else {
		return;
	}

	USART1->CR2 &= ~(3<<9);
	if (parity == 0) {
	} else if (parity == 1) { // ODD
		USART1->CR2 |= (3<<9);
	} else if (parity == 2) { // EVEN
		USART1->CR2 |= (2<<9);
	} else { // others not supported (MARK/SPACE)
		return;
	}

#if 0
	uint32_t i = (0x19 * SystemFrequency_APB1Clk) / (0x04 * br);
	uint32_t brr = (i/0x64) << 4;
	uint32_t f = i - (0x64 * i);
	brr |= ((((f * 0x10) + 0x32) / 0x64)) & 0x0f;

	USART1->BRR = brr;
#else
	USART1->BRR = ((2*SystemFrequency_APB2Clk) / br + 1) / 2;
#endif

	USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart1_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	gpio_init(uart1_gpio, uart1_gpio_cnt);

	/* DMA_Channel (triggered by USART Tx event) */
	DMA1_Channel4->CPAR = (uint32_t)&USART1->DR;
	DMA1_Channel4->CCR = 0x3092;

	NVIC_SetPriority(USART2_IRQn, 1);
	NVIC_EnableIRQ(USART1_IRQn);

	NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);

	fifo_init(&uart1_tx_fifo, uart1_tx_buf, sizeof(uint8_t), UART_BUFFER);

	USART1->CR1 |= USART_CR1_RXNEIE;
	USART1->CR3 |= USART_CR3_DMAT;

	uart1_set(9600, 8, 1, 0);
}

void uart1_set_rx_cb(uart1_rx_cb_t cb)
{
	uart1_rx_cb = cb;
}

void uart1_disable_int(void)
{
	NVIC_DisableIRQ(USART1_IRQn);
}

void uart1_enable_int(void)
{
	NVIC_EnableIRQ(USART1_IRQn);
}

void uart1_send(const char *b, uint32_t l)
{
	fifo_write_buf(&uart1_tx_fifo, b, l);

	NVIC_DisableIRQ(DMA1_Channel4_IRQn);

	if (LIKELY(uart1_sending == 0)) {
		// disable receiver enable transmitter
		//USART1->CR1 &= ~USART_CR1_RE;
		//USART1->CR1 |= USART_CR1_TE;

		uart1_sending = 1;
		uart1_sending_size = (uint32_t)fifo_get_read_count_cont(&uart1_tx_fifo);
		DMA1_Channel4->CMAR = (uint32_t)fifo_get_read_addr(&uart1_tx_fifo);
		DMA1_Channel4->CNDTR = uart1_sending_size;
		DMA1_Channel4->CCR |= 1;
	}

	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

void DMA1_Channel4_IRQHandler()
{
	DMA1_Channel4->CCR &= ~1;
	DMA1->IFCR |= DMA_IFCR_CTCIF4;

	fifo_read_done_count(&uart1_tx_fifo, uart1_sending_size);

	if (UNLIKELY(FIFO_EMPTY(&uart1_tx_fifo))) {
		uart1_sending = 0;
		uart1_sending_size = 0;
		return;
	}

	uart1_sending_size = (uint32_t)fifo_get_read_count_cont(&uart1_tx_fifo);
	DMA1_Channel4->CMAR = (uint32_t)fifo_get_read_addr(&uart1_tx_fifo);
	DMA1_Channel4->CNDTR = uart1_sending_size;
	DMA1_Channel4->CCR |= 1;
}

void USART1_IRQHandler()
{
	if (USART1->SR & USART_SR_RXNE) {
		char ch = USART1->DR & 0xff;
	
		if (uart1_rx_cb) {
			uart1_rx_cb(&ch, 1);
		}
	}
}

