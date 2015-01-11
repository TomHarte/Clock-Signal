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

extern const LLZ80InternalInstructionFunction llz80_iop_setReadAndMemoryRequest;
extern const LLZ80InternalInstructionFunction llz80_iop_setMemoryRequest;

extern const LLZ80InternalInstructionFunction llz80_iop_incrementProgramCounter;
extern const LLZ80InternalInstructionFunction llz80_iop_incrementTemporaryAddress;
extern const LLZ80InternalInstructionFunction llz80_iop_incrementStackPointer;
extern const LLZ80InternalInstructionFunction llz80_iop_decrementStackPointer;

extern const LLZ80InternalInstructionFunction llz80_iop_setPCToTemporaryAddress;

extern void llz80_schedulePauseForCycles(LLZ80ProcessorState *const z80, unsigned int numberOfCycles);

#endif
