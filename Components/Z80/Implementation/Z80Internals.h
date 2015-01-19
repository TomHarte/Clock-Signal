//
//  Z80Internals.h
//  LLZ80
//
//  Created by Thomas Harte on 11/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#ifndef LLZ80_Z80Internal_h
#define LLZ80_Z80Internal_h

#include "Z80.h"
#include "stdbool.h"
#include "ReferenceCountedObject.h"
#include "StandardBusLines.h"

typedef union
{
	uint16_t fullValue;
	struct
	{
		uint8_t low, high;
	} bytes;
} LLZ80RegisterPair;

typedef enum
{
	LLZ80InterruptStateNone = 0,
	LLZ80InterruptStateHalted,
	LLZ80InterruptStateIRQ,
	LLZ80InterruptStateNMI
} LLZ80InterruptState;

typedef enum
{
	LLZ80ConditionNZ,
	LLZ80ConditionZ,
	LLZ80ConditionNC,
	LLZ80ConditionC,
	LLZ80ConditionPO,
	LLZ80ConditionPE,
	LLZ80ConditionP,
	LLZ80ConditionM,
	LLZ80ConditionYES	// always set; a convenient fiction
} LLZ80Condition;

struct LLZ80ProcessorState;
struct LLZ80InternalInstruction;

typedef void (* LLZ80InternalInstructionFunction)(struct LLZ80ProcessorState *const z80, const struct LLZ80InternalInstruction *const metadata);
extern const LLZ80InternalInstructionFunction llz80_iop_advanceHalfCycleCounter;
#define LLZ80iop(x) static void x(LLZ80ProcessorState *const z80, const LLZ80InternalInstruction *const instruction)
#define LLZ80iop_restrict(x) static void x(LLZ80ProcessorState *const restrict z80, const LLZ80InternalInstruction *const restrict instruction)

typedef struct LLZ80InternalInstruction
{
	LLZ80InternalInstructionFunction function;

	union LLZ80InternalInstructionExtraData
	{
		struct LLZ80InternalInstructionExtraDataDecode
		{
			LLZ80RegisterPair *indexRegister;
			bool addOffset;
		} opcodeDecode;

		struct LLZ80InternalInstructionExtraDataReadOrWriteAddress
		{
			uint16_t *address;
		} readOrWriteAddress;

		struct LLZ80InternalInstructionExtraDataReadOrWriteValue
		{
			uint8_t *value;
		} readOrWriteValue;

		struct LLZ80InternalInstructionExtraDataReferenceToIndexRegister
		{
			LLZ80RegisterPair *indexRegister;
		} referenceToIndexRegister;

		struct LLZ80InternalInstructionExtraDataReferenceToRegister
		{
			uint8_t *registerReference;
		} referenceToRegister;

		struct LLZ80InternalInstructionExtraDataConditional
		{
			LLZ80Condition condition;
		} conditional;

		struct LLZ80InternalInstructionExtraDataALUOrShiftOp
		{
			int operation;
		} ALUOrShiftOp;

		struct LLZ80InternalInstructionExtraDataBitOp
		{
			uint8_t mask;
			uint8_t *value;
		} bitOp;

		struct LLZ80InternalInstructionExtraDataAdvanceOp
		{
			bool isWaitCycle;
		} advance;

	} extraData;
} LLZ80InternalInstruction;

#define kLLZ80HalfCycleQueueLength	64

struct LLZ80GenericLinkedListRecord
{
	void *next, *last;
};

struct LLZ80InstructionObserverRecord
{
	void *next, *last;

	llz80_instructionObserver observer;
	void *context;
};

typedef struct LLZ80ProcessorState
{
	CSReferenceCountedObject referenceCountedObject;

	CSBusState internalBusState;
	CSBusState externalBusState;

	uint8_t aRegister;
	uint8_t generalFlags;
	uint8_t lastSignResult;
	uint8_t lastZeroResult;
	uint8_t bit5And3Flags;

	uint8_t aDashRegister;
	uint8_t fDashRegister;

	LLZ80RegisterPair bcRegister;
	LLZ80RegisterPair deRegister;
	LLZ80RegisterPair hlRegister;

	LLZ80RegisterPair bcDashRegister;
	LLZ80RegisterPair deDashRegister;
	LLZ80RegisterPair hlDashRegister;

	LLZ80RegisterPair ixRegister;
	LLZ80RegisterPair iyRegister;
	LLZ80RegisterPair spRegister;
	LLZ80RegisterPair pcRegister;

	bool iff1, iff2;
	unsigned int interruptMode;
	uint8_t rRegister;
	uint8_t iRegister;

	LLZ80InterruptState interruptState;
	LLZ80InterruptState proposedInterruptState;

	uint8_t temporary8bitValue;
	uint8_t temporaryOffset;
	LLZ80RegisterPair temporaryAddress;

	unsigned int instructionReadPointer;
	unsigned int instructionWritePointer;
	LLZ80InternalInstruction scheduledInstructions[kLLZ80HalfCycleQueueLength];
	LLZ80InternalInstruction *lastWrittenInstruction;

	struct LLZ80SignalObserverRecord *signalObservers;
	unsigned int numberOfSignalObservers, numberOfAllocatedSignalObservers;
	unsigned int allObservedSetLines, allObservedResetLines;

	struct LLZ80InstructionObserverRecord *instructionObservers;

	unsigned int internalTime;
	bool isWaiting;
	
	int nmiStatus;

} LLZ80ProcessorState;

