//
//  FlatBus.c
//  Clock Signal
//
//  Created by Thomas Harte on 05/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

#include "ReferenceCountedObject.h"

#include "BusState.h"
#include "StandardBusLines.h"

#include "RateConverter.h"
#include "AllocatingArray.h"

#include "FlatBus.h"
#include "ComponentInternals.h"

#ifdef __BLOCKS__
#include <dispatch/dispatch.h>
#endif

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	struct CSFlatBusComponentSet
	{
		void *components;
		CSBusState state;
		CSBusState lastExternalState;
		uint64_t allObservedSetLines, allObservedResetLines, allObservedChangeLines;
	} clockedComponents, trueFalseComponents, trueComponents;

	CSBusState currentBusState;
	unsigned int halfCyclesToDate;
	CSRateConverterState time;

	csComponent_prefilter filterFunction;
	void *filterFunctionContext;
} CSFlatBus;

static void csFlatBus_updateSetForNewComponent(struct CSFlatBusComponentSet *set)
{
	set->allObservedSetLines = 0;
	set->allObservedResetLines = 0;
	set->allObservedChangeLines = 0;

	unsigned int numberOfComponents;
	CSBusComponent *components = (CSBusComponent *)csAllocatingArray_getCArray(set->components, &numberOfComponents);
	for(unsigned int c = 0; c < numberOfComponents; c++)
	{
		uint64_t changedLines = components[c].condition.changedLines;
		uint64_t lineMask = components[c].condition.lineMask;
		uint64_t lineValues = components[c].condition.lineValues;

		uint64_t setLineMask = lineMask & lineValues;
		uint64_t resetLineMask = lineMask & ~lineValues;
		uint64_t changeLineMask = changedLines &~ (setLineMask | resetLineMask);

		set->allObservedSetLines |= setLineMask;// | changeLineMask;
		set->allObservedResetLines |= resetLineMask;// | changeLineMask;
		set->allObservedChangeLines |= changeLineMask;
	}
}

static void csFlatBus_initialiseSet(struct CSFlatBusComponentSet *set)
{
	set->components = csAllocatingArray_createWithObjectSize(sizeof(CSBusComponent), true);
	set->state = csBus_defaultState();
	set->lastExternalState = csBus_defaultState();
}

static void csFlatBus_destroySet(struct CSFlatBusComponentSet *set)
{
	csObject_release(set->components);
}

void csFlatBus_setModalComponentFilter(
	void *opaqueBus,
	csComponent_prefilter filterFunction,
	void *context)
{
	CSFlatBus *flatBus = (CSFlatBus *)opaqueBus;

	csObject_release(flatBus->filterFunctionContext);
	flatBus->filterFunctionContext = csObject_retain(context);
	flatBus->filterFunction = filterFunction;
}

void *csFlatBus_createComponent(
	void *opaqueBus,
	csComponent_handlerFunction function,
	CSBusCondition necessaryCondition,
	uint64_t outputLines,
	void *context)

{
	CSFlatBus *flatBus = (CSFlatBus *)opaqueBus;
	CSBusComponent *component;
	struct CSFlatBusComponentSet *set;

	// add the new component to the clocked set if it observes the clock line,
	// the unclocked set otherwise
	if(csBusCondition_observedLines(necessaryCondition) == CSBusStandardClockLine)
	{
		set = &flatBus->clockedComponents;
	}
	else
	{
		if(necessaryCondition.signalOnTrueOnly)
			set = &flatBus->trueComponents;
		else
			set = &flatBus->trueFalseComponents;
	}

	component = csAllocatingArray_newObject(set->components);
	csComponent_init(component, function, necessaryCondition, outputLines, context);
	csFlatBus_updateSetForNewComponent(set);

	if(flatBus->filterFunction)
	{
		csComponent_setPreFilter(component, flatBus->filterFunction, flatBus->filterFunctionContext);
	}

	return component;
}

