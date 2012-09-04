//
//  ClockGenerator.h
//  Clock Signal
//
//  Created by Thomas Harte on 29/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "stdint.h"

#ifndef ClockSignal_ClockGenerator_h
#define ClockSignal_ClockGenerator_h

void *csClockGenerator_createWithBus(void *bus, uint32_t ticksPerSecond);
void csClockGenerator_runForHalfCycles(void *opaqueGenerator, unsigned int halfCycles);
unsigned int csClockGenerator_getHalfCyclesToDate(void *);

#endif
