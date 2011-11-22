//
//  AbstractTape.c
//  Clock Signal
//
//  Created by Thomas Harte on 27/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "AbstractTape.h"
#include "Implementation/AbstractTapeInternals.h"

// a function to wrap a tape into an abstract wrapper and return the thing
CSAbstractTapeInternals *cstape_createAbstractWrapperForTape(void *tape)
{
	CSAbstractTapeInternals *internals;

	// allocate a new abstract tape wrapper
	internals = (CSAbstractTapeInternals *)calloc(1, sizeof(CSAbstractTapeInternals));

	// if we got one, store the tape pointer on in there
	if(internals)
	{
		internals->tape = tape;
		internals->retainCount = 1;
	}

	// and return the thing
	return internals;
}

// a function to release an abstract tape (ie, the wrapper and the
// thing wrapped inside)
static void cstape_destroy(void *opaqueTape)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;

	// if this tape supplied a destroy function then call it
	if(tape->destroy) tape->destroy(tape->tape);

	// and free the wrapper
	free(tape);
}

void *cstape_retain(void *opaqueTape)
{
	((CSAbstractTapeInternals *)opaqueTape)->retainCount++;
	return opaqueTape;
}

void cstape_release(void *opaqueTape)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	
	tape->retainCount--;
	if(!tape->retainCount) cstape_destroy(tape);
}

// A whole bunch of very thin wrappers to the actual tape file.
//
// Note the conventions: a concrete tape need not have supplied a
// function for everything, which aids with development and is a
// good practice to adopt since it'll allow the abstract tape
// interface to be expanded in the future. At present I'd imagine
// that a complete implementation of any file format will provide
// a complete set of functions.

void cstape_setSampleRate(void *opaqueTape, uint64_t sampleRate)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	if(tape->setSampleRate) tape->setSampleRate(tape->tape, sampleRate);
}

uint64_t cstape_getLength(void *opaqueTape)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	if(tape->getLength) return tape->getLength(tape->tape);
	return 0;
}

CSTapeLevel cstape_getLevelAtTime(void *opaqueTape, uint64_t sampleTime)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	if(tape->getLevelAtTime) return tape->getLevelAtTime(tape->tape, sampleTime);
	return CSTapeLevelUnrecognised;
}

void cstape_getLevelPeriodAroundTime(void *opaqueTape, uint64_t sampleTime, uint64_t *startOfPeriod, uint64_t *endOfPeriod)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	
	uint64_t throwawayValue;
	if(!startOfPeriod) startOfPeriod = &throwawayValue;
	if(!endOfPeriod) endOfPeriod = &throwawayValue;
	if(tape->getLevelPeriodAroundTime) return tape->getLevelPeriodAroundTime(tape->tape, sampleTime, startOfPeriod, endOfPeriod);
}

void *cstape_copy(void *opaqueTape)
{
	CSAbstractTapeInternals *tape = (CSAbstractTapeInternals *)opaqueTape;
	if(tape->copy) return tape->copy(tape->tape);
	return NULL;
}

CSTapeWaveType cstape_getWaveType(void *opaqueTape)
{
	return ((CSAbstractTapeInternals *)opaqueTape)->waveType;
}

uint64_t cstape_getMinimumAccurateSampleRate(void *opaqueTape)
{
	return ((CSAbstractTapeInternals *)opaqueTape)->minimumAccurateSampleRate;
}
