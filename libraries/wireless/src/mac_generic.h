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
#ifndef _GENERICMAC_MAC_H_
#define _GENERICMAC_MAC_H_


typedef struct struct_genericmac_str GenericMacData;

//UserCodeStart
#include <math.h>
#include "phy_802_11.h"
#include "network_ip.h"

#ifdef PARALLEL //Parallel
#include "phy_abstract.h"
#include "parallel.h"
#endif //endParallel

typedef enum CHANNEL_ACCESS_ENUM {
    CSMA,
    CSMA_CA,
    PSMA,
    SL_CSMA,// SLOT_CSMA
    SL_CSMA_CA // SLOT_CSMA_CA
} MacGenericChannelAccessType;

typedef enum MAC_GENERIC_FRAME_TYPE_ENUM {
    MGENERIC_RTS,
    MGENERIC_CTS,
    MGENERIC_DATA,
    MGENERIC_ACK
} MacGenericFrameType;


typedef enum MAC_GENERIC_STATUS_ENUM {
    MGENERIC_S_IDLE,
    MGENERIC_S_START,
    MGENERIC_S_YIELD,
    MGENERIC_S_BO,
    MGENERIC_S_WFCTS,
    MGENERIC_S_WFDATA,
    MGENERIC_S_WFACK,
    MGENERIC_X_RTS,
    MGENERIC_X_CTS,
    MGENERIC_X_BROADCAST,
    MGENERIC_X_UNICAST,
    MGENERIC_X_ACK
} MacGenericStatus;


typedef enum MAC_GENERIC_YIELD_TYPE_ENUM {
    MGENERIC_YIELD_RTS = MGENERIC_RTS,
    MGENERIC_YIELD_CTS = MGENERIC_CTS,
    MGENERIC_YIELD_DATA = MGENERIC_DATA,
    MGENERIC_YIELD_ACK = MGENERIC_ACK,
    MGENERIC_YIELD_DEQUEUE,
    MGENERIC_YIELD_NONE
} MacGenericYieldType;



typedef struct generic_mac_control_frame_str {
    unsigned char frameType;
    int duration;
    Mac802Address destAddr;
    Mac802Address sourceAddr;
} GenericMacControlFrame;


typedef struct generic_mac_frame_hdr_str {
    unsigned char frameType;
    int duration;
    Mac802Address destAddr;
    Mac802Address sourceAddr;
    unsigned short seqNo;
} GenericMacFrameHdr;


typedef struct generic_mac_frame_str {
    GenericMacFrameHdr hdr;
    char payload[MAX_NW_PKT_SIZE];
} GenericMacFrame;


typedef struct struct_mac_generic_seqno_entry_t {
    Mac802Address ipAddress;
    unsigned short fromSeqNo;
    unsigned short toSeqNo;
    struct struct_mac_generic_seqno_entry_t* next;
} GenericMacSeqNoEntry;



//UserCodeEnd

typedef struct {
    int placeholder;
} GenericMacStatsType;


typedef enum {
    GENERICMAC_NETWORKLAYERHASPACKETTOSEND,
    GENERICMAC_CHANNELPOLICYBRANCH,
    GENERICMAC_CSMA_CSMACA_BRANCH,
    GENERICMAC_PSMABRANCH,
    GENERICMAC_CTSFINISHED,
    GENERICMAC_ACKFINISHED,
    GENERICMAC_BUSYTOIDLE,
    GENERICMAC_SENDCTS,
    GENERICMAC_SENDACK,
    GENERICMAC_ACKPROCESS,
    GENERICMAC_DATAFINISHED,
    GENERICMAC_CTSPROCESS,
    GENERICMAC_RECEIVEPHYSTATUSCHANGENOTIFICATION,
    GENERICMAC_IDLE,
    GENERICMAC_FINAL_STATE,
    GENERICMAC_RTSPROCESS,
    GENERICMAC_IDLETOBUSY,
    GENERICMAC_SENDRTS,
    GENERICMAC_RTSFINISHED,
    GENERICMAC_SETTIMER,
    GENERICMAC_YIELD,
    GENERICMAC_HANDLETIMEOUT,
    GENERICMAC_DEQUEUEPACKET,
    GENERICMAC_RECEIVEPACKETFROMPHY,
    GENERICMAC_PACKETPROCESS,
    GENERICMAC_RETRANSMIT,
    GENERICMAC_SENDPACKET,
    GENERICMAC_INITIAL_STATE,
    GENERICMAC_DEFAULT_STATE
} GenericMacState;



struct struct_genericmac_str
{
    GenericMacState state;

    MacData* myMacData;
    Message* currentMessage;
    // The old `currentMessage' is actually `currentOutMessage'
    // which is my message waiting for ACK from my neighbor.
    // We need a `currentInMessage' which is to be ACKed
    // if ARQ from my neighbor towards me happens again.
    Message* currentInMessage;
    MacGenericChannelAccessType channelAccess;
    BOOL rtsCtsMode;
    BOOL ackMode;
    MacGenericStatus status;
    Mac802Address currentNextHopAddress;
    Mac802Address selfAddr;
    Mac802Address waitingForAckOrCtsFromAddress;
    Int64 bandwidth;
    clocktype extraPropDelay;
    clocktype ctsOrAckTransmissionDuration;
    Mac802Address ipNextHopAddr;
    NodeAddress ipSourceAddr;
    NodeAddress ipDestAddr;
    clocktype BO;
    clocktype lastBOTimestamp;
    clocktype slotTime;
    clocktype CW;
    int RC;
    unsigned int timerSequenceNumber;
    GenericMacSeqNoEntry* seqNoHead;
    int retryLimit;
    clocktype noResponseTimeoutDuration;
    clocktype dataTransmissionDuration;
    MacGenericYieldType yieldType;
    clocktype csma_ca_counter;
    clocktype csma_ca_idle_start;
    BOOL csma_ca_timer_on;
    GenericMacStatsType stats;
    RandomSeed seed;

    int pktsToSend;
    int pktsLost;
    int initStats;
    int printStats;

    clocktype slot; // the unit for Slotted CSMA/CSMACA
    // the backoff multiplier based on generic mac protocol
    clocktype backoff_mul;

    double p_access; // the probability for csma-short

    clocktype maxTranxTime; // maximum_transmit_time
    int tsbWinLength; // TSB_window_length

    int waitTimeInTSB; // Wait_Time_In_TSBs
    clocktype phyIdle; // the number of consecutive Idle TSBs

#ifdef PARALLEL
    LookaheadHandle     lookaheadHandle;
#endif
};


void
GenericMacInit(Node* node, int interfaceIndex,
             const NodeInput* nodeInput, const int nodesInSubnet);


void
GenericMacFinalize(Node* node, int interfaceIndex);


void
GenericMacProcessEvent(Node* node, int interfaceIndex, Message* msg);


void GenericMacRunTimeStat(Node* node, GenericMacData* dataPtr);


void MacGenericMacNetworkLayerHasPacketToSend(Node* node, GenericMacData* dataPtr);
void MacGenericMacReceivePacketFromPhy(Node* node, GenericMacData* dataPtr, Message* msg);
void MacGenericMacReceivePhyStatusChangeNotification(Node* node, GenericMacData* dataPtr,
                                                     PhyStatusType oldPhyStatus,
                                                     PhyStatusType newPhyStatus);
#endif
