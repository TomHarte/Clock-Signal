//
//  LineGraph.h
//  Clock Signal
//
//  Created by Thomas Harte on 08/11/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface CSLineGraph : NSView
{
	NSUInteger graphValue;
}

- (void)pushBit:(unsigned int)value;


@end