unsigned int csFlatBus_getHalfCyclesToDate(void *opaqueBus)
{
	return ((CSFlatBus *)opaqueBus)->halfCyclesToDate;
}

void csFlatBus_runForHalfCycles(void *context, unsigned int halfCycles)
{
	CSFlatBus *const restrict flatBus = (CSFlatBus *)context;
	CSBusState totalState;
	uint64_t changedLines, setLines, resetLines;
	CSRateConverterState time = flatBus->time;
	unsigned int halfCyclesToDate = flatBus->halfCyclesToDate;

	unsigned int numberOfClockedComponents;
	CSBusComponent *const restrict clockedComponents = (CSBusComponent *)csAllocatingArray_getCArray(flatBus->clockedComponents.components, &numberOfClockedComponents);

	unsigned int numberOfTrueFalseComponents;
	CSBusComponent *const restrict trueFalseComponents = (CSBusComponent *)csAllocatingArray_getCArray(flatBus->trueFalseComponents.components, &numberOfTrueFalseComponents);

	unsigned int numberOfTrueComponents;
	CSBusComponent *const restrict trueComponents = (CSBusComponent *)csAllocatingArray_getCArray(flatBus->trueComponents.components, &numberOfTrueComponents);

	unsigned int componentIndex;

#define callHandler(x, status) \
	x.handlerFunction(\
		x.context,\
		&x.currentInternalState,\
		x.preFilter ? x.preFilter(x.preFilterContext, totalState ) : totalState,\
		status,\
		csRateConverter_getLocation(time))

	while(halfCycles--)
	{
		flatBus->currentBusState.lineValues ^= CSBusStandardClockLine;

		// get total state as viewed from the true and true/false components
		totalState.lineValues = flatBus->currentBusState.lineValues & flatBus->trueComponents.state.lineValues & flatBus->trueFalseComponents.state.lineValues & flatBus->clockedComponents.state.lineValues;

		// hence get the changed, set and reset lines
		changedLines = flatBus->trueComponents.lastExternalState.lineValues ^ totalState.lineValues;
		flatBus->trueComponents.lastExternalState = totalState;

		setLines = totalState.lineValues & changedLines;
		resetLines = setLines ^ changedLines;

		// is it possible some are now true that weren't a moment ago from the true set?
		if(flatBus->trueComponents.allObservedSetLines&setLines || flatBus->trueComponents.allObservedResetLines&resetLines)
		{
			flatBus->trueComponents.state.lineValues = ~0llu;

			componentIndex = numberOfTrueComponents;
			while(componentIndex--)
			{
				// if one of the monitored lines just changed and the condition is now true,
				// it can't have been before so this is a time to message
				if(
					(trueComponents[componentIndex].condition.lineMask&changedLines) &&
					(trueComponents[componentIndex].condition.lineValues == (trueComponents[componentIndex].condition.lineMask&totalState.lineValues)))
				{
					callHandler(trueComponents[componentIndex], true);
				}

				flatBus->trueComponents.state.lineValues &= trueComponents[componentIndex].currentInternalState.lineValues;
			}
		}

		// maybe some have gone true or mutated while true from the true/false set?
		if(
			(flatBus->trueFalseComponents.allObservedSetLines&setLines) || 
			(flatBus->trueFalseComponents.allObservedResetLines&resetLines) ||
			(flatBus->trueFalseComponents.allObservedChangeLines&changedLines))
		{
			flatBus->trueFalseComponents.state.lineValues = ~0llu;

			componentIndex = numberOfTrueFalseComponents;
			while(componentIndex--)
			{
				// so, logic is:
				//
				//	if
				//			mask condition has changed, or
				//			mask condition is true and one of the other monitored lines has changed
				bool newEvaluation = trueFalseComponents[componentIndex].condition.lineValues == (trueFalseComponents[componentIndex].condition.lineMask&totalState.lineValues);

				if(
					(newEvaluation != trueFalseComponents[componentIndex].lastResult) || (newEvaluation && trueFalseComponents[componentIndex].condition.changedLines&changedLines))
				{
					callHandler(trueFalseComponents[componentIndex], newEvaluation);
					trueFalseComponents[componentIndex].lastResult = newEvaluation;
				}

				flatBus->trueFalseComponents.state.lineValues &= trueFalseComponents[componentIndex].currentInternalState.lineValues;
			}
		}
		else
		{
			// nobody has become true or mutated, so check everyone that's currently true to see if they've become false
			if( (flatBus->trueFalseComponents.allObservedSetLines | flatBus->trueFalseComponents.allObservedResetLines)&changedLines)
			{
				flatBus->trueFalseComponents.state.lineValues = ~0llu;

				componentIndex = numberOfTrueFalseComponents;
				while(componentIndex--)
				{
					if(trueFalseComponents[componentIndex].lastResult)
					{
						bool newEvaluation = 
								trueFalseComponents[componentIndex].condition.lineValues == (trueFalseComponents[componentIndex].condition.lineMask&totalState.lineValues);

						if(!newEvaluation)
						{
							callHandler(trueFalseComponents[componentIndex], false);
							trueFalseComponents[componentIndex].lastResult = newEvaluation;
						}
					}

					flatBus->trueFalseComponents.state.lineValues &= trueFalseComponents[componentIndex].currentInternalState.lineValues;
				}
			}
		}

		// get total state as viewed from the true and true/false components
		totalState.lineValues = flatBus->currentBusState.lineValues & flatBus->trueComponents.state.lineValues & flatBus->trueFalseComponents.state.lineValues & flatBus->clockedComponents.state.lineValues;

		// hence get the changed, set and reset lines
		flatBus->clockedComponents.lastExternalState = totalState;
		flatBus->clockedComponents.state.lineValues = ~0llu;

		componentIndex = numberOfClockedComponents;
		bool newClockLine = !!(flatBus->currentBusState.lineValues & CSBusStandardClockLine);
		while(componentIndex--)
		{
			if(newClockLine || !clockedComponents[componentIndex].condition.signalOnTrueOnly)
			{
				callHandler(clockedComponents[componentIndex], newClockLine);
			}

			flatBus->clockedComponents.state.lineValues &= clockedComponents[componentIndex].currentInternalState.lineValues;
		}

		halfCyclesToDate++;
		flatBus->halfCyclesToDate = halfCyclesToDate;

		csRateConverter_advance(time)
	}

	flatBus->time = time;
}

