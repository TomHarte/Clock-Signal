//
//  ZX80ULA.h
//  LLZX80
//
//  Created by Thomas Harte on 18/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_ZX8081ULA_h
#define ClockSignal_ZX8081ULA_h

#include "stdint.h"
#include "stdbool.h"

void *llzx8081_create(void);	// returns a csObject

// this will read 'length' bytes from the pointer
// given, so careful now. Providing lengths that
// aren't a power of two will have odd effects
void llzx8081_provideROM(void *ula, const uint8_t *ROM, unsigned int length);

typedef enum
{
	LLZX8081RAMSize1Kb,
	LLZX8081RAMSize2Kb,
	LLZX8081RAMSize16Kb,
	LLZX8081RAMSize64Kb
} LLZX8081RAMSize;

void llzx8081_setRAMSize(void *ula, LLZX8081RAMSize ramSize);

void llzx8081_runForHalfCycles(void *opaqueULA, unsigned int numberOfHalfCycles);

void *llzx8081_getCRT(void *ula);
void *llzx8081_getCPU(void *ula);

void llzx8081_setTape(void *ula, void *tape);
void *llzx8081_getTapePlayer(void *ula);
unsigned int llzx8081_getTimeStamp(void *ula);
void llzx8081_setFastLoadingIsEnabled(void *ula, bool isEnabled);

// use this to get the contents of memory; it'll negotiate the memory
// map to return contents of ROM or RAM as appropriate, applying the
// normal mirroring rules
bool llzx8081_copyMemory(void *ula, uint8_t *dest, uint16_t startAddress, uint16_t length);

//uint16_t llzx80ula_writeToRAM(void *ula, uint8_t *source, uint16_t startAddress, uint16_t length);
//uint16_t llzx80ula_readFromRAM(void *ula, uint8_t *source, uint16_t startAddress, uint16_t length);

// Implementation note: these virtual keys have a very predictable
// format, the top byte being the key line and the bottom byte
// being the line mask
typedef enum
{
	LLZX8081VirtualKeyShift	= 0x0001,
	LLZX8081VirtualKeyZ		= 0x0002,
	LLZX8081VirtualKeyX		= 0x0004,
	LLZX8081VirtualKeyC		= 0x0008,
	LLZX8081VirtualKeyV		= 0x0010,

	LLZX8081VirtualKeyA		= 0x0101,
	LLZX8081VirtualKeyS		= 0x0102,
	LLZX8081VirtualKeyD		= 0x0104,
	LLZX8081VirtualKeyF		= 0x0108,
	LLZX8081VirtualKeyG		= 0x0110,

	LLZX8081VirtualKeyQ		= 0x0201,
	LLZX8081VirtualKeyW		= 0x0202,
	LLZX8081VirtualKeyE		= 0x0204,
	LLZX8081VirtualKeyR		= 0x0208,
	LLZX8081VirtualKeyT		= 0x0210,

	LLZX8081VirtualKey1		= 0x0301,
	LLZX8081VirtualKey2		= 0x0302,
	LLZX8081VirtualKey3		= 0x0304,
	LLZX8081VirtualKey4		= 0x0308,
	LLZX8081VirtualKey5		= 0x0310,

	LLZX8081VirtualKey0		= 0x0401,
	LLZX8081VirtualKey9		= 0x0402,
	LLZX8081VirtualKey8		= 0x0404,
	LLZX8081VirtualKey7		= 0x0408,
	LLZX8081VirtualKey6		= 0x0410,

	LLZX8081VirtualKeyP		= 0x0501,
	LLZX8081VirtualKeyO		= 0x0502,
	LLZX8081VirtualKeyI		= 0x0504,
	LLZX8081VirtualKeyU		= 0x0508,
	LLZX8081VirtualKeyY		= 0x0510,

	LLZX8081VirtualKeyEnter	= 0x0601,
	LLZX8081VirtualKeyL		= 0x0602,
	LLZX8081VirtualKeyK		= 0x0604,
	LLZX8081VirtualKeyJ		= 0x0608,
	LLZX8081VirtualKeyH		= 0x0610,

	LLZX8081VirtualKeySpace	= 0x0701,
	LLZX8081VirtualKeyDot	= 0x0702,
	LLZX8081VirtualKeyM		= 0x0704,
	LLZX8081VirtualKeyN		= 0x0708,
	LLZX8081VirtualKeyB		= 0x0710,
} LLZX8081VirtualKey;

void llzx8081_setKeyDown(void *ula, LLZX8081VirtualKey key);
void llzx8081_setKeyUp(void *ula, LLZX8081VirtualKey key);
//void llzx80ula_typeCharacter(void *ula, char character);

typedef enum
{
	LLZX8081MachineTypeZX80 = 0,
	LLZX8081MachineTypeZX81
} LLZX8081MachineType;

void llzx8081_setMachineType(void *ula, LLZX8081MachineType type);

#endif
