//
//  Arithmetic8BitOps.h
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_Arithmetic8BitOps_h
#define LLZ80_Arithmetic8BitOps_h

#include "../Z80Internals.h"

void llz80_compare(LLZ80ProcessorState *const z80, uint8_t value);

void llz80_subtract_8bit(LLZ80ProcessorState *const z80, uint8_t value);
void llz80_subtractWithCarry_8bit(LLZ80ProcessorState *const z80, uint8_t value);

void llz80_add_8bit(LLZ80ProcessorState *const z80, uint8_t value);
void llz80_addWithCarry_8bit(LLZ80ProcessorState *const z80, uint8_t value);

void llz80_increment_8bit(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_decrement_8bit(LLZ80ProcessorState *const z80, uint8_t *const value);

#endif
