//
//  RotateAndShift.c
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80RotateAndShiftOps.h"

void llz80_rla(LLZ80ProcessorState *z80)
{
	uint8_t newCarry = z80->aRegister >> 7;
	z80->aRegister = (uint8_t)((z80->aRegister << 1) | (z80->generalFlags&LLZ80FlagCarry));
	z80->bit5And3Flags = z80->aRegister;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagParityOverflow) |
		newCarry;
}

void llz80_rlca(LLZ80ProcessorState *z80)
{
	uint8_t newCarry = z80->aRegister >> 7;
	z80->aRegister = (uint8_t)((z80->aRegister << 1) | newCarry);
	z80->bit5And3Flags = z80->aRegister;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagParityOverflow) |
		newCarry;
}

void llz80_rra(LLZ80ProcessorState *z80)
{
	uint8_t newCarry = z80->aRegister & 1;
	z80->aRegister = (uint8_t)((z80->aRegister >> 1) | ((z80->generalFlags&LLZ80FlagCarry) << 7));
	z80->bit5And3Flags = z80->aRegister;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagParityOverflow) |
		newCarry;
}

void llz80_rrca(LLZ80ProcessorState *z80)
{
	uint8_t newCarry = z80->aRegister & 1;
	z80->aRegister = (uint8_t)((z80->aRegister >> 1) | (newCarry << 7));
	z80->bit5And3Flags = z80->aRegister;
	z80->generalFlags =
		(z80->generalFlags & LLZ80FlagParityOverflow) |
		newCarry;
}

void llz80_rlc(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value >> 7;
	*value = (uint8_t)((*value << 1) | carry);

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_rrc(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value & 1;
	*value = (uint8_t)((*value >> 1) | (carry << 7));

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_rl(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value >> 7;
	*value = (uint8_t)((*value << 1) | (z80->generalFlags&LLZ80FlagCarry));

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_rr(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value & 1;
	*value = (uint8_t)((*value >> 1) | (z80->generalFlags << 7));

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_sla(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value >> 7;
	*value <<= 1;

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_sra(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value & 1;
	*value = (*value & 0x80) | (*value >> 1);

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_sll(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value >> 7;
	*value = (uint8_t)((*value << 1) | 1);

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}

void llz80_srl(LLZ80ProcessorState *z80, uint8_t *value)
{
	uint8_t carry = *value & 1;
	*value >>= 1;

	llz80_calculateParity(*value);
	z80->generalFlags = carry | parity;
	z80->bit5And3Flags = z80->lastSignResult = z80->lastZeroResult = *value;
}
