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

#ifndef LAR1_H
#define LAR1_H

typedef struct lar1_location {
    Int32 x;
    Int32 y;
} LAR1_Location;

typedef struct lar1_zone {
    LAR1_Location bottomLeft;
    LAR1_Location topLeft;
    LAR1_Location topRight;
    LAR1_Location bottomRight;
} LAR1_Zone;

typedef struct lar1_rcentry {
    NodeAddress destAddr;
    BOOL inUse;
    BOOL valid;
    double destVelocity;
    NodeAddress *path;
    int pathLength;
    clocktype locationTimestamp;
    LAR1_Location destLocation;
    struct lar1_rcentry *next;
} LAR1_RouteCacheEntry;

typedef struct lar1_reqseenentry {
    NodeAddress sourceAddr;
    int seqNum;
    clocktype lifetime;
    struct lar1_reqseenentry *next;
} LAR1_RequestSeenEntry;

typedef struct lar1_reqsententry {
    NodeAddress destAddr;
    struct lar1_reqsententry *next;
} LAR1_RequestSentEntry;

typedef struct lar1_sendbufferentry {
    NodeAddress destAddr;
    Message *msg;
    BOOL reTx;
    clocktype times;
} LAR1_SendBufferEntry;

typedef struct struct_network_lar1 {
    LAR1_RouteCacheEntry *routeCacheHead;
    LAR1_RequestSeenEntry *reqSeenHead;
    LAR1_RequestSentEntry *reqSentHead;
    LAR1_SendBufferEntry **sendBuf;

    int sendBufHead,
        sendBufTail;
    int seqNum;

    int DataPacketsSentAsSource,
        DataPacketsRelayed,
        RouteRequestsSentAsSource,
        RouteRepliesSentAsRecvr,
        RouteErrorsSentAsErrorSource,
        RouteRequestsRelayed,
        RouteRepliesRelayed,
        RouteErrorsRelayed;

    BOOL statsCollected;

    NodeAddress localAddress;
    RandomSeed seed;
} Lar1Data;



//
// FUNCTION     Lar1Init()
// PURPOSE      Initialize LAR1 Routing Protocol Dataspace
// PARAMETERS   lar1            - pointer for dataspace
//              nodeInput
//
void Lar1Init(
   Node* node,
   Lar1Data** lar1,
   const NodeInput* nodeInput,
   int interfaceIndex);

//
// FUNCTION     Lar1Finalize()
// PURPOSE      Finalize statistics Collection
//
void Lar1Finalize(Node *node);

//
// FUNCTION     Lar1HandleProtocolPacket()
// PURPOSE      Process a LAR1 generated control packet
// PARAMETERS   msg             - The packet
//
void Lar1HandleProtocolPacket(Node* node, Message* msg);

//
// FUNCTION     Lar1HandleCheckTimeoutAlarm()
// PURPOSE      Process timeouts sent by LAR1 to itself
// PARAMETERS   msg             - the timer
//
void Lar1HandleCheckTimeoutAlarm(Node* node, Message* msg);

//
// FUNCTION     Lar1RouterFunction()
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              previousHopAddr - previous hop of packet
//              PacketWasRouted - variable that indicates to IP that it
//                                no longer needs to act on this data packet
//
/* The Following must match RouterFunctionType defined in ip.h */

void Lar1RouterFunction(Node* node,
                        Message* msg,
                        NodeAddress destAddr,
                        NodeAddress previousHopAddr,
                        BOOL *PacketWasRouted);

//
// FUNCTION     Lar1MacLayerStatusHandler()
// PURPOSE      Handle internal messages from MAC Layer
// PARAMETERS   msg             - the packet returned by MAC
//
void Lar1MacLayerStatusHandler(Node *node,
                               const Message* msg,
                               const NodeAddress nextHopAddress,
                               const int interfaceIndex);


#endif
