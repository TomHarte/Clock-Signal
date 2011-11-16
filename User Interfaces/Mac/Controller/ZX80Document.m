//
//  ZX80Document.m
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#import "ZX80Document.h"
#import "Z80.h"
#import "Z80Internals.h"
#import "Z80DebugDrawer.h"
#include "ZX8081.h"
#include "CRT.h"
#import <OpenGL/gl.h>
#include "AbstractTape.h"
#include "ZX80Tape.h"
#include "TapePlayer.h"
#include "Z80Disassembler.h"

@interface ZX80Document ()
@property (assign) BOOL isRunning;
@property (assign) unsigned int lastInternalTime;

@property (nonatomic, assign) GLuint textureID;

@property (assign) void *ULA;

@property (nonatomic, retain) Z80DebugDrawer *debugDrawer;
@property (assign) void *tape;

@property (assign) AudioQueueRef audioQueue;

- (void)setDefaultConfiguration;

@end

@implementation ZX80Document

@synthesize openGLView;
@synthesize ULA;
@synthesize pauseOrPlayButton;

@synthesize isRunning;

@synthesize lastInternalTime;
@synthesize debugDrawer;
@synthesize machineOptionsDrawer;

@synthesize textureID;

@synthesize machineMatrix;
@synthesize RAMMatrix;
@synthesize ROMMatrix;
@synthesize tape;

@synthesize speedSlider;
@synthesize speedLabel;

@synthesize speedMultiplier;

@synthesize audioQueue;

@synthesize fastLoad;
//@synthesize fastLoadButton;

- (void)audioQueue:(AudioQueueRef)theAudioQueue didCallbackWithBuffer:(AudioQueueBufferRef)buffer
{
	@synchronized(self)
	{
		if(queuedAudioStreamSegments > 2) isOutputtingAudio = YES;

		if(isOutputtingAudio && queuedAudioStreamSegments)
		{
			queuedAudioStreamSegments--;
			memcpy(buffer->mAudioData, &audioStream[audioStreamReadPosition], buffer->mAudioDataByteSize);
			audioStreamReadPosition = (audioStreamReadPosition + kZX80DocumentAudioBufferLength)%kZX80DocumentAudioStreamLength;
		}
		else
		{
			memset(buffer->mAudioData, 0, buffer->mAudioDataByteSize);
			isOutputtingAudio = NO;
		}
		AudioQueueEnqueueBuffer(theAudioQueue, buffer, 0, NULL);
	}
}

- (void)enqueueAudioBuffer:(short *)buffer numberOfSamples:(int)lengthInSamples
{
	@synchronized(self)
	{
		memcpy(&audioStream[audioStreamWritePosition], buffer, lengthInSamples * sizeof(short));
		audioStreamWritePosition = (audioStreamWritePosition + lengthInSamples)%kZX80DocumentAudioStreamLength;

		if(queuedAudioStreamSegments == (kZX80DocumentAudioStreamLength/kZX80DocumentAudioBufferLength))
		{
			audioStreamReadPosition = (audioStreamReadPosition + lengthInSamples)%kZX80DocumentAudioStreamLength;
		}
		else
		{
			queuedAudioStreamSegments++;
		}
	}
}

static void audioOutputCallback(
	void *inUserData,
	AudioQueueRef inAQ,
	AudioQueueBufferRef inBuffer)
{
	[(ZX80Document *)inUserData audioQueue:inAQ didCallbackWithBuffer:inBuffer];
}

static void	ZX80DocumentAudioCallout(
	void *tapePlayer,
	int numberOfSamples,
	short *sampleBuffer,
	void *context)
{
	[(ZX80Document *)context enqueueAudioBuffer:sampleBuffer numberOfSamples:numberOfSamples];
}

- (id)init
{
    self = [super init];

    if (self)
	{
		// set a default speed of 100%
		self.speedMultiplier = 100.0f;

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
			self,
			NULL,
			kCFRunLoopCommonModes,
			0,
			&audioQueue);

		audioStreamWritePosition = kZX80DocumentAudioBufferLength;
		UInt32 bufferBytes = kZX80DocumentAudioBufferLength * sizeof(short);

		int c = kZX80DocumentNumAudioBuffers;
		while(c--)
		{
			AudioQueueAllocateBuffer(audioQueue, bufferBytes, &audioBuffers[c]);
			memset(audioBuffers[c]->mAudioData, 0, bufferBytes);
			audioBuffers[c]->mAudioDataByteSize = bufferBytes;
			AudioQueueEnqueueBuffer(audioQueue, audioBuffers[c], 0, NULL);
		}

		AudioQueueStart(audioQueue, NULL);

		fastLoad = [[NSUserDefaults standardUserDefaults] boolForKey:@"zx80.enableFastLoading"];
    }

    return self;
}

