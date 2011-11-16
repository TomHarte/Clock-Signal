//
//  PassthroughBusNode.h
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_PassthroughBusNode_h
#define ClockSignal_PassthroughBusNode_h

#include "BusNode.h"
#include "BusNodeInternals.h"
#include "Component.h"

typedef void (* csPassthroughNode_prefilter)(void *context, CSBusCondition *busCondition, uint64_t *outputLines);

typedef struct
{
	CSBusNode busNode;

	void *context;
	csComponent_handlerFunction filterFunction;
	csPassthroughNode_prefilter prefilter;

	void *childComponent;

} CSBusPassthroughNode;


// the passed component will be supplied with a CSBusPassthroughNode
// as its context, which it should act as a man-in-the-middle for
void *csPassthroughNode_createWithFilter(void *context, csComponent_handlerFunction filterFunction, csPassthroughNode_prefilter prefilter);

#endif
