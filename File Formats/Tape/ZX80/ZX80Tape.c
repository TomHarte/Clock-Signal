//
//  ZX80Tape.c
//  Clock Signal
//
//  Created by Thomas Harte on 27/09/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ZX80Tape.h"
#include "../Abstract Tape/Implementation/AbstractTapeInternals.h"

typedef struct
{
	uint8_t *data;
	unsigned int length;

	uint64_t sampleRate;
	uint64_t reportedLength;

	uint64_t startTimeOfLastReadBit;
	unsigned int locationOfLastReadBit;
} CSZX80Tape;

/*

	Conventions applied:

		We'll output 2 seconds of silence, then the data, then a further
		1 second of silence.

		Standard ZX80 and 81 tape format is obeyed, so:

			- a 0 is four pulses; a 1 is nine pulses;
			- bits are separated by a 1300us pause;
			- a pulse is 150us high then 150us low;
			- bytes are written with the MSB first.

		Therefore: a 0 takes 2500us, a 1 takes 4000us

	Internal time units are 1 unit = 50us. Internal time is converted to
	the sample rate as and when necessary.

*/
#define kCSZ80TapeFrontPadding			40000
#define kCSZ80TapeBackPadding			20000
#define kCSZ80TapePulseLength			3
#define kCSZ80TapePauseLength			26
#define kCSZ80TapeInternalSampleRate	20000

#define kCSZ80TapeInternalTimeToSampleTime(v)	\
	(((v) * tape->sampleRate) / 20000)

#define kCSZ80TapeSampleTimeToInternalTime(v)	\
	(((v) * 20000) / tape->sampleRate)

static void cszx80tape_setSampleRate(void *opaqueTape, uint64_t sampleRate)
{
	CSZX80Tape *tape = (CSZX80Tape *)opaqueTape;

	// copy in the value
	tape->sampleRate = sampleRate;
	tape->reportedLength = 0;

	// and return us to having no idea where we last were
	tape->startTimeOfLastReadBit =
	tape->locationOfLastReadBit = 0;
}

static uint64_t cszx80tape_getLength(void *opaqueTape)
{
	CSZX80Tape *tape = (CSZX80Tape *)opaqueTape;

	// have we already got the length?
	if(tape->reportedLength) return tape->reportedLength;

	// this is a bit painful, actually
	uint64_t totalLength = 0;

	// output the initial silence
	totalLength += kCSZ80TapeFrontPadding;

	// output the bytes, one at a time
	for(int byteIndex = 0; byteIndex < tape->length; byteIndex++)
	{
		// get the byte and consider the bits
		uint8_t byte = tape->data[byteIndex];
		int bit = 8;
		while(bit--)
		{
			// decide whether we'd output 9 or 4 high and low pulses
			if(byte&0x80)
			{
				totalLength += (kCSZ80TapePulseLength * 2)*9;
			}
			else
			{
				totalLength += (kCSZ80TapePulseLength * 2)*4;
			}

			// output a pause
			totalLength += kCSZ80TapePauseLength;
			byte <<= 1;
		}
	}

	// output the terminating silence
	totalLength += kCSZ80TapeBackPadding;

	// convert the toal and return
	return tape->reportedLength = kCSZ80TapeInternalTimeToSampleTime(totalLength);
}

#define kCSZ80TapeCurrentBit()	\
	((tape->data[tape->locationOfLastReadBit >> 3] << (tape->locationOfLastReadBit&7))&0x80)


