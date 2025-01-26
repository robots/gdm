/*
 * 2014 Michal Demin
 *
 * Lock-free implementation of fifo.
 *
 * Write will never modify read ptr, and vice-versa.
 * If read/write ptr changes during write/read it will
 * create more space for write/read operation.
 *
 * Fifo will hold up to e_size-1 elements. (one is always wasted)
 *
 */
#include <string.h>

#define MIN(x,y)           ((x)>(y)?(y):(x))

#include "fifo.h"

/*
 * buffer size needs to be "e_size * e_num"
 */
void fifo_init(struct fifo_t *b, byte *buffer, unsigned long e_size, unsigned long e_num)
{
	b->write = 0;
	b->read = 0;
	b->e_num = e_num;
	b->e_size = e_size;
	b->buffer = buffer;
}

void fifo_reset(struct fifo_t *b)
{
	b->write = 0;
	b->read = 0;
}

unsigned long fifo_is_empty(struct fifo_t *b)
{
	if (FIFO_EMPTY(b)) {
		return 1;
	}

	return 0;
}

unsigned long fifo_get_read_count(struct fifo_t *b)
{
	unsigned long out;

	if (b->read > b->write) {
		out = b->read - b->write;
		out = b->e_num - out;
	} else {
		out = b->write - b->read;
	}

	return out;
}

unsigned long fifo_get_write_count(struct fifo_t *b)
{
	unsigned long out;

	out = fifo_get_read_count(b);
	out = b->e_num - out - 1;

	return out;
}


byte *fifo_get_read_addr(struct fifo_t *b)
{
	return (void *) ((unsigned long)b->buffer + b->read * b->e_size);
}

byte *fifo_get_write_addr(struct fifo_t *b)
{
	return (void *) ((unsigned long)b->buffer + b->write * b->e_size);
}

void fifo_read_done(struct fifo_t *b)
{
	// if there is something more to read -> advance
	if (!FIFO_EMPTY(b)) {
		b->read++;
		b->read %= b->e_num;
	}
}

void fifo_write_done(struct fifo_t *b)
{
	if (FIFO_FULL(b)) {
		return;
	}

	b->write ++;
	b->write %= b->e_num;

}

// returns length of continuous buffer
unsigned long fifo_get_read_count_cont(struct fifo_t *b)
{
	if (b->write < b->read) {
		return b->e_num - b->read;
	}
	
	if (b->write > b->read) {
		return b->write - b->read;
	}

	return 0;
}

void fifo_read_done_count(struct fifo_t *b, unsigned long size)
{
	b->read += size;
	b->read %= b->e_num;
}

// assumes that:
// - buf/cnt will fit into the fifo
// - noone else tries to write fifo
//
void fifo_write_buf(struct fifo_t *b, const void *buf, unsigned long cnt)
{
#if 0
	unsigned long i;
	void *ptr;

	for (i = 0; i < cnt; i++) {
		ptr = fifo_GetWriteAddr(b);

		if (b->e_size == 1) {
			(byte *ptr)[i] = (byte *buf)[i];
		} else if (b->e_size == 2) {
			(unsigned int *ptr)[i] = (unsigned int *buf)[i];
		} else if (b->e_size == 4) {
			(unsigned long *ptr)[i] = (unsigned long *buf)[i];
		} else {
			memcpy(ptr, buf+b->e_size*i, b->e_size);
		}

		fifo_WriteDone(b);
	}
#else
	if ((cnt > 0) && (b->write >= b->read)) {
		unsigned long c = MIN(cnt, b->e_num-b->write);

		memcpy((void *)((unsigned long)b->buffer + b->write * b->e_size), buf, c * b->e_size);

		b->write += c;
		b->write %= b->e_num;

		cnt -= c;
		buf = (void*)((unsigned long)buf + c * b->e_size);
	}

	if ((cnt > 0) && (b->read >= b->write)) {
		unsigned long c = MIN(cnt, b->read - b->write - 1);

		memcpy((void *)((unsigned long)b->buffer + b->write * b->e_size), buf, c * b->e_size);

		b->write += c;

		cnt -= c;
		buf = (void*)((unsigned long)buf + c * b->e_size);
	}
#endif
}
