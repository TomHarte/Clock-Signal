//
//  BitwiseOps.c
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#include "Z80BitwiseOps.h"

void llz80_bitwiseAnd(LLZ80ProcessorState *z80, uint8_t value)
{
	z80->aRegister &= value;

	z80->lastSignResult = z80->lastZeroResult = 
	z80->bit5And3Flags = z80->aRegister;

	llz80_calculateParity(z80->aRegister);

	z80->generalFlags =
		LLZ80FlagHalfCarry |
		parity;
}

void llz80_bitwiseOr(LLZ80ProcessorState *z80, uint8_t value)
{
	z80->aRegister |= value;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = z80->aRegister;

	llz80_calculateParity(z80->aRegister);
	z80->generalFlags = parity;
}

void llz80_bitwiseXOr(LLZ80ProcessorState *z80, uint8_t value)
{
	z80->aRegister ^= value;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = z80->aRegister;

	llz80_calculateParity(z80->aRegister);
	z80->generalFlags = parity;
}
