//
//  Array.c
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "Array.h"
#include "ReferenceCountedObject.h"
#include <stdlib.h>
#include <string.h>

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	void **objects;
	unsigned int numberOfObjects, numberOfAllocatedObjects;

	void *(* retain)(void *);
	void (* release)(void *);

} CSRetainedArray;

static void *noOp_retain(void *p)
{
	return p;
}

static void noOp_release(void *p)
{
}

static void csArray_destroy(void *opaqueArray)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	// release all objects, then deallocate the storage
	for(int c = 0; c < array->numberOfObjects; c++)
	{
		array->release(array->objects[c]);
	}
	free(array->objects);
}

void *csArray_create(bool retainObjects)
{
	CSRetainedArray *array = (CSRetainedArray *)calloc(1, sizeof(CSRetainedArray));

	if(array)
	{
		csObject_init(array);
		array->referenceCountedObject.dealloc = csArray_destroy;

		if(retainObjects)
		{
			array->retain = csObject_retain;
			array->release = csObject_release;
		}
		else
		{
			array->retain = noOp_retain;
			array->release = noOp_release;
		}
	}

	return array;
}

bool csArray_addObject(void *opaqueArray, void *object)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	// check whether we have enough storage available
	if(array->numberOfObjects == array->numberOfAllocatedObjects)
	{
		void **newBuffer;
		unsigned int newNumberOfAllocatedObjects = (array->numberOfAllocatedObjects << 1) + 1;

		// if not then ask for some more
		newBuffer = (void **)realloc(array->objects, sizeof(void *)*newNumberOfAllocatedObjects);

		// if that fails we can't insert the new object
		if(!newBuffer) return false;

		// if that succeeded then update our array struct
		array->objects = newBuffer;
		array->numberOfAllocatedObjects = newNumberOfAllocatedObjects;
	}

	// store the object, return success
	array->objects[array->numberOfObjects] = array->retain(object);
	array->numberOfObjects++;

	return true;
}

bool csArray_removeObject(void *opaqueArray, void *object)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;
	bool foundObject = false;

	// do a linear search for the object
	for(int locationOfObject = 0; locationOfObject < array->numberOfObjects; locationOfObject++)
	{
		if(array->objects[locationOfObject] == object)
		{
			// we've found an instance the object, so remove it
			array->release(object);
			array->numberOfObjects--;

			memmove(&array->objects[locationOfObject], &array->objects[locationOfObject+1], (array->numberOfObjects - locationOfObject)*sizeof(void *));
		}
	}

	// we didn't find the object, so we failed to remove it
	return foundObject;
}

void csArray_removeAllObjects(void *opaqueArray)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	for(int locationOfObject = 0; locationOfObject < array->numberOfObjects; locationOfObject++)
		array->release(array->objects[locationOfObject]);

	array->numberOfObjects = 0;
}

void *csArray_getObjectAtIndex(void *opaqueArray, unsigned int index)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	if(index >= array->numberOfObjects) return NULL;
	return array->objects[index];
}

bool csArray_removeObjectAtIndex(void *opaqueArray, unsigned int index)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	if(index >= array->numberOfObjects) return false;
	array->release(array->objects[index]);
	array->numberOfObjects--;
	memmove(&array->objects[index], &array->objects[index+1], (array->numberOfObjects - index)*sizeof(void *));

	return true;
}

void **csArray_getCArray(void *opaqueArray, unsigned int *numberOfObjects)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;
	*numberOfObjects = array->numberOfObjects;
	return array->objects;
}
