//
//  Z80DebugDrawer.h
//  Clock Signal
//
//  Created by Thomas Harte on 20/09/2011.
//  Copyright 2011 acrossair. All rights reserved.
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
		id <Z80DebugDrawerDelegate> delegate;
		NSTextField *aRegisterField, *fRegisterField;

		NSTextField	*bRegisterField,		*cRegisterField,	*dRegisterField,		*eRegisterField;
		NSTextField *hRegisterField,		*lRegisterField,	*ixRegisterField,		*iyRegisterField;
		NSTextField *spRegisterField,		*pcRegisterField,	*afDashRegisterField,	*bcDashRegisterField;
		NSTextField *deDashRegisterField,	*hlDashRegisterField;

		NSTextField *iff1FlagField,			*iff2FlagField,		*interuptModeField,		*rRegisterField;
		NSTextField *iRegisterField;

		NSTextField *addressField;
		NSTextField *cyclesRunForField;

		NSButton *runButton,				*pauseButton;

		BOOL isRunning;
		
		unsigned int lastInternalTime;
		NSMutableArray *memoryCommentLines;

		NSPanel *busPanel;
		NSButton *showBusButton;

		unsigned int lastBusTime;

		CSLineGraph *clockLine, *m1Line, *mReqLine, *ioReqLine;
		CSLineGraph *refreshLine, *readLine, *writeLine, *waitLine;
		CSLineGraph *haltLine, *irqLine, *nmiLine, *resetLine;
		CSLineGraph *busReqLine, *busAckLine;

		CSLineGraph *d7Line, *d6Line, *d5Line, *d4Line;
		CSLineGraph *d3Line, *d2Line, *d1Line, *d0Line;

		CSLineGraph *a15Line, *a14Line, *a13Line, *a12Line;
		CSLineGraph *a11Line, *a10Line, *a9Line, *a8Line;
		CSLineGraph *a7Line, *a6Line, *a5Line, *a4Line;
		CSLineGraph *a3Line, *a2Line, *a1Line, *a0Line;
		
		NSArray *allLines;
}

+ (id)debugInterface;

- (void)refresh;
- (void)updateBus;
- (void)addComment:(NSString *)comment;

- (void)show;

@property (nonatomic, assign) id <Z80DebugDrawerDelegate> delegate;

/*

	Various hooks and bits for Interface Builder; don't use these

*/
- (IBAction)runForOneInstruction:(id)sender;
- (IBAction)runForHalfACycle:(id)sender;
- (IBAction)showBus:(id)sender;
- (IBAction)runUntilAddress:(id)sender;
- (IBAction)run:(id)sender;
- (IBAction)pause:(id)sender;

@property (nonatomic, assign) IBOutlet NSTextField *aRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *fRegisterField;

@property (nonatomic, assign) IBOutlet NSTextField *bRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *cRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *dRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *eRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *hRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *lRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *ixRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *iyRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *spRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *pcRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *afDashRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *bcDashRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *deDashRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *hlDashRegisterField;

@property (nonatomic, assign) IBOutlet NSTextField *iff1FlagField;
@property (nonatomic, assign) IBOutlet NSTextField *iff2FlagField;
@property (nonatomic, assign) IBOutlet NSTextField *interuptModeField;
@property (nonatomic, assign) IBOutlet NSTextField *rRegisterField;
@property (nonatomic, assign) IBOutlet NSTextField *iRegisterField;

@property (nonatomic, assign) IBOutlet NSTextField *addressField;

@property (nonatomic, assign) IBOutlet NSTextField *cyclesRunForField;

@property (nonatomic, assign) IBOutlet NSButton *runButton;
@property (nonatomic, assign) IBOutlet NSButton *pauseButton;

@property (nonatomic, assign) IBOutlet NSButton *showBusButton;
@property (nonatomic, assign) IBOutlet NSPanel *busPanel;

@property (nonatomic, assign) BOOL isRunning;

@property (nonatomic, assign) IBOutlet CSLineGraph *clockLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *m1Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *mReqLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *ioReqLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *refreshLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *readLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *writeLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *waitLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *haltLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *irqLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *nmiLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *resetLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *busReqLine;
@property (nonatomic, assign) IBOutlet CSLineGraph *busAckLine;

@property (nonatomic, assign) IBOutlet CSLineGraph *d7Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d6Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d5Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d4Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d3Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d2Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d1Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *d0Line;

@property (nonatomic, assign) IBOutlet CSLineGraph *a15Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a14Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a13Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a12Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a11Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a10Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a9Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a8Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a7Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a6Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a5Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a4Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a3Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a2Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a1Line;
@property (nonatomic, assign) IBOutlet CSLineGraph *a0Line;

@end
