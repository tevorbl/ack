.define __brk
.define __sbrk
.extern __brk
.extern __sbrk
.sect .text
.sect .rom
.sect .data
.sect .bss
.sect .text
__sbrk:		move.l nd,a0
		add.l  4(sp),a0
		move.w #0x11,d0
		trap #0
		bcs lcerror
		move.l nd,d0
		move.l d0,a0
		add.l  4(sp),a0
		move.l a0,nd
		rts
lcerror:	jmp cerror
__brk:		move.w #0x11,d0
		move.l 4(sp),a0
		trap #0
		bcs lcerror
		move.l 4(sp),nd
		clr.l d0
		rts
.sect .data
nd:
	.data4	endbss
.sect .text
