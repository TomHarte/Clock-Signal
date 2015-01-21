//
//  Component.c
//  Clock Signal
//
//  Created by Thomas Harte on 01/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "ComponentInternals.h"
#include "ReferenceCountedObject.h"
#include <stdlib.h>

static void csComponent_destroy(void *opaqueComponent)
{
	CSBusComponent *component = (CSBusComponent *)opaqueComponent;

	csObject_release(component->context);
	csObject_release(component->preFilterContext);
}

void *csComponent_init(void *opaqueComponent, csComponent_handlerFunction function, CSBusCondition necessaryCondition, uint64_t outputLines, void *context)
{
	CSBusComponent *component = (CSBusComponent *)opaqueComponent;

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

void csComponent_setPreFilter(void *opaqueComponent, csComponent_prefilter filterFunction, void *context)
{
	CSBusComponent *component = (CSBusComponent *)opaqueComponent;

	csObject_release(component->preFilterContext);
	component->preFilterContext = csObject_retain(context);
	component->preFilter = filterFunction;
}