// this finds the bit that is being output at the nominated time and
// the time at which its output began. The last result is cached and
// this works forward from there to the new time if possible, to avoid
// lots of unnecessary working given that ZX80 tape files aren't particularly
// easy to access randomly and generally aren't accessed randomly.
// It considers bits to last from the beginning of the first pulse through
// to the end of the silence
static void cszx80tape_setToTime(CSZX80Tape *tape, uint64_t tapeTime)
{
	// oh, this is a rewind. Not much we can do then, but start
	// at the beginning again.
	if(tapeTime < tape->startTimeOfLastReadBit)
	{
		tape->startTimeOfLastReadBit =
		tape->locationOfLastReadBit = 0;
	}

	// if we're still in the intial silence then that was easy!
	if(tapeTime < kCSZ80TapeFrontPadding) return;
	tapeTime -= kCSZ80TapeFrontPadding;

	// loop until we find the bit we're currently in or run out of
	// data to search
	while((tape->locationOfLastReadBit >> 3) < tape->length)
	{
		// get current bit, work out how many pulses it has
		int bit = kCSZ80TapeCurrentBit();
		int pulsesInBit = bit ? 9 : 4;

		// hence work out its length and check whether the requested time
		// falls within this bit
		int lengthOfBit = (pulsesInBit * 2 * kCSZ80TapePulseLength) + kCSZ80TapePauseLength;
		if(lengthOfBit > tapeTime - tape->startTimeOfLastReadBit) return;

		// if not then advance a bit
		tape->locationOfLastReadBit++;
		tape->startTimeOfLastReadBit += lengthOfBit;
	}

}

static CSTapeLevel cszx80tape_getLevelAtTime(void *opaqueTape, uint64_t sampleTime)
{
	CSZX80Tape *tape = (CSZX80Tape *)opaqueTape;
	uint64_t tapeTime = kCSZ80TapeSampleTimeToInternalTime(sampleTime);

	// move to the time requested
	cszx80tape_setToTime(tape, tapeTime);

	// if we're in the intial padding then life is easy
	if(tapeTime < kCSZ80TapeFrontPadding) return CSTapeLevelUnrecognised;
	tapeTime -= kCSZ80TapeFrontPadding;

	// life is also easy if we've run past the end of data
	if((tape->locationOfLastReadBit >> 3) >= tape->length)
		return CSTapeLevelUnrecognised;

	// otherwise we're in the middle of a bit; find out what
	// sort of bit it is and how many pulses it has
	int bit = kCSZ80TapeCurrentBit();
	int pulsesInBit = bit ? 9 : 4;

	// find out how far we are into this bit
	int timeIntoBit = (int)(tapeTime - tape->startTimeOfLastReadBit);

	// if we're still in the pulse period then output a suitable
	// bit of pulse
	if(timeIntoBit < pulsesInBit*2*kCSZ80TapePulseLength)
	{
		return ((timeIntoBit / kCSZ80TapePulseLength)&1) ? CSTapeLevelLow : CSTapeLevelHigh;
	}

	// otherwise, output silence
	return CSTapeLevelUnrecognised;
}

static void cszx80tape_getLevelPeriodAroundTime(void *opaqueTape, uint64_t sampleTime, uint64_t *startOfPeriod, uint64_t *endOfPeriod)
{
	CSZX80Tape *tape = (CSZX80Tape *)opaqueTape;
	uint64_t tapeTime = kCSZ80TapeSampleTimeToInternalTime(sampleTime);

	// move to the time requested
	cszx80tape_setToTime(tape, tapeTime);

	// if we're in the intial padding then the time is the length
	// of the padding
	if(tapeTime < kCSZ80TapeFrontPadding)
	{
		*startOfPeriod = 0;
		*endOfPeriod = kCSZ80TapeInternalTimeToSampleTime(kCSZ80TapeFrontPadding);
		return;
	}
	tapeTime -= kCSZ80TapeFrontPadding;

	// if we're past the end of the data then this is the back padding
	if((tape->locationOfLastReadBit >> 3) >= tape->length)
	{
		*startOfPeriod = kCSZ80TapeInternalTimeToSampleTime(tape->startTimeOfLastReadBit + kCSZ80TapeFrontPadding);
		*endOfPeriod = kCSZ80TapeInternalTimeToSampleTime(tape->startTimeOfLastReadBit + kCSZ80TapeBackPadding + kCSZ80TapeFrontPadding);
		return;
	}

	// otherwise we're in the middle of a bit; find out what
	// sort of bit it is and jemce how many pulses it has
	int bit = kCSZ80TapeCurrentBit();
	int pulsesInBit = bit ? 9 : 4;

	// find out how far we are into this bit
	int timeIntoBit = (int)(tapeTime - tape->startTimeOfLastReadBit);

	// if we're still in the pulse period then output a suitable
	// window
	if(timeIntoBit < pulsesInBit*2*kCSZ80TapePulseLength)
	{
		int pulseNumber = (timeIntoBit / kCSZ80TapePulseLength);
		*startOfPeriod = kCSZ80TapeInternalTimeToSampleTime(kCSZ80TapeFrontPadding + tape->startTimeOfLastReadBit + pulseNumber*kCSZ80TapePulseLength);
		*endOfPeriod = kCSZ80TapeInternalTimeToSampleTime(kCSZ80TapeFrontPadding + tape->startTimeOfLastReadBit + pulseNumber*kCSZ80TapePulseLength + kCSZ80TapePulseLength);
		return;
	}

	// otherwise, we're in the post-bit silence
	*startOfPeriod = kCSZ80TapeInternalTimeToSampleTime(kCSZ80TapeFrontPadding + tape->startTimeOfLastReadBit + pulsesInBit*2*kCSZ80TapePulseLength);
	*endOfPeriod = kCSZ80TapeInternalTimeToSampleTime(kCSZ80TapeFrontPadding + tape->startTimeOfLastReadBit + pulsesInBit*2*kCSZ80TapePulseLength + kCSZ80TapePauseLength);
}

