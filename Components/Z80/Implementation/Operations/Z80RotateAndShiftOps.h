//
//  RotateAndShift.h
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_RotateAndShift_h
#define LLZ80_RotateAndShift_h

#include "../Z80Internals.h"

void llz80_rla(LLZ80ProcessorState *const z80);
void llz80_rlca(LLZ80ProcessorState *const z80);

void llz80_rra(LLZ80ProcessorState *const z80);
void llz80_rrca(LLZ80ProcessorState *const z80);

void llz80_rlc(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_rrc(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_rl(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_rr(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_sla(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_sra(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_sll(LLZ80ProcessorState *const z80, uint8_t *const value);
void llz80_srl(LLZ80ProcessorState *const z80, uint8_t *const value);

#endif
