//
//  StandardPageDecode.c
//  ZX81
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include "Z80StandardPageDecode.h"

#include "../Scheduling Building Blocks/Z80StandardSchedulingComponents.h"
#include "../Scheduling Building Blocks/Z80ScheduleInstructionFetch.h"
#include "../Scheduling Building Blocks/Z80ScheduleReadOrWrite.h"
#include "../Scheduling Building Blocks/Z80ScheduleInputOrOutput.h"

#include "../Operations/Z80BitwiseOps.h"
#include "../Operations/Z80RotateAndShiftOps.h"
#include "../Operations/Z80Arithmetic8BitOps.h"
#include "../Operations/Z80Arithmetic16BitOps.h"

#include "Z80EDPageDecode.h"
#include "Z80CBPageDecode.h"

static void llz80_schedulePop(LLZ80ProcessorState *const z80, LLZ80RegisterPair *const registerPair)
{
	llz80_scheduleRead(z80, &registerPair->bytes.low, &z80->spRegister.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_incrementStackPointer);

	llz80_scheduleRead(z80, &registerPair->bytes.high, &z80->spRegister.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_incrementStackPointer);
}

static void llz80_schedulePush(LLZ80ProcessorState *const z80, LLZ80RegisterPair *const registerPair)
{
	// predecrement the stack pointer
	llz80_scheduleHalfCycleForFunction(z80, llz80_iop_decrementStackPointer);
	llz80_beginNewHalfCycle(z80);

	// store out the old program counter, then adjust it
	llz80_scheduleWrite(z80, &registerPair->bytes.high, &z80->spRegister.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);

	llz80_scheduleWrite(z80, &registerPair->bytes.low, &z80->spRegister.fullValue);
}

static void llz80_scheduleCallToTemporaryAddress(LLZ80ProcessorState *const z80)
{
	llz80_schedulePush(z80, &z80->pcRegister);
	llz80_scheduleFunction(z80, llz80_iop_setPCToTemporaryAddress);
}

static void llz80_schedule8BitReadFromPC(LLZ80ProcessorState *const z80, uint8_t *const targetRegister)
{
	llz80_scheduleRead(z80, targetRegister, &z80->pcRegister.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_incrementProgramCounter);
}

static void llz80_schedule16BitReadFromPC(LLZ80ProcessorState *const z80, LLZ80RegisterPair *const targetRegister)
{
	llz80_schedule8BitReadFromPC(z80, &targetRegister->bytes.low);
	llz80_schedule8BitReadFromPC(z80, &targetRegister->bytes.high);
}

static void llz80_iop_finishPopAF(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_setF(z80, z80->temporary8bitValue);
	z80->spRegister.fullValue++;
}

static void llz80_copyTemporaryAddressToRegister(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	*instruction->extraData.referenceToIndexRegister.indexRegister = z80->temporaryAddress;
}

static void llz80_doALUOp(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	switch(instruction->extraData.ALUOrShiftOp.operation)
	{
		default: break;
		case 0:	llz80_add_8bit(z80, z80->temporary8bitValue);				break;
		case 1:	llz80_addWithCarry_8bit(z80, z80->temporary8bitValue);		break;
		case 2:	llz80_subtract_8bit(z80, z80->temporary8bitValue);			break;
		case 3:	llz80_subtractWithCarry_8bit(z80, z80->temporary8bitValue);	break;
		case 4:	llz80_bitwiseAnd(z80, z80->temporary8bitValue);				break;
		case 5:	llz80_bitwiseXOr(z80, z80->temporary8bitValue);				break;
		case 6:	llz80_bitwiseOr(z80, z80->temporary8bitValue);				break;
		case 7:	llz80_compare(z80, z80->temporary8bitValue);				break;
	}
}

static void llz80_incrementTemporary8BitValue(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_increment_8bit(z80, &z80->temporary8bitValue);
}

static void llz80_decrementTemporary8BitValue(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	llz80_decrement_8bit(z80, &z80->temporary8bitValue);
}