/*

	static inline methods; glorified macros essentially

*/
static inline LLZ80InternalInstruction *llz80_scheduleFunction(LLZ80ProcessorState *const z80, LLZ80InternalInstructionFunction function)
{
	LLZ80InternalInstruction *const instruction = &z80->scheduledInstructions[z80->instructionWritePointer];
	z80->scheduledInstructions[z80->instructionWritePointer].function = function;
	z80->instructionWritePointer = (z80->instructionWritePointer + 1)%kLLZ80HalfCycleQueueLength;

	return instruction;
}

static inline LLZ80InternalInstruction *llz80_beginNewHalfCycle(LLZ80ProcessorState *const z80)
{
	LLZ80InternalInstruction *const instruction = llz80_scheduleFunction(z80, llz80_iop_advanceHalfCycleCounter);
	instruction->extraData.advance.isWaitCycle = false;
	return instruction;
}

static inline LLZ80InternalInstruction *llz80_scheduleHalfCycleForFunction(LLZ80ProcessorState *const z80, LLZ80InternalInstructionFunction function)
{
	llz80_beginNewHalfCycle(z80);
	return llz80_scheduleFunction(z80, function);
}

#define llz80m_beginScheduling()					unsigned int writePointer = z80->instructionWritePointer
#define llz80m_advanceWritePointer()				writePointer = (writePointer + 1) % kLLZ80HalfCycleQueueLength
#define llz80m_scheduleFunction(func)				z80->scheduledInstructions[writePointer].function = func; llz80m_advanceWritePointer()
#define llz80m_beginNewHalfCycle(isWait)			z80->scheduledInstructions[writePointer].function = llz80_iop_advanceHalfCycleCounter; z80->scheduledInstructions[writePointer].extraData.advance.isWaitCycle = isWait; llz80m_advanceWritePointer()
#define llz80m_scheduleHalfCycleForFunction(func)	llz80m_beginNewHalfCycle(false); llz80m_scheduleFunction(func)
#define llz80m_endScheduling()						z80->instructionWritePointer = writePointer

extern bool llz80_conditionIsTrue(const LLZ80ProcessorState *const z80, LLZ80Condition condition);
extern uint8_t llz80_getF(const LLZ80ProcessorState *const z80);
extern void llz80_setF(LLZ80ProcessorState *const z80, uint8_t value);

#define LLZ80FlagCarry				0x01
#define LLZ80FlagSubtraction		0x02
#define LLZ80FlagParityOverflow		0x04
#define LLZ80FlagBit3				0x08
#define LLZ80FlagHalfCarry			0x10
#define LLZ80FlagBit5				0x20
#define LLZ80FlagZero				0x40
#define LLZ80FlagSign				0x80

#define llz80_setLinesInactive(z80, lines)\
	{\
		z80->internalBusState.lineValues |= (lines);\
	}

#define llz80_setLinesActive(z80, lines)\
	{\
		z80->internalBusState.lineValues &= ~(lines);\
	}

#define llz80_setAddress(z80, value)\
	{\
		z80->internalBusState.lineValues = 	\
			(z80->internalBusState.lineValues & ~CSBusStandardAddressMask) | \
			((uint64_t)(value) << CSBusStandardAddressShift);\
	}


#define llz80_setDataOutput(z80, value)\
	{\
		z80->internalBusState.lineValues =	\
			(z80->internalBusState.lineValues&~CSBusStandardDataMask) | \
			((uint64_t)(value) << CSBusStandardDataShift);\
	}

#define llz80_setDataExpectingInput(z80)\
		z80->internalBusState.lineValues |= CSBusStandardDataMask;

#define llz80_getDataInput(z80)\
	((z80->externalBusState.lineValues&CSBusStandardDataMask) >> CSBusStandardDataShift)

#define llz80_linesAreActive(z80, line) !(z80->externalBusState.lineValues&(line))

/*

	parity flag is set when parity is even

*/
#define llz80_calculateParity(v)	\
	uint8_t parity = v^1;\
	parity ^= parity >> 4;\
	parity ^= parity << 2;\
	parity ^= parity >> 1;\
	parity &= LLZ80FlagParityOverflow;

#endif
