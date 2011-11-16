//
//  OwnThreadTimer.m
//  Clock Signal
//
//  Created by Thomas Harte on 10/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#import "OwnThreadTimer.h"

@interface CSOwnThreadTimer ()
@property (assign) BOOL quit;
@end

@implementation CSOwnThreadTimer

@synthesize quit;

- (id)initWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo
{
	self = [super init];
	if (self)
	{
		timer = 
			[[NSTimer
				timerWithTimeInterval:interval
				target:target
				selector:selector
				userInfo:userInfo
				repeats:YES] retain];
	
		[self start];
	}

	return self;
}

- (void)main
{
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];

	while(!self.quit)
	{
//		NSLog(@"2s");
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2.0]];
	}

	[timer invalidate];
	[timer release];

	[self performSelectorOnMainThread:@selector(autorelease) withObject:nil waitUntilDone:NO];
}

- (void)invalidate
{
	[timer performSelector:@selector(invalidate) onThread:self withObject:nil waitUntilDone:YES];
	self.quit = YES;
}

@end
