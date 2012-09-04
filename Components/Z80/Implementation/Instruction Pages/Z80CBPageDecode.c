//
//  CBPageDecode.c
//  LLZ80
//
//  Created by Thomas Harte on 13/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include "Z80EDPageDecode.h"

#include "../Scheduling Building Blocks/Z80ScheduleReadOrWrite.h"
#include "../Scheduling Building Blocks/Z80StandardSchedulingComponents.h"

#include "../Operations/Z80SetResetTestOps.h"
#include "../Operations/Z80RotateAndShiftOps.h"

static void llz80_iop_copyTemporary8BitValueToRegister(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	*instruction->extraData.referenceToRegister.registerReference = z80->temporary8bitValue;
}

static void llz80_iop_doShiftOp(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	switch(instruction->extraData.ALUOrShiftOp.operation)
	{
		case 0: llz80_rlc	(z80, &z80->temporary8bitValue); break;
		case 1: llz80_rrc	(z80, &z80->temporary8bitValue); break;
		case 2: llz80_rl	(z80, &z80->temporary8bitValue); break;
		case 3: llz80_rr	(z80, &z80->temporary8bitValue); break;
		case 4: llz80_sla	(z80, &z80->temporary8bitValue); break;
		case 5: llz80_sra	(z80, &z80->temporary8bitValue); break;
		case 6: llz80_sll	(z80, &z80->temporary8bitValue); break;
		case 7: llz80_srl	(z80, &z80->temporary8bitValue); break;
	}
}

#define llz80_metadata_setMaskAndValue(metadata, maskVal, valueVal)	\
	(metadata)->extraData.bitOp.mask = (maskVal);\
	(metadata)->extraData.bitOp.value = (valueVal);

static void llz80_iop_CBPageDecode_imp(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	uint8_t opcode = z80->temporary8bitValue;
	bool addOffset = instruction->extraData.opcodeDecode.addOffset;

	uint8_t *rTable[] =
	{
		&z80->bcRegister.bytes.high,
		&z80->bcRegister.bytes.low,
		&z80->deRegister.bytes.high,
		&z80->deRegister.bytes.low,
		&z80->hlRegister.bytes.high,
		&z80->hlRegister.bytes.low,
		NULL,
		&z80->aRegister
	};
	uint8_t *value = rTable[opcode&7];

	if(addOffset)
	{
		// accesses that nominate a register do the operation on the value
		// from the indexed address and also store the result to that register,
		// with the exception of BIT that doesn't store anything...
		switch(opcode)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			{
				int operation = opcode >> 3;

				// read value; the address was calculated while the opcode was read
				llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
				llz80_scheduleFunction(z80, llz80_iop_doShiftOp)->extraData.ALUOrShiftOp.operation = operation;
				if(value)
				{
					llz80_scheduleFunction(z80, llz80_iop_copyTemporary8BitValueToRegister)
						->extraData.referenceToRegister.registerReference = value;
				}
				llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
			}
			break;

			default:	// bit, res, set
			{
				LLZ80InternalInstructionFunction functionTable[] =
				{
					NULL,
					llz80_iop_bit,
					llz80_iop_res, 
					llz80_iop_set
				};

				// read value; the address was calculated while the opcode was read
				llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);

				// do the instruction
				LLZ80InternalInstruction *instruction = llz80_scheduleFunction(z80, functionTable[opcode >> 6]);
				llz80_metadata_setMaskAndValue(instruction, 
						1 << ((opcode >> 3)&7),
						&z80->temporary8bitValue);

				// if this is a res or a set, write the value back
				if(opcode&0x80)
				{
					if(value)
						llz80_scheduleFunction(z80, llz80_iop_copyTemporary8BitValueToRegister)
							->extraData.referenceToRegister.registerReference = value;
					llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
				}

			}
			break;
		}
	}
	else
	{	
		switch(opcode)
		{
			// roll/shift
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			{
				int operation = opcode >> 3;

				if(value)
				{
					switch (operation)
					{
						case 0: llz80_rlc(z80, value); break;
						case 1: llz80_rrc(z80, value); break;
						case 2: llz80_rl(z80, value); break;
						case 3: llz80_rr(z80, value); break;
						case 4: llz80_sla(z80, value); break;
						case 5: llz80_sra(z80, value); break;
						case 6: llz80_sll(z80, value); break;
						case 7: llz80_srl(z80, value); break;
					}
				}
				else
				{
					llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
					llz80_scheduleFunction(z80, llz80_iop_doShiftOp)->extraData.ALUOrShiftOp.operation = operation;
					llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
				}
			}
			break;

			default:	// bit, res, set
			{
				LLZ80InternalInstructionFunction functionTable[] =
				{
					NULL,
					llz80_iop_bit,
					llz80_iop_res, 
					llz80_iop_set
				};

				if(value)
				{
					LLZ80InternalInstruction halfCycleMetaData;
					llz80_metadata_setMaskAndValue(&halfCycleMetaData, 1 << ((opcode >> 3)&7), value);

					functionTable[opcode >> 6](z80, &halfCycleMetaData);
				}
				else
				{
					llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);

					llz80_schedulePauseForCycles(z80, 1);
					LLZ80InternalInstruction *instruction = llz80_scheduleFunction(z80, functionTable[opcode >> 6]);
					llz80_metadata_setMaskAndValue(instruction, 
							1 << ((opcode >> 3)&7),
							&z80->temporary8bitValue);

					// we'll need a write back for res or set...
					if(opcode&0x80)
					{
						llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
					}
				}
			}
			break;
		}
	}
}

LLZ80InternalInstructionFunction llz80_iop_CBPageDecode = llz80_iop_CBPageDecode_imp;
