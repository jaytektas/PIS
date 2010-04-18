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

#ifndef _REGISTERS_H_
#define _REGISTERS_H_
#include "types.h"

/******************************
 IO REGISTERS 0x4000 - 0x400C
******************************/

// 0x4000
extern volatile unsigned char SPISR;

// 0x4001
extern volatile unsigned char IOCON;
typedef struct tagIOCON
{
	unsigned SCIPS				:2;		// sci prescaler 8|16|32|64
	unsigned SPIPS				:2;		// spi prescaler 2|4|16|32
	unsigned				:2;
	unsigned SI				:1;
	unsigned SPIGODONE			:1;
} IOCONBITS;
extern volatile IOCONBITS IOCONbits;

// 0x4002
extern unsigned char CHPSEL;
typedef struct tagCHPSEL
{
	unsigned FAULT				:3;
	unsigned INJA				:1;
	unsigned ADCCS				:1;
	unsigned IICS				:1;
	unsigned IACB				:1;
	unsigned IACA				:1;
} CHPSELBITS;
extern CHPSELBITS CHPSELbits;

// (7) FAULT
// (6) FAULT
// (5) FAULT
// (4) INJA
// (3) ADCCS
// (2) IICS
// (1) IACB
// (0) IACA

// 0x4003
extern unsigned char DDR;

// 0x4004
extern unsigned char PPORT;
typedef struct tagPPORT
{
	unsigned 				:1;
	unsigned MATCH_ENABLE			:1;
	unsigned SCI_BAUD			:2;
	unsigned SXR_ENABLE			:1;
	unsigned IAC_ENABLE			:1;
	unsigned FAN_ENABLE			:1;
	unsigned				:1;
} PPORTBITS;
extern PPORTBITS PPORTbits;

// (7)
// (6) MATCH_ENABLE						// enable clock match (when CLOCK == MATCH) interrupt
// (5) SCI_BAUD							// SCI communications rates 00 = 16384, 01 = 8192, 10 = 1024, 11 = 256
// (4) SCI_BAUD
// (3) SXR_ENABLE						// PORT43 SXR chip enable line
// (2) IAC_ENABLE						// PORT42 IAC chip enable line
// (1) FAN_ENABLE						// PORT41 high speed cooling fan
// (0)

// 0x4005
extern volatile unsigned char CLOCK;
// 0x4006
extern volatile unsigned char MATCH;

// 0x4007
extern volatile unsigned char SCCR2;
typedef struct tagSCCR2
{
	unsigned TIE				:1;			// Transmit Interrupt Enable
	unsigned TCIE				:1;			// Transmit Complete Interrupt Enable
	unsigned RIE				:1;			// Receive Interrupt Enable
	unsigned ILIE				:1;			// Idle-Line Interrupt Enable
	unsigned TE				:1;			// Transmit Enable
	unsigned RE				:1;			// Recieve Enable
	unsigned RWU				:1;			// Receiver Wakup
	unsigned MATCHIE			:1;			// Send Break??
} SCCR2BITS;
extern volatile SCCR2BITS SCCR2bits;

// 0x4008
extern volatile UBYTE SCSR;
typedef struct tagSCSR
{
	unsigned TDRE				:1;		// transmit data register empty
	unsigned TC				:1;		// transmit complete
	unsigned RDRF				:1;		// recieve data register full
	unsigned IDLE				:1;		// Idle-Line Detect
	unsigned OR				:1;		// Overrun Error
	unsigned NF				:1;		// Noise Flag
	unsigned FE				:1;		// Framing Error
	unsigned MATCH				:1;		// Timer Match
} SCSRBITS;

extern volatile SCSRBITS SCSRbits;

// 0x4009
extern volatile unsigned char SCIRX;
// 0x400A
extern volatile unsigned char SCITX;
// 0x400B
extern volatile unsigned char COPLCK;
// 0x400C
extern volatile unsigned char COPCLK;

/******************************
FMD - Fuel Modeling Device
0x3FC0 - 0x3FFF
******************************/

