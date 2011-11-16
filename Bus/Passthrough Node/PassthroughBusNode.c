//
//  PassthroughBusNode.c
//  Clock Signal
//
//  Created by Thomas Harte on 30/10/2011.
//  Copyright 2011 acrossair. All rights reserved.
//

#include "PassthroughBusNode.h"
#include "BusNodeInternals.h"
#include "ComponentInternals.h"
#include <stdlib.h>


static void csPassthroughNode_addComponent(void *opaqueNode, void *component)
{
	CSBusPassthroughNode *node = (CSBusPassthroughNode *)opaqueNode;

	// we can accept only one component
	csObject_release(node->childComponent);
	node->childComponent = csObject_retain(component);

	// do a prefilter if necessary
	if(node->prefilter)
	{
		node->prefilter(node, &((CSBusComponent *)component)->condition, &((CSBusComponent *)component)->outputLines);
	}
}

static void *csPassthroughNode_createComponent(void *opaqueNode)
{
	CSBusPassthroughNode *node = (CSBusPassthroughNode *)opaqueNode;

	return csComponent_create(
		node->filterFunction, 
		((CSBusComponent *)node->childComponent)->condition, 
		((CSBusComponent *)node->childComponent)->outputLines, 
		node);
}

static void csPassthroughNode_destroy(void *node)
{
	CSBusPassthroughNode *passthroughNode = (CSBusPassthroughNode *)node;
	csObject_release(passthroughNode->childComponent);
	csObject_release(passthroughNode->context);
}

void *csPassthroughNode_createWithFilter(void *context, csComponent_handlerFunction filterFunction, csPassthroughNode_prefilter prefilter)
{
	CSBusPassthroughNode *passthroughNode = (CSBusPassthroughNode *)calloc(1, sizeof(CSBusPassthroughNode));

	if(passthroughNode)
	{
		csBusNode_init(passthroughNode);
		passthroughNode->busNode.referenceCountedObject.dealloc =  csPassthroughNode_destroy;

		passthroughNode->filterFunction = filterFunction;
		passthroughNode->busNode.addComponent = csPassthroughNode_addComponent;
		passthroughNode->busNode.createComponent = csPassthroughNode_createComponent;

		passthroughNode->context = csObject_retain(context);
		passthroughNode->prefilter = prefilter;
	}

	return passthroughNode;
}
