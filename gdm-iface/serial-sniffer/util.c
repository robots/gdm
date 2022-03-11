#include "platform.h"

#include "util.h"

static uint32_t dtab[] = {
	1000000000, //0
	100000000, //1
	10000000, //2
	1000000, //3
	100000, //4
	10000, //5
	// inset dot here
	1000, //6
	100, //7
	10, //8
	1, //9
};

uint32_t ts_to_str(uint32_t v, char *out)
{
	char *s = out;
	uint32_t i = 0;
	uint32_t d;

	if (v == 0) {
		// trivial case
		*out = '0';
		out ++;
	} else {
		//
		// skip initial zero
		while (i < 6 && v < dtab[i]) i++;

		// print digits
		while (1) {
			d = v / dtab[i];
			v = v % dtab[i];

			*out = '0' + d;
			out++;

			if (i == 6) {
				*out = '.';
				out++;
			}

			i++;
			if (i >= ARRAY_SIZE(dtab))
				break;
		}
	}

	*out = 0;

	return out-s;
}
