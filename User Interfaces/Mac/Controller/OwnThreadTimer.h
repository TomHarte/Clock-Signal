//
//  OwnThreadTimer.h
//  Clock Signal
//
//  Created by Thomas Harte on 10/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface CSOwnThreadTimer : NSThread
{
	NSTimer *timer;
	BOOL quit;
}

- (id)initWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo;
- (void)invalidate;

@end
