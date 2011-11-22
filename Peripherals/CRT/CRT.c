//
//  CRT.c
//  LLCRT
//
//  Created by Thomas Harte on 22/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CRT.h"
#include "ReferenceCountedObject.h"

#define kLLCRTFieldSyncOverrun	30
#define kLLCRTLineSyncOverrun	1

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	unsigned int cyclesPerLine;
	unsigned int visibleCyclesPerLine;
	unsigned int visibleLinesPerField;
	unsigned int linesPerField;	// rounded up to the nearest whole integer

	LLCRTDisplayType displayType;
	uint8_t *displayBuffer;
	int displayWidth;
	int displayHeight;

	llcrt_endOfFieldDelegate delegate;
	void *delegateContext;

	int currentPositionInLine;
	int currentLine;
	unsigned int currentTimeStamp;

	uint8_t currentLevel;

	unsigned int lastSyncEventTime;
	unsigned int syncChargeLevel;
	bool syncActive;

	bool nextFrameIsEven;

} LLCRTState;

static unsigned int llcrt_nextPowerOfTwoAfter(unsigned int input)
{
	unsigned int result = 1;

	while(result < input) result <<= 1;

	return result;
}

static void llcrt_destroy(void *opaqueCrt)
{
	// there's little to say; deallocate the buffer
	// and then deallocate ourself
	LLCRTState *crt = (LLCRTState *)opaqueCrt;

	free(crt->displayBuffer);
}

void *llcrt_create(unsigned int cyclesPerLine, LLCRTInputTiming timingMode, LLCRTDisplayType displayType)
{
	LLCRTState *crt = (LLCRTState *)calloc(1, sizeof(LLCRTState));

	if(crt)
	{
		// set up as a reference counted object
		csObject_init(crt);
		crt->referenceCountedObject.dealloc = llcrt_destroy;
	
		// copy this line directly ...
		crt->cyclesPerLine = cyclesPerLine;

		// figure out total lines per field (rounded up)
		// based on the timing mode. TODO: seed a whole
		// bunch more default timings from this
		switch(timingMode)
		{
			case LLCRTInputTimingPAL:
				crt->linesPerField = 312;
				crt->visibleLinesPerField = 288;
				crt->visibleCyclesPerLine = (cyclesPerLine * 5195) / 6400;
			break;

			case LLCRTInputTimingNTSC:
				crt->linesPerField = 263;
				crt->visibleLinesPerField = 243;
				crt->visibleCyclesPerLine = (cyclesPerLine * 422) / 529;
			break;
		}

		// work out the total buffer size we'll want
		crt->displayWidth = crt->cyclesPerLine;
		if(crt->displayWidth&3) crt->displayWidth += 4 - (crt->displayWidth&3);
		crt->displayHeight = crt->linesPerField;

		// use the display type to decide what sort of buffer to write to
		crt->displayType = displayType;
		switch(displayType)
		{
			case LLCRTDisplayTypeLuminance:
				crt->displayBuffer = (uint8_t *)calloc(crt->displayWidth * crt->displayHeight, 1);
			break;
			case LLCRTDisplayTypeRGB:
				crt->displayBuffer = (uint8_t *)calloc(crt->displayWidth * crt->displayHeight * 3, 1);
			break;
		}

		// fill with randomised garbage
//		for(int x = 0; x < crt->displayWidth * crt->displayHeight; x++)
//			crt->displayBuffer[x] = rand();

		// if we didn't get a buffer to draw to, report a failure to initialise
		if(!crt->displayBuffer)
		{
			free(crt);
			return NULL;
		}
		
		// work out the proportion used then
//		crt->proportionOfDisplayWidthUsed = (float)cyclesPerLine / crt->displayWidth;
//		crt->proportionOfDisplayHeightUsed = (float)crt->linesPerField / crt->displayHeight;

		// implicitly from calloc: we're at the beginning of recorded time,
		// with no current delegate, at the start of the first line
	}

	return crt;
}

void llcrt_setEndOfFieldDelegate(void *opaqueCrt, llcrt_endOfFieldDelegate delegate, void *context)
{
	// just fill in the appropriate fields
	LLCRTState *crt = (LLCRTState *)opaqueCrt;
	
	crt->delegate = delegate;
	crt->delegateContext = context;
}

