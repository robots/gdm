#include "platform.h"

#include <string.h>

#include "gpio.h"
#include "fifo.h"

#include "bsp_uart.h"

#include "uart3.h"

#define UART_BUFFER 256


uart3_rx_cb_t uart3_rx_cb;
struct fifo_t uart3_tx_fifo;
uint8_t uart3_tx_buf[UART_BUFFER];

// dma stuff
volatile uint8_t uart3_sending = 0;
volatile uint32_t uart3_sending_size;

void uart3_set(uint32_t br, uint8_t bits, uint8_t stop, uint8_t parity)
{
	USART3->CR1 &= ~USART_CR1_UE;

	if (bits == 8) {
		USART3->CR1 &= ~USART_CR1_M;
	} else if (bits == 9) {
		USART3->CR1 |= USART_CR1_M;
	} else {
		return;
	}
	
	USART3->CR2 &= ~(3<<12);
	if (stop == 0) { // USB_CDC_1_STOP_BITS
		USART3->CR2 |= 0 << 12;
	} else if (stop == 1) { // USB_CDC_1_5_STOP_BITS
		USART3->CR2 |= 3 << 12;
	} else if (stop == 2) { // USB_CDC_2_STOP_BITS
		USART3->CR2 |= 2 << 12;
	} else {
		return;
	}

	USART3->CR2 &= ~(3<<9);
	if (parity == 0) {
	} else if (parity == 1) { // ODD
		USART3->CR2 |= (3<<9);
	} else if (parity == 2) { // EVEN
		USART3->CR2 |= (2<<9);
	} else { // others not supported (MARK/SPACE)
		return;
	}

#if 0
	uint32_t i = (0x19 * SystemFrequency_APB1Clk) / (0x04 * br);
	uint32_t brr = (i/0x64) << 4;
	uint32_t f = i - (0x64 * i);
	brr |= ((((f * 0x10) + 0x32) / 0x64)) & 0x0f;

	USART3->BRR = brr;
#else
	USART3->BRR = ((2*SystemFrequency_APB1Clk) / br + 1) / 2;
#endif

	USART3->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart3_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	gpio_init(uart3_gpio, uart3_gpio_cnt);

	/* DMA_Channel (triggered by USART Tx event) */
	DMA1_Channel2->CPAR = (uint32_t)&USART3->DR;
	DMA1_Channel2->CCR = 0x3092;

	NVIC_SetPriority(USART2_IRQn, 1);
	NVIC_EnableIRQ(USART3_IRQn);

	NVIC_SetPriority(DMA1_Channel2_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	fifo_init(&uart3_tx_fifo, uart3_tx_buf, sizeof(uint8_t), UART_BUFFER);

	USART3->CR1 |= USART_CR1_RXNEIE;
	USART3->CR3 |= USART_CR3_DMAT;

	uart3_set(9600, 8, 1, 0);
}

void uart3_set_rx_cb(uart3_rx_cb_t cb)
{
	uart3_rx_cb = cb;
}

void uart3_disable_int(void)
{
	NVIC_DisableIRQ(USART3_IRQn);
}

void uart3_enable_int(void)
{
	NVIC_EnableIRQ(USART3_IRQn);
}

void uart3_send(const char *b, uint32_t l)
{
	fifo_write_buf(&uart3_tx_fifo, b, l);

	NVIC_DisableIRQ(DMA1_Channel2_IRQn);

	if (LIKELY(uart3_sending == 0)) {
		// disable receiver enable transmitter
		//USART3->CR1 &= ~USART_CR1_RE;
		//USART3->CR1 |= USART_CR1_TE;

		uart3_sending = 1;
		uart3_sending_size = (uint32_t)fifo_get_read_count_cont(&uart3_tx_fifo);
		DMA1_Channel2->CMAR = (uint32_t)fifo_get_read_addr(&uart3_tx_fifo);
		DMA1_Channel2->CNDTR = uart3_sending_size;
		DMA1_Channel2->CCR |= 1;
	}

	NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

void DMA1_Channel2_IRQHandler()
{
	DMA1_Channel2->CCR &= ~1;
	DMA1->IFCR |= DMA_IFCR_CTCIF2;

	fifo_read_done_count(&uart3_tx_fifo, uart3_sending_size);

	if (UNLIKELY(FIFO_EMPTY(&uart3_tx_fifo))) {
		uart3_sending = 0;
		uart3_sending_size = 0;
		return;
	}

	uart3_sending_size = (uint32_t)fifo_get_read_count_cont(&uart3_tx_fifo);
	DMA1_Channel2->CMAR = (uint32_t)fifo_get_read_addr(&uart3_tx_fifo);
	DMA1_Channel2->CNDTR = uart3_sending_size;
	DMA1_Channel2->CCR |= 1;
}

void USART3_IRQHandler()
{
	if (USART3->SR & USART_SR_RXNE) {
		char ch = USART3->DR & 0xff;
	
		if (uart3_rx_cb) {
			uart3_rx_cb(&ch, 1);
		}
	}
}

