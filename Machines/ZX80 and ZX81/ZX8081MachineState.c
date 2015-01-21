//
//  ZX8081MachineState.c
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "ZX8081MachineState.h"
#include "CRT.h"
#include "TapePlayer.h"
#include "FlatBus.h"
#include "BusState.h"
#include "Z80.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "StaticMemory.h"
#include "StandardBusLines.h"

static void inline llzx80ula_considerSync(LLZX8081MachineState *const restrict machineState)
{
	// set the current output level on the CRT
	unsigned int currentTime = csFlatBus_getHalfCyclesToDate(machineState->bus);
	if(machineState->vsyncIsActive || machineState->hsyncIsActive)
		llcrt_setSyncLevel(machineState->CRT, currentTime);
	else
		llcrt_setLuminanceLevel(machineState->CRT, currentTime, 0xff);
}

static void inline llz80ula_setHsyncActive(LLZX8081MachineState *const restrict machineState)
{
	machineState->hsyncIsActive = true;
	if(!machineState->lastHSyncLevel)
		machineState->lineCounter++;
	machineState->lastHSyncLevel = true;
}

static void inline llz80ula_resetHsyncActive(LLZX8081MachineState *const restrict machineState)
{
	machineState->hsyncIsActive = machineState->lastHSyncLevel = false;
}

/*
	Memory map:

      0-8K     BASIC ROM.
      8-16K    Shadow of BASIC ROM. Can be disabled by 64K RAM pack.
      16K-17K  Area occupied by 1K of onboard RAM. Disabled by RAM packs.
      16K-32K  Area occupied by 16K RAM pack.
      8K-64K   Area occupied by 64K RAM pack.

*/
csComponent_observer(llzx80ula_observeRefresh)
{
	// if this is a memory refresh cycle then make a note
	// of the address and echo bit 6 to the INT line,
	// recalling that it's active low

	if(conditionIsTrue)
	{
		// set interrupt request according to bit 6 of the refresh address
		internalState->lineValues =
			(internalState->lineValues &~ LLZ80SignalInterruptRequest) |
			(
				((externalState.lineValues >> CSBusStandardAddressShift) & 0x40) ?
					LLZ80SignalInterruptRequest : 0
			);
	}
	else
	{
		// interrupt is held only during the refresh cycle
		internalState->lineValues |= LLZ80SignalInterruptRequest;

		LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;

		// grab a video byte if we've decided to output video this refresh cycle
		if(machineState->fetchVideoByte)
		{
			machineState->fetchVideoByte = false;
			uint8_t videoByte;

			// if so, would the ROM actually serve this address?
			videoByte = (uint8_t)(externalState.lineValues >> CSBusStandardDataShift);

			// this byte might be intended to be inverted
			videoByte ^= machineState->videoByteXorMask;

			// and push it out to the CRT
			llcrt_output1BitLuminanceByte(
				machineState->CRT,
				csFlatBus_getHalfCyclesToDate(machineState->bus),
				videoByte);
		}
	}
}

