//
//  ZX80Document.m
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#import "ZX80Document.h"

#import "Z80Internals.h"
#include "ZX8081.h"
#import "Z80DebugInterface.h"

#include "CRT.h"
#include "ZX80Tape.h"
#include "TapePlayer.h"

#import <OpenGL/gl.h>
#import <AudioToolbox/AudioToolbox.h>

#import "CSOpenGLViewBillboard.h"


typedef enum
{
	ZX80DocumentRunningStatePaused,
	ZX80DocumentRunningStateRunning,
} ZX80DocumentRunningState;

@interface ZX80Document () <CSKeyDelegate, Z80DebugInterfaceDelegate>

@property (nonatomic, weak) IBOutlet CSOpenGLViewBillboard *openGLView;
@property (nonatomic, weak) IBOutlet NSDrawer *machineOptionsDrawer;
@property (nonatomic, weak) IBOutlet NSSlider *speedSlider;
@property (nonatomic, weak) IBOutlet NSTextField *speedLabel;
@property (nonatomic, weak) IBOutlet NSMatrix *machineMatrix;
@property (nonatomic, weak) IBOutlet NSMatrix *RAMMatrix;
@property (nonatomic, weak) IBOutlet NSMatrix *ROMMatrix;
@property (nonatomic, weak) IBOutlet NSButton *pauseOrPlayButton;

@property (nonatomic, assign) BOOL shouldFastLoad;

@end

@implementation ZX80Document
{
	dispatch_queue_t _serialDispatchQueue;

	Z80DebugInterface *_debugInterface;
	void *_ULA;
	void *_tape;

	AudioQueueRef _audioQueue;
	AudioQueueBufferRef _audioBuffers[kZX80DocumentNumAudioBuffers];
	unsigned int _audioStreamReadPosition, _audioStreamWritePosition, _queuedAudioStreamSegments;
	short _audioStream[kZX80DocumentAudioStreamLength];
	BOOL _isOutputtingAudio;

	int _instructionRunningCount;
	BOOL _atBreakpoint;
	uint16_t _targetAddress;

	float _speedMultiplier;
	GLuint _textureID;

	NSConditionLock *_runningLock;
	BOOL _isRunning;
}

#pragma mark -
#pragma mark Standard lifecycle

- (id)init
{
    self = [super init];

    if (self)
	{
		// set a default speed of 100%
		_speedMultiplier = 100.0f;

		// create a means to output audio
		[self setupAudioQueue];

		// restore
		_shouldFastLoad = [[NSUserDefaults standardUserDefaults] boolForKey:@"zx80.enableFastLoading"];

		// build a locking mechanism for cross-queue communications
		_runningLock = [[NSConditionLock alloc] initWithCondition:ZX80DocumentRunningStatePaused];
    }

    return self;
}

- (void)close
{
	[self stopRunning];
	if(_serialDispatchQueue)
	{
		dispatch_release(_serialDispatchQueue);
		_serialDispatchQueue = NULL;
	}
	[super close];
}

- (void)dealloc
{
	[self stopRunning];
	[self.openGLView.openGLContext makeCurrentContext];
	glDeleteTextures(1, &_textureID);

	if(_tape)
		cstape_release(_tape);

	csObject_release(_ULA);

	if(_audioQueue)
	{
		AudioQueueDispose(_audioQueue, NO);

		// per the documentation, this should dispose of any
		// buffers tied to the queue also, so there's no need
		// to do anything explicit about the audioBuffers
	}

}

#pragma mark -
#pragma mark NSDocument overrides

