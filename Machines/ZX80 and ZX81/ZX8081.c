//
//  ZX80ULA.c
//  LLZX80
//
//  Created by Thomas Harte on 18/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stdbool.h"

#include "ZX8081.h"
#include "CRT.h"
#include "Z80.h"
#include "TapePlayer.h"
#include "ReferenceCountedObject.h"
#include "BusNode.h"
#include "ClockGenerator.h"
#include "ZX8081MachineState.h"
#include "StaticMemory.h"
#include "FlatBus.h"

typedef struct LLZX80ULAState
{
	CSReferenceCountedObject referenceCountedObject;

	// the standard parts; a CPU + a bunch of ZX80 specific stuff
	void *CPU, *CRT, *tapePlayer;
	LLZX8081MachineState *machineState;

	// the internal state for the ZX80/81-specific components
	void *tapeTrapObserver;

	// current machine type
	LLZX8081MachineType machineType;
	LLZX8081RAMSize ramSize;
	uint8_t ROM[8192];
	size_t ROMSize;
	bool fastLoadingEnabled;

} LLZX80ULAState;

static int16_t llzx80ula_lookAheadForTapeByte(LLZX8081MachineState *machineState)
{
	unsigned int currentTime = csClockGenerator_getHalfCyclesToDate(machineState->clockGenerator);
	uint64_t tapeTime = cstapePlayer_getTapeTime(machineState->tapePlayer, currentTime);

	void *tape = cstapePlayer_getTape(machineState->tapePlayer);

	/*
	
		we're expecting, per bit:
		
			a 'long' pause (we'll accept anything more than 300us)
			a sequence of pulses — four for a 0, nine for a 1
			and repeat
	
	*/

	int bits = 8;
	uint8_t byte = 0;
	while(bits--)
	{
		uint64_t endTime;
		cstape_getLevelPeriodAroundTime(tape, tapeTime, NULL, &endTime);

		// check for initial pause
		// 1950 ticks = 300 us
		if(endTime - tapeTime < 1950)	break;
		tapeTime = endTime;

		// count acceptable pulses
		int numPulses = 0;
		while(1)
		{
			cstape_getLevelPeriodAroundTime(tape, tapeTime, NULL, &endTime);
			if((endTime - tapeTime) > 1462) break;
			numPulses++;
			tapeTime = endTime;
		}

		if(numPulses != 8 && numPulses != 18) break;
		byte = (uint8_t)((byte << 1) | ((numPulses == 8) ? 0 : 1));
	}

	if(bits != -1) return -1;

	cstapePlayer_setTapeTime(machineState->tapePlayer, currentTime, tapeTime);
	return byte;
}

static void llzx80ula_observeInstructionForTapeTrap(void *z80, void *context)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)context;

	uint16_t programCounter = (uint16_t)llz80_monitor_getInternalValue(z80, LLZ80MonitorValuePCRegister);

	// if the program counter is in the appropriate place for the ZX81
	// ROM and this looks like the ZX81 ROM, read a byte, deposit it
	// to (HL) and then move to the next bit of the ROM routines
	if(
		(programCounter == 0x037c) &&
		ula->ROM[0x037c] == 0xcd &&
		ula->ROM[0x037d] == 0x4c &&
		ula->ROM[0x037e] == 0x03 &&
		ula->ROM[0x037f] == 0x71)
	{
		// we don't have a tape anyway, dummy
		if(!cstapePlayer_getTape(ula->tapePlayer)) return;

		// CPU is about to call in-byte to load another byte;
		// decode next byte for ourselves and set PC to 0x380
		int16_t nextByte = llzx80ula_lookAheadForTapeByte(ula->machineState);

		// if we collected 8 bits without error then
		// write the thing out to HL and skip forward;
		// otherwise we don't know what's going on so
		// leave the ROM to it
		if(nextByte != -1)
		{
			uint16_t hlRegister = (uint16_t)llz80_monitor_getInternalValue(z80, LLZ80MonitorValueHLRegister);

			uint8_t byteOnly = (uint8_t)nextByte;
			csStaticMemory_setContents(ula->machineState->RAM, hlRegister - 16384, &byteOnly, 1);

			llz80_monitor_setInternalValue(z80, LLZ80MonitorValuePCRegister, 0x0380);
		}
	}

	// if this looks like a ZX80 ROM get byte, then do much the same thing
	if(
		(programCounter == 0x220) &&
		ula->ROM[0x220] == 0x1e &&
		ula->ROM[0x221] == 0x08 &&
		ula->ROM[0x222] == 0x3e &&
		ula->ROM[0x223] == 0x7f
	)
	{
		// we don't have a tape anyway, dummy
		if(!cstapePlayer_getTape(ula->tapePlayer)) return;

		// CPU is about to call in-byte to load another byte;
		// decode next byte for ourselves and set PC to 0x380
		int16_t nextByte = llzx80ula_lookAheadForTapeByte(ula->machineState);

		// if we collected 8 bits without error then
		// write the thing out to HL and skip forward;
		// otherwise we don't know what's going on so
		// leave the ROM to it
		if(nextByte != -1)
		{
			uint16_t hlRegister = (uint16_t)llz80_monitor_getInternalValue(z80, LLZ80MonitorValueHLRegister);

			uint8_t byteOnly = (uint8_t)nextByte;
			csStaticMemory_setContents(ula->machineState->RAM, hlRegister - 16384, &byteOnly, 1);

			llz80_monitor_setInternalValue(z80, LLZ80MonitorValuePCRegister, 0x0248);
		}
	}
}

static void llzx8081_destroy(void *opaqueULA)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	csObject_release(ula->CPU);
	csObject_release(ula->machineState->clockGenerator);
	csObject_release(ula->machineState);
	csObject_release(ula->CRT);
	csObject_release(ula->tapePlayer);
}

