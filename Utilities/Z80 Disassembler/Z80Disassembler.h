//
//  Z80Disassembler.h
//  Clock Signal
//
//  Created by Thomas Harte on 11/11/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_Z80Disassembler_h
#define ClockSignal_Z80Disassembler_h

#include "ReferenceCountedObject.h"
#include "stdint.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	const char *text;
	uint16_t address;
} Z80Line;

// this will return a csArray filled with Z80Lines
void *csZ80Disassembler_createDisassembly(uint8_t *sourceData, uint16_t startAddress, uint16_t length);

#endif
