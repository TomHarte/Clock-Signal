//
//  ReferenceCountedObject.h
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_ReferenceCountedObject_h
#define ClockSignal_ReferenceCountedObject_h

#include "stdint.h"
#include <stdio.h>

typedef struct
{
	unsigned int retainCount;

	const char *type;
	void (* dealloc)(void *object);
	char *(* copyDescription)(void *object);
} CSReferenceCountedObject;

void csObject_init(void *object);
void *csObject_retain(void *object);
void csObject_release(void *object);

char *csObject_copyDescription(void *object);
void csObject_printDescription(void *object, FILE *outputstream);

#endif
