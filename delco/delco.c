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

#include "delco.h"
#include "registers.h"
#include "config.h"
#include "types.h"
#include "status.h"
#include "interrupt.h"

#define BOOST_SOLENOID D12OUT
#define TURBO_TIMER B12OUT
#define EGO_WIDEBAND MAP2

extern struct _config config;
struct _status status;

int main(void)
{
UBYTE t;
USHORT APW, AIAR;

	SCCR2 = 0x27; 						// sci ready to recieve
	FMDCR = 0xfb12;
	PPORTbits.SXR_ENABLE = TRUE;				// sxr tx enable

	INTR_ON();

	adc_read(TEST);						// init adc

	// reset the iac motor to known position
	status.flags.iac_reset = TRUE;
	status.flags.cts_update = TRUE;

	switch (config.cylinders)
	{
		case 1:
			status.rpm.magic = 240;
			break;

		case 2:
			status.rpm.magic = 120;
			break;

		case 3:
			status.rpm.magic = 80;
			break;

		case 4:
			status.rpm.magic = 60;
			break;

		case 5:
			status.rpm.magic = 48;
			break;

		case 6:
			status.rpm.magic = 40;
			break;

		case 8:
			status.rpm.magic = 30;
			break;

		case 10:
			status.rpm.magic = 24;
			break;

		case 12:
			status.rpm.magic = 20;
			break;

		case 16:
			status.rpm.magic = 15;
			break;
	}

	if (!config.two_stroke) status.rpm.magic <<= 1;

	FOREVER
	{
		// tps roc
		if (status.flags.roc_update) roc_update();
		if (status.flags.cts_update) cts_update();

		status.adc.volt = adc_read(VOLT);
		if (status.adc.volt	< 40) asm("swi");

		status.adc.wbo2 = adc_read(EGO_WIDEBAND);
		status.adc.nbo2 = adc_read(O2);

		// currently not used (yet)
//		status.adc.pumpvolt = adc_read(PUMPVOLT);
//		status.adc.diag = adc_read(DIAG);
//		status.adc.esc = adc_read(ESC);
//		status.adc.vmaf = adc_read(VMAF);

		// ADC map -> kpa
		status.adc.map = adc_read(MAP);
		status.map.map = interpolate_u16(status.adc.map, 0, 255, config.map.lo_kpa, config.map.hi_kpa);
		t = MAP_BINS - 2;
	    do
	    {
	    	if (status.map.map >= config.map_bins[t]) break;
	    } while (--t);
		status.idx.map = t;

		// ADC mat -> kelvins
		status.adc.mat = adc_read(MAT);
		status.mat.kelvin = config.mat.kelvin[status.adc.mat];
		t = MAT_BINS - 2;
		do
		{
			if (status.mat.kelvin >= config.mat.bins[t]) break;
		} while (--t);
		status.idx.mat = t;

	    if (status.flags.refper_valid)
	    	status.rpm.rpm = magic_rpm(status.rpm.magic);
		else
		{
			status.rpm.rpm = 0;
			status.flags.cranking = FALSE;
			status.flags.running = FALSE;
		}

		t = RPM_BINS - 2;
		do
		{
			if (status.rpm.rpm >= config.rpm_bins[t]) break;
		} while (--t);
		status.idx.rpm = t;

		// at least once
		if (!status.flags.running)
		{
			if (status.rpm.rpm >=0) status.flags.cranking = TRUE;

			// EST bypass
			if (config.ign.crank.bypass) FMDCR &= 0xFFEF;

			// IAC into start position
			status.iac.target =
				interpolate_u16(status.cts.kelvin,
					config.iac.cts_bins[status.idx.iac], config.iac.cts_bins[status.idx.iac+1],
						config.iac.cranking[status.idx.iac], config.iac.cranking[status.idx.iac+1]);

			// iac update
			if (config.iac.enabled)
				if (status.flags.iac_update)
					iac_update();

			// check for flood clear
			if (status.tps.tps > config.tps.flood_clear) APW = 0;
			else
			{
				APW	= interpolate_u16(status.cts.kelvin, config.cts.bins[status.idx.cts],
					config.cts.bins[status.idx.cts+1], config.CPW[status.idx.cts], config.CPW[status.idx.cts+1]);
			}

			status.ign.dwell = config.ign.crank.dwell;
		}

		if (status.flags.running || (status.flags.refper_valid && status.rpm.rpm >= config.cranking_threshold))
		{
		    status.flags.running = TRUE;

			// just entered into run mode?
		    if (status.flags.cranking)
		    {
		    	status.flags.cranking = FALSE;

		    	// EST bypass
		    	FMDCR |= 0x0010;

		    	// initialise ASC
				status.flags.asc = TRUE;

			    status.asc.base = interpolate_u16(status.cts.kelvin,
			    	config.cts.bins[status.idx.cts], config.cts.bins[status.idx.cts+1],
			    		config.asc.base[status.idx.cts], config.asc.base[status.idx.cts+1]);

		        status.asc.duration = interpolate_u16(status.cts.kelvin,
		        	config.cts.bins[status.idx.cts], config.cts.bins[status.idx.cts+1],
		        		config.asc.duration[status.idx.cts], config.asc.duration[status.idx.cts+1]);

			    // start the ASC clock
				status.asc_clk = status.asc.duration;
		    }

			if (config.iac.enabled && status.flags.iac_update)
			{
				if (status.flags.idle_control)
				{
					// idle low
					if (status.rpm.rpm < (status.iac.idle_rpm - config.iac.hysterisis))
					    if (status.iac.target < config.iac.range) status.iac.target++;

					// idle hi
					if (status.rpm.rpm > (status.iac.idle_rpm + config.iac.hysterisis))
						if (status.iac.target > status.iac.minimum) status.iac.target--;
				}

				if (!status.flags.idle_control)
					status.iac.target = interpolate_u16(status.tps.tps, 0, 1000,
						status.iac.start, config.iac.on_throttle);

				iac_update();
			}

			// alpha-N base pulse width
			if (status.rpm.rpm < config.alpha_N.full_map_rpm)
			{
				status.flags.alpha_n = TRUE;
			    // 3D BPW table interpolation
			    APW =
				    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
				    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
				    		config.alpha_N.BPW[status.idx.tps][status.idx.rpm], config.alpha_N.BPW[status.idx.tps+1][status.idx.rpm]),
						    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
						    		config.alpha_N.BPW[status.idx.tps][status.idx.rpm+1], config.alpha_N.BPW[status.idx.tps+1][status.idx.rpm+1]));

			    if (status.rpm.rpm > config.alpha_N.start_map_rpm)
			    {
					APW = interpolate_u16(status.rpm.rpm, config.alpha_N.start_map_rpm, config.alpha_N.full_map_rpm,

					    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
					    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
					    		config.alpha_N.BPW[status.idx.tps][status.idx.rpm], config.alpha_N.BPW[status.idx.tps+1][status.idx.rpm]),
							    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
							    		config.alpha_N.BPW[status.idx.tps][status.idx.rpm+1], config.alpha_N.BPW[status.idx.tps+1][status.idx.rpm+1])),

					    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
					    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
					    		config.BPW[status.idx.map][status.idx.rpm], config.BPW[status.idx.map+1][status.idx.rpm]) ,
							    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
							    		config.BPW[status.idx.map][status.idx.rpm+1], config.BPW[status.idx.map+1][status.idx.rpm+1])));
			    }
			}
			else
			{
			    status.flags.alpha_n = FALSE;
			    // 3D BPW table interpolation
			    APW =
				    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
				    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
				    		config.BPW[status.idx.map][status.idx.rpm], config.BPW[status.idx.map+1][status.idx.rpm]),
						    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
						    		config.BPW[status.idx.map][status.idx.rpm+1], config.BPW[status.idx.map+1][status.idx.rpm+1]));
			}

			// CTS fuel correction
			status.cts.PW = scale_u16(APW, status.cts.correction) - APW;

			// MAT fuel correction
			status.mat.correction = interpolate_u16(status.mat.kelvin, config.mat.bins[status.idx.mat],
				config.mat.bins[status.idx.mat+1], config.mat.correction[status.idx.mat],
					config.mat.correction[status.idx.mat+1]);

			status.mat.PW = scale_u16(APW, status.mat.correction) - APW;

			// ASC
			if (status.flags.asc)
			{
				if (!status.asc_clk) status.flags.asc = FALSE;
				else
				{
						status.asc.applied = interpolate_u16(status.asc_clk,  status.asc.duration, 0, status.asc.base, 32768);
						status.asc.PW = scale_u16(APW, status.asc.applied) - APW;
				}
			}

			// EGO
			if (config.ego.enabled && status.flags.ego_update) ego_update();
			status.ego.PW = scale_u16(APW, status.ego.correction) - APW;

			// DECEL
			// CUT

			// boost target
			if (!status.flags.boost_limit)
			{
				if (status.map.map >= status.map.limit)
				{
					status.flags.boost_limit = TRUE;
					// switch solenoid on
					BOOST_SOLENOID = 0xffff;
				}
			}
			else
			{
				if (status.map.map < status.map.limit - config.boost.hysterisis)
				{
					status.flags.boost_limit = FALSE;
					// switch solenoid off
					BOOST_SOLENOID = 0;
				}
			}

			// rpm limit
			if (status.rpm.limit)
			{
				// soft limit
				if (status.rpm.rpm > status.rpm.limit - config.rpm_limiter.soft_offset) status.flags.soft_limit = TRUE;
				else status.flags.soft_limit = FALSE;

				// hard limit
				if (status.rpm.rpm >= status.rpm.limit)
				{
					status.flags.hard_limit = TRUE;
				}
				else
				{
					if (status.rpm.rpm < status.rpm.limit - config.rpm_limiter.hysterisis)
						status.flags.hard_limit = FALSE;
				}
			}
			else
			{
				status.flags.hard_limit = FALSE;
				status.flags.soft_limit = FALSE;
			}

			// NOS?

		    // final sync fuel
		    APW += status.cts.PW;
		    APW += status.mat.PW;
		    if (status.flags.closed_loop) APW += status.ego.PW;
		    if (status.flags.asc) APW += status.asc.PW;
		}

		// alpha-N base ignition +advance / -retard
		if (status.flags.alpha_n)
		{
			    // 3D BIAR table interpolation
			    AIAR =
				    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
				    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
				    		config.alpha_N.BIAR[status.idx.tps][status.idx.rpm], config.alpha_N.BIAR[status.idx.tps+1][status.idx.rpm]),
						    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
						    		config.alpha_N.BIAR[status.idx.tps][status.idx.rpm+1], config.alpha_N.BIAR[status.idx.tps+1][status.idx.rpm+1]));

			    if (status.rpm.rpm > config.alpha_N.start_map_rpm)
			    {
					AIAR = interpolate_u16(status.rpm.rpm, config.alpha_N.start_map_rpm, config.alpha_N.full_map_rpm,

					    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
					    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
					    		config.alpha_N.BIAR[status.idx.tps][status.idx.rpm], config.alpha_N.BIAR[status.idx.tps+1][status.idx.rpm]),
							    	interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
							    		config.alpha_N.BIAR[status.idx.tps][status.idx.rpm+1], config.alpha_N.BIAR[status.idx.tps+1][status.idx.rpm+1])),

					    interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
					    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
					    		config.ign.BIAR[status.idx.map][status.idx.rpm], config.ign.BIAR[status.idx.map+1][status.idx.rpm]) ,
							    	interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
							    		config.ign.BIAR[status.idx.map][status.idx.rpm+1], config.ign.BIAR[status.idx.map+1][status.idx.rpm+1])));
			    }
		}
		else
		{
			// MAP base ignition +advance / -retard
		    AIAR =
			    interpolate_s16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
			    	interpolate_s16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
			    		config.ign.BIAR[status.idx.map][status.idx.rpm], config.ign.BIAR[status.idx.map+1][status.idx.rpm]) ,
					    	interpolate_s16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
					    		config.ign.BIAR[status.idx.map][status.idx.rpm+1], config.ign.BIAR[status.idx.map+1][status.idx.rpm+1])
		    	);
		}

		// MAT ignition correction
		status.mat.iar = interpolate_s16(status.mat.kelvin,
			config.mat.bins[status.idx.mat], config.mat.bins[status.idx.mat+1],
				config.mat.iar[status.idx.mat], config.mat.iar[status.idx.mat+1]);

		AIAR += status.mat.iar;
		AIAR += status.cts.iar;

		// soft limit
		if (status.flags.soft_limit)
		{
			AIAR += interpolate_s16(status.rpm.rpm, status.rpm.limit - config.rpm_limiter.soft_offset,
						status.rpm.limit, config.rpm_limiter.soft_iar, config.rpm_limiter.hard_iar);
		}

		// +advance / -retard limit check
		//if (timing < config.ign.min_iar) timing = config.ign.min_iar;
		//if (timing < config.ign.max_iar) timing = config.ign.max_iar;

		// save for external reference
		status.ign.AIAR = AIAR;

		// timing relative to TDC
		AIAR -= config.ign.ref_angle;

		t = status.adc.volt >> 5;
		status.ign.dwell = interpolate_u16(status.adc.volt & 0x0f, 0, 15, config.ign.run.dwell[t], config.ign.run.dwell[t+1]);

		// dwell limit checking
		if (status.ign.dwell < config.ign.run.min_dwell) status.ign.dwell = config.ign.run.min_dwell;
		if (status.ign.dwell > config.ign.run.max_dwell) status.ign.dwell = config.ign.run.max_dwell;

		// set the ignition
		set_ignition(status.ign.dwell, AIAR);

		// add injector pintle opening time (voltage compensated)
		status.open_delay = interpolate_u16(status.adc.volt & 0x0f, 0, 15, config.open_delay[t], config.open_delay[t+1]);

		if (APW) APW += status.open_delay;

		// set the fuel
		INTR_OFF();
		status.APW = APW;
		INTR_ON();
	}
}