- (NSString *)windowNibName
{
	// Override returning the nib file name of the document
	// If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
	return @"ZX80Document";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
	[super windowControllerDidLoadNib:aController];

	// constrain the window to 4:3 and establish button state
	[aController.window setContentAspectRatio:NSSizeFromCGSize( CGSizeMake(4.0f, 3.0f) )];
	[self setDefaultConfiguration];

	// we'll want to receive keyboard input
	self.openGLView.keyDelegate = self;

	// get a Z80 debug interface and open our drawer
	_debugInterface = [Z80DebugInterface debugInterface];
	_debugInterface.delegate = self;
	[self.machineOptionsDrawer openOnEdge:NSMaxXEdge];

	// create a machine with the default configuration
	[self reconfigureMachine:nil];
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError
{
	/*
	 Insert code here to write your document to data of the specified type. If outError != NULL, ensure that you create and set an appropriate error when returning nil.
	You can also choose to override -fileWrapperOfType:error:, -writeToURL:ofType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.
	*/
	NSLog(@"saving type %@", typeName);

	return [NSData data];
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError
{
	/*
	Insert code here to read your document from the given data of the specified type. If outError != NULL, ensure that you create and set an appropriate error when returning NO.
	You can also choose to override -readFromFileWrapper:ofType:error: or -readFromURL:ofType:error: instead.
	If you override either of these, you should also override -isEntireFileLoaded to return NO if the contents are lazily loaded.
	*/

	if(
		[typeName isEqualToString:@"com.zx80.tapeimage"] ||
		[typeName isEqualToString:@"com.zx81.tapeimage"])
	{
		// create the tape
		_tape = cszx80tape_createWithData([data bytes], (unsigned int)[data length], [typeName isEqualToString:@"com.zx81.tapeimage"]);

		NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
		if([data length] < 650)
			[userDefaults setInteger:1024 forKey:@"zx80.amountOfRAM"];
		else
		{
			if([data length] < 2664)
				[userDefaults setInteger:3072 forKey:@"zx80.amountOfRAM"];
			else
			{
				if([data length] < 16010)
					[userDefaults setInteger:16384 forKey:@"zx80.amountOfRAM"];
				else
					[userDefaults setInteger:65536 forKey:@"zx80.amountOfRAM"];
			}
		}

		if([typeName isEqualToString:@"com.zx81.tapeimage"])
		{
			[userDefaults setBool:YES forKey:@"zx80.machineIsZX81"];
			[userDefaults setBool:YES forKey:@"zx80.machineHasZX81ROM"];
		}
		else
		{
			[userDefaults setBool:NO forKey:@"zx80.machineIsZX81"];
			[userDefaults setBool:NO forKey:@"zx80.machineHasZX81ROM"];
		}
	}

	return YES;

}

+ (BOOL)autosavesInPlace
{
    return YES;
}

- (void)setDefaultConfiguration
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];

	[self.machineMatrix selectCellAtRow:[userDefaults boolForKey:@"zx80.machineIsZX81"] ? 1 : 0 column:0];

	switch([userDefaults integerForKey:@"zx80.amountOfRAM"])
	{
		case 1024:	[self.RAMMatrix selectCellAtRow:0 column:0];	break;
		case 3072:	[self.RAMMatrix selectCellAtRow:1 column:0];	break;
		case 16384:	[self.RAMMatrix selectCellAtRow:2 column:0];	break;
		default:	[self.RAMMatrix selectCellAtRow:3 column:0];	break;
	}

	[self.ROMMatrix selectCellAtRow:[userDefaults boolForKey:@"zx80.machineHasZX81ROM"] ? 1 : 0 column:0];
}

#pragma mark -
#pragma mark 'Timer' logic

- (void)startRunning
{
	if(_isRunning) return;
	_debugInterface.isRunning = YES;

	[_runningLock lockWhenCondition:ZX80DocumentRunningStatePaused];
	_isRunning = YES;
	[self scheduleNextRunForHalfField];
	[_runningLock unlockWithCondition:ZX80DocumentRunningStateRunning];
}

- (void)stopRunning
{
	if(!_isRunning) return;
	_debugInterface.isRunning = NO;

	[_runningLock lockWhenCondition:ZX80DocumentRunningStateRunning];
	_isRunning = NO;
	[_runningLock unlockWithCondition:ZX80DocumentRunningStatePaused];
}

