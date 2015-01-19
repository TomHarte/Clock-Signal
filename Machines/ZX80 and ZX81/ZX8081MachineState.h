//
//  ZX8081MachineState.h
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_ZX8081MachineState_h
#define ClockSignal_ZX8081MachineState_h

#include "stdint.h"
#include "ZX8081.h"
#include "ReferenceCountedObject.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	// the bus, CRT and tape player
	void *bus;
	void *CRT;
	void *tapePlayer;
	void *ROM;
	void *RAM;

	// the line counter, of which only the low 3 bits
	// are present on real hardware
	int lineCounter;

	// the video fetch address, XOR mask and a flag to
	// indicate whether a fetch should occur; these are
	// set when a video fetch is detected and then the
	// flag is reset when the fetch has been fulfilled
	uint16_t videoFetchAddress;
	uint8_t videoByteXorMask;
	bool fetchVideoByte;

	// the current keyboard status; this is supplied by
	// the machine specific stuff so we just cache it
	uint8_t keyLines[8];

	// the hsync counter is the 207-cycle counter on a
	// ZX81 or the M1-clocked pulse counter on a ZX80
	int hsyncCounter;

	// we keep track of whether we're outputting
	// sync via vsyncIsActive and hsyncIsActive
	bool vsyncIsActive, hsyncIsActive;

	// having the common code deal with the line counter
	// means being able to deal with either the ZX80 or
	// ZX81 way of generating horizontal sync — this
	// variable allows that
	bool lastHSyncLevel;

	// we also keep track of which machine we're emulating,
	// and if it's a ZX81 we need to record whether NMIs
	// are enabled
	LLZX8081MachineType machineType;
	bool nmiIsEnabled;

} LLZX8081MachineState;

LLZX8081MachineState *llzx8081_createMachineStateOnBus(
	void *bus,
	LLZX8081MachineType machineType,
	LLZX8081RAMSize ramSize,
	void *CRT,
	void *tapePlayer);

#endif
