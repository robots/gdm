#include "platform.h"

#include "config.h"
#include "systime.h"

struct {
	uint8_t active:1;
	uint8_t oneshot:1;

	uint32_t time_last;
	uint32_t period;

	timer_fnc fnc;

} systime_timer[SYSTIME_TIMERS];

volatile uint32_t systime_localtime = 0;

void systime_init()
{
	uint32_t i;

	systime_localtime = 0;

	for (i = 0; i < SYSTIME_TIMERS; i++) {
		systime_timer[i].active = 0;
	}

	SysTick_Config(SystemFrequency / 100);

	/* Update the SysTick IRQ priority should be higher than the Ethernet IRQ */
	/* The Localtime should be updated during the Ethernet packets processing */
	NVIC_SetPriority (SysTick_IRQn, 2);
}

void systime_periodic(void)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(systime_timer); i++) {
		if (systime_timer[i].active == 0)
			continue;

		if (systime_localtime - systime_timer[i].time_last > systime_timer[i].period) {
			if (systime_timer[i].fnc) {
				systime_timer[i].fnc();
			}

			if (systime_timer[i].oneshot) {
				systime_timer[i].active = 0;
			}

			systime_timer[i].time_last = systime_localtime;
		}
	}
}

static void systime_add_(timer_fnc fnc, uint32_t period, uint8_t oneshot)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(systime_timer); i++) {
		if (systime_timer[i].active == 0)
			break;
	}

	if (i == SYSTIME_TIMERS)
		return;

	systime_timer[i].fnc = fnc;
	systime_timer[i].period = period;
	systime_timer[i].time_last = systime_localtime;

	systime_timer[i].oneshot = !!oneshot;
	systime_timer[i].active = 1;

}

void systime_add(timer_fnc fnc, uint32_t period)
{
	systime_add_(fnc, period, 0);
}

void systime_add_oneshot(timer_fnc fnc, uint32_t period)
{
	systime_add_(fnc, period, 1);
}

void systime_remove(timer_fnc fnc)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(systime_timer); i++) {
		if (systime_timer[i].active == 1) {
			if (systime_timer[i].fnc == fnc) {
				systime_timer[i].active = 0;
			}
		}
	}
}

void systime_delay(uint32_t delay)
{
	uint32_t time;

	time = systime_localtime + delay;

	while(time > systime_localtime);
}

uint32_t systime_get(void)
{
	return systime_localtime;
}

void SysTick_Handler(void)
{
	systime_localtime += SYSTIME_PERIOD_MS;
}
