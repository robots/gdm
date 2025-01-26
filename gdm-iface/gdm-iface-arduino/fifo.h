/*
 * 2010-2014 Michal Demin
 *
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <Arduino.h>

#define FIFO_EMPTY(x) ((((struct fifo_t *)x)->write == (x)->read))
#define FIFO_FULL(x)  (((((struct fifo_t *)x)->write + 1) % (x)->e_num) == (x)->read)

struct fifo_t {
	unsigned long read;   // read pointer
	unsigned long write;  // write pointer
	byte e_num;  // number of elements
	byte e_size; // element size
	byte *buffer;
};

void fifo_init(struct fifo_t *b, byte *buffer, unsigned long e_size, unsigned long e_num);
void fifo_reset(struct fifo_t *);

unsigned long fifo_is_empty(struct fifo_t *b);
unsigned long fifo_get_read_count(struct fifo_t *b);
unsigned long fifo_get_write_count(struct fifo_t *b);

byte *fifo_get_read_addr(struct fifo_t *b);
void fifo_read_done(struct fifo_t *b);

byte *fifo_get_write_addr(struct fifo_t *b);
void fifo_write_done(struct fifo_t *b);

unsigned long fifo_get_read_count_cont(struct fifo_t *b);
void fifo_read_done_count(struct fifo_t *b, unsigned long size);

void fifo_write_buf(struct fifo_t *b, const void *buf, unsigned long cnt);

#endif
