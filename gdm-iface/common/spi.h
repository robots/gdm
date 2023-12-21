/*
 * SPI Slave driver for STM32 family processors
 *
 * 2010 Michal Demin
 *
 */

#ifndef SPI_H_
#define SPI_H_

void spi_init(void);
void spi_deinit(void);

uint8_t spi_is_done();
uint8_t spi_is_locked(void);
uint8_t spi_lock(uint8_t cs);
uint8_t spi_unlock(void);
void spi_cs_on(void);
void spi_cs_off(void);
uint8_t spi_wait_miso(uint8_t state);
uint8_t spi_send_slow(const uint8_t *outbuf, uint8_t *inbuf, uint32_t size);
uint8_t spi_send(const uint8_t *outbuf, uint8_t *inbuf, uint32_t size);
void spi_wait_done();
void spi_send_blocking(uint8_t *outbuf, uint8_t *inbuf, uint32_t size);

#endif

