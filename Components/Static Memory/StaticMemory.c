//
//  StaticMemory.c
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "StaticMemory.h"
#include <stdlib.h>
#include <string.h>
#include "ReferenceCountedObject.h"
#include "BusState.h"
#include "StandardBusLines.h"
#include "BusNode.h"
#include "Component.h"
#include <stdio.h>

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	uint8_t *contents;
	unsigned int sizeMinusOne;

} CSStaticMemory;

const char *staticMemoryType = "static memory";

csComponent_observer(csStaticMemory_observeMemoryRead)
{
	if(conditionIsTrue)
	{
		// output something...

		// fetch the address from the bus
		unsigned int address = (externalState.lineValues&CSBusStandardAddressMask) >> CSBusStandardAddressShift;

		// reduce that down to an address in our range
		const CSStaticMemory *const memory = (CSStaticMemory *const)context;
		address &= memory->sizeMinusOne;

//		if(externalState.lineValues&0x200000000000)
//			printf("r %04x\n", address);
//		else
//			printf("f %04x\n", address);

		// and load the data lines
		internalState->lineValues &= ((uint64_t)memory->contents[address] << CSBusStandardDataShift) | ~CSBusStandardDataMask;
	}
	else
	{
		// stop outputting anything whatsoever
		internalState->lineValues |= CSBusStandardDataMask;
//		printf("r-\n");
	}
}

csComponent_observer(csStaticMemory_observeMemoryWrite)
{
	if(!conditionIsTrue)
	{
		// store the incoming value...

		// fetch the address from the bus
		unsigned int address = (externalState.lineValues&CSBusStandardAddressMask) >> CSBusStandardAddressShift;

		// reduce that down to an address in our range
		const CSStaticMemory *const memory = (CSStaticMemory *const)context;
		address &= memory->sizeMinusOne;

		// and load the data lines
		memory->contents[address] = (uint8_t)(externalState.lineValues >> CSBusStandardDataShift);
	}
}

static void csStaticMemory_dealloc(void *opaqueMemory)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	if(memory->contents) free(memory->contents);
}

void csStaticMemory_setContents(void *opaqueMemory, unsigned int dest, const uint8_t *source, size_t length)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	// do some trivial bounds checking and adjustment
	if(dest > memory->sizeMinusOne + 1) return;
	if(dest + length > memory->sizeMinusOne + 1) length = memory->sizeMinusOne + 1 - dest;

	// copy
	memcpy(&memory->contents[dest], source, length);
}

void csStaticMemory_getContents(void *opaqueMemory, uint8_t *dest, unsigned int source, size_t length)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	// do some trivial bounds checking and adjustment
	if(source > memory->sizeMinusOne + 1) return;
	if(source + length > memory->sizeMinusOne + 1) length = memory->sizeMinusOne + 1 - source;

	// copy
	memcpy(dest, &memory->contents[source], length);
}

void *csStaticMemory_createOnBus(void *bus, unsigned int size, CSBusCondition readCondition, CSBusCondition writeCondition)
{
	CSStaticMemory *memory = (CSStaticMemory *)calloc(1, sizeof(CSStaticMemory));

	if(memory)
	{
		// set up as a standard reference counted object
		csObject_init(memory);
		memory->referenceCountedObject.dealloc = csStaticMemory_dealloc;
		memory->referenceCountedObject.type = staticMemoryType;

		// allocate space and store the length
		memory->contents = (uint8_t *)malloc(size);
		memory->sizeMinusOne = size-1;

		// if this is readonly, add just the read component;
		// otherwise create a read/write node
		void *readComponent = csComponent_create(
			csStaticMemory_observeMemoryRead,
			readCondition,
			CSBusStandardDataMask,
			memory
		);
		csBusNode_addComponent(bus, readComponent);
		csObject_release(readComponent);

		// don't bother adding the write observer if the
		// condition is impossible to satisfy
		if(!csBusCondition_isImpossible(writeCondition))
		{
			void *writeComponent = csComponent_create(
				csStaticMemory_observeMemoryWrite,
				writeCondition,
				CSBusStandardDataMask,
				memory);
			csBusNode_addComponent(bus, writeComponent);
			csObject_release(writeComponent);
		}
	}

	return memory;
}
