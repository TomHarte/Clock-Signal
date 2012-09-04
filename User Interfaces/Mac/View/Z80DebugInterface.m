//
//  Z80DebugInterface.m
//  Clock Signal
//
//  Created by Thomas Harte on 20/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#import "Z80DebugInterface.h"
#import "LineGraph.h"

#include "Z80.h"
#include "StandardBusLines.h"

@interface Z80DebugInterface ()
@property (nonatomic, assign) unsigned int lastInternalTime;
@property (nonatomic, retain) NSMutableArray *memoryCommentLines;
@property (nonatomic, retain) NSArray *allLines;
@end


@implementation Z80DebugInterface

@synthesize aRegisterField, fRegisterField;
@synthesize bRegisterField, cRegisterField, dRegisterField, eRegisterField, hRegisterField, lRegisterField;
@synthesize ixRegisterField, iyRegisterField, spRegisterField, pcRegisterField;
@synthesize afDashRegisterField, bcDashRegisterField, deDashRegisterField, hlDashRegisterField;
@synthesize iff1FlagField, iff2FlagField, interuptModeField;
@synthesize rRegisterField, iRegisterField;
@synthesize addressField;
@synthesize cyclesRunForField;
@synthesize lastInternalTime;
@synthesize delegate;
@synthesize memoryCommentLines;
@synthesize isRunning;
@synthesize runButton, pauseButton;
@synthesize busPanel, showBusButton;

@synthesize clockLine;
@synthesize m1Line;
@synthesize mReqLine;
@synthesize ioReqLine;
@synthesize refreshLine;
@synthesize readLine;
@synthesize writeLine;
@synthesize waitLine;
@synthesize haltLine;
@synthesize irqLine;
@synthesize nmiLine;
@synthesize resetLine;
@synthesize busReqLine;
@synthesize busAckLine;

@synthesize d7Line, d6Line, d5Line, d4Line;
@synthesize d3Line, d2Line, d1Line, d0Line;

@synthesize a15Line, a14Line, a13Line, a12Line;
@synthesize a11Line, a10Line, a9Line, a8Line;
@synthesize a7Line, a6Line, a5Line, a4Line;
@synthesize a3Line, a2Line, a1Line, a0Line;

@synthesize allLines;

- (void)dealloc
{
	self.memoryCommentLines = nil;
	self.allLines = nil;
	[super dealloc];
}

- (void)awakeFromNib
{
	self.allLines = [NSArray arrayWithObjects:
		clockLine, m1Line, mReqLine, ioReqLine, refreshLine, readLine, writeLine, waitLine, haltLine,
		irqLine, nmiLine, resetLine, busReqLine, busAckLine,
		d7Line, d6Line, d5Line, d4Line, d3Line, d2Line, d1Line, d0Line,
		a15Line, a14Line, a13Line, a12Line, a11Line, a10Line, a9Line, a8Line,
		a7Line, a6Line, a5Line, a4Line, a3Line, a2Line, a1Line, a0Line,
		nil];
}

- (IBAction)runForOneInstruction:(id)sender
{
	[delegate debugInterfaceRunForOneInstruction:self];
}

- (IBAction)runForHalfACycle:(id)sender
{
	[delegate debugInterfaceRunForHalfACycle:self];
}

- (void)show
{
	[busPanel setIsVisible:YES];
}

/*- (IBAction)showBus:(id)sender
{
	if(busPanel.isVisible)
	{
		[showBusButton setStringValue:@"Hide Bus"];
		[busPanel setIsVisible:NO];
	}
	else
	{
		[showBusButton setStringValue:@"Show Bus"];
		[busPanel setIsVisible:YES];
	}
}*/

- (IBAction)runUntilAddress:(id)sender
{
	NSScanner *scanner = [[NSScanner alloc] initWithString:addressField.stringValue];
	unsigned int address;
	[scanner scanHexInt:&address];
	[scanner release];

	[delegate debugInterface:self runUntilAddress:(uint16_t)address];
}

