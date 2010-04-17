/*
	Copyright 2010 Jason Roughley

	This file is part of PIS firmware.

    PIS firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIS firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIS firmware.  If not, see <http://www.gnu.org/licenses/>.
*/

	.section .softregs

_.frame:
	.word	0
_.z:
	.word	0
_.xy:
	.word	0
_.tmp:
	.word 	0
_.d1:
	.word	0
_.d2:
	.word	0
_.d3:
	.word	0
_.d4:
	.word	0

	.globl	_.z, _.xy, _.frame, _.tmp, _.d1, _.d2, _.d3, _.d4, sci_index
	.globl	_stack

	.globl	_start, main, _stack

	.section .text

_start:
	lds		#_stack					; load stack pointer

	ldx		#__data_load_start
	ldy		#__data_start

$load_map:
	ldaa	0, x
	staa	0, y
	inx
	iny

	cpx		#__data_load_end
	bne		$load_map


	ldd		#__bss_size
	beq		$done

	ldx		#__bss_start

$bss_init:
	clr		0,x
	inx
	subd	#1
	bne		$bss_init
$done:

	ldd		#0
	ldx		#0x3FC0

$init_fmd:
	std		0, x
	inx
	inx
	cpx		#0x3FFA
	bne		$init_fmd

	ldaa	#0x8c
	staa	IOCON

	ldaa	#0x8f
	staa	DDR

	ldaa	#0x90
	staa	PPORT

	ldx		#0xff00
	stx		COPLCK

	ldaa	CLOCK
	adda	#133
	staa	MATCH

$loop:
	jsr main
	bra $loop
