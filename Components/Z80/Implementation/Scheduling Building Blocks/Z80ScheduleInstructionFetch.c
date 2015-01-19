//
//  ScheduleInstructionFetch.c
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80StandardSchedulingComponents.h"
#include "Z80ScheduleInstructionFetch.h"

LLZ80iop_restrict(llz80_iop_instructionfetchHalfCycle1)
{
	// load the program counter to the address line;
	// set m1, increment the refresh register
	llz80_setAddress(z80, z80->pcRegister.fullValue);
	llz80_setLinesActive(z80, LLZ80SignalMachineCycleOne);
	llz80_setDataExpectingInput(z80);

	// increment the program counter; since it's purely
	// internal, now is as good a time as any
	z80->pcRegister.fullValue++;
}

LLZ80iop_restrict(llz80_iop_instructionfetchHalfCycle1_noIncrement)
{
	// load the program counter to the address line;
	// set m1, increment the refresh register
	llz80_setAddress(z80, z80->pcRegister.fullValue);
	llz80_setLinesActive(z80, LLZ80SignalMachineCycleOne);
}

LLZ80iop_restrict(llz80_iop_instructionfetchHalfCycle5)
{
	// store the current value of the data line away
	// for safe keeping
	z80->temporary8bitValue = llz80_getDataInput(z80);

	// start the refresh cycle
	llz80_setLinesInactive(z80, LLZ80SignalMemoryRequest | LLZ80SignalRead | LLZ80SignalMachineCycleOne);
	llz80_setLinesActive(z80, LLZ80SignalRefresh);
	llz80_setAddress(z80, (z80->iRegister << 8) | z80->rRegister);
	z80->rRegister = (z80->rRegister&0x80) | ((z80->rRegister+1)&0x7f);
}

LLZ80iop_restrict(llz80_iop_irqAcknowledgeHalfCycle6)
{
	llz80_setLinesActive(z80, LLZ80SignalInputOutputRequest);
}

LLZ80iop_restrict(llz80_iop_instructionfetchHalfCycle8)
{
	// end the refresh cycle
	llz80_setLinesInactive(z80, LLZ80SignalMemoryRequest | LLZ80SignalRefresh);
}

LLZ80iop_restrict(llz80_iop_irqAcknowledgeHalfCycle9)
{
	z80->temporary8bitValue = llz80_getDataInput(z80);
	llz80_setLinesInactive(z80, LLZ80SignalMachineCycleOne | LLZ80SignalInputOutputRequest);
}

void llz80_scheduleInstructionFetchForFunction(
	LLZ80ProcessorState *const z80,
	LLZ80InternalInstructionFunction function,
	LLZ80RegisterPair *const indexRegister,
	bool addOffset)
{
	llz80m_beginScheduling();

	llz80m_scheduleHalfCycleForFunction(llz80_iop_instructionfetchHalfCycle1);
	llz80m_scheduleHalfCycleForFunction(llz80_iop_setReadAndMemoryRequest);

	llz80m_beginNewHalfCycle(false);
	llz80m_beginNewHalfCycle(true);

	llz80m_scheduleHalfCycleForFunction(llz80_iop_instructionfetchHalfCycle5);
	llz80m_scheduleHalfCycleForFunction(llz80_iop_setMemoryRequest);

	llz80m_beginNewHalfCycle(false);
	llz80m_scheduleHalfCycleForFunction(llz80_iop_instructionfetchHalfCycle8);

	z80->scheduledInstructions[writePointer].function = function;
	z80->scheduledInstructions[writePointer].extraData.opcodeDecode.indexRegister = indexRegister;
	z80->scheduledInstructions[writePointer].extraData.opcodeDecode.addOffset = addOffset;
	llz80m_advanceWritePointer();

	llz80m_endScheduling();

	// TODO: refresh should actually last a half cycle longer
}

void llz80_schedulePseudoInstructionFetch(LLZ80ProcessorState *const z80)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle1_noIncrement);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setReadAndMemoryRequest);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle5);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setMemoryRequest);

	llz80_beginNewHalfCycle(z80);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle8);
}

void llz80_scheduleNOP(LLZ80ProcessorState *const z80)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle1_noIncrement);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setReadAndMemoryRequest);
//	llz80_schedulePauseForCycles(z80, 1);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle5);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setMemoryRequest);

	llz80_beginNewHalfCycle(z80);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle8);
}

void llz80_scheduleIRQAcknowledge(LLZ80ProcessorState *const z80)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_instructionfetchHalfCycle1_noIncrement);
	llz80_beginNewHalfCycle(z80);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80);

	llz80_beginNewHalfCycle(z80);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_irqAcknowledgeHalfCycle6);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_irqAcknowledgeHalfCycle9);
	llz80_beginNewHalfCycle(z80);
}
