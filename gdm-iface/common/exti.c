#include "platform.h"

#include "exti.h"

#define EXTI_COUNT 21

exti_fnc_t exti_fnc[EXTI_COUNT];

uint8_t exti_irqchan[EXTI_COUNT] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	// below are internal events
	PVD_IRQn, RTCAlarm_IRQn, OTG_FS_WKUP_IRQn, ETH_WKUP_IRQn
};

void exti_set_handler(uint8_t exti, exti_fnc_t fnc)
{
	if (exti >= EXTI_COUNT)
		return;

	exti_fnc[exti] = fnc;
}

void exti_enable(uint8_t exti, EXTITrigger_TypeDef trig, uint8_t gpio)
{
	if (exti >= EXTI_COUNT)
		return;

	EXTI->IMR |= 1 << exti;
	if (trig == EXTI_Trigger_Rising_Falling) {
		EXTI->RTSR |= 1 << exti;
		EXTI->FTSR |= 1 << exti;
	} else if (trig == EXTI_Trigger_Rising) {
		EXTI->RTSR |= 1 << exti;
	} else if (trig == EXTI_Trigger_Falling) {
		EXTI->FTSR |= 1 << exti;
	}

	if (exti < 16) {
		GPIO_EXTILineConfig(gpio, exti);
	}

	NVIC_EnableIRQ(exti_irqchan[exti]);
	NVIC_SetPriority(exti_irqchan[exti], 10);
}

void exti_reenable(uint8_t exti)
{
	if (exti >= EXTI_COUNT)
		return;

	EXTI->IMR |= 1 << exti;
}

void exti_disable(uint8_t exti)
{
	if (exti >= EXTI_COUNT)
		return;

	EXTI->PR = 1 << exti; 
	EXTI->IMR &= ~(1 << exti);
}

void exti_trigger(uint8_t exti)
{
	if (exti >= 20)
		return;

	EXTI->SWIER |= 1 << (exti);
}

int exti_status(uint8_t exti)
{
	if (exti >= 20)
		return 0;

	return !!(EXTI->IMR & (1<<exti));
}

#define EXEC(x) \
	do { \
		if (EXTI->PR & EXTI->IMR & (1 << (x))) { \
			if (exti_fnc[(x)]) \
				exti_fnc[(x)]();\
			EXTI->PR = 1 << (x); \
		} \
	} while (0);

void EXTI0_IRQHandler(void)
{
	EXEC(0);
}

void EXTI1_IRQHandler(void)
{
	EXEC(1);
}

void EXTI2_IRQHandler(void)
{
	EXEC(2);
}

void EXTI3_IRQHandler(void)
{
	EXEC(3);
}

void EXTI4_IRQHandler(void)
{
	EXEC(4);
}

void EXTI9_5_IRQHandler(void)
{
	EXEC(5);
	EXEC(6);
	EXEC(7);
	EXEC(8);
	EXEC(9);
}

void EXTI15_10_IRQHandler(void)
{
	EXEC(10);
	EXEC(11);
	EXEC(12);
	EXEC(13);
	EXEC(14);
	EXEC(15);
}

void PVD_IRQHandler(void)
{
	EXEC(16);
}

void RTCAlarm_IRQHandler(void)
{
	EXEC(17);
}

