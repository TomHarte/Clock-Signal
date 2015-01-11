//
//  ScheduleReadOrWrite.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_ScheduleReadOrWrite_h
#define LLZ80_ScheduleReadOrWrite_h

#include "../Z80Internals.h"

extern LLZ80InternalInstruction *llz80_scheduleRead(
	LLZ80ProcessorState *const z80,
	uint8_t *const value,
	uint16_t *const address);

extern LLZ80InternalInstruction *llz80_scheduleWrite(
	LLZ80ProcessorState *const z80,
	uint8_t *const value,
	uint16_t *const address);

#endif