- (void)scheduleNextRunForHalfField
{
	// the point here is: each dispatch is 0.01 seconds apart; i.e. there are 100 in a second;
	// i.e. given that each one runs for half a field, there are 50 fields in a second
	dispatch_after(
		dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.01 * NSEC_PER_SEC)),
		_serialDispatchQueue,
		^{
			[self runForHalfField];
		});
}

- (void)runForHalfField
{
	if([_runningLock tryLockWhenCondition:ZX80DocumentRunningStateRunning])
	{
		[self scheduleNextRunForHalfField];

		unsigned int cyclesToRunFor = (unsigned int)(_speedMultiplier * 325);
																	// because multiplier is
																	// a percentage there's a
																	// hidden multiply by 100 here

		llzx8081_runForHalfCycles(_ULA, cyclesToRunFor << 1);

		[_runningLock unlockWithCondition:ZX80DocumentRunningStateRunning];
	}
}

#pragma mark -
#pragma mark Z80 instruction observer

- (void)z80WillFetchInstruction
{
	_instructionRunningCount--;
	_atBreakpoint = (llz80_monitor_getInternalValue(llzx8081_getCPU(_ULA), LLZ80MonitorValuePCRegister) == _targetAddress);
}

static void ZX80DocumentInstructionObserverBreakIn(void *z80, void *context)
{
	[(__bridge ZX80Document *)context z80WillFetchInstruction];
}

/*static void llzx8081_Z80WillFetchNewInstruction(void *z80, void *context)
{
	LLZX80ULAState *ula = (LLZX80ULAState *)context;

	ula->instructionsToRun--;
	uint16_t pc = llz80_monitor_getInternalValue(z80, LLZ80MonitorValuePCRegister);
	if(
		(pc == ula->pcTarget) ||
		(pc >= ula->pcTarget && ula->lastPC < ula->pcTarget && (pc - ula->lastPC) < 3)
	)
	{
		llz80_stopRunning(z80);
		ula->instructionsToRun = 0;
	}

	ula->lastPC = pc;
}*/

#pragma mark -
#pragma mark Z80DebugDebugInterfaceDelegate

- (void *)z80ForDebugInterface:(Z80DebugInterface *)debugInterface
{
	return llzx8081_getCPU(_ULA);
}

- (void)debugInterfaceRunForHalfACycle:(Z80DebugInterface *)debugInterface
{
	[self stopRunning];

	[debugInterface addComment:@"----"];
	llzx8081_runForHalfCycles(_ULA, 1);

	[debugInterface refresh];
}

- (void)debugInterfaceRunForOneInstruction:(Z80DebugInterface *)debugInterface
{
	[self stopRunning];

	[debugInterface addComment:@"----"];
	_instructionRunningCount = 1;

	void *instructionObserver = llz80_monitor_addInstructionObserver([self z80ForDebugInterface:debugInterface], ZX80DocumentInstructionObserverBreakIn, (__bridge void *)(self));
	while(_instructionRunningCount)
	{
		llzx8081_runForHalfCycles(_ULA, 1);
		[debugInterface updateBus];
	}
	llz80_monitor_removeInstructionObserver([self z80ForDebugInterface:debugInterface], instructionObserver);

	[debugInterface refresh];
}

- (void)debugInterface:(Z80DebugInterface *)debugInterface runUntilAddress:(uint16_t)address
{
	[self stopRunning];

	[debugInterface addComment:@"----"];
	
	_targetAddress = address;
	_atBreakpoint = NO;

	int maxCycles = 3250000;
	void *instructionObserver = llz80_monitor_addInstructionObserver([self z80ForDebugInterface:debugInterface], ZX80DocumentInstructionObserverBreakIn, (__bridge void *)(self));
	while(!_atBreakpoint && maxCycles--)
	{
		llzx8081_runForHalfCycles(_ULA, 1);
		[debugInterface updateBus];
	}
	llz80_monitor_removeInstructionObserver([self z80ForDebugInterface:debugInterface], instructionObserver);

	[debugInterface refresh];
}

- (void)debugInterfaceRun:(Z80DebugInterface *)debugInterface
{
	[self startRunning];
	[debugInterface refresh];
}

