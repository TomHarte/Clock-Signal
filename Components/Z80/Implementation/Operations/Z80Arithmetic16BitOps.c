//
//  Arithmetic16BitOps.c
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80Arithmetic16BitOps.h"

void llz80_subtractWithCarry_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand)
{
	int result = source->fullValue - *operand - (z80->generalFlags&LLZ80FlagCarry);
	int halfResult = (source->fullValue&0xfff) - (*operand&0xfff) - (z80->generalFlags&LLZ80FlagCarry);

	// subtraction, so parity rules are:
	// signs of operands were different, 
	// sign of result is different
	int overflow = (result ^ source->fullValue) & (*operand ^ source->fullValue);

	source->fullValue = (uint16_t)result;

	z80->bit5And3Flags = z80->lastSignResult = (uint8_t)(result >> 8);
	z80->lastZeroResult	= (uint8_t)(result | z80->lastSignResult);
	z80->generalFlags = 
		LLZ80FlagSubtraction					|
		((result >> 16)&LLZ80FlagCarry)			|
		((halfResult >> 8)&LLZ80FlagHalfCarry)	|
		((overflow&0x8000) >> 13);
}

void llz80_add_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand)
{
	int result = source->fullValue + *operand;
	int halfResult = (source->fullValue&0xfff) + (*operand&0xfff);

	z80->bit5And3Flags = (uint8_t)(result >> 8);
	z80->generalFlags =
		(z80->generalFlags&LLZ80FlagParityOverflow)	|
		((result >> 16)&LLZ80FlagCarry)			|
		((halfResult >> 8)&LLZ80FlagHalfCarry);	// implicitly, subtract isn't set

	source->fullValue = (uint16_t)result;
}

void llz80_addWithCarry_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand)
{
	int result = source->fullValue + *operand + (z80->generalFlags&LLZ80FlagCarry);
	int halfResult = (source->fullValue&0xfff) + (*operand&0xfff) + (z80->generalFlags&LLZ80FlagCarry);

	int overflow = (result ^ source->fullValue) & ~(*operand ^ source->fullValue);

	z80->bit5And3Flags = z80->lastSignResult = (uint8_t)(result >> 8);
	z80->lastZeroResult	= (uint8_t)(result | z80->lastSignResult);
	z80->generalFlags =
		((result >> 16)&LLZ80FlagCarry)			|
		((halfResult >> 8)&LLZ80FlagHalfCarry) |
		((overflow&0x8000) >> 13);	// implicitly, subtract isn't set

	source->fullValue = (uint16_t)result;
}
