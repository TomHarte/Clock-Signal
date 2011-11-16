//
//  AbstractTapeInternals.h
//  Clock Signal
//
//  Created by Thomas Harte on 27/09/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_AbstractTapeInternals_h
#define ClockSignal_AbstractTapeInternals_h

#include "AbstractTape.h"

// Abstraction is achieved using a bog-standard
// 'function pointers in a struct' conceit. Makes
// you long for object-oriented languages,
// doesn't it?

typedef struct
{
	unsigned int retainCount;
	void *tape;

	void (* setSampleRate)(void *tape, uint64_t sampleRate);
	uint64_t (* getLength)(void *tape);
	CSTapeLevel (* getLevelAtTime)(void *tape, uint64_t sampleTime);
	void *(* copy)(void *tape);

	// NB: concrete implementations needn't worry about getting NULL
	// for either of the two final parameters; that's handled by the
	// abstract wrapper
	void (* getLevelPeriodAroundTime)(void *tape, uint64_t sampleTime, uint64_t *startOfPeriod, uint64_t *endOfPeriod);
	CSTapeWaveType waveType;
	void (* destroy)(void *tape);
	uint64_t minimumAccurateSampleRate;

} CSAbstractTapeInternals;

// returns a new wrapper, initialised however the abstract wrapper
// functions want it to be, ready to have such of the fields above
// as a concrete subclass actually wants to supply populated.
CSAbstractTapeInternals *cstape_createAbstractWrapperForTape(void *tape);

#endif