csComponent_observer(llzx80ula_observeIORead)
{
	// an IO read request
	if(conditionIsTrue)
	{
		LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;
		uint16_t address = (uint16_t)(externalState.lineValues >> CSBusStandardAddressShift);

		switch(address&7)
		{
			default: break;

			case 6:
			{
				// start vertical sync 
				if(!machineState->nmiIsEnabled)
				{
					machineState->vsyncIsActive = true;
					llzx80ula_considerSync(machineState);
				}

				// do a keyboard read
				uint8_t result = 0x7f;
				if(!(address&0x0100)) result &= machineState->keyLines[0];
				if(!(address&0x0200)) result &= machineState->keyLines[1];
				if(!(address&0x0400)) result &= machineState->keyLines[2];
				if(!(address&0x0800)) result &= machineState->keyLines[3];
				if(!(address&0x1000)) result &= machineState->keyLines[4];
				if(!(address&0x2000)) result &= machineState->keyLines[5];
				if(!(address&0x4000)) result &= machineState->keyLines[6];
				if(!(address&0x8000)) result &= machineState->keyLines[7];

				// read the tape input too, if there is any
				void *tape = cstapePlayer_getTape(machineState->tapePlayer);
				if(tape)
				{
					unsigned int currentTime = csFlatBus_getHalfCyclesToDate(machineState->bus);
					uint64_t tapeTime = cstapePlayer_getTapeTime(machineState->tapePlayer, currentTime);

					if(cstape_getLevelAtTime(tape, tapeTime) == CSTapeLevelLow)
						result |= 0x80;
				}

				// and load the result
				internalState->lineValues &= ((uint64_t)result << CSBusStandardDataShift) | ~CSBusStandardDataMask;
			}
			break;
		}
	}
	else
	{
		internalState->lineValues |= CSBusStandardDataMask;
	}
}

csComponent_observer(llzx80ula_observeIOWrite)
{
	// an IO write request ...
	LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;
	uint16_t address = (uint16_t)(externalState.lineValues >> CSBusStandardAddressShift);

	// determine whether activate or deactive the NMI generator;
	// this is relevant to the ZX81 only, and since NMI enabled
	// prevents output of 'vertical' sync, it would adversely
	// affect ZX80 emulation if we went ahead regardless
	if(!(address&1))
	{
		machineState->nmiIsEnabled = (machineState->machineType == LLZX8081MachineTypeZX81);
	}

	if(!(address&2))
	{
		machineState->nmiIsEnabled = false;
	}

	if(!(address&4))
	{
	}

	if((address&7) == 7)
	{
		if(!machineState->nmiIsEnabled)
		{
			machineState->lineCounter = 0;
			machineState->vsyncIsActive = false;
			llzx80ula_considerSync(machineState);
		}
	}

//	if(!(address&0x80))
//	{
//		printf("AY index? %02x", address);
//	}
//	
//	if(!(address&0x80))
//	{
//		printf("AY data? %02x", address);
//	}
}

csComponent_observer(llzx80ula_observeIntAck)
{
	// This triggers on IO request + M1 active, i.e. interrupt acknowledge;
	// we need to arrange for horizontal sync to occur; the ZX81 has a
	// well-documented counter for the purpose and we'll need to count M1
	// cycles on a ZX80 so the action is the same either way
	((LLZX8081MachineState *)context)->hsyncCounter = 0;
}


// This one is hooked up for the ZX80 only
csComponent_observer(llzx80ula_observeMachineCycleOne)
{
	// M1 cycles clock the horizontal sync generator, but
	// only when we're in an hsync cycle
	LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;
	if(machineState->hsyncCounter < 4)
	{
		if(machineState->hsyncCounter == 1)
		{
			llz80ula_setHsyncActive(machineState);
			llzx80ula_considerSync(machineState);
		}

		if(machineState->hsyncCounter == 3)
		{
			llz80ula_resetHsyncActive(machineState);
			llzx80ula_considerSync(machineState);
		}

		machineState->hsyncCounter++;
	}
}

// This one is hooked up for the ZX81 only
csComponent_observer(llzx80ula_observeClock)
{
	LLZX8081MachineState *const restrict machineState = (LLZX8081MachineState *const)context;

	// increment the hsync counter, check whether sync output is
	// currently active as a result
	machineState->hsyncCounter++;
	if(machineState->hsyncCounter == 207) machineState->hsyncCounter = 0;

	if( (machineState->hsyncCounter >= 16) && (machineState->hsyncCounter < 32) )
		llz80ula_setHsyncActive(machineState);
	else
		llz80ula_resetHsyncActive(machineState);

	if(machineState->nmiIsEnabled && machineState->hsyncIsActive)
	{
		internalState->lineValues &= ~ LLZ80SignalNonMaskableInterruptRequest;

		if(externalState.lineValues & LLZ80SignalHalt)
			internalState->lineValues &= ~ LLZ80SignalWait;
		else
			internalState->lineValues |= LLZ80SignalWait;
	}
	else
	{
		internalState->lineValues |= LLZ80SignalWait | LLZ80SignalNonMaskableInterruptRequest;
	}

	// determine what to do about sync level output as a result
	llzx80ula_considerSync(machineState);
}

