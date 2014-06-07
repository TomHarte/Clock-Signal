//
//  Z80DebugDrawer.h
//  Clock Signal
//
//  Created by Thomas Harte on 20/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#import <Foundation/Foundation.h>

@class Z80DebugInterface;

@protocol Z80DebugDrawerDelegate <NSObject>

- (void)debugInterfaceRun:(Z80DebugInterface *)drawer;
- (void)debugInterfacePause:(Z80DebugInterface *)drawer;
- (void)debugInterface:(Z80DebugInterface *)drawer runUntilAddress:(uint16_t)address;
- (void)debugInterfaceRunForOneInstruction:(Z80DebugInterface *)drawer;
- (void)debugInterfaceRunForHalfACycle:(Z80DebugInterface *)drawer;
- (void *)z80ForDebugInterface;

@end

@class CSLineGraph;

@interface Z80DebugInterface : NSObject <NSTableViewDataSource>
{
	// for the 32bit users out there ...
	@private
		id <Z80DebugDrawerDelegate> __unsafe_unretained delegate;
		NSTextField *__weak aRegisterField, *__weak fRegisterField;

		NSTextField	*__weak bRegisterField,		*__weak cRegisterField,	*__weak dRegisterField,		*__weak eRegisterField;
		NSTextField *__weak hRegisterField,		*__weak lRegisterField,	*__weak ixRegisterField,		*__weak iyRegisterField;
		NSTextField *__weak spRegisterField,		*__weak pcRegisterField,	*__weak afDashRegisterField,	*__weak bcDashRegisterField;
		NSTextField *__weak deDashRegisterField,	*__weak hlDashRegisterField;

		NSTextField *__weak iff1FlagField,			*__weak iff2FlagField,		*__weak interuptModeField,		*__weak rRegisterField;
		NSTextField *__weak iRegisterField;

		NSTextField *__weak addressField;
		NSTextField *__weak cyclesRunForField;

		NSButton *__weak runButton,				*__weak pauseButton;

		BOOL isRunning;
		
		unsigned int lastInternalTime;
		NSMutableArray *memoryCommentLines;

		NSPanel *__weak busPanel;
		NSButton *__weak showBusButton;

		unsigned int lastBusTime;

		CSLineGraph *__weak clockLine, *__weak m1Line, *__weak mReqLine, *__weak ioReqLine;
		CSLineGraph *__weak refreshLine, *__weak readLine, *__weak writeLine, *__weak waitLine;
		CSLineGraph *__weak haltLine, *__weak irqLine, *__weak nmiLine, *__weak resetLine;
		CSLineGraph *__weak busReqLine, *__weak busAckLine;

		CSLineGraph *__weak d7Line, *__weak d6Line, *__weak d5Line, *__weak d4Line;
		CSLineGraph *__weak d3Line, *__weak d2Line, *__weak d1Line, *__weak d0Line;

		CSLineGraph *__weak a15Line, *__weak a14Line, *__weak a13Line, *__weak a12Line;
		CSLineGraph *__weak a11Line, *__weak a10Line, *__weak a9Line, *__weak a8Line;
		CSLineGraph *__weak a7Line, *__weak a6Line, *__weak a5Line, *__weak a4Line;
		CSLineGraph *__weak a3Line, *__weak a2Line, *__weak a1Line, *__weak a0Line;
		
		NSArray *allLines;
}

+ (id)debugInterface;

- (void)refresh;
- (void)updateBus;
- (void)addComment:(NSString *)comment;

- (void)show;

@property (nonatomic, unsafe_unretained) id <Z80DebugDrawerDelegate> delegate;

/*

	Various hooks and bits for Interface Builder; don't use these

*/
- (IBAction)runForOneInstruction:(id)sender;
- (IBAction)runForHalfACycle:(id)sender;
- (IBAction)runUntilAddress:(id)sender;
- (IBAction)run:(id)sender;
- (IBAction)pause:(id)sender;

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

@property (nonatomic, assign) BOOL isRunning;

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