// every ~100ms
void roc_update()
{
UBYTE t;
USHORT AEBPW;

	status.roc_clk = 100;	// come back see us in 100ms
	status.flags.roc_update = FALSE;

	status.rpm.roc = status.rpm.rpm - status.rpm.last;
	status.rpm.last = status.rpm.rpm;

	status.tps.last = status.tps.tps;
	// ADC tps -> % x 10
	status.adc.tps = adc_read(TPS);
	status.tps.tps = interpolate_u16(status.adc.tps, config.tps.lo_count, config.tps.hi_count, 0, 1000);

	// idle stable enough for idle control?
	if (status.rpm.roc <= 0)
		if (!status.rpm.roc || ~status.rpm.roc <= config.iac.rpm_roc)
			if (status.tps.tps <= config.iac.idle_tps)
				status.flags.idle_control = TRUE;
			else
				status.flags.idle_control = FALSE;

	t = TPS_BINS - 2;
    do
    {
    	if (status.tps.tps >= config.tps.bins[t]) break;
    } while (--t);
	status.idx.tps = t;

	// boost limit 3D table interpolation
    status.map.limit =
        interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
    		interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
		    	config.boost.target[status.idx.tps][status.idx.rpm], config.boost.target[status.idx.tps+1][status.idx.rpm]),
		    		interpolate_u16(status.tps.tps, config.tps.bins[status.idx.tps], config.tps.bins[status.idx.tps+1],
		    			config.boost.target[status.idx.tps][status.idx.rpm+1], config.boost.target[status.idx.tps+1][status.idx.rpm+1]));

	status.tps.roc = status.tps.tps - status.tps.last;

	if (status.tps.roc >= 0 && status.tps.roc >= config.AE.tps.threshold)
	{
		t = AE_RPM_BINS - 2;
	    do
	    {
	    	if (status.rpm.rpm >= config.AE.rpm.bins[t]) break;
	    } while (--t);
		status.idx.AE_rpm = t;

		t = AE_TPS_BINS - 2;
	    do
	    {
	    	if (status.tps.last >= config.AE.tps.bins[t]) break;
	    } while (--t);
		status.idx.AE_tps = t;

		AEBPW =
	        interpolate_u16(status.rpm.rpm, config.AE.rpm.bins[status.idx.AE_rpm], config.AE.rpm.bins[status.idx.AE_rpm+1],
	    		interpolate_u16(status.tps.last, config.AE.tps.bins[status.idx.AE_tps], config.AE.tps.bins[status.idx.AE_tps+1],
			    	config.AE.BPW[status.idx.AE_tps][status.idx.AE_rpm], config.AE.BPW[status.idx.AE_tps+1][status.idx.AE_rpm]),
			    		interpolate_u16(status.tps.last, config.AE.tps.bins[status.idx.AE_tps], config.AE.tps.bins[status.idx.AE_tps+1],
			    			config.AE.BPW[status.idx.AE_tps][status.idx.AE_rpm+1], config.AE.BPW[status.idx.AE_tps+1][status.idx.AE_rpm+1]));

		t = AE_ROC_BINS - 2;
	    do
	    {
	    	if (status.tps.roc >= config.AE.roc.bins[t]) break;
	    } while (--t);
		status.idx.AE_roc = t;

		status.AE.roc_factor =
			interpolate_u16(status.tps.roc, config.AE.roc.bins[status.idx.AE_roc], config.AE.roc.bins[status.idx.AE_roc+1],
				config.AE.roc.factor[status.idx.AE_roc], config.AE.roc.factor[status.idx.AE_roc+1]);

		AEBPW = scale_u16(AEBPW, status.AE.roc_factor);
		AEBPW = scale_u16(AEBPW, status.AE.cts_factor);

		// already acceleration enrichment?
		if (status.flags.accel)
		{
			INTR_OFF();
			if (AEBPW >= status.AE.BPW) status.AE.BPW = AEBPW;
			INTR_ON();
		}
		else
		{
			status.flags.accel = TRUE;
			INTR_OFF();
			status.ae_clk = config.AE.cycles;
			status.AE.BPW = AEBPW;
			INTR_ON();
		}
	}
}

