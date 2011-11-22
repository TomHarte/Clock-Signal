//
//  BusNodeInternals.h
//  Clock Signal
//
//  Created by Thomas Harte on 29/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_BusNodeInternals_h
#define ClockSignal_BusNodeInternals_h

#include "ReferenceCountedObject.h"
#include "stdbool.h"
#include "stdint.h"
#include "BusState.h"
#include "ComponentInternals.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	void (* addComponent)(void *node, void *component);
	void *(* createComponent)(void *node);

	int (* getNumberOfChildren)(void *node);
} CSBusNode;

void csBusNode_init(void *node);


typedef struct
{
	CSBusNode busNode;
	unsigned int childWritePointer;

	CSBusComponent *children[2];

	void (* messageWithFeedback)(void *opaqueBusNode, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue);
	void (* messageWithoutFeedback)(void *opaqueBusNode, CSBusState *internalState, CSBusState externalState, bool conditionIsTrue);

} CSBinaryBusNode;

void csBinaryBusNode_init(void *node);

#endif
