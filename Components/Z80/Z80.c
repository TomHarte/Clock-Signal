//
//  Z80.c
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#include "Z80Internals.h"
#include "BusState.h"
#include "BusNode.h"
#include "Component.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Z80StandardPageDecode.h"

#include "Z80ScheduleInstructionFetch.h"
#include "Z80StandardSchedulingComponents.h"
#include "Z80ScheduleReadOrWrite.h"

static void llz80_destroyGenericList(struct LLZ80GenericLinkedListRecord *list)
{
	while(list)
	{
		void *const thisRecord = list;
		list = (struct LLZ80GenericLinkedListRecord *)list->next;
		free(thisRecord);
	}
}

static void llz80_insertItemIntoGenericList(struct LLZ80GenericLinkedListRecord **const list, struct LLZ80GenericLinkedListRecord *const item)
{
	item->next = *list;
	if(*list)
	{
		(*list)->last = item;
	}
	*list = item;
}

static void llz80_removeItemFromGenericList(struct LLZ80GenericLinkedListRecord **const list, struct LLZ80GenericLinkedListRecord *const item)
{
	// if listener record has a 'last' then unlink it from
	// mid list
	if(item->last)
	{
		((struct LLZ80GenericLinkedListRecord *)item->last)->next = item->next;

		if(item->next)
			((struct LLZ80GenericLinkedListRecord *)item->next)->last = item->last;
	}
	else
	{
		// if it didn't have a 'last' then it's at the start of the list
		*list = (struct LLZ80GenericLinkedListRecord *)item->next;

		if(item->next)
			((struct LLZ80GenericLinkedListRecord *)item->next)->last = NULL;
	}

	free(item);
}

static void llz80_destroy(void *const opaqueZ80)
{
	const LLZ80ProcessorState *const z80 = opaqueZ80;

	// release all existing refereces to listeners
	free(z80->signalObservers);
	llz80_destroyGenericList((struct LLZ80GenericLinkedListRecord *)z80->instructionObservers);
}

LLZ80iop(llz80_iop_advanceHalfCycleCounter_imp)
{
	// increment the internal time counter
	z80->internalTime++;

	// if this is a leading edge, check for interrupts and
	// work out what we'd do if this does turn out to be
	// the final sample before an instruction fetch
	if(z80->externalBusState.lineValues & CSBusStandardClockLine)
	{
		if(z80->nmiStatus == 1)
			z80->proposedInterruptState = LLZ80InterruptStateNMI;
		else
			z80->proposedInterruptState = (llz80_linesAreActive(z80, LLZ80SignalInterruptRequest) && z80->iff1) ? LLZ80InterruptStateIRQ : LLZ80InterruptStateNone;
	}

	// if this is a cycle that checks the wait state then do so
	if(instruction->extraData.advance.isWaitCycle)
		z80->isWaiting = llz80_linesAreActive(z80, LLZ80SignalWait);
}

LLZ80iop(llz80_iop_setupForInterruptMode1)
{
	z80->temporary8bitValue = 0xff;
}

LLZ80iop(llz80_iop_setupForInterruptMode2)
{
	z80->temporaryAddress.bytes.high = z80->iRegister;
	z80->temporaryAddress.bytes.low = z80->temporary8bitValue;

	// it's unclear what order the following things happen in; this is a guess
	// based on the fact that the Z80 doesn't use two temporary 16bit values
	// at any other time

	// predecrement the stack pointer
	llz80_beginNewHalfCycle(z80);
	llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);
	llz80_beginNewHalfCycle(z80);

	llz80_beginNewHalfCycle(z80);
	llz80_beginNewHalfCycle(z80);

	// store out the old program counter
	llz80_scheduleWrite(z80, &z80->pcRegister.bytes.high, &z80->spRegister.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);

	llz80_scheduleWrite(z80, &z80->pcRegister.bytes.low, &z80->spRegister.fullValue);

	// read new program counter
	llz80_scheduleRead(z80, &z80->pcRegister.bytes.low, &z80->temporaryAddress.fullValue);
	llz80_scheduleFunction(z80, llz80_iop_incrementTemporaryAddress);
	llz80_scheduleRead(z80, &z80->pcRegister.bytes.high, &z80->temporaryAddress.fullValue);
}

