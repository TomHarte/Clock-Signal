//
//  SetResetTestOps.c
//  LLZ80
//
//  Created by Thomas Harte on 15/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include "Z80SetResetTestOps.h"

LLZ80iop(llz80_iop_bit_imp)
{
	uint8_t result = instruction->extraData.bitOp.mask & *instruction->extraData.bitOp.value;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = result;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagCarry) |
		LLZ80FlagHalfCarry |
		(result ? 0 : LLZ80FlagParityOverflow);
}

LLZ80iop(llz80_iop_set_imp)
{
	*instruction->extraData.bitOp.value |= instruction->extraData.bitOp.mask;
}

LLZ80iop(llz80_iop_res_imp)
{
	*instruction->extraData.bitOp.value &= ~instruction->extraData.bitOp.mask;
}

const LLZ80InternalInstructionFunction llz80_iop_bit = llz80_iop_bit_imp;
const LLZ80InternalInstructionFunction llz80_iop_set = llz80_iop_set_imp;
const LLZ80InternalInstructionFunction llz80_iop_res = llz80_iop_res_imp;
