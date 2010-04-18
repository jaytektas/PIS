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

	.section .text

	.globl magic_rpm, spi_io, adc_read, mult_u32, interpolate_u16, div_u16
	.globl scale_u16, set_ignition, iac_increase, iac_decrease, bus_delay
    .globl illegal

; emulate the missing register exchange op codes!

illegal:
	tsx
	ldy		7, x		; PC that caused illegal instruction
	ldaa	0, y		; opcode

	; xgdx ?
	cmpa	#0x8f
	beq		do_xgdx

	; xgdy?
	cmpa	#0x18		; xgdy
	bne		dead
	ldaa	1, y
	cmpa	#0x8f
	beq		do_xgdy

dead:
	bra	dead

do_xgdx:
	iny
	sty		7, x
	; swap x and d around on stack
	ldaa	2, x
	ldab	3, x
	staa	3, x
	stab	2, x
	ldaa	1, x
	ldab	4, x
	staa	4, x
	stab	1, x
	rti

do_xgdy:
	; update returning PC to skip instruction
	iny
	iny
	sty		7, x
	; swap x and d around on stack
	ldaa	2, x
	ldab	5, x
	staa	5, x
	stab	2, x
	ldaa	1, x
	ldab	6, x
	staa	6, x
	stab	1, x
	rti

; USHORT magic_rpm(USHORT magic_number);

magic_rpm:
	ldx		0x3fc0
	fdiv
	xgdx
	lsrd
	lsrd
	rts

; UBYTE spi_io(UBYTE tx);

spi_io:
	ldx		#IO_BASE
	stab	0, x
	bclr	1, x #0x80				; go

	ldaa	#22
	clc

spi_wait:
	brset	1, x #0x80 spi_read
	deca
	bne		spi_wait
	sec

spi_read:
	ldaa	0, x
    tab
	rts

; UBYTE adc_read(UBYTE channel);
adc_read:
	tba
	ldx		#IO_BASE
	sei
	bclr	2, x #0x08			; select ADC
	jsr		spi_io

	ldaa	#7
delay_loop:
	deca
	bne		delay_loop

	ldaa	#0xb0				; test chan
	jsr		spi_io
	bset	2, x #0x08			; deselect ADC
	cli
	tab							; result
	rts


; ULONG mult_u32(USHORT value, USHORT multiplier);

prod3	=		0
prod2	=		1
prod1	=		2
prod0	=		3

mpr1	=		4
mpr0	=		5

mpd1	=		8
mpd0	=		9

mult_u32:
	tsy					; save stack to y for later restore

	pshb				; D operand 16 bit to stack
	psha

	ldx		#0			; reserve and clear 32 bit product
	pshx
	pshx

	tsx					; current stack pointer

	ldaa     mpr0, x
	ldab     mpd0, x
	mul
	std     prod1, x

	ldaa     mpr0, x
	ldab     mpd1, x
	mul
	addd    prod2, x
	std     prod2, x

	ldaa     mpr1, x
	ldab     mpd0, x
	mul
	addd    prod2, x
	std     prod2, x

	rol     prod3, x
	ldaa     mpr1, x
	ldab     mpd1, x
	mul
	addd    prod3, x
	std     prod3, x

	pulx
	pula
	pulb

	tys		; restore stack
	rts


; USHORT interpolate_u16(USHORT index, USHORT slope1a, USHORT slope1b, USHORT slope2a, USHORT slope2b);
; returns 16 bit interpolated value from slope2 relative to index into slope 1
; the slopes can be in any direction(s)
; ~300 tics only!

s2_range = 0
multu32 = 2
neg_flag_u8 = 6
index_u16 = 7
ret_16 = 9
s1a_u16 = 11
s1b_u16 = 13
s2a_u16 = 15
s2b_u16 = 17

interpolate_u16:
	; save index on stack
	pshb
	psha

	clra
	psha						; neg_flag

	ldx		#0
	pshx						; mult32
	pshx
	pshx						; s2_range

	tsy							; stack indexing register

	; correct slope 1 as positive
	ldd		s1b_u16, y				; s1b -> D
	subd	s1a_u16, y
	beq		slope_too_small			; no slope
	bcc		s1_pos					; s1 positive already

	; swap s1a / s1b
	bset	neg_flag_u8, y #1		; flag
	ldd		s1a_u16, y
	ldx		s1b_u16, y
	stx		s1a_u16, y
	std		s1b_u16, y

	subd	s1a_u16, y

s1_pos:
	xgdx

	; correct index
	ldd		index_u16, y
	cpd		s1a_u16, y
	bls		index_too_small

	cpd		s1b_u16, y
	bhs		index_too_large

	subd	s1a_u16, y

	fdiv
	; if negative slope then invert the result
	brclr	neg_flag_u8, y #1 s1_pos_2
	xgdx
	eora 	#0xff
	eorb 	#0xff
	xgdx