static void csFlatBus_destroy(void *bus)
{
	CSFlatBus *flatBus = (CSFlatBus *)bus;
	csFlatBus_destroySet(&flatBus->clockedComponents);
	csFlatBus_destroySet(&flatBus->trueComponents);
	csFlatBus_destroySet(&flatBus->trueFalseComponents);
	csObject_release(flatBus->filterFunctionContext);
}

void *csFlatBus_create(void)
{
	CSFlatBus *flatBus = (CSFlatBus *)calloc(1, sizeof(CSFlatBus));

	if(flatBus)
	{
		// set up as a bus node, with our destroy function
		csObject_init(flatBus);
		flatBus->referenceCountedObject.dealloc = csFlatBus_destroy;

		csFlatBus_initialiseSet(&flatBus->clockedComponents);
		csFlatBus_initialiseSet(&flatBus->trueComponents);
		csFlatBus_initialiseSet(&flatBus->trueFalseComponents);

		// for the purposes of clock signal generation...
		flatBus->currentBusState = csBus_defaultState();
	}

	return flatBus;
}

void csFlatBus_setTicksPerSecond(void *bus, uint32_t ticksPerSecond)
{
	CSFlatBus *flatBus = (CSFlatBus *)bus;
	ticksPerSecond <<= 1;
	csRateConverter_setup(flatBus->time, ticksPerSecond, 1000000000)
}
