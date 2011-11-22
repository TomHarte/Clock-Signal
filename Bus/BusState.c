//
//  BusState.c
//  Clock Signal
//
//  Created by Thomas Harte on 22/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "BusState.h"

CSBusState csBus_defaultState(void)
{
	CSBusState state;

	state.lineValues = 0xffffffffffffffff;

	return state;
}

uint64_t csBusCondition_observedLines(CSBusCondition condition)
{
	return condition.changedLines | condition.lineMask;
}

CSBusCondition csBus_maskCondition(uint64_t observedLines, uint64_t lineMask, uint64_t lineValues, bool signalOnTrueOnly)
{
	CSBusCondition test;

	test.changedLines = observedLines;
	test.lineMask = lineMask;
	test.lineValues = lineValues;
	test.signalOnTrueOnly = signalOnTrueOnly;

	return test;
}

CSBusCondition csBus_testCondition(uint64_t relevantLines, uint64_t lineValues, bool signalOnTrueOnly)
{
	CSBusCondition test;

	test.changedLines = 0;
	test.lineMask = relevantLines;
	test.lineValues = lineValues;
	test.signalOnTrueOnly = signalOnTrueOnly;

	return test;
}

CSBusCondition csBus_setCondition(uint64_t relevantLines, bool signalOnTrueOnly)
{
	CSBusCondition test;

	test.changedLines = 0;
	test.lineMask = relevantLines;
	test.lineValues = relevantLines;
	test.signalOnTrueOnly = signalOnTrueOnly;

	return test;
}

CSBusCondition csBus_resetCondition(uint64_t relevantLines, bool signalOnTrueOnly)
{
	CSBusCondition test;

	test.changedLines = 0;
	test.lineMask = relevantLines;
	test.lineValues = 0;
	test.signalOnTrueOnly = signalOnTrueOnly;

	return test;
}

CSBusCondition csBus_changeCondition(uint64_t relevantLines)
{
	CSBusCondition test;

	test.changedLines = relevantLines;
	test.lineMask = 0;
	test.lineValues = 0;
	test.signalOnTrueOnly = true;

	return test;
}

CSBusCondition csBus_impossibleCondition(void)
{
	CSBusCondition test;

	test.changedLines = 0;
	test.lineMask = 0;
	test.lineValues = 1;

	return test;
}

bool csBusCondition_isImpossible(CSBusCondition condition)
{
	// test can never be satisfied if:
	//
	//	(i)		no lines are observed for changes, and
	//	(ii)	the mask condition requires some lines to be set but the mask doesn't
	//			allow them to be
	return
		!condition.changedLines &&
		condition.lineValues &&
		!(condition.lineMask & condition.lineValues);
}
