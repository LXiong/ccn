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

#ifndef _ZRP_NETWORK_H_
#define _ZRP_NETWORK_H_


typedef struct struct_zrp_str ZrpData;

//UserCodeStart

typedef void (*IerpUpdateFunc) ( Node* node,  NodeAddress destinationAddress,  NodeAddress subnetMask,  NodeAddress path[],  int length); //Define a function pointer type that will be used to update ierp routing table

//UserCodeEnd

typedef struct {
    int placeholder;
} ZrpStatsType;


enum {
    ZRP_RECEIVE_PACKET_FROM_NETWORK,
    ZRP_ROUTE_PACKET,
    ZRP_IDLE_STATE,
    ZRP_INITIAL_STATE,
    ZRP_FINAL_STATE,
    ZRP_DEFAULT_STATE
};



struct struct_zrp_str
{
    int state;

    void* iarpProtocolStruct;
    void* ierpProtocolStruct;
    RouterFunctionType ierpRouterFunction;
    RouterFunctionType iarpRouterFunction;
    MacLayerStatusEventHandlerFunctionType macStatusHandlar;
    ZrpStatsType stats;
    RandomSeed seed;
    int initStats;
    int printStats;
    BOOL statsPrinted;
};


void
ZrpInit(Node* node, ZrpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex);


void
ZrpFinalize(Node* node);


void
ZrpProcessEvent(Node* node, Message* msg);


void ZrpRunTimeStat(Node* node, ZrpData* dataPtr);


void ZrpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                             NodeAddress destAddr, int interfaceIndex, unsigned ttl);
void ZrpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                       NodeAddress previousHopAddr, BOOL* packetWasRouted);



//Interface Utility Functions


// FUNCTION ZrpIsEnabled
// PURPOSE  checking if ZRP is already enabled
// Parameters:
//  node:   node which is checking
BOOL ZrpIsEnabled(Node* node);


// FUNCTION ZrpReturnProtocolData
// PURPOSE  returning pointer to protocol data structure
// Parameters:
//  node:   node which is returning protocol data
//  routingProtocolType:    the routing Protocol Type
void* ZrpReturnProtocolData(Node* node,NetworkRoutingProtocolType routingProtocolType);



#endif