csComponent_observer(llzx80ula_observeVideoRead)
{
	// condition: data&0x40 is low, m1 is low, address&0x8000 is high
	if(conditionIsTrue)
	{
		LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;

		uint8_t value = (uint8_t)(externalState.lineValues >> CSBusStandardDataShift);

		machineState->videoFetchAddress = 
			(uint16_t)((value&0x3f) << 3) |
			(machineState->lineCounter&7);	// 9 bits of address, to substitute onto the low 9 address lines
											// in front of the ROM, for the duration of fetchVideoByte being true

		machineState->fetchVideoByte = true;
		machineState->videoByteXorMask = (value&0x80) ? 0x00 : 0xff;

		// force a NOP onto the data bus
		internalState->lineValues &= ~CSBusStandardDataMask;
	}
	else
	{
		internalState->lineValues |= CSBusStandardDataMask;
	}
}

static CSBusState llzx80ula_romAddressShuffle(void *const context, CSBusState externalState)
{
	const LLZX8081MachineState *const machineState = (LLZX8081MachineState *const)context;
	if(machineState->fetchVideoByte)
	{
		// we replace the low 9 bits of the address, so...
		externalState.lineValues =
			(externalState.lineValues & ~(511u << CSBusStandardAddressShift)) |
			(uint64_t)(machineState->videoFetchAddress << CSBusStandardAddressShift);
	}
	
	return externalState;
}

static void llzx8081_destroyMachineState(void *opaqueMachineState)
{
	LLZX8081MachineState *const machineState = (LLZX8081MachineState *const )opaqueMachineState;
	csObject_release(machineState->tapePlayer);
	csObject_release(machineState->CRT);
	csObject_release(machineState->ROM);
	csObject_release(machineState->RAM);
}

LLZX8081MachineState *llzx8081_createMachineStateOnBus(void *bus, LLZX8081MachineType machineType, LLZX8081RAMSize ramSize, void *CRT, void *tapePlayer)
{
	LLZX8081MachineState *machineState = (LLZX8081MachineState *)calloc(1, sizeof(LLZX8081MachineState));

	if(machineState)
	{
		csObject_init(machineState);
		machineState->referenceCountedObject.dealloc = llzx8081_destroyMachineState;

		// create a CRT
		machineState->CRT = csObject_retain(CRT);

		// create a tape player
		machineState->tapePlayer = csObject_retain(tapePlayer);

		// we can handle ROMs up to an amazing
		// 8kb in size! ROM is triggered on
		// the top two address lines being zero,
		// memory request being active (ie, low) and
		// write being inactive (ie, high)
		if(ramSize != LLZX8081RAMSize64Kb)
		{
			// responds when the top two bits of the address bus are clear,
			// i.e. occupies the lowest 16kb of address space
			machineState->ROM = csStaticMemory_createOnBus(
				bus, 8192,
				csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0xc000 << CSBusStandardAddressShift), LLZ80SignalWrite, false),
				csBus_impossibleCondition() 
				);
		}
		else
		{
			// responds when the top three bits of the address bus are clear,
			// i.e. occupies the lowest 8kb of address space
			machineState->ROM = csStaticMemory_createOnBus(
				bus, 8192,
				csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0xe000 << CSBusStandardAddressShift), LLZ80SignalWrite, false),
				csBus_impossibleCondition() 
				);
		}
