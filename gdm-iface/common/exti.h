#ifndef EXTI_h_
#define EXTI_h_

#include "platform.h"

typedef void(*exti_fnc_t)(void);

#ifndef EXTI_PortSourceGPIOA
#define EXTI_PortSourceGPIOA       GPIO_PortSourceGPIOA
#define EXTI_PortSourceGPIOB       GPIO_PortSourceGPIOB
#define EXTI_PortSourceGPIOC       GPIO_PortSourceGPIOC
#define EXTI_PortSourceGPIOD       GPIO_PortSourceGPIOD
#define EXTI_PortSourceGPIOE       GPIO_PortSourceGPIOE
#define EXTI_PortSourceGPIOF       GPIO_PortSourceGPIOF
#define EXTI_PortSourceGPIOG       GPIO_PortSourceGPIOG
#define EXTI_PortSourceGPIOH       GPIO_PortSourceGPIOH
#define EXTI_PortSourceGPIOI       GPIO_PortSourceGPIOI
#define EXTI_PortSourceGPIOJ       GPIO_PortSourceGPIOJ
#define EXTI_PortSourceGPIOK       GPIO_PortSourceGPIOK
#endif

enum exti_interrupt_t {
	EXTI_0,
	EXTI_1,
	EXTI_2,
	EXTI_3,
	EXTI_4,
	EXTI_5,
	EXTI_6,
	EXTI_7,
	EXTI_8,
	EXTI_9,
	EXTI_10,
	EXTI_11,
	EXTI_12,
	EXTI_13,
	EXTI_14,
	EXTI_15,
	EXTI_16,
	EXTI_17,
	EXTI_18,
	EXTI_19,
};

void exti_set_handler(uint8_t exti, exti_fnc_t fnc);
void exti_enable(uint8_t exti, EXTITrigger_TypeDef trig, uint8_t gpio);
void exti_reenable(uint8_t exti);
void exti_disable(uint8_t exti);
void exti_trigger(uint8_t exti);
int exti_status(uint8_t exti);


#endif
