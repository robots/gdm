#ifndef EMUL_IO_h_
#define EMUL_IO_h_


void emul_io_init(void);
uint8_t emul_io_bus_read();
uint8_t emul_io_p1_read();
uint8_t emul_io_p2_read();
uint8_t emul_io_t_read(int t);
void emul_io_bus_write(uint8_t data);
void emul_io_p1_write(uint8_t data);
void emul_io_p2_write(uint8_t data);

#endif
