#        Copyright 2010 Jason Roughley
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


# Default binary commands see README
AS = m6811-elf-as
LD = m6811-elf-ld
CC = m6811-elf-gcc
OBJCOPY = m6811-elf-objcopy

INC = include
SRC = src
OBJ = obj
BIN = bin

objects = $(OBJ)/startup.o $(OBJ)/delco.o $(OBJ)/interrupt.o $(OBJ)/helpers.o $(OBJ)/vectors.o $(OBJ)/config.o

all: delco.s19

delco.s19: $(OBJ)/delco.elf
	$(OBJCOPY) -O srec $(OBJ)/delco.elf delco.s19

$(OBJ)/delco.elf: $(objects) delco.ld
	$(LD) -g -T delco.ld -o $(OBJ)/delco.elf $(objects)

$(OBJ)/delco.o: delco.ld $(SRC)/delco.c $(INC)/delco.h $(INC)/registers.h $(INC)/types.h $(INC)/status.h $(OBJ)/config.o $(OBJ)/interrupt.o
	$(CC) -mshort -g -c -o $(OBJ)/delco.o $(SRC)/delco.c -I $(INC)

$(OBJ)/config.o: delco.ld $(INC)/types.h $(INC)/config.h $(SRC)/config.c
	$(CC) -mshort -g -c -o $(OBJ)/config.o $(SRC)/config.c -I $(INC)

$(OBJ)/interrupt.o: delco.ld $(INC)/types.h $(INC)/status.h $(OBJ)/config.o $(SRC)/interrupt.c $(INC)/interrupt.h
	$(CC) -mshort -g -c -o $(OBJ)/interrupt.o $(SRC)/interrupt.c -I $(INC)

$(OBJ)/startup.o: delco.ld $(SRC)/startup.s
		$(AS) -o $(OBJ)/startup.o $(SRC)/startup.s

$(OBJ)/helpers.o: delco.ld $(SRC)/helpers.s
		$(AS) -o $(OBJ)/helpers.o $(SRC)/helpers.s

$(OBJ)/vectors.o: delco.ld $(SRC)/vectors.s
		$(AS) -o $(OBJ)/vectors.o $(SRC)/vectors.s

.PHONY : clean

clean:
	rm $(OBJ)/*
	rm delco.s19