//- (BOOL)isDocumentEdited
//{
//	return YES;
//}

- (void)close
{
	[timer invalidate]; timer = nil;
	[super close];
}

- (void)doNothing
{
}

- (void)dealloc
{
	[openGLView.openGLContext makeCurrentContext];
	glDeleteTextures(1, &textureID);

	self.openGLView = nil;
	self.debugDrawer = nil;

	if (self.tape)
		cstape_release(self.tape);

	csObject_release(ULA);

	if(self.audioQueue)
	{
		AudioQueueDispose(self.audioQueue, NO);

		// per the documentation, this should dispose of any
		// buffers tied to teh queue also, so there's no need
		// to do anything explicit about the audioBuffers
	}

	[super dealloc];
}

- (NSString *)windowNibName
{
	// Override returning the nib file name of the document
	// If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
	return @"ZX80Document";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
	[super windowControllerDidLoadNib:aController];
	// Add any code here that needs to be executed once the windowController has loaded the document's window.

	[aController.window setContentAspectRatio:NSSizeFromCGSize( CGSizeMake(4.0f, 3.0f) )];
	[self setDefaultConfiguration];

	openGLView.keyDelegate = self;

	// get a Z80 debug drawer
	self.debugDrawer = [Z80DebugDrawer debugDrawer];

	debugDrawer.parentWindow = [self windowForSheet];
	debugDrawer.delegate = self;

	[machineOptionsDrawer openOnEdge:NSMaxXEdge];
//	[debugDrawer openOnEdge:NSMinXEdge];

//	[linePanel setIsVisible:YES];

	// create a machine with the default configuration
	[self reconfigureMachine:nil];
}

