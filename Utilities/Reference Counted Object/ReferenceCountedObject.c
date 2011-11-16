//
//  ReferenceCountedObject.c
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "ReferenceCountedObject.h"
#include <stdlib.h>

void csObject_init(void *object)
{
	((CSReferenceCountedObject *)object)->retainCount = 1;
	((CSReferenceCountedObject *)object)->dealloc = NULL;
}

void *csObject_retain(void *object)
{
	// retaining a NULL object results in the NULL object
	if(!object) return object;

	((CSReferenceCountedObject *)object)->retainCount++;
	return object;
}

void csObject_release(void *opaqueObject)
{
	// it's acceptable to release a NULL object
	if(!opaqueObject) return;

	CSReferenceCountedObject *object = (CSReferenceCountedObject *)opaqueObject;

	object->retainCount--;
	if(!object->retainCount)
	{
		if(object->dealloc) object->dealloc(opaqueObject);
		free(object);
	}
}

const char *csObject_description(void *object)
{
	if(!object) return "(null)";
	return csObject_description(object);
}