- (void)debugInterfacePause:(Z80DebugInterface *)debugInterface
{
	[self stopRunning];
	[debugInterface refresh];
}

#pragma mark -
#pragma mark Keyboard Input

- (void)setKey:(unsigned short)key isPressed:(BOOL)isPressed
{
#define setKey(key) if(isPressed) llzx8081_setKeyDown(_ULA, key); else llzx8081_setKeyUp(_ULA, key);

	switch(key)
	{
		default:
			NSLog(@"unknown virtual key: %d\n", key);
		break;

		case 35:	setKey(LLZX8081VirtualKeyP);		break;	// p
		case 31:	setKey(LLZX8081VirtualKeyO);		break;	// o
		case 34:	setKey(LLZX8081VirtualKeyI);		break;	// i
		case 32:	setKey(LLZX8081VirtualKeyU);		break;	// u
		case 16:	setKey(LLZX8081VirtualKeyY);		break;	// y

		case 12:	setKey(LLZX8081VirtualKeyQ);		break;	// q
		case 13:	setKey(LLZX8081VirtualKeyW);		break;	// w
		case 14:	setKey(LLZX8081VirtualKeyE);		break;	// e
		case 15:	setKey(LLZX8081VirtualKeyR);		break;	// r
		case 17:	setKey(LLZX8081VirtualKeyT);		break;	// t

		case 0:		setKey(LLZX8081VirtualKeyA);		break;	// a
		case 1:		setKey(LLZX8081VirtualKeyS);		break;	// s
		case 2:		setKey(LLZX8081VirtualKeyD);		break;	// d
		case 3:		setKey(LLZX8081VirtualKeyF);		break;	// f
		case 5:		setKey(LLZX8081VirtualKeyG);		break;	// g

		case 18:	setKey(LLZX8081VirtualKey1);		break;	// 1
		case 19:	setKey(LLZX8081VirtualKey2);		break;	// 2
		case 20:	setKey(LLZX8081VirtualKey3);		break;	// 3
		case 21:	setKey(LLZX8081VirtualKey4);		break;	// 4
		case 23:	setKey(LLZX8081VirtualKey5);		break;	// 5

		case 29:	setKey(LLZX8081VirtualKey0);		break;	// 0
		case 25:	setKey(LLZX8081VirtualKey9);		break;	// 9
		case 28:	setKey(LLZX8081VirtualKey8);		break;	// 8
		case 26:	setKey(LLZX8081VirtualKey7);		break;	// 7
		case 22:	setKey(LLZX8081VirtualKey6);		break;	// 6

		case 36:	setKey(LLZX8081VirtualKeyEnter);	break;	// enter
		case 37:	setKey(LLZX8081VirtualKeyL);		break;	// l
		case 40:	setKey(LLZX8081VirtualKeyK);		break;	// k
		case 38:	setKey(LLZX8081VirtualKeyJ);		break;	// j
		case 4:		setKey(LLZX8081VirtualKeyH);		break;	// h

		case 49:	setKey(LLZX8081VirtualKeySpace);	break;	// space
		case 47:	setKey(LLZX8081VirtualKeyDot);		break;	// dot
		case 46:	setKey(LLZX8081VirtualKeyM);		break;	// m
		case 45:	setKey(LLZX8081VirtualKeyN);		break;	// n
		case 11:	setKey(LLZX8081VirtualKeyB);		break;	// b

		// shift doesn't generate an event, so it's
		// handled elsewhere by looking for a change
		// in modifiers
		case 6:		setKey(LLZX8081VirtualKeyZ);		break;	// z
		case 7:		setKey(LLZX8081VirtualKeyX);		break;	// x
		case 8:		setKey(LLZX8081VirtualKeyC);		break;	// c
		case 9:		setKey(LLZX8081VirtualKeyV);		break;	// v

		case 51:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKey0);	break;	// delete = shift + 0

		case 126:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKey7);	break;	// up = shift + 7
		case 125:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKey6);	break;	// down = shift + 6
		case 123:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKey5);	break;	// left = shift + 5
		case 124:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKey8);	break;	// right = shift + 8

		case 43:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKeyDot);	break;	// comma = shift + .

		case 24:	setKey(LLZX8081VirtualKeyShift);	setKey(LLZX8081VirtualKeyL);	break;	// equals = shift + l
	}