static void llzx80801_destroyMachine(LLZX80ULAState *ula)
{
	csObject_release(ula->machineState); ula->machineState = NULL;
	csObject_release(ula->CPU); ula->CPU = NULL;
	ula->tapeTrapObserver = NULL;
}

static void llzx80801_createMachine(LLZX80ULAState *ula)
{
	// build a bus containing all of our components
	void *bus = csFlatBus_create();

	// we'll attach to a Z80
	ula->CPU = llz80_createOnBus(bus);

	// we'll need the rest of the stuff of a ZX80 / 81
	ula->machineState =
		llzx8081_createMachineStateOnBus(bus, ula->machineType, ula->ramSize, ula->CRT, ula->tapePlayer);

	// check whether any of those failed to
	// be created successfully
/*	if(!ula->CPU || !ula->machineState)
	{
		csObject_release(ula->CPU);
		csObject_release(ula->machineState);
		free(ula);

		return NULL;
	}*/

	// get a tree from that
	ula->machineState->clockGenerator = csClockGenerator_createWithBus(bus, 3250000);
	csObject_release(bus);

	// install the current ROM
	csStaticMemory_setContents(ula->machineState->ROM, 0, ula->ROM, ula->ROMSize);
	
	// possibly install fast tape hack
	llzx8081_setFastLoadingIsEnabled(ula, ula->fastLoadingEnabled);
}

void *llzx8081_create(void)
{
	LLZX80ULAState *ula;

	ula = (LLZX80ULAState *)calloc(1, sizeof(LLZX80ULAState));

	if(ula)
	{
		// this is a reference counted object
		csObject_init(ula);
		ula->referenceCountedObject.dealloc = llzx8081_destroy;

		// set default: a 1kb ZX80
		ula->machineType = LLZX8081MachineTypeZX80;
		ula->ramSize = LLZX8081RAMSize16Kb;
		ula->CRT = llcrt_create(414, LLCRTInputTimingPAL, LLCRTDisplayTypeLuminance);
		ula->tapePlayer = cstapePlayer_create(6500000);
	}

	return ula;
}

void llzx8081_setFastLoadingIsEnabled(void *opaqueULA, bool isEnabled)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	ula->fastLoadingEnabled = isEnabled;
	if (!ula->machineState) return;

	// make sure we're not already in the requested state
	if(isEnabled && ula->tapeTrapObserver) return;
	if(!isEnabled && !ula->tapeTrapObserver) return;

	// set the requested state, by installing or
	// removing the fast tape instruction observer
	if(isEnabled)
	{
		ula->tapeTrapObserver = 
			llz80_monitor_addInstructionObserver(ula->CPU, llzx80ula_observeInstructionForTapeTrap, ula);
	}
	else
	{
		llz80_monitor_removeInstructionObserver(ula->CPU, ula->tapeTrapObserver);
		ula->tapeTrapObserver = NULL;
	}
}

void llzx8081_setMachineType(void *opaqueULA, LLZX8081MachineType type)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	
	if(ula->machineType != type)
	{
		ula->machineType = type;
		llzx80801_destroyMachine(ula);
	}
}

void llzx8081_provideROM(void *opaqueULA, const uint8_t *ROM, unsigned int length)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	memcpy(ula->ROM, ROM, length);
	ula->ROMSize = length;
	if(ula->machineState)
		csStaticMemory_setContents(ula->machineState->ROM, 0, ROM, length);
}

void llzx8081_setRAMSize(void *opaqueULA, LLZX8081RAMSize ramSize)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	if(ula->ramSize != ramSize)
	{
		ula->ramSize = ramSize;
		llzx80801_destroyMachine(ula);
	}
}

void llzx8081_runForHalfCycles(void *opaqueULA, unsigned int numberOfHalfCycles)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	
	// do we need to create a machine?
	if(!ula->machineState)
	{
		llzx80801_createMachine(ula);
	}

	// simple enough; have the Z80 run for that many cycles
	// (we'll respond to events as they arise through the
	// observer), then ensure the CRT is up-to-date on
	// the current output time
	csClockGenerator_runForHalfCycles(ula->machineState->clockGenerator, numberOfHalfCycles);

	unsigned int timeNow = csClockGenerator_getHalfCyclesToDate(ula->machineState->clockGenerator);
	llcrt_runToTime(ula->CRT, timeNow);
	cstapePlayer_runToTime(ula->tapePlayer, timeNow);
}

void *llzx8081_getCRT(void *opaqueULA)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	return ula->CRT;
}

void *llzx8081_getCPU(void *opaqueULA)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	return ula->CPU;
}

void llzx8081_setKeyDown(void *opaqueULA, LLZX8081VirtualKey key)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	int line = key >> 8;
	int mask = key & 0xff;
	ula->machineState->keyLines[line] &= ~mask;
}

void llzx8081_setKeyUp(void *opaqueULA, LLZX8081VirtualKey key)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;

	int line = key >> 8;
	int mask = key & 0xff;
	ula->machineState->keyLines[line] |= mask;
}

void llzx8081_setTape(void *opaqueULA, void *tape)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	cstapePlayer_setTape(
		ula->tapePlayer, 
		tape,
		llzx8081_getTimeStamp(ula));
}

void *llzx8081_getTapePlayer(void *ula)
{
	return ((LLZX80ULAState *)ula)->tapePlayer;
}

unsigned int llzx8081_getTimeStamp(void *opaqueULA)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)opaqueULA;
	return ula->machineState ? csClockGenerator_getHalfCyclesToDate(ula->machineState->clockGenerator) : 0;
}
