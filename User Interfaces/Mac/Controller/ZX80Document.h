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
		NSDrawer *__weak machineOptionsDrawer;
		NSSlider *__weak speedSlider;
		NSTextField *__weak speedLabel;
		float speedMultiplier;
		NSMatrix *__weak machineMatrix;
		NSMatrix *__weak RAMMatrix;
		NSMatrix *__weak ROMMatrix;
		NSButton *__weak pauseOrPlayButton;

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

@property (nonatomic, strong) IBOutlet CSOpenGLViewBillboard *openGLView;
//@property (nonatomic, retain) IBOutlet NSPanel *linePanel;

@property (nonatomic, weak) IBOutlet NSDrawer *machineOptionsDrawer;

- (IBAction)showDebugger:(id)sender;
- (IBAction)showMachineDrawer:(id)sender;

@property (nonatomic, weak) IBOutlet NSSlider *speedSlider;
@property (nonatomic, weak) IBOutlet NSTextField *speedLabel;
@property (atomic, assign) float speedMultiplier;
- (IBAction)setNormalSpeed:(id)sender;

- (IBAction)reconfigureMachine:(id)sender;
@property (nonatomic, weak) IBOutlet NSMatrix *machineMatrix;
@property (nonatomic, weak) IBOutlet NSMatrix *RAMMatrix;
@property (nonatomic, weak) IBOutlet NSMatrix *ROMMatrix;

- (IBAction)pauseOrPlayTape:(id)sender;
@property (nonatomic, weak) IBOutlet NSButton *pauseOrPlayButton;

@property (nonatomic, assign) BOOL fastLoad;

@end
