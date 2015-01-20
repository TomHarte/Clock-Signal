//
//  LinearFilter.c
//  Clock Signal
//
//  Created by Thomas Harte on 01/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "LinearFilter.h"
#include "RateConverter.h"

#ifdef VDSP
#include <Accelerate/Accelerate.h>
#endif

/*

	A Kaiser-Bessel filter is a real time window filter. It looks at the last n samples
	of an incoming data source and computes a filtered value, which is the value you'd
	get after applying the specified filter, at the centre of the sampling window.

	Hence, if you request a 37 tap filter then filtering introduces a latency of 18
	samples. Suppose you're receiving input at 44,100Hz and using 4097 taps, then you'll
	introduce a latency of 2048 samples, which is about 46ms.

	There's a correlation between the number of taps and the quality of the filtering.
	More samples = better filtering, at the cost of greater latency. Internally, applying
	the filter involves calculating a weighted sum of previous values, so increasing the
	number of taps is quite cheap in processing terms.

	Original source for this filter:

		"DIGITAL SIGNAL PROCESSING, II", IEEE Press, pages 123–126.
*/

struct CSLinearFilter
{
	unsigned int retainCount;

	short *filterCoefficients;
	short *valueQueue;
	unsigned int numberOfTaps;
	unsigned int writePosition;

	CSRateConverterState rateConverter;
};

// our little fixed point scheme
#define kCSKaiserBesselFilterFixedMultiplier	32767.0f
#define kCSKaiserBesselFilterFixedShift			15

/* ino evaluates the 0th order Bessel function at a */
static float csfilter_ino(float a)
{
	float d = 0.0f;
	float ds = 1.0f;
	float s = 1.0f;

	do
	{
		d += 2.0f;
		ds *= (a * a) / (d * d);
		s += ds;
	}
	while(ds > s*1e-6f);

	return s;
}

static void csfilter_setIdealisedFilterResponse(short *restrict filterCoefficients, float *restrict A, float attenuation, unsigned int numberOfTaps)
{
	/* calculate alpha, which is the Kaiser-Bessel window shape factor */
	float a;	// to take the place of alpha in the normal derivation

	if(attenuation < 21.0f)
		a = 0.0f;
	else
	{
		if(attenuation > 50.0f)
			a = 0.1102f * (attenuation - 8.7f);
		else
			a = 0.5842f * powf(attenuation - 21.0f, 0.4f) + 0.7886f * (attenuation - 21.0f);
	}

	float *filterCoefficientsFloat = (float *)malloc(sizeof(float) * numberOfTaps);

	/* work out the right hand side of the filter coefficients */
	unsigned int Np = (numberOfTaps - 1) / 2;
	float I0 = csfilter_ino(a);
	float NpSquared = (float)(Np * Np);
	for(unsigned int i = 0; i <= Np; i++)
	{
		filterCoefficientsFloat[Np + i] = 
				A[i] * 
				csfilter_ino(a * sqrtf(1.0f - ((float)(i * i) / NpSquared) )) /
				I0;
	}

	/* coefficients are symmetrical, so copy from right hand side to left side */
	for(unsigned int i = 0; i < Np; i++)
	{
		filterCoefficientsFloat[i] = filterCoefficientsFloat[numberOfTaps - 1 - i];
	}
	
	/* scale back up so that we retain 100% of input volume */
	float coefficientTotal = 0.0f;
	for(unsigned int i = 0; i < numberOfTaps; i++)
	{
		coefficientTotal += filterCoefficientsFloat[i];
	}

	/* we'll also need integer versions, potentially */
	float coefficientMultiplier = 1.0f / coefficientTotal;
	for(unsigned int i = 0; i < numberOfTaps; i++)
	{
		filterCoefficients[i] = (short)(filterCoefficientsFloat[i] * kCSKaiserBesselFilterFixedMultiplier * coefficientMultiplier);
	}

	free(filterCoefficientsFloat);
}

