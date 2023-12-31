//***************************  ><(((*>   <*)))><  ******************************
//*                                                                            *
//*                          D A T U M    T E C H                              *
//*                            www.datumtech.com                               *
//*                                                                            *
//***************************  ><(((*>   <*)))><  ******************************
//*
//*
//*
//***************************  ><(((*>   <*)))><  ******************************
//=========================================================
// src/EFM8BB10F8G_MeirProducts_DrvWayMonitor_main.c: generated by Hardware Configurator
//
// This file will be updated when saving a document.
// leave the sections inside the "$[...]" comment tags alone
// or they will be overwritten!!
//=========================================================

//-----------------------------------------------------------------------------
// EFM8BB10F8G_MeirProducts_DrvWayMonitor_main.c
//-----------------------------------------------------------------------------
// Lake Road Engineering
// Rich Lysaght
// 11/25/2020
//
// Produced for Mier Products
// Description:
// This is a driveway monitoring receiver that generates tones for a chime system
// and controls some LEDs and relays
//
// Target:         EFM8BB1
// Tool chain:     Generic
//
// Version 2.0 of the chime products
//    - Initial Release
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB1_Register_Enums.h>
#include "InitDevice.h"
#include "flash.h"
#include "relays.h"
#include "timers.h"
#include "mute.h"
#include "sound.h"
#include "general.h"
#include "input.h"
#include "options.h"

#define RELAY_TIME 15

typedef enum
{
  ZONE_STAT_READY,
  ZONE_STAT_WAIT_FOR_LOW,
  ZONE_STAT_WAIT_FOR_FINISH
}zoneStat_t;

typedef enum
{
  SOUND_STAT_IDLE,
  SOUND_STAT_ARMED,
  SOUND_STAT_FIRED,
  SOUND_STAT_COMPLETE
}soundStat_t;

typedef enum
{
	MAIN_POWERUP,
	MAIN_DETERMINE_CONNECTION,
	MAIN_WIRELESS,
	MAIN_WIRED
}mainState_t;

typedef enum
{
	NOT_TRIGGERED,
	TRIGGERED
}triggered_t;

typedef enum
{
	ALL_DETECT_DISC,
	WIRED_Z1_DISC,
	WIRED_Z2_DISC,
	NUM_DISC
}discriminatorNames_t;

typedef struct
{
	soundMode_t soundMode;
	inputNames_t input;
	relayNames_t output;
	soundStat_t soundStatus;
	zoneStat_t zoneStatus;
} zone_t;

typedef struct
{
	triggered_t trigger;
	inputNames_t input;
	mainState_t target;
} discriminator_t;

void initDiscriminators(void);
void initZones(void);
void wirelessOp(void);
void wiredOp(void);
void zoneRelays(zoneNames_t zone);
void sounds(void);
void lowBattCheck(void);
void programCheck(void);

zone_t zones[ZONE_NUM];
discriminator_t discriminator[NUM_DISC];

SI_SBIT(LoBattLED, SFR_P0, 5);

//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup (void)
{
  // $[SiLabs Startup]
  // [SiLabs Startup]$
}

//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
int main(void)
{
	//zoneNames_t zone;
	mainState_t mainState = MAIN_POWERUP;
	uint8_t i;

	initHardware();

	//run initialization routines
	inputInit();
	relayInit();
	timerInit();
	setSoundMode(SOUND_MODE_IDLE);
	initZones();
	initDiscriminators();

	//start 1ms timer
	TMR2CN0 |= TMR2CN0_TR2__RUN;

	//If eeprom read fails, write default values
	if (eeprom_read_all() != 0)
	{
		setMuteMode(MUTE_DISABLE);
		setGenMode(GEN_DISABLE);
		eeprom_write_all();
	}

	startTimer(TIMER_GEN, 2);

	for (;;)
	{
		switch (mainState)
		{
		case MAIN_POWERUP:
			if (getTimerStat(TIMER_GEN) == TIMER_DONE)
			{
				mainState = MAIN_DETERMINE_CONNECTION;
			}
			break;

		case MAIN_DETERMINE_CONNECTION:
			for(i=0;i<NUM_DISC;i++)
			{
				if((getInput(discriminator[i].input) == INPUT_LOW)||discriminator[i].trigger == TRIGGERED)
				{
					discriminator[i].trigger = TRIGGERED;
					if(getInput(discriminator[i].input) == INPUT_HIGH)
					{
						mainState = discriminator[i].target;
					}
				}
			}

			programCheck();

			break;

		case MAIN_WIRELESS:
			wirelessOp();
			break;

		case MAIN_WIRED:
			zones[ZONE1].input = INPUT_WIRED_ZONE1;
			zones[ZONE2].input = INPUT_WIRED_ZONE2;
			wiredOp();
			break;
		}
	}
}