const LLZ80InternalInstructionFunction llz80_iop_advanceHalfCycleCounter = llz80_iop_advanceHalfCycleCounter_imp;
static LLZ80InternalInstruction waitCycles[2];

csComponent_observer(llz80_observeClock)
{
	LLZ80ProcessorState *const z80 = (LLZ80ProcessorState *const)context;

	z80->externalBusState = externalState;

	// NMI is edge sampled, continuously
	if(!(externalState.lineValues&LLZ80SignalNonMaskableInterruptRequest))
	{
		if(!z80->nmiStatus)
			z80->nmiStatus = 1;
	}
	else
	{
		if(z80->nmiStatus == 2)
			z80->nmiStatus = 0;
	}

	if(z80->isWaiting)
	{
		llz80_iop_advanceHalfCycleCounter_imp(z80, &waitCycles[z80->internalTime&1]);
	}
	else
	{
		while(1)
		{
			while(z80->instructionReadPointer != z80->instructionWritePointer)
			{
				const LLZ80InternalInstruction *const instruction =
					&z80->scheduledInstructions[z80->instructionReadPointer];
				z80->instructionReadPointer = (z80->instructionReadPointer+1)%kLLZ80HalfCycleQueueLength;

				instruction->function(z80, instruction);
				if(instruction->function == llz80_iop_advanceHalfCycleCounter_imp) goto doubleBreak;
			}

			if(	(z80->proposedInterruptState == LLZ80InterruptStateIRQ) ||
				(z80->proposedInterruptState == LLZ80InterruptStateNMI))
			{
				if(z80->interruptState == LLZ80InterruptStateHalted)
				{
					llz80_setLinesInactive(z80, LLZ80SignalHalt);
				}

				z80->interruptState = z80->proposedInterruptState;
			}

			switch(z80->interruptState)
			{
				default:
				{
					// if someone is observing for the beginning of new instruction fetches,
					// then give them a shout out now
					const struct LLZ80InstructionObserverRecord *instructionObserver = z80->instructionObservers;
					while(instructionObserver)
					{
						instructionObserver->observer(z80, instructionObserver->context);
						instructionObserver = instructionObserver->next;
					}

					llz80_scheduleInstructionFetchForFunction(z80, llz80_iop_standardPageDecode, &z80->hlRegister, false);
				}
				break;

				case LLZ80InterruptStateHalted:
				{
					llz80_scheduleNOP(z80);
				}
				break;

				case LLZ80InterruptStateIRQ:
				{
					// accept IRQ
					z80->iff1 = z80->iff2 = false;
					z80->interruptState = LLZ80InterruptStateNone;

					llz80_scheduleIRQAcknowledge(z80);		// 5 cycles
					LLZ80InternalInstruction *instruction = NULL;

					switch (z80->interruptMode)
					{
						case 0:
							// actually use whatever was put on the bus as the opcode
							llz80_schedulePauseForCycles(z80, 1);
							instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_iop_standardPageDecode);
						break;

						case 1:
							llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setupForInterruptMode1);
							instruction = llz80_scheduleHalfCycleForFunction(z80, llz80_iop_standardPageDecode);
								// so that's the acknowledge 5 cycles + 1 cycle here + the time to do an RST 38
								// (without a fetch), which takes a further 7, for 13 total
						break;

						case 2:
							llz80_scheduleHalfCycleForFunction(z80, llz80_iop_setupForInterruptMode2);
						break;
					}

					if(instruction)
					{
						instruction->extraData.opcodeDecode.indexRegister = &z80->hlRegister;
						instruction->extraData.opcodeDecode.addOffset = false;
					}
				}
				break;

				case LLZ80InterruptStateNMI:
				{
					z80->nmiStatus = 2;
					z80->proposedInterruptState =
					z80->interruptState = LLZ80InterruptStateNone;

					// accept an NMI; this is
					// achieved by a spurious instruction read
					// and then a call to 0x66,
					// after setting the IFF1 flag
					// to NO so that an IRQ can't
					// interrupt from now on.
					z80->iff1 = false;

					// do the pseudo instruction fetch
					llz80_schedulePseudoInstructionFetch(z80);

					// predecrement the stack pointer
					llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);

					// do the call to 0x66
					z80->temporaryAddress.fullValue = 0x66;

					// store out the old program counter, then adjust it
					llz80_scheduleWrite(z80, &z80->pcRegister.bytes.high, &z80->spRegister.fullValue);
					llz80_scheduleFunction(z80, llz80_iop_decrementStackPointer);

					llz80_scheduleWrite(z80, &z80->pcRegister.bytes.low, &z80->spRegister.fullValue);
					llz80_scheduleFunction(z80, llz80_iop_setPCToTemporaryAddress);
				}
				break;
			}
		}
	}

	doubleBreak:
	*internalState = z80->internalBusState;
}

