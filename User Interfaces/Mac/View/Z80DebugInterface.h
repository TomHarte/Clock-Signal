//
//  Z80DebugDrawer.h
//  Clock Signal
//
//  Created by Thomas Harte on 20/09/2011.
//  Copyright 2011 Thomas Harte. All rights reserved.
//

#import <Foundation/Foundation.h>

@class Z80DebugInterface;

@protocol Z80DebugInterfaceDelegate <NSObject>

- (void)debugInterfaceRun:(Z80DebugInterface *)debugInterface;
- (void)debugInterfacePause:(Z80DebugInterface *)debugInterface;
- (void)debugInterface:(Z80DebugInterface *)debugInterface runUntilAddress:(uint16_t)address;
- (void)debugInterfaceRunForOneInstruction:(Z80DebugInterface *)debugInterface;
- (void)debugInterfaceRunForHalfACycle:(Z80DebugInterface *)debugInterface;
- (void *)z80ForDebugInterface:(Z80DebugInterface *)debugInterface;

@end

@class CSLineGraph;

@interface Z80DebugInterface : NSObject <NSTableViewDataSource>

+ (id)debugInterface;

- (void)refresh;
- (void)updateBus;
- (void)addComment:(NSString *)comment;

- (void)show;

@property (nonatomic, weak) id <Z80DebugInterfaceDelegate> delegate;
@property (nonatomic, assign) BOOL isRunning;

@end