void initDiscriminators(void)
{
	discriminator[ALL_DETECT_DISC].trigger = NOT_TRIGGERED;
	discriminator[ALL_DETECT_DISC].input = INPUT_DETECT;
	discriminator[ALL_DETECT_DISC].target = MAIN_WIRELESS;

	discriminator[WIRED_Z1_DISC].trigger = NOT_TRIGGERED;
	discriminator[WIRED_Z1_DISC].input = INPUT_WIRED_ZONE1;
	discriminator[WIRED_Z1_DISC].target = MAIN_WIRED;

	discriminator[WIRED_Z2_DISC].trigger = NOT_TRIGGERED;
	discriminator[WIRED_Z2_DISC].input = INPUT_WIRED_ZONE2;
	discriminator[WIRED_Z2_DISC].target = MAIN_WIRED;
}

void initZones(void)
{

  //zones[ZONE1].input = INPUT_WIRED_ZONE1;
  zones[ZONE1].input = INPUT_WIRELESS_ZONE1;
  zones[ZONE1].output = RELAY_ZONE1;
  zones[ZONE1].soundMode = SOUND_MODE_DING_DONG;
  zones[ZONE1].soundStatus = SOUND_STAT_COMPLETE;
  zones[ZONE1].zoneStatus = ZONE_STAT_READY;

  //zones[ZONE2].input = INPUT_WIRED_ZONE2;
  zones[ZONE2].input = INPUT_WIRELESS_ZONE2;
  zones[ZONE2].output = RELAY_ZONE2;
  zones[ZONE2].soundMode = SOUND_MODE_DING_DING_DING;
  zones[ZONE2].soundStatus = SOUND_STAT_COMPLETE;
  zones[ZONE2].zoneStatus = ZONE_STAT_READY;

  zones[ZONE3].input = INPUT_WIRELESS_ZONE3;
  zones[ZONE3].output = RELAY_ZONE3;
  zones[ZONE3].soundMode = SOUND_MODE_DONG_DING_DONG;
  zones[ZONE3].soundStatus = SOUND_STAT_COMPLETE;
  zones[ZONE3].zoneStatus = ZONE_STAT_READY;

  zones[MANUAL].input = INPUT_DETECT;
  zones[MANUAL].output = RELAY_ALL_DET;
  zones[MANUAL].soundMode = SOUND_MODE_DING_DONG;
  zones[MANUAL].soundStatus = SOUND_STAT_COMPLETE;
  zones[MANUAL].zoneStatus = ZONE_STAT_READY;

  zones[LOBATT].soundMode = SOUND_MODE_DONG;
  zones[LOBATT].soundStatus = SOUND_STAT_COMPLETE;
}

void wirelessOp(void)
{
	zoneNames_t zone;

	for (;;)
	{
		//check for relay activation
		switch (getZoneMode())
		{
		case ZONE_MODE_AUTO:
			for (zone = ZONE1; zone < MANUAL; zone++)
			{
				zoneRelays(zone);
			}
			break;

		case ZONE_MODE_MANUAL:
			for (zone = ZONE1; zone < MANUAL; zone++)
			{
				if (getInput(zones[zone].input) == INPUT_HIGH)
				{
					zones[MANUAL].soundMode = zones[zone].soundMode;
				}
			}
			zoneRelays(MANUAL);
			break;
		}

		//sounds();

		lowBattCheck();

		programCheck();
	}
}

