//
//  Z80.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_Z80_h
#define LLZ80_Z80_h

/*

	Notes on Patterns
	=================

	This Z80 core is intended to be accurate to the nearest
	half cycle.

	External devices connect to the Z80 via its pins. This
	code is written in terms of 'signals', which are the things
	carried by pins. Quite a lot of them are Boolean, but the
	data signal is a collection of eight of the original pins
	and the address signal is a collection of sixteen of the
	original pins.
	
	Connected devices are observers. The simulated Z80 actually
	has quite a lot of boiler-plate logic built in. So observers
	generally request to be notified only when a given test
	is satisfied.

	Supplied tests are:

		-	when any of a given set of Boolean signals changes value
		-	when a given set of Boolean signals assumes a specified
			state
		-	a combination of the above two

	For the purposes of supplying debugging tools and for loading
	and saving machine state files, the following functionality is
	also provided:

		-	call outs in between every instruction fetch
		-	reading and writing of otherwise unexposed internal
			state, including all registers
		-	an internal half-cycle timing count is kept, which
			can be read from or written to

	Note that the semantics related to active lines may initially
	be confusing. To allow logic to be expressed clearly in C, an
	active Boolean signal has a non-zero value and an inactive
	signal has a zero value. The alternative was to give an active
	line a zero value because on the Z80 lines are active low and
	both zero and a low signal indicate absence.

	Although I was conflicted and the aim of this simulator is to
	be accurate to real hardware, I decided to favour semantics
	over the voltage metaphor.

*/

#include "BusState.h"

/*

	Creation and destruction. We're reference counting.

		Use llz80_create to create a new Z80 instance.
		You'll get back NULL on failure or an opaque
		handle to a Z80, which is an owning reference.

		Use llz80_retain and llz80_release to increment
		and decrement the retain count. The Z80 will
		be deallocated and all memory released back
		to the system when the retain count gets to zero.

*/
void *llz80_createOnBus(void *bus);	// the returned object conforms to csObject, and
									// to csBusComponent

/*

	Signal usage.

*/

// signals the Z80 will produce
#define	LLZ80SignalInputOutputRequest			0x10000000000
#define	LLZ80SignalMachineCycleOne				0x20000000000
#define	LLZ80SignalRead							0x40000000000
#define	LLZ80SignalWrite						0x80000000000
#define	LLZ80SignalMemoryRequest				0x100000000000
#define	LLZ80SignalRefresh						0x200000000000
#define	LLZ80SignalBusAcknowledge				0x400000000000
#define	LLZ80SignalHalt							0x800000000000

// signals the Z80 will react to
#define	LLZ80SignalInterruptRequest				0x1000000000000
#define	LLZ80SignalNonMaskableInterruptRequest	0x2000000000000
#define	LLZ80SignalReset						0x4000000000000
#define	LLZ80SignalWait							0x8000000000000
#define	LLZ80SignalBusRequest					0x10000000000000

// the z80 will also respond to the bus clock signal;
// 8 data + 16 address + the 13 listed + the clock signal
// + 5v and ground = the 40 pins of the Z80

/*

	Monitoring functionality.

		A bunch of unrealistic hooks that allow certain
		components of the Z80's internal state to be
		observed or polled.

		Specifically: an observer can be notified whenever
		a new instruction fetch is about to occur (ie, in
		between opcodes).

		Internal registers can be read or written. You can't
		do that on a real Z80 and no external emulated
		components should do so. This functionality is
		provided primarily so that debuggers can be built
		and state saves can be implemented.

*/

typedef void (* llz80_instructionObserver)(void *z80, void *context);
void *llz80_monitor_addInstructionObserver(void *z80, llz80_instructionObserver observer, void *context);
void llz80_monitor_removeInstructionObserver(void *z80, void *observer);

typedef enum
{
	// normal registers
	LLZ80MonitorValueARegister,	LLZ80MonitorValueFRegister,
	LLZ80MonitorValueBRegister,	LLZ80MonitorValueCRegister,
	LLZ80MonitorValueDRegister,	LLZ80MonitorValueERegister,
	LLZ80MonitorValueHRegister,	LLZ80MonitorValueLRegister,

	LLZ80MonitorValueAFRegister,
	LLZ80MonitorValueBCRegister,
	LLZ80MonitorValueDERegister,
	LLZ80MonitorValueHLRegister,

	// dash registers
	LLZ80MonitorValueADashRegister,	LLZ80MonitorValueFDashRegister,
	LLZ80MonitorValueBDashRegister,	LLZ80MonitorValueCDashRegister,
	LLZ80MonitorValueDDashRegister,	LLZ80MonitorValueEDashRegister,
	LLZ80MonitorValueHDashRegister,	LLZ80MonitorValueLDashRegister,

	LLZ80MonitorValueAFDashRegister,
	LLZ80MonitorValueBCDashRegister,
	LLZ80MonitorValueDEDashRegister,
	LLZ80MonitorValueHLDashRegister,

	// miscellaneous registers
	LLZ80MonitorValueRRegister,
	LLZ80MonitorValueIRegister,

	// special-purpose registers
	LLZ80MonitorValueIXRegister,
	LLZ80MonitorValueIYRegister,
	LLZ80MonitorValueSPRegister,
	LLZ80MonitorValuePCRegister,

	// flags
	LLZ80MonitorValueIFF1Flag,
	LLZ80MonitorValueIFF2Flag,

	// misc
	LLZ80MonitorValueInterruptMode,

	// fictitious
	LLZ80MonitorValueHalfCyclesToDate

} LLZ80MonitorValue;

/*

	The getter and setter for those 'values' listed above, which
	are all registers and register pairs, the interrupt flags and
	mode and the count of half cycles since this z80 began running.

	The half cycle count is guaranteed to be accurate to half-a-cyle.

	Most other values are guaranteed to be accurate only to within
	the current whole instruction. Since these aren't exposed by a
	real Z80, it's more than sufficiently accurate for a register
	modified by an opcode to be modified anywhere during the time
	that operation is documented to take. 

	The exceptions are the I and R registers. They are exposed by
	a real Z80 since together they form the refresh address. They
	therefore should be accurate to the half cycle.

	Corollary: if you're writing values, you probably want to do
	it within an instruction observer, to ensure you don't adjust
	state mid-operation and end up with an unpredictable result.

*/
unsigned int llz80_monitor_getInternalValue(void *z80, LLZ80MonitorValue key);
void llz80_monitor_setInternalValue(void *z80, LLZ80MonitorValue key, unsigned int value);

uint64_t llz80_monitor_getBusLineState(void *z80);

#endif
