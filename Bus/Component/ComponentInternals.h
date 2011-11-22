//
//  ComponentInternals.h
//  Clock Signal
//
//  Created by Thomas Harte on 01/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_ComponentInternals_h
#define ClockSignal_ComponentInternals_h

#include "Component.h"
#include "BusState.h"
#include "ReferenceCountedObject.h"

typedef struct CSBusComponent
{
	// this is a reference counted object
	CSReferenceCountedObject referenceCountedObject;

	// these describe the component in abstract terms
	CSBusCondition condition;
	csComponent_handlerFunction handlerFunction;
	uint64_t outputLines;
	void *context;

	// these relate to its current state
	CSBusState currentInternalState;
	uint64_t lastLineValues;
	bool lastResult;

} CSBusComponent;

#endif
