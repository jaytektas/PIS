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

	.section .vectors

	.word	0xffff		; SWI user interrupt
	.word	interrupt	; soft timer interrupt
	.word	0xffff		; hard irq, not used
	.word	illegal		; illegal opcode
	.word	0xffff		; illegal address
	.word	0xffff		; cop timed out
	.word	0xffff 		; clock failed
	.word	_start