static void *cszx80tape_copy(void *tape);

static void *cszx80tape_abstractInterfaceForTape(void *tape)
{
	CSAbstractTapeInternals *abstractTape = cstape_createAbstractWrapperForTape(tape);

	if(abstractTape)
	{
		abstractTape->setSampleRate = cszx80tape_setSampleRate;
		abstractTape->getLength = cszx80tape_getLength;
		abstractTape->getLevelAtTime = cszx80tape_getLevelAtTime;
		abstractTape->getLevelPeriodAroundTime = cszx80tape_getLevelPeriodAroundTime;
		abstractTape->waveType = CSTapeWaveTypeSquare;
		abstractTape->minimumAccurateSampleRate = kCSZ80TapeInternalSampleRate;
		abstractTape->copy = cszx80tape_copy;
	}

	return abstractTape;
}

void *cszx80tape_createFromFile(const char *filename)
{
	CSZX80Tape *tape = (CSZX80Tape *)calloc(1, sizeof(CSZX80Tape));

	if(tape)
	{
		FILE *inputStream = fopen(filename, "rb");
		if(!inputStream)
		{
			free(tape);
			return NULL;
		}

		// get length of file
		tape->length = fseek(inputStream, 0, SEEK_END);
		fseek(inputStream, 0, SEEK_SET);

		tape->data = (uint8_t *)malloc(tape->length);
		if(!tape->data)
		{
			fclose(inputStream);
			free(tape);
			return NULL;
		}

		fread(tape->data, 1, tape->length, inputStream);
		fclose(inputStream);
	}
	else return NULL;

	return cszx80tape_abstractInterfaceForTape(tape);
}

void *cszx80tape_createWithData(const uint8_t *data, unsigned int length, bool isPTape)
{
	CSZX80Tape *tape = (CSZX80Tape *)calloc(1, sizeof(CSZX80Tape));
	if(tape)
	{
		tape->data = (uint8_t *)malloc(isPTape ? (length + 1) : length);
		if(!tape->data)
		{
			free(tape);
			return NULL;
		}

		if(isPTape)
		{
			tape->length = length+1;
			memcpy(&tape->data[1], data, length);
			tape->data[0] = 0x80;
		}
		else
		{
			tape->length = length;
			memcpy(tape->data, data, length);
		}
	}
	else return NULL;

	return cszx80tape_abstractInterfaceForTape(tape);
}

static void *cszx80tape_copy(void *opaqueOldTape)
{
	CSZX80Tape *oldTape = (CSZX80Tape *)opaqueOldTape;
	CSZX80Tape *tape = (CSZX80Tape *)calloc(1, sizeof(CSZX80Tape));

	if(tape)
	{
		tape->data = (uint8_t *)malloc(oldTape->length);
		if(!tape->data)
		{
			free(tape);
			return NULL;
		}

		tape->length = oldTape->length;
		memcpy(tape->data, oldTape->data, oldTape->length);
	}
	else return NULL;

	return cszx80tape_abstractInterfaceForTape(tape);
}

