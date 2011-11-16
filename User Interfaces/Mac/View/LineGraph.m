//
//  LineGraph.m
//  Clock Signal
//
//  Created by Thomas Harte on 08/11/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#import "LineGraph.h"

@implementation CSLineGraph

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];

	if(self)
	{
		graphValue = -1;
	}

	return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    // get the view's bounds
	NSRect bounds = [self bounds];
	
	// set a black background
	[[NSColor blackColor] set];
	NSRectFill(bounds);

	// add a grey band
//	[[NSColor colorWithDeviceWhite:0.25 alpha:1.0] set];
//	NSRectFill(CGRectMake(0.0, 2.0, bounds.size.width, bounds.size.height-4.0));

	// get context
	CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];

	// draw grid, in grey
	[[NSColor grayColor] set];
	CGFloat currentX = bounds.size.width;
	CGContextSetLineWidth(context, 1.0);

	CGContextBeginPath(context);
	while(currentX >= 0.0)
	{
		CGContextMoveToPoint(context, currentX, 0.0);
		CGContextAddLineToPoint(context, currentX, bounds.size.height);
		currentX -= 10;
	}
	CGContextStrokePath(context);

	// draw lines, in
	[[NSColor greenColor] set];

	// 2 points wide with rounded ends
	CGContextSetLineWidth(context, bounds.size.height >= 14.0 ? 2.0 : 1.0);
	CGContextSetLineCap(context, kCGLineCapRound);

	// figure out the scale
	CGFloat lowY = 2.0;
	CGFloat highY = bounds.size.height - 2.0;
	currentX = bounds.size.width;

	// draw a digital value, thusly...
	NSUInteger value = graphValue;

	CGContextBeginPath(context);

	CGContextMoveToPoint(context, currentX, (value&1) ? highY : lowY);
	while(currentX >= 0.0)
	{
		CGContextAddLineToPoint(context, currentX, (value&1) ? highY : lowY);
		currentX -= 10;
		CGContextAddLineToPoint(context, currentX, (value&1) ? highY : lowY);
		value >>= 1;
	}

	CGContextStrokePath(context);
}

- (void)pushBit:(unsigned int)value
{
	graphValue = (graphValue << 1) | (value ? 1 : 0);
//	[self setNeedsDisplay:YES];
}

@end
