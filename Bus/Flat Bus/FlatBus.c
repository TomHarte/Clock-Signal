//
//  FlatBus.c
//  Clock Signal
//
//  Created by Thomas Harte on 05/11/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

#include "FlatBus.h"
#include "ReferenceCountedObject.h"
#include "BusNodeInternals.h"
#include "Array.h"
#include "BusState.h"
#include "StandardBusLines.h"
#include "RateConverter.h"

#ifdef __BLOCKS__
#include <dispatch/dispatch.h>
#endif

typedef struct
{
	CSBusNode busNode;

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

} CSFlatBus;

static void csFlatBus_addComponentToSet(struct CSFlatBusComponentSet *set, void *newComponent)
{
	csArray_addObject(set->components, newComponent);

	set->allObservedSetLines = 0;
	set->allObservedResetLines = 0;
	set->allObservedChangeLines = 0;

	unsigned int numberOfComponents;
	void **components = csArray_getCArray(set->components, &numberOfComponents);
	for(unsigned int c = 0; c < numberOfComponents; c++)
	{
		CSBusComponent *component = components[c];
		uint64_t changedLines = component->condition.changedLines;
		uint64_t lineMask = component->condition.lineMask;
		uint64_t lineValues = component->condition.lineValues;

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
	set->components = csArray_create(true);
	set->state = csBus_defaultState();
	set->lastExternalState = csBus_defaultState();
}

static void csFlatBus_destroySet(struct CSFlatBusComponentSet *set)
{
	csObject_release(set->components);
}

static void csFlatBus_addComponent(void *node, void *opaqueComponent)
{
	CSFlatBus *flatBus = (CSFlatBus *)node;
	CSBusComponent *component = (CSBusComponent *)opaqueComponent;

	// add the new component to the clocked set if it observes the clock line,
	// the unclocked set otherwise
	if(csBusCondition_observedLines(component->condition) == CSBusStandardClockLine)
	{
		csFlatBus_addComponentToSet(&flatBus->clockedComponents, component);
	}
	else
	{
		if(component->condition.signalOnTrueOnly)
			csFlatBus_addComponentToSet(&flatBus->trueComponents, component);
		else
			csFlatBus_addComponentToSet(&flatBus->trueFalseComponents, component);
	}
}

/*static void csBusNode_binaryPrint(uint64_t value)
{
	uint64_t mask = 0x8000000000000000;
	int position = 63;
	while(mask)
	{
		printf("%c", (value&mask) ? '1' : '0');
		mask >>= 1;

		switch(position)
		{
			default: break;
			case 32:
			case 40:
			case 48:
			case 56:
				printf(".");
			break;

			case 24:
			case 8:
				printf("|");
			break;
		}
		position--;
	}
}*/

/*static void inline csFlatBus_messageList(CSFlatBus *flatBus, void *list, uint64_t listFlags, CSBusState *state, CSBusState *oldExternalState, CSBusState externalState)
{
	// composite the external and internal state
	CSBusState totalState;
	totalState.lineValues = externalState.lineValues & flatBus->otherComponentsState.lineValues & flatBus->clockedComponentsState.lineValues;

	// work out what's changed since the last message
	uint64_t changedLines = oldExternalState->lineValues ^ totalState.lineValues;
	*oldExternalState = totalState;

	// will we consider the clock guys?
	if(changedLines&listFlags)
	{
		state->lineValues = -1;

		unsigned int numberOfComponents;
		void **components = csArray_getCArray(list, &numberOfComponents);
		while(numberOfComponents--)
		{
			CSBusComponent *component = (CSBusComponent *)components[numberOfComponents];

			if(component->condition.changedLines&changedLines)
			{
				bool newEvaluation = 
					(component->condition.lineValues == (component->condition.lineMask&totalState.lineValues));

				if(newEvaluation != component->lastResult)
				{
					component->handlerFunction(
						component->context,
						&component->currentInternalState,
						totalState,
						newEvaluation);
					component->lastResult = newEvaluation;
				}
			}

			state->lineValues &= component->currentInternalState.lineValues;
		}
	}
}*/

//csComponent_observer(csFlatBus_message)

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
	CSBusComponent **const restrict clockedComponents = (CSBusComponent **)csArray_getCArray(flatBus->clockedComponents.components, &numberOfClockedComponents);
	
	unsigned int numberOfTrueFalseComponents;
	CSBusComponent **const restrict trueFalseComponents = (CSBusComponent **)csArray_getCArray(flatBus->trueFalseComponents.components, &numberOfTrueFalseComponents);

	unsigned int numberOfTrueComponents;
	CSBusComponent **const restrict trueComponents = (CSBusComponent **)csArray_getCArray(flatBus->trueComponents.components, &numberOfTrueComponents);

	unsigned int componentIndex;

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
					(trueComponents[componentIndex]->condition.lineMask&changedLines) &&
					(trueComponents[componentIndex]->condition.lineValues == (trueComponents[componentIndex]->condition.lineMask&totalState.lineValues)))
				{
					trueComponents[componentIndex]->handlerFunction(
						trueComponents[componentIndex]->context,
						&trueComponents[componentIndex]->currentInternalState,
						totalState,
						true,
						csRateConverter_getLocation(time));
				}

				flatBus->trueComponents.state.lineValues &= trueComponents[componentIndex]->currentInternalState.lineValues;
			}
		}

		// get total state as viewed from the true and true/false components
		totalState.lineValues = flatBus->currentBusState.lineValues & flatBus->trueComponents.state.lineValues & flatBus->trueFalseComponents.state.lineValues & flatBus->clockedComponents.state.lineValues;

		// hence get the changed, set and reset lines
		changedLines = flatBus->trueFalseComponents.lastExternalState.lineValues ^ totalState.lineValues;
		flatBus->trueFalseComponents.lastExternalState = totalState;

		setLines = totalState.lineValues & changedLines;
		resetLines = setLines ^ changedLines;

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
				bool newEvaluation = trueFalseComponents[componentIndex]->condition.lineValues == (trueFalseComponents[componentIndex]->condition.lineMask&totalState.lineValues);

				if(
					(newEvaluation != trueFalseComponents[componentIndex]->lastResult) || (newEvaluation && trueFalseComponents[componentIndex]->condition.changedLines&changedLines))
				{
					trueFalseComponents[componentIndex]->handlerFunction(
						trueFalseComponents[componentIndex]->context,
						&trueFalseComponents[componentIndex]->currentInternalState,
						totalState,
						newEvaluation,
						csRateConverter_getLocation(time));
					trueFalseComponents[componentIndex]->lastResult = newEvaluation;
				}

				flatBus->trueFalseComponents.state.lineValues &= trueFalseComponents[componentIndex]->currentInternalState.lineValues;
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
					if(trueFalseComponents[componentIndex]->lastResult)
					{
						bool newEvaluation = 
								trueFalseComponents[componentIndex]->condition.lineValues == (trueFalseComponents[componentIndex]->condition.lineMask&totalState.lineValues);

						if(!newEvaluation)
						{
							trueFalseComponents[componentIndex]->handlerFunction(
								trueFalseComponents[componentIndex]->context,
								&trueFalseComponents[componentIndex]->currentInternalState,
								totalState,
								newEvaluation,
								csRateConverter_getLocation(time));
							trueFalseComponents[componentIndex]->lastResult = newEvaluation;
						}
					}

					flatBus->trueFalseComponents.state.lineValues &= trueFalseComponents[componentIndex]->currentInternalState.lineValues;
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
			if(newClockLine || !clockedComponents[componentIndex]->condition.signalOnTrueOnly)
				clockedComponents[componentIndex]->handlerFunction(
					clockedComponents[componentIndex]->context,
					&clockedComponents[componentIndex]->currentInternalState,
					totalState,
					newClockLine,
					csRateConverter_getLocation(time));

			flatBus->clockedComponents.state.lineValues &= clockedComponents[componentIndex]->currentInternalState.lineValues;
		}

		halfCyclesToDate++;
		flatBus->halfCyclesToDate = halfCyclesToDate;

		csRateConverter_advance(time)
	}

	flatBus->time = time;
}

