//
//  Internals.c
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80Internals.h"
#include <stdio.h>

bool llz80_conditionIsTrue(const LLZ80ProcessorState *const z80, LLZ80Condition condition)
{
	switch(condition)
	{
		case LLZ80ConditionNZ:	return !!z80->lastZeroResult;
		case LLZ80ConditionZ:	return !z80->lastZeroResult;
		case LLZ80ConditionNC:	return !(z80->generalFlags & LLZ80FlagCarry);
		case LLZ80ConditionC:	return !!(z80->generalFlags & LLZ80FlagCarry);
		case LLZ80ConditionPO:	return !(z80->generalFlags & LLZ80FlagParityOverflow);
		case LLZ80ConditionPE:	return !!(z80->generalFlags & LLZ80FlagParityOverflow);
		case LLZ80ConditionP:	return !(z80->lastSignResult & 0x80);
		case LLZ80ConditionM:	return !!(z80->lastSignResult & 0x80);

		case LLZ80ConditionYES:	return true;
	}
}

uint8_t llz80_getF(const LLZ80ProcessorState *const z80)
{
	return 
		(z80->bit5And3Flags & (LLZ80FlagBit3 | LLZ80FlagBit5)) |
		(z80->generalFlags & (LLZ80FlagCarry | LLZ80FlagHalfCarry | LLZ80FlagParityOverflow | LLZ80FlagSubtraction)) |
		(z80->lastSignResult & LLZ80FlagSign) |
		(z80->lastZeroResult ? 0 : LLZ80FlagZero);
}

void llz80_setF(LLZ80ProcessorState *const z80, uint8_t value)
{
	z80->bit5And3Flags = value & (LLZ80FlagBit3 | LLZ80FlagBit5);
	z80->generalFlags = value & (LLZ80FlagCarry | LLZ80FlagHalfCarry | LLZ80FlagParityOverflow | LLZ80FlagSubtraction);
	z80->lastSignResult = value & LLZ80FlagSign;
	z80->lastZeroResult = !(value & LLZ80FlagZero);
}
