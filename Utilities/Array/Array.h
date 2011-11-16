//
//  Array.h
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_Array_h
#define ClockSignal_Array_h

#include "stdbool.h"

// An array for pointers that will optionally retain objects
// added to it.
//
// As a rule, inserts are cheap and removes are expensive.
//

// creation function; specify now whether you want additions
// to be retained
void *csArray_create(bool retainObjects);

// methods to add and remove objects
bool csArray_addObject(void *array, void *object);
bool csArray_removeObject(void *array, void *object);

// functions to get (or remove) objects by index
void *csArray_getObjectAtIndex(void *array, unsigned int index);
bool csArray_removeObjectAtIndex(void *array, unsigned int index);

void csArray_removeAllObjects(void *array);

// use this to get a C array of the contents and the array
// and a count of the objects in the array — so you can
// iterate through the contents
void **csArray_getCArray(void *array, unsigned int *numberOfObjects);

#endif
