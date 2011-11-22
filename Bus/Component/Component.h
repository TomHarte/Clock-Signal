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
typedef void (* csComponent_handlerFunction)(void *context, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue);

void *csComponent_create(csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context);
void csComponent_addToBus(void *bus, csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context);

#endif
