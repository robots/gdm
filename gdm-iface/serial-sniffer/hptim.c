
#include "platform.h"

#include "hptim.h"

volatile uint16_t hptim_cnt;

void hptim_init(void)
{
	uint32_t prescaler = SystemFrequency / 1000000;

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM2EN;

	TIM2->PSC = prescaler - 1;
	TIM2->ARR = 0xffff;
	TIM2->CR2 = 0x20; // output TRGO

	TIM3->ARR = 0xffff;
	TIM3->PSC = 0;
	TIM3->SMCR = 7 | (1 << 4); // external clock, Internal trigger TIM2

	TIM3->CR1 |= TIM_CR1_CEN;
	TIM2->CR1 |= TIM_CR1_CEN;
}

uint32_t hptim_get(void)
{
	uint32_t x;
	uint32_t y;

	do {
		x = TIM3->CNT;
		y = TIM2->CNT;
	} while (TIM3->CNT != x);

	return (x << 16) | y;
}
