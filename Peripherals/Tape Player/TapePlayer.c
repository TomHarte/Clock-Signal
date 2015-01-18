//
//  TapePlayer.c
//  Clock Signal
//
//  Created by Thomas Harte on 02/10/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "TapePlayer.h"
#include "LinearFilter.h"
#include "ReferenceCountedObject.h"

typedef struct
{
	CSReferenceCountedObject referenceCountedObject;

	unsigned int tapeSampleRate;

	void *tape;
	void *audioCopyOfTape;

	uint64_t tapeTime;
	unsigned int currentTimeStamp;
	bool tapeIsRunning;

	cstapePlayer_audioDelegate audioDelegate;
	void *audioDelegateContext;
	unsigned int audioSampleRate;

	unsigned int audioBufferWritePointer;
	unsigned int minimumAudioSamplesToProcess;
	unsigned int audioBufferOutputSize;
	short *audioBuffer;

	void *audioFilter;

} CSTapePlayer;

#define kCSTapeNumTaps		 	23

static void cstapePlayer_destroy(void *opaquePlayer)
{
	CSTapePlayer *player = (CSTapePlayer *)opaquePlayer;

	if(player->audioBuffer)		free(player->audioBuffer);
	if(player->tape)			cstape_release(player->tape);
	if(player->audioCopyOfTape)	cstape_release(player->audioCopyOfTape);
	if(player->audioFilter)		csfilter_release(player->audioFilter);
}

void *cstapePlayer_create(unsigned int sampleRate)
{
	CSTapePlayer *player = (CSTapePlayer *)calloc(1, sizeof(CSTapePlayer));

	if(player)
	{
		csObject_init(player);
		player->referenceCountedObject.dealloc = cstapePlayer_destroy;
		player->tapeSampleRate = sampleRate;
	}

	return player;
}

void cstapePlayer_setTape(void *opaquePlayer, void *tape, unsigned int timeStamp)
{
	cstapePlayer_runToTime(opaquePlayer, timeStamp);

	CSTapePlayer *player = (CSTapePlayer *)opaquePlayer;

	if(player->tape)
	{
		cstape_release(player->tape);
		cstape_release(player->audioCopyOfTape);
	}
	player->tape = cstape_retain(tape);
	player->audioCopyOfTape = cstape_copy(tape);

	cstape_setSampleRate(tape, player->tapeSampleRate);
//	cstape_setSampleRate(
//		player->audioCopyOfTape,
//		cstape_getMinimumAccurateSampleRate(player->audioCopyOfTape));
	cstape_setSampleRate(
		player->audioCopyOfTape,
		player->tapeSampleRate);
}

void *cstapePlayer_getTape(void *opaquePlayer)
{
	CSTapePlayer *player = (CSTapePlayer *)opaquePlayer;
	return player->tape;
}

void cstapePlayer_runToTime(void *opaquePlayer, unsigned int timeStamp)
{
	CSTapePlayer *player = (CSTapePlayer *)opaquePlayer;
	unsigned int timeToRunFor = timeStamp - player->currentTimeStamp;

	if(player->audioDelegate && player->tape)
	{
		uint64_t tapeLength = cstape_getLength(player->tape);
		if(player->tapeTime < tapeLength)
		{
			while(timeToRunFor)
			{
				unsigned int samplesToWrite = timeToRunFor;
				unsigned int samplesLeftToFill = player->minimumAudioSamplesToProcess - player->audioBufferWritePointer;
				if(samplesToWrite > samplesLeftToFill) samplesToWrite = samplesLeftToFill;
				timeToRunFor -= samplesToWrite;
			
				if(player->tapeIsRunning)
				{
					uint64_t timeToSample = player->tapeTime;

					// add tape waveform to buffer
					while(samplesToWrite)
					{
						if(timeToSample >= cstape_getLength(player->audioCopyOfTape))
						{
							player->tapeIsRunning = false;
							break;
						}

						uint64_t endTime;
						cstape_getLevelPeriodAroundTime(player->audioCopyOfTape, timeToSample, NULL, &endTime);

						uint32_t level;
						switch(cstape_getLevelAtTime(player->audioCopyOfTape, timeToSample))
						{
							default:
								level = 0;
							break;
							case CSTapeLevelHigh:
								level = 32767;
							break;
							case CSTapeLevelLow:
								level = 32769;	// which is -32767 if the low 16bits are cut off
							break;
						}
						level |= level << 16;

						unsigned int count = (unsigned int)(endTime - timeToSample);
						if(count > samplesToWrite) count = samplesToWrite;

						memset_pattern4(&player->audioBuffer[player->audioBufferWritePointer], &level, count << 1);

						player->audioBufferWritePointer += count;
						timeToSample += count;
						samplesToWrite -= count;
					}
				}

				if(!player->tapeIsRunning)
				{
					// add silence to buffer
					memset(&player->audioBuffer[player->audioBufferWritePointer], 0x00, samplesToWrite * sizeof(short));
					player->audioBufferWritePointer += samplesToWrite;
				}

				// if the buffer is sufficiently full,
				// pass a segment on to the delegate
				if(player->audioBufferWritePointer == player->minimumAudioSamplesToProcess)
				{
					short *outputBuffer = (short *)malloc(sizeof(short)*player->audioBufferOutputSize);

					unsigned int samplesTaken =
						csfilter_applyToBuffer(
							player->audioFilter,
							outputBuffer,
							player->audioBuffer,
							(float)player->tapeSampleRate / (float)player->audioSampleRate,
							player->audioBufferOutputSize);

					unsigned int residue = player->minimumAudioSamplesToProcess - samplesTaken;
					memcpy(player->audioBuffer, &player->audioBuffer[samplesTaken], residue * sizeof(short));
					player->audioBufferWritePointer = residue;

					player->audioDelegate(player, player->audioBufferOutputSize, outputBuffer, player->audioDelegateContext);
				}			
			}
		}
	}

	if(player->tapeIsRunning)
	{
		player->tapeTime += timeStamp - player->currentTimeStamp;
	}

	player->currentTimeStamp = timeStamp;
}

