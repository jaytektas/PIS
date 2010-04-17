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

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_
#include "types.h"
#include "status.h"

void interrupt(void) __attribute__((interrupt));
void timer_interrupt(void);
void recieve(void);
void transmit(void);
extern struct _status status;

extern const char _title[];
extern const char _version[];

#endif
