//
//  StandardBusLines.h
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

/*

	Some 'standard' lines, to bring some sort of uniformity to
	the order that components expect lines to be represented in
	within the CSBusState structure

*/

#ifndef ClockSignal_StandardBusLines_h
#define ClockSignal_StandardBusLines_h

// we'll use the lowest 8 bits for the data lines
#define CSBusStandardDataMask		0xff
#define CSBusStandardDataShift		0

// the next 16 bits are then used for the address lines
#define CSBusStandardAddressMask	0xffff00
#define CSBusStandardAddressShift	8

// we'll then use the most significant bit for the
// standard clock signal
#define CSBusStandardClockLine		0x8000000000000000

#endif
