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

#ifndef _ALOHA_MAC_H_
#define _ALOHA_MAC_H_


typedef struct struct_aloha_str AlohaData;

//UserCodeStart
#include <math.h>

typedef enum Aloha_Mode {
    ALOHA_IDLE,
    ALOHA_EXPECTING_ACK,
    ALOHA_YIELD
} AlohaMode;


typedef enum Aloha_TX_Mode {
    ALOHA_TX_IDLE,
    ALOHA_TX_ACK,
    ALOHA_TX_DATA
} AlohaTxMode;



typedef struct aloha_header_str {

    Mac802Address sourceAddr;
    Mac802Address destAddr;
    int payloadSize;
    int priority;
    BOOL ACK;
} AlohaHeader;



//UserCodeEnd

typedef struct {
    int pktsSent;
    int pktsSentId;
    int pktsReceived;
    int pktsReceivedId;
    int pktsDropped;
    int pktsDroppedId;
} AlohaStatsType;


enum {
    ALOHA_FINAL_STATE,
    ALOHA_IDLE_STATE,
    ALOHA_HANDLEDATA,
    ALOHA_NETWORKLAYERHASPACKETTOSEND,
    ALOHA_RETRANSMIT,
    ALOHA_INITIAL_STATE,
    ALOHA_RECEIVEPACKETFROMPHY,
    ALOHA_RECEIVEPACKET,
    ALOHA_RECEIVEPHYSTATUSCHANGENOTIFICATION,
    ALOHA_TRANSMIT,
    ALOHA_CHECKTRANSMIT,
    ALOHA_RECEIVEACK,
    ALOHA_DEFAULT_STATE
};



struct struct_aloha_str
{
    int state;
    MacData* myMacData;
    BOOL expectingACK;
    Mac802Address ACKAddress;
    int numTimeouts;
    Message* currentFrame;
    AlohaMode mode;
    Message* retransmitTimer;
    AlohaTxMode txMode;
    AlohaStatsType stats;
    RandomSeed seed;
    int initStats;
    int printStats;
};


void
AlohaInit(Node* node, int interfaceIndex, const NodeInput* nodeInput, const int nodesInSubnet);


void
AlohaFinalize(Node* node, int interfaceIndex);


void
AlohaProcessEvent(Node* node, int interfaceIndex, Message* msg);


void AlohaRunTimeStat(Node* node, AlohaData* dataPtr);


void MacAlohaNetworkLayerHasPacketToSend(Node* node, AlohaData* dataPtr);
void MacAlohaReceivePacketFromPhy(Node* node, AlohaData* dataPtr, Message* msg);
void MacAlohaReceivePhyStatusChangeNotification(Node* node, AlohaData* dataPtr,
                                                PhyStatusType oldPhyStatus, PhyStatusType newPhyStatus);


#endif
