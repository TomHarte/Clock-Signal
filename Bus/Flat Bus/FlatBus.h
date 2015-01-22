//
//  FlatBus.h
//  Clock Signal
//
//  Created by Thomas Harte on 05/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_FlatBus_h
#define ClockSignal_FlatBus_h

#include "Component.h"

void *csFlatBus_create(void);
void *csFlatBus_createComponent(
   void *,
   csComponent_handlerFunction function,
   CSBusCondition necessaryCondition,
   uint64_t outputLines,
   void *context);
void csFlatBus_setModalComponentFilter(
	void *,
	csComponent_prefilter filterFunction,
	void *context);

void csFlatBus_setTicksPerSecond(void *, uint32_t ticksPerSecond);

void csFlatBus_runForHalfCycles(void *, unsigned int halfCycles);
unsigned int csFlatBus_getHalfCyclesToDate(void *);

#endif
