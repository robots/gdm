#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/unistd.h>

/* sbrk stuff*/
extern unsigned int _end_ram; // end of ram
extern unsigned int _end_sram; // end of bss

static caddr_t heap = NULL;

// low level bulk memory allocator - used by malloc
caddr_t _sbrk ( int increment )
{
	caddr_t prevHeap;
	caddr_t nextHeap;

	if (heap == NULL) {
		// first allocation
		heap = (caddr_t)&_end_sram;
	}

	prevHeap = heap;

	// Always return data aligned on a 8 byte boundary
	nextHeap = (caddr_t)(((unsigned int)(heap + increment) + 7) & ~7);

	// get current stack pointer
	//register caddr_t stackPtr asm ("sp");

	// Check enough space and there is no collision with stack coming the other way
	// if stack is above start of heap
	if (nextHeap >= (caddr_t)&_end_ram) {
		return NULL; // error - no more memory
	} else {
		heap = nextHeap;
		return (caddr_t) prevHeap;
	}
}