void *llz80_createOnBus(void *const bus)
{
	LLZ80ProcessorState *const z80 = (LLZ80ProcessorState *)calloc(1, sizeof(LLZ80ProcessorState));

	if(z80)
	{
		// set up our wait cycle
		waitCycles[0].extraData.advance.isWaitCycle = false;
		waitCycles[1].extraData.advance.isWaitCycle = true;

		// return an owning reference
		csObject_init(z80);
		z80->referenceCountedObject.dealloc = llz80_destroy;

		// place the z80 in power-on state
		z80->aRegister = 0xff;
		z80->generalFlags = z80->lastSignResult = z80->bit5And3Flags = 0xff;
		z80->spRegister.fullValue = 0xffff;

		// add to the bus
		z80->internalBusState = csBus_defaultState();
		void *component = 
			csComponent_create(
				llz80_observeClock,
				csBus_resetCondition(CSBusStandardClockLine, false),
				CSBusStandardDataMask | CSBusStandardAddressMask | LLZ80SignalInputOutputRequest |
				LLZ80SignalMachineCycleOne | LLZ80SignalRead | LLZ80SignalWrite |
				LLZ80SignalMemoryRequest | LLZ80SignalRefresh | LLZ80SignalBusAcknowledge |
				LLZ80SignalHalt,
				z80);
		csBusNode_addComponent(bus, component);
		csObject_release(component);
	}

	return z80;
}

void *llz80_monitor_addInstructionObserver(void *const opaqueZ80, llz80_instructionObserver observer, void *const context)
{
	const LLZ80ProcessorState *const z80 = (LLZ80ProcessorState *)opaqueZ80;
	struct LLZ80InstructionObserverRecord *observerRecord = 
		(struct LLZ80InstructionObserverRecord *)calloc(1, sizeof(struct LLZ80InstructionObserverRecord));

	if(observerRecord)
	{
		observerRecord->observer = observer;
		observerRecord->context = context;

		llz80_insertItemIntoGenericList(
			(struct LLZ80GenericLinkedListRecord **)(&z80->instructionObservers),
			(struct LLZ80GenericLinkedListRecord *)observerRecord);
	}

	return observerRecord;
}

void llz80_monitor_removeInstructionObserver(void *opaqueZ80, void *observer)
{
	LLZ80ProcessorState *z80 = opaqueZ80;

	llz80_removeItemFromGenericList(
			(struct LLZ80GenericLinkedListRecord **)(&z80->instructionObservers),
			observer);
}

uint64_t llz80_monitor_getBusLineState(void *opaqueZ80)
{
	const LLZ80ProcessorState *const z80 = opaqueZ80;

	return
		z80->internalBusState.lineValues & 
		(
			z80->externalBusState.lineValues | 
				~(
					LLZ80SignalInterruptRequest | LLZ80SignalNonMaskableInterruptRequest |
					LLZ80SignalReset | LLZ80SignalWait | LLZ80SignalBusRequest |
					CSBusStandardAddressMask | CSBusStandardClockLine | CSBusStandardDataMask
				)
		);
}

