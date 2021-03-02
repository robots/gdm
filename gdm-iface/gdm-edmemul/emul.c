#include "platform.h"

#include "gpio.h"

#include "i8039.h"
#include "emul.h"
#include "emul_io.h"

#include "memory_code.c"

void emul_init(void)
{
	emul_io_init();
	i8039_reset(NULL);
}

void emul_periodic(void)
{
	i8039_execute(30, 0);
}

void emul_terminate(int where)
{
	(void)where;
	while(1);
}

// interface for i8039 cpu emul
UINT8 emul_rom_read(UINT16 address)
{
	if (address >= codemem_length) {
		emul_terminate(1);
	}

	return codemem[address];
}

UINT8 emul_io_read(UINT8 address)
{
	UINT8 data;

	switch (0x100 | address) {
		case I8039_p1:
			data = emul_io_p1_read();
			break;

		case I8039_p2:
			data = emul_io_p2_read();
			break;

		case I8039_t0:
			data = emul_io_t_read(0);
			break;

		case I8039_t1:
			data = emul_io_t_read(1);
			break;

		case I8039_t1+1:
			data = emul_io_t_read(2);
			break;

		case I8039_bus:
			data = emul_io_bus_read();
			break;

		default:
			emul_terminate(4);
			break;
	}

	return data;
}

void emul_io_write(UINT8 address, UINT8 data)
{
	switch (0x100 | address) {
		case I8039_p1:
			emul_io_p1_write(data);
			break;

		case I8039_p2:
			emul_io_p2_write(data);
			break;

		case I8039_bus:
			emul_io_bus_write(data);
			break;

		default:
			emul_terminate(5);
			break;
	}
}
