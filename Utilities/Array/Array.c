//
//  Array.c
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "Array.h"
#include "ReferenceCountedObject.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
	for(unsigned int c = 0; c < array->numberOfObjects; c++)
	{
		array->release(array->objects[c]);
	}
	free(array->objects);
}

static char *csArray_description(void *opaqueArray)
{
	// we'll want to grow this storage dynamically, so
	// frame things in that context
	size_t returnStringLength = 3;
	char *returnString = (char *)malloc(returnStringLength);
	char *outputPointer = returnString;

	// open with a curly brace
	outputPointer[0] = '{';
	outputPointer[1] = '\n';
	outputPointer += 2;

	// iterate through the contents, getting descriptions
	// for them where they report it
	unsigned int numberOfObjects;
	void **objects = csArray_getCArray(opaqueArray, &numberOfObjects);
	for(unsigned int c = 0; c < numberOfObjects; c++)
	{
		char *newDescription = csObject_copyDescription(objects[c]);

		while((size_t)(outputPointer - returnString) + strlen(newDescription) + 5 > returnStringLength)
		{
			// output pointer is possibly about to end up
			// pointing off into space, so we'll need to
			// be able to fix that
			size_t outputOffset = (size_t)(outputPointer - returnString);

			// ask for a double sized buffer
			size_t newLength = returnStringLength + 256;
			char *newString = (char *)realloc(returnString, newLength);

			// return what we have if the extra storage isn't available
			if(!newString) return returnString;

			// otherwise update our outlook
			returnString = newString;
			returnStringLength = newLength;
			outputPointer = returnString + outputOffset;
		}

		// append the new description
		sprintf(outputPointer, "\t%s\n", newDescription);
		outputPointer += strlen(outputPointer);
	}
	outputPointer[0] = '}';
	outputPointer[1] = '\0';

	return returnString;
}

void *csArray_create(bool retainObjects)
{
	CSRetainedArray *array = (CSRetainedArray *)calloc(1, sizeof(CSRetainedArray));

	if(array)
	{
		csObject_init(array);
		array->referenceCountedObject.dealloc = csArray_destroy;
		array->referenceCountedObject.copyDescription = csArray_description;

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

bool csArray_addObject(void *restrict opaqueArray, void *restrict object)
{
	CSRetainedArray *const restrict array = (CSRetainedArray *)opaqueArray;

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

bool csArray_removeObject(void *restrict opaqueArray, void *restrict object)
{
	CSRetainedArray *const restrict array = (CSRetainedArray *)opaqueArray;
	bool foundObject = false;

	// do a linear search for the object
	for(unsigned int locationOfObject = 0; locationOfObject < array->numberOfObjects; locationOfObject++)
	{
		if(array->objects[locationOfObject] == object)
		{
			// we've found an instance the object, so remove it
			array->release(object);
			array->numberOfObjects--;

			memmove(&array->objects[locationOfObject], &array->objects[locationOfObject+1], (size_t)(array->numberOfObjects - locationOfObject)*sizeof(void *));
		}
	}

	// we didn't find the object, so we failed to remove it
	return foundObject;
}

void csArray_removeAllObjects(void *opaqueArray)
{
	CSRetainedArray *array = (CSRetainedArray *)opaqueArray;

	for(unsigned int locationOfObject = 0; locationOfObject < array->numberOfObjects; locationOfObject++)
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

void **csArray_getCArray(void *restrict opaqueArray, unsigned int *restrict numberOfObjects)
{
	CSRetainedArray *const restrict array = (CSRetainedArray *)opaqueArray;
	*numberOfObjects = array->numberOfObjects;
	return array->objects;
}
