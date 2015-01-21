//
//  AllocatingArray.h
//  Clock Signal
//
//  Created by Thomas Harte on 20/01/2015.
//  Copyright (c) 2015 acrossair. All rights reserved.
//

#ifndef __Clock_Signal__AllocatingArray__
#define __Clock_Signal__AllocatingArray__

#include <stdlib.h>

// An allocating array creates a contiguous, tightly
// packed (where possible) direct array of new memory blocks;
// it can only be added to.

void *csAllocatingArray_createWithObjectSize(size_t objectSize);

void *csAllocatingArray_newObject(void *array);
void *csAllocatingArray(void *array, unsigned int *numberOfObjects);

#endif /* defined(__Clock_Signal__AllocatingArray__) */
