#ifndef LED_h_
#define LED_h_

enum {
	LED_1,
	LED_2,
};

#define LED_REPEAT       0x80

#define LED_OFF          (LED_REPEAT | 1)
#define LED_ON           (LED_REPEAT | 2)
#define LED_BLINK_FAST   (LED_REPEAT | 3)
#define LED_BLINK_SLOW   (LED_REPEAT | 4)
#define LED_3BLINK       (LED_REPEAT | 5)
#define LED_4BLINK       (LED_REPEAT | 6)

void led_init(void);
void led_set(uint8_t led, uint8_t state);
void led_toggle(uint8_t led);
void led_periodic(void);

#endif
