//
//  CRT.h
//  LLCRT
//
//  Created by Thomas Harte on 22/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef LLCRT_CRT_h
#define LLCRT_CRT_h

#include "stdint.h"
#include "stdbool.h"

/*

	Creation and destruction.

		Notice that three things define a CRT instance:

			-	samplesPerLine; i.e. the number of discrete values per line that
				this CRT should expect from the input; this is to allow inputs
				to provide timings relative to whatever clock they like —
				so we're dealing with a PCM approximation of an analogue input and
				need to know what rate that was sampled at. It's anticipated that
				for most devices, this value will be the dot clock or an integer
				multiple of it.

			-	timingMode; i.e. PAL (625 line) or NTSC (525 line).

			-	displayType; luminance or RGB. YUV possibly to come. Telling the
				CRT what sort of data to expect later allows it to plan its
				internal tables in an optimum fashion.

		Observations then:

			this is a CRT with an RGB+sync or Y+sync video input connection.

*/

typedef enum
{
	LLCRTInputTimingPAL,
	LLCRTInputTimingNTSC
} LLCRTInputTiming;

typedef enum
{
	LLCRTDisplayTypeLuminance,
	LLCRTDisplayTypeRGB
} LLCRTDisplayType;

void *llcrt_create(unsigned int samplesPerLine, LLCRTInputTiming timingMode, LLCRTDisplayType displayType);

/*

	The end-of-field delegate is the actor that knows what to do with a newly
	completed field. Usually it'll put it on a screen for the user to look at.

	The delegate receives:

		- a reference to the CRT posting an image
		- the whole width and height of the buffer that's being passed
		- the proportion of the width and height (in the range 0.0 to 1.0) that
			is within the area the CRT can reach
		- the type of the buffer (which will be one of the display types)
		- whether this is an odd or (implicitly) even field
		- a pointer to the buffer itself

	This class will always pass out a linear buffer with power-of-two dimensions.
	The two proportion fields are used because the input will never have produced
	a power-of-two output (given PAL and NTSC field sizes)

*/
typedef void (* llcrt_endOfFieldDelegate)(
	void *crt,
	unsigned int widthOfBuffer,
	unsigned int heightOfBuffer,
	LLCRTDisplayType bufferType,
	bool isOddField,
	void *buffer,
	void *context);

void llcrt_setEndOfFieldDelegate(void *crt, llcrt_endOfFieldDelegate delegate, void *context);

/*

	runToTime is a basic 'keepalive' style call — it alerts the CRT that
	the system has advanced to the specified time. The CRT is a purely passive
	subsystem, so it won't push frames out unless you keep clocking it.

	Inputs on all connections are time stamped and automatically keep the
	CRT running.

*/
void llcrt_runToTime(void *crt, unsigned int timeStamp);

/*

	setLuminanceLevel sets the current decoded output level, in black and
	white. 0 is black, 255 is white, values in between are linear.

	setSyncLevel sets the current output level as sync. So the CRT will output
	the blanking level and may start some sort of retrace depeding on
	sync timings.

*/
void llcrt_setLuminanceLevel(void *crt, unsigned int timeStamp, uint8_t level);
void llcrt_setSyncLevel(void *crt, unsigned int timeStamp);

/*

	output1BitLiminanceByte queues up a byte of luminance bits, MSB first,
	with each bit being set to output white, clear to output black.

	It outputs one pixel per cycle, so this is exactly identical to:
	
	int c = 8;
	while(c--)
	{
		llcrt_setLuminanceLevel(crt, timeStamp, (luminanceByte&0x80) ? 0xff : 0x00);
		timeStamp++;
		luminanceByte <<= 1;
	}

*/
void llcrt_output1BitLuminanceByte(void *crt, unsigned int timeStamp, uint8_t luminanceByte);

/*

	TODO: add methods for RGB output

*/

#endif
