//
//  DynamicRAM.h
//  Clock Signal
//
//  Created by Thomas Harte on 02/09/2012.
//  Copyright (c) 2012 acrossair. All rights reserved.
//

#ifndef Clock_Signal_DynamicRAM_h
#define Clock_Signal_DynamicRAM_h

typedef enum
{
	CSDynamicRAMType4116
} CSDynamicRAMType;

void *csDynamicRAM_createOnBus(void *bus, CSDynamicRAMType type);

/*

	Depending on which type of RAM chip you create,
	the following lines may be honoured. At the minute
	I'm emulating 1-bit RAM chips only.

*/

// there will be (up to) 8 address lines; if the chip you're
// emulating has fewer then only the bottom ones will be used
#define CSComponentDynamicRAMAddressMask		0xff
#define CSComponentDynamicRAMAddressShift		0

/*

	The following signals are also used:

*/

// chip enable and write enable
#define CSComponentDynamicRAMSignalChipEnable	0x10000
#define CSComponentDynamicRAMSignalWrite		0x20000

// the row-address and column-address strobes
#define CSComponentDynamicRAMSignalRAS			0x40000
#define CSComponentDynamicRAMSignalCAS			0x80000

// and data input and output (which are very commonly
// wired together)
#define CSComponentDynamicRAMSignalDataInput	0x100
#define CSComponentDynamicRAMSignalDataOutput	0x200

#endif