#define address(x, y) ((y) * crt->displayWidth + (x))
#define buffer(x, y) crt->displayBuffer[address(x, y)]

static void llcrt_moveToNextField(LLCRTState *crt)
{
	if(crt->currentLine < crt->displayHeight && crt->currentPositionInLine < crt->displayWidth)
		memset(
			&buffer(crt->currentPositionInLine, crt->currentLine),
			crt->currentLevel,
			crt->displayWidth*crt->displayHeight - address(crt->currentPositionInLine, crt->currentLine));

	if(crt->currentLine && crt->delegate)
	{
		crt->delegate(
			crt,
			crt->displayWidth, crt->displayHeight, 
			crt->displayType,
			crt->nextFrameIsEven,
			crt->displayBuffer,
			crt->delegateContext);
	}
	crt->currentLine = 0;

	crt->nextFrameIsEven = 
		(crt->currentPositionInLine < (crt->cyclesPerLine >> 2)) ||
		(crt->currentPositionInLine > ((crt->cyclesPerLine * 3) >> 2));
}

static void llcrt_moveToNextScanline(LLCRTState *crt)
{
	// output black to fill up space, as though
	// nothing is supplied as output
	if(crt->currentPositionInLine < crt->displayWidth && crt->currentLine < crt->displayHeight)
		memset(&buffer(crt->currentPositionInLine, crt->currentLine), crt->currentLevel, crt->displayWidth - crt->currentPositionInLine);

	crt->currentPositionInLine = 0;
	crt->currentLine++;
	if(crt->currentLine == crt->linesPerField + kLLCRTFieldSyncOverrun)
	{
		llcrt_moveToNextField(crt);
	}
}

static void llcrt_didDetectHSync(LLCRTState *crt)
{
	// do the hsync
	if(crt->currentPositionInLine >	crt->visibleCyclesPerLine)
		llcrt_moveToNextScanline(crt);
}

static void llcrt_didDetectVSync(LLCRTState *crt)
{
	// do the vsync if we're in the correct window
	if(crt->currentLine > crt->visibleLinesPerField)
	{
		llcrt_moveToNextField(crt);
	}
}

static void llcrt_unsetSyncLevel(LLCRTState *crt)
{
	// check whether we appear to have received a
	// horizontal sync; we're considering anything less than
	// about 10 us and greater than or equal to about 3 to be
	// a horizontal sync
//	unsigned int timeSinceSyncBegan = crt->currentTimeStamp - crt->lastSyncEventTime;

//	if(
//		(timeSinceSyncBegan >= ((3 * crt->cyclesPerLine) / 64)) &&
//		(timeSinceSyncBegan <= ((10 * crt->cyclesPerLine) / 64))
//	)
//	{
//		llcrt_didDetectHSync(crt);
//	}

	crt->syncActive = false;
}

static void llcrt_runToTimeInternal(LLCRTState *crt, unsigned int timeStamp)
{
	// work out how much time to run for
	unsigned int timeToRunFor = timeStamp - crt->currentTimeStamp;

	// decide whether to run to the end of this line
	unsigned int timeToEndOfLine = crt->cyclesPerLine + kLLCRTLineSyncOverrun - crt->currentPositionInLine;
	if(timeToRunFor > timeToEndOfLine)
	{
		llcrt_moveToNextScanline(crt);
		timeToRunFor -= timeToEndOfLine;

		// continue for any complete lines with no output
		while(timeToRunFor > crt->cyclesPerLine + kLLCRTLineSyncOverrun)
		{
			llcrt_moveToNextScanline(crt);
			timeToRunFor -= (crt->cyclesPerLine - kLLCRTLineSyncOverrun);
		}
	}

	// and move to the relevant position in the current line
	if(crt->currentLine < crt->displayHeight && crt->currentPositionInLine < crt->displayWidth)
	{
		int width = timeToRunFor;
		if(width > crt->displayWidth - crt->currentPositionInLine)
			width = crt->displayWidth - crt->currentPositionInLine;

		memset(
			&buffer(crt->currentPositionInLine, crt->currentLine),
			crt->currentLevel,
			width);
	}
	crt->currentPositionInLine += timeToRunFor;
	crt->currentTimeStamp = timeStamp;
}

