objects = startup.o delco.o interrupt.o helpers.o vectors.o config.o

delco.s19: delco.elf
	m6811-elf-objcopy -O srec delco.elf delco.s19

delco.elf: $(objects) delco.ld
	m6811-elf-ld -g -T delco.ld -o delco.elf $(objects)

delco.o: delco.ld delco.c delco.h registers.h types.h status.h config.o interrupt.o
	m6811-elf-gcc -mshort -g -c -o delco.o delco.c

config.o: delco.ld types.h config.h config.c
	m6811-elf-gcc -mshort -g -c -o config.o config.c

interrupt.o: delco.ld types.h status.h config.o interrupt.c interrupt.h
	m6811-elf-gcc -mshort -g -c -o interrupt.o interrupt.c

startup.o: delco.ld startup.s
		m6811-elf-as -o startup.o startup.s

helpers.o: delco.ld helpers.s
		m6811-elf-as -o helpers.o helpers.s

vectors.o: delco.ld vectors.s
		m6811-elf-as -o vectors.o vectors.s

.PHONY : clean

clean:
	del $(objects) \
    del delco.elf  \
    del delco.s19  \
