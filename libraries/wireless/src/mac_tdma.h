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

#ifndef MAC_TDMA_H
#define MAC_TDMA_H

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

enum {
    TDMA_STATUS_TX,
    TDMA_STATUS_RX,
    TDMA_STATUS_IDLE
};

typedef struct tdma_header_str {
    Mac802Address sourceAddr;
    Mac802Address destAddr;
    int priority;
} TdmaHeader;

typedef struct struct_tdma_stats_str {
    int pktsGotUnicast;
    int pktsGotBroadcast;
    int pktsSentUnicast;
    int pktsSentBroadcast;
    int numTxSlotsMissed;
} TdmaStats;

typedef struct struct_mac_tdma_str {
    MacData* myMacData;

    int currentStatus;
    int currentSlotId;
    int nextStatus;
    int nextSlotId;

    int channelIndex;

    int currentReceiveSlotId;

    Message*  timerMsg;
    clocktype timerExpirationTime;

    int       numSlotsPerFrame;
    char*     frameDescriptor;
    clocktype slotDuration;
    clocktype guardTime;
    clocktype interFrameTime;

    TdmaStats stats;
#ifdef PARALLEL
    LookaheadHandle lookaheadHandle;
#endif
} MacDataTdma;


/*
 * FUNCTION    MacTdmaInit
 * PURPOSE     Initialization function for TDMA protocol of MAC layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacTdmaInit(
    Node *node,
    int interfaceIndex,
    const NodeInput* nodeInput,
    const int subnetListIndex,
    const int slotsperframe);

/*
 * FUNCTION    MacTdmaLayer
 * PURPOSE     Models the behaviour of the MAC layer with the TDMA protocol
 *             on receiving the message enclosed in msgHdr.
 *
 * Parameters:
 *     node:     node which received the message
 *     msgHdr:   message received by the layer
 */
void MacTdmaLayer(
    Node *node, int interfaceIndex, Message *msg);


/*
 * FUNCTION    MacTdmaFinalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of TDMA protocol of the MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void MacTdmaFinalize(Node *node, int interfaceIndex);

/*
 * FUNCTION    MacTdmaNetworkLayerHasPacketToSend
 * PURPOSE     To tell TDMA that the network layer has a packet to send.
 */

void MacTdmaNetworkLayerHasPacketToSend(Node *node, MacDataTdma *tdma);


void MacTdmaReceivePacketFromPhy(
    Node* node, MacDataTdma* tdma, Message* msg);


void MacTdmaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataTdma* tdma,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus);

#endif

