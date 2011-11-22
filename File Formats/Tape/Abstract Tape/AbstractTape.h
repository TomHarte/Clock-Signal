//
//  AbstractTape.h
//  Clock Signal
//
//  Created by Thomas Harte on 27/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_AbstractTape_h
#define ClockSignal_AbstractTape_h

// Okay, so... you used one of the specific file format implementations
// to get an abstract tape, which is presented to you as an opaque
// void * pointer. Now you want to do something with that. Such as...

#include "stdint.h"

// Set the sample rate. That is, the number of ticks in a second as
// far as you're concerned. Note that this, and all integer quantities
// are 64-bit integers. So if your machine runs at 10Mhz and you use
// double its clock as the sample rate, you'll still only run into
// integer overflow problems every 30ish centuries
void cstape_setSampleRate(void *tape, uint64_t sampleRate);
uint64_t cstape_getMinimumAccurateSampleRate(void *tape);

// Query the total length of the tape. The result is in the units
// implied by the sample rate you set.
uint64_t cstape_getLength(void *tape);

// For the purposes of the abstract tape interface, at any given
// moment a tape is outputting one of three states — high, low or
// unrecognised. Each of these states lasts for a period.
//
//	cstape_getLevelAtTime returns the current level at the
//	nominated time
//
//	cstape_getLevelPeriodAroundTime returns the outer extents
//	of the period that includes the nominated time. The extents
//	are inclusive, so e.g. you might get 0 and 10 for one period,
//	then 11 and 17 for the next, then 18 and 29 for the one
//	after that. It's acceptable to pass NULL for either parameter.
typedef enum
{
	CSTapeLevelHigh,
	CSTapeLevelUnrecognised,	// implicitly: includes silence
	CSTapeLevelLow
} CSTapeLevel;

CSTapeLevel cstape_getLevelAtTime(void *tape, uint64_t sampleTime);
void cstape_getLevelPeriodAroundTime(void *tape, uint64_t sampleTime, uint64_t *startOfPeriod, uint64_t *endOfPeriod);

// For now we're considering that there are essentially two types
// of tape encoding in the world: those based on square waves (such
// as the Sinclair machines) and those based on sine waves (such as
// those using the Kansas City Standard, the Acorn machines included).
// 
// It's the sine ones that make it helpful to divide a tape into
// periods, since position within a period then makes a difference if
// you're reconstructing a wave.
typedef enum
{
	CSTapeWaveTypeSquare,
	CSTapeWaveTypeSine
} CSTapeWaveType;

CSTapeWaveType cstape_getWaveType(void *tape);

// Tapes are reference counted; use retain and release appropriately.
void *cstape_retain(void *tape);
void cstape_release(void *tape);
void *cstape_copy(void *tape);

#endif
