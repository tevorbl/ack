.define __times
.extern __times
.sect .text
.sect .rom
.sect .data
.sect .bss
.sect .text
__times:		move.w #0x2B,d0
		move.l 4(sp),a0
		trap #0
		rts
