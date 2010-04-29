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

#include "types.h"
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define RPM_BINS 		16
#define MAP_BINS 		16
#define MAT_BINS		10
#define CTS_BINS		10
#define TPS_BINS		10
#define IAC_BINS		4
#define AE_ROC_BINS    	4
#define AE_TPS_BINS 	4
#define AE_RPM_BINS 	4

#define DISABLED		0
#define ENABLED			1

// all data is word size / aligned to allow "live" updates
struct _config
{
	USHORT inj_skips;			// engine
	USHORT cylinders;			// number of engine cylinders
	USHORT two_stroke;			// engine two stroke?

	// idx bins
	USHORT rpm_bins[RPM_BINS];								// rpm bins
	USHORT map_bins[MAP_BINS];

	struct
	{
		// load bins
		USHORT hysterisis;
		USHORT target[RPM_BINS][TPS_BINS];					// boost solenoid controller
	} boost;

	// cranking
	USHORT cranking_threshold;								// above this rpm switch strategy to run mode
	USHORT CPW[CTS_BINS];									// cranking pulse widths

	struct
	{
	    USHORT bins[CTS_BINS];								// kelvin bins
	    USHORT correction[CTS_BINS];						// BPW % correction vs CTS
	    SHORT iar[CTS_BINS];								// ignition -advance / +retard vs CTS
	    USHORT kelvin[256];									// ADC to kelvins map
	    USHORT fan_on;										// cts to turn fan on
	    USHORT fan_off;										// cts to turn fan off
	} cts;

	struct
	{
	    USHORT bins[MAT_BINS];								// kelvin bins
	    USHORT correction[MAT_BINS];						// BPW % correction vs MAT
	    SHORT iar[MAT_BINS];								// ignition -advance / +retard vs MAT
	    USHORT kelvin[256];									// ADC to kelvins map
	} mat;

	struct
	{
		USHORT base[CTS_BINS];								// adjustment %000.00 x 327.68 to fuel
		USHORT duration[CTS_BINS];							// injections over which adjustment will be tapered off
	} asc;

	// fuel
	USHORT BPW[RPM_BINS][MAP_BINS];							// base pulse widths before corrections

	// injector pintle opening delay vs battery volts
	USHORT open_delay[9];

	struct
	{
		// ignition
		SHORT ref_angle;									// reference angle relative to TDC degrees * 2.8444 +advance / -retard
		struct
		{
			USHORT bypass;									// should we active EST bypass during cranking
			USHORT dwell;									// coil dwell period during cranking mode ms * 66
		} crank;

		struct
		{
			USHORT dwell[9];								// base dwell vs volts
		    USHORT min_dwell;								// minimum dwell ms * 66
		    USHORT max_dwell;								// maximum dwell ms * 66
		} run;

		// limiting
		SHORT min_iar;										// minimum ignition +advance / -retard
		SHORT max_iar;										// maximum ignition +advance / -retard

		// base ignition +advance / -retard
		SHORT BIAR[RPM_BINS][MAP_BINS];						// base ignition +advance / -retard degrees * 2.8444
	} ign;

	// sensors
	struct
	{
		USHORT lo_count;									// TPS ADC @ 0V
		USHORT hi_count;									// TPS ADC @ 5V
		USHORT flood_clear;									// flood clear TPS
	    USHORT bins[TPS_BINS];								// tps bins
	} tps;

	struct
	{
		USHORT lo_kpa;										// MAP kPa @ 0V
		USHORT hi_kpa;										// MAP kPa @ 5V
	} map;

	struct
	{
		// IAC
		USHORT enabled;
		USHORT range;										// IAC stepper range
		USHORT update_ms;									// IAC update rate
		USHORT cts_bins[IAC_BINS];							// CTS bins
		USHORT idle_rpm[IAC_BINS];							// target idle RPM vs CTS
		USHORT cranking[IAC_BINS];							// IAC vs CTS during cranking
		USHORT start[IAC_BINS];								// IAC position @ idle_tps
		USHORT on_throttle;									// IAC position @ 100.00% TPS
		USHORT minimum[IAC_BINS];

		USHORT hysterisis;									// target idle error before making any iac change
		USHORT rpm_roc;										// when > rpm roc / update_ms enter idle control
		USHORT idle_tps;									// maximum TPS to allow closed loop idle control
		USHORT delay; 										// closed loop idle delay after tps <= idle_tps
	} iac;

	struct
	{
		USHORT limits[CTS_BINS];							// hard limits vs CTS
		USHORT hysterisis;									// hard hysterisis
		USHORT soft_offset;									// rpm below hard limit to begin soft ignition retard
		SHORT soft_iar;										// initial retard @ rpm_limit - soft_offset
		SHORT hard_iar;										// retard @ hard limit
	} rpm_limiter;

	struct
	{
		USHORT enabled;
		USHORT update_ms;

		// requirements to be met for entering closed loop
		USHORT min_rpm;										// minimum rpm
		USHORT max_rpm;										// maximum rpm

		USHORT min_tps;										// minimum tps %
		USHORT max_tps;										// maximum tps %

		USHORT min_map;										// minimum map kPa
		USHORT max_map;										// maximum map kPa

		USHORT min_uptime;									// minimum ecu time (sensor warmup)
		USHORT min_cts_kelvin;								// minimum cts

		SHORT min_correction;								// max % enleanment
		SHORT max_correction;								// max % enrichment
		USHORT transient_delay;								// closed loop re-entrant delay after transient event

		SHORT proportional;
		SHORT integral;
		USHORT target[RPM_BINS][MAP_BINS];					// closed loop adc targets
	} ego;

	// APW * CTS * ROC -> 0 over cycles
	struct
	{
		USHORT cycles;
		struct c_AE_tps
		{
		    USHORT threshold;								// minimum rate of change before acceleration enrichment
		    USHORT bins[AE_TPS_BINS];						// tps bins
		} tps;

		struct c_AE_rpm
		{
		    USHORT bins[AE_RPM_BINS];						// rpm bins
		} rpm;

		struct c_AE_roc
		{
		    USHORT bins[AE_ROC_BINS];						// roc bins
		    USHORT factor[AE_ROC_BINS];						// rate of change vs PW factor
		} roc;

		struct c_AE_cts
		{
		    SHORT factor[CTS_BINS];							// cts vs PW factor
		} cts;

		// actual base acceleration pulse widths before corrections
        USHORT BPW[AE_RPM_BINS][AE_TPS_BINS];				// base pulse widths
	} AE;

	struct
	{
		USHORT start_map_rpm;
		USHORT full_map_rpm;
		USHORT BPW[RPM_BINS][TPS_BINS];
		USHORT BIAR[RPM_BINS][TPS_BINS];
	} alpha_N;
};

#endif
