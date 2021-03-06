/*
	AB_list[s] holds the actual base of stack frame  s; this
	is the highest stack pointer of frame  s-1.
	Segments have the following numbers:
		-2			DATA_SEGMENT
		-1			HEAP_SEGMENT
		0, 1, .., curr_frame	stackframes
	Note that  AB_list[s] increases for decreasing s.
*/

/* $Id$ */

#include	"segcheck.h"
#include	"segment.h"
#include	"global.h"
#include	"mem.h"
#include	"alloc.h"

#ifdef	SEGCHECK

#define	ABLISTSIZE	100L		/* initial AB_list size */

#define	DATA_SEGMENT	-2
#define	HEAP_SEGMENT	-1

PRIVATE ptr *AB_list;
PRIVATE size frame_limit;
PRIVATE size curr_frame;

/** Allocate space for AB_list & initialize frame variables */
void init_AB_list(void)
{
	frame_limit = ABLISTSIZE;
	curr_frame = 0L;
	AB_list = (ptr *) Malloc(frame_limit * sizeof (ptr), "AB_list");
	AB_list[curr_frame] = AB;
}

void push_frame(ptr p)
{
	if (++curr_frame == frame_limit) {
		frame_limit = allocfrac(frame_limit);
		AB_list = (ptr *) Realloc((char *) AB_list,
				frame_limit * sizeof (ptr), "AB_list");
	}
	AB_list[curr_frame] = p;
}

void pop_frames(void)
{
	while (AB_list[curr_frame] < AB) {
		curr_frame--;
	}
}

int ptr2seg(ptr p)
{
	register int s;

	if (in_gda(p)) {
		s = DATA_SEGMENT;
	}
	else if (!in_stack(p)) {
		s = HEAP_SEGMENT;
	}
	else {
		for (s = curr_frame; s > 0; s--) {
			if (AB_list[s] > p)
				break;
		}
	}
	return s;
}

#else	/* SEGCHECK */

void init_AB_list(void) {}

void push_frame(ptr) {}

void pop_frames(void) {}

#endif	/* SEGCHECK */