- (void *)z80ForDebugDrawer
{
	return llzx8081_getCPU(ULA);
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
		self.tape = cszx80tape_createWithData([data bytes], (unsigned int)[data length], [typeName isEqualToString:@"com.zx81.tapeimage"]);

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

- (void)runForHalfField:(NSTimer *)timer
{
	@synchronized(self)
	{
		if(self.isRunning)
		{
			unsigned int cyclesToRunFor = self.speedMultiplier * 325;	// because multiplier is
																		// a percentage there's a
																		// hidden multiply by 100 here

			llzx8081_runForHalfCycles(ULA, cyclesToRunFor << 1);
		}
	}
}

- (void)z80WillFetchInstruction
{
	instructionRunningCount--;
	atBreakpoint = (llz80_monitor_getInternalValue([self z80ForDebugDrawer], LLZ80MonitorValuePCRegister) == targetAddress);
}

static void ZX80DocumentInstructionObserverBreakIn(void *z80, void *context)
{
	[(ZX80Document *)context z80WillFetchInstruction];
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
- (void)debugDrawerRunForHalfACycle:(Z80DebugDrawer *)drawer
{
	@synchronized(self)
	{
		self.isRunning = NO;
		drawer.isRunning = NO;
	}

	[drawer addComment:@"----"];

	llzx8081_runForHalfCycles(ULA, 1);

	[drawer refresh];
}

- (void)debugDrawerRunForOneInstruction:(Z80DebugDrawer *)drawer
{
	@synchronized(self)
	{
		self.isRunning = NO;
		drawer.isRunning = NO;
	}

	[drawer addComment:@"----"];
	
	instructionRunningCount = 1;

	void *instructionObserver = llz80_monitor_addInstructionObserver([self z80ForDebugDrawer], ZX80DocumentInstructionObserverBreakIn, self);
	while(instructionRunningCount)
	{
		llzx8081_runForHalfCycles(ULA, 1);
		[drawer updateBus];
	}
	llz80_monitor_removeInstructionObserver([self z80ForDebugDrawer], instructionObserver);

	[drawer refresh];
}

- (void)debugDrawer:(Z80DebugDrawer *)drawer runUntilAddress:(uint16_t)address
{
	@synchronized(self)
	{
		self.isRunning = NO;
		drawer.isRunning = NO;
	}

	[drawer addComment:@"----"];
	
	targetAddress = address;
	atBreakpoint = NO;

	int maxCycles = 3250000;
	void *instructionObserver = llz80_monitor_addInstructionObserver([self z80ForDebugDrawer], ZX80DocumentInstructionObserverBreakIn, self);
	while(!atBreakpoint && maxCycles--)
	{
		llzx8081_runForHalfCycles(ULA, 1);
		[drawer updateBus];
	}
	llz80_monitor_removeInstructionObserver([self z80ForDebugDrawer], instructionObserver);

	[drawer refresh];
}

- (void)debugDrawerRun:(Z80DebugDrawer *)drawer
{
	@synchronized(self)
	{
		self.isRunning = YES;
		drawer.isRunning = YES;
	}
}

- (void)debugDrawerPause:(Z80DebugDrawer *)drawer
{
	@synchronized(self)
	{
		self.isRunning = NO;
		drawer.isRunning = NO;
		[drawer refresh];
	}
}

- (IBAction)showDebugDrawer:(id)sender
{
	[debugDrawer openOnEdge:NSMinXEdge];
}

- (IBAction)showMachineDrawer:(id)sender
{
	[self.machineOptionsDrawer openOnEdge:NSMaxXEdge];
}

- (void)setKey:(unsigned short)key isPressed:(BOOL)isPressed
{
#define setKey(key) if(isPressed) llzx8081_setKeyDown(ULA, key); else llzx8081_setKeyUp(ULA, key);

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
		llzx8081_setKeyDown(ULA, LLZX8081VirtualKeyShift);
	else
		llzx8081_setKeyUp(ULA, LLZX8081VirtualKeyShift);
}

static void ZX80DocumentCRTBreakIn(
	void *crt,
	int widthOfBuffer,
	int heightOfBuffer,
	LLCRTDisplayType bufferType,
	bool isOddField,
	void *buffer,
	void *context)
{
	ZX80Document *document = (ZX80Document *)context;

/*	uint8_t *fatBuffer = (uint8_t *)malloc(widthOfBuffer*2*heightOfBuffer);
	for(int y = 0; y < heightOfBuffer; y++)
	{
		uint8_t *sourcePointer = &buffer[y * widthOfBuffer];
		uint8_t *targetPointer = &fatBuffer[y * widthOfBuffer * 2];

		for(int x = 0; x < widthOfBuffer-1; x++)
		{
			uint8_t sourceValue = sourcePointer[x];
			uint8_t quarterValue = sourceValue >> 2;
			uint8_t threeQuarterValue = sourceValue - quarterValue;

			int tx = x << 1;
			targetPointer[tx] += quarterValue;
			targetPointer[tx+1] += threeQuarterValue;
			targetPointer[tx+2] = threeQuarterValue;
			targetPointer[tx+3] = quarterValue;
		}
	}
	NSDictionary *infoDictionary = 
		[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:widthOfBuffer*2], @"widthOfBuffer",
			[NSNumber numberWithInt:heightOfBuffer], @"heightOfBuffer",
			[NSValue valueWithPointer:fatBuffer], @"buffer",
			nil];*/

	uint8_t *bufferCopy = (uint8_t *)malloc(widthOfBuffer*heightOfBuffer);
	memcpy(bufferCopy, buffer, widthOfBuffer*heightOfBuffer);

	NSDictionary *infoDictionary = 
		[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:widthOfBuffer], @"widthOfBuffer",
			[NSNumber numberWithInt:heightOfBuffer], @"heightOfBuffer",
			[NSValue valueWithPointer:bufferCopy], @"buffer",
			nil];

	[document
		performSelectorOnMainThread:@selector(updateDisplay:)
		withObject:infoDictionary
		waitUntilDone:NO];
}

- (void)updateDisplay:(NSDictionary *)details
{
	CSOpenGLViewBillboard *openGLBillboard = self.openGLView;

	[openGLBillboard.openGLContext makeCurrentContext];

	if(!self.textureID)
	{
		GLuint newTextureID;
		glGenTextures(1, &newTextureID);
		glBindTexture(GL_TEXTURE_2D, newTextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);	// no mip mapping for you
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		self.textureID = newTextureID;

		openGLBillboard.minimumSourceRect =
//			NSMakeRect(
//				0.0f,
//				0.0f,
//				1.0f,
//				1.0f);
			NSMakeRect(
				0.18f,
				0.075f,
				0.8f,
				0.8f);

		openGLBillboard.textureID = self.textureID;
	}
	else
		glBindTexture(GL_TEXTURE_2D, self.textureID);

	void *buffer = [[details objectForKey:@"buffer"] pointerValue];
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_LUMINANCE,
		[[details objectForKey:@"widthOfBuffer"] intValue], [[details objectForKey:@"heightOfBuffer"] intValue],
		0,
		GL_LUMINANCE, GL_UNSIGNED_BYTE,
		buffer);

	free(buffer);

	glGenerateMipmap(GL_TEXTURE_2D);

	[openGLBillboard setNeedsDisplay:YES];
}

