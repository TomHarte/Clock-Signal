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
@property (nonatomic, weak) IBOutlet NSTextField *aRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *fRegisterField;

@property (nonatomic, weak) IBOutlet NSTextField *bRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *cRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *dRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *eRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *hRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *lRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *ixRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *iyRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *spRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *pcRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *afDashRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *bcDashRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *deDashRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *hlDashRegisterField;

@property (nonatomic, weak) IBOutlet NSTextField *iff1FlagField;
@property (nonatomic, weak) IBOutlet NSTextField *iff2FlagField;
@property (nonatomic, weak) IBOutlet NSTextField *interuptModeField;
@property (nonatomic, weak) IBOutlet NSTextField *rRegisterField;
@property (nonatomic, weak) IBOutlet NSTextField *iRegisterField;

@property (nonatomic, weak) IBOutlet NSTextField *addressField;

@property (nonatomic, weak) IBOutlet NSTextField *cyclesRunForField;

@property (nonatomic, weak) IBOutlet NSButton *runButton;
@property (nonatomic, weak) IBOutlet NSButton *pauseButton;

@property (nonatomic, weak) IBOutlet NSButton *showBusButton;
@property (nonatomic, weak) IBOutlet NSPanel *busPanel;

@property (nonatomic, weak) IBOutlet CSLineGraph *clockLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *m1Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *mReqLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *ioReqLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *refreshLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *readLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *writeLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *waitLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *haltLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *irqLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *nmiLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *resetLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *busReqLine;
@property (nonatomic, weak) IBOutlet CSLineGraph *busAckLine;

@property (nonatomic, weak) IBOutlet CSLineGraph *d7Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d6Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d5Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d4Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d3Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d2Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d1Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *d0Line;

@property (nonatomic, weak) IBOutlet CSLineGraph *a15Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a14Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a13Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a12Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a11Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a10Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a9Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a8Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a7Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a6Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a5Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a4Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a3Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a2Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a1Line;
@property (nonatomic, weak) IBOutlet CSLineGraph *a0Line;

@end

@implementation Z80DebugInterface
{
	unsigned int _lastInternalTime;
	unsigned int _lastBusTime;

	NSArray *_allLines;
}


- (void)awakeFromNib
{
	_allLines =
		@[
			self.clockLine,		self.m1Line,		self.mReqLine,		self.ioReqLine,
			self.refreshLine,	self.readLine,		self.writeLine,		self.waitLine,
			self.haltLine,		self.irqLine,		self.nmiLine,		self.resetLine,
			self.busReqLine,	self.busAckLine,	self.d7Line,		self.d6Line,
			self.d5Line,		self.d4Line,		self.d3Line,		self.d2Line,
			self.d1Line,		self.d0Line,		self.a15Line,		self.a14Line,
			self.a13Line,		self.a12Line,		self.a11Line,		self.a10Line,
			self.a9Line,		self.a8Line,		self.a7Line,		self.a6Line,
			self.a5Line,		self.a4Line,		self.a3Line,		self.a2Line,
			self.a1Line,		self.a0Line];
}

- (IBAction)runForOneInstruction:(id)sender
{
	[self.delegate debugInterfaceRunForOneInstruction:self];
}

- (IBAction)runForHalfACycle:(id)sender
{
	[self.delegate debugInterfaceRunForHalfACycle:self];
}

- (void)show
{
	[self.busPanel setIsVisible:YES];
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
	NSScanner *scanner = [[NSScanner alloc] initWithString:self.addressField.stringValue];
	unsigned int address;
	[scanner scanHexInt:&address];

	[self.delegate debugInterface:self runUntilAddress:(uint16_t)address];
}

- (IBAction)run:(id)sender
{
	[self.delegate debugInterfaceRun:self];
}

- (IBAction)pause:(id)sender
{
	[self.delegate debugInterfacePause:self];
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
			[NSString stringWithFormat:@"%02x", llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], value)]
			toField:field];
}

- (void)set16BitValue:(LLZ80MonitorValue)value toField:(NSTextField *)field
{
	[self
		setString:
			[NSString stringWithFormat:@"%04x", llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], value)]
			toField:field];
}

- (void)set16BitValueLow:(LLZ80MonitorValue)lowValue high:(LLZ80MonitorValue)highValue toField:(NSTextField *)field
{
	[self
		setString:
			[NSString stringWithFormat:@"%04x",
				llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], lowValue) |
				(llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], highValue) << 8)
			]
			toField:field];
}