- (IBAction)run:(id)sender
{
	[delegate debugInterfaceRun:self];
}

- (IBAction)pause:(id)sender
{
	[delegate debugInterfacePause:self];
}

- (void)setString:(NSString *)string toField:(NSTextField *)field
{
	NSString *oldString = field.stringValue;

	if([oldString isEqualToString:string])
	{
		field.textColor = [NSColor blackColor];
		return;
	}

	field.stringValue = string;
	field.textColor = [NSColor redColor];
}

- (void)set8BitValue:(LLZ80MonitorValue)value toField:(NSTextField *)field
{
	[self
		setString:
			[NSString stringWithFormat:@"%02x", llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, value)]
			toField:field];
}

- (void)set16BitValue:(LLZ80MonitorValue)value toField:(NSTextField *)field
{
	[self
		setString:
			[NSString stringWithFormat:@"%04x", llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, value)]
			toField:field];
}

- (void)set16BitValueLow:(LLZ80MonitorValue)lowValue high:(LLZ80MonitorValue)highValue toField:(NSTextField *)field
{
	[self
		setString:
			[NSString stringWithFormat:@"%04x",
				llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, lowValue) |
				(llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, highValue) << 8)
			]
			toField:field];
}

- (void)updateBus
{
	unsigned int z80Time = llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, LLZ80MonitorValueHalfCyclesToDate);
	
	if(z80Time == lastBusTime) return;
	lastBusTime = z80Time;

	uint64_t busState = llz80_monitor_getBusLineState(delegate.z80ForDebugInterface);

	[clockLine pushBit:(busState&CSBusStandardClockLine) ? 1 : 0];
	[m1Line pushBit:(busState&LLZ80SignalMachineCycleOne) ? 1 : 0];
	[mReqLine pushBit:(busState&LLZ80SignalMemoryRequest) ? 1 : 0];
	[ioReqLine pushBit:(busState&LLZ80SignalInputOutputRequest) ? 1 : 0];
	[refreshLine pushBit:(busState&LLZ80SignalRefresh) ? 1 : 0];
	[readLine pushBit:(busState&LLZ80SignalRead) ? 1 : 0];
	[writeLine pushBit:(busState&LLZ80SignalWrite) ? 1 : 0];
	[waitLine pushBit:(busState&LLZ80SignalWait) ? 1 : 0];
	[haltLine pushBit:(busState&LLZ80SignalHalt) ? 1 : 0];
	[irqLine pushBit:(busState&LLZ80SignalInterruptRequest) ? 1 : 0];
	[nmiLine pushBit:(busState&LLZ80SignalNonMaskableInterruptRequest) ? 1 : 0];
	[resetLine pushBit:(busState&LLZ80SignalReset) ? 1 : 0];
	[busReqLine pushBit:(busState&LLZ80SignalBusRequest) ? 1 : 0];
	[busAckLine pushBit:(busState&LLZ80SignalBusAcknowledge) ? 1 : 0];

	uint8_t dataValue = (uint8_t)(busState >> CSBusStandardDataShift);
	for(CSLineGraph *graph in [NSArray arrayWithObjects:d7Line, d6Line, d5Line, d4Line, d3Line, d2Line, d1Line, d0Line, nil])
	{
		[graph pushBit:(dataValue & 0x80) ? 1 : 0];
		dataValue <<= 1;
	}

	uint16_t addressValue = (uint16_t)(busState >> CSBusStandardAddressShift);
	for(CSLineGraph *graph in [NSArray arrayWithObjects:a15Line, a14Line, a13Line, a12Line, a11Line, a10Line, a9Line, a8Line, a7Line, a6Line, a5Line, a4Line, a3Line, a2Line, a1Line, a0Line, nil])
	{
		[graph pushBit:(addressValue & 0x8000) ? 1 : 0];
		addressValue <<= 1;
	}
}