void cstapePlayer_setTapeTime(void *player, unsigned int timeStamp, uint64_t tapeTime)
{
	cstapePlayer_runToTime(player, timeStamp);
	((CSTapePlayer *)player)->tapeTime = tapeTime;
}

uint64_t cstapePlayer_getTapeTime(void *player, unsigned int timeStamp)
{
	cstapePlayer_runToTime(player, timeStamp);
	return ((CSTapePlayer *)player)->tapeTime;
}

void cstapePlayer_play(void *player, unsigned int timeStamp)
{
	cstapePlayer_runToTime(player, timeStamp);
	((CSTapePlayer *)player)->tapeIsRunning = true;
}

void cstapePlayer_pause(void *player, unsigned int timeStamp)
{
	cstapePlayer_runToTime(player, timeStamp);
	((CSTapePlayer *)player)->tapeIsRunning = false;
}

void cstapePlayer_rewindToStart(void *player, unsigned int timeStamp)
{
	cstapePlayer_runToTime(player, timeStamp);
	((CSTapePlayer *)player)->tapeTime = 0;
}

bool cstapePlayer_isTapePlaying(void *player)
{
	return ((CSTapePlayer *)player)->tapeIsRunning;
}

void cstapePlayer_setAudioDelegate(
	void *opaquePlayer,
	cstapePlayer_audioDelegate delegate,
	unsigned int outputSampleRate,
	unsigned int singleBufferSize,
	void *context)
{
	CSTapePlayer *player = (CSTapePlayer *)opaquePlayer;

	player->audioDelegateContext = context;
	player->audioDelegate = delegate;
	player->audioSampleRate = outputSampleRate;
	player->audioBufferWritePointer = 0;

	// calculate the minimum number of samples we'll need to accumulate
	// at the tape's sample rate in order to be able to provide 2048
	// samples at the audio sample rate. Use:
	//
	//	ceil( singlebufferSize * tapeSampleRate / audioSampleRate )
	//
	uint64_t bigSampleRate = player->tapeSampleRate;
	bigSampleRate *= singleBufferSize;
	bigSampleRate += player->audioSampleRate - 1;
	bigSampleRate /= player->audioSampleRate;
	player->minimumAudioSamplesToProcess = (unsigned int)bigSampleRate;
	player->minimumAudioSamplesToProcess += kCSTapeNumTaps;
	player->audioBufferOutputSize = singleBufferSize;

	if(delegate)
	{
		if(!player->audioBuffer)
		{
			player->audioBuffer = (short *)malloc(sizeof(short) * player->minimumAudioSamplesToProcess);
			player->audioFilter = csfilter_createBandPass(kCSTapeNumTaps, player->tapeSampleRate, 0, outputSampleRate >> 1, kCSFilterDefaultAttenuation);
		}
	}
	else
	{
		if(player->audioBuffer)
		{
			free(player->audioBuffer);
			player->audioBuffer = NULL;

			csfilter_release(player->audioFilter);
			player->audioFilter = NULL;
		}
	}
}
