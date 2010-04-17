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

#include "interrupt.h"
#include "registers.h"
#include "helpers.h"
#include "status.h"
#include "config.h"

const char TITLE[] __attribute__((section(".rom"))) = "PIS firmware (C) 2010 Jason Roughley. pis.controller@gmail.com";
const char VERSION[] __attribute__((section(".rom"))) = "V1.00";

extern struct _config config;

void interrupt(void)
{
		if (SCSRbits.MATCH) timer_interrupt();
		if (SCCR2bits.RIE && SCSRbits.RDRF) recieve();
		if (SCCR2bits.TIE && SCSRbits.TDRE) transmit();
}

// every 1ms
void timer_interrupt(void)
{
static UBYTE skip_discrete;
static UBYTE discrete_tx = 0x04;
FMDSRBITS fmdsr;
FMDCRBITS fmdcr;

	// come back in another 1ms
	MATCH += 32;

	// irq tics
	if (++status.irq_tics > 999)
	{
		status.irq_tics = 0;
		status.seconds++;
		status.flags.cts_update = TRUE;
		if (status.transient_clk) status.transient_clk--;
	}

	// roc clock / flag update
	if (status.roc_clk) status.roc_clk--;
	else status.flags.roc_update = TRUE;

	// iac clock / flag update
	if (status.iac_clk) status.iac_clk--;
	else status.flags.iac_update = TRUE;

	// ego clock / flag update
	if(status.ego_clk) status.ego_clk--;
	else status.flags.ego_update = TRUE;


	// COP reset for *this* cpu
	COPLCK = 0xff;
	COPCLK = 0x00;

	// fetch discrete every 10ms
	if (!skip_discrete--)
	{
		discrete_tx ^= 0x02;
		discrete_tx &= 0x7f;	// select discrete inputs

		CHPSELbits.IICS = TRUE;
		status.discrete = spi_io(discrete_tx) << 8;

		discrete_tx |= 0x80;	// select status byte
		status.discrete |= spi_io(discrete_tx);

		CHPSELbits.IICS = FALSE;
		skip_discrete = 10;
	}

	fmdsr = FMDSRbits;

	// was there an injection event over on the FMD?
	if (fmdsr.INJ_OCCURRED)
	{
		status.flags.inj_occurred = TRUE;
		if (!status.inj_clk)
		{
			INJPER = status.APW;
			status.inj_clk = config.inj_skips;
			if (status.asc_clk) status.asc_clk--;
		}
		else
		{
			INJPER = 0;
			status.inj_clk--;
		}

		// acceleration enrichment
		if (status.ae_clk)
		{
			status.ae_clk--;
			status.AE.APW =	interpolate_u16(status.ae_clk, 0, config.AE.cycles, 0, status.AE.BPW);
			ASYNC = status.AE.APW;
			asm("ldd 0x3ffc");
			asm("oraa #0x04");
			asm("jsr bus_delay");
			asm("std 0x3ffc");
			asm("anda #0xfb");
			asm("jsr bus_delay");
			asm("std 0x3ffc");
		} else status.flags.accel = FALSE;

	}
}

UBYTE command;
USHORT length;
UBYTE *data;
// checksum function removed for megatunix
//UBYTE chksm = 0;
USHORT wbuff;

void recieve(void)
{
UBYTE rx;
static USHORT offset;
static UBYTE state = 0;

	rx = SCIRX;

//	if (!state) chksm = 0;
//	if (state != 5) chksm ^= rx;

	switch (state)
	{
		case 0:
			command = rx;
			switch (rx)
			{
				case 'a':		// all runtime vars to satisfy megatunix
					length = sizeof(status);
					data = (UBYTE *) &status;
					transmit();
					break;

				case 'c':		// two byte seconds clock request to satisfy megatunix
					length = sizeof(status.seconds);
					data = (UBYTE *) &status.seconds;
					transmit();
					break;

			    case 'r':		// read config
				case 'w':		// write config

				case 's':		// read status

				case '?':		// read memory
				case '!':		// write memory
					state++;
					break;

				case 't':		// title
					length = sizeof(TITLE);
					data = (UBYTE *) TITLE;
					transmit();
					break;

				case 'v':		// version
					length = sizeof(VERSION);
					data = (UBYTE *) VERSION;
					transmit();
					return;
					break;

				default:
					// send serial break
					break;
			}
			break;

		case 1:
			offset = rx;
			state++;
			break;

		case 2:
			offset <<= 8;
			offset += rx;
			state++;
			break;

		case 3:
			length = rx;
			state++;
	   		break;

	   	case 4:
	   		length <<= 8;
			length += rx;

			switch (command)
			{
				case '!':
					// we only accept word aligned addresses! this is to allow "live" updates
					// byte wise data transfer would mean 1/2 written values!
					if ((offset & 0x0001 == FALSE) && (length & 0x0001 == FALSE))
					{
					    data = (UBYTE *) offset;
					    state++;
					}
					else state = 0;
					break;

				case '?':
					data = (UBYTE *) offset;
					transmit();
					state = 0;
					break;

				case 's':
					data = (UBYTE *) &status;
					data += offset;
					transmit();
					state = 0;
					break;

				case 'w':
					// we only accept word aligned addresses! this is to allow "live" updates
					// byte wise data transfer would mean 1/2 written values!
					if ((offset & 0x0001 == FALSE) && (length & 0x0001 == FALSE))
					{
					    data = (UBYTE *) &config;
					    data += offset;
					    state++;
					}
					else state = 0;
					break;

				case 'r':
					data = (UBYTE *) &config;
					data += offset;
					transmit();
					state = 0;
					break;

				default:
					state = 0;
					break;
			}
 			break;

		// write data word wise
		case 5:
			if ((USHORT) data & 0x0001)
			{
				wbuff += rx;
				*((USHORT *)data) = wbuff;
			} else wbuff = rx << 8;
			length--;
			data++;
			if (!length) state = 0;
			break;
	}
}

void transmit(void)
{
	SCCR2 = 0x89;
	if (length)
	{
		SCITX = *data++;
		length--;
	}
	else SCCR2 = 0x27;
}
