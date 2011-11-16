//
//  StaticMemory.h
//  Clock Signal
//
//  Created by Thomas Harte on 21/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_StaticMemory_h
#define ClockSignal_StaticMemory_h

#include "stdbool.h"
#include "stdint.h"
#include "../../Bus/BusState.h"

extern const char *staticMemoryType;

/*

	Static memory is ROM or static RAM.

	Size should always be a power of two.

	Pass an impossible write condition for
	read-only memory (ie, ROM).

*/
void *csStaticMemory_createOnBus(void *bus, unsigned int size, CSBusCondition readCondition, CSBusCondition writeCondition);
void csStaticMemory_setContents(void *memory, unsigned int dest, const uint8_t *source, unsigned int length);
void csStaticMemory_getContents(void *memory, uint8_t *dest, unsigned int source, unsigned int length);

#endif
