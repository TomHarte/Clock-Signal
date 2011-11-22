//
//  OwnThreadTimer.m
//  Clock Signal
//
//  Created by Thomas Harte on 10/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#import "OwnThreadTimer.h"

@interface CSOwnThreadTimer ()
@property (assign) BOOL quit;	// i.e. atomic
@end

@implementation CSOwnThreadTimer

@synthesize quit;

- (id)initWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo
{
	self = [super init];
	if (self)
	{
		// create the timer here since it conveniently records the interval, target, selector and userInfo
		timer = 
			[[NSTimer
				timerWithTimeInterval:interval
				target:target
				selector:selector
				userInfo:userInfo
				repeats:YES] retain];

		// start the thread on which we'll run the timer
		[self start];
	}

	return self;
}

- (void)main
{
	// attach the timer to the runloop associated with this thread
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];

	// repeat while this own-thead timer hasn't been invalidated
	while(!self.quit)
	{
		// create an autorelease pool
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

		// run this thread's runloop for 2 seconds (arbitrarily)
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2.0]];

		// drain the pool
		[pool drain];
	}

	// release the timer
	[timer release];

	// that's the end for this whole class, so schedule its release
	[self performSelectorOnMainThread:@selector(autorelease) withObject:nil waitUntilDone:NO];
}

- (void)invalidate
{
	// invalidate the timer immediately so that no further calls out are made
	[timer performSelector:@selector(invalidate) onThread:self withObject:nil waitUntilDone:YES];

	// request that the thread stop pumping the runloop when next it thinks about it
	self.quit = YES;
}

@end
