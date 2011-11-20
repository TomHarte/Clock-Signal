//
//  ReferenceCountedObject.c
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "ReferenceCountedObject.h"
#include <stdlib.h>
#include <string.h>

void csObject_init(void *object)
{
	// the initial retain count is 1, and we set dealloc to
	// NULL explicitly, at least until I'm sure I've stuck
	// to the calloc convention everywhere
	((CSReferenceCountedObject *)object)->retainCount = 1;
	((CSReferenceCountedObject *)object)->dealloc = NULL;
}

void *csObject_retain(void *object)
{
	// retaining a NULL object results in the NULL object
	if(!object) return object;

	// for all other objects, it increments the retain count
	((CSReferenceCountedObject *)object)->retainCount++;
	return object;
}

void csObject_release(void *opaqueObject)
{
	// it's acceptable to release a NULL object in our semantics
	if(!opaqueObject) return;

	CSReferenceCountedObject *object = (CSReferenceCountedObject *)opaqueObject;

	// decrement the retain count, deallocate the object if
	// the retain count is now zero
	object->retainCount--;
	if(!object->retainCount)
	{
		if(object->dealloc) object->dealloc(opaqueObject);
		free(object);
	}
}

char *csObject_copyDescription(void *opaqueObject)
{
	// if there's no object, then return "(null)"
	CSReferenceCountedObject *object = (CSReferenceCountedObject *)opaqueObject;
	if(!object) return strdup("(null)");

	// otherwise return the object's description if it provides one,
	// otherwise its type if available,
	// otherwise a string indicating that the description is unavailable
	if(object->copyDescription) return object->copyDescription(object);
	if(object->type) return strdup(object->type);
	return strdup("Unknown object");
}

void csObject_printDescription(void *object, FILE *outputstream)
{
	// get the description, output it to the stream, free it
	void *description = csObject_copyDescription(object);
	fputs(description, outputstream);
	fputc('\n', outputstream);
	free(description);
}
