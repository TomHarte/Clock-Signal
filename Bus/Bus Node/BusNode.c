//
//  BusNode.c
//  Clock Signal
//
//  Created by Thomas Harte on 29/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "BusNode.h"
#include "BusNodeInternals.h"
#include "ComponentInternals.h"

void *csBusNode_createComponent(void *node)
{
	return ((CSBusNode *)node)->createComponent(node);
}

CSBusCondition csBusNode_getCondition(void *node)
{
	void *component = csBusNode_createComponent(node);
	CSBusCondition condition = ((CSBusComponent *)component)->condition;
	csObject_release(component);
	return condition;
}

void csBusNode_addComponent(void *node, void *component)
{
	((CSBusNode *)node)->addComponent(node, component);
}

void csBusNode_addChildNode(void *node, void *childNode)
{
	void *component = csBusNode_createComponent(childNode);
	csBusNode_addComponent(node, component);
	csObject_release(component);
}

int csBusNode_getNumberOfChildren(void *opaqueNode)
{
	CSBusNode *node = (CSBusNode *)opaqueNode;
	if(node->getNumberOfChildren) return node->getNumberOfChildren(node);
	return 1;
}

void csBusNode_init(void *node)
{
	csObject_init(node);
}
