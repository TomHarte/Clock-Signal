//
//  BitwiseOps.h
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#ifndef LLZ80_BitwiseOps_h
#define LLZ80_BitwiseOps_h

#include "../Z80Internals.h"

void llz80_bitwiseAnd(LLZ80ProcessorState *z80, uint8_t value);
void llz80_bitwiseOr(LLZ80ProcessorState *z80, uint8_t value);
void llz80_bitwiseXOr(LLZ80ProcessorState *z80, uint8_t value);

#endif