// 0x3FC0
extern volatile unsigned short REFPER;
// 0x3FD0
extern unsigned short INJPER;
// 0x3FD2
extern unsigned short A4OUT;
// 0x3FD4
extern unsigned short A2OUT;
// 0x3FD6
extern unsigned short A7OUT;
// 0x3FD8
extern unsigned short A3OUT;
// 0x3FDA
extern unsigned short D12OUT;
// 0x3FCC
extern unsigned short C2OUT;
// 0x3FEA
extern unsigned short PWM;
// 0x3FDC
extern unsigned short DWELL;
// 0x3FF2
extern unsigned short ASYNC;

// 0x3FFA
extern volatile unsigned short FMDSR;
typedef struct tagFMDSR
{
	unsigned				:1;
	unsigned INJ_OCCURRED			:1;
	unsigned				:1;
	unsigned				:1;

	unsigned 				:1;
	unsigned				:1;
	unsigned 				:1;
	unsigned				:1;

	unsigned				:1;
	unsigned				:1;
	unsigned				:1;
	unsigned				:1;

	unsigned				:1;
	unsigned				:1;
	unsigned				:1;
	unsigned				:1;
} FMDSRBITS;
extern volatile FMDSRBITS FMDSRbits;

// 0x3FFC
extern volatile unsigned short FMDCR;
typedef struct tagFMDCR
{
	unsigned 				:1;
	unsigned				:1;
	unsigned				:1;
	unsigned				:1;

	unsigned SYNC_ENABLE			:1;
	unsigned ASYNC_ENABLE			:1;
	unsigned				:1;
	unsigned				:1;

	unsigned				:1;
	unsigned ASYNC_EXTENDS			:1;
	unsigned DUAL_INJECT			:1;
	unsigned				:1;

	unsigned CEL_CLEAR			:1;
	unsigned 				:1;
	unsigned				:1;
	unsigned				:1;
} FMDCRBITS;
extern volatile FMDCRBITS FMDCRbits;

// (15)	A7 	1 = REFPER tics 65535/s 0 = REFPER tics 3 x 65535/s
// (14)	A6	PP2  ENABLE ON I6  1 = ENABLE, 0 = DISABLE
// (13) A5	ENABLE ON I5  1 = ENABLE, 0 = DISABLE
// (12) A4	1 = PA ENABLE ON I2

// (11) A3	* 1 = ENABLE SYNCHRONOUS FUEL (P1) (EITHER SINGLE OR DUAL) disables refper *
// (10) A2	1 = INITIATE ASYNC PULSE (P0)
// (9)	A1	1 = ENABLE P3 AS PW1 OUTPUT (UNLESS CR5=1
// (8)	A0	1 = ENABLE  P4 THRU P9

// (7)	B7	NOT USED
// (6)	B6	1 = EXTEND ASYNC PULSE (P0) IF OVERLAP W/SYNC (P1)
// (5)	B5	(P1,P3) SINGLE OR DUAL INJECTOR
// (4)	B4	*(P2) EST ENABLE   1 = EST BYPASS 5V, 0 = EST BYPASS 0V*

// (3)	B3 	*CEL, 0 = CEL light ON, 1 = CEL light off*
// (2)	B2	DISCRETE OUTPUT
// (1)	B1	MASTER OUTPUT ENABLE   1 = ENABLE, 0
// (0)	B0 	0 = disable spark, 1 = spark enable	  ECU TEST    0 FOR NORMAL OPERATION

typedef struct tagDISCRETE
{
	unsigned STATUS_SEL			:1;		// 0 = read discrete byte, 1 = read status register
	unsigned IRQ_EN				:1;		// once asserted, must be disabled to clear INT
	unsigned UNKNOWN			:1;
	unsigned P8SRC				:1;		// Pin 8 source; 0 == pin 9 input, 1 == bit3
	unsigned P8OUT				:1;		// Pin 8 output data when bit4 is set
	unsigned EST_ENABLE			:1;
	unsigned LIMP_WATCHDOG			:1;		// must be toggles every 12ms
	unsigned CTS_PULLUP			:1;		// 0 = 4k, 1 = 348 ohm
} DISCRETEBITS;

#endif