s1_pos_2:
	stx		index_u16, y	; index now a % of slope 1
	; multiply our fraction by slope 2 then shift >> 16

	; find s2 range and direction
	ldd		s2b_u16, y				; s1b -> D
	subd	s2a_u16, y
	beq		slope_too_small			; no slope
	bcc		s2_pos					; s1 positive already

	; swap s2a / s2b
	bset	neg_flag_u8, y #2		; flag
	ldd		s2a_u16, y
	subd	s2b_u16, y

s2_pos:
	std		s2_range, y

	; index * s2_range
	ldaa    s2_range+1, y
	ldab    index_u16+1, y
	mul
	std		multu32+2, y

	ldaa    s2_range+1, y
	ldab    index_u16, y
	mul
	addd    multu32+1, y
	std     multu32+1, y

	ldaa    s2_range, y
	ldab    index_u16+1, y
	mul
	addd    multu32+1, y
	std     multu32+1, y

	rol     multu32, y
	ldaa    s2_range, y
	ldab    index_u16, y
	mul
	addd    multu32, y
	std     multu32, y

	brclr	neg_flag_u8, y #2 s2_pos_2
	ldd		s2a_u16, y
	subd	multu32, y
	bra		exit

s2_pos_2:
	addd	s2a_u16, y

exit:
	pulx
	pulx
	pulx
	pulx
	ins
	rts

slope_too_small:
index_too_small:
	ldd		s2a_u16, y
	bra		exit

index_too_large:
	ldd		s2b_u16, y
	bra		exit



; USHORT div_u16(ULONG dividend, USHORT divisor)
; divides 32 bit / 16 bit returning 16 bit quotient
; slow ~1000 tics

quotient = 0
dividend = 2
return = 6
divisor = 8

div_u16:
; divisor
	pshb
	psha
	pshx

; quotient
	clra
	psha
	psha

	tsx
	ldy		#17

	ldd		dividend, x
	cpd		divisor, x
	bcs		overflow

q_test:
	clc
	rol		quotient+1, x
	rol		quotient, x

	; rotate left dividend
	rol		dividend+3, x
	rol		dividend+2, x
	rol		dividend+1, x
	rol		dividend, x

	ldd		dividend, x
	cpd		divisor, x
	bcs		wont_fit
	bset	quotient+1, x #1

	subd	divisor, x
	std		dividend, x

wont_fit:

	dey
	bne		q_test

	ldd		quotient, x

exit2:
	pulx
	pulx
	pulx
	rts

overflow:
	ldd		#0xFFFF
	bra		exit2


product = 0
multiplicand = 4
return = 6
percent_000_00_x_327_68 = 8

; USHORT scale_u16(USHORT multiplicand, USHORT percent_000_00_x_327_68);
; 16 x 16 multiply
; divide by 32768

scale_u16:
	; multiplicand
	pshb
	psha

	; product
	ldx		#0
	pshx
	pshx

	tsx

	clc

	ldaa	percent_000_00_x_327_68 + 1, x
	ldab	multiplicand + 1, x
	mul
	std		product + 2, x

	ldaa	percent_000_00_x_327_68 + 1, x
	ldab	multiplicand, x
	mul
	addd	product + 1, x
	std		product + 1, x

	ldaa	percent_000_00_x_327_68, x
	ldab	multiplicand + 1, x
	mul
	addd	product + 1, x
	std		product + 1, x

	rol		product, x						; any carry
	ldaa	percent_000_00_x_327_68, x
	ldab	multiplicand, x
	mul
	addd	product, x
	std		product, x

	; divide by 32
	clc
	rol		product + 2, x
	rol		product + 1, x
	rol		product, x
	ldd		product, x

	pulx
	pulx
	pulx
	rts

; void set_spark(USHORT dwell, USHORT advance)

ESTRCT = 0x3fe6 ; [Timer] Rising Charge Time - (ESTFIRE(current)-ESTFIRE(last)) + (DWELL(last)-DWELL(current))
ESTFCT = 0x3fe8 ; [Timer] Falling Charge Time change - ESTFIRE(CURRENT)-ESTFIRE(LAST),1/65/5KHZ CTS
ESTFIRE = 0x3ff6 ; [Timer] Time From Ref Pulse To Fire(Neg. For Advance)
DWELL = 0x3fdc ; [Pulse Out] SPK Dwell period cnt'r

advance = 0
dwell = 2

set_ignition:
	pshb
	psha

    pshx
    pulb
    pula
    eora    #0xff
    eorb    #0xff
    psha
    pshb

	tsx
	sei

	ldd		advance, x
	subd	ESTFIRE			;0x3ff6
	bsr		bus_delay

	std		ESTFCT			;0x3fe8
	bsr		bus_delay

	addd 	DWELL			;0x3fdc
	bsr		bus_delay

	subd	dwell, x
	std		ESTRCT			;0x3fe6
	bsr		bus_delay

	ldd		dwell, x
	std		DWELL			;0x3fdc
	bsr		bus_delay

	ldd		advance, x
	std		ESTFIRE			;0x3ff6

	cli

	pulx
	pulx

bus_delay:
	rts

	.section	.data
iac:
	.byte	204

	.section	.text

; void iac_increase();
iac_increase:
	ldaa	iac
	lsla
	bcc		bit_skip1
	adda	#1

