//
//  ReferenceCountedObject.h
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_ReferenceCountedObject_h
#define ClockSignal_ReferenceCountedObject_h

#include "stdint.h"

typedef struct
{
	unsigned int retainCount;

	const char *type;
	void (* dealloc)(void *object);
	const char *(* description)(void *object);
} CSReferenceCountedObject;

void csObject_init(void *object);
void *csObject_retain(void *object);
void csObject_release(void *object);
const char *csObject_description(void *object);

#endif
