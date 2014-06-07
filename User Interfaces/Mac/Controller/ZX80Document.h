//
//  ZX80Document.h
//  Clock Signal
//
//  Created by Thomas Harte on 08/09/2011.
//  Copyright (c) 2011 Thomas Harte. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define kZX80DocumentNumAudioBuffers	3
#define kZX80DocumentAudioStreamLength	16384
#define kZX80DocumentAudioBufferLength	2048

@interface ZX80Document : NSDocument 

@end
