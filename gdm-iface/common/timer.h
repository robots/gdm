#ifndef TIMER_h_
#define TIMER_h_

enum {
	TIMER_TIM2,
	TIMER_TIM3,
	TIMER_TIM4,
};

typedef void (*timer_fnc_t)(void);

void timer_init(uint8_t timer);
void timer_intconf(uint8_t timer, int prio);
void timer_delay(uint8_t timer, uint32_t microsec);
void timer_set_handler(uint8_t timer, timer_fnc_t fnc);
void timer_timeout(uint8_t timer, uint32_t microsec);
void timer_abort(uint8_t timer);

#endif
