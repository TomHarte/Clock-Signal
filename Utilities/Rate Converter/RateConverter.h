//
//  RateConverter.h
//  Clock Signal
//
//  Created by Thomas Harte on 19/01/2015.
//  Copyright (c) 2015 acrossair. All rights reserved.
//

#ifndef Clock_Signal_RateConverter_h
#define Clock_Signal_RateConverter_h

typedef struct 
{
	uint64_t timeToNow, wholeStep;
	int64_t accumulatedError, adjustmentUp, adjustmentDown;
} CSRateConverterState;

// this is standard Bresenham run-slice stuff; add the
// whole step, which is floor(y/x), then see whether
// doing so has accumulated enough error to push us
// up an extra spot

#define csRateConverter_setup(state, inputRate, outputRate)	\
	{\
		state.wholeStep = outputRate / inputRate; \
		state.adjustmentUp = (outputRate % inputRate) << 1; \
		state.adjustmentDown = inputRate << 1; \
		state.accumulatedError = state.adjustmentUp - state.adjustmentDown; \
	}
#define csRateConverter_advance(state) \
	{\
		state.timeToNow += state.wholeStep; \
		state.accumulatedError += state.adjustmentUp; \
		if(state.accumulatedError > 0) \
		{\
			state.timeToNow++;\
			state.accumulatedError -= state.adjustmentDown;\
		}\
	}
#define csRateConverter_getLocation(state) state.timeToNow

#endif
