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

#ifndef _DELCO_H_
#define _DELCO_H_

#include "types.h"

#define MAP2			0x00
#define VOLT			0x10
#define O2				0x20
#define MAP				0x30
#define CTS				0x40
#define TPS				0x50
#define PUMPVOLT		0x60
#define DIAG			0x70
#define MAT				0x80
#define ESC				0x90
#define VMAF			0xA0
#define TEST			0xB0

#ifndef INTR_ON
#define INTR_ON()	asm("	cli")
#define INTR_OFF()	asm("	sei")
#endif

#define FOREVER while(1)

int main(void) __attribute__((noreturn));
extern UBYTE sci_index;
extern void interrupt(void);
extern USHORT magic_rpm(USHORT magic);
extern UBYTE adc_read(UBYTE channel);
extern ULONG mult_u32(USHORT, USHORT);
extern USHORT div_u32(ULONG, USHORT);
extern USHORT interpolate_u16(USHORT index, USHORT s1a, USHORT s1b, USHORT s2a, USHORT s2b);
extern USHORT interpolate_s16(USHORT index, USHORT s1a, USHORT s1b, SHORT s2a, SHORT s2b);
extern USHORT scale_u16(USHORT multiplicand, USHORT percent_000_00_x_327_68);
extern USHORT mult_u16(UBYTE multiplicand, UBYTE multiplier);
extern void set_ignition(USHORT dwell, SHORT iar);
extern UBYTE open_delay(UBYTE voltage, UBYTE *delay_table);

void roc_update(void);
void iac_update(void);
void cts_update(void);
void ego_update(void);

/* magic macros -- don't change these */
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define SIZEOF(TYPE, MEMBER) (sizeof(((TYPE *)0)->MEMBER))

#define _DEFINE(sym, val) asm ("\n-> " #sym " %0 " #val "\n" : : "i" (val))
#define DEFINE(s, m) \
	_DEFINE(offsetof_##s##_##m, offsetof(s, m)); \
	_DEFINE(sizeof_##s##_##m, SIZEOF(s, m));

/* function with your structures and members */

#endif
