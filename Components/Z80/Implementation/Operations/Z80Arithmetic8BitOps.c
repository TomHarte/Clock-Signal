//
//  Arithmetic8BitOps.c
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80Arithmetic8BitOps.h"

void llz80_compare(LLZ80ProcessorState *z80, uint8_t value)
{
	int result = z80->aRegister - value;
	int halfResult = (z80->aRegister&0xf) - (value&0xf);

	// overflow for a subtraction is when the signs were originally
	// different and the result is different again
	int overflow = (value^z80->aRegister) & (result^z80->aRegister);

	z80->lastSignResult =			// set sign and zero
	z80->lastZeroResult = result;
	z80->bit5And3Flags = value;		// set the 5 and 3 flags, which come
									// from the operand atypically
	z80->generalFlags =
		((result >> 8) & LLZ80FlagCarry)	|		// carry flag
		(halfResult & LLZ80FlagHalfCarry)	|		// half carry flag
		((overflow&0x80) >> 5)				|		// overflow flag
		LLZ80FlagSubtraction;						// and this counts as a subtraction
}

void llz80_subtract_8bit(LLZ80ProcessorState *z80, uint8_t value)
{
	int result = z80->aRegister - value;
	int halfResult = (z80->aRegister&0xf) - (value&0xf);

	// overflow for a subtraction is when the signs were originally
	// different and the result is different again
	int overflow = (value^z80->aRegister) & (result^z80->aRegister);

	z80->aRegister = result;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = result;			// set sign, zero and 5 and 3
	z80->generalFlags =
		((result >> 8) & LLZ80FlagCarry)	|		// carry flag
		(halfResult & LLZ80FlagHalfCarry)	|		// half carry flag
		((overflow&0x80) >> 5)				|		// overflow flag
		LLZ80FlagSubtraction;						// and this counts as a subtraction
}

void llz80_subtractWithCarry_8bit(LLZ80ProcessorState *z80, uint8_t value)
{
	int result = z80->aRegister - value - (z80->generalFlags&LLZ80FlagCarry);
	int halfResult = (z80->aRegister&0xf) - (value&0xf) - (z80->generalFlags&LLZ80FlagCarry);;

	// overflow for a subtraction is when the signs were originally
	// different and the result is different again
	int overflow = (value^z80->aRegister) & (result^z80->aRegister);

	z80->aRegister = result;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = result;			// set sign, zero and 5 and 3
	z80->generalFlags =
		((result >> 8) & LLZ80FlagCarry)	|		// carry flag
		(halfResult & LLZ80FlagHalfCarry)	|		// half carry flag
		((overflow&0x80) >> 5)				|		// overflow flag
		LLZ80FlagSubtraction;						// and this counts as a subtraction
}

void llz80_add_8bit(LLZ80ProcessorState *z80, uint8_t value)
{
	int result = z80->aRegister + value;
	int halfResult = (z80->aRegister&0xf) + (value&0xf);

	// overflow for addition is when the signs were originally
	// the same and the result is different
	int overflow = ~(value^z80->aRegister) & (result^z80->aRegister);

	z80->aRegister = result;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = result;			// set sign, zero and 5 and 3
	z80->generalFlags =
		((result >> 8) & LLZ80FlagCarry)	|		// carry flag
		(halfResult & LLZ80FlagHalfCarry)	|		// half carry flag
		((overflow&0x80) >> 5);						// overflow flag
													// subtraction is implicitly unset
}

void llz80_addWithCarry_8bit(LLZ80ProcessorState *z80, uint8_t value)
{
	int result = z80->aRegister + value + (z80->generalFlags&LLZ80FlagCarry);
	int halfResult = (z80->aRegister&0xf) + (value&0xf) + (z80->generalFlags&LLZ80FlagCarry);

	// overflow for addition is when the signs were originally
	// the same and the result is different
	int overflow = ~(value^z80->aRegister) & (result^z80->aRegister);

	z80->aRegister = result;

	z80->lastSignResult = z80->lastZeroResult =
	z80->bit5And3Flags = result;			// set sign, zero and 5 and 3
	z80->generalFlags =
		((result >> 8) & LLZ80FlagCarry)	|		// carry flag
		(halfResult & LLZ80FlagHalfCarry)	|		// half carry flag
		((overflow&0x80) >> 5);						// overflow flag
													// subtraction is implicitly unset
}

void llz80_increment_8bit(LLZ80ProcessorState *z80, uint8_t *value)
{
	int result = (*value) + 1;
	
	// with an increment, overflow occurs if the sign changes from
	// positive to negative
	int overflow = (*value ^ result) & ~(*value);
	int halfResult = (*value&0xf) + 1;

	*value = result;

	// sign, zero and 5 & 3 are set directly from the result
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = result;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagCarry) |		// carry isn't affected
		(halfResult&LLZ80FlagHalfCarry) |			// half carry
		((overflow >> 5)&LLZ80FlagParityOverflow);	// overflow
													// implicitly: subtraction is reset
}

void llz80_decrement_8bit(LLZ80ProcessorState *z80, uint8_t *value)
{
	int result = (*value) - 1;

	// with a decrement, overflow occurs if the sign changes from
	// negative to positive
	int overflow = (*value ^ result) & (*value);
	int halfResult = (*value&0xf) - 1;

	*value = result;

	// sign, zero and 5 & 3 are set directly from the result
	z80->bit5And3Flags = z80->lastZeroResult = z80->lastSignResult = result;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagCarry) |		// carry isn't affected
		(halfResult&LLZ80FlagHalfCarry) |			// half carry
		((overflow >> 5)&LLZ80FlagParityOverflow) |	// overflow
		LLZ80FlagSubtraction;						// subtraction is set
}
