//
//  ZX80Document.h
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <AudioToolbox/AudioToolbox.h>
#import "CSOpenGLViewBillboard.h"
#import "Z80DebugInterface.h"
#import "OwnThreadTimer.h"

#define kZX80DocumentNumAudioBuffers	3
#define kZX80DocumentAudioStreamLength	16384
#define kZX80DocumentAudioBufferLength	2048

@interface ZX80Document : NSDocument <CSKeyDelegate, Z80DebugDrawerDelegate>
{
	// For the 32bit users out there; let's offend DRY even
	// more than usual, put implementation specified into the
	// interface file and invoke fragile base class problems
	@private

		CSOpenGLViewBillboard *openGLView;
		NSDrawer *machineOptionsDrawer;
		NSSlider *speedSlider;
		NSTextField *speedLabel;
		unsigned int speedMultiplier;
		NSMatrix *machineMatrix;
		NSMatrix *RAMMatrix;
		NSMatrix *ROMMatrix;
		NSButton *pauseOrPlayButton;

		CSOwnThreadTimer *timer;
		BOOL isRunning;
		unsigned int lastInternalTime;

		GLuint textureID;

		void *ULA;

		Z80DebugInterface *debugInterface;
		void *tape;

		AudioQueueRef audioQueue;
		AudioQueueBufferRef audioBuffers[kZX80DocumentNumAudioBuffers];
		unsigned int audioStreamReadPosition, audioStreamWritePosition, queuedAudioStreamSegments;
		short audioStream[kZX80DocumentAudioStreamLength];
		BOOL isOutputtingAudio;

		BOOL fastLoad;

		int instructionRunningCount;
		BOOL atBreakpoint;
		uint16_t targetAddress;
}

@property (nonatomic, retain) IBOutlet CSOpenGLViewBillboard *openGLView;
//@property (nonatomic, retain) IBOutlet NSPanel *linePanel;

@property (nonatomic, assign) IBOutlet NSDrawer *machineOptionsDrawer;

- (IBAction)showDebugger:(id)sender;
- (IBAction)showMachineDrawer:(id)sender;

@property (nonatomic, assign) IBOutlet NSSlider *speedSlider;
@property (nonatomic, assign) IBOutlet NSTextField *speedLabel;
@property (nonatomic, assign) unsigned int speedMultiplier;
- (IBAction)setNormalSpeed:(id)sender;

- (IBAction)reconfigureMachine:(id)sender;
@property (nonatomic, assign) IBOutlet NSMatrix *machineMatrix;
@property (nonatomic, assign) IBOutlet NSMatrix *RAMMatrix;
@property (nonatomic, assign) IBOutlet NSMatrix *ROMMatrix;

- (IBAction)pauseOrPlayTape:(id)sender;
@property (nonatomic, assign) IBOutlet NSButton *pauseOrPlayButton;

@property (nonatomic, assign) BOOL fastLoad;

@end
