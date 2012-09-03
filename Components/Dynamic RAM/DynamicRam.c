//
//  DynamicRam.c
//  Clock Signal
//
//  Created by Thomas Harte on 02/09/2012.
//  Copyright (c) 2012 acrossair. All rights reserved.
//

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

} CSDynamicRAM;

const char *dynamicRAMType = "dynamic RAM";

static void csDynamicRAM_dealloc(void *opaqueMemory)
{
	CSDynamicRAM *memory = (CSDynamicRAM *)opaqueMemory;

	if(memory->contents) free(memory->contents);
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
		memory->address = (memory->address & memory->casMask) | ((externalState.lineValues << memory->rasShift) & memory->rasMask);
	}

	// column select, maybe?
	if(!externalState.lineValues&CSComponentDynamicRAMSignalRAS)
	{
		// dump our existing column number and shift in the new value
		memory->address = (memory->address & memory->rasMask) | ((externalState.lineValues << memory->casShift) & memory->casMask);

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

		int activeLines = 0;
		switch(type)
		{
			case CSDynamicRAMType4116:
				// a 4116 holds 2kb of memory and supplies seven
				// address lines
				memory->contents = (uint8_t *)malloc(2048);
				activeLines = 7;
			break;
		}

		// we'll use CAS to set the low bits of our internal address
		// latch and RAS to set the high bits
		memory->casMask = (1 << activeLines) - 1;
		memory->casShift = 0;

		memory->rasMask = memory->casMask << activeLines;
		memory->rasShift = activeLines;

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