//		csComponent_setPreFilter(machineState->ROM, llzx80ula_romAddressShuffle, machineState);

		// RAM can be up to 64kb! That's more than
		// anybody could or would ever want

		switch(ramSize)
		{
			case LLZX8081RAMSize1Kb:
				// RAM is active between 16384 and +1kb,
				// and mirrored 32kb higher
				// so that's bits 10 to 13 clear and
				// bit 14 set
				machineState->RAM = csStaticMemory_createOnBus(
					bus, 1024,
					csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0x7c00 << CSBusStandardAddressShift), LLZ80SignalWrite | (0x4000 << CSBusStandardAddressShift), false),
					csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0x7c00 << CSBusStandardAddressShift), (0x4000 << CSBusStandardAddressShift), false)
					);
			break;

			default:
			case LLZX8081RAMSize16Kb:
				// RAM is active between 16384 and 32768,
				// and mirrored 32kb higher
				// so that's bit 14 set
				machineState->RAM = csStaticMemory_createOnBus(
					bus, 16384,
					csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0x4000 << CSBusStandardAddressShift), LLZ80SignalWrite | (0x4000 << CSBusStandardAddressShift), false),
					csBus_testCondition(LLZ80SignalMemoryRequest | LLZ80SignalWrite | (0x4000 << CSBusStandardAddressShift), (0x4000 << CSBusStandardAddressShift), false)
					);
			break;
		}

		if(!machineState->CRT || !machineState->tapePlayer || !machineState->ROM || !machineState->RAM)
		{
			csObject_release(machineState->CRT);
			csObject_release(machineState->tapePlayer);
			csObject_release(machineState->ROM);
			csObject_release(machineState->RAM);
			free(machineState);
			return NULL;
		}

		// set no keys currently pressed
		memset(machineState->keyLines, 0xff, 8);

		// add machine emulation components to the bus
		csFlatBus_createComponent(
			bus,
			llzx80ula_observeIntAck,
			csBus_resetCondition(LLZ80SignalMachineCycleOne | LLZ80SignalInputOutputRequest, true), 
			0,
			machineState);

		csFlatBus_createComponent(
			bus,
			llzx80ula_observeRefresh,
			csBus_resetCondition(LLZ80SignalRefresh, false), 
			LLZ80SignalInterruptRequest,
			machineState);

		csFlatBus_createComponent(
			bus,
			llzx80ula_observeVideoRead,
			csBus_testCondition(
				(0x8000 << CSBusStandardAddressShift) | LLZ80SignalMachineCycleOne | (0x40 << CSBusStandardDataShift) | LLZ80SignalHalt,
				(0x8000 << CSBusStandardAddressShift) | LLZ80SignalHalt,
				false),
			CSBusStandardDataMask,
			machineState);

		csFlatBus_createComponent(
			bus,
			llzx80ula_observeIORead,
			csBus_resetCondition(LLZ80SignalInputOutputRequest | LLZ80SignalRead, false), 
			CSBusStandardDataMask,
			machineState);

		csFlatBus_createComponent(
			bus,
			llzx80ula_observeIOWrite,
			csBus_resetCondition(LLZ80SignalInputOutputRequest | LLZ80SignalWrite, true), 
			0,
			machineState);

		if(machineType == LLZX8081MachineTypeZX80)
		{
			// add M1 observer to get a ZX80
			csFlatBus_createComponent(
				bus,
				llzx80ula_observeMachineCycleOne,
				csBus_resetCondition(LLZ80SignalMachineCycleOne, true), 
				0,
				machineState);
		}
		else
		{
			// add clock observer to get a ZX81
			csFlatBus_createComponent(
				bus,
				llzx80ula_observeClock,
				csBus_resetCondition(CSBusStandardClockLine, true), 
				LLZ80SignalNonMaskableInterruptRequest | LLZ80SignalWait,
				machineState);
		}
		machineState->machineType = machineType;
	}

	return machineState;
}
