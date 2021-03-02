
#include "platform.h"

#include "gpio.h"
#include "bsp_led.h"
#include "led.h"
#include "systime.h"

#undef DEBUG

#ifdef DEBUG
#include <stdlib.h>
#include "console.h"
static struct console_command_t led_cmd;
#endif

const struct {
	uint8_t num;
	uint32_t time[10];
} led_timing[] = {
	{0, {}},
	{2, {0, 50}},
	{2, {50, 0}},
	{2, {50, 50}},
	{2, {200, 200}},
	{4, {200, 200, 200, 1000}},
	{6, {200, 200, 200, 200, 200, 1000}},
};

uint32_t led_timer[2];
uint8_t led_state[2];
uint8_t led_mode[2];
uint8_t led_nextmode[2];

void led_init(void)
{
	gpio_init(led_gpio, led_gpio_cnt); 

	for (uint32_t i = 0; i < led_gpio_cnt; i++) {
		led_mode[i] = 0;
		led_nextmode[i] = 0xff;
	}

#ifdef DEBUG
	console_add_command(&led_cmd);
	console_printf(CON_INFO, "Led: menu added\r\n");
#endif
}

void led_set(uint8_t led, uint8_t mode)
{
	if (led >= led_gpio_cnt)
		return;

	if ((mode & 0x7f) >= ARRAY_SIZE(led_timing))
		return;

	led_nextmode[led] = mode;
}

void led_toggle(uint8_t led)
{
	uint8_t state;

	if (led >= led_gpio_cnt)
		return;

	// into manual mode
	led_mode[led] = 0;

	led_state[led] = !led_state[led];

	state = led_state[led] ^ led_pol[led];

	gpio_set(&led_gpio[led], state ? GPIO_RESET : GPIO_SET);
}

void led_periodic()
{
	static uint32_t time_last = 0;
	uint8_t mode;
	uint32_t state;

	uint32_t diff = systime_get() - time_last;
	time_last = systime_get();

	for (uint8_t i = 0; i < led_gpio_cnt; i++) {
		mode = led_mode[i] & 0x7f;

		if (mode == 0) {
			if (led_nextmode[i] != 0xff) {
				led_mode[i] = led_nextmode[i];
				led_nextmode[i] = 0xff;
				led_state[i] = 0;
				led_timer[i] = 0;
			}
		} else if (led_timer[i] == 0) {

			// is end of pattern?
			if (led_state[i] >= led_timing[mode].num) {
				led_state[i] = 0;
				if (led_mode[i] & 0x80) {
					if (led_nextmode[i] != 0xff) {
						led_mode[i] = led_nextmode[i];
						led_nextmode[i] = 0xff;
						led_timer[i] = 0;
						continue;
					}
				} else {
					mode = led_mode[i] = 0;
				}
			}

			led_timer[i] = led_timing[mode].time[led_state[i]];
			state = (led_state[i] % 0x02) ^ led_pol[i];

			gpio_set(&led_gpio[i], state ? GPIO_RESET : GPIO_SET);

			led_state[i]++;
		} else {
			if (led_timer[i] >= diff) {
				led_timer[i] -= diff;
			} else {
				led_timer[i] = 0;
			}
		}
	}
}

#ifdef DEBUG
static uint8_t led_cmd_handler(struct console_session_t *cs, char **args)
{
	char out[CONSOLE_CMD_OUTBUF];
	uint8_t ret = 1;
	uint32_t len;

	if (args[0] == NULL) {
		len = tfp_sprintf(out, "Valid: toggle {led_num}, set {led_num} {mode_num}\r\n");
		cs->output(cs, out, len);
		return 1;
	}

	if (strcmp(args[0], "toggle") == 0) {
		led_toggle(strtoll(args[1], 0, 0));
	} else if (strcmp(args[0], "set") == 0) {
		led_set(strtoll(args[1], 0, 0), strtoll(args[2], 0, 0));
	}

	return ret;
}

static struct console_command_t led_cmd = {
	"led",
	led_cmd_handler,
	"Led test menu",
	"Led menu help",
	NULL
};
#endif
