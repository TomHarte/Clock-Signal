//
//  ParallelDispatch.h
//  Clock Signal
//
//  Created by Thomas Harte on 09/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef Clock_Signal_ParallelDispatch_h
#define Clock_Signal_ParallelDispatch_h

#ifdef __BLOCKS__
#include <dispatch/dispatch.h>

#define csdispatch_dispatchToQueue(queue, x)	\
	dispatch_async(queue, ^{x})

#define csdispatch_dispatch(x)	\
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{x})

#define csdispatch_dispatchToUIThread(x)	\
	dispatch_async(dispatch_get_main_queue(), ^{x})

#define csdispatch_queue dispatch_queue_t

#define csdispatch_createQueue(label)	\
	dispatch_queue_create(label, DISPATCH_QUEUE_SERIAL)

#define csdispatch_releaseQueue(queue)	\
	dispatch_release(queue)

#define csdispatch_waitForQueueToEmpty(queue)	\
	dispatch_sync(queue, ^{})

#else

#define csdispatch_dispatchToQueue(queue, x)	x
#define csdispatch_dispatch(x)					x
#define csdispatch_dispatchToUIThread(x)		x

#define csdispatch_queue 						// no storage
#define csdispatch_createQueue(label)			// do nothing
#define csdispatch_releaseQueue(queue)			// do nothing
#define csdispatch_waitForQueueToEmpty(queue)	// do nothing

#endif


#endif