#undef setKey
} 

- (void)keyUp:(NSEvent *)event
{
	[self setKey:[event keyCode] isPressed:NO];
}

- (void)keyDown:(NSEvent *)event
{
	if([event modifierFlags] & NSCommandKeyMask) return;
	[self setKey:[event keyCode] isPressed:YES];
}

- (void)flagsChanged:(NSEvent *)event
{
	NSUInteger newModifiers = [event modifierFlags];

	if(newModifiers&NSShiftKeyMask)
		llzx8081_setKeyDown(_ULA, LLZX8081VirtualKeyShift);
	else
		llzx8081_setKeyUp(_ULA, LLZX8081VirtualKeyShift);
}

#pragma mark -
#pragma mark Display

static void ZX80DocumentCRTBreakIn(
	void *crt,
	unsigned int widthOfBuffer,
	unsigned int heightOfBuffer,
	LLCRTDisplayType bufferType,
	bool isOddField,
	void *buffer,
	void *context)
{
	ZX80Document *document = (__bridge ZX80Document *)context;

	NSDictionary *infoDictionary =
		@{@"widthOfBuffer": @(widthOfBuffer),
			@"heightOfBuffer": @(heightOfBuffer),
			@"buffer": [NSData dataWithBytes:buffer length:widthOfBuffer*heightOfBuffer]};

	dispatch_async(dispatch_get_main_queue(),
	^{
		[document updateDisplay:infoDictionary];
	});
}

- (void)updateDisplay:(NSDictionary *)details
{
	CSOpenGLViewBillboard *openGLBillboard = self.openGLView;

	[openGLBillboard.openGLContext makeCurrentContext];

	if(!_textureID)
	{
		GLuint newTextureID;
		glGenTextures(1, &newTextureID);
		glBindTexture(GL_TEXTURE_2D, newTextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		_textureID = newTextureID;

		// just show a middle portion of the raster display;
		// like a real TV
		openGLBillboard.minimumSourceRect =
			NSMakeRect(
				0.18f,
				0.075f,
				0.8f,
				0.8f);

		openGLBillboard.textureID = _textureID;
	}
	else
		glBindTexture(GL_TEXTURE_2D, _textureID);

	const void *buffer = [details[@"buffer"] bytes];
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_LUMINANCE,
		[details[@"widthOfBuffer"] intValue], [details[@"heightOfBuffer"] intValue],
		0,
		GL_LUMINANCE, GL_UNSIGNED_BYTE,
		buffer);

	[openGLBillboard setNeedsDisplay:YES];
}

#pragma mark -
#pragma mark AudioQueue callbacks and setup; for pushing audio out

- (void)audioQueue:(AudioQueueRef)theAudioQueue didCallbackWithBuffer:(AudioQueueBufferRef)buffer
{
	@synchronized(self)
	{
		if(_queuedAudioStreamSegments > 2) _isOutputtingAudio = YES;

		if(_isOutputtingAudio && _queuedAudioStreamSegments)
		{
			_queuedAudioStreamSegments--;
			memcpy(buffer->mAudioData, &_audioStream[_audioStreamReadPosition], buffer->mAudioDataByteSize);
			_audioStreamReadPosition = (_audioStreamReadPosition + kZX80DocumentAudioBufferLength)%kZX80DocumentAudioStreamLength;
		}
		else
		{
			memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
			_isOutputtingAudio = NO;
		}
		AudioQueueEnqueueBuffer(theAudioQueue, buffer, 0, NULL);
	}
}

static void audioOutputCallback(
	void *inUserData,
	AudioQueueRef inAQ,
	AudioQueueBufferRef inBuffer)
{
	[(__bridge ZX80Document *)inUserData audioQueue:inAQ didCallbackWithBuffer:inBuffer];
}

