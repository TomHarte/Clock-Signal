//
//  EDPageDecode.c
//  LLZ80
//
//  Created by Thomas Harte on 13/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include "Z80EDPageDecode.h"

#include "../Scheduling Building Blocks/Z80ScheduleInputOrOutput.h"
#include "../Scheduling Building Blocks/Z80StandardSchedulingComponents.h"
#include "../Scheduling Building Blocks/Z80ScheduleReadOrWrite.h"

#include "../Operations/Z80Arithmetic16BitOps.h"

void llz80_iop_EDPageDecode_imp	(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

void llz80_iop_setInputFlags	(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_doRRD			(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_doRLD			(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

void llz80_iop_finishLDI		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishLDIR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishLDD		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishLDDR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

void llz80_iop_finishCPI		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishCPIR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishCPD		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishCPDR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

void llz80_iop_finishINI		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishINIR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishIND		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishINDR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

void llz80_iop_finishOUTI		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishOUTIR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishOUTD		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);
void llz80_iop_finishOUTDR		(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction);

static LLZ80InternalInstructionFunction ldInstructions[4] =
{
	llz80_iop_finishLDI,
	llz80_iop_finishLDD,
	llz80_iop_finishLDIR,
	llz80_iop_finishLDDR
};

static LLZ80InternalInstructionFunction cpInstructions[4] =
{
	llz80_iop_finishCPI,
	llz80_iop_finishCPD,
	llz80_iop_finishCPIR,
	llz80_iop_finishCPDR
};

static LLZ80InternalInstructionFunction inInstructions[4] =
{
	llz80_iop_finishINI,
	llz80_iop_finishIND,
	llz80_iop_finishINIR,
	llz80_iop_finishINDR
};

static LLZ80InternalInstructionFunction outInstructions[4] =
{
	llz80_iop_finishOUTI,
	llz80_iop_finishOUTD,
	llz80_iop_finishOUTIR,
	llz80_iop_finishOUTDR
};

void llz80_iop_EDPageDecode_imp(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	// the ed page isn't affected by an fd or dd prefix,
	// so this is all always HL

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

	LLZ80RegisterPair *rpTable[] =
	{
		&z80->bcRegister,
		&z80->deRegister,
		&z80->hlRegister,
		&z80->spRegister
	};

	uint8_t opcode = z80->temporary8bitValue;
	switch(opcode)
	{
		default:
			// everything not defined below is a two-byte nop, essentially
		break;

		// im 0, 1 and 2
		case 0x4e:	case 0x66:	case 0x6e:	case 0x46:
								z80->interruptMode = 0;	break;
		case 0x76:	case 0x56:	z80->interruptMode = 1;	break;
		case 0x7e:	case 0x5e:	z80->interruptMode = 2;	break;
		
		case 0x40:	case 0x48:	case 0x50:	case 0x58:	// in r, (c)
		case 0x60:	case 0x68:	case 0x70:	case 0x78:
		{
			uint8_t *destination = rTable[(opcode >> 3)&7];
			if(!destination) destination = &z80->temporary8bitValue;

			llz80_scheduleInput(z80, destination, &z80->bcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_setInputFlags);
		}
		break;
		
		case 0x41:	case 0x49:	case 0x51:	case 0x59:	// out (c), r
		case 0x61:	case 0x69:	case 0x71:	case 0x79:
		{
			uint8_t *source = rTable[(opcode >> 3)&7];
			if(!source)
			{
				// what would otherwise be out (c), (hl) is
				// actually out (c), 0
				source = &z80->temporary8bitValue;
				z80->temporary8bitValue = 0;
			}

			llz80_scheduleOutput(z80, source, &z80->bcRegister.fullValue);
		}
		break;

		case 0x42:	case 0x52:	// sbc hl, rr
		case 0x62:	case 0x72:
		{
			llz80_schedulePauseForCycles(z80, 7);
			llz80_subtractWithCarry_16bit(z80, &z80->hlRegister, &rpTable[(opcode >> 4)&3]->fullValue);
		}
		break;

		case 0x4a:	case 0x5a:	// adc hl, rr
		case 0x6a:	case 0x7a:
		{
			llz80_schedulePauseForCycles(z80, 7);
			llz80_addWithCarry_16bit(z80, &z80->hlRegister, &rpTable[(opcode >> 4)&3]->fullValue);
		}
		break;

		case 0x57:	// ld a, i
		case 0x5f:	// ld a, r
		{
			llz80_schedulePauseForCycles(z80, 1);

			z80->aRegister = (opcode == 0x57) ? z80->iRegister : z80->rRegister;
			z80->lastSignResult = z80->lastZeroResult = z80->bit5And3Flags = z80->aRegister;
			z80->generalFlags = 
				(z80->generalFlags&LLZ80FlagCarry) |
				(z80->iff2 ? LLZ80FlagParityOverflow : 0);
		}
		break;

		case 0x47:	// ld i, a
			llz80_schedulePauseForCycles(z80, 1);
			z80->iRegister = z80->aRegister;
		break;

		case 0x4f:	// ld r, a
			llz80_schedulePauseForCycles(z80, 1);
			z80->rRegister = z80->aRegister;
		break;

		case 0x43:	case 0x53: // ld (nn), rr
		case 0x63:	case 0x73:
		{
			// load target address
			llz80_scheduleRead(z80, &z80->temporaryAddress.bytes.low, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementProgramCounter);

			llz80_scheduleRead(z80, &z80->temporaryAddress.bytes.high, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementProgramCounter);

			// store register
			LLZ80RegisterPair *source = rpTable[(opcode >> 4)&3];
			llz80_scheduleWrite(z80, &source->bytes.low, &z80->temporaryAddress.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementTemporaryAddress);

			llz80_scheduleWrite(z80, &source->bytes.high, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x4b:	case 0x5b: // ld rr, (nn)
		case 0x6b:	case 0x7b:
		{
			// load target address
			llz80_scheduleRead(z80, &z80->temporaryAddress.bytes.low, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementProgramCounter);

			llz80_scheduleRead(z80, &z80->temporaryAddress.bytes.high, &z80->pcRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementProgramCounter);

			// load register
			LLZ80RegisterPair *source = rpTable[(opcode >> 4)&3];
			llz80_scheduleRead(z80, &source->bytes.low, &z80->temporaryAddress.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementTemporaryAddress);

			llz80_scheduleRead(z80, &source->bytes.high, &z80->temporaryAddress.fullValue);
		}
		break;

		case 0x44:	case 0x4c:	case 0x54:	case 0x5c: // neg
		case 0x64:	case 0x6c:	case 0x74:	case 0x7c:
		{
			// -128 is the only thing that'll overflow
			// when negated
			int overflow = (z80->aRegister == 0x80);
			int result = 0 - z80->aRegister;
			int halfResult = 0 - (z80->aRegister&0xf);

			z80->aRegister = (uint8_t)result;
			z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = z80->aRegister;
			z80->generalFlags =
				(overflow ? LLZ80FlagParityOverflow : 0) |
				LLZ80FlagSubtraction |
				((result >> 8)&LLZ80FlagCarry) |
				((halfResult << 4)&LLZ80FlagHalfCarry);
		}
		break;
		
		case 0x4d:	// reti
		case 0x55:	// retn
		case 0x5d:
		case 0x65:
		case 0x6d:
		case 0x75:
		case 0x7d:
		case 0x45:
			z80->iff1 = z80->iff2;

			llz80_scheduleRead(z80, &z80->pcRegister.bytes.low, &z80->spRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementStackPointer);

			llz80_scheduleRead(z80, &z80->pcRegister.bytes.high, &z80->spRegister.fullValue);
			llz80_scheduleFunction(z80, llz80_iop_incrementStackPointer);
		break;

		case 0x67:	// rrd
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
			llz80_schedulePauseForCycles(z80, 3);
			llz80_beginNewHalfCycle(z80);
			llz80_scheduleHalfCycleForFunction(z80, llz80_iop_doRRD);
			llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
		break;

		case 0x6f:	// rld
			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
			llz80_schedulePauseForCycles(z80, 3);
			llz80_beginNewHalfCycle(z80);
			llz80_scheduleHalfCycleForFunction(z80, llz80_iop_doRLD);
			llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
		break;
		
		/*

			To implement: in,out block instructions

		*/
		case 0xa0:	// ldi
		case 0xa8:	// ldd
		case 0xb0:	// ldir
		case 0xb8:	// lddr
			// 3 cycles: read from (hl)
			// 5 cycles: write to (de), dec bc

			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);

			llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->deRegister.fullValue);
			llz80_scheduleFunction(z80, ldInstructions[(opcode >> 3)&3]);

			llz80_schedulePauseForCycles(z80, 2);
		break;

		case 0xa1:	// cpi
		case 0xa9:	// cpd
		case 0xb1:	// cpir
		case 0xb9:	// cpdr
			// 3 cycles: read from (hl)
			// 5 cycles: compare, etc

			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);

			llz80_schedulePauseForCycles(z80, 3);
			llz80_scheduleFunction(z80, cpInstructions[(opcode >> 3)&3]);

			llz80_schedulePauseForCycles(z80, 2);
		break;

		case 0xa2:	// ini
		case 0xaa:	// ind
		case 0xb2:	// inir
		case 0xba:	// indr

			// 4 cycles: input from (c)
			// 3 cycles: write out to (hl)
			// 1 cycle: think about life

			llz80_scheduleInput(z80, &z80->temporary8bitValue, &z80->bcRegister.fullValue);
			llz80_scheduleWrite(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
			llz80_schedulePauseForCycles(z80, 1);
			llz80_scheduleFunction(z80, inInstructions[(opcode >> 3)&3]);
		break;

		case 0xa3:	// outi
		case 0xab:	// outd
		case 0xb3:	// outir
		case 0xbb:	// outdr

			// 3 cycles: read from (hl)
			// 4 cycles: output to (c)
			// 1 cycle: think about life

			llz80_scheduleRead(z80, &z80->temporary8bitValue, &z80->hlRegister.fullValue);
			llz80_scheduleOutput(z80, &z80->temporary8bitValue, &z80->bcRegister.fullValue);
			llz80_schedulePauseForCycles(z80, 1);
			llz80_scheduleFunction(z80, outInstructions[(opcode >> 3)&3]);
		break;
	}
}

void llz80_setLDFlags(LLZ80ProcessorState *z80);

void llz80_setLDFlags(LLZ80ProcessorState *z80)
{
	uint8_t n = z80->aRegister + z80->temporary8bitValue;
	
	z80->generalFlags =
		(z80->generalFlags&LLZ80FlagCarry) |
		(z80->bcRegister.fullValue ? LLZ80FlagParityOverflow : 0);
	z80->bit5And3Flags = (uint8_t)((n&0x8) | ((n&0x2) << 4));
}

void llz80_iop_finishLDI(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->deRegister.fullValue++;
	z80->hlRegister.fullValue++;

	llz80_setLDFlags(z80);
}

void llz80_iop_finishLDIR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->deRegister.fullValue++;
	z80->hlRegister.fullValue++;

	llz80_setLDFlags(z80);

	if(z80->bcRegister.fullValue)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_iop_finishLDD(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->deRegister.fullValue--;
	z80->hlRegister.fullValue--;

	llz80_setLDFlags(z80);
}

void llz80_iop_finishLDDR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->deRegister.fullValue--;
	z80->hlRegister.fullValue--;

	llz80_setLDFlags(z80);
	if(z80->bcRegister.fullValue)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_setCPFlags(LLZ80ProcessorState *z80);

void llz80_setCPFlags(LLZ80ProcessorState *z80)
{
	uint8_t result = z80->aRegister - z80->temporary8bitValue;
	uint8_t halfResult = (z80->aRegister&0xf) - (z80->temporary8bitValue&0xf);

	// sign, zero, half-carry: set by compare of (hl) and a
	// yf, xf: copies of bits 1 and 3 of n
	// parity: set if bc is not 0
	// subtract: set

	z80->generalFlags =
		(z80->generalFlags&LLZ80FlagCarry) |
		(z80->bcRegister.fullValue ? LLZ80FlagParityOverflow : 0) |
		(halfResult & LLZ80FlagHalfCarry) |
		LLZ80FlagSubtraction;
	z80->bit5And3Flags = (uint8_t)((result&0x8) | ((result&0x2) << 4));
	z80->lastSignResult = z80->lastZeroResult = result;
}

void llz80_iop_finishCPI(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->hlRegister.fullValue++;

	llz80_setCPFlags(z80);
}

void llz80_iop_finishCPIR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->hlRegister.fullValue++;

	llz80_setCPFlags(z80);

	if(z80->bcRegister.fullValue && z80->lastZeroResult)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_iop_finishCPD(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->hlRegister.fullValue--;

	llz80_setCPFlags(z80);
}

void llz80_iop_finishCPDR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->bcRegister.fullValue--;
	z80->hlRegister.fullValue--;

	llz80_setCPFlags(z80);
	if(z80->bcRegister.fullValue && z80->lastZeroResult)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_setINFlags(LLZ80ProcessorState *z80, int cAdder);

void llz80_setINFlags(LLZ80ProcessorState *z80, int cAdder)
{
	z80->bcRegister.bytes.high--;
	
	// sign, zero, 5 and 3 are set per the result of the decrement of B
	z80->lastSignResult = z80->lastZeroResult = z80->bit5And3Flags = z80->bcRegister.bytes.high;
	
	z80->generalFlags = ((z80->temporary8bitValue&0x80) >> 6);		// subtract flag is copied from bit 8 of the incoming value

	int summation = z80->temporary8bitValue + ((cAdder + z80->bcRegister.bytes.low)&0xff);
	if(summation > 0xff) z80->generalFlags |= LLZ80FlagHalfCarry | LLZ80FlagCarry;

	summation = (summation&7) ^ z80->bcRegister.bytes.high;
	llz80_calculateParity((uint8_t)summation);
	z80->generalFlags |= parity;
}

void llz80_iop_finishINI(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue++;
	llz80_setINFlags(z80, 1);
}

void llz80_iop_finishINIR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue++;
	llz80_setINFlags(z80, 1);

	if(z80->bcRegister.bytes.high)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_iop_finishIND(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue--;
	llz80_setINFlags(z80, -1);
}

void llz80_iop_finishINDR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue--;
	llz80_setINFlags(z80, -1);

	if(z80->bcRegister.bytes.high)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_setOUTFlags(LLZ80ProcessorState *z80);

void llz80_setOUTFlags(LLZ80ProcessorState *z80)
{
	z80->bcRegister.bytes.high--;
	
	// sign, zero, 5 and 3 are set per the result of the decrement of B
	z80->lastSignResult = z80->lastZeroResult = z80->bit5And3Flags = z80->bcRegister.bytes.high;
	
	z80->generalFlags = ((z80->temporary8bitValue&0x80) >> 6);		// subtract flag is copied from bit 8 of the incoming value

	int summation = z80->temporary8bitValue + z80->hlRegister.bytes.low;
	if(summation > 0xff) z80->generalFlags |= LLZ80FlagHalfCarry | LLZ80FlagCarry;

	summation = (summation&7) ^ z80->bcRegister.bytes.high;
	llz80_calculateParity((uint8_t)summation);
	z80->generalFlags |= parity;
}

void llz80_iop_finishOUTI(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue++;
	llz80_setOUTFlags(z80);
}

void llz80_iop_finishOUTIR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue++;
	llz80_setOUTFlags(z80);

	if(z80->bcRegister.bytes.high)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_iop_finishOUTD(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue--;
	llz80_setOUTFlags(z80);
}

void llz80_iop_finishOUTDR(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	z80->hlRegister.fullValue--;
	llz80_setOUTFlags(z80);

	if(z80->bcRegister.bytes.high)
	{
		z80->pcRegister.fullValue -= 2;
		llz80_schedulePauseForCycles(z80, 5);
	}
}

void llz80_iop_doRRD(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	int lowNibble = z80->aRegister&0xf;
	z80->aRegister = (z80->aRegister&0xf0) | (z80->temporary8bitValue & 0xf);
	z80->temporary8bitValue = (uint8_t)((z80->temporary8bitValue >> 4) | (lowNibble << 4));

	llz80_calculateParity(z80->aRegister);
	z80->generalFlags =
		parity |
		(z80->generalFlags&LLZ80FlagCarry);
	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = z80->aRegister;
}

void llz80_iop_doRLD(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	int lowNibble = z80->aRegister&0xf;
	z80->aRegister = (z80->aRegister&0xf0) | (z80->temporary8bitValue >> 4);
	z80->temporary8bitValue = (uint8_t)((z80->temporary8bitValue << 4) | lowNibble);

	llz80_calculateParity(z80->aRegister);
	z80->generalFlags =
		parity |
		(z80->generalFlags&LLZ80FlagCarry);
	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = z80->aRegister;
}

void llz80_iop_setInputFlags(LLZ80ProcessorState *z80, LLZ80InternalInstruction *instruction)
{
	// the input will still be on the data lines, so...
	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = llz80_getDataInput(z80);

	int parity = z80->bit5And3Flags;
	parity ^= parity >> 4;
	parity ^= parity >> 2;
	parity ^= parity >> 1;

	z80->generalFlags =
		(uint8_t)(
			(z80->generalFlags & LLZ80FlagCarry) |
			((parity&1) << 2));
}

LLZ80InternalInstructionFunction llz80_iop_EDPageDecode = llz80_iop_EDPageDecode_imp;