- (void)setDefaultConfiguration
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];

	[machineMatrix selectCellAtRow:[userDefaults boolForKey:@"zx80.machineIsZX81"] ? 1 : 0 column:0];

	switch([userDefaults integerForKey:@"zx80.amountOfRAM"])
	{
		case 1024:	[RAMMatrix selectCellAtRow:0 column:0];	break;
		case 3072:	[RAMMatrix selectCellAtRow:1 column:0];	break;
		case 16384:	[RAMMatrix selectCellAtRow:2 column:0];	break;
		default:	[RAMMatrix selectCellAtRow:3 column:0];	break;
	}

	[ROMMatrix selectCellAtRow:[userDefaults boolForKey:@"zx80.machineHasZX81ROM"] ? 1 : 0 column:0];
}

- (IBAction)reconfigureMachine:(id)sender
{
	[timer invalidate];
	timer = nil;

	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];

	// create the ULA
	csObject_release(ULA);
	ULA = llzx8081_create();

	NSLog(@"machine created");

	// set the machine type; if it's a ZX81 then
	// disabled the ROM selection and force the
	// ZX81 ROM
	switch(machineMatrix.selectedRow)
	{
		default:
			ROMMatrix.enabled = YES;
			llzx8081_setMachineType(ULA, LLZX8081MachineTypeZX80);
			[userDefaults setBool:NO forKey:@"zx80.machineIsZX81"];
		break;
		case 1:
			[ROMMatrix selectCellAtRow:1 column:0];
			ROMMatrix.enabled = NO;
			llzx8081_setMachineType(ULA, LLZX8081MachineTypeZX81);
			[userDefaults setBool:YES forKey:@"zx80.machineIsZX81"];
		break;
	}

	// set the requested amount of RAM
	switch(RAMMatrix.selectedRow)
	{
		default:
			llzx8081_setRAMSize(ULA, 1024);
			[userDefaults setInteger:1024 forKey:@"zx80.amountOfRAM"];
		break;
		case 1:
			llzx8081_setRAMSize(ULA, 3072);
			[userDefaults setInteger:3072 forKey:@"zx80.amountOfRAM"];
		break;
		case 2:
			llzx8081_setRAMSize(ULA, 16384);
			[userDefaults setInteger:16384 forKey:@"zx80.amountOfRAM"];
		break;
		case 3:
			llzx8081_setRAMSize(ULA, 65536);
			[userDefaults setInteger:65536 forKey:@"zx80.amountOfRAM"];
		break;
	}

	// load the requested ROM
	NSString *pathToROM;
	switch(ROMMatrix.selectedRow)
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
	llzx8081_provideROM(ULA, [contentsOfROM bytes], (unsigned int)[contentsOfROM length]);

	// quick test
	void *object = csZ80Disassembler_createDisassembly((uint8_t *)[contentsOfROM bytes], 0, [contentsOfROM length]);
	csObject_release(object);

	// do some tidying up and set our outputter as the
	// end-of-field delegate for the CRT
	if(textureID)
	{
		glDeleteTextures(1, &textureID);
		textureID = 0;
	}
	void *CRT = llzx8081_getCRT(ULA);
	llcrt_setEndOfFieldDelegate(CRT, ZX80DocumentCRTBreakIn, self);

	// hand over our tape
	if(self.tape)
	{
		llzx8081_setTape(ULA, self.tape);
	}

	void *tapePlayer = llzx8081_getTapePlayer(ULA);
	cstapePlayer_setAudioDelegate(tapePlayer, ZX80DocumentAudioCallout, 44100, 2048, self);
	[pauseOrPlayButton setTitle:@"Play Tape"];

	llzx8081_setFastLoadingIsEnabled(ULA, fastLoad);

	self.isRunning = YES;

	NSLog(@"machine configured");

	// create a thread on which to run the machine
	timer =
		[[CSOwnThreadTimer alloc]
			initWithTimeInterval: 1.0 / 100.0f
			target:self
			selector:@selector(runForHalfField:)
			userInfo:nil];

	NSLog(@"timer started");
}



- (IBAction)setNormalSpeed:(id)sender
{
	self.speedMultiplier = 100;
}

- (void)setFastLoad:(BOOL)newFastLoad
{
	fastLoad = newFastLoad;

	llzx8081_setFastLoadingIsEnabled(ULA, fastLoad);

	[[NSUserDefaults standardUserDefaults]
		setBool:newFastLoad forKey:@"zx80.enableFastLoading"];
}

- (IBAction)pauseOrPlayTape:(id)sender
{
	void *tapePlayer = llzx8081_getTapePlayer(ULA);

	if(cstapePlayer_isTapePlaying(tapePlayer))
	{
		cstapePlayer_pause(tapePlayer, llzx8081_getTimeStamp(ULA));
		[pauseOrPlayButton setTitle:@"Play Tape"];
	}
	else
	{
		cstapePlayer_play(tapePlayer, llzx8081_getTimeStamp(ULA));
		[pauseOrPlayButton setTitle:@"Pause Tape"];
	}
}

@end
