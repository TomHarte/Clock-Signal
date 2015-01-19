//
//  FlatBus.h
//  Clock Signal
//
//  Created by Thomas Harte on 05/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_FlatBus_h
#define ClockSignal_FlatBus_h

void *csFlatBus_create(void);

void csFlatBus_setTicksPerSecond(void *, uint32_t ticksPerSecond);

void csFlatBus_runForHalfCycles(void *, unsigned int halfCycles);
unsigned int csFlatBus_getHalfCyclesToDate(void *);

#endif
