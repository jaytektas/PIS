
_IO_BASE = 0x4000

_SPISSR = 0x4000
SPISSR = 0x00

_IOCON = 0x4001
IOCON = 0x01

_CHPSEL = 0x4002
CHPSEL = 0x02

_DDR = 0x4003
DDR = 0x03

_PORT = 0x4004
PORT = 0x04

_CLOCK = 0x4005
CLOCK = 0x05

_MATCH = 0x4006
MATCH = 0x06

_SCCR2 = 0x4007
SCCR2 = 0x07
TMR 		= 0x01 			; timer interrupt enable
RWU			= 0x02			; reciever wakeup enable (puts to sleep)
RE			= 0x04			; recieve enable
TE			= 0x08			; transmit enable
ILIE		= 0x10			; idle interrupt enable
RIE			= 0x20			; recieve interrupt enable
TCIE		= 0x40			; transmit complete interrupt enable
TIE			= 0x80			; transmit interrupt enable

_SCSR = 0x4008
SCSR = 0x08
TMR			= 0x01			; timer match interrupt
FE			= 0x02			; SCI framing error
NF			= 0x04			; SCI noise flag
OR			= 0x08			; SCI overrun error
IDLE		= 0x10			; line idle detect
RDRF		= 0x20			; recieve data register full
TC			= 0x40			; transmit complete
TDRE		= 0x80			; transmit data register empty

_SCIRX = 0x4009
SCIRX = 0x09

_SCITX = 0x400a
SCITX = 0x0a

_COPLOCK = 0x400b
COPLOCK = 0x0b

_COPRESET = 0x400c
COPRESET = 0x0c

TABLE_ID		= 1
OFFSET_MSB		= 2
OFFSET_LSB		= 3
LENGTH_MSB		= 4
LENGTH_LSB		= 5
EXECUTE			= 6

	.globl _start, _interrupt_handler

	.section .data

; buffer space used for both sci rx / tx

sci_ptr:
	.word	0

sci_index:
	.byte	0

sci_buffer:
sci_command:
	.byte	0

sci_offset:
	.word	0

sci_length:
	.word	0

_STATUS:
tmrloop:
	.byte	0
seconds:
	.word	0
status_end:

	.section .text

_start:
    lds		#0x1ff           ; load stack pointer to end of ram

	ldaa	#0x8c
	staa	_IOCON

	ldaa	#0x8f
	staa	_DDR

	ldaa	#0x90
	staa	_PORT

	ldx		#0xff00
	stx		_COPLOCK

	ldaa	_CLOCK
	adda	#205
	staa	_MATCH

	ldaa	#0x2f		; SCI ready to recieve
	staa	_SCCR2

	cli

	clr		sci_index

loop:
	bra 	loop


_interrupt_handler:
	ldx		#_IO_BASE
	brclr	SCSR, x #TMR check_rx			; every 6.25ms

	ldaa	_CLOCK							; 32768khz clock
	adda	#205							; 6.25ms
	staa	_MATCH

	ldd		#0xff00
	std		_COPLOCK

	rti

check_rx:
	brclr	SCCR2, x #RIE	check_tx	  	; recieve interrupt enabled?
	brset	SCSR, x #RDRF 	rx_process		; something rx for us?

check_tx:
	brclr	SCCR2, x #TIE no_tx				; transmit interrupt enabled?
	brclr	SCSR, x #TDRE no_tx				; space in the tx que?

	jmp		tx_process

no_tx:
	rti

rx_process:
	ldaa	_SCIRX				; fetch the rx byte

	ldab	sci_index			; current rx buffer offset
	beq		rx_command			; 0? start of command?

	cmpb	#4	 				; more than 5 bytes recieved?
	bgt		rx_write			; go write value

	; save byte to buffer
	ldy		#sci_buffer
	aby
	staa	0, y

	; did we just save the 5th byte?
	cmpb	#4
	beq		rx_do_cmd

	; guess not
	inc		sci_index
	rti

rx_write:
	ldy		sci_ptr
	staa	0, y
	iny
	sty		sci_ptr
	ldy		sci_length
	dey
	sty		sci_length
	bne		rx_write_done
	clr		sci_index

rx_write_done:
	rti

rx_do_cmd:
	ldd		sci_length		; non zero length?
	bne		rx_length_ok
	clr		sci_index		; abort command
	rti

rx_length_ok:
	ldd		sci_offset
	addd	sci_ptr
	std		sci_ptr

	ldaa	sci_command
	cmpa	#'w'
	bne		tx_process

	inc		sci_index
	rti

; search for sci command
rx_command:
	cmpa	#'t'
	beq		tx_title

	cmpa	#'v'
	beq		tx_version

	cmpa	#'s'
	bne		w
	ldy		#_STATUS
	bra		rx_cmd_ok

w:
	cmpa	#'w'
	bne		r
	ldy		#_CONFIG
	bra		rx_cmd_ok

r:
	cmpa	#'r'
	bne		rx_error
	ldy		#_CONFIG
	bra		rx_cmd_ok

rx_error:
	rti

rx_cmd_ok:
	staa	sci_command
	sty		sci_ptr
	inc		sci_index
	rti

; output the title
tx_title:
	ldy		#_TITLE
	sty		sci_ptr

	ldy		#60
	sty		sci_length

	bra		tx_process

; output the version
tx_version:
	ldy		#_VERSION
	sty		sci_ptr

	ldy		#5
	sty		sci_length

tx_process:
	ldaa	#0x89								; transmit mode
	staa	SCCR2, x

	ldy		sci_length
	beq		tx_finished

	dey
	sty		sci_length

	ldy		sci_ptr
	ldaa	0, y
	iny
	sty		sci_ptr
	staa	SCITX, x
	rti

tx_finished:
	clr		sci_index
	ldaa	#0x27								; prepare to recieve again
	staa	SCCR2, x
	rti

_TITLE:
	.ascii "PIS Controller (C) 2009 Jason Roughley. All Rights Reserved."
_VERSION:
	.ascii "V1.00"

_CONFIG:
cylinders:
	.byte	'!'
stroke:
	.byte	'J'

	.section .vectors

	.word	0xffff
	.word	_interrupt_handler
	.word	0xffff
	.word	0xffff
	.word	0xffff
	.word	0xffff
	.word	0xffff
	.word	_start

    end