- (void)setupAudioQueue
{
	/*
	
		Describe a mono, 16bit, 44.1Khz audio format
	
	*/
	AudioStreamBasicDescription outputDescription;

	outputDescription.mSampleRate = 44100;

	outputDescription.mFormatID = kAudioFormatLinearPCM;
	outputDescription.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;

	outputDescription.mBytesPerPacket = 2;
	outputDescription.mFramesPerPacket = 1;
	outputDescription.mBytesPerFrame = 2;
	outputDescription.mChannelsPerFrame = 1;
	outputDescription.mBitsPerChannel = 16;

	outputDescription.mReserved = 0;

	// create an audio output queue along those lines
	AudioQueueNewOutput(
		&outputDescription,
		audioOutputCallback,
		(__bridge void *)(self),
		NULL,
		kCFRunLoopCommonModes,
		0,
		&_audioQueue);

	_audioStreamWritePosition = kZX80DocumentAudioBufferLength;
	UInt32 bufferBytes = kZX80DocumentAudioBufferLength * sizeof(short);

	int c = kZX80DocumentNumAudioBuffers;
	while(c--)
	{
		AudioQueueAllocateBuffer(_audioQueue, bufferBytes, &_audioBuffers[c]);
		memset(_audioBuffers[c]->mAudioData, 0, bufferBytes);
		_audioBuffers[c]->mAudioDataByteSize = bufferBytes;
		AudioQueueEnqueueBuffer(_audioQueue, _audioBuffers[c], 0, NULL);
	}

	AudioQueueStart(_audioQueue, NULL);
}

#pragma mark -
#pragma mark ULA audio callbacks; for receiving audio in

- (void)enqueueAudioBuffer:(short *)buffer numberOfSamples:(unsigned int)lengthInSamples
{
	@synchronized(self)
	{
		memcpy(&_audioStream[_audioStreamWritePosition], buffer, lengthInSamples * sizeof(short));
		_audioStreamWritePosition = (_audioStreamWritePosition + lengthInSamples)%kZX80DocumentAudioStreamLength;

		if(_queuedAudioStreamSegments == (kZX80DocumentAudioStreamLength/kZX80DocumentAudioBufferLength))
		{
			_audioStreamReadPosition = (_audioStreamReadPosition + lengthInSamples)%kZX80DocumentAudioStreamLength;
		}
		else
		{
			_queuedAudioStreamSegments++;
		}
	}
}

static void	ZX80DocumentAudioCallout(
	void *tapePlayer,
	unsigned int numberOfSamples,
	short *sampleBuffer,
	void *context)
{
	[(__bridge ZX80Document *)context enqueueAudioBuffer:sampleBuffer numberOfSamples:numberOfSamples];
}

- (void)setupAudioInput
{
	void *tapePlayer = llzx8081_getTapePlayer(_ULA);
	cstapePlayer_setAudioDelegate(tapePlayer, ZX80DocumentAudioCallout, 44100, 2048, (__bridge void *)(self));
}

#pragma mark -
#pragma mark IBActions

- (IBAction)showDebugger:(id)sender
{
	[_debugInterface show];
}

- (IBAction)showMachineDrawer:(id)sender
{
	[self.machineOptionsDrawer openOnEdge:NSMaxXEdge];
}

