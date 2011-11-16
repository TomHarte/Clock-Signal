//
//  Arithmetic16BitOps.h
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#ifndef LLZ80_Arithmetic16BitOps_h
#define LLZ80_Arithmetic16BitOps_h

#include "../Z80Internals.h"

void llz80_subtractWithCarry_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand);

void llz80_add_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand);
void llz80_addWithCarry_16bit(LLZ80ProcessorState *z80, LLZ80RegisterPair *source, uint16_t *operand);

#endif
