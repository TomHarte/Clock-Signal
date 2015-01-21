//
//  AllocatingArray.c
//  Clock Signal
//
//  Created by Thomas Harte on 20/01/2015.
//  Copyright (c) 2015 acrossair. All rights reserved.
//

#include "AllocatingArray.h"
#include "ReferenceCountedObject.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;
	
	uint8_t *objects;
	unsigned int numberOfObjects, numberOfAllocatedObjects;
	size_t objectSize;

} CSAllocatingArray;

static void csAllocatingArray_destroy(void *opaqueArray)
{
	CSAllocatingArray *array = (CSAllocatingArray *)opaqueArray;
	free(array->objects);
}

void *csAllocatingArray_createWithObjectSize(size_t objectSize)
{
	CSAllocatingArray *array = (CSAllocatingArray *)calloc(1, sizeof(CSAllocatingArray));
	
	if(array)
	{
		csObject_init(array);
		array->referenceCountedObject.dealloc = csAllocatingArray_destroy;
	}
	
	return array;
}

void *csAllocatingArray_newObject(void *opaqueArray)
{
	CSAllocatingArray *array = (CSAllocatingArray *)opaqueArray;
	
	if(array->numberOfObjects == array->numberOfAllocatedObjects)
	{
		uint8_t *newBuffer;
		unsigned int newNumberOfAllocatedObjects = (array->numberOfAllocatedObjects << 1) + 1;

		newBuffer = (uint8_t *)realloc(array->objects, array->objectSize*newNumberOfAllocatedObjects);

		if(!newBuffer) return NULL;
		
		array->objects = newBuffer;
		array->numberOfAllocatedObjects = newNumberOfAllocatedObjects;
	}

	void *newMemory = &array->objects[array->numberOfObjects * array->objectSize];
	array->numberOfObjects++;
	return newMemory;
}

void *csAllocatingArray_getCArray(void *opaqueArray, unsigned int *numberOfObjects)
{
	CSAllocatingArray *const array = (CSAllocatingArray *)opaqueArray;
	*numberOfObjects = array->numberOfObjects;
	return array->objects;
}
