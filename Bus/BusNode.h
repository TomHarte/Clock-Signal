//
//  BusNode.h
//  Clock Signal
//
//  Created by Thomas Harte on 29/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#ifndef ClockSignal_BusNode_h
#define ClockSignal_BusNode_h

#include "BusState.h"

// this is the standard interface for adding things to
// a bus node; these are implemented by the specific types
// of node

// methods for adding children; trees should be built from
// the bottom up and at present this is a strictly binary
// node so attempting to add more than two children has
// an undefined result. There's no real algorithmic reason
// why these nodes couldn't have arbitrarily more children,
// other than that it'd increase complexity and therefore
// the probability of error

// adding a child node is as simple as linking them up; they
// know how to organise themselves
void csBusNode_addChildNode(void *node, void *childNode);

// a getter for the number of children below this node
int csBusNode_getNumberOfChildren(void *node);

// adding a component means indicating which signals the
// component responds to.
//
// The test applied by a node is:
//
//	(incomingChangedLines & linesTheComponentObserves) ||
//	((currentBusValues & linesTheComponentWantsASpecificValueFor) == nominatedSpecificValues)
//
// Or any degenerate case springing from that. Most of the
// methods available for adding components below are short-cuts
// to the most useful of the degenerate cases.
void csBusNode_addComponent(
	void *node,
	void *component);

// a means to get a component from a bus node; note that
// it returns an owning reference
void *csBusNode_createComponent(void *node);

// a means to get just the condition attached to this node
CSBusCondition csBusNode_getCondition(void *node);

// TEMPORARY
void csBusNode_print(void *node);

#endif
