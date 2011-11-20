//
//  BusState.h
//  Clock Signal
//
//  Created by Thomas Harte on 20/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_BusState_h
#define ClockSignal_BusState_h

#include "stdint.h"
#include "stdbool.h"

// this is the most basic description of bus state
typedef struct
{
	// current line values; open collector logic means
	// that unused values should be left high
	uint64_t lineValues;

	// a bitfield indicating which lines are 'inactive'
	// in the sense that whoever sent this message isn't
	// actively loading some value onto them
//	uint64_t highImpedanceMask;

} CSBusState;

// a way to get a bus state that's initialised so that
// all lines have an unloaded value. This guards against
// problems if the bus state struct is extended in the
// future.
CSBusState csBus_defaultState(void);

// this is the most general description of the test the
// bus can perform to determine whether to pass
// a message to an attached component. Giving the bus
// responsibility here allows some optimisation in
// message passing
typedef struct
{
	uint64_t changedLines;
	uint64_t lineMask;
	uint64_t lineValues;

	bool signalOnTrueOnly;

	// The standard test is:
	//
	//	(1)	components are active for messaging when bus&lineMask == lineValues;
	//	(2)	components are messaged immediately when they become active and
	//		immediately when they become inactive;
	//	(3)	components are also messaged if any of the lines in changedLines
	//		changes value while they are active.
	//
	// Components that have requested to be signalled on true only may not
	// be signalled when they become inactive, if it's faster for the bus not
	// to message them (so, it's a hint, effectively to say that the component
	// acts only on the true condition)

} CSBusCondition;

uint64_t csBusCondition_observedLines(CSBusCondition condition);

// quick helpers for building messaging tests

// applies the complete list
CSBusCondition csBus_maskCondition(uint64_t observedLines, uint64_t lineMask, uint64_t lineValues, bool signalOnTrueOnly);

// applies the test: do the relevant lines have the dictated values?
CSBusCondition csBus_testCondition(uint64_t relevantLines, uint64_t lineValues, bool signalOnTrueOnly);

// applies the test: are all of the relevant lines currently set?
CSBusCondition csBus_setCondition(uint64_t relevantLines, bool signalOnTrueOnly);

// applies the test: are all of the relevant lines currently reset?
CSBusCondition csBus_resetCondition(uint64_t relevantLines, bool signalOnTrueOnly);

// applies the test: have any of the relevant lines changed value?
CSBusCondition csBus_changeCondition(uint64_t relevantLines);

// the impossible condition is always false, so a component that
// connects with the impossible condition will never be signalled
CSBusCondition csBus_impossibleCondition(void);
bool csBusCondition_isImpossible(CSBusCondition condition);

#endif
