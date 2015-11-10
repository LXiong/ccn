// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "routing_zrp.h"

//UserCodeStartBodyData
#include "routing_iarp.h"
#include "routing_ierp.h"


//UserCodeEndBodyData

//Statistic Function Declarations
static void ZrpInitStats(Node* node, ZrpStatsType *stats);
void ZrpPrintStats(Node *node);

//Utility Function Declarations

//State Function Declarations
static void enterZrpIdleState(Node* node, ZrpData* dataPtr, Message* msg);

//Utility Function Definitions
//UserCodeStart
// FUNCTION ZrpIsEnabled
// PURPOSE  checking if ZRP is already enabled
// Parameters:
// node: node which is checking
BOOL ZrpIsEnabled(Node* node) {
     return  (NetworkIpGetRoutingProtocol(
                  node,
                  ROUTING_PROTOCOL_ZRP) != NULL);
}


// FUNCTION ZrpReturnProtocolData
// PURPOSE  returning pointer to protocol data structure
// Parameters:
//  node:   node which is returning protocol data
//  routingProtocolType:    the routing Protocol Type
void* ZrpReturnProtocolData(Node* node,NetworkRoutingProtocolType routingProtocolType) {
    ZrpData* dataPtr = (ZrpData*) NetworkIpGetRoutingProtocol(
                                      node,
                                      ROUTING_PROTOCOL_ZRP);
    if (dataPtr == NULL)
    {
        ERROR_Assert(FALSE, "ZRP not found in this node");
        abort();
    }
    switch (routingProtocolType)
    {
        case ROUTING_PROTOCOL_IERP:
        {
            return dataPtr->ierpProtocolStruct;
        }
        case ROUTING_PROTOCOL_IARP:
        {
            return dataPtr->iarpProtocolStruct;
        }
        default:
        {
            return NULL;
        }
    }
}



//UserCodeEnd

//State Function Definitions
void ZrpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                             NodeAddress destAddr, int interfaceIndex, unsigned ttl) {
        //UserCodeStartReceivePacketFromNetwork
    // NO operation.
    // Control of code MUST NOT come here.
    ZrpData *dataPtr = (ZrpData*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_ZRP);
    ERROR_Assert(FALSE, "Control Must not reach Here");

    //UserCodeEndReceivePacketFromNetwork

    dataPtr->state = ZRP_IDLE_STATE;
    enterZrpIdleState(node, dataPtr, msg);
}


void ZrpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                       NodeAddress previousHopAddr, BOOL* packetWasRouted) {
        //UserCodeStartRoutePacket
    ZrpData *dataPtr = (ZrpData*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_ZRP);

    // call iarp router function first
    // if IARP is unable to route call IERP to route
    dataPtr->iarpRouterFunction(
        node,
        msg,
        destAddr,
        previousHopAddr,
        packetWasRouted);

    if (*packetWasRouted == FALSE)
    {
        // call ierp router function
        dataPtr->ierpRouterFunction(
            node,
            msg,
            destAddr,
            previousHopAddr,
            packetWasRouted);
    }
    // Set the message pointer to NULL. This will not effect
    // the original pointer passed to this function, but it will prevent
    // the  message from being freed at the idle state.
    msg = NULL;

    //UserCodeEndRoutePacket

    dataPtr->state = ZRP_IDLE_STATE;
    enterZrpIdleState(node, dataPtr, msg);
}


static void enterZrpIdleState(Node* node,
                              ZrpData* dataPtr,
                              Message *msg) {

    //UserCodeStartIdleState
    // Free the message if it is null.
    if (msg != NULL)
    {
        MESSAGE_Free(node, msg);
    }

    //UserCodeEndIdleState


}


//Statistic Function Definitions
static void ZrpInitStats(Node* node, ZrpStatsType *stats) {
    if (node->guiOption) {
    }

}

void ZrpPrintStats(Node* node) {
}

void ZrpRunTimeStat(Node* node, ZrpData* dataPtr) {
}

void
ZrpInit(Node* node, ZrpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex) {

    Message *msg = NULL;

    //UserCodeStartInit
    // The following line of code should be executed only
    // for the *first* initialization of an interface that
    // shares this data structure.

    // Initialize ZrpProtocol data structure
    (*dataPtr) = (ZrpData*) MEM_malloc(sizeof(ZrpData));
    memset((*dataPtr), 0, sizeof(ZrpData));
    (*dataPtr)->printStats = TRUE;

    // The routing protocols IARP and IERP will be Initialized
    // from ZRP. So first check that if IARP or IER is already
    // initialized or not. If they are initialized before flash
    // an error message and the abort execution.
    if (NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IERP) ||
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IARP))
    {
        ERROR_ReportError("IERP or IARP was initialized before \n"
                          "disable IARP or IERP first\n");
        abort();
    }

    // Register ZRP with IARP protocol
    IarpRegisterZrp(
        node,
        (IarpData**) &((*dataPtr)->iarpProtocolStruct),
        nodeInput,
        interfaceIndex,
        &((*dataPtr)->iarpRouterFunction));

    // Register ZRP with IERP protocol
    IerpRegisterZrp(
        node,
        (IerpData**) &((*dataPtr)->ierpProtocolStruct),
        nodeInput,
        interfaceIndex,
        &((*dataPtr)->ierpRouterFunction),
        &((*dataPtr)->macStatusHandlar));

    NetworkIpSetRouterFunction(
        node,
        ZrpRouterFunction,
        interfaceIndex);

    NetworkIpSetMacLayerStatusEventHandlerFunction(
        node,
        (*dataPtr)->macStatusHandlar,
        interfaceIndex);

    IarpSetIerpUpdeteFuncPtr(node,
        IerpReturnRouteTableUpdateFunction(node));

    if ((*dataPtr)->initStats)
    {
        ZrpInitStats(node, &((*dataPtr)->stats));
    }

    //UserCodeEndInit
    if ((*dataPtr)->initStats) {
        ZrpInitStats(node, &((*dataPtr)->stats));
    }
    (*dataPtr)->state = ZRP_IDLE_STATE;
    enterZrpIdleState(node, *dataPtr, msg);
}


void
ZrpFinalize(Node* node) {

    ZrpData *dataPtr = (ZrpData*) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_ZRP);

    //UserCodeStartFinalize
    IarpFinalize(node);
    IerpFinalize(node);

    //UserCodeEndFinalize

    if (dataPtr->statsPrinted) {
        return;
    }

    if (dataPtr->printStats) {
        dataPtr->statsPrinted = TRUE;
        ZrpPrintStats(node);
    }
}


void
ZrpProcessEvent(Node* node,   Message* msg) {

    ZrpData* dataPtr = (ZrpData*)  NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_ZRP);

    switch (dataPtr->state) {
        case ZRP_IDLE_STATE:
            assert(FALSE);
            break;
        default:
            ERROR_ReportError("Illegal transition");
            break;
    }

}

