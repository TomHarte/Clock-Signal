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
	unsigned int size;

} CSStaticMemory;

const char *staticMemoryType = "static memory";

static void csStaticMemory_observeMemoryRead(void *opaqueMemory, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue)
{
	if(conditionIsTrue)
	{
		// output something...

		// fetch the address from the bus
		unsigned int address = (externalState.lineValues&CSBusStandardAddressMask) >> CSBusStandardAddressShift;

		// reduce that down to an address in our range
		CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;
		address &= (memory->size - 1);

//		if(externalState.lineValues&0x200000000000)
//			printf("r %04x\n", address);
//		else
//			printf("f %04x\n", address);

		// and load the data lines
		internalState->lineValues &= (memory->contents[address] << CSBusStandardDataShift) | ~CSBusStandardDataMask;
	}
	else
	{
		// stop outputting anything whatsoever
		internalState->lineValues |= CSBusStandardDataMask;
//		printf("r-\n");
	}
}

static void csStaticMemory_observeMemoryWrite(void *opaqueMemory, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue)
{
	if(!conditionIsTrue)
	{
		// store the incoming value...

		// fetch the address from the bus
		unsigned int address = (externalState.lineValues&CSBusStandardAddressMask) >> CSBusStandardAddressShift;

		// reduce that down to an address in our range
		CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;
		address &= (memory->size - 1);

		// and load the data lines
		memory->contents[address] = externalState.lineValues >> CSBusStandardDataShift;
	}
}

static void csStaticMemory_dealloc(void *opaqueMemory)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	if(memory->contents) free(memory->contents);
}

void csStaticMemory_setContents(void *opaqueMemory, unsigned int dest, const uint8_t *source, unsigned int length)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	// do some trivial bounds checking and adjustment
	if(dest > memory->size) return;
	if(dest + length > memory->size) length = memory->size - dest;

	// copy
	memcpy(&memory->contents[dest], source, length);
}

void csStaticMemory_getContents(void *opaqueMemory, uint8_t *dest, unsigned int source, unsigned int length)
{
	CSStaticMemory *memory = (CSStaticMemory *)opaqueMemory;

	// do some trivial bounds checking and adjustment
	if(source > memory->size) return;
	if(source + length > memory->size) length = memory->size - source;

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
		memory->size = size;
		
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