static void llz80_jrConditional(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	z80->pcRegister.fullValue++;

	if(llz80_conditionIsTrue(z80, instruction->extraData.conditional.condition))
	{
		z80->pcRegister.fullValue += (int8_t)z80->temporary8bitValue;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

static void llz80_addOffsetToIndexRegister(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	z80->temporaryAddress.fullValue = 
		instruction->extraData.referenceToIndexRegister.indexRegister->fullValue + z80->temporaryOffset;
}

static void llz80_scheduleCalculationOfSourceAddress(LLZ80ProcessorState *const z80, LLZ80RegisterPair *const indexRegister, bool addOffset)
{
	if(addOffset)
	{
		// load offset
		llz80_schedule8BitReadFromPC(z80, &z80->temporaryOffset);

		// spend 2 cycles working out what the total is
		llz80_schedulePauseForCycles(z80, 1);
		llz80_beginNewHalfCycle(z80);

		LLZ80InternalInstruction *instruction;
		instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_addOffsetToIndexRegister);
		instruction->extraData.referenceToIndexRegister.indexRegister = indexRegister;
	}
	else
		z80->temporaryAddress = *indexRegister;
}

static void llz80_djnz(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	z80->pcRegister.fullValue++;
	z80->bcRegister.bytes.high--;

	if(z80->bcRegister.bytes.high)
	{
		llz80_schedulePauseForCycles(z80, 5);
		z80->pcRegister.fullValue += (int8_t)z80->temporary8bitValue;
	}
}

static void llz80_iop_standardPageDecode_imp(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
{
	LLZ80RegisterPair *const indexRegister = instruction->extraData.opcodeDecode.indexRegister;
	bool addOffset = instruction->extraData.opcodeDecode.addOffset;

	uint8_t *rTable[] =
	{
		&z80->bcRegister.bytes.high,
		&z80->bcRegister.bytes.low,
		&z80->deRegister.bytes.high,
		&z80->deRegister.bytes.low,
		&indexRegister->bytes.high,
		&indexRegister->bytes.low,
		NULL,
		&z80->aRegister
	};

	LLZ80RegisterPair *rpTable[] =
	{
		&z80->bcRegister,
		&z80->deRegister,
		indexRegister,
		&z80->spRegister
	};

	uint8_t opcode = z80->temporary8bitValue;
	switch(opcode)
	{
		default: break;

		case 0x00:	break;	// nop

		case 0xdd:
		case 0xfd:	// do one of the indexed register pages
		{
			LLZ80RegisterPair *newIndexRegister = (opcode == 0xdd) ? &z80->ixRegister : &z80->iyRegister;
			llz80_scheduleInstructionFetchForFunction(z80, llz80_iop_standardPageDecode_imp, newIndexRegister, true);
		}
		break;

		case 0xcb:
		{
			// do something on the CB page;
			// on the CB page you get the displacement
			// before the final part of the opcode, if relevant
			llz80_scheduleCalculationOfSourceAddress(z80, indexRegister, addOffset);
			llz80_scheduleInstructionFetchForFunction(z80, llz80_iop_CBPageDecode, indexRegister, addOffset);
		}
		break;

		case 0xed:
		{
			// do something on the ED page
			llz80_scheduleInstructionFetchForFunction(z80, llz80_iop_EDPageDecode, indexRegister, addOffset);
		}
		break;

		case 0x10:	// djnz
		{
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_djnz);
			llz80_schedulePauseForCycles(z80, 1);
		}
		break;
		
		case 0xe9:	// jp hl (or ix or iy)
			z80->pcRegister = *indexRegister;
		break;

		case 0xc4:	// call (conditional)
		case 0xcc:
		case 0xd4:
		case 0xdc:
		case 0xe4:
		case 0xec:
		case 0xf4:
		case 0xfc:
		case 0xcd:	// call (unconditional)
		{
			// read the target address
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);

			// judge the condition now, since it can't change
			LLZ80Condition condition = (opcode >> 3)&7;
			if((opcode == 0xcd) || llz80_conditionIsTrue(z80, condition))
				llz80_scheduleCallToTemporaryAddress(z80);
		}
		break;

		case 0xc7:
		case 0xcf:
		case 0xd7:
		case 0xdf:
		case 0xe7:
		case 0xef:
		case 0xf7:
		case 0xff:	// RSTs
		{
			// decode the target address
			z80->temporaryAddress.fullValue = opcode & 0x38;

			// schedule a call
			llz80_scheduleCallToTemporaryAddress(z80);
		}
		break;
		
		case 0xc9:	// ret (unconditional)
		{
			llz80_schedulePop(z80, &z80->pcRegister);
		}
		break;

		case 0xc0:	// ret (conditional)
		case 0xc8:
		case 0xd0:
		case 0xd8:
		case 0xe0:
		case 0xe8:
		case 0xf0:
		case 0xf8:
		{
			// the cycle the Z80 would spend evaluating the condition, presumably
			llz80_schedulePauseForCycles(z80, 1);

			// judge the condition now, since it can't change
			LLZ80Condition condition = (opcode >> 3)&7;
			if(llz80_conditionIsTrue(z80, condition))
				llz80_schedulePop(z80, &z80->pcRegister);
		}
		break;

		case 0xc1:	// pop rr
		case 0xd1:
		case 0xe1:
		{
			llz80_schedulePop(z80, rpTable[(opcode >> 4)&3]);
		}
		break;
		
		case 0xc5:	// push rr
		case 0xd5:
		case 0xe5:
		{
			llz80_schedulePush(z80, rpTable[(opcode >> 4)&3]);
		}
		break;
		
		case 0xf1:	// pop af
		{
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->spRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementStackPointer);

			llz80_scheduleRead(z80, &z80->aRegister, &z80->spRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_finishPopAF);
		}
		break;
		
		case 0xf5:	// push af
		{
			// get f
			z80->temporary8bitValue = llz80_getF(z80);

			// predecrement the stack pointer
			llz80_scheduleHalfCycleForFunction(z80, llz80_iop_decrementStackPointer);
			llz80_beginNewHalfCycle(z80);

			llz80_scheduleWrite(z80, &z80->aRegister, &z80->spRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);

			llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->spRegister.fullValue);
		}
		break;

		case 0x01:	// ld rr, nn
		case 0x11:
		case 0x21:
		case 0x31:
		{
			llz80_schedule16BitReadFromPC(z80, rpTable[opcode >> 4]);
		}
		break;

		case 0x2a:	// ld hl, (nn) (or ix/iy)
		{
			// load target address
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);

			// load the index register from the target address
			llz80_scheduleRead(z80, &indexRegister->bytes.low, &z80->temporaryAddress.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementTemporaryAddress);

			llz80_scheduleRead(z80, &indexRegister->bytes.high, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x22:	// ld (nn), hl (or ix/iy)
		{
			// load target address
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);

			// store the index register from the target address
			llz80_scheduleWrite(z80, &indexRegister->bytes.low, &z80->temporaryAddress.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementTemporaryAddress);

			llz80_scheduleWrite(z80, &indexRegister->bytes.high, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x32:	// ld (nn), a
		{
			// load target address
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);

			// store the index register from the target address
			llz80_scheduleWrite(z80, &z80->aRegister, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x3a:	// ld a, (nn)
		{
			// load target address
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);

			// load the index register from the target address
			llz80_scheduleRead(z80, &z80->aRegister, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x02:	// ld (bc), a
			llz80_scheduleWrite(z80, &z80->aRegister, &z80->bcRegister.fullValue);
		break;
		case 0x0a:	// ld a, (bc)
			llz80_scheduleRead(z80, &z80->aRegister, &z80->bcRegister.fullValue);
		break;

		case 0x12:	// ld (de), a
			llz80_scheduleWrite(z80, &z80->aRegister, &z80->deRegister.fullValue);
		break;
		case 0x1a:	// ld a, (de)
			llz80_scheduleRead(z80, &z80->aRegister, &z80->deRegister.fullValue);
		break;
		
		case 0x27:	// daa (yuck)
		{
			int lowNibble = z80->aRegister & 0xf;
			int highNibble = z80->aRegister >> 4;

			int amountToAdd = 0;

			if(z80->generalFlags & LLZ80FlagCarry)
			{
				if(lowNibble > 0x9 || z80->generalFlags&LLZ80FlagHalfCarry)
					amountToAdd = 0x66;
				else
					amountToAdd = 0x60;
			}
			else
			{
				if(z80->generalFlags & LLZ80FlagHalfCarry)
				{
					amountToAdd = (highNibble > 0x9) ? 0x66 : 0x60;
				}
				else
				{
					if(lowNibble > 0x9)
					{
						if(highNibble > 0x8)
							amountToAdd = 0x66;
						else
							amountToAdd = 0x6;
					}
					else
					{
						amountToAdd = (highNibble > 0x9) ? 0x60 : 0x00;
					}
				}
			}

			int newCarry = z80->generalFlags & LLZ80FlagHalfCarry;
			if(!newCarry)
			{
				if(lowNibble > 0x9)
				{
					if(highNibble > 0x8) newCarry = LLZ80FlagCarry;
				}
				else
				{
					if(highNibble > 0x9) newCarry = LLZ80FlagCarry;
				}
			}

			int newHalfCarry = 0;
			if(z80->generalFlags&LLZ80FlagSubtraction)
			{
				newHalfCarry = (lowNibble > 0x9) ? LLZ80FlagHalfCarry : 0;
			}
			else
			{
				if(z80->generalFlags&LLZ80FlagHalfCarry)
				{
					newHalfCarry = (lowNibble < 0x6) ? LLZ80FlagHalfCarry : 0;
				}
			}
			
			z80->aRegister += amountToAdd;

			z80->lastSignResult = z80->lastZeroResult =
			z80->bit5And3Flags = z80->aRegister;
			
			uint8_t parity = z80->aRegister;
			parity ^= (parity >> 4);
			parity ^= (parity >> 2);
			parity ^= (parity >> 1);

			z80->generalFlags =
				(uint8_t)(
					newCarry |
					newHalfCarry |
					((parity&1) << 3) |
					(z80->generalFlags&LLZ80FlagSubtraction));
		}
		break;
		
		case 0x3f:	// ccf
		{
			z80->bit5And3Flags = z80->aRegister;
			z80->generalFlags =
				(uint8_t)(
					(z80->generalFlags & LLZ80FlagParityOverflow) |
					((z80->generalFlags & LLZ80FlagCarry) << 4) |	// so half carry is what carry was
					((z80->generalFlags^LLZ80FlagCarry)&LLZ80FlagCarry));
				// implicitly setting subtract and half carry to 0
		}
		break;

		case 0x37:	// scf
		{
			z80->bit5And3Flags = z80->aRegister;
			z80->generalFlags =
				(z80->generalFlags & LLZ80FlagParityOverflow) |
				LLZ80FlagCarry;
				// implicitly setting subtract and half carry to 0
		}
		break;
		
		case 0x2f:	// cpl
		{
			z80->aRegister ^= 0xff;
			z80->generalFlags |=
				LLZ80FlagHalfCarry |
				LLZ80FlagSubtraction;
			z80->bit5And3Flags = z80->aRegister;
		}
		break;

		case 0x06:
		case 0x0e:
		case 0x16:
		case 0x1e:
		case 0x26:
		case 0x2e:
		case 0x36:
		case 0x3e:	// ld r, n
		{
			uint8_t *destination = rTable[opcode >> 3];

			if(destination)
			{
				// this is a simple immediate load
				llz80_schedule8BitReadFromPC(z80, destination);
			}
			else
			{
				// fine, this is an immediate load and store,
				// possibly with an offset in the case of
				// faintly ridiculous ld (ix+d), n
				llz80_scheduleCalculationOfSourceAddress(z80, indexRegister, addOffset);
				llz80_schedule8BitReadFromPC(z80, &z80->temporary8bitValue);

				llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
			}
		}
		break;

		case 0x0b:
		case 0x1b:
		case 0x2b:
		case 0x3b:	// dec rr, which doesn't set any flags
		{
			rpTable[opcode >> 4]->fullValue --;
			llz80_schedulePauseForCycles(z80, 2);	// pretend that took 2 cycles
		}
		break;
		
		case 0x03:
		case 0x13:
		case 0x23:
		case 0x33:	// inc rr, which doesn't set any flags
		{
			rpTable[opcode >> 4]->fullValue ++;
			llz80_schedulePauseForCycles(z80, 2);	// pretend that took 2 cycles
		}
		break;

		case 0x07:		llz80_rlca(z80);	break;	// rlca
		case 0x0f:		llz80_rrca(z80);	break;	// rrca
		case 0x17:		llz80_rla(z80);		break;	// rla
		case 0x1f:		llz80_rra(z80);		break;	// rrca

		case 0x08:		// ex af, af'
		{
			uint8_t temporaryStore;

			temporaryStore = z80->aRegister;
			z80->aRegister = z80->aDashRegister;
			z80->aDashRegister = temporaryStore;

			temporaryStore = llz80_getF(z80);
			llz80_setF(z80, z80->fDashRegister);
			z80->fDashRegister = temporaryStore;
		}
		break;

		case 0xd9:		// exx
		{
			LLZ80RegisterPair temporaryStore;

			temporaryStore = z80->bcRegister;
			z80->bcRegister = z80->bcDashRegister;
			z80->bcDashRegister = temporaryStore;

			temporaryStore = z80->deRegister;
			z80->deRegister = z80->deDashRegister;
			z80->deDashRegister = temporaryStore;

			temporaryStore = z80->hlRegister;
			z80->hlRegister = z80->hlDashRegister;
			z80->hlDashRegister = temporaryStore;
		}
		break;
		
		case 0xeb:		// ex de, hl	(always hl, not an index register)
		{
			LLZ80RegisterPair temporaryStore;

			temporaryStore = z80->deRegister;
			z80->deRegister = z80->hlRegister;
			z80->hlRegister = temporaryStore;
		}
		break;
		
		case 0xe3:		// ex (sp), [index pointer]
		{
			//	timing is (4,) 3, 4, 3, 5

			// pop a 16 bit value, into the temporary address
			// register
			llz80_schedulePop(z80, &z80->temporaryAddress);
			llz80_schedulePush(z80, indexRegister);
			llz80_scheduleFunction(z80, llz80_copyTemporaryAddressToRegister)->extraData.referenceToIndexRegister.indexRegister = indexRegister;

			// that swap should have cost us a couple of cycles, so...
			llz80_schedulePauseForCycles(z80, 2);
		}
		break;

		case 0x20:
		case 0x28:
		case 0x30:
		case 0x38:	// jr ss, e
		{
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_jrConditional)->extraData.conditional.condition = (opcode >> 3)&3;
		}
		break;

		case 0x18:	// unconditional jr (which we'll pretend is conditional, attached to our 'yes' condition)
		{
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_jrConditional)->extraData.conditional.condition = LLZ80ConditionYES;
		}
		break;

		case 0xc2:	// jp cc, nn
		case 0xca:
		case 0xd2:
		case 0xda:
		case 0xe2:
		case 0xea:
		case 0xf2:
		case 0xfa:
		{
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);
			if(llz80_conditionIsTrue(z80, (opcode >> 3)&7))
			{
				llz80_scheduleFunction(z80, llz80_iop_setPCToTemporaryAddress);
			}
		}
		break;

		case 0xc3:	// jp nn
		{
			// read to a temporary place,
			// then copy over to the PC
			llz80_schedule16BitReadFromPC(z80, &z80->temporaryAddress);
			llz80_scheduleFunction(z80, llz80_iop_setPCToTemporaryAddress);
		}
		break;

		case 0x09:
		case 0x19:
		case 0x29:
		case 0x39:	// add [current index register], ss
		{
			llz80_schedulePauseForCycles(z80, 7);
			llz80_add_16bit(z80, indexRegister, &rpTable[opcode >> 4]->fullValue);
		}
		break;

		case 0x04: case 0x05:
		case 0x0c: case 0x0d:
		case 0x14: case 0x15:
		case 0x1c: case 0x1d:
		case 0x24: case 0x25:
		case 0x2c: case 0x2d:
		case 0x34: case 0x35:
		case 0x3c: case 0x3d:	// inc r and dec r
		{
			uint8_t *source = rTable[opcode >> 3];

			if(source)
			{
				if(opcode&1)
					llz80_decrement_8bit(z80, source);
				else
					llz80_increment_8bit(z80, source);
			}
			else
			{
				llz80_scheduleCalculationOfSourceAddress(z80, indexRegister, addOffset);

				// + 3
				llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);

				// + 1
				llz80_schedulePauseForCycles(z80, 1);
				llz80_scheduleFunction(z80,
						(opcode&1) ? llz80_decrementTemporary8BitValue : llz80_incrementTemporary8BitValue);

				// + 3
				llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
			}
		}
		break;

		// ld r, r
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
		{
			uint8_t *source = rTable[opcode&7];
			uint8_t *destination = rTable[(opcode >> 3)&7];

			// if source and destination are registers, just do
			// the copy
			if(source && destination)
			{
				*destination = *source;
				return;
			}

			// if we have neither a source nor a destination then
			// this is the halt opcode rather than a load
			if(!source && !destination)
			{
				// then this is a halt really
				z80->interruptState = LLZ80InterruptStateHalted;
				llz80_setLinesActive(z80, LLZ80SignalHalt);
				return;
			}

			// okay, it's indirect via an index register, so we'll
			// need to work out what address that leaves us pointing
			// to
			llz80_scheduleCalculationOfSourceAddress(z80, indexRegister, addOffset);

			// NB: if an offset and a memory read/write is involved,
			// the named register is always one of H or L, not
			// half of one of the index registers
			uint8_t *rHLTable[] =
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

			if(!source)
			{
				// this is a load then...
				llz80_scheduleRead(z80, rHLTable[(opcode >> 3)&7], &z80->temporaryAddress.fullValue);
			}
			else
			{
				// a write it is then
				llz80_scheduleWrite(z80, rHLTable[opcode&7], &z80->temporaryAddress.fullValue);
			}
		}
		break;
		
		case 0xfb:	// ei and di
		case 0xf3:
			z80->iff1 = z80->iff2 = !!(opcode&8);
		break;

		case 0xc6:	// ALU operations that don't fit in the group below, because they
		case 0xce:	// take an immediate operand
		case 0xd6:
		case 0xde:
		case 0xe6:
		case 0xee:
		case 0xf6:
		case 0xfe:
		{
			int operation = (opcode >> 3)&7;

			llz80_schedule8BitReadFromPC(z80, &z80->temporary8bitValue);
			llz80_scheduleFunction(z80, llz80_doALUOp)->extraData.ALUOrShiftOp.operation = operation;
		}
		break;

		// [most of] the ALU operations, en masse
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		{
			uint8_t *source = rTable[opcode&7];
			int operation = (opcode >> 3)&7;

			if(source)
			{
				switch(operation)
				{
					default: break;
					case 0:	llz80_add_8bit(z80, *source);				break;
					case 1:	llz80_addWithCarry_8bit(z80, *source);		break;
					case 2:	llz80_subtract_8bit(z80, *source);			break;
					case 3:	llz80_subtractWithCarry_8bit(z80, *source);	break;
					case 4:	llz80_bitwiseAnd(z80, *source);				break;
					case 5:	llz80_bitwiseXOr(z80, *source);				break;
					case 6:	llz80_bitwiseOr(z80, *source);				break;
					case 7:	llz80_compare(z80, *source);				break;
				}
			}
			else
			{
				llz80_scheduleCalculationOfSourceAddress(z80, indexRegister, addOffset);

				llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->temporaryAddress.fullValue);
				llz80_scheduleFunction(z80, llz80_doALUOp)->extraData.ALUOrShiftOp.operation = operation;
			}
		}
		break;

		case 0xd3:	// out (n), a
		{
			z80->temporaryAddress.bytes.high = z80->aRegister;
			llz80_schedule8BitReadFromPC(z80, &z80->temporaryAddress.bytes.low);

			llz80_scheduleOutput(z80, &z80->aRegister, &z80->temporaryAddress.fullValue);
		}
		break;
		
		case 0xdb:	// in a, (n)
		{
			// this doesn't set any flags, in contrast to
			// the various ins on the ed page
			z80->temporaryAddress.bytes.high = z80->aRegister;
			llz80_schedule8BitReadFromPC(z80, &z80->temporaryAddress.bytes.low);

			llz80_scheduleInput(z80, &z80->aRegister, &z80->temporaryAddress.fullValue);
		}
		break;
		
		case 0xf9:	// ld sp, indexRegister
		{
			z80->spRegister.fullValue = indexRegister->fullValue;
			llz80_schedulePauseForCycles(z80, 2);
		}
		break;
	}
}

LLZ80InternalInstructionFunction llz80_iop_standardPageDecode = llz80_iop_standardPageDecode_imp;