- (void)refresh
{
	[self set8BitValue:LLZ80MonitorValueARegister toField:aRegisterField];

	uint8_t flagRegister = (uint8_t)llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, LLZ80MonitorValueFRegister);
	[self setString:
	 	[NSString stringWithFormat:@"%c%c%c%c%c%c%c%c",
		(flagRegister&0x80) ? 'S' : '-',
		(flagRegister&0x40) ? 'Z' : '-',
		(flagRegister&0x20) ? '5' : '-',
		(flagRegister&0x10) ? 'H' : '-',
		(flagRegister&0x08) ? '3' : '-',
		(flagRegister&0x04) ? 'V' : '-',
		(flagRegister&0x02) ? 'S' : '-',
		(flagRegister&0x01) ? 'C' : '-'
		]
		toField:fRegisterField];

	[self set8BitValue:LLZ80MonitorValueBRegister toField:bRegisterField];
	[self set8BitValue:LLZ80MonitorValueCRegister toField:cRegisterField];
	[self set8BitValue:LLZ80MonitorValueDRegister toField:dRegisterField];
	[self set8BitValue:LLZ80MonitorValueERegister toField:eRegisterField];
	[self set8BitValue:LLZ80MonitorValueHRegister toField:hRegisterField];
	[self set8BitValue:LLZ80MonitorValueLRegister toField:lRegisterField];

	[self set8BitValue:LLZ80MonitorValueIRegister toField:iRegisterField];
	[self set8BitValue:LLZ80MonitorValueRRegister toField:rRegisterField];

	[self set16BitValue:LLZ80MonitorValueIXRegister toField:ixRegisterField];
	[self set16BitValue:LLZ80MonitorValueIYRegister toField:iyRegisterField];
	[self set16BitValue:LLZ80MonitorValuePCRegister toField:pcRegisterField];
	[self set16BitValue:LLZ80MonitorValueSPRegister toField:spRegisterField];

	[self set16BitValueLow:LLZ80MonitorValueFDashRegister high:LLZ80MonitorValueADashRegister toField:afDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueCDashRegister high:LLZ80MonitorValueBDashRegister toField:bcDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueEDashRegister high:LLZ80MonitorValueDDashRegister toField:deDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueLDashRegister high:LLZ80MonitorValueHDashRegister toField:hlDashRegisterField];

	unsigned int newInternalTime = llz80_monitor_getInternalValue(delegate.z80ForDebugInterface, LLZ80MonitorValueHalfCyclesToDate);
	unsigned int timeElapsed = newInternalTime - lastInternalTime;
	lastInternalTime = newInternalTime;
	cyclesRunForField.stringValue = [NSString stringWithFormat:@"%0.1f", (float)timeElapsed * 0.5f];

	[self updateBus];
	[self.allLines makeObjectsPerformSelector:@selector(setNeedsDisplay:) withObject:[NSNumber numberWithBool:YES]];
//	NSMutableString *totalString = [NSMutableString string];
//	for(NSString *line in memoryCommentLines)
//	{
//		[totalString appendFormat:@"%@\n", line];
//	}
//	memoryCommentsField.string = totalString;
}

- (void)addComment:(NSString *)comment
{
//	[memoryCommentLines addObject:line];
//	if([memoryCommentLines count] > 20)
//	{
//		[memoryCommentLines removeObjectAtIndex:0];
//	}
}

- (void)setIsRunning:(BOOL)newIsRunning
{
	isRunning = newIsRunning;

	if(isRunning)
	{
		runButton.enabled = NO;
		pauseButton.enabled = YES;
	}
	else
	{
		runButton.enabled = YES;
		pauseButton.enabled = NO;
	}
}

+ (id)debugInterface
{
	id returnObject = [[[[self class] alloc] init] autorelease];
	[NSBundle loadNibNamed:@"Z80DebugInterface" owner:returnObject];
	return returnObject;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
	return 15;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
	return @"Hat";
}

@end
