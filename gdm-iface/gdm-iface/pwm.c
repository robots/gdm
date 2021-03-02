#include "platform.h"
#include "gpio.h"

#include "pwm.h"

const struct gpio_init_table_t pwm_gpio[] = {
	{
		.gpio = GPIOA,
		.pin = 0x0f, // pa0-3
		.mode = GPIO_MODE_AF_PP,
		.speed = GPIO_SPEED_MED,
	},
};

// 72000000Hz/2 / 36 / 20000 = 50 Hz pwm
// 1ms - 2ms => 1000 - 2000
void pwm_init(void)
{
	gpio_init(pwm_gpio, ARRAY_SIZE(pwm_gpio));

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	TIM2->PSC = 36-1;
	TIM2->ARR = 20000-1;
	TIM2->EGR = TIM_EGR_UG;

	// init cc outptu
	TIM2->CCMR1 = 0x60 | TIM_CCMR1_OC1PE | (0x60 << 8) | TIM_CCMR1_OC2PE;
	TIM2->CCMR2 = 0x60 | TIM_CCMR2_OC3PE | (0x60 << 8) | TIM_CCMR2_OC4PE;
	TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	TIM2->CCR1 = 1000;
	TIM2->CCR2 = 1000;
	TIM2->CCR3 = 1000;
	TIM2->CCR4 = 1000;

	TIM2->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void pwm_set(uint8_t ch, uint16_t pwm)
{
	switch (ch) {
		case 1:
			TIM2->CCR1 = pwm;
			break;
		case 2:
			TIM2->CCR2 = pwm;
			break;
		case 3:
			TIM2->CCR3 = pwm;
			break;
		case 4:
			TIM2->CCR4 = pwm;
			break;
		default:
		break;
	}
}
