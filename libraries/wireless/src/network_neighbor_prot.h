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

#ifndef NEIGHBOR_PROTOCOL_H
#define NEIGHBOR_PROTOCOL_H

typedef struct {
    int pktsRecv;
    int pktsSent;
} NeighborProtocolStats;


typedef struct {
    NodeAddress neighbor;
    clocktype lastHeard;
} NeighborInfoRecord;


typedef void (*NeighborProtocolUpdateFunctionType)(
   Node *node,
   NodeAddress neighbor,
   BOOL neighborIsReachable);


typedef struct {
    NodeAddress localAddr;
    int uniqueId;
    clocktype sendFrequency;
    clocktype entryLifetime;
    clocktype nextTimerExpiration;

    NeighborInfoRecord *nbrInfo;
    int numNbrInfoRecords;
    int maxNbrInfoRecords;

    NeighborProtocolUpdateFunctionType updateFunction;
    NeighborProtocolStats *stats;
} NeighborProtocolData;


void
NeighborProtocolRegisterNeighborUpdateFunction(
    Node* node,
    NeighborProtocolUpdateFunctionType updateFunction,
    NodeAddress localAddr,
    clocktype updateFrequency,
    clocktype deleteInterval);


NeighborProtocolData*
NeighborProtocolInit(
    Node* node,
    NodeAddress localAddr,
    clocktype sendFrequency,
    clocktype entryLifetime);


void
NeighborProtocolProcessMessage(
    Node* node,
    Message * msg);

void
NeighborProtocolFinalize(
    Node *node,
    AppInfo* appInfo);

#endif


