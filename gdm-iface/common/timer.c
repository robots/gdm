#include "platform.h"

#include <string.h>

#include "timer.h"

timer_fnc_t timer_handler[] = {NULL, NULL};
const uint8_t timer_irqc[] = {TIM2_IRQn, TIM3_IRQn};
const uint32_t timer_bus[] = {1, 1};
const uint32_t timer_rcc[] = {RCC_APB1ENR_TIM2EN, RCC_APB1ENR_TIM3EN};
const TIM_TypeDef *timer_tim[] = {TIM2, TIM3 };
uint32_t timer_timeouts[] = {0, 0, 0};

void timer_init(uint8_t timer)
{
	TIM_TypeDef *timx = (TIM_TypeDef *)timer_tim[timer];
	uint32_t prescaler = SystemFrequency / 1000000;

	timer_timeouts[timer] = 0;
	timer_handler[timer] = NULL;

	if (timer_bus[timer] == 1) {
		RCC->APB1ENR |= timer_rcc[timer];
	} else if (timer_bus[timer] == 2) {
		RCC->APB2ENR |= timer_rcc[timer];
	} else {
		return;
	}

	timx->ARR = 10000; // dummy value
	timx->PSC = prescaler;
	timx->EGR |= TIM_EGR_UG;

	timx->CR1 |= TIM_CR1_URS;

	timx->CR1 |= TIM_CR1_URS;

	//TIM_ITConfig(timx, TIM_IT_Update, ENABLE);
	timx->DIER |= TIM_DIER_UIE;

	NVIC_SetPriority(timer_irqc[timer], 10);
	NVIC_EnableIRQ(timer_irqc[timer]);
}

void timer_intconf(uint8_t timer, int prio)
{
	NVIC_SetPriority(timer_irqc[timer], prio);
}

void timer_delay(uint8_t timer, uint32_t microsec)
{
	TIM_TypeDef *timx = (TIM_TypeDef *)timer_tim[timer];

	timer_timeout(timer, microsec);
	while (timx->CR1 & TIM_CR1_CEN);
}

void timer_set_handler(uint8_t timer, timer_fnc_t fnc)
{
	timer_handler[timer] = fnc;
}

void timer_timeout(uint8_t timer, uint32_t microsec)
{
	TIM_TypeDef *timx = (TIM_TypeDef *)timer_tim[timer];

	timx->CR1 &= ~TIM_CR1_CEN;

	timer_timeouts[timer] = microsec >> 16;
	timx->ARR = (microsec & 0xFFFF);

	timx->CR1 |= TIM_CR1_CEN;
	timx->EGR |= TIM_EGR_UG;
}

void timer_abort(uint8_t timer)
{
	TIM_TypeDef *timx = (TIM_TypeDef *)timer_tim[timer];

	timx->CR1 &= ~TIM_CR1_CEN;
	timx->SR = (uint16_t)~TIM_SR_UIF;
}

#define TIM_ISR(x) void TIM ## x ## _IRQHandler(void) \
{ \
	if (TIM ## x->SR & TIM_SR_UIF) { \
		TIM ## x->SR = (uint16_t)~TIM_SR_UIF; \
\
		if (timer_timeouts[TIMER_TIM ## x] == 0) { \
			TIM ## x->CR1 &= ~TIM_CR1_CEN; \
\
			if (timer_handler[TIMER_TIM ## x]) { \
				timer_handler[TIMER_TIM ## x](); \
			} \
		} else { \
			timer_timeouts[TIMER_TIM ## x]--; \
			TIM ## x->ARR = 0xffff; \
			TIM ## x->EGR |= TIM_EGR_UG; \
		} \
	} \
}

TIM_ISR(2);
TIM_ISR(3);
