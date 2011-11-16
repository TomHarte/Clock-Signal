//
//  ZX80Tape.h
//  Clock Signal
//
//  Created by Thomas Harte on 27/09/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_ZX80Tape_h
#define ClockSignal_ZX80Tape_h

#include "stdint.h"
#include "stdbool.h"

// these both return a suitable pointer for use with the abstract tape
// interface defined in AbstractTape.h. createWithData tapes a copy of
// the data passed in; length is in bytes.
// 
// P files aren't a complete copy of the data stored to tape, they're
// a complete copy of the data that would load from tape. The difference
// is that a tape would store the filename, then the data — so P files
// are adjusted by the addition of an empty file name at the beginning.
//
// createFromFile looks at the file extension to determine what to do,
// you'll have to tell createWithData what to do for yourself.
void *cszx80tape_createFromFile(const char *filename);
void *cszx80tape_createWithData(const uint8_t *data, unsigned int length, bool isPTape);

#endif