- (IBAction)reconfigureMachine:(id)sender
{
	[self stopRunning];

	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];

	// destroy any existing ULA and create a new one
	csObject_release(_ULA);
	_ULA = llzx8081_create();

	// set the machine type; if it's a ZX81 then
	// disabled the ROM selection and force the
	// ZX81 ROM
	switch(self.machineMatrix.selectedRow)
	{
		default:
			self.ROMMatrix.enabled = YES;
			llzx8081_setMachineType(_ULA, LLZX8081MachineTypeZX80);
			[userDefaults setBool:NO forKey:@"zx80.machineIsZX81"];
		break;
		case 1:
			[self.ROMMatrix selectCellAtRow:1 column:0];
			self.ROMMatrix.enabled = NO;
			llzx8081_setMachineType(_ULA, LLZX8081MachineTypeZX81);
			[userDefaults setBool:YES forKey:@"zx80.machineIsZX81"];
		break;
	}

	// set the requested amount of RAM
	switch(self.RAMMatrix.selectedRow)
	{
		default:
			llzx8081_setRAMSize(_ULA, LLZX8081RAMSize1Kb);
			[userDefaults setInteger:1024 forKey:@"zx80.amountOfRAM"];
		break;
		case 1:
			llzx8081_setRAMSize(_ULA, LLZX8081RAMSize2Kb);
			[userDefaults setInteger:3072 forKey:@"zx80.amountOfRAM"];
		break;
		case 2:
			llzx8081_setRAMSize(_ULA, LLZX8081RAMSize16Kb);
			[userDefaults setInteger:16384 forKey:@"zx80.amountOfRAM"];
		break;
		case 3:
			llzx8081_setRAMSize(_ULA, LLZX8081RAMSize64Kb);
			[userDefaults setInteger:65536 forKey:@"zx80.amountOfRAM"];
		break;
	}

	// load the requested ROM
	NSString *pathToROM;
	switch(self.ROMMatrix.selectedRow)
	{
		default:
			pathToROM = [[NSBundle mainBundle] pathForResource:@"zx80rom" ofType:@"bin"];
			[userDefaults setBool:NO forKey:@"zx80.machineHasZX81ROM"];
		break;
		case 1:
			pathToROM = [[NSBundle mainBundle] pathForResource:@"zx81rom" ofType:@"bin"];
			[userDefaults setBool:YES forKey:@"zx80.machineHasZX81ROM"];
		break;
	}
	NSData *contentsOfROM = [NSData dataWithContentsOfFile:pathToROM];
	llzx8081_provideROM(_ULA, [contentsOfROM bytes], (unsigned int)[contentsOfROM length]);

	// do some tidying up and set our outputter as the
	// end-of-field delegate for the CRT
	if(_textureID)
	{
		glDeleteTextures(1, &_textureID);
		_textureID = 0;
	}
	void *CRT = llzx8081_getCRT(_ULA);
	llcrt_setEndOfFieldDelegate(CRT, ZX80DocumentCRTBreakIn, (__bridge void *)(self));

	// hand over our tape
	if(_tape)
	{
		llzx8081_setTape(_ULA, _tape);
	}

	// set ourself to receive audio
	[self setupAudioInput];

	// configure tape button and fast loading state
	[self.pauseOrPlayButton setTitle:@"Play Tape"];
	llzx8081_setFastLoadingIsEnabled(_ULA, _shouldFastLoad);

	// create a queue on which to run the machine if we don't already have one and start running
	if(!_serialDispatchQueue)
		_serialDispatchQueue = dispatch_queue_create("Clock Signal emulationqueue", DISPATCH_QUEUE_SERIAL);
	[self startRunning];
}

- (IBAction)setNormalSpeed:(id)sender
{
	_speedMultiplier = 100.0f;
}

- (IBAction)pauseOrPlayTape:(id)sender
{
	void *tapePlayer = llzx8081_getTapePlayer(_ULA);

	if(cstapePlayer_isTapePlaying(tapePlayer))
	{
		cstapePlayer_pause(tapePlayer, llzx8081_getTimeStamp(_ULA));
		[self.pauseOrPlayButton setTitle:@"Play Tape"];
	}
	else
	{
		cstapePlayer_play(tapePlayer, llzx8081_getTimeStamp(_ULA));
		[self.pauseOrPlayButton setTitle:@"Pause Tape"];
	}
}

#pragma mark -
#pragma mark Cocoa Bindings bound properties

- (void)setShouldFastLoad:(BOOL)newFastLoad
{
	_shouldFastLoad = newFastLoad;

	llzx8081_setFastLoadingIsEnabled(_ULA, _shouldFastLoad);

	[[NSUserDefaults standardUserDefaults]
		setBool:newFastLoad forKey:@"zx80.enableFastLoading"];
}

@end
