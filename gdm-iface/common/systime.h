#ifndef SYSTIME_h_
#define SYSTIME_h_

#define SYSTIME_PERIOD_MS    10
#define SYSTIME_SEC(x)       ((uint32_t)((x)*1000))

#define SYSTIME_TO_SEC(x)    ((uint32_t)((x)/1000))

typedef void (*timer_fnc)(void);

void systime_init(void);
void systime_periodic(void);
void systime_add(timer_fnc fnc, uint32_t period);
void systime_add_oneshot(timer_fnc fnc, uint32_t period);
void systime_remove(timer_fnc fnc);
void systime_delay(uint32_t delay);
uint32_t systime_get(void);

#endif