void wiredOp(void)
{
	for (;;)
	{
		zoneRelays(ZONE1);
		zoneRelays(ZONE2);
		sounds();
		programCheck();
	}
}

void zoneRelays(zoneNames_t zone)
{
	switch (zones[zone].zoneStatus)
	{
	case ZONE_STAT_READY:
		if (getInput(zones[zone].input) == INPUT_HIGH)
		{
			#ifndef RETRIGGER
			zones[zone].zoneStatus = ZONE_STAT_WAIT_FOR_LOW;
			#else
			zones[zone].zoneStatus = ZONE_STAT_WAIT_FOR_FINISH;
			#endif
			zones[zone].soundStatus = SOUND_STAT_ARMED;
			setRelayTime(zones[zone].output, RELAY_TIME);
			setRelayTime(RELAY_TMR, (10 * getGenTime()));
			if (zone != MANUAL)
			{
				setRelayTime(RELAY_ALL_DET, RELAY_TIME);
			}
		}
		break;

	case ZONE_STAT_WAIT_FOR_LOW:
		if (getInput(zones[zone].input) == INPUT_LOW)
		{
			zones[zone].zoneStatus = ZONE_STAT_WAIT_FOR_FINISH;
		}
		break;

	case ZONE_STAT_WAIT_FOR_FINISH:
		if (getRelayState(zones[zone].output) == STATE_OFF
				&& zones[zone].soundStatus == SOUND_STAT_COMPLETE)
		{
			zones[zone].zoneStatus = ZONE_STAT_READY;
		}
		break;
	}
}

void sounds(void)
{
	static zoneNames_t zone = ZONE1;

	switch (zones[zone].soundStatus)
	{
	case SOUND_STAT_IDLE:
	case SOUND_STAT_COMPLETE:
		if ((++zone) >= ZONE_NUM)
		{
			zone = ZONE1;
		}
		break;

	case SOUND_STAT_ARMED:
		setSoundMode(zones[zone].soundMode);
		zones[zone].soundStatus = SOUND_STAT_FIRED;
		break;

	case SOUND_STAT_FIRED:
		if (getSoundMode() == SOUND_MODE_IDLE)
		{
			zones[zone].soundStatus = SOUND_STAT_COMPLETE;
		}
		break;
	}
}

void lowBattCheck(void)
{
	static triggered_t trigger = TRIGGERED;

	switch (trigger)
	{
	case TRIGGERED:
		if (getInput(INPUT_DETECT) == INPUT_HIGH)
		{
			if (getInput(INPUT_LO_BATT) == INPUT_HIGH)
			{
				LoBattLED = 1;
				switch(LoBattLED)
				{
					case SOUND_STAT_IDLE:
					case SOUND_STAT_COMPLETE:
					case SOUND_STAT_ARMED:
						setSoundMode(LOBATT.soundMode);
						LOBATT.soundStatus = SOUND_STAT_FIRED;
						break;
					case SOUND_STAT_FIRED:
						if (getSoundMode() == SOUND_MODE_IDLE)
						{
							LOBATT.soundStatus = SOUND_STAT_COMPLETE;
						}
						break;
				}
			}
			else
			{
				LoBattLED = 0;
			}
			trigger = NOT_TRIGGERED;
		}
		break;
	case NOT_TRIGGERED:
		if (getInput(INPUT_DETECT) == INPUT_LOW)
		{
			trigger = TRIGGERED;
		}
		break;
	}
}

void programCheck(void)
{
	typedef enum
	{
		PROGRAM_MUTE,
		PROGRAM_GEN
	} programCheck_t;

	static programCheck_t programCheck = PROGRAM_MUTE;

	switch (programCheck)
	{
	case PROGRAM_MUTE:
		if (muteMonitor() == MUTE_STAT_IDLE)
		{
			programCheck = PROGRAM_GEN;
		}
		break;

	case PROGRAM_GEN:
		if (genMonitor() == GEN_STAT_IDLE)
		{
			programCheck = PROGRAM_MUTE;
		}
		break;
	}
}


