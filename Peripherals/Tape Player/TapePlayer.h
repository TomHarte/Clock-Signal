//
//  TapePlayer.h
//  Clock Signal
//
//  Created by Thomas Harte on 02/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#ifndef ClockSignal_TapePlayer_h
#define ClockSignal_TapePlayer_h

#include "AbstractTape.h"
#include "stdbool.h"

/*

	De rigeur creation and destruction stuff;
	a reference counted object as per usual.

*/
void *cstapePlayer_create(unsigned int sampleRate);

/*

	Getter and setter for the tape currently
	in this tape player.

*/
void cstapePlayer_setTape(void *player, void *tape, unsigned int timeStamp);
void *cstapePlayer_getTape(void *player);

/*

	A means to get the tape-referential time based
	on the current machine time stamp. Most machines
	will want to get the tape using the getter above,
	get the tape time using this method then query the
	tape for a waveform.

*/
uint64_t cstapePlayer_getTapeTime(void *player, unsigned int timeStamp);
void cstapePlayer_setTapeTime(void *player, unsigned int timeStamp, uint64_t tapeTime);

/*

	For now all the tape player has is a play button,
	a pause button and a rewind-to-start button.

*/
void cstapePlayer_play(void *player, unsigned int timeStamp);
void cstapePlayer_pause(void *player, unsigned int timeStamp);
bool cstapePlayer_isTapePlaying(void *player);
void cstapePlayer_rewindToStart(void *player, unsigned int timeStamp);

void cstapePlayer_runToTime(void *player, unsigned int timeStamp);

/*

	The tape player can also play the tape in the
	sense of produce the output audio of a tape.

	Register a delegate in order to receieve
	signed mono 16bit audio in the native endianness,
	at the sample rate set along with the delegate.

*/
typedef void (* cstapePlayer_audioDelegate)(
	void *tapePlayer,
	unsigned int numberOfSamples,
	short *sampleBuffer,
	void *context);

void cstapePlayer_setAudioDelegate(
	void *player,
	cstapePlayer_audioDelegate delegate,
	unsigned int outputSampleRate,
	unsigned int singleBufferSize,
	void *context);

#endif
