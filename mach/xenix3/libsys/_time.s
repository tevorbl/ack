.sect .text; .sect .rom; .sect .data; .sect .bss
.define __time
.sect .text
__time:
	mov	ax,13
	call	syscal
	mov	dx,bx
	ret