void *csfilter_createBandPass(unsigned int numberOfTaps, unsigned int inputSampleRate, unsigned int outputSampleRate, float lowFrequency, float highFrequency, float attenuation)
{
	// we must be asked to filter based on an odd number of
	// taps, and at least three
	if(numberOfTaps < 3) numberOfTaps = 3;
	if(attenuation < 21.0f) attenuation = 21.0f;

	struct CSLinearFilter *filter;

	filter = (struct CSLinearFilter *)calloc(1, sizeof(struct CSLinearFilter));

	if(filter)
	{
		// ensure we have an odd number of taps
		numberOfTaps |= 1;

		filter->numberOfTaps = numberOfTaps;
		filter->filterCoefficients = (short *)malloc(sizeof(short)*numberOfTaps);
		filter->valueQueue = (short *)malloc(sizeof(short)*numberOfTaps);
		filter->retainCount = 1;

		// we'll be stepping through the output when we filter so the output
		// sample rate will be the input to the rate converter
		csRateConverter_setup(filter->rateConverter, outputSampleRate, inputSampleRate);

		if(!filter->filterCoefficients || !filter->valueQueue)
		{
			if(filter->filterCoefficients) free(filter->filterCoefficients);
			if(filter->valueQueue) free(filter->valueQueue);
			free(filter);
			return NULL;
		}

		/* calculate idealised filter response */
		unsigned int Np = (numberOfTaps - 1) / 2;
		float twoOverSampleRate = 2.0f / (float)inputSampleRate;

		float *A = (float *)malloc(sizeof(float)*(Np+1));
		A[0] = 2.0f * (highFrequency - lowFrequency) / (float)inputSampleRate;
		for(unsigned int i = 1; i <= Np; i++)
		{
			float iPi = (float)i * (float)M_PI;
			A[i] = 
				(
					sinf(twoOverSampleRate * iPi * highFrequency) -
					sinf(twoOverSampleRate * iPi * lowFrequency)
				) / iPi;
		}

		csfilter_setIdealisedFilterResponse(filter->filterCoefficients, A, attenuation, numberOfTaps);

		/* clean up */
		free(A);
	}

	return filter;
}

static void csfilter_destroy(struct CSLinearFilter *filter)
{
	free(filter->filterCoefficients);
	free(filter->valueQueue);
	free(filter);
}

void *csfilter_retain(void *opaqueFilter)
{
	((struct CSLinearFilter *)opaqueFilter)->retainCount++;
	return opaqueFilter;
}

void csfilter_release(void *opaqueFilter)
{
	struct CSLinearFilter *filter = (struct CSLinearFilter *)opaqueFilter;

	filter->retainCount--;
	if(!filter->retainCount) csfilter_destroy(filter);
}

void csfilter_pushShort(void *opaqueFilter, short value)
{
	struct CSLinearFilter *filter = (struct CSLinearFilter *)opaqueFilter;

	filter->valueQueue[filter->writePosition] = value;
	filter->writePosition = (filter->writePosition+1)%filter->numberOfTaps;
}

short csfilter_getFilteredShort(void *opaqueFilter)
{
	struct CSLinearFilter *filter = (struct CSLinearFilter *)opaqueFilter;

	int result = 0;
	unsigned int c = filter->numberOfTaps;
	while(c--)
	{
		result += filter->filterCoefficients[c] * filter->valueQueue[ (c + filter->writePosition) % filter->numberOfTaps ];
	}

	return (short)(result >> kCSKaiserBesselFilterFixedShift);
}

unsigned int csfilter_applyToBuffer(void *opaqueFilter, short *targetBuffer, const short *sourceBuffer, unsigned int numberOfOutputSamples)
{
	struct CSLinearFilter *filter = (struct CSLinearFilter *)opaqueFilter;
	for(unsigned int sampleToWrite = 0; sampleToWrite < numberOfOutputSamples; sampleToWrite++)
	{
		size_t shortReadPosition = (size_t)csRateConverter_getLocation(filter->rateConverter);

#ifdef VDSP
		vDSP_dotpr_s1_15(filter->filterCoefficients, 1, &sourceBuffer[shortReadPosition], 1, &targetBuffer[sampleToWrite], filter->numberOfTaps);
#else
		int outputValue = 0;
		for(unsigned int c = 0; c < filter->numberOfTaps; c++)
		{
			outputValue += filter->filterCoefficients[c] * sourceBuffer[shortReadPosition + c];
		}
		targetBuffer[sampleToWrite] = (short)(outputValue >> kCSKaiserBesselFilterFixedShift);
#endif

		csRateConverter_advance(filter->rateConverter)
	}

	unsigned int readPosition = (unsigned int)csRateConverter_getLocation(filter->rateConverter);
	csRateConverter_decreaseLocation(filter->rateConverter, readPosition);
	return readPosition;
}