- (void)updateBus
{
	unsigned int z80Time = llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], LLZ80MonitorValueHalfCyclesToDate);

	if(z80Time == _lastBusTime) return;
	_lastBusTime = z80Time;

	uint64_t busState = llz80_monitor_getBusLineState([self.delegate z80ForDebugInterface:self]);

	[self.clockLine		pushBit:(busState&CSBusStandardClockLine) ? 1 : 0];
	[self.m1Line		pushBit:(busState&LLZ80SignalMachineCycleOne) ? 1 : 0];
	[self.mReqLine		pushBit:(busState&LLZ80SignalMemoryRequest) ? 1 : 0];
	[self.ioReqLine		pushBit:(busState&LLZ80SignalInputOutputRequest) ? 1 : 0];
	[self.refreshLine	pushBit:(busState&LLZ80SignalRefresh) ? 1 : 0];
	[self.readLine		pushBit:(busState&LLZ80SignalRead) ? 1 : 0];
	[self.writeLine		pushBit:(busState&LLZ80SignalWrite) ? 1 : 0];
	[self.waitLine		pushBit:(busState&LLZ80SignalWait) ? 1 : 0];
	[self.haltLine		pushBit:(busState&LLZ80SignalHalt) ? 1 : 0];
	[self.irqLine		pushBit:(busState&LLZ80SignalInterruptRequest) ? 1 : 0];
	[self.nmiLine		pushBit:(busState&LLZ80SignalNonMaskableInterruptRequest) ? 1 : 0];
	[self.resetLine		pushBit:(busState&LLZ80SignalReset) ? 1 : 0];
	[self.busReqLine	pushBit:(busState&LLZ80SignalBusRequest) ? 1 : 0];
	[self.busAckLine	pushBit:(busState&LLZ80SignalBusAcknowledge) ? 1 : 0];

	uint8_t dataValue = (uint8_t)(busState >> CSBusStandardDataShift);
	for(CSLineGraph *graph in @[self.d7Line, self.d6Line, self.d5Line, self.d4Line, self.d3Line, self.d2Line, self.d1Line, self.d0Line])
	{
		[graph pushBit:(dataValue & 0x80) ? 1 : 0];
		dataValue <<= 1;
	}

	uint16_t addressValue = (uint16_t)(busState >> CSBusStandardAddressShift);
	for(CSLineGraph *graph in @[self.a15Line, self.a14Line, self.a13Line, self.a12Line, self.a11Line, self.a10Line, self.a9Line, self.a8Line, self.a7Line, self.a6Line, self.a5Line, self.a4Line, self.a3Line, self.a2Line, self.a1Line, self.a0Line])
	{
		[graph pushBit:(addressValue & 0x8000) ? 1 : 0];
		addressValue <<= 1;
	}
}

- (void)refresh
{
	[self set8BitValue:LLZ80MonitorValueARegister toField:self.aRegisterField];

	uint8_t flagRegister = (uint8_t)llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], LLZ80MonitorValueFRegister);
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
		toField:self.fRegisterField];

	[self set8BitValue:LLZ80MonitorValueBRegister toField:self.bRegisterField];
	[self set8BitValue:LLZ80MonitorValueCRegister toField:self.cRegisterField];
	[self set8BitValue:LLZ80MonitorValueDRegister toField:self.dRegisterField];
	[self set8BitValue:LLZ80MonitorValueERegister toField:self.eRegisterField];
	[self set8BitValue:LLZ80MonitorValueHRegister toField:self.hRegisterField];
	[self set8BitValue:LLZ80MonitorValueLRegister toField:self.lRegisterField];

	[self set8BitValue:LLZ80MonitorValueIRegister toField:self.iRegisterField];
	[self set8BitValue:LLZ80MonitorValueRRegister toField:self.rRegisterField];

	[self set16BitValue:LLZ80MonitorValueIXRegister toField:self.ixRegisterField];
	[self set16BitValue:LLZ80MonitorValueIYRegister toField:self.iyRegisterField];
	[self set16BitValue:LLZ80MonitorValuePCRegister toField:self.pcRegisterField];
	[self set16BitValue:LLZ80MonitorValueSPRegister toField:self.spRegisterField];

	[self set16BitValueLow:LLZ80MonitorValueFDashRegister high:LLZ80MonitorValueADashRegister toField:self.afDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueCDashRegister high:LLZ80MonitorValueBDashRegister toField:self.bcDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueEDashRegister high:LLZ80MonitorValueDDashRegister toField:self.deDashRegisterField];
	[self set16BitValueLow:LLZ80MonitorValueLDashRegister high:LLZ80MonitorValueHDashRegister toField:self.hlDashRegisterField];

	unsigned int newInternalTime = llz80_monitor_getInternalValue([self.delegate z80ForDebugInterface:self], LLZ80MonitorValueHalfCyclesToDate);
	unsigned int timeElapsed = newInternalTime - _lastInternalTime;
	_lastInternalTime = newInternalTime;
	self.cyclesRunForField.stringValue = [NSString stringWithFormat:@"%0.1f", (float)timeElapsed * 0.5f];

	[self updateBus];
	[_allLines makeObjectsPerformSelector:@selector(setNeedsDisplay:) withObject:@YES];
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
	_isRunning = newIsRunning;

	if(_isRunning)
	{
		self.runButton.enabled = NO;
		self.pauseButton.enabled = YES;
	}
	else
	{
		self.runButton.enabled = YES;
		self.pauseButton.enabled = NO;
	}
}

+ (id)debugInterface
{
	id returnObject = [[[self class] alloc] init];
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
