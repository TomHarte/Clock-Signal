//
//  StandardSchedulingComponents.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_StandardSchedulingComponents_h
#define LLZ80_StandardSchedulingComponents_h

#include "../Z80Internals.h"

extern LLZ80InternalInstructionFunction llz80_iop_setReadAndMemoryRequest;
extern LLZ80InternalInstructionFunction llz80_iop_setMemoryRequest;

extern LLZ80InternalInstructionFunction llz80_iop_incrementProgramCounter;
extern LLZ80InternalInstructionFunction llz80_iop_incrementTemporaryAddress;
extern LLZ80InternalInstructionFunction llz80_iop_incrementStackPointer;
extern LLZ80InternalInstructionFunction llz80_iop_decrementStackPointer;

extern LLZ80InternalInstructionFunction llz80_iop_setPCToTemporaryAddress;

void llz80_schedulePauseForCycles(LLZ80ProcessorState *const z80, unsigned int numberOfCycles);

#endif
