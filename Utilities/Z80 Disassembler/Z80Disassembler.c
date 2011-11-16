//
//  Z80Disassembler.c
//  Clock Signal
//
//  Created by Thomas Harte on 11/11/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "Z80Disassembler.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "Array.h"
#include "stdbool.h"

static char *ccTable[] = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
static char *aluTable[] = {"ADD A,", "ADC A,", "SUB ", "SBC A,", "AND", "XOR", "OR", "CP"};
static char *rotTable[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
static char *imTable[] = {"0", "0/1", "1", "2", "0", "0/1", "1", "2"};
static char *bliTable[] = { 
			"LDI", "CPI", "INI", "OUTI",
			"LDD", "CPD", "IND", "OUTD",
			"LDIR", "CPIR", "INIR", "OTIR",
			"LDDR", "CPDR", "INDR", "OTDR"};
#define bliTable(a, b) bliTable[((a) << 2) | (b)]

typedef struct
{
	uint8_t *sourceData;
	uint16_t startAddress;
	uint16_t currentOffset;
	uint16_t length;
	void *output;
	void *releasePool;

	char *indexRegister;
	bool addOffset;
} CSZ80DisassemblerState;

static char *csZ80Disassembler_getTemporaryBuffer(CSZ80DisassemblerState *state, size_t length)
{
	char *newBuffer = (char *)malloc(length);
	csArray_addObject(state->releasePool, newBuffer);
	return newBuffer;
}

static uint8_t csZ80Disassembler_getByte(CSZ80DisassemblerState *state)
{
	if(state->currentOffset == state->length) return 0;
	return state->sourceData[state->currentOffset++];
}

static char *csZ80Disassembler_getOffset(CSZ80DisassemblerState *state)
{
	char *newBuffer = csZ80Disassembler_getTemporaryBuffer(state, 6);
	snprintf(newBuffer, 6, "$%04x", (int8_t)csZ80Disassembler_getByte(state) + state->currentOffset + state->startAddress);
	return newBuffer;
}

static uint16_t csZ80Disassembler_getShort(CSZ80DisassemblerState *state)
{
	uint16_t returnValue = csZ80Disassembler_getByte(state);
	returnValue |= ((uint16_t)csZ80Disassembler_getByte(state)) << 8;
	return returnValue;
}

static char *csZ80Disassembler_getAddress(CSZ80DisassemblerState *state)
{
	char *newBuffer = csZ80Disassembler_getTemporaryBuffer(state, 6);
	snprintf(newBuffer, 6, "L%04x", csZ80Disassembler_getShort(state) + state->startAddress);
	return newBuffer;
}

static void csZ80Disassembler_beginOpcode(CSZ80DisassemblerState *state)
{
	printf("%04x: ", state->currentOffset + state->startAddress);
}

static void csZ80Disassembler_setOutput(CSZ80DisassemblerState *state, char *text, ...)
{
	va_list args;
	va_start(args, text);

	vprintf(text, args);
	printf("\n");

	va_end(args);
}

static char *indexPlusOffset(CSZ80DisassemblerState *state, int8_t offset)
{
	char *buffer = csZ80Disassembler_getTemporaryBuffer(state, 11);

	if(offset >= 0)
		snprintf(buffer, 11, "(%s + $%02x)", state->indexRegister, offset);
	else
		snprintf(buffer, 11, "(%s - $%02x)", state->indexRegister, abs(offset));

	return buffer;
}

static char *rTable(CSZ80DisassemblerState *state, int index)
{
	static char *rTable[] = {"B", "C", "D", "E", "H", "L", NULL, "A"};

	if(rTable[index]) return rTable[index];
	if(state->addOffset)
	{
		return indexPlusOffset(state, csZ80Disassembler_getByte(state));
	}
	else
		return "(HL)";
}

static char *rpTable(CSZ80DisassemblerState *state, int index)
{
	char *rpTable[] = {"BC", "DE", NULL, "SP"};
	return rpTable[index] ? rpTable[index]: state->indexRegister;
}

static char *rp2Table(CSZ80DisassemblerState *state, int index)
{
	char *rp2Table[] = {"BC", "DE", NULL, "AF"};
	return rp2Table[index] ? rp2Table[index]: state->indexRegister;
}


static void csZ80Disassembler_disassembleCBPage(CSZ80DisassemblerState *state)
{
	int8_t displacement = 0;
	if(state->addOffset) displacement = csZ80Disassembler_getByte(state);
	uint8_t opcode = csZ80Disassembler_getByte(state);
	int x = opcode >> 6;
	int y = (opcode >> 3)&7;
	int z = opcode&7;

	if(state->addOffset)
	{
		switch(x)
		{
			case 0:
				if(z == 6)
					csZ80Disassembler_setOutput(state, "%s %s", rotTable[y], indexPlusOffset(state, displacement));
				else
					csZ80Disassembler_setOutput(state, "LD %s, %s %s", rTable(state, z), rotTable[y], indexPlusOffset(state, displacement));
			break;
			case 1:
				if(z == 6)
					csZ80Disassembler_setOutput(state, "BIT %d, %s", y, indexPlusOffset(state, displacement));
				else
					csZ80Disassembler_setOutput(state, "BIT %d, %s", y, rTable(state, z));
			break;
			case 2:
				if(z == 6)
					csZ80Disassembler_setOutput(state, "RES %d, %s", y, indexPlusOffset(state, displacement));
				else
					csZ80Disassembler_setOutput(state, "LD %s, RES %d, (%s + %d)", rTable(state, y), y, state->indexRegister, displacement);
			break;
			case 3:
				if(z == 6)
					csZ80Disassembler_setOutput(state, "SET %d, %s", y, indexPlusOffset(state, displacement));
				else
					csZ80Disassembler_setOutput(state, "SET %d, %s", y, rTable(state, z));
			break;
		}
	}
	else
	{
		switch(x)
		{
			case 0: csZ80Disassembler_setOutput(state, "%s %s", rotTable[y], rTable(state, z));		break;
			case 1: csZ80Disassembler_setOutput(state, "BIT %d, %s", y, rTable(state, z));			break;
			case 2: csZ80Disassembler_setOutput(state, "RES %d, %s", y, rTable(state, z));			break;
			case 3: csZ80Disassembler_setOutput(state, "SET %d, %s", y, rTable(state, z));			break;
		}
	}
}

static void csZ80Disassembler_disassembleEDPage(CSZ80DisassemblerState *state)
{
	uint8_t opcode = csZ80Disassembler_getByte(state);
	int x = opcode >> 6;
	int y = (opcode >> 3)&7;
	int z = opcode&7;

	switch(x)
	{
		case 0:
		case 3:	csZ80Disassembler_setOutput(state, "NOP");					break;
		case 2:	csZ80Disassembler_setOutput(state, "%s", bliTable(y, z));	break;
		case 1:
			switch(z)
			{
				case 0:
					if(y == 6)
						csZ80Disassembler_setOutput(state, "IN (C)");
					else
						csZ80Disassembler_setOutput(state, "IN %s, (C)", rTable(state, y));
				break;
				case 1:
					if(y == 6)
						csZ80Disassembler_setOutput(state, "OUT (C)");
					else
						csZ80Disassembler_setOutput(state, "OUT (C), %s", rTable(state, y));
				break;
				case 2:
					if(y&1)
						csZ80Disassembler_setOutput(state, "ADC HL, %s", rpTable(state, y >> 1));
					else
						csZ80Disassembler_setOutput(state, "SBC HL, %s", rpTable(state, y >> 1));
				break;
				case 3:
					if(y&1)
						csZ80Disassembler_setOutput(state, "LD %s, (%s)", rpTable(state, y >> 1), csZ80Disassembler_getAddress(state));
					else
						csZ80Disassembler_setOutput(state, "LD (%s), %s", csZ80Disassembler_getAddress(state), rpTable(state, y >> 1));
				break;
				case 4:	csZ80Disassembler_setOutput(state, "NEG");				break;
				case 5:
					if(y == 1)
						csZ80Disassembler_setOutput(state, "RETN");
					else
						csZ80Disassembler_setOutput(state, "RETI");
				break;
				case 6:
					csZ80Disassembler_setOutput(state, "IM %s", imTable[y]);
				break;
				case 7:
					switch(y)
					{
						case 0: csZ80Disassembler_setOutput(state, "LD I, A");	break;
						case 1: csZ80Disassembler_setOutput(state, "LD R, A");	break;
						case 2: csZ80Disassembler_setOutput(state, "LD A, I");	break;
						case 3: csZ80Disassembler_setOutput(state, "LD A, R");	break;
						case 4: csZ80Disassembler_setOutput(state, "RRD");		break;
						case 5: csZ80Disassembler_setOutput(state, "RLD");		break;
						case 6: csZ80Disassembler_setOutput(state, "NOP");		break;
						case 7: csZ80Disassembler_setOutput(state, "NOP");		break;
					}
				break;
			}
		break;
	}
}

static void csZ80Disassembler_disassembleStandardPage(CSZ80DisassemblerState *state)
{
	uint8_t opcode = csZ80Disassembler_getByte(state);
	int x = opcode >> 6;
	int y = (opcode >> 3)&7;
	int z = opcode&7;

	switch(x)
	{
		case 0:
			switch(z)
			{
				case 0:
					switch(y)
					{
						case 0: csZ80Disassembler_setOutput(state, "NOP"); 											break;
						case 1: csZ80Disassembler_setOutput(state, "EX AF, AF'"); 									break;
						case 2:	csZ80Disassembler_setOutput(state, "DJNZ %s", csZ80Disassembler_getOffset(state));	break;
						case 3:	csZ80Disassembler_setOutput(state, "JR %s", csZ80Disassembler_getOffset(state));	break;
						default:
							csZ80Disassembler_setOutput(state, "JR %s, %s", ccTable[y - 4], csZ80Disassembler_getOffset(state));
						break;
					}
				break;
				case 1:
					if(y&1)
						csZ80Disassembler_setOutput(state, "ADD HL, %s", rpTable(state, y >> 1));
					else
						csZ80Disassembler_setOutput(state, "LD %s, $%04x", rpTable(state, y >> 1), csZ80Disassembler_getShort(state));
				break;
				case 2:
					switch(y)
					{
						case 0: csZ80Disassembler_setOutput(state, "LD (BC), A"); 										break;
						case 1: csZ80Disassembler_setOutput(state, "LD A, (BC)"); 										break;
						case 2: csZ80Disassembler_setOutput(state, "LD (DE), A"); 										break;
						case 3: csZ80Disassembler_setOutput(state, "LD A, (DE)"); 										break;
						case 4: csZ80Disassembler_setOutput(state, "LD (%s), HL", csZ80Disassembler_getAddress(state));	break;
						case 5: csZ80Disassembler_setOutput(state, "LD HL, (%s)", csZ80Disassembler_getAddress(state));	break;
						case 6: csZ80Disassembler_setOutput(state, "LD (%s), A", csZ80Disassembler_getAddress(state));	break;
						case 7: csZ80Disassembler_setOutput(state, "LD A, (%s)", csZ80Disassembler_getAddress(state));	break;
					}
				break;
				case 3:
					if(y&1)
						csZ80Disassembler_setOutput(state, "DEC %s", rpTable(state, y >> 1));
					else
						csZ80Disassembler_setOutput(state, "INC %s", rpTable(state, y >> 1));
				break;
				case 4:	csZ80Disassembler_setOutput(state, "INC %s", rTable(state, y));											break;
				case 5:	csZ80Disassembler_setOutput(state, "DEC %s", rTable(state, y));											break;
				case 6:	csZ80Disassembler_setOutput(state, "LD %s, $%02x", rTable(state, y), csZ80Disassembler_getByte(state));	break;
				case 7:
					switch(y)
					{
						case 0: csZ80Disassembler_setOutput(state, "RLCA");		break;
						case 1: csZ80Disassembler_setOutput(state, "RRCA");		break;
						case 2: csZ80Disassembler_setOutput(state, "RLA");		break;
						case 3: csZ80Disassembler_setOutput(state, "RRA");		break;
						case 4: csZ80Disassembler_setOutput(state, "DAA");		break;
						case 5: csZ80Disassembler_setOutput(state, "CPL");		break;
						case 6: csZ80Disassembler_setOutput(state, "SCF");		break;
						case 7: csZ80Disassembler_setOutput(state, "CCF");		break;
					}
				break;
			}
		break;
		case 1:
		{
			if((z == 6) && (y == 6))
			{
				csZ80Disassembler_setOutput(state, "HALT");
			}
			else
			{
				csZ80Disassembler_setOutput(state, "LD %s, %s", rTable(state, y), rTable(state, z));
			}
		}
		break;
		case 2:
			csZ80Disassembler_setOutput(state, "%s %s", aluTable[y], rTable(state, z));
		break;
		case 3:
			switch(z)
			{
				case 0:	csZ80Disassembler_setOutput(state, "RET %s", ccTable[y]);	break;
				case 1:
					if(y&1)
					{
						switch(y >> 1)
						{
							case 0: csZ80Disassembler_setOutput(state, "RET");								break;
							case 1: csZ80Disassembler_setOutput(state, "EXX");								break;
							case 2: csZ80Disassembler_setOutput(state, "JP %s", state->indexRegister);		break;
							case 3: csZ80Disassembler_setOutput(state, "LD SP, %s", state->indexRegister);	break;
						}
					}
					else
					{
						csZ80Disassembler_setOutput(state, "POP %s", rp2Table(state, y >> 1));
					}
				break;
				case 2:
					csZ80Disassembler_setOutput(state, "JP %s, %0s", ccTable[y], csZ80Disassembler_getAddress(state));
				break;
				case 3:
					switch(y)
					{
						case 0: csZ80Disassembler_setOutput(state, "JP %0s", csZ80Disassembler_getAddress(state)); 		break;
						case 1: csZ80Disassembler_disassembleCBPage(state);												break;
						case 2: csZ80Disassembler_setOutput(state, "OUT ($%02x), A", csZ80Disassembler_getByte(state));	break;
						case 3: csZ80Disassembler_setOutput(state, "IN A, ($%02x)", csZ80Disassembler_getByte(state));	break;
						case 4: csZ80Disassembler_setOutput(state, "EX (SP), HL");										break;
						case 5: csZ80Disassembler_setOutput(state, "EX DE, HL");										break;
						case 6: csZ80Disassembler_setOutput(state, "DI");												break;
						case 7: csZ80Disassembler_setOutput(state, "EI");												break;
					}
				break;
				case 4:
					csZ80Disassembler_setOutput(state, "CALL %s, %s", ccTable[y], csZ80Disassembler_getAddress(state));
				break;
				case 5:
					if(y&1)
					{
						switch(y >> 1)
						{
							case 0: csZ80Disassembler_setOutput(state, "CALL %s", csZ80Disassembler_getAddress(state));	break;
							case 1: 
								state->indexRegister = "IX";
								state->addOffset = true;
								csZ80Disassembler_disassembleStandardPage(state);
							break;
							case 2: csZ80Disassembler_disassembleEDPage(state);											break;
							case 3:
								state->indexRegister = "IY";
								state->addOffset = true;
								csZ80Disassembler_disassembleStandardPage(state);
							break;
						}
					}
					else
					{
						csZ80Disassembler_setOutput(state, "PUSH %s", rp2Table(state, y >> 1));
					}
				break;
				case 6:
					csZ80Disassembler_setOutput(state, "%s $%02x", aluTable[y], csZ80Disassembler_getByte(state));
				break;
				case 7:
					csZ80Disassembler_setOutput(state, "RST %02xh", y << 3);
				break;
			}
		break;
	}
}

void *csZ80Disassembler_createDisassembly(uint8_t *sourceData, uint16_t startAddress, uint16_t length)
{
	CSZ80DisassemblerState state;
	state.currentOffset = 0;
	state.length = length;
	state.output = csArray_create(true);
	state.releasePool = csArray_create(false);
	state.sourceData = sourceData;
	state.startAddress = startAddress;

	while(state.currentOffset < state.length)
	{
		// start a new opcode (which allows us to store the start address)
		csZ80Disassembler_beginOpcode(&state);

		// begin disassembly of the opcode
		state.indexRegister = "HL";
		state.addOffset = false;
		csZ80Disassembler_disassembleStandardPage(&state);

		// clean up the release pool
		unsigned int numberOfThingsToFree;
		void **thingsToFree = csArray_getCArray(state.releasePool, &numberOfThingsToFree);
		while(numberOfThingsToFree--)
			free(thingsToFree[numberOfThingsToFree]);
		csArray_removeAllObjects(state.releasePool);
	}

	csObject_release(state.releasePool);

	return state.output;
}
