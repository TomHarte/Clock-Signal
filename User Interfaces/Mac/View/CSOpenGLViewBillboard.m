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
{
//	GLuint _arrayBuffer;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
	self = [super initWithCoder:aDecoder];

	if(self)
	{
		[self.openGLContext makeCurrentContext];

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);

//		glGenBuffers(1, &_arrayBuffer);
//		glBindBuffer(GL_ARRAY_BUFFER, _arrayBuffer);
//		GLbyte vertices[] =
//		{
//			-1, -1, -1, 1,
//			1, 1, 1, -1
//		};
//		glBufferData(GL_ARRAY_BUFFER_ARB, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glColor3f(1.0, 1.0, 1.0);
	}

	return self;
}

- (void)reshape
{
	// if we have a retina display then because OpenGL is a low-level fragment-oriented
	// API we need to deal with scaling up ourselves; we'll adjust the viewport, taking
	// advantage of convertPointToBacking: if available
	NSPoint farEdge = NSMakePoint(self.bounds.size.width, self.bounds.size.height);
	if([self respondsToSelector:@selector(convertPointToBacking:)])
		farEdge = [self convertPointToBacking:farEdge];

	glViewport(0, 0, (GLsizei)farEdge.x, (GLsizei)farEdge.y);
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSRect outputRect = _minimumSourceRect;

	glBegin(GL_QUADS);
		glTexCoord2f((GLfloat)outputRect.origin.x, (GLfloat)(outputRect.origin.y + outputRect.size.height));
		glVertex2i(-1, -1);

		glTexCoord2f((GLfloat)outputRect.origin.x, (GLfloat)outputRect.origin.y);
		glVertex2i(-1, 1);

		glTexCoord2f((GLfloat)(outputRect.origin.x + outputRect.size.width), (GLfloat)outputRect.origin.y);
		glVertex2i(1, 1);

		glTexCoord2f((GLfloat)(outputRect.origin.x + outputRect.size.width), (GLfloat)(outputRect.origin.y + outputRect.size.height));
		glVertex2i(1, -1);
	glEnd();

	glSwapAPPLE();
}

- (void)setTextureID:(GLuint)textureID
{
	if(_textureID == textureID) return;

	[self.openGLContext makeCurrentContext];
	_textureID = textureID;
	glBindTexture(GL_TEXTURE_2D, _textureID);
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)keyUp:(NSEvent *)theEvent
{
	[self.keyDelegate keyUp:theEvent];
}

- (void)keyDown:(NSEvent *)theEvent
{
	[self.keyDelegate keyDown:theEvent];
}

- (void)flagsChanged:(NSEvent *)theEvent
{
	[self.keyDelegate flagsChanged:theEvent];
}

@end
