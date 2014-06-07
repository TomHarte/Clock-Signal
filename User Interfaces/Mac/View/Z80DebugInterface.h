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

+ (id)debugInterface;

- (void)refresh;
- (void)updateBus;
- (void)addComment:(NSString *)comment;

- (void)show;

@property (nonatomic, weak) id <Z80DebugDrawerDelegate> delegate;
@property (nonatomic, assign) BOOL isRunning;

@end