unsigned int llz80_monitor_getInternalValue(void *const opaqueZ80, LLZ80MonitorValue key)
{
	const LLZ80ProcessorState *const z80 = opaqueZ80;

	switch(key)
	{
		default: return 0;

		case LLZ80MonitorValueARegister: return z80->aRegister;
		case LLZ80MonitorValueFRegister:
		{
			uint8_t flagRegister = llz80_getF(z80);
			return flagRegister;
		}
		case LLZ80MonitorValueBRegister: 		return z80->bcRegister.bytes.high;
		case LLZ80MonitorValueCRegister: 		return z80->bcRegister.bytes.low;
		case LLZ80MonitorValueDRegister: 		return z80->deRegister.bytes.high;
		case LLZ80MonitorValueERegister: 		return z80->deRegister.bytes.low;
		case LLZ80MonitorValueHRegister: 		return z80->hlRegister.bytes.high;
		case LLZ80MonitorValueLRegister: 		return z80->hlRegister.bytes.low;

		case LLZ80MonitorValueADashRegister:	return z80->aDashRegister;
		case LLZ80MonitorValueFDashRegister:	return z80->fDashRegister;
		case LLZ80MonitorValueBDashRegister:	return z80->bcDashRegister.bytes.high;
		case LLZ80MonitorValueCDashRegister:	return z80->bcDashRegister.bytes.low;
		case LLZ80MonitorValueDDashRegister:	return z80->deDashRegister.bytes.high;
		case LLZ80MonitorValueEDashRegister:	return z80->deDashRegister.bytes.low;
		case LLZ80MonitorValueHDashRegister:	return z80->hlDashRegister.bytes.high;
		case LLZ80MonitorValueLDashRegister:	return z80->hlDashRegister.bytes.low;

		case LLZ80MonitorValueAFDashRegister:	return (unsigned)((z80->aDashRegister << 8) | z80->fDashRegister);
		case LLZ80MonitorValueBCDashRegister:	return z80->bcDashRegister.fullValue;
		case LLZ80MonitorValueDEDashRegister:	return z80->deDashRegister.fullValue;
		case LLZ80MonitorValueHLDashRegister:	return z80->hlDashRegister.fullValue;

		case LLZ80MonitorValueAFRegister:
		{
			uint8_t flagRegister = llz80_getF(z80);
			return (unsigned)((z80->aRegister << 8) | flagRegister);
		}
		case LLZ80MonitorValueBCRegister:		return z80->bcRegister.fullValue;
		case LLZ80MonitorValueDERegister:		return z80->deRegister.fullValue;
		case LLZ80MonitorValueHLRegister:		return z80->hlRegister.fullValue;

		case LLZ80MonitorValueRRegister: 		return z80->rRegister;
		case LLZ80MonitorValueIRegister: 		return z80->iRegister;

		case LLZ80MonitorValueIXRegister: 		return z80->ixRegister.fullValue;
		case LLZ80MonitorValueIYRegister: 		return z80->iyRegister.fullValue;
		case LLZ80MonitorValueSPRegister: 		return z80->spRegister.fullValue;
		case LLZ80MonitorValuePCRegister: 		return z80->pcRegister.fullValue;

		case LLZ80MonitorValueIFF1Flag:			return z80->iff1;
		case LLZ80MonitorValueIFF2Flag:			return z80->iff2;

		case LLZ80MonitorValueInterruptMode:	return z80->interruptMode;

		case LLZ80MonitorValueHalfCyclesToDate:	return z80->internalTime;
	}
}

