ENTRY(_start)

MEMORY
{
	RAM (rwx)		: ORIGIN = 0x0000, LENGTH = 512
	ROM (rx)		: ORIGIN = 0x8000, LENGTH = 28K
	CONFIG (rx)		: ORIGIN = 0xF000, LENGTH = 4K
	VECTORS (r)		: ORIGIN = 0xfff0, LENGTH = 16
}

SECTIONS
{
	TXT :
	{
	    *(.text)
	} > ROM

    ROM :
    {
        *(.rom)
        *(.rodata)
    } > ROM

	CFG :
	{
	    *(.config)
	} > CONFIG

	REGS :
	{
	    *(.softregs)
	} > RAM

	DATA : AT(ADDR(ROM) + SIZEOF(ROM))
	{
		 PROVIDE(__data_start = .);
		 *(.data)
		 PROVIDE(__data_end = .);
	} > RAM

	BSS :
	{
			PROVIDE(__bss_start = .);
			*(.bss)
	} > RAM

	PROVIDE(__bss_size = SIZEOF(.bss));

	VECTORS :
	{
	    *(.vectors)
	} > VECTORS
}

__data_load_start = LOADADDR(DATA);
__data_load_end = __data_load_start + SIZEOF(DATA);


PROVIDE (_stack = 0x1FF);

PROVIDE (IO_BASE = 0x4000);
PROVIDE (SPISSR = 0x4000);
PROVIDE (IOCON = 0x4001);
PROVIDE (CHPSEL = 0x4002);
PROVIDE (CHPSELbits = CHPSEL);

PROVIDE (DDR = 0x4003);
PROVIDE (PPORT = 0x4004);
PROVIDE (PPORTbits = PPORT);

PROVIDE (CLOCK = 0x4005);
PROVIDE (MATCH = 0x4006);
PROVIDE (SCCR2 = 0x4007);
PROVIDE (SCCR2bits = SCCR2);

PROVIDE (SCSR = 0x4008);
PROVIDE (SCSRbits = SCSR);

PROVIDE (SCIRX = 0x4009);
PROVIDE (SCITX = 0x400a);
PROVIDE (COPLCK = 0x400b);
PROVIDE (COPCLK = 0x400c);

PROVIDE (REFPER = 0x3fc0);
PROVIDE (INJPER = 0x3fd0);

PROVIDE (A4OUT = 0x3fd2);
PROVIDE (A2OUT = 0x3fd4);
PROVIDE (A7OUT = 0x3fd6);
PROVIDE (A3OUT = 0x3fd8);
PROVIDE (D12OUT = 0x3fda);
PROVIDE (C2OUT = 0x3fcc);
PROVIDE (PWM = 0x3fea);

PROVIDE (DWELL = 0x3fdc);
PROVIDE (ASYNC = 0x3ff2);
PROVIDE (FMDSR = 0x3ffa);
PROVIDE (FMDSRbits = FMDSR);

PROVIDE (FMDCR = 0x3ffc);
PROVIDE (FMDCRbits = FMDCR);
