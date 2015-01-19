//
//  LinearFilter.h
//  Clock Signal
//
//  Created by Thomas Harte on 01/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_LinearFilter_h
#define ClockSignal_LinearFilter_h

/*

	The linear filter takes a 1d PCM signal with
	a given sample rate and filters it according
	to a specified filter (band pass only at
	present, more to come if required). The number
	of taps (ie, samples considered simultaneously
	to make an output sample) is configurable;
	smaller numbers produce a filter that operates
	more quickly and with less lag but less
	effectively.

	Create the filter, push new values to it
	using pushShort and then get new filtered
	values out.

	Alternatively, use apply to buffer to
	simultaneously apply the filter and then
	point sample the result. The filter has a memory
	so you'll get correct results if you
	progressively push source buffers from the same
	source.

	Summary: this is a great way to take an
	ongoing waveform sampled at one rate and convert
	it to a waveform of a lesser rate — by applying
	a band pass filter to get rid of inaudible
	frequencies and then point sampling. Hence this
	functionality is expected to be used by the
	controller portions of the user interface when
	presenting a sound output to the end user,
	given that modern hardware generally takes a
	PCM wave at CD-or-thereabouts sampling rate,
	whereas old sound generators tend to be clocked
	in the megahertz.

*/

#define kCSFilterDefaultAttenuation	60.0f

void *csfilter_createBandPass(unsigned int numberOfTaps, unsigned int inputSampleRate, unsigned int outputSampleRate, float lowFrequency, float highFrequency, float attenuation);
void *csfilter_retain(void *filter);
void csfilter_release(void *filter);

void csfilter_pushShort(void *filter, short value);
short csfilter_getFilteredShort(void *filter);

unsigned int csfilter_applyToBuffer(void *filter, short *targetBuffer, const short *sourceBuffer, unsigned int numberOfOutputSamples);

#endif
