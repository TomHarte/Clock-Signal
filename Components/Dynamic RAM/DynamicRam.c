//
//  DynamicRam.c
//  Clock Signal
//
//  Created by Thomas Harte on 02/09/2012.
//  Copyright (c) 2012 acrossair. All rights reserved.
//

/*

	Logic for dynamic RAM is:

		falling RAS edge => load the new row address and refresh the row
		falling CAS edge => load the new column and do the read or write
		falling write edge => do the write

		data out is in high impedance whenever CAS is high

	and that's all subject to chip enable, naturally

*/

#include "DynamicRAM.h"
#include "ReferenceCountedObject.h"
#include <stdlib.h>

#include "BusState.h"
#include "BusNode.h"
#include "Component.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	uint8_t *contents;
	uint64_t rasMask, rasShift;
	uint64_t casMask, casShift;
	uint16_t address;

	CSComponentNanoseconds *lastRefreshTimes;
	CSComponentNanoseconds refreshPeriod;

} CSDynamicRAM;

const char *dynamicRAMType = "dynamic RAM";

static void csDynamicRAM_dealloc(void *opaqueMemory)
{
	CSDynamicRAM *memory = (CSDynamicRAM *)opaqueMemory;

	if(memory->contents) free(memory->contents);
	if(memory->lastRefreshTimes) free(memory->lastRefreshTimes);
}

static void csDynamicRAM_observeStrobes(void *opaqueMemory, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue, CSComponentNanoseconds timeSinceLaunch)
{
	CSDynamicRAM *memory = (CSDynamicRAM *)opaqueMemory;

	// is this chip even enabled?
	if(externalState.lineValues&CSComponentDynamicRAMSignalChipEnable)
	{
//		internalState->lineValues |= CSBusStandardDataMask;
		return;
	}

	// is this a row select?
	if(!externalState.lineValues&CSComponentDynamicRAMSignalRAS)
	{
		// TODO: this should refresh this row

		// dump our existing row number and shift in the new value
		memory->address = (uint16_t)((memory->address & memory->casMask) | ((externalState.lineValues << memory->rasShift) & memory->rasMask));
	}

	// column select, maybe?
	if(!externalState.lineValues&CSComponentDynamicRAMSignalRAS)
	{
		// dump our existing column number and shift in the new value
		memory->address = (uint16_t)((memory->address & memory->rasMask) | ((externalState.lineValues << memory->casShift) & memory->casMask));

		// do an input or output
		if(!externalState.lineValues&CSComponentDynamicRAMSignalWrite)
		{
		}
		else
		{
			// TODO: allow for potential capacitor leak to return random
			// values if this ram hasn't been refreshed appropriately
		}
	}
}

void *csDynamicRAM_createOnBus(void *bus, CSDynamicRAMType type)
{
	CSDynamicRAM *memory = (CSDynamicRAM *)calloc(1, sizeof(CSDynamicRAM));

	if(memory)
	{
		// set up as a standard reference counted object
		csObject_init(memory);
		memory->referenceCountedObject.dealloc = csDynamicRAM_dealloc;
		memory->referenceCountedObject.type = dynamicRAMType;

		unsigned int activeLines = 0;
		unsigned int bitsPerAddress = 0;
		switch(type)
		{
			case CSDynamicRAMType4116:
				// a 4116 supplies seven address lines, stores 1 bit at each address and has a 2 ms refresh period
				activeLines = 7;
				bitsPerAddress = 1;
				memory->refreshPeriod = 2000;
			break;

			case CSDynamicRAMType4164:
				// a 4164 supplies eight address lines, stores 1 bit at each address and has a 4 ms refresh period
				activeLines = 8;
				bitsPerAddress = 1;
				memory->refreshPeriod = 4000;
			break;

			case CSDynamicRAMType2164:
				// a 4164 supplies eight address lines, stores 1 bit at each address and has a 2 ms refresh period
				activeLines = 8;
				bitsPerAddress = 1;
				memory->refreshPeriod = 2000;
			break;

			case CSDynamicRAMType41464:
				// a 4164 supplies eight address lines, stores 4 bits at each address and has a 4 ms refresh period
				activeLines = 8;
				bitsPerAddress = 4;
				memory->refreshPeriod = 4000;
			break;

			default: break;
		}

		// allocate the memory we'll need for storage
		memory->contents = (uint8_t *)malloc((size_t)(((1 << (activeLines + activeLines)) * bitsPerAddress) >> 3));

		// we'll use CAS to set the low bits of our internal address
		// latch and RAS to set the high bits
		memory->casMask = (1 << activeLines) - 1;
		memory->casShift = 0;

		memory->rasMask = memory->casMask << activeLines;
		memory->rasShift = activeLines;

		// this is dynamic memory, so we'll keep track of refresh times and
		// corrupt cells that aren't properly updated
		memory->lastRefreshTimes = (CSComponentNanoseconds *)calloc(memory->casMask, sizeof(CSComponentNanoseconds));

		// component to handle the strobes
		void *component = 
			csComponent_create(
				csDynamicRAM_observeStrobes,
				csBus_resetCondition(
					CSComponentDynamicRAMSignalRAS |
					CSComponentDynamicRAMSignalCAS, false),
				CSComponentDynamicRAMSignalDataInput | CSComponentDynamicRAMSignalDataOutput,
				memory);

		csBusNode_addComponent(bus, component);
		csObject_release(component);
	}

	return memory;
}