void llz80_monitor_setInternalValue(void *const opaqueZ80, LLZ80MonitorValue key, unsigned int value)
{
	LLZ80ProcessorState *const z80 = opaqueZ80;

	switch(key)
	{
		default: break;

		case LLZ80MonitorValueARegister: z80->aRegister = (uint8_t)value;	break;
		case LLZ80MonitorValueFRegister:
		{
			llz80_setF(z80, (uint8_t)value);
		}
		break;
		case LLZ80MonitorValueBRegister: 		z80->bcRegister.bytes.high = (uint8_t)value;		break;
		case LLZ80MonitorValueCRegister: 		z80->bcRegister.bytes.low = (uint8_t)value;			break;
		case LLZ80MonitorValueDRegister: 		z80->deRegister.bytes.high = (uint8_t)value;		break;
		case LLZ80MonitorValueERegister: 		z80->deRegister.bytes.low = (uint8_t)value;			break;
		case LLZ80MonitorValueHRegister: 		z80->hlRegister.bytes.high = (uint8_t)value;		break;
		case LLZ80MonitorValueLRegister: 		z80->hlRegister.bytes.low = (uint8_t)value;			break;

		case LLZ80MonitorValueADashRegister:	z80->aDashRegister = (uint8_t)value;				break;
		case LLZ80MonitorValueFDashRegister:	z80->fDashRegister = (uint8_t)value;				break;
		case LLZ80MonitorValueBDashRegister:	z80->bcDashRegister.bytes.high = (uint8_t)value;	break;
		case LLZ80MonitorValueCDashRegister:	z80->bcDashRegister.bytes.low = (uint8_t)value;		break;
		case LLZ80MonitorValueDDashRegister:	z80->deDashRegister.bytes.high = (uint8_t)value;	break;
		case LLZ80MonitorValueEDashRegister:	z80->deDashRegister.bytes.low = (uint8_t)value;		break;
		case LLZ80MonitorValueHDashRegister:	z80->hlDashRegister.bytes.high = (uint8_t)value;	break;
		case LLZ80MonitorValueLDashRegister:	z80->hlDashRegister.bytes.low = (uint8_t)value;		break;

		case LLZ80MonitorValueAFDashRegister:
		{
			z80->aDashRegister = (uint8_t)(value >> 6);
			z80->fDashRegister = (uint8_t)value;
		}
		break;
		case LLZ80MonitorValueBCDashRegister:	z80->bcDashRegister.fullValue = (uint16_t)value;	break;
		case LLZ80MonitorValueDEDashRegister:	z80->deDashRegister.fullValue = (uint16_t)value;	break;
		case LLZ80MonitorValueHLDashRegister:	z80->hlDashRegister.fullValue = (uint16_t)value;	break;

		case LLZ80MonitorValueAFRegister:
		{
			llz80_setF(z80, (uint8_t)value);
			z80->aRegister = (uint8_t)(value >> 8);
		}
		break;
		case LLZ80MonitorValueBCRegister:		z80->bcRegister.fullValue = (uint16_t)value;		break;
		case LLZ80MonitorValueDERegister:		z80->deRegister.fullValue = (uint16_t)value;		break;
		case LLZ80MonitorValueHLRegister:		z80->hlRegister.fullValue = (uint16_t)value;		break;

		case LLZ80MonitorValueRRegister: 		z80->rRegister = (uint8_t)value;					break;
		case LLZ80MonitorValueIRegister: 		z80->iRegister = (uint8_t)value;					break;

		case LLZ80MonitorValueIXRegister: 		z80->ixRegister.fullValue = (uint16_t)value;		break;
		case LLZ80MonitorValueIYRegister: 		z80->iyRegister.fullValue = (uint16_t)value;		break;
		case LLZ80MonitorValueSPRegister: 		z80->spRegister.fullValue = (uint16_t)value;		break;
		case LLZ80MonitorValuePCRegister: 		z80->pcRegister.fullValue = (uint16_t)value;		break;

		case LLZ80MonitorValueIFF1Flag:			z80->iff1 = value;									break;
		case LLZ80MonitorValueIFF2Flag:			z80->iff2 = value;									break;

		case LLZ80MonitorValueInterruptMode:	z80->interruptMode = value;							break;

		case LLZ80MonitorValueHalfCyclesToDate:	z80->internalTime = value;							break;
	}
}
