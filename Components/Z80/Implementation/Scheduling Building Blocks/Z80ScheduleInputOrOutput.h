//
//  ScheduleInputOrOutput.h
//  LLZ80
//
//  Created by Thomas Harte on 12/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#ifndef LLZ80_ScheduleInputOrOutput_h
#define LLZ80_ScheduleInputOrOutput_h

#include "../Z80Internals.h"

LLZ80InternalInstruction *llz80_scheduleInput(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address);

LLZ80InternalInstruction *llz80_scheduleOutput(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address);

#endif
