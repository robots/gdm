#include "platform.h"
#include "delay.h"

// From http://forums.arm.com/index.php?showtopic=13949

volatile uint32_t *DWT_CYCCNT   = (volatile uint32_t *)0xE0001004;
volatile uint32_t *DWT_CONTROL  = (volatile uint32_t *)0xE0001000;
volatile uint32_t *SCB_DEMCR    = (volatile uint32_t *)0xE000EDFC;

uint32_t delay_us_tick;

void delay_init(void)
{
	static int enabled = 0;


	if (!enabled) {
		enabled = 1;

		*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
		*DWT_CYCCNT = 0; // reset the counter
		*DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter

		delay_us_tick = SystemFrequency / 1000000;
	}
}
 
void delay(uint32_t tick)
{
	uint32_t start, current;

	start = *DWT_CYCCNT;

	do {
		current = *DWT_CYCCNT;
	} while ((current - start) < tick);
}

void delay_us(uint32_t us)
{
	delay(us * delay_us_tick); 
}

void delay_ms(uint32_t ms)
{
	delay_us(ms*1000);
}
