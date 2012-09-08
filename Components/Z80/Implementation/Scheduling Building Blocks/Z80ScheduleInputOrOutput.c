//
//  ScheduleInputOrOutput.c
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80ScheduleInputOrOutput.h"
#include "Z80StandardSchedulingComponents.h"

/*

	Input

*/
static void llz80_iop_inputOrOutputHalfCycle1(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setAddress(z80, *instruction->extraData.readOrWriteAddress.address);
}

static void llz80_iop_inputHalfCycle3(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setLinesActive(z80, LLZ80SignalRead | LLZ80SignalInputOutputRequest);
}

static void llz80_iop_inputHalfCycle8(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	*instruction->extraData.readOrWriteValue.value = llz80_getDataInput(z80);
	llz80_setLinesInactive(z80, LLZ80SignalRead | LLZ80SignalInputOutputRequest);
}


LLZ80InternalInstruction *llz80_scheduleInput(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_inputOrOutputHalfCycle1)->extraData.readOrWriteAddress.address = address;
	llz80_beginNewHalfCycle(z80);

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_inputHalfCycle3);
	llz80_beginNewHalfCycle(z80);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;

	llz80_beginNewHalfCycle(z80);

	LLZ80InternalInstruction *instruction;
	instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_iop_inputHalfCycle8);
	instruction->extraData.readOrWriteValue.value = value;

	return instruction;
}


/*

	Output

*/
static void llz80_iop_outputHalfCycle2(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setDataOutput(z80, *instruction->extraData.readOrWriteValue.value);
}

static void llz80_iop_outputHalfCycle3(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setLinesActive(z80, LLZ80SignalInputOutputRequest | LLZ80SignalWrite);
}

static void llz80_iop_outputHalfCycle7(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setLinesInactive(z80, LLZ80SignalInputOutputRequest | LLZ80SignalWrite);
}


LLZ80InternalInstruction *llz80_scheduleOutput(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address)
{

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_inputOrOutputHalfCycle1)->extraData.readOrWriteAddress.address = address;
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_outputHalfCycle2)->extraData.readOrWriteValue.value = value;

	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_outputHalfCycle3);
	llz80_beginNewHalfCycle(z80);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;

	llz80_beginNewHalfCycle(z80);
	return llz80_scheduleHalfCycleForFunction(z80, llz80_iop_outputHalfCycle7);
}
