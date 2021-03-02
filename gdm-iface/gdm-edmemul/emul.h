#ifndef EMUL_h_
#define EMUL_h_

void emul_init(void);
void emul_periodic(void);
void emul_terminate(int where);
UINT8 emul_rom_read(UINT16 address);
UINT8 emul_io_read(UINT8 address);
void emul_io_write(UINT8 address, UINT8 data);

#endif
