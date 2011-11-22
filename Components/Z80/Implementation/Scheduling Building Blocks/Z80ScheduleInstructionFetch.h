//
//  ScheduleInstructionFetch.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_ScheduleInstructionFetch_h
#define LLZ80_ScheduleInstructionFetch_h

#include "../Z80Internals.h"

void llz80_scheduleInstructionFetchForFunction(
	LLZ80ProcessorState *z80,
	LLZ80InternalInstructionFunction function,
	LLZ80RegisterPair *indexRegister,
	bool addOffset);

/*
	a pseudo instruction fetch is defined to be one that
	doesn't increment the PC and throws away the result
*/
void llz80_schedulePseudoInstructionFetch(LLZ80ProcessorState *z80);

void llz80_scheduleNOP(LLZ80ProcessorState *z80);
void llz80_scheduleIRQAcknowledge(LLZ80ProcessorState *z80);

#endif
