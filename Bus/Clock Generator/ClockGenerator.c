//
//  ClockGenerator.c
//  Clock Signal
//
//  Created by Thomas Harte on 29/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

#include "ClockGenerator.h"
#include "ReferenceCountedObject.h"
#include "BusState.h"
#include "BusNode.h"
#include "ComponentInternals.h"
#include "StandardBusLines.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	unsigned int halfCyclesToDate;
	CSBusComponent *component;
	CSBusState currentBusState;

} CSClockGenerator;

static void csClockGenerator_destroy(void *opaqueGenerator)
{
	CSClockGenerator *generator = (CSClockGenerator *)opaqueGenerator;
	csObject_release(generator->component);
}

void *csClockGenerator_createWithBus(void *bus)
{
	CSClockGenerator *generator = (CSClockGenerator *)calloc(1, sizeof(CSClockGenerator));

	if(generator)
	{
		csObject_init(generator);
		generator->referenceCountedObject.dealloc = csClockGenerator_destroy;
		generator->component = csBusNode_createComponent(bus);
		generator->currentBusState = csBus_defaultState();
	}

	return generator;
}

void csClockGenerator_runForHalfCycles(void *opaqueGenerator, unsigned int halfCycles)
{
	CSClockGenerator *generator = (CSClockGenerator *)opaqueGenerator;
	CSBusState throwawayState;

//	printf("starting clock generator loop\n");
	while(halfCycles--)
	{
		generator->currentBusState.lineValues ^= CSBusStandardClockLine;
		generator->component->handlerFunction(generator->component->context, &throwawayState, generator->currentBusState, true);
		generator->halfCyclesToDate++;
	}
//	printf("clock generator loop done\n");
}

unsigned int csClockGenerator_getHalfCyclesToDate(void *opaqueGenerator)
{
	return ((CSClockGenerator *)opaqueGenerator)->halfCyclesToDate;
}
