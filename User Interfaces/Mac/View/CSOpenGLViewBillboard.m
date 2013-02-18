//
//  CSOpenGLViewBillboard.m
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#import "CSOpenGLViewBillboard.h"
#import <OpenGL/gl.h>

@implementation CSOpenGLViewBillboard

@synthesize textureID;
@synthesize keyDelegate;

@synthesize minimumSourceRect;
@synthesize idealSourceRect;
@synthesize maximumSourceRect;
@synthesize pixelSizeOfIdealRect;

- (id)initWithCoder:(NSCoder *)aDecoder
{
	self = [super initWithCoder:aDecoder];

	if(self)
	{
		[self.openGLContext makeCurrentContext];

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glScalef(3.0f / 4.0f, 1.0f, 1.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// if we have a retina display then because OpenGL is a low-level fragment-oriented
	// API we need to deal with scaling up ourselves; we'll adjust the viewport, taking
	// advantage of convertPointToBacking: if available
	NSPoint farEdge = NSMakePoint(self.bounds.size.width, self.bounds.size.height);
	if([self respondsToSelector:@selector(convertPointToBacking:)])
		farEdge = [self convertPointToBacking:farEdge];

	glViewport(0, 0, (GLsizei)farEdge.x, (GLsizei)farEdge.y);

	NSRect outputRect = minimumSourceRect;

		// drawing here
		glPushMatrix();

			glColor3f(1.0, 1.0, 1.0);

			glBegin(GL_QUADS);
				glTexCoord2f((GLfloat)outputRect.origin.x, (GLfloat)(outputRect.origin.y + outputRect.size.height));
				glVertex2f(-4.0f / 3.0f, -1.0);

				glTexCoord2f((GLfloat)outputRect.origin.x, (GLfloat)outputRect.origin.y);
				glVertex2f(-4.0f / 3.0f, 1.0);

				glTexCoord2f((GLfloat)(outputRect.origin.x + outputRect.size.width), (GLfloat)outputRect.origin.y);
				glVertex2f(4.0f / 3.0f, 1.0);

				glTexCoord2f((GLfloat)(outputRect.origin.x + outputRect.size.width), (GLfloat)(outputRect.origin.y + outputRect.size.height));
				glVertex2f(4.0f / 3.0f, -1.0);
			glEnd();
		glPopMatrix();

	glSwapAPPLE();
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)keyUp:(NSEvent *)theEvent
{
	[keyDelegate keyUp:theEvent];
}

- (void)keyDown:(NSEvent *)theEvent
{
	[keyDelegate keyDown:theEvent];
}

- (void)flagsChanged:(NSEvent *)theEvent
{
	[keyDelegate flagsChanged:theEvent];
}

@end
