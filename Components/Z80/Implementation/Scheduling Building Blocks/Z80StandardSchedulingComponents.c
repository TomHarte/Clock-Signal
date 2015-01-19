//
//  StandardSchedulingComponents.c
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80StandardSchedulingComponents.h"

LLZ80iop_restrict(llz80_iop_setReadAndMemoryRequest_imp)
{
	llz80_setLinesActive(z80, LLZ80SignalRead | LLZ80SignalMemoryRequest);
}

LLZ80iop_restrict(llz80_iop_setMemoryRequest_imp)
{
	llz80_setLinesActive(z80, LLZ80SignalMemoryRequest);
}

LLZ80iop_restrict(llz80_iop_incrementProgramCounter_imp)
{
	z80->pcRegister.fullValue++;
}

LLZ80iop_restrict(llz80_iop_incrementTemporaryAddress_imp)
{
	z80->temporaryAddress.fullValue++;
}

LLZ80iop_restrict(llz80_iop_incrementStackPointer_imp)
{
	z80->spRegister.fullValue++;
}

LLZ80iop_restrict(llz80_iop_decrementStackPointer_imp)
{
	z80->spRegister.fullValue--;
}

LLZ80iop_restrict(llz80_iop_setPCToTemporaryAddress_imp)
{
	z80->pcRegister.fullValue = z80->temporaryAddress.fullValue;
}

void llz80_schedulePauseForCycles(LLZ80ProcessorState *const z80, unsigned int numberOfCycles)
{
	while(numberOfCycles--)
	{
		llz80_beginNewHalfCycle(z80);
		llz80_beginNewHalfCycle(z80);
	}
}

const LLZ80InternalInstructionFunction llz80_iop_setReadAndMemoryRequest = llz80_iop_setReadAndMemoryRequest_imp;
const LLZ80InternalInstructionFunction llz80_iop_setMemoryRequest = llz80_iop_setMemoryRequest_imp;

const LLZ80InternalInstructionFunction llz80_iop_incrementProgramCounter = llz80_iop_incrementProgramCounter_imp;
const LLZ80InternalInstructionFunction llz80_iop_incrementTemporaryAddress = llz80_iop_incrementTemporaryAddress_imp;
const LLZ80InternalInstructionFunction llz80_iop_incrementStackPointer = llz80_iop_incrementStackPointer_imp;
const LLZ80InternalInstructionFunction llz80_iop_decrementStackPointer = llz80_iop_decrementStackPointer_imp;

const LLZ80InternalInstructionFunction llz80_iop_setPCToTemporaryAddress = llz80_iop_setPCToTemporaryAddress_imp;
