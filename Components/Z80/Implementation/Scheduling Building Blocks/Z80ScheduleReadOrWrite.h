//
//  ScheduleReadOrWrite.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 acrossair. All rights reserved.
//

#ifndef LLZ80_ScheduleReadOrWrite_h
#define LLZ80_ScheduleReadOrWrite_h

#import "../Z80Internals.h"

LLZ80InternalInstruction *llz80_scheduleRead(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address);

LLZ80InternalInstruction *llz80_scheduleWrite(
	LLZ80ProcessorState *z80,
	uint8_t *value,
	uint16_t *address);

#endif
