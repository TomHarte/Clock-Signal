//
//  Component.h
//  Clock Signal
//
//  Created by Thomas Harte on 01/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_Component_h
#define ClockSignal_Component_h

#include "BusState.h"

typedef uint64_t CSComponentNanoseconds;

// this is the standard form of a 'component' — anything
// that can receive a change record and overall status,
// describing activity everywhere else on the bus, and
// populate a change record indicating how this component
// responds as a result.
//
// Assumptions made by components:
//
//		- the response has a valid initial state
//
typedef void (* csComponent_handlerFunction)(
	void *const context,						// the context is whatever you supplied to csComponent_create; it will have been retained
	CSBusState *const internalState,			// the internal state is your component's internal bus state
	const CSBusState externalState,				// the external state is the state of the bus externally
	const bool conditionIsTrue,					// this flag indicates whether the condition supplied to csComponent_create has just become true;
												// if not then logically it has just become false. You can specify whether you want to receive the
												// falses when creating the condition
	const CSComponentNanoseconds timeSinceLaunch);
												// timeSinceLaunch is a count of the number of nanoseconds since the bus started working.
												// Components should generally track time by observing the clock line. However for those
												// components that also have time-dependant characteristics (such as dynamic RAM), it can
												// be helpful to be able to track real time rather than clock time

void *csComponent_create(csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context);
void csComponent_addToBus(void *bus, csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context);

#endif
