#include "platform.h"
#include "gpio.h"

// just to satisfy NULL
#include <string.h>

#include "bsp_spi.h"
#include "spi.h"

volatile uint8_t spi_locked = 0;
volatile uint8_t spi_done = 1;
volatile uint8_t spi_csn = 0;
volatile uint8_t spi_dummy = 0;
volatile uint8_t spi_inited = 0;

static void spi_cs(uint8_t state);

enum {
	GPIO_SCK,
	GPIO_MOSI,
	GPIO_MISO,
};

void spi_init(void)
{
	if (spi_inited == 1)
		return;

	spi_inited = 1;

	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	// Configure SPI1 pins
	gpio_init(spi_gpio, spi_gpio_cnt);
	gpio_init(spi_cs_gpio, spi_cs_gpio_cnt);

	SPI1->CR1 = 0x104; // Master
	SPI1->CR1 |= SPI_CR1_SSM;
	SPI1->CR1 |= // prescaler;

	SPI1->CR1 |= SPI_CR1_SPE;

	spi_cs(0);
}

void spi_deinit(void)
{
	SPI1->CR1 &= ~SPI_CR1_SPE;
}

static void spi_cs(uint8_t state)
{
	const struct gpio_init_table_t *cs;

	cs = &spi_cs_gpio[spi_csn];

	if (state) {
		gpio_set(cs, GPIO_RESET);
	} else {
		gpio_set(cs, GPIO_SET);
	}
}

uint8_t spi_is_done()
{
	return spi_done;
}

uint8_t spi_is_locked(void)
{
	return spi_locked;
}

uint8_t spi_lock(uint8_t cs)
{
	if (spi_locked == 1) {
		return 1;
	}

	spi_locked = 1;
	spi_csn = cs;
	spi_done = 0;

	return 0;
}

uint8_t spi_unlock(void)
{
	spi_locked = 0;
	return 0;
}

void spi_cs_on(void)
{
	spi_cs(1);
}

void spi_cs_off(void)
{
	spi_cs(0);
}

uint8_t spi_wait_miso(uint8_t state)
{
	volatile uint32_t time = 0xFFFFFF;
	struct gpio_init_table_t gpio_miso = spi_gpio[GPIO_MISO];


	while (--time && ((!!(gpio_miso.gpio->IDR & gpio_miso.pin)) != (!!state)));

	return (time > 0)?0:1;
}

uint8_t spi_send(const uint8_t *outbuf, uint8_t *inbuf, uint32_t size)
{
	uint32_t i;
	volatile uint8_t dummy = 0;
	(void)dummy;

	for (i = 0; i < size; i ++) {
		while (!(SPI1->SR & SPI_SR_TXE));
		if (outbuf) {
			SPI1->DR = outbuf[i];
		} else {
			SPI1->DR = 0xff;
		}
		while (!(SPI1->SR & SPI_SR_RXNE));
		if (inbuf) {
			inbuf[i] = SPI1->DR;
		} else {
			dummy = SPI1->DR;
		}
	}

	return 0;
}

/*
uint8_t spi_send(uint8_t *outbuf, uint8_t *inbuf, uint32_t size)
{
	spi_done = 0;

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

	if (inbuf == NULL) {
		DMA1_Channel2->CMAR = (uint32_t)&spi_dummy;
		DMA1_Channel2->CCR &= ~(DMA_CCR_MINC);
	} else {
		DMA1_Channel2->CMAR = (uint32_t)inbuf;
		DMA1_Channel2->CCR |= DMA_CCR_MINC;
	}
	DMA1_Channel2->CNDTR = size;
	DMA1_Channel2->CCR |= DMA_CCR_CEN;

	if (outbuf != NULL) {
		SPI1->DR = outbuf[0];

		if (size > 1) {
			DMA1_Channel3->CMAR = (uint32_t)&outbuf[1];
			DMA1_Channel3->CCR |= DMA_CCR_MINC; 
			DMA1_Channel3->CNDTR = size - 1;
			DMA1_Channel3->CCR |= DMA_CCR_CEN;

		} else {
			//SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);
			//SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, ENABLE);
		}
	} else {
		SPI1->DR = spi_dummy;

		if (size > 1) {
			DMA1_Channel3->CMAR = (uint32_t)&spi_dummy;
			DMA1_Channel3->CNDTR = size - 1;
			DMA1_Channel3->CCR &= ~(DMA_CCR_MINC);
			DMA1_Channel3->CCR |= DMA_CCR_CEN;
		}
	}

	return 0;
}

void spi_wait_done()
{
	while (spi_done == 0);
}

void spi_send_blocking(uint8_t *outbuf, uint8_t *inbuf, uint32_t size)
{
	spi_send(outbuf, inbuf, size);
	spi_wait_done();
}

void SPI1_IRQHandler(void)
{
	SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, DISABLE);
	SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, DISABLE);
}

void DMA1_Channel2_3_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC2)) {
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

		DMA_ClearITPendingBit(DMA1_IT_GL2);
		DMA1_Channel2->CCR &= ~DMA_CCR_CEN;


		spi_done = 1;
	}

	if (DMA_GetITStatus(DMA1_IT_TC3)) {
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);

		DMA_ClearITPendingBit(DMA1_IT_GL3);
		DMA1_Channel3->CCR &= ~DMA_CCR_CEN;
	}
}
*/
