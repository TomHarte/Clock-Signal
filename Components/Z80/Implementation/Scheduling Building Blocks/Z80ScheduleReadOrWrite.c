//
//  ScheduleReadOrWrite.c
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#include "Z80ScheduleReadOrWrite.h"
#include "Z80StandardSchedulingComponents.h"


/*

	Reading

*/
static void llz80_iop_readOrWriteHalfCycle1(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	llz80_setAddress(z80, *instruction->extraData.readOrWriteAddress.address);
	llz80_setDataExpectingInput(z80);
}

static void llz80_iop_readHalfCycle6(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	*instruction->extraData.readOrWriteValue.value = llz80_getDataInput(z80);
	llz80_setLinesInactive(z80, LLZ80SignalRead | LLZ80SignalMemoryRequest);
}


LLZ80InternalInstruction *llz80_scheduleRead(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_readOrWriteHalfCycle1)->extraData.readOrWriteAddress.address = address;
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setReadAndMemoryRequest);
	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80)->extraData.advance.isWaitCycle = true;
	llz80_beginNewHalfCycle(z80);

	LLZ80InternalInstruction *instruction;
	instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_iop_readHalfCycle6);
	instruction->extraData.readOrWriteValue.value = value;

	return instruction;
}


/*

	Writing

*/
static void llz80_iop_writeHalfCycle2(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	llz80_setDataOutput(z80, *instruction->extraData.readOrWriteValue.value);
	llz80_setLinesActive(z80, LLZ80SignalMemoryRequest);
}

static void llz80_iop_writeHalfCycle4(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	llz80_setLinesActive(z80, LLZ80SignalWrite);
}

static void llz80_iop_writeHalfCycle6(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	llz80_setLinesInactive(z80, LLZ80SignalWrite | LLZ80SignalMemoryRequest);
}


LLZ80InternalInstruction *llz80_scheduleWrite(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address)
{
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_readOrWriteHalfCycle1)->extraData.readOrWriteAddress.address = address;
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_writeHalfCycle2)->extraData.readOrWriteValue.value = value;

	llz80_beginNewHalfCycle(z80);
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_writeHalfCycle4)->extraData.advance.isWaitCycle = true;

	llz80_beginNewHalfCycle(z80);
	LLZ80InternalInstruction *instruction;
	instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_iop_writeHalfCycle6);
	instruction->extraData.readOrWriteValue.value = value;

	return instruction;
}
