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

#ifndef _STATUS_H_
#define _STATUS_H_

#include "types.h"

struct _status
{
    USHORT seconds;							// 2
	USHORT irq_tics;  						// 0

	struct s_flags							// updated from interrupt
	{
		unsigned inj_occurred	:1;			// 4
		unsigned cranking		:1;
		unsigned running		:1;
		unsigned asc			:1;
		unsigned roc_update		:1;
		unsigned iac_update		:1;
		unsigned iac_reset		:1;
		unsigned cts_update		:1;

	    unsigned hard_limit		:1;			// 5
	    unsigned soft_limit		:1;
	    unsigned boost_limit	:1;
	    unsigned ego_update		:1;
	    unsigned closed_loop	:1;
	    unsigned accel			:1;
	    unsigned decel			:1;
	    unsigned fuel_cut		:1;

		unsigned idle_control	:1;			// 6
		unsigned refper_valid	:1;
		unsigned alpha_n		:1;
		unsigned fan_on			:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;

	    unsigned 				:1;			// 7
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	    unsigned 				:1;
	} flags;

	USHORT	discrete;						// 8

	// update clocks
	USHORT roc_clk; 						// 10
	USHORT iac_clk;							// 12
	USHORT ego_clk;							// 14
	USHORT asc_clk;							// 16
	USHORT ae_clk;							// 18
	USHORT inj_clk;							// 20
	USHORT transient_clk;					// 22

	struct s_rpm
	{
		USHORT rpm;	 						// 24
		USHORT magic;						// 26
		SHORT roc;							// 28
		USHORT last;						// 30
		USHORT limit;						// 32
	} rpm;

	struct s_cts
	{
		SHORT iar;							// 34	// final ignition -advance / +retarard adjustment vs CTS
	    USHORT kelvin;						// 36	// air temperature in kelvins
	    USHORT correction;					// 38	// correction as %000.00 x 327.68 vs CTS
	    SHORT PW;							// 40	// final base pulse width adjustment
	} cts;

	struct s_mat
	{
		SHORT iar;							// 42	// final ignition -advance / +retarard adjustment vs MAT
	    USHORT kelvin;						// 44	// air temperature in kelvins
	    USHORT correction;					// 46	// correction as %000.00 x 327.68 vs MAT
	    SHORT PW;							// 48	// final base pulse width adjustment
	} mat;

	struct s_asc
	{
	    USHORT base;						// 50	// correction vs CTS
	    USHORT duration;					// 52	// duration / tapering vs CTS
		USHORT applied;						// 54	// correction as %000.00 x 327.68
		SHORT PW;							// 56	// final base pulse width adjustment
	} asc;

	struct s_tps
	{
	    USHORT tps;							// 58
	    USHORT last;						// 60
		SHORT roc;							// 62
	} tps;

	struct s_iac
	{
		USHORT start;  						// 64
		USHORT minimum;						// 66
		USHORT target;						// 68
		USHORT position;					// 70
		USHORT idle_rpm;					// 72
	} iac;

	struct s_ign
	{
		USHORT AIAR;						// 74
	    SHORT dwell;						// 76
	} ign;

	struct s_map
	{
		USHORT map;							// 78
		USHORT limit;						// 80	// boost limit kPa
	} map;

	struct s_ego
	{
		USHORT correction;					// 82
		USHORT target;						// 84 // ADC target
		USHORT error;						// 86
		SHORT integral;						// 88
		SHORT PW;							// 90
	} ego;

	struct s_AE
	{
		USHORT cts_factor;					// 92  // last factor % applied to BPW chosen according to CTS
		USHORT roc_factor;					// 94  // first factor % applied to BPW chosen according to TPS ROC
		USHORT BPW;							// 96  // base pw chosen according to RPM VS TPS
		USHORT APW;							// 98  // actual pulse width being sent out as AE pulse, linear tapers to nothing vs injection cycles chosen.
	} AE;

	// Applied Pulse Width after all corrections
    USHORT APW;								// 100
    USHORT open_delay;						// 102

	// table indexes
	struct s_idx
	{
	    UBYTE rpm;							// 104
	    UBYTE map;							// 105
	    UBYTE cts;							// 106
	    UBYTE mat;							// 107
	    UBYTE tps;							// 108
	    UBYTE iac;							// 109
		UBYTE AE_tps;						// 110
		UBYTE AE_rpm;						// 111
		UBYTE AE_roc;						// 112
	} idx;

	struct s_adc
	{
	    UBYTE wbo2;							// 113 // ECU pin D8
	    UBYTE volt;							// 114
	    UBYTE nbo2;							// 115
	    UBYTE map;							// 116
	    UBYTE cts;							// 117
	    UBYTE tps;							// 118
	    UBYTE pumpvolt;						// 119
	    UBYTE diag;							// 120
	    UBYTE mat;							// 121
	    UBYTE esc;							// 122
	    UBYTE vmaf;							// 123
	} adc;
};

#endif