void iac_update()
{
    status.iac_clk = config.iac.update_ms;
    status.flags.iac_update = FALSE;

	if (status.flags.iac_reset)
	{
	    PPORT |= 0x04;
		iac_increase();
		if (status.iac.position++ >= config.iac.range + 10)
		{
			status.flags.iac_reset = FALSE;
			status.iac.position = config.iac.range;
		}
	}
	else // step towards target if need be
	{
		if (status.iac.position < status.iac.target && status.iac.position < config.iac.range)
		{
		    PPORT |= 0x04;
		    iac_increase();
			status.iac.position++;
		}
		else if (status.iac.position > status.iac.target && status.iac.position > 0)
		{
		    PPORT |= 0x04;
			iac_decrease();
			status.iac.position--;
		}
		else PPORT &= 0xfb;
	}
}

// every 1 second
void cts_update()
{
UBYTE t;

	status.flags.cts_update = FALSE;

	// did we see an injection event in the last 1 second?
	if (status.flags.inj_occurred) status.flags.refper_valid = TRUE;
	else status.flags.refper_valid = FALSE;

	// set during interrupt
	status.flags.inj_occurred = FALSE;

    // ADC clt -> kelvins
    status.adc.cts = adc_read(CTS);
    status.cts.kelvin = config.cts.kelvin[status.adc.cts];
    t = CTS_BINS - 2;
    do
    {
    	if (status.cts.kelvin >= config.cts.bins[t]) break;
    } while (--t);
    status.idx.cts = t;

	// AE cts factor
	status.AE.cts_factor =
		interpolate_u16(status.cts.kelvin,
			config.cts.bins[status.idx.cts], config.cts.bins[status.idx.cts+1],
				config.AE.cts.factor[status.idx.cts], config.AE.cts.factor[status.idx.cts+1]);

	// IAC values based on CTS
	if (config.iac.enabled)
	{
	    // IAC CTS idx
	    t = IAC_BINS - 2;
	    do
	    {
	    	if (status.cts.kelvin >= config.iac.cts_bins[t]) break;
	    } while (--t);
	    status.idx.iac = t;

		// iac start position upon entering idle control
		status.iac.start =
			interpolate_u16(status.cts.kelvin,
				config.iac.cts_bins[status.idx.iac], config.iac.cts_bins[status.idx.iac+1],
					config.iac.start[status.idx.iac], config.iac.start[status.idx.iac+1]);

		// idle rpm target
		status.iac.idle_rpm =
			interpolate_u16(status.cts.kelvin,
				config.iac.cts_bins[status.idx.iac], config.iac.cts_bins[status.idx.iac+1],
					config.iac.idle_rpm[status.idx.iac], config.iac.idle_rpm[status.idx.iac+1]);

		// minimum iac position
		status.iac.minimum =
			interpolate_u16(status.cts.kelvin,
				config.iac.cts_bins[status.idx.iac], config.iac.cts_bins[status.idx.iac+1],
					config.iac.minimum[status.idx.iac], config.iac.minimum[status.idx.iac+1]);

	}

    status.rpm.limit = interpolate_u16(status.cts.kelvin,
    	config.cts.bins[status.idx.cts], config.cts.bins[status.idx.cts+1],
    		config.rpm_limiter.limits[status.idx.cts], config.rpm_limiter.limits[status.idx.cts+1]);

    // CTS fuel correction % to PW
    status.cts.correction = interpolate_u16(status.cts.kelvin, config.cts.bins[status.idx.cts],
    	config.cts.bins[status.idx.cts+1], config.cts.correction[status.idx.cts],
    		config.cts.correction[status.idx.cts+1]);

    // CTS Ignition +advance / -retard Correction
    status.cts.iar = interpolate_s16(status.cts.kelvin,
    	config.cts.bins[status.idx.cts], config.cts.bins[status.idx.cts+1],
    		config.cts.iar[status.idx.cts], config.cts.iar[status.idx.cts+1]);

    // cooling fan control
	if (status.cts.kelvin >= config.cts.fan_on)
	{
		status.flags.fan_on = TRUE;
		PPORTbits.FAN_ENABLE = TRUE;
	}
	else if (status.cts.kelvin < config.cts.fan_off)
	{
		status.flags.fan_on = FALSE;
		PPORTbits.FAN_ENABLE = FALSE;
	}
}