bit_skip1:
	staa	iac
	anda	#3
	ldab	CHPSEL
	andb	#0xfc
	aba
	staa	CHPSEL
	rts

; void iac_decrease();
iac_decrease:
	ldaa	iac
	lsra
	bcc		bit_skip
	adda	#0x80

bit_skip:
	staa	iac
	anda	#3
	ldab	CHPSEL
	andb	#0xfc
	aba
	staa	CHPSEL
	rts

; USHORT mult_u16(UBYTE multiplicand, UBYTE multiplier)

	.globl	mult_u16

multiplier = 3

mult_u16:
	tsx
	ldaa	multiplier, x
	mul
	rts

; SHORT interpolate_u16(USHORT index, USHORT slope1a, USHORT slope1b, SHORT slope2a, SHORT slope2b);
; returns signed 16 bit interpolated value from slope2 relative to index into slope 1
; the slopes can be in any direction(s) but only the second slope is signed!
; ~300 tics only!

$s2_range = 0
$mult_u32 = 2
$neg_flag_u8 = 6
$index_u16 = 7
$ret_16 = 9
$s1a_u16 = 11
$s1b_u16 = 13
$s2a_s16 = 15
$s2b_s16 = 17

	.globl	interpolate_s16

interpolate_s16:
	; save index on stack
	pshb
	psha

	clra
	psha						; neg_flag

	ldx		#0
	pshx						; mult32
	pshx
	pshx						; s2_range

	tsy							; stack indexing register


	ldaa	#0x80
	eora	$s2a_s16, y
	staa	$s2a_s16, y

	ldaa	#0x80
	eora	$s2b_s16, y
	staa	$s2b_s16, y

	; correct slope 1 as positive
	ldd		$s1b_u16, y				; s1b -> D
	subd	$s1a_u16, y
	beq		$slope_too_small			; no slope
	bcc		$s1_pos					; s1 positive already

	; swap s1a / s1b
	bset	$neg_flag_u8, y #1		; flag
	ldd		$s1a_u16, y
	ldx		$s1b_u16, y
	stx		$s1a_u16, y
	std		$s1b_u16, y

	subd	$s1a_u16, y

$s1_pos:
	xgdx

	; correct index
	ldd		$index_u16, y
	cpd		$s1a_u16, y
	bls		$index_too_small

	cpd		$s1b_u16, y
	bhs		$index_too_large

	subd	$s1a_u16, y

	fdiv
	; if negative slope then invert the result
	brclr	$neg_flag_u8, y #1 $s1_pos_2
	xgdx
	eora 	#0xff
	eorb 	#0xff
	xgdx

$s1_pos_2:
	stx		$index_u16, y	; index now a % of slope 1
	; multiply our fraction by slope 2 then shift >> 16

	; find s2 range and direction
	ldd		$s2b_s16, y				; s1b -> D
	subd	$s2a_s16, y
	beq		$slope_too_small			; no slope
	bcc		$s2_pos					; s1 positive already

	; swap s2a / s2b
	bset	$neg_flag_u8, y #2		; flag
	ldd		$s2a_s16, y
	subd	$s2b_s16, y

$s2_pos:
	std		$s2_range, y

	; index * s2_range
	ldaa    $s2_range + 1, y
	ldab    $index_u16 + 1, y
	mul
	std		$mult_u32+2, y

	ldaa    $s2_range + 1, y
	ldab    $index_u16, y
	mul
	addd    $mult_u32 + 1, y
	std     $mult_u32 + 1, y

	ldaa    $s2_range, y
	ldab    $index_u16 + 1, y
	mul
	addd    $mult_u32+1, y
	std     $mult_u32+1, y

	rol     $mult_u32, y
	ldaa    $s2_range, y
	ldab    $index_u16, y
	mul
	addd    $mult_u32, y
	std     $mult_u32, y

	brclr	$neg_flag_u8, y #2 $s2_pos_2
	ldd		$s2a_s16, y
	subd	$mult_u32, y
	bra		$exit

$s2_pos_2:
	addd	$s2a_s16, y

$exit:
	pulx
	pulx
	pulx
	pulx
	ins
	psha
	tsx
	ldaa	#0x80
	eora	0, x
	ins
	rts

$slope_too_small:
$index_too_small:
	ldd		$s2a_s16, y
	bra		$exit

$index_too_large:
	ldd		$s2b_s16, y
	bra		$exit


; UBYTE inj_comp(UBYTE voltage, UBYTE *table)

	.globl open_delay

open_delay:
    lsrb
	andb    #0x7f
    tba

	; table index
    andb    #0x70
    lsrb
    lsrb
    lsrb
    lsrb

	; index
	abx

	ldab	1, x
	cmpb	0, x
	blo		negative

	subb	0, x

	; span
	anda	#0x0F
	mul

	rora
	rorb
    lsrb
    lsrb
    lsrb

	ldaa	0, x
    aba
    tab

	rts

negative:
	ldab	0, x
	subb	1, x
	anda	#0x0F
	mul

	rora
	rorb
    lsrb
    lsrb
    lsrb

	ldaa	0, x
    sba
    tab

	rts
