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
	GLuint _arrayBuffer;
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

		glMatrixMode(GL_TEXTURE);
		glScalef(1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f);

		glColor3f(1.0, 1.0, 1.0);

		[self setMinimumSourceRect:NSMakeRect(0.0, 0.0, 1.0, 1.0)];

		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glVertexPointer(2, GL_SHORT, 4 * sizeof(GLshort), (void *)0);
		glTexCoordPointer(2, GL_SHORT, 4 * sizeof(GLshort), (void *)4);
	}

	return self;
}

#pragma mark - Setters

- (void)setMinimumSourceRect:(NSRect)minimumSourceRect
{
	[self.openGLContext makeCurrentContext];
	_minimumSourceRect = minimumSourceRect;
	
	if(!_arrayBuffer)
		glGenBuffers(1, &_arrayBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, _arrayBuffer);
	GLshort coordinates[] =
	{
		-1, -1,
		(GLshort)(32767.0f * (GLfloat)minimumSourceRect.origin.x),
		(GLshort)(32767.0f * (GLfloat)(minimumSourceRect.origin.y + minimumSourceRect.size.height)),

		-1, 1,
		(GLshort)(32767.0f * (GLfloat)minimumSourceRect.origin.x),
		(GLshort)(32767.0f * (GLfloat)minimumSourceRect.origin.y),

		1, 1,
		(GLshort)(32767.0f * (GLfloat)(minimumSourceRect.origin.x + minimumSourceRect.size.width)),
		(GLshort)(32767.0f * (GLfloat)minimumSourceRect.origin.y),

		1, -1,
		(GLshort)(32767.0f * (GLfloat)(minimumSourceRect.origin.x + minimumSourceRect.size.width)),
		(GLshort)(32767.0f * (GLfloat)(minimumSourceRect.origin.y + minimumSourceRect.size.height))
	};
	glBufferData(GL_ARRAY_BUFFER_ARB, sizeof(coordinates), coordinates, GL_STATIC_DRAW);
}

- (void)setTextureID:(GLuint)textureID
{
	if(_textureID == textureID) return;

	[self.openGLContext makeCurrentContext];
	_textureID = textureID;
	glBindTexture(GL_TEXTURE_2D, _textureID);
}

#pragma mark - Drawing

- (void)reshape
{
	// if we have a retina display then because OpenGL is a low-level fragment-oriented
	// API we need to deal with scaling up ourselves; we'll adjust the viewport, taking
	// advantage of convertPointToBacking: if available
	NSPoint farEdge = NSMakePoint(self.bounds.size.width, self.bounds.size.height);
	if([self respondsToSelector:@selector(convertPointToBacking:)])
		farEdge = [self convertPointToBacking:farEdge];
	
	[self.openGLContext makeCurrentContext];
	glViewport(0, 0, (GLsizei)farEdge.x, (GLsizei)farEdge.y);
}

- (void)drawRect:(NSRect)dirtyRect
{
	[self.openGLContext makeCurrentContext];
	glDrawArrays(GL_QUADS, 0, 4);
	glSwapAPPLE();
}

#pragma mark - NSResponder

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
