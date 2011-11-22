//
//  Component.c
//  Clock Signal
//
//  Created by Thomas Harte on 01/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "ComponentInternals.h"
#include "ReferenceCountedObject.h"
#include "BusNode.h"
#include <stdlib.h>

static void csComponent_destroy(void *component)
{
	csObject_release(((CSBusComponent *)component)->context);
}

void *csComponent_create(csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context)
{
	CSBusComponent *component = (CSBusComponent *)calloc(1, sizeof(CSBusComponent));

	if(component)
	{
		csObject_init(component);
		component->referenceCountedObject.dealloc = csComponent_destroy;
		
		component->handlerFunction = function;
		component->condition = necessaryCondition;
		component->outputLines = outputLines;
		component->context = csObject_retain(context);
		component->currentInternalState = csBus_defaultState();
	}

	return component;
}

void csComponent_addToBus(void *bus, csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context)
{
	void *component = 
		csComponent_create(function, necessaryCondition, outputLines, context);
	csBusNode_addComponent(bus, component);
	csObject_release(component);
}
