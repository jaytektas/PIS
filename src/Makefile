#        Copyright 2010 Jason Roughley, Fred Cooke
#
#        This file is part of PIS firmware.
#
#    PIS firmware is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    PIS firmware is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with PIS firmware.  If not, see <http://www.gnu.org/licenses/>.


# Command for cleaning up, uncomment the second one for *nix
#ifdef _WIN32_
DELETE = del
#else
DELETE = rm
#endif

# Default binary commands see README
AS = m6811-elf-as
LD = m6811-elf-ld
GCC = m6811-elf-gcc
OBJCOPY = m6811-elf-objcopy

INCLUDE = ../include

objects = startup.o delco.o interrupt.o helpers.o vectors.o config.o

delco.s19: delco.elf
	$(OBJCOPY) -O srec delco.elf delco.s19

delco.elf: $(objects) delco.ld
	$(LD) -g -T delco.ld -o delco.elf $(objects)

delco.o: delco.ld delco.c $(INCLUDE)/delco.h $(INCLUDE)/registers.h $(INCLUDE)/types.h $(INCLUDE)/status.h config.o interrupt.o
	$(GCC) -mshort -g -c -o delco.o delco.c -I $(INCLUDE)

config.o: delco.ld $(INCLUDE)/types.h $(INCLUDE)/config.h config.c
	$(GCC) -mshort -g -c -o config.o config.c -I $(INCLUDE)

interrupt.o: delco.ld $(INCLUDE)/types.h $(INCLUDE)/status.h config.o interrupt.c $(INCLUDE)/interrupt.h
	$(GCC) -mshort -g -c -o interrupt.o interrupt.c -I $(INCLUDE)

startup.o: delco.ld startup.s
		$(AS) -o startup.o startup.s

helpers.o: delco.ld helpers.s
		$(AS) -o helpers.o helpers.s

vectors.o: delco.ld vectors.s
		$(AS) -o vectors.o vectors.s

.PHONY : clean

clean:
	$(DELETE) $(objects)
	$(DELETE) delco.elf
	$(DELETE) delco.s19

