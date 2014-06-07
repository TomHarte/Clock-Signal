//
//  CSOpenGLViewBillboard.h
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

@protocol CSKeyDelegate <NSObject>

// a very small subset of NSResponder
- (void)keyDown:(NSEvent *)event;
- (void)keyUp:(NSEvent *)event;
- (void)flagsChanged:(NSEvent *)newModifiers;

@end

@interface CSOpenGLViewBillboard : NSOpenGLView
{
	// for the 32bit users out there ...
	@private
		id <CSKeyDelegate> __unsafe_unretained keyDelegate;
		GLuint textureID;
		NSRect minimumSourceRect;
		NSRect idealSourceRect;
		NSRect maximumSourceRect;
		NSSize pixelSizeOfIdealRect;
}

@property (nonatomic, unsafe_unretained) id <CSKeyDelegate> keyDelegate;

@property (nonatomic, assign) GLuint textureID;

@property (nonatomic, assign) NSRect minimumSourceRect;
@property (nonatomic, assign) NSRect idealSourceRect;
@property (nonatomic, assign) NSRect maximumSourceRect;

@property (nonatomic, assign) NSSize pixelSizeOfIdealRect;

@end