void llcrt_runToTime(void *opaqueCrt, unsigned int timeStamp)
{
	LLCRTState *crt = (LLCRTState *)opaqueCrt;

	// work out how much time to run for
	unsigned int timeToRunFor = timeStamp - crt->currentTimeStamp;

	// if time is seemingly negative, don't do anything
	if(timeToRunFor > crt->cyclesPerLine * crt->linesPerField * 30)
	{
		return;
	}

	// check whether the vertical sync capacitor charged during
	// this time window
	if(crt->syncActive)
	{
		unsigned int baseTimeStamp = crt->currentTimeStamp;
		const unsigned int timeToChargeCapacitor = (crt->cyclesPerLine * 23) / 10;

		while((crt->syncChargeLevel + timeToRunFor) >= timeToChargeCapacitor)
		{
			unsigned int timeUntilCharge = timeToChargeCapacitor - crt->syncChargeLevel;
			unsigned int targetStamp = baseTimeStamp + timeUntilCharge;

			llcrt_runToTimeInternal(crt, targetStamp);
			llcrt_didDetectVSync(crt);

			baseTimeStamp = targetStamp;
			timeToRunFor -= timeUntilCharge;
			crt->syncChargeLevel = 0;
		}
		crt->syncChargeLevel += timeToRunFor;
		llcrt_runToTimeInternal(crt, baseTimeStamp + timeToRunFor);
	}
	else
	{
		// otherwise it's leaking, in a very approximate manner
		if(crt->syncChargeLevel)
		{
			unsigned int oldSyncLevel = crt->syncChargeLevel;
			crt->syncChargeLevel -= timeToRunFor;
			if(oldSyncLevel < crt->syncChargeLevel)
				crt->syncChargeLevel = 0;
		}

	}

	// do all of the line running at once
	llcrt_runToTimeInternal(crt, timeStamp);
}

void llcrt_setSyncLevel(void *opaqueCrt, unsigned int timeStamp)
{
	LLCRTState *crt = (LLCRTState *)opaqueCrt;

	// do nothing if this isn't a transition
	if(crt->syncActive) return;

	// make sure we're up-to-date
	llcrt_runToTime(crt, timeStamp);

	crt->syncActive = true;
	crt->lastSyncEventTime = timeStamp;
	llcrt_didDetectHSync(crt);

	// which also definitely means we're at the
	// blanking level for colour purposes (as in,
	// technically we're below it but you can't
	// get blacker than black so this will do)
	crt->currentLevel = 0;
}

void llcrt_setLuminanceLevel(void *opaqueCrt, unsigned int timeStamp, uint8_t level)
{
	LLCRTState *crt = (LLCRTState *)opaqueCrt;

	// do nothing if this isn't a transition
	if(crt->currentLevel == level && !crt->syncActive) return;

	// make sure we're up-to-date
	llcrt_runToTime(crt, timeStamp);
	llcrt_unsetSyncLevel(crt);

	// store the new level
	crt->currentLevel = level;
}

void llcrt_output1BitLuminanceByte(void *opaqueCrt, unsigned int timeStamp, uint8_t luminanceByte)
{
	LLCRTState *crt = (LLCRTState *)opaqueCrt;

	if(
		(!luminanceByte && !crt->currentLevel) ||
		(luminanceByte == 0xff && crt->currentLevel == 0xff))
		return;

	uint8_t shiftByte = luminanceByte;
	
	// make sure we're up-to-date
	llcrt_runToTime(crt, timeStamp);
	llcrt_unsetSyncLevel(crt);

	// output the byte in the 8 cycles that follow
	if(crt->currentLine < crt->displayHeight)
	{
		int pixels = 8;
		while(pixels--)
		{
			if(crt->currentPositionInLine >= crt->displayWidth) break;

			buffer(crt->currentPositionInLine, crt->currentLine) =
				(shiftByte&0x80) ? 0xff : 0x00;

			shiftByte <<= 1;
			crt->currentPositionInLine ++;
		}
	}
	crt->currentTimeStamp += 8;
}