//static void csFlatBus_addChangeFlagsFromSet(struct CSFlatBusComponentSet *set, uint64_t *outputLines, uint64_t *observedLines)
//{
//	unsigned int numberOfComponents;
//	void **components = csArray_getCArray(set->components, &numberOfComponents);
//	for(unsigned int c = 0; c < numberOfComponents; c++)
//	{
//		*outputLines |= ((CSBusComponent *)components[c])->outputLines;
//		*observedLines |= csBusCondition_observedLines(((CSBusComponent *)components[c])->condition);
//	}
//}

static void *csFlatBus_createComponent(void *node)
{
//	CSFlatBus *flatBus = (CSFlatBus *)node;
//
//	uint64_t outputLines = 0;
//	uint64_t observedLines = 0;
//
//	csFlatBus_addChangeFlagsFromSet(&flatBus->clockedComponents, &outputLines, &observedLines);
//	csFlatBus_addChangeFlagsFromSet(&flatBus->trueFalseComponents, &outputLines, &observedLines);
//	csFlatBus_addChangeFlagsFromSet(&flatBus->trueComponents, &outputLines, &observedLines);
//
//	return csComponent_create(
//		csFlatBus_message,
//		csBus_changeCondition(observedLines),
//		outputLines,
//		node);
	return NULL;
}

static int csFlatBus_getNumberOfChildren(void *node)
{
	return 0;
}

static void csFlatBus_destroy(void *bus)
{
	CSFlatBus *flatBus = (CSFlatBus *)bus;
	csFlatBus_destroySet(&flatBus->clockedComponents);
	csFlatBus_destroySet(&flatBus->trueComponents);
	csFlatBus_destroySet(&flatBus->trueFalseComponents);
}

void *csFlatBus_create(void)
{
	CSFlatBus *flatBus = (CSFlatBus *)calloc(1, sizeof(CSFlatBus));

	if(flatBus)
	{
		// set up as a bus node, with our destroy function
		csBusNode_init(flatBus);
		flatBus->busNode.referenceCountedObject.dealloc = csFlatBus_destroy;

		csFlatBus_initialiseSet(&flatBus->clockedComponents);
		csFlatBus_initialiseSet(&flatBus->trueComponents);
		csFlatBus_initialiseSet(&flatBus->trueFalseComponents);

/*		if(!flatBus->clockedComponents || !flatBus->otherComponents)
		{
			csObject_release(flatBus->clockedComponents);
			csObject_release(flatBus->otherComponents);
			free(flatBus);
			return NULL;
		}*/

		// set up methods to catch added components
		flatBus->busNode.addComponent = csFlatBus_addComponent;
		flatBus->busNode.createComponent = csFlatBus_createComponent;
		flatBus->busNode.getNumberOfChildren = csFlatBus_getNumberOfChildren;

		// for the purposes of clock signal generation...
		flatBus->currentBusState = csBus_defaultState();
	}

	return flatBus;
}

void csFlatBus_setTicksPerSecond(void *bus, uint32_t ticksPerSecond)
{
	CSFlatBus *flatBus = (CSFlatBus *)bus;
	csRateConverter_setup(flatBus->time, 1000000000, ticksPerSecond)
}