void ego_update()
{
	status.ego_clk = config.ego.update_ms;
	status.flags.ego_update = FALSE;

	// set / clear closed loop
	if (!status.flags.hard_limit && !status.flags.fuel_cut && !status.flags.accel && !status.flags.asc && !status.flags.decel)
	{
		if (config.ego.enabled && status.cts.kelvin >= config.ego.min_cts_kelvin && status.seconds >= config.ego.min_uptime)
		{
			if (status.rpm.rpm >= config.ego.min_rpm && status.rpm.rpm <= config.ego.max_rpm)
			{
			   	if (status.tps.tps >= config.ego.min_tps && status.tps.tps <= config.ego.max_tps)
			   	{
					if (status.map.map >= config.ego.min_map && status.map.map <= config.ego.max_map)
					{
						if (!status.flags.closed_loop && !status.transient_clk)
						{
							status.flags.closed_loop = TRUE;
							status.ego.correction = 100 * 327.68;		// 100%
							status.ego.integral = 0;
						}
					} else status.flags.closed_loop = FALSE;
				} else status.flags.closed_loop = FALSE;
			} else status.flags.closed_loop = FALSE;
		} else status.flags.closed_loop = FALSE;
	}
	else
	{
		status.flags.closed_loop = FALSE;
		status.ego.correction = 32768;
		status.transient_clk = config.ego.transient_delay;
	}

	// ego (adc) target
    status.ego.target =
        interpolate_u16(status.rpm.rpm, config.rpm_bins[status.idx.rpm], config.rpm_bins[status.idx.rpm+1],
    		interpolate_u16(status.map.map, config.map_bins[status.idx.map], config.map_bins[status.idx.map+1],
		    	config.ego.target[status.idx.map][status.idx.rpm], config.ego.target[status.idx.map+1][status.idx.rpm]),
		    		interpolate_u16(status.map.map, config.tps.bins[status.idx.map], config.tps.bins[status.idx.map+1],
		    			config.ego.target[status.idx.map][status.idx.rpm+1], config.ego.target[status.idx.map+1][status.idx.rpm+1]));



	if (status.flags.closed_loop)
	{
		if (status.adc.wbo2 < status.ego.target) // rich
		{
			status.ego.error = status.ego.target - status.adc.wbo2;
			status.ego.integral -= mult_u16(status.ego.error, config.ego.integral);
			if (status.ego.integral < -10000) status.ego.integral = -10000;
			status.ego.correction = 32768 - mult_u16(status.ego.error, config.ego.proportional)+ status.ego.integral;
		}
		else if (status.adc.wbo2 > status.ego.target) // lean
		{
		    status.ego.error = status.adc.wbo2 - status.ego.target;
		    status.ego.integral += mult_u16(status.ego.error, config.ego.integral);
			if (status.ego.integral > 10000) status.ego.integral = 10000;
		    status.ego.correction = 32768 + mult_u16(status.ego.error, config.ego.proportional) + status.ego.integral;
		}

		if (status.ego.correction < config.ego.min_correction) status.ego.correction = config.ego.min_correction;
		if (status.ego.correction > config.ego.max_correction) status.ego.correction = config.ego.max_correction;
	}
}


