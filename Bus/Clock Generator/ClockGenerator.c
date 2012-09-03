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

	CSComponentNanoseconds timeToNow;
	int64_t accumulatedError, wholeStep, adjustmentUp, adjustmentDown;

} CSClockGenerator;

static void csClockGenerator_destroy(void *opaqueGenerator)
{
	CSClockGenerator *generator = (CSClockGenerator *)opaqueGenerator;
	csObject_release(generator->component);
}

void *csClockGenerator_createWithBus(void *bus, uint32_t ticksPerSecond)
{
	CSClockGenerator *generator = (CSClockGenerator *)calloc(1, sizeof(CSClockGenerator));

	if(generator)
	{
		csObject_init(generator);
		generator->referenceCountedObject.dealloc = csClockGenerator_destroy;
		generator->component = csBusNode_createComponent(bus);
		generator->currentBusState = csBus_defaultState();

		// These constitute a fairly basic implementation of run-slice
		// Bresenham (since we'll be stepping along the smaller delta and
		// projecting onto the big)
		//
		// i.e. this is like drawing a first quadrant line with
		// x = 1000000000 (ie, nanoseconds in a second)
		// y = clock ticks per second
		generator->wholeStep = 1000000000 / ticksPerSecond;
		generator->adjustmentUp = (1000000000 % ticksPerSecond) << 1;
		generator->adjustmentDown = ticksPerSecond << 1;
		generator->accumulatedError = generator->adjustmentUp - generator->adjustmentDown;
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
		generator->component->handlerFunction(generator->component->context, &throwawayState, generator->currentBusState, true, generator->timeToNow);
		generator->halfCyclesToDate++;

		// this is standard Bresenham run-slice stuff; add the
		// whole step, which is floor(y/x), then see whether
		// doing so has accumulated enough error to push us
		// up an extra spot
		generator->timeToNow += generator->wholeStep;
		generator->accumulatedError += generator->adjustmentUp;
		if(generator->accumulatedError > 0)
		{
			generator->timeToNow++;
			generator->accumulatedError -= generator->adjustmentDown;
		}
	}
//	printf("clock generator loop done\n");
}

unsigned int csClockGenerator_getHalfCyclesToDate(void *opaqueGenerator)
{
	return ((CSClockGenerator *)opaqueGenerator)->halfCyclesToDate;
}
