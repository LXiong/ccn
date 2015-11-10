// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
// All Rights Reserved.
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
#include "partition.h"
#include "network_ip.h"
#include "mac_generic.h"

#ifdef ENTERPRISE_LIB
#include "mpls_shim.h"
#endif // ENTERPRISE_LIB

//UserCodeStartBodyData
#define MGENERIC_RETRY_LIMIT   25
#define MGENERIC_CW_MIN   31
#define MGENERIC_DELAY_UNTIL_SIGNAL_AIRBORN  (5 * MICRO_SECOND)
#define MGENERIC_CW_MAX   1023
#define MGENERIC_PROPAGATION_DELAY   (1 * MICRO_SECOND)
#define MGENERIC_SIFS   (10*MICRO_SECOND)
#define MGENERIC_BO_MULTIPLIER   (20*MICRO_SECOND)
#define MAC_GENERIC_DEBUG  0 
#define MAC_GENERIC_SCHEDULER  0

// default value for slot csma medium access probability
#define SLOT_CSMA_ACCESS_PROBABILITY 0.25
// default value for tsb window length
#define TSB_WINDOW_LENGTH  256
#define MAXIMUM_TRANSMIT_TIME (200* MILLI_SECOND)

// default time-slot for Slotted CSMA & Slotted CSMACA
#define MGENERIC_SLOT       (10*MILLI_SECOND)

//UserCodeEndBodyData

//Statistic Function Declarations
static void GenericMacInitStats(Node* node,
                                GenericMacStatsType *stats,
                                GenericMacData* dataPtr);
void GenericMacPrintStats(Node *node, GenericMacData* dataPtr,
                                                                                        int interfaceIndex);

//Utility Function Declarations
static clocktype
                    GenericMacCalcFragmentTransmissionDuration(
                                                                       Int64 bandwidthBytesPerSec,
                                                                       int dataSizeBytes);
static void GenericMacIncreaseCW(Node* node,
                                                            GenericMacData* dataPtr);
static void GenericMacDecreaseCW(Node* node,
                                                            GenericMacData* dataPtr);
static void GenericMacSetBO(Node* node,GenericMacData* dataPtr);
static void GenericMacCancelTimer(Node* node,
                                                            GenericMacData* dataPtr);
static GenericMacSeqNoEntry* GenericMacGetSeqNo(Node* node,
                                                                        GenericMacData* dataPtr,
                                                                        Mac802Address destAddr);
static BOOL GenericMacIsWaitingForResponseState(
                                                                            MacGenericStatus status);
static BOOL GenericMacCorrectSeqenceNumber(Node* node,
                                                                         GenericMacData* dataPtr,
                                                                         Mac802Address sourceAddr,
                                                                         int seqNo);
static void GenericMacHandOffSuccessfullyReceivedUnicast(
                                                                            Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void GenericMacHandOffSuccessfullyReceivedBroadcast(
                                                                            Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void GenericMacDataComplete(Node* node,
                                                                   GenericMacData* dataPtr,
                                                                   Mac802Address sourceAddress,
                                                                   BOOL ack);
static void GenericMacChangeStatus(Node* node,
                                                                       GenericMacData* dataPtr,
                                                                     MacGenericStatus newStatus);
// return the simulation time at the boundary of specified timeslot
static clocktype GenericMacGetTimeSlot(GenericMacData* dataPtr,
                                                                                        clocktype _simulT);
static clocktype genericMacGetDelayUntilAirborne(Node* node,
                                                                            GenericMacData* dataPtr);
static BOOL getProtocolAccess(Node* node, GenericMacData* dataPtr);
static void GenericMacRecvOrSrcEOT(Node * node,
                                                                 GenericMacData* dataPtr);

//State Function Declarations
static void enterGenericMacChannelPolicyBranch(Node* node,
                                                                             GenericMacData* dataPtr,
                                                                             Message* msg);
static void enterGenericMacCSMA_CSMACA_Branch(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacPSMABranch(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacCTSFinished(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacACKFinished(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacBusyToIdle(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacSendCTS(Node* node,
                                                                             GenericMacData* dataPtr,
                                                                             Message* msg);
static void enterGenericMacSendACK(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacACKProcess(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacDataFinished(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacCTSProcess(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacIdle(Node* node,
                                                                            GenericMacData* dataPtr,
                                                                            Message* msg);
static void enterGenericMacRTSProcess(Node* node,
                                                                             GenericMacData* dataPtr,
                                                                             Message* msg);
static void enterGenericMacIdleToBusy(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacSendRTS(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacRTSFinished(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacSetTimer(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacYield(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacHandleTimeout(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacDequeuePacket(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacPacketProcess(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacRetransmit(Node* node, GenericMacData* dataPtr, Message* msg);
static void enterGenericMacSendPacket(Node* node, GenericMacData* dataPtr, Message* msg);

//Utility Function Definitions
//UserCodeStart
static clocktype GenericMacCalcFragmentTransmissionDuration(Int64 bandwidthBytesPerSec,int dataSizeBytes) {

    return (clocktype) (PHY802_11b_SYNCHRONIZATION_TIME +
            ( (clocktype) ((dataSizeBytes + sizeof(GenericMacFrameHdr)) * SECOND) /
            (clocktype) bandwidthBytesPerSec));
}


static void GenericMacIncreaseCW(Node* node,GenericMacData* dataPtr) {
    dataPtr->CW = MIN((((dataPtr->CW + 1) * 2) - 1), MGENERIC_CW_MAX);
}


static void GenericMacDecreaseCW(Node* node,GenericMacData* dataPtr) {
    dataPtr->CW = MGENERIC_CW_MIN;
}


static void GenericMacSetBO(Node* node,GenericMacData* dataPtr) {
    if (dataPtr->BO == 0) {
        clocktype nRand = RANDOM_nrand(dataPtr->seed);
        dataPtr->BO = (nRand % dataPtr->CW) * dataPtr->backoff_mul;
        dataPtr->lastBOTimestamp = node->getNodeTime();
        //dataPtr->lastBOTimestamp = 0;
        if (dataPtr->BO == 0) {
            dataPtr->BO = 1;
        }
    }
}


static void GenericMacCancelTimer(Node* node,GenericMacData* dataPtr) {
    dataPtr->timerSequenceNumber++;

    if ((dataPtr->channelAccess == CSMA_CA)
        ||(dataPtr->channelAccess == SL_CSMA_CA)) {
        dataPtr->csma_ca_timer_on = FALSE;
    }
}


static GenericMacSeqNoEntry* GenericMacGetSeqNo(Node* node, GenericMacData* dataPtr, Mac802Address destAddr) {
    GenericMacSeqNoEntry* entry = dataPtr->seqNoHead;
    GenericMacSeqNoEntry* prev = NULL;
    if (!entry) {
        entry = (GenericMacSeqNoEntry*) MEM_malloc(sizeof(GenericMacSeqNoEntry));

        MAC_CopyMac802Address(&entry->ipAddress , &destAddr );

        entry->fromSeqNo = 0;
        entry->toSeqNo = 0;
        entry->next = NULL;
        dataPtr->seqNoHead = entry;
        return entry;
    }
    while (entry) {
        if (MAC_IsIdenticalMac802Address(&entry->ipAddress , &destAddr)) {
            return entry;
        }
        else {
            prev = entry;
            entry = entry->next;
        }
    }
    entry = (GenericMacSeqNoEntry*) MEM_malloc(sizeof(GenericMacSeqNoEntry));
    MAC_CopyMac802Address(&entry->ipAddress , &destAddr );
    entry->fromSeqNo = 0;
    entry->toSeqNo = 0;
    entry->next = NULL;
    prev->next = entry;
    return entry;
}


static BOOL GenericMacIsWaitingForResponseState(MacGenericStatus status) {
    return((MGENERIC_S_WFCTS <= status) && (status <= MGENERIC_S_WFACK));
}


static BOOL GenericMacCorrectSeqenceNumber(
                    Node* node,
                    GenericMacData* dataPtr,
                    Mac802Address sourceAddr,
                    int seqNo) {
    GenericMacSeqNoEntry* entry;
    entry = GenericMacGetSeqNo(node, dataPtr, sourceAddr);
    ERROR_Assert(entry,
        "GenericMacCorrectSeqenceNumber: "
        "Sequence number entry not found.\n");
    return (entry->fromSeqNo == seqNo);
}

static Int32 GenericMacCompareSeqenceNumber(
                    Node* node,
                    GenericMacData* dataPtr,
                    Mac802Address sourceAddr,
                    int seqNo) {
    GenericMacSeqNoEntry* entry;
    entry = GenericMacGetSeqNo(node, dataPtr, sourceAddr);
    ERROR_Assert(entry,
        "GenericMacCorrectSeqenceNumber: "
        "Sequence number entry not found.\n");
    if (entry->fromSeqNo == seqNo)
        return(0);
    else if (entry->fromSeqNo > seqNo)
        return(-1);
    else // if (entry->fromSeqNo < seqNo)
        return(1);
}

static void GenericMacHandOffSuccessfullyReceivedUnicast(
                    Node* node,
                    GenericMacData* dataPtr,
                    Message* msg) {
    GenericMacSeqNoEntry *entry;
    GenericMacFrameHdr* hdr =
        (GenericMacFrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&sourceAddr , &hdr->sourceAddr );

    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(GenericMacFrameHdr),
                         TRACE_GENERICMAC);

    {

        MacHWAddress srcmacHWAddr;
        Convert802AddressToVariableHWAddress(node, &srcmacHWAddr,
                                            &sourceAddr);
        MAC_HandOffSuccessfullyReceivedPacket(node,
        dataPtr->myMacData->interfaceIndex, msg, &srcmacHWAddr);


    }
    // Upate sequence number
    entry = GenericMacGetSeqNo(node, dataPtr, sourceAddr);
    ERROR_Assert(entry,
        "GenericMacHandOffSuccessfullyReceivedUnicast: "
        "Unable to create or find sequence number entry.\n");
    entry->fromSeqNo += 1;
}

static void GenericMacHandOffSuccessfullyReceivedBroadcast(
                Node* node,
                GenericMacData* dataPtr,
                Message* msg) {
    GenericMacFrameHdr* lHdr =
        (GenericMacFrameHdr*) MESSAGE_ReturnPacket(msg);

    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&sourceAddr , &lHdr->sourceAddr );

    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(GenericMacFrameHdr),
                         TRACE_GENERICMAC);
    {

        MacHWAddress srcmacHWAddr;
        Convert802AddressToVariableHWAddress(node , &srcmacHWAddr ,
                                            &sourceAddr);
        MAC_HandOffSuccessfullyReceivedPacket(node,
        dataPtr->myMacData->interfaceIndex, msg, &srcmacHWAddr);

    }
}


static void GenericMacDataComplete(
                 Node* node,
                 GenericMacData* dataPtr,
                 Mac802Address sourceAddress,
                 BOOL ack) {
    GenericMacSeqNoEntry *entry;
    // Reset retry counts since frame is acknowledged
    dataPtr->RC = 0;

       if (dataPtr->channelAccess != PSMA){
            GenericMacDecreaseCW(node, dataPtr);
        }

     const unsigned char Invalid_mac_addr[6]={0xff,0xff,0xff,0xff,0xff,0xfe};
    memcpy(&dataPtr->waitingForAckOrCtsFromAddress,
            Invalid_mac_addr,
            MAC_ADDRESS_DEFAULT_LENGTH);

    if (ack) {

        MacHWAddress srcHWAddress;
        Convert802AddressToVariableHWAddress(node,&srcHWAddress,
                                             &sourceAddress);
        MAC_MacLayerAcknowledgement(node,
                                    dataPtr->myMacData->interfaceIndex,
                                    dataPtr->currentMessage, srcHWAddress);
        MESSAGE_Free(node, dataPtr->currentMessage);
    }

    dataPtr->currentMessage = NULL;

    memcpy(&dataPtr->currentNextHopAddress , ANY_MAC802, sizeof(Mac802Address));
    memcpy(&dataPtr->ipNextHopAddr , ANY_MAC802 , sizeof(Mac802Address));
    dataPtr->ipDestAddr = ANY_DEST;
    dataPtr->ipSourceAddr = ANY_DEST;

    dataPtr->csma_ca_counter = 0;
    dataPtr->csma_ca_idle_start = 0;


    if (ack) {
        // Update exchange sequence number
        entry = GenericMacGetSeqNo(node, dataPtr, sourceAddress);
        ERROR_Assert(entry,
                     "GenericMacUpdateForSuccessfullyReceivedAck: "
                     "Unable to find sequence number entry.\n");
        entry->toSeqNo += 1;
    }
}


static void GenericMacChangeStatus(Node* node,GenericMacData* dataPtr,MacGenericStatus newStatus) {
    if (MAC_GENERIC_DEBUG) {
        printf("Changing status at Node %d from :%d: to :%d:\n", node->nodeId, dataPtr->status,
               newStatus);
    }

    if (newStatus > MGENERIC_S_WFACK){
        dataPtr->phyIdle = 0;
    }
    dataPtr->status = newStatus;
}

//UserCodeEnd


//State Function Definitions
void MacGenericMacNetworkLayerHasPacketToSend(Node* node, GenericMacData* dataPtr) {
    //UserCodeStartNetworkLayerHasPacketToSend
    BOOL idle = (dataPtr->status == MGENERIC_S_IDLE);

    if (MAC_GENERIC_DEBUG) {
        printf("NetworkLayerHasPackToSend for node %d, *called*\n", node->nodeId);
    }
    //printf("PHY STATUS at node %d is %d\n", node->nodeId,
    //       PHY_GetStatus(node, dataPtr->myMacData->phyNumber));


    //UserCodeEndNetworkLayerHasPacketToSend
    if (idle) {
        enterGenericMacDequeuePacket(node, dataPtr, NULL);
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, NULL);
    }
}


void MacGenericMacReceivePacketFromPhy(Node* node, GenericMacData* dataPtr, Message* msg) {
    //UserCodeStartReceivePacketFromPhy
    GenericMacControlFrame* hdr = (GenericMacControlFrame*) MESSAGE_ReturnPacket(msg);

    BOOL ifMine = FALSE;

    BOOL broadcast = (hdr->frameType == MGENERIC_DATA)
                        && (MAC_IsBroadcastMac802Address(&hdr->destAddr));

    BOOL yield = FALSE;
    BOOL process = FALSE;
    BOOL backoff = FALSE;
    BOOL ignore = FALSE;

    unsigned char frameType = hdr->frameType;
    int rts, cts, data, ack;

    ERROR_Assert(dataPtr->status < MGENERIC_X_RTS,
                 "GenericMacReceivePacketFromPhy: "
                 "Cannot receive packet while in transmit state.\n");

    rts =  (frameType == MGENERIC_RTS);
    cts =  (frameType == MGENERIC_CTS);
    data = (frameType == MGENERIC_DATA);
    ack =  (frameType == MGENERIC_ACK);

    if (MAC_IsIdenticalMac802Address(&dataPtr->selfAddr ,
                                &hdr->destAddr) || broadcast) {
        // My neighbor, who failed to receive my ACK, sends me the packet again

        if (broadcast || rts || cts || data)
        {
            ifMine = TRUE;
        }
        else if (ack)
        {
            if (dataPtr->status == MGENERIC_S_WFACK)
            {
                 if (MAC_IsIdenticalMac802Address(
                            &dataPtr->waitingForAckOrCtsFromAddress,
                            &hdr->sourceAddr))
                 {
                     ifMine = TRUE;
                 }
                else
                {
                    ifMine = FALSE;
                }
            }
        }
    } else {
        ifMine = FALSE;
    }

    if (MAC_GENERIC_DEBUG) {
        if (rts) {
            printf("Received RTS Packet!! %d\n", node->nodeId);
        } else if (cts) {
            printf("Received CTS Packet!! %d\n", node->nodeId);
        } else if (data) {
            printf("Received DATA Packet!! %d\n", node->nodeId);
        } else if (ack) {
            printf("Received ACK Packet!! %d\n", node->nodeId);
        } else {
            printf("Received *UNDETERMINED* Packet!! %d\n", node->nodeId);
        }

        if (ifMine) {
            printf("Node %d: YES ITS OUR PACKET\n", node->nodeId);
        } else {
            printf("Node %d: NO ITS NOT OUR PACKET\n", node->nodeId);
            printf("Destination Address : ");
            MAC_PrintMacAddr(&hdr->destAddr);
            printf("\n Self Address : ");
            MAC_PrintMacAddr(&hdr->sourceAddr);
            printf("\n");
        }
    }

     if (dataPtr->status == MGENERIC_S_YIELD && data)
     {
         if (ifMine)
         {
             process = TRUE;
         }
         else
         {
             ignore = TRUE;
         }
     }
     else if (dataPtr->status == MGENERIC_S_YIELD ||
        (dataPtr->status == MGENERIC_S_BO && dataPtr->channelAccess == CSMA_CA) ||
        (dataPtr->status == MGENERIC_S_BO && dataPtr->channelAccess == SL_CSMA_CA)) {

        if ((dataPtr->status == MGENERIC_S_BO && dataPtr->channelAccess == CSMA_CA) ||
        (dataPtr->status == MGENERIC_S_BO && dataPtr->channelAccess == SL_CSMA_CA))
        {
            BOOL free = PHY_GetStatus(node, dataPtr->myMacData->phyNumber) == PHY_IDLE;
            if (free)
            {
                Message *timerMsg = NULL;
                if (dataPtr->BO != 0)
                {
                    enterGenericMacSetTimer(node, dataPtr, timerMsg);
                }
            }
            if (free && (broadcast
                ||((!broadcast)&&(!dataPtr->ackMode)&&(!ifMine)&&(data))
                ||(ack && (!ifMine))))
            {
                if (MAC_GENERIC_DEBUG) {
                    printf("Making an extra carrier sensing node = %d "
                        "broadcast = %d, ack = %d,"
                        "ifMine = %d, data = %d \n",
                        node->nodeId, broadcast, ack, ifMine, data);
                }
                dataPtr->state = GENERICMAC_BUSYTOIDLE;
                enterGenericMacBusyToIdle(node, dataPtr, NULL);
                MESSAGE_Free(node, msg);
                return;
            }
        }

        ignore = TRUE;
        if (MAC_GENERIC_DEBUG) {
            printf("Ignoring packet, either in YIELD, or BO/CSMACA mode\n");
        }
    } else if (!ifMine && (dataPtr->status == MGENERIC_S_IDLE)) {
        if ((data && !dataPtr->ackMode) || ack) {
            ignore = TRUE;
            if (MAC_GENERIC_DEBUG) {
                printf("**ignoring packet** (idle, not ours, data or ack)\n");
            }
        } else {
            yield = TRUE;
            dataPtr->yieldType = (MacGenericYieldType)frameType;
            if (MAC_GENERIC_DEBUG) {
                printf("**Yielding on packRec** (idle, not ours, rts, cts, data w/reply)\n");
            }
            if (rts) {
                dataPtr->noResponseTimeoutDuration = hdr->duration;
            }
        }
    } else if ((dataPtr->status == MGENERIC_S_BO)) {
        //MAC in backoff mode
         ERROR_Assert(dataPtr->channelAccess == CSMA || dataPtr->channelAccess == SL_CSMA,
                 "False");
        //during backoff, process the packets.
         if (ifMine){
            // mine packet. Calculate BO time left and process the incoming
             //packet
            GenericMacCancelTimer(node, dataPtr);
            dataPtr->BO = dataPtr->BO - (node->getNodeTime() - dataPtr->lastBOTimestamp);
            dataPtr->lastBOTimestamp = node->getNodeTime();
            process = TRUE;
        }
        else{
            // not my packet. start yielding
            if (rts || cts || (data && dataPtr->ackMode)){
                dataPtr->BO = 0;
                GenericMacCancelTimer(node, dataPtr);
                dataPtr->yieldType = (MacGenericYieldType)frameType;
                yield = TRUE;
            }
            else if (data || ack){
                ignore = TRUE;
            }
        }
    } else if (((rts && (dataPtr->status != MGENERIC_S_IDLE)) ||
                (cts && (dataPtr->status != MGENERIC_S_WFCTS)) ||
                (ack && (dataPtr->status != MGENERIC_S_WFACK)))) {
        // cancel timer , backoff
        if ((dataPtr->status == MGENERIC_S_WFCTS) ||
            (dataPtr->status == MGENERIC_S_WFACK)) {
                GenericMacCancelTimer(node, dataPtr);
                dataPtr->BO = 0;
                backoff = TRUE;
        } else {
                ignore = TRUE;
        }
    } else if (data &&
               ((dataPtr->rtsCtsMode && !broadcast && (dataPtr->status != MGENERIC_S_WFDATA)) ||
                (!dataPtr->rtsCtsMode && (dataPtr->status != MGENERIC_S_IDLE)) ||
                (broadcast && (dataPtr->status!= MGENERIC_S_IDLE)))) {
        // cancel timer , backoff
        if ((dataPtr->status == MGENERIC_S_WFCTS) ||
            (dataPtr->status == MGENERIC_S_WFACK)) {
                GenericMacCancelTimer(node, dataPtr);
                backoff = TRUE;
        } else {
                ignore = TRUE;
        }
    } else {
        if (ifMine) {
            process = TRUE;
        } else {
            ignore = TRUE;
        }
    }


    ERROR_Assert(process || ignore || backoff || yield,
                 "MacGenericReceivePacketFromPhy : No Action Chosen!\n");


    //UserCodeEndReceivePacketFromPhy

    if (yield) {
        if ((process && ack) ||
            (process && rts) ||
            (process && data) ||
            (process && cts) ||
             ignore ||
             backoff ) {
             ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_YIELD;
        enterGenericMacYield(node, dataPtr, msg);

        MESSAGE_Free(node, msg);
        return;
    } else if (process && ack) {
        if (yield ||
            (process && rts) ||
            (process && data) ||
            (process && cts) ||
             ignore ||
             backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_ACKPROCESS;
        enterGenericMacACKProcess(node, dataPtr, msg);
        return;
    } else if (process && rts) {
        if (yield ||
            (process && ack) ||
            (process && data) ||
            (process && cts) ||
             ignore ||
             backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_RTSPROCESS;
        enterGenericMacRTSProcess(node, dataPtr, msg);

        MESSAGE_Free(node, msg);
        return;
    } else if (process && data) {
        if (yield ||
            (process && ack) ||
            (process && rts) ||
            (process && cts) ||
            ignore ||
            backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_PACKETPROCESS;
        enterGenericMacPacketProcess(node, dataPtr, msg);
        return;
    } else if (process && cts) {
        if (yield ||
            (process && ack) ||
            (process && rts) ||
            (process && data) ||
            ignore ||
            backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_CTSPROCESS;
        enterGenericMacCTSProcess(node, dataPtr, msg);
        return;
    } else if (ignore) {
        if (yield ||
            (process && ack) ||
            (process && rts) ||
            (process && data) ||
            (process && cts) ||
            backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);

        MESSAGE_Free(node, msg);
        return;
    } else if (backoff) {
        if (yield ||
            (process && ack) ||
            (process && rts) ||
            (process && data) ||
            (process && cts) ||
            ignore ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_RETRANSMIT;
        enterGenericMacRetransmit(node, dataPtr, msg);

        MESSAGE_Free(node, msg);
        return;
    }
}

static void GenericMacRecvOrSrcEOT(Node * node, GenericMacData* dataPtr)
{
    // set the idle time
    // note merge if needed
    if (dataPtr->phyIdle == 0)
    {
        if (MAC_GENERIC_DEBUG) {
            printf("node %d, phyIdle - %" TYPES_64BITFMT "d\n",
                    node->nodeId,
                    dataPtr->phyIdle);
        }
        dataPtr->phyIdle = node->getNodeTime();
    }

    // recalculate the waitTimeInTSB
    //if (dataPtr->waitTimeInTSB == -1)
    //{
        int nRand = RANDOM_nrand(dataPtr->seed);
        dataPtr->waitTimeInTSB = (nRand % dataPtr->tsbWinLength) ;
        if (dataPtr->waitTimeInTSB == 0) {
            dataPtr->waitTimeInTSB = 1;
        }
    //}
    return ;
}

//--------------------------------------------------------------------------
//  NAME:        GenericMacStationPauseBackoff.
//  PURPOSE:     Stop the backoff counter once carrier is sensed.
//  PARAMETERS:  Node* node
//                  Pointer to node that is stopping its own backoff.
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void GenericMacStationPauseBackoff(Node* node, GenericMacData* dataPtr)
{
    clocktype backoff = (node->getNodeTime() - dataPtr->lastBOTimestamp);

    if (dataPtr->BO > 0)
    {
        // Minus already backed off time
        dataPtr->BO = (dataPtr->BO - backoff) > 0?(dataPtr->BO - backoff):0;
    }
    // If backoff timer expired, won't be here, otherwise error
    ERROR_Assert(dataPtr->BO >= 0, "GENERICMAC: Backoff time calculation error.\n");
}// MacDot11StationPauseBackoff

void MacGenericMacReceivePhyStatusChangeNotification(Node* node, GenericMacData* dataPtr,
                                                     PhyStatusType oldPhyStatus,
                                                     PhyStatusType newPhyStatus) {


    //UserCodeStartReceivePhyStatusChangeNotification
    BOOL busyToIdle = FALSE;
    BOOL idleToBusy = FALSE;
    char clockStr[100];

    if (MAC_GENERIC_DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("PhyStatusChangeNotification node %d at %s, new - %d\n",
               node->nodeId, clockStr, newPhyStatus);
    }

    if ((dataPtr->status == MGENERIC_S_BO) &&
        ((dataPtr->channelAccess == CSMA_CA)
            || (dataPtr->channelAccess == SL_CSMA_CA)))
    {
        if (//((oldPhyStatus == PHY_SENSING) || (oldPhyStatus == PHY_RECEIVING)) &&
             (newPhyStatus == PHY_IDLE)) {
            if (MAC_GENERIC_DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %d: *****BUSY TO IDLE SET TRUE at %s!\n",
                       node->nodeId, clockStr);
            }

            Message *msg = NULL;
            if (dataPtr->BO != 0 && dataPtr->status == MGENERIC_S_BO)
            {
                enterGenericMacSetTimer(node, dataPtr, msg);
            }

            busyToIdle = TRUE;
        }

        if ((oldPhyStatus == PHY_IDLE) &&
            ((newPhyStatus == PHY_SENSING) || (newPhyStatus == PHY_RECEIVING)))
        {
            if (MAC_GENERIC_DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %d: *****IDLE TO BUSY SET TRUE at %s!\n",
                       node->nodeId, clockStr);
            }
            idleToBusy = TRUE;

            GenericMacStationPauseBackoff(node,dataPtr);
            if (dataPtr->BO != 0)
            {
                //GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
                GenericMacCancelTimer(node, dataPtr);
            }
            else
            {
                // Otherwise, no change so that backoff timer will be fired
                if (MAC_GENERIC_DEBUG) {
                    ctoa(node->getNodeTime(), clockStr);
                    printf("Node %d: idletobusy BO == 0 at %s\n",
                           node->nodeId, clockStr);
                }
            }
        }
    }

    // Note: there is no IDLE -> TRANSMITTING
    if ((oldPhyStatus == PHY_TRANSMITTING) && (newPhyStatus == PHY_IDLE)){
        if (dataPtr->phyIdle == 0){
            dataPtr->phyIdle = node->getNodeTime();
        }
    }
    if ((oldPhyStatus == PHY_RECEIVING) && (newPhyStatus == PHY_IDLE)){
        if (dataPtr->phyIdle == 0){
            dataPtr->phyIdle = node->getNodeTime();
        }
    }
    if ((oldPhyStatus == PHY_IDLE) &&
        ((newPhyStatus == PHY_SENSING) || (newPhyStatus == PHY_RECEIVING))){
            dataPtr->phyIdle = 0;
    }

    //UserCodeEndReceivePhyStatusChangeNotification

    if ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) ||
             idleToBusy || busyToIdle ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_RTSFINISHED;
        enterGenericMacRTSFinished(node, dataPtr, NULL);
        return;
    } else if ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) ||
             idleToBusy || busyToIdle ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_CTSFINISHED;
        enterGenericMacCTSFinished(node, dataPtr, NULL);
        return;
    } else if ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) ||
             idleToBusy || busyToIdle ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_DATAFINISHED;
        enterGenericMacDataFinished(node, dataPtr, NULL);
        return;
    } else if ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) ||
             idleToBusy || busyToIdle ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_ACKFINISHED;
        enterGenericMacACKFinished(node, dataPtr, NULL);
        return;
    } else if (idleToBusy) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) ||
             busyToIdle ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_IDLETOBUSY;
        enterGenericMacIdleToBusy(node, dataPtr, NULL);
        return;
    } else if (busyToIdle) {
        if (((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_RTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_CTS)) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && ((dataPtr->status == MGENERIC_X_UNICAST) || (dataPtr->status == MGENERIC_X_BROADCAST))) ||
            ((oldPhyStatus == PHY_TRANSMITTING) && (dataPtr->status == MGENERIC_X_ACK)) ||
             idleToBusy ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_BUSYTOIDLE;
        enterGenericMacBusyToIdle(node, dataPtr, NULL);
        return;
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, NULL);
        return;
    }
}


static void enterGenericMacChannelPolicyBranch(Node* node,
                                               GenericMacData* dataPtr,
                                               Message *msg) {

    //UserCodeStartChannelPolicyBranch
    MacGenericChannelAccessType chan_access = dataPtr->channelAccess;
    BOOL psma = (chan_access == PSMA);
    BOOL csma = (chan_access == CSMA);
    BOOL csma_ca = (chan_access == CSMA_CA);
    // slotted csma
    BOOL sl_csma = (chan_access == SL_CSMA);
    // slotted csma_ca
    BOOL sl_csma_ca = (chan_access == SL_CSMA_CA);

    ERROR_Assert(dataPtr->currentMessage != NULL,
                 "MacGenericChannelPolicyBranch: "
                 "A packet does not exist in the local buffer.\n");

    ERROR_Assert((chan_access == PSMA) || (chan_access == CSMA) || (chan_access == CSMA_CA) ||
                (chan_access == SL_CSMA) || (chan_access == SL_CSMA_CA),
                 "MacGenericChannelPolicyBranch: "
                 "Only support of CSMA/PSMA/CSMA-CA/SLOTTED CSMA/SLOTTED CSMA-CA"
                 "currently.\n");

    //UserCodeEndChannelPolicyBranch

    if (psma) {
        if (csma || csma_ca || sl_csma || sl_csma_ca) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_PSMABRANCH;
        enterGenericMacPSMABranch(node, dataPtr, msg);
        return;
    } else if (csma || csma_ca || sl_csma || sl_csma_ca) {
        if (psma ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_CSMA_CSMACA_BRANCH;
        enterGenericMacCSMA_CSMACA_Branch(node, dataPtr, msg);
        return;
    }
}


static BOOL getProtocolAccess(Node* node, GenericMacData* dataPtr)
{
    // true if (csma-short-immediate || probability > p_access
    //          : || csma-normal-immediate || # of idle slots > thres)
    //          : false else

    // rn is a random number between 0 ... 1
    BOOL free = PHY_GetStatus(node, dataPtr->myMacData->phyNumber) == PHY_IDLE;

    if ((dataPtr->channelAccess != SL_CSMA) && (dataPtr->channelAccess != SL_CSMA_CA)) {
        return free;
    }

    return free;


}


static void enterGenericMacCSMA_CSMACA_Branch(Node* node,
                                              GenericMacData* dataPtr,
                                              Message *msg) {
    // UserCodeStartSL_CSMA_CSMACA_Branch
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);
    // BOOL free = PHY_GetStatus(node, dataPtr->myMacData->phyNumber) == PHY_IDLE;
    BOOL rts = (dataPtr->rtsCtsMode &&
            (! MAC_IsBroadcastMac802Address(
                            &dataPtr->currentNextHopAddress)));

    BOOL free_or_proto = getProtocolAccess(node, dataPtr);

    //BOOL free_or_proto = free || _proto;

    if (MAC_GENERIC_DEBUG) {
        if (free_or_proto) {
            printf("CSMA : Channel is FREE at node %d at %s!\n",
                node->nodeId, clockStr);
        } else {
            printf("CSMA : Channel is BUSY, retransmitting at node %d"
                    "at %s!\n", node->nodeId, clockStr);
        }
    }

    // UserCodeEndCSMA_CSMACA_Branch
    if (dataPtr->channelAccess == CSMA
            || dataPtr->channelAccess == SL_CSMA)
    {
        if (dataPtr->BO != 0)
        {
            //calculate the BO time left.
            dataPtr->BO = dataPtr->BO - (node->getNodeTime()
                                    - dataPtr->lastBOTimestamp);
            if (dataPtr->BO < 0)
            {
                //the time at which original BO was about to expire is less than
                //current time. Start a fresh backoff.
                dataPtr->BO = 0;
                dataPtr->state = GENERICMAC_RETRANSMIT;
                enterGenericMacRetransmit(node, dataPtr, msg);
                return;
            }
            else if (dataPtr->BO == 0)
            {
                //dont do anything. start transmission if PHY is free.
            }
            else
            {
                //BO > 0, backoff still left, start backoff for this interval
             /*dataPtr->lastBOTimestamp = node->getNodeTime(); // not required*/
                 Message *msg = NULL;
                GenericMacChangeStatus(node, dataPtr, MGENERIC_S_BO);
                enterGenericMacSetTimer(node, dataPtr, msg);
                return;
            }
        }
    }
    // UserCodeEndSL_CSMA_CSMACA_Branch

    if (free_or_proto && dataPtr->BO!= 0)
    {
        Message *msg = NULL;
        GenericMacChangeStatus(node, dataPtr, MGENERIC_S_BO);
        enterGenericMacSetTimer(node, dataPtr, msg);
        return;
    }


    if (!rts && free_or_proto) {
        if ((rts && free_or_proto) ||
            (!free_or_proto) ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_SENDPACKET;
        enterGenericMacSendPacket(node, dataPtr, msg);
        return;
    } else if (rts && free_or_proto) {
        if ((!rts && free_or_proto) ||
            (!free_or_proto)) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_SENDRTS;
        enterGenericMacSendRTS(node, dataPtr, msg);
        return;
    } else if (!free_or_proto) {
        if ((!rts && free_or_proto) ||
            (rts && free_or_proto)) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_RETRANSMIT;
        enterGenericMacRetransmit(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacPSMABranch(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartPSMABranch

    BOOL rts = (dataPtr->rtsCtsMode &&
            (! MAC_IsBroadcastMac802Address
                        (&dataPtr->currentNextHopAddress)));
    //end

    if (MAC_GENERIC_DEBUG) {
        printf("PSMA channel policy chosen for node %d\n", node->nodeId);
    }

    //UserCodeEndPSMABranch

    if (!rts) {
        if (rts ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_SENDPACKET;
        enterGenericMacSendPacket(node, dataPtr, msg);
        return;
    } else if (rts) {
        if (!rts ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_SENDRTS;
        enterGenericMacSendRTS(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacCTSFinished(Node* node,
                                       GenericMacData* dataPtr,
                                       Message *msg) {

    //UserCodeStartCTSFinished
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_WFDATA);

    if (MAC_GENERIC_DEBUG) {
        printf("finished SENDING CTS at node %d, time %s\n", node->nodeId, clockStr);
    }

    //UserCodeEndCTSFinished

    dataPtr->state = GENERICMAC_SETTIMER;
    enterGenericMacSetTimer(node, dataPtr, msg);
}

static void enterGenericMacACKFinished(Node* node,
                                       GenericMacData* dataPtr,
                                       Message *msg) {

    //UserCodeStartACKFinished
    BOOL dequeuePacket = FALSE;
    BOOL resumeTransmit = FALSE;
    char clockStr[100];

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);

    ctoa(node->getNodeTime(), clockStr);

    if (dataPtr->currentMessage != NULL)
    {
        resumeTransmit = TRUE;
    }
    else
    {
        dequeuePacket = !MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex);
    }

    if (MAC_GENERIC_DEBUG) {
        printf("ACKFINISHED , node - %d at %s\n", node->nodeId, clockStr);
    }

    //UserCodeEndACKFinished

    if (dequeuePacket) {
        if (resumeTransmit ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_DEQUEUEPACKET;
        enterGenericMacDequeuePacket(node, dataPtr, msg);
        return;
    } else if (resumeTransmit) {
        if (dequeuePacket ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_CHANNELPOLICYBRANCH;
        enterGenericMacChannelPolicyBranch(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacBusyToIdle(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartBusyToIdle
    Message *newMsg;
    //clocktype timerDelay = 0;

    if (MAC_GENERIC_DEBUG) {
        printf("Busy to Idle for node %d\n", node->nodeId);
    }

    dataPtr->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_GENERICMAC,
                           MSG_MAC_TimerExpired);

    MESSAGE_SetInstanceId(newMsg, (short) dataPtr->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(dataPtr->timerSequenceNumber));
    *((int*)MESSAGE_ReturnInfo(newMsg)) = dataPtr->timerSequenceNumber;

    dataPtr->csma_ca_idle_start = node->getNodeTime();
    dataPtr->csma_ca_timer_on = TRUE;


    MESSAGE_Send(node, newMsg, dataPtr->csma_ca_counter);


    //UserCodeEndBusyToIdle

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacSendCTS(Node* node,
                                   GenericMacData* dataPtr,
                                   Message *msg) {

    //UserCodeStartSendCTS

    Mac802Address destAddr;

    Message* pktToPhy;
    GenericMacControlFrame* hdr =
        (GenericMacControlFrame*) MESSAGE_ReturnPacket(msg);
    GenericMacControlFrame newHdr;
    clocktype delayAir;
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);

    GenericMacCancelTimer(node, dataPtr);

    MAC_CopyMac802Address(&destAddr , &hdr->sourceAddr );

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        sizeof(GenericMacControlFrame),
                        TRACE_GENERICMAC);

    newHdr.frameType = MGENERIC_CTS;

    MAC_CopyMac802Address(&newHdr.destAddr , &destAddr );
    MAC_CopyMac802Address(&newHdr.sourceAddr , &dataPtr->selfAddr );


    // Subtract RTS transmit time.
    newHdr.duration = (int) (hdr->duration);

    // Subtract off CTS transmit time from the total duration
    // to get the CTS timeout time.
    dataPtr->noResponseTimeoutDuration =  hdr->duration;
                                          //- dataPtr->extraPropDelay
                                          //- dataPtr->ctsOrAckTransmissionDuration
                                          //- dataPtr->extraPropDelay;
                                          //+ EPSILON_DELAY;

    memcpy(MESSAGE_ReturnPacket(pktToPhy), &(newHdr), sizeof(GenericMacControlFrame));

    GenericMacChangeStatus(node, dataPtr, MGENERIC_X_CTS);

    if (MAC_GENERIC_DEBUG) {
        printf("sending *CTS* at node %d, time %s\n", node->nodeId, clockStr);
    }
    // NOTE: we need to add the delay time for slot boundary at wfdata
    delayAir = genericMacGetDelayUntilAirborne(node, dataPtr);
    PHY_StartTransmittingSignal(node, dataPtr->myMacData->phyNumber,
                                pktToPhy, TRUE, delayAir);



    //UserCodeEndSendCTS

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacSendACK(Node* node,
                                   GenericMacData* dataPtr,
                                   Message *msg) {

    //UserCodeStartSendACK
    Message* pktToPhy;
    GenericMacControlFrame newHdr;
    GenericMacFrameHdr* hdr = (GenericMacFrameHdr*)
                                        MESSAGE_ReturnPacket(msg);


    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&sourceAddr , &hdr->sourceAddr );

    char clockStr[100];
    //char destAddrStr[100];
    clocktype delayAir = 0;

    ctoa(node->getNodeTime(), clockStr);

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        sizeof(GenericMacControlFrame),
                        TRACE_GENERICMAC);

    newHdr.frameType = MGENERIC_ACK;

    MAC_CopyMac802Address(&newHdr.destAddr , &sourceAddr );
    MAC_CopyMac802Address(&newHdr.sourceAddr , &dataPtr->selfAddr);

    newHdr.duration = 0;

    memcpy(MESSAGE_ReturnPacket(pktToPhy), &(newHdr),
           sizeof(GenericMacControlFrame));

    GenericMacChangeStatus(node, dataPtr, MGENERIC_X_ACK);
    //printf("SIZEOF ACKpacket = %d\n", sizeof(pktToPhy));

    if (MAC_GENERIC_DEBUG) {
        printf("sending *ACK* - node %d to ",node->nodeId);
        MAC_PrintMacAddr(&newHdr.destAddr);
        printf(" at %s\n", clockStr);
    }

    // NOTE: need to change time out duration at wfack
    delayAir = genericMacGetDelayUntilAirborne(node, dataPtr);
    PHY_StartTransmittingSignal(node, dataPtr->myMacData->phyNumber,
                                pktToPhy, TRUE, delayAir);

    Int32 seqMatchType = GenericMacCompareSeqenceNumber(
                                                node,
                                                dataPtr,
                                                sourceAddr,
                                                hdr->seqNo);

    if (seqMatchType == 0)
    {
        if (MAC_GENERIC_DEBUG)
        {
            printf("Passing up MAC, node %d\n", node->nodeId);
        }
        GenericMacHandOffSuccessfullyReceivedUnicast(node, dataPtr, msg);
    }
    else if (seqMatchType == 1)
    {
        GenericMacSeqNoEntry* entry = GenericMacGetSeqNo(
                                                node,
                                                dataPtr,
                                                sourceAddr);
        entry->fromSeqNo = hdr->seqNo;
        if (MAC_GENERIC_DEBUG)
        {
            printf("Passing up MAC, node %d\n", node->nodeId);
        }
        GenericMacHandOffSuccessfullyReceivedUnicast(node, dataPtr, msg);
    }
    else
    {
        // Wrong sequence number, Drop.
        if (MAC_GENERIC_DEBUG)
        {
            printf("INCORRECT sequence number, node %d, Expecting %d, Got %d\n",
                  node->nodeId, 
                  GenericMacGetSeqNo(node, dataPtr, sourceAddr)->fromSeqNo,
                  hdr->seqNo);
        }
        MESSAGE_Free(node, msg);
    }

    //UserCodeEndSendACK

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacACKProcess(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartACKProcess
    Mac802Address sourceAddr;
    char clockStr[100];

    ctoa(node->getNodeTime(), clockStr);

    if (MAC_GENERIC_DEBUG) {
        printf("*processing* ACK - node %d - time - %s\n", node->nodeId, clockStr);
    }

#ifdef CYBER_LIB
    // In IA, the adversary may inject any packets, including
    // ACK packets.  It is invalid for QualNet to quit or warn
    // on any injected packet.
    if (dataPtr->status != MGENERIC_S_WFACK)
    {
        // Ignore the packet
        MESSAGE_Free(node, msg);
        return;
    }
#else // CYBER_LIB
    ERROR_Assert(dataPtr->status == MGENERIC_S_WFACK,
                 "GenericMacAckProcess: "
                 "Received unexpected ACK packet.\n");
#endif // CYBER_LIB

    GenericMacCancelTimer(node, dataPtr);

    MAC_CopyMac802Address(&sourceAddr ,
                          &dataPtr->waitingForAckOrCtsFromAddress );

    ERROR_Assert(dataPtr->currentMessage != NULL,
        "GenericMacACKProcess: "
        "There is no message in local buffer.\n");

    GenericMacDataComplete(node,dataPtr,
                           sourceAddr, TRUE);

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
    dataPtr->yieldType = MGENERIC_YIELD_DEQUEUE;


    MESSAGE_Free(node, msg);


    //UserCodeEndACKProcess

    dataPtr->state = GENERICMAC_YIELD;
    enterGenericMacYield(node, dataPtr, msg);
}

static void enterGenericMacDataFinished(Node* node,
                                        GenericMacData* dataPtr,
                                        Message *msg) {

    //UserCodeStartDataFinished
    BOOL broadcast = FALSE;
    BOOL ackMode = dataPtr->ackMode;
    BOOL queueWaiting = FALSE;
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);
    broadcast = (dataPtr->status == MGENERIC_X_BROADCAST);

    if (!ackMode || broadcast) {
        GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
        dataPtr->yieldType = MGENERIC_YIELD_DEQUEUE;
    }

    if (!broadcast && ackMode) {
        GenericMacChangeStatus(node, dataPtr, MGENERIC_S_WFACK);
    }

    if (!MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex)) {
        queueWaiting = TRUE;
    }


    if (MAC_GENERIC_DEBUG) {
        if (broadcast) {
            printf("DATAFINISHED - BROADCAST - NODE %d, time - %s\n", node->nodeId, clockStr);
        } else {
            printf("DATAFINISHED - UNICAST - NODE %d, time - %s\n", node->nodeId, clockStr);
        }
    }

    //UserCodeEndDataFinished

    if (!broadcast && ackMode) {
        dataPtr->state = GENERICMAC_SETTIMER;
        enterGenericMacSetTimer(node, dataPtr, msg);
        return;
    } else {
        // broadcast || !ackMode

        dataPtr->RC = 0;

        if (dataPtr->channelAccess != PSMA){
            GenericMacDecreaseCW(node, dataPtr);
        }

        dataPtr->state = GENERICMAC_YIELD;
        enterGenericMacYield(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacCTSProcess(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartCTSProcess
    GenericMacControlFrame* hdr =
            (GenericMacControlFrame*) MESSAGE_ReturnPacket(msg);

#ifdef CYBER_LIB
    // In IA, the adversary may inject any packets, including
    // CTS packets.  It is invalid for QualNet to quit or warn
    // on any injected packet.
    if (dataPtr->status != MGENERIC_S_WFCTS)
    {
        // Ignore the packet
        MESSAGE_Free(node, msg);
        return;
    }
#else // CYBER_LIB
    ERROR_Assert(dataPtr->status == MGENERIC_S_WFCTS,
                 "GenericMacReceivePacketFromPhy: "
                 "Received unexpected CTS packet.\n");
#endif // CYBER_LIB


    ERROR_Assert(MAC_IsIdenticalMac802Address(
                                    &hdr->sourceAddr,
                                    &dataPtr->currentNextHopAddress),
                 "GenericMacReceivePacketFromPhy: "
                 "CTS source does not match RTS destination.\n");

    if (MAC_GENERIC_DEBUG) {
        printf("\n*processing* CTS at node %d\n", node->nodeId);
        printf("source : ");
        MAC_PrintMacAddr(&hdr->sourceAddr);
        printf(" currentNextHop :");
        MAC_PrintMacAddr(&dataPtr->currentNextHopAddress);
        printf("\n");
    }

    GenericMacCancelTimer(node, dataPtr);
    MESSAGE_Free(node, msg);


    //UserCodeEndCTSProcess

    dataPtr->state = GENERICMAC_SENDPACKET;
    enterGenericMacSendPacket(node, dataPtr, msg);
}

static void enterGenericMacIdle(Node* node,
                                GenericMacData* dataPtr,
                                Message *msg) {

    //UserCodeStartIdle

    //UserCodeEndIdle


}

static void enterGenericMacRTSProcess(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartRTSProcess
    BOOL waitingResponse = (dataPtr->status >= MGENERIC_S_WFCTS) &&
                           (dataPtr->status <= MGENERIC_S_WFACK);

    if (MAC_GENERIC_DEBUG) {
        printf("*processing* RTS at node %d\n", node->nodeId);
    }

    //UserCodeEndRTSProcess

    if (!waitingResponse) {
        dataPtr->state = GENERICMAC_SENDCTS;
        enterGenericMacSendCTS(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacIdleToBusy(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartIdleToBusy
    clocktype timePassed = node->getNodeTime() - dataPtr->csma_ca_idle_start;

    if (dataPtr->csma_ca_timer_on) {

        GenericMacCancelTimer(node, dataPtr);
        if (MAC_GENERIC_DEBUG) {
            printf("Idle to Busy for node %d\n", node->nodeId);
        }

        dataPtr->csma_ca_counter = dataPtr->csma_ca_counter - timePassed;

        ERROR_Assert(dataPtr->csma_ca_counter >= 0,
                     "MacGenericIdleToBusy : Error, counter has negative value\n");
    }


    //UserCodeEndIdleToBusy

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacSendRTS(Node* node,
                                   GenericMacData* dataPtr,
                                   Message *msg) {

    //UserCodeStartSendRTS
    GenericMacControlFrame* hdr;
    Message* pktToPhy;

    Mac802Address destAddr;

    clocktype packetTransmissionDuration;
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);
    clocktype delayAir = 0;


    MAC_CopyMac802Address(&destAddr , &dataPtr->currentNextHopAddress );


    ERROR_Assert(dataPtr->currentMessage != NULL,
        "GenericMacSendRTS: "
        "There is no message in the local buffer.\n");

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        sizeof(GenericMacControlFrame),
                        TRACE_GENERICMAC);

    hdr = (GenericMacControlFrame*) MESSAGE_ReturnPacket(pktToPhy);

    hdr->frameType = MGENERIC_RTS;

    MAC_CopyMac802Address(&hdr->sourceAddr , &dataPtr->selfAddr );
    MAC_CopyMac802Address(&hdr->destAddr , &destAddr );
    MAC_CopyMac802Address(&dataPtr->waitingForAckOrCtsFromAddress ,
                         &destAddr );

    packetTransmissionDuration =
            GenericMacCalcFragmentTransmissionDuration(dataPtr->bandwidth,
                                            MESSAGE_ReturnPacketSize(dataPtr->currentMessage));


    hdr->duration = (int)
            (dataPtr->extraPropDelay +
             packetTransmissionDuration +
             dataPtr->extraPropDelay);


    GenericMacChangeStatus(node, dataPtr, MGENERIC_X_RTS);

    if (MAC_GENERIC_DEBUG) {
        printf("sending *RTS* at node %d, time %s\n", node->nodeId, clockStr);
    }

    // NOTE: need to change rts time out duration at wf_cts
    delayAir = genericMacGetDelayUntilAirborne(node, dataPtr);
    PHY_StartTransmittingSignal(node, dataPtr->myMacData->phyNumber,
                                pktToPhy, TRUE, delayAir);



    //UserCodeEndSendRTS

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacRTSFinished(Node* node,
                                       GenericMacData* dataPtr,
                                       Message *msg) {

    //UserCodeStartRTSFinished
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_WFCTS);

    if (MAC_GENERIC_DEBUG) {
        printf("finished SENDING RTS at node %d, time %s\n",
                                                                                node->nodeId, clockStr);
    }

    //UserCodeEndRTSFinished

    dataPtr->state = GENERICMAC_SETTIMER;
    enterGenericMacSetTimer(node, dataPtr, msg);
}

static void enterGenericMacSetTimer(Node* node,
                                    GenericMacData* dataPtr,
                                    Message *msg) {

    //UserCodeStartSetTimer
    Message *newMsg;
    clocktype timerDelay = 0;
    char timerStr[100];

    BOOL phyIdle = PHY_GetStatus(node,
                                       dataPtr->myMacData->phyNumber) == PHY_IDLE;

    dataPtr->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(node,
                                MAC_LAYER, MAC_PROTOCOL_GENERICMAC,
                                MSG_MAC_TimerExpired);
    MESSAGE_SetInstanceId(newMsg,
                                            (short) dataPtr->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg,
                                                    sizeof(dataPtr->timerSequenceNumber));
    *((int*)MESSAGE_ReturnInfo(newMsg)) =
                                                           dataPtr->timerSequenceNumber;

    switch (dataPtr->status) {
        case MGENERIC_S_WFCTS:
            timerDelay = (2 * dataPtr->extraPropDelay) +
                     dataPtr->ctsOrAckTransmissionDuration
                     + genericMacGetDelayUntilAirborne(node, dataPtr);
                     //+ dataPtr->slot;

            if (MAC_GENERIC_DEBUG) {
                ctoa(timerDelay, timerStr);
                printf("TIMER SET for WFCTS, node %d, delay %s\n",
                                                                                node->nodeId, timerStr);
            }
            break;
        case MGENERIC_S_WFDATA:
            timerDelay = dataPtr->noResponseTimeoutDuration +
                genericMacGetDelayUntilAirborne(node, dataPtr);
                //dataPtr->slot;
            // consider the slot boundary

            if (MAC_GENERIC_DEBUG) {
                ctoa(timerDelay, timerStr);
                printf("TIMER SET for WFDATA, node %d, delay %s\n",
                                                                                node->nodeId, timerStr);
            }
            break;
        case MGENERIC_S_WFACK:
            timerDelay = (2 * dataPtr->extraPropDelay) +
                         dataPtr->ctsOrAckTransmissionDuration
                         + genericMacGetDelayUntilAirborne(node, dataPtr);
                         //+ dataPtr->slot;
            // consider the slot boundary;

            if (MAC_GENERIC_DEBUG) {
                ctoa(timerDelay, timerStr);
                printf("TIMER SET for waitingAck, node %d, delay %s\n",
                                                                                node->nodeId, timerStr);
            }
            break;
        case MGENERIC_S_BO:
            timerDelay = dataPtr->BO;

            if ((dataPtr->channelAccess == CSMA_CA)||
                    (dataPtr->channelAccess == SL_CSMA_CA)) {
                dataPtr->csma_ca_counter = timerDelay;
                if (phyIdle) {
                    if (MAC_GENERIC_DEBUG) {
                        printf("Starting CSMACA backoff, node %d,"
                                "We are IDLE, timer ON\n", node->nodeId);
                    }
                    dataPtr->csma_ca_idle_start = node->getNodeTime();
                    dataPtr->csma_ca_timer_on = TRUE;
                } else {
                    if (MAC_GENERIC_DEBUG) {
                        printf("Starting CSMACA backoff, node %d,"
                               "We are BUSY, timer OFF\n", node->nodeId);
                    }
                    dataPtr->csma_ca_idle_start = 0;
                }
                dataPtr->lastBOTimestamp = node->getNodeTime();
                if (MAC_GENERIC_DEBUG)
                {
                    printf("Node %d: reset lastBOTime %" TYPES_64BITFMT "d\n",
                           node->nodeId, dataPtr->lastBOTimestamp);
                }
            }

            if (MAC_GENERIC_DEBUG) {
                ctoa(timerDelay, timerStr);
                printf("TIMER SET for BACKOFF, node %d, delay %s\n",
                         node->nodeId, timerStr);
            }
            break;
        default:
            break;
    }

    ERROR_Assert(timerDelay != 0,
                 "GenericMacTimerDelay : Timer Delay cannot be zero.\n");
    MESSAGE_Send(node, newMsg, timerDelay);

    //UserCodeEndSetTimer

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacYield(Node* node,
                                 GenericMacData* dataPtr,
                                 Message *msg) {

    //UserCodeStartYield
    Message *newMsg;
    clocktype timerDelay = 0;
    GenericMacControlFrame* hdr;

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_YIELD);

    dataPtr->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER,
                                                        MAC_PROTOCOL_GENERICMAC,
                                                         MSG_MAC_TimerExpired);
    MESSAGE_SetInstanceId(newMsg,
                                            (short) dataPtr->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg,
                                                    sizeof(dataPtr->timerSequenceNumber));
    *((int*)MESSAGE_ReturnInfo(newMsg)) =
                                                                dataPtr->timerSequenceNumber;

    ERROR_Assert(dataPtr->yieldType != MGENERIC_YIELD_NONE,
                 "MacGenericYield : No yield type specified\n");

    if ((dataPtr->yieldType == MGENERIC_YIELD_RTS) ||
        (dataPtr->yieldType == MGENERIC_YIELD_DATA)) {
        timerDelay = dataPtr->extraPropDelay +
                     dataPtr->ctsOrAckTransmissionDuration
                     + genericMacGetDelayUntilAirborne(node, dataPtr);
        if (MAC_GENERIC_DEBUG) {
            printf("***YIELDING*** for RTS/DATA at node %d\n", node->nodeId);
        }

    } else if (dataPtr->yieldType == MGENERIC_YIELD_CTS) {
        hdr = (GenericMacControlFrame*) MESSAGE_ReturnPacket(msg);
        timerDelay = dataPtr->extraPropDelay +
                     hdr->duration
                     + genericMacGetDelayUntilAirborne(node, dataPtr);

        if (MAC_GENERIC_DEBUG) {
            printf("***YIELDING*** for CTS at node %d\n", node->nodeId);
        }
    } else if (dataPtr->yieldType == MGENERIC_YIELD_DEQUEUE) {
        if (dataPtr->slot){
            timerDelay = dataPtr->slot;
        }else{
            timerDelay = 5 * MILLI_SECOND;
                //20 * MILLI_SECOND;
        }

        if (MAC_GENERIC_DEBUG) {
            printf("***YIELDING*** for DEQUEUE at node %d\n", node->nodeId);
        }
    } else {
        ERROR_Assert(0,
                     "MacGenericYield : Unknown Yield Type\n");
    }

    dataPtr->yieldType = MGENERIC_YIELD_NONE;

    MESSAGE_Send(node, newMsg, timerDelay);

    //UserCodeEndYield

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

static void enterGenericMacHandleTimeout(Node* node,
                                         GenericMacData* dataPtr,
                                         Message *msg) {

    //UserCodeStartHandleTimeout
    unsigned timerSequenceNumber = *(int*)MESSAGE_ReturnInfo(msg);
    BOOL retransmit = FALSE;
    BOOL dequeuePacket = FALSE;
    BOOL startBackoff = FALSE;
    BOOL resumeTransmit = FALSE;
    //ctoa(node->getNodeTime(), clockStr);

    if (timerSequenceNumber == dataPtr->timerSequenceNumber) {
        switch (dataPtr->status) {
            case MGENERIC_S_WFCTS:
                if (MAC_GENERIC_DEBUG) {
                    printf("Handling timeout for WFCTS at node %d\n",
                                                                                                node->nodeId);
                }

            case MGENERIC_S_WFACK:
                if (MAC_GENERIC_DEBUG) {
                    printf("Handling timeout for WFACK at node %d\n",
                                                                                                node->nodeId);
                }
                dataPtr->BO = 0;
                startBackoff = TRUE;
                break;
            case MGENERIC_S_BO:
                if (MAC_GENERIC_DEBUG) {
                    printf("Handling timeout for BACKOFF at node %d,"
                                                          "RETRANSMITTING\n", node->nodeId);
                }
                dataPtr->BO = 0;
                GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
                retransmit = TRUE;
                if ((dataPtr->channelAccess == CSMA_CA)||
                        (dataPtr->channelAccess == SL_CSMA_CA)) {
                    dataPtr->csma_ca_timer_on = FALSE;
                }
                break;
            case MGENERIC_S_WFDATA:
                if (MAC_GENERIC_DEBUG) {
                    printf("Handling timeout for WFDATA at node %d\n",
                                                                                                node->nodeId);
                }
                GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
                if (dataPtr->currentMessage != NULL) {
                    resumeTransmit = TRUE;
                } else {
                    dequeuePacket = !MAC_OutputQueueIsEmpty(node,
                                                        dataPtr->myMacData->interfaceIndex);
                }
                break;
            case MGENERIC_S_YIELD:
                if (MAC_GENERIC_DEBUG) {
                    printf("YIELD TIMEOUT CALLED at node %d\n", node->nodeId);
                }
                GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
                if (dataPtr->currentMessage != NULL) {
                    resumeTransmit = TRUE;
                } else {
                    dequeuePacket = !MAC_OutputQueueIsEmpty(node,
                                                        dataPtr->myMacData->interfaceIndex);
                }
                break;
            default:
                break;
        }
    } else {
        if (MAC_GENERIC_DEBUG) {
            printf("TIMEOUT *CANCELLED* at node %d\n", node->nodeId);
        }
    }


    //UserCodeEndHandleTimeout

    if (dequeuePacket) {
        if (resumeTransmit || retransmit ||
            startBackoff ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                        "from same state.");
        }
        dataPtr->state = GENERICMAC_DEQUEUEPACKET;
        enterGenericMacDequeuePacket(node, dataPtr, msg);
        return;
    } else if (resumeTransmit || retransmit) {
        if (dequeuePacket ||
            startBackoff ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                        "from same state.");
        }
        dataPtr->state = GENERICMAC_CHANNELPOLICYBRANCH;
        enterGenericMacChannelPolicyBranch(node, dataPtr, msg);
        return;
    } else if (startBackoff) {
        if (dequeuePacket ||
            resumeTransmit || retransmit ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                      "from same state.");
        }
        dataPtr->state = GENERICMAC_RETRANSMIT;
        enterGenericMacRetransmit(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);
        return;
    }
}


static void enterGenericMacDequeuePacket(Node* node,
                                         GenericMacData* dataPtr,
                                         Message *msg) {

    //UserCodeStartDequeuePacket
    TosType priority;
    int networkType;
    IpHeaderType* ipHeader;
    int interfaceIndex = dataPtr->myMacData->interfaceIndex;

    if (dataPtr->currentMessage != NULL)
        return;

    dataPtr->pktsToSend++;
    dataPtr->state = GENERICMAC_DEQUEUEPACKET;
    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_START);

    ERROR_Assert(dataPtr->currentMessage == NULL,
                 "MacGenericDequeuePacket: "
                 "A packet exists in the local buffer.\n");

    ERROR_Assert(!MAC_OutputQueueIsEmpty(node,
                                                        dataPtr->myMacData->interfaceIndex),
                 "MacGenericDequeuePacket: "
                 "No packet in the Queue.\n");

    MacHWAddress currentnextHopAddr;

    MAC_OutputQueueTopPacket(node,
                             interfaceIndex,
                             &dataPtr->currentMessage,
                             &currentnextHopAddr,
                             &networkType,
                             &priority);

    MAC_OutputQueueDequeuePacketForAPriority(node,
                                             interfaceIndex, priority,
                                             &dataPtr->currentMessage,
                                             &currentnextHopAddr,
                                             &networkType);

    ConvertVariableHWAddressTo802Address(node,&currentnextHopAddr,
                                         &dataPtr->currentNextHopAddress);


     MAC_CopyMac802Address(&dataPtr->ipNextHopAddr ,
                           &dataPtr->currentNextHopAddress );


    // Extract the source/dest address from the packet
    // Note: Only IP4 and MPLS assumed here.
    if (dataPtr->myMacData->mplsVar == NULL)
    {
        {
            // Treat as an IP message
            char *payload = MESSAGE_ReturnPacket(dataPtr->currentMessage);
            if (LlcIsEnabled(node, (int)DEFAULT_INTERFACE))
            {
                payload = payload + sizeof(LlcHeader);
            }
            ipHeader = (IpHeaderType*)payload;
        }
    }
#ifdef ENTERPRISE_LIB
    else {
        // Treat this packet as an MPLS message
        char *aHeader;
        LlcHeader* llcHeader = NULL;
        aHeader = (MESSAGE_ReturnPacket(dataPtr->currentMessage)) ;
        if (LlcIsEnabled(node, (int)DEFAULT_INTERFACE))
        {
            llcHeader = (LlcHeader*) aHeader;
            aHeader = aHeader + sizeof(LlcHeader);
        }
        if (llcHeader->etherType == PROTOCOL_TYPE_MPLS)
        {
            aHeader = aHeader + sizeof(Mpls_Shim_LabelStackEntry);
        }

        ipHeader = (IpHeaderType*)aHeader;
    }
#endif // ENTERPRISE_LIB

    dataPtr->ipSourceAddr = ipHeader->ip_src;
    dataPtr->ipDestAddr = ipHeader->ip_dst;


    if (MAC_IsBroadcastMac802Address(&dataPtr->currentNextHopAddress)){
            memcpy(&dataPtr->currentNextHopAddress,ANY_MAC802,
                                                                                sizeof(Mac802Address));
    }

    if (MAC_GENERIC_DEBUG) {
        //printf("Message Packet : %d\n",
        //MESSAGE_ReturnPacket(dataPtr->currentMessage));
        printf("DEQUEUING node %d, destination address :",
                        node->nodeId);
        MAC_PrintMacAddr(&dataPtr->currentNextHopAddress);
        printf("\n");
    }

    //UserCodeEndDequeuePacket

    dataPtr->state = GENERICMAC_CHANNELPOLICYBRANCH;
    enterGenericMacChannelPolicyBranch(node, dataPtr, msg);
}

static void enterGenericMacPacketProcess(Node* node,
                                         GenericMacData* dataPtr,
                                         Message *msg) {

    //UserCodeStartPacketProcess
    BOOL dequeuePacket = FALSE;
    GenericMacFrameHdr* hdr =
                            (GenericMacFrameHdr*) MESSAGE_ReturnPacket(msg);
    //NodeAddress sourceAddr = hdr->sourceAddr;
    BOOL ack = dataPtr->ackMode;
    BOOL broadcast = (MAC_IsBroadcastMac802Address(&hdr->destAddr));
    BOOL transmitAck = FALSE;
    BOOL resumeTransmit = FALSE;
    char clockStr[100];
    ctoa(node->getNodeTime(), clockStr);

    if (MAC_GENERIC_DEBUG) {
        printf("*processing* Data Packet at node %d, time %s\n",
                                                                                node->nodeId, clockStr);
    }

    // The old `currentMessage' is actually `currentOutMessage'
    // which is my message waiting for ACK from my neighbor.
    // We need a `currentInMessage' which is to be ACKed
    // if ARQ from my neighbor towards me happens again.
    if (dataPtr->currentInMessage != NULL)
    {
        MESSAGE_Free(node, dataPtr->currentInMessage);
    }
    dataPtr->currentInMessage = MESSAGE_Duplicate(node, msg);

    if ((dataPtr->status != MGENERIC_S_WFDATA) &&
       (GenericMacIsWaitingForResponseState(dataPtr->status)) &&
       (dataPtr->rtsCtsMode))
    {
        if (MAC_GENERIC_DEBUG) {
            printf("Freeing message at node %d\n", node->nodeId);
        }
        MESSAGE_Free(node, msg);
        transmitAck = FALSE;
    } else {
        if (broadcast || !ack) {
            if (MAC_GENERIC_DEBUG) {
                if (broadcast) {
                    printf("received broadcast/non ack, checking queue at %d\n",
                                                                                               node->nodeId);
                } else if (!broadcast && !ack) {
                    printf("received unicast (ack disabled) packet node at %d,"
                                                               "checking queue\n", node->nodeId);
                }
            }


            GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);

            GenericMacHandOffSuccessfullyReceivedBroadcast(
                    node, dataPtr, msg);

            if (dataPtr->status == MGENERIC_S_IDLE) {
                if (dataPtr->currentMessage != NULL) {
                    resumeTransmit = TRUE;
                } else {
                    dequeuePacket = !MAC_OutputQueueIsEmpty(node,
                                                         dataPtr->myMacData->interfaceIndex);
                }
            }

            transmitAck = FALSE;
        } else {
            if (MAC_GENERIC_DEBUG) {
                printf("received unicast (ack enabled) packet at node %d\n",
                                                                                                node->nodeId);
            }
            transmitAck = TRUE;
            GenericMacCancelTimer(node, dataPtr);
        }
    }


    //UserCodeEndPacketProcess

    if (ack && !broadcast) {
        if (dequeuePacket ||
            resumeTransmit ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                       "from same state.");
        }
        dataPtr->state = GENERICMAC_SENDACK;
        enterGenericMacSendACK(node, dataPtr, msg);
        return;
    } else if (dequeuePacket) {
        if ((ack && !broadcast) ||
            resumeTransmit ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                      "from same state.");
        }
        dataPtr->state = GENERICMAC_DEQUEUEPACKET;
        enterGenericMacDequeuePacket(node, dataPtr, msg);
        return;
    } else if (resumeTransmit) {
        if ((ack && !broadcast) ||
            dequeuePacket ) {
            ERROR_ReportError("Multiple true guards found in transitions"
                                                                                      "from same state.");
        }
        dataPtr->state = GENERICMAC_CHANNELPOLICYBRANCH;
        enterGenericMacChannelPolicyBranch(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacRetransmit(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartRetransmit
    BOOL backoff = FALSE;
    BOOL dequeue = FALSE;
     BOOL yield = FALSE;
    char boStr[100];

    dataPtr->RC++;

    if (dataPtr->RC < dataPtr->retryLimit) {
        if (MAC_GENERIC_DEBUG) {
            printf("Starting backoff #%d at node %d\n",
                                  dataPtr->RC, node->nodeId);
        }

        if (dataPtr->channelAccess == PSMA)
        {
            yield = TRUE;
            dataPtr->yieldType = MGENERIC_YIELD_DATA;
        }
        else
        {
             backoff = TRUE;
             GenericMacIncreaseCW(node, dataPtr);
             GenericMacSetBO(node, dataPtr);

             if (MAC_GENERIC_DEBUG) {
                ctoa(dataPtr->BO, boStr);
                printf("After setting BO : %s\n", (char*) boStr);
             }

             GenericMacChangeStatus(node, dataPtr, MGENERIC_S_BO);
        }

        //back off, then retransmit
    } else {
        if (MAC_GENERIC_DEBUG) {
            printf("BACKOFF is over the count, dropping packet at node %d\n", node->nodeId);
        }


        dataPtr->pktsLost++;
        dataPtr->RC = 0;

        if (MAC_GENERIC_DEBUG) {
            //printf("*** Message Packet : %d\n", MESSAGE_ReturnPacket(dataPtr->currentMessage));
        }


        MacHWAddress nextHopHWAddr;
        Convert802AddressToVariableHWAddress(node,&nextHopHWAddr,
                                             &dataPtr->ipNextHopAddr);

        MAC_NotificationOfPacketDrop(node,
                                     nextHopHWAddr,
                                     dataPtr->myMacData->interfaceIndex,
                                     dataPtr->currentMessage);
                                     //msg); // BUG FIX

        if (!MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex)) {
            dequeue = TRUE;
        }
        GenericMacSeqNoEntry* entry = GenericMacGetSeqNo(
                                                    node,
                                                    dataPtr,
                                                    dataPtr->ipNextHopAddr);
        entry->toSeqNo++;
        dataPtr->currentMessage = NULL;

        memcpy(&dataPtr->currentNextHopAddress , ANY_MAC802 ,
                                                      sizeof(Mac802Address));
        memcpy(&dataPtr->ipNextHopAddr , ANY_MAC802 ,
                                                      sizeof(Mac802Address));
        dataPtr->ipDestAddr = ANY_DEST;
        dataPtr->ipSourceAddr = ANY_DEST;
        dataPtr->csma_ca_counter = 0;
        dataPtr->csma_ca_idle_start = 0;



        GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
        GenericMacDecreaseCW(node, dataPtr);

        //check for new packets to send ***WITH backoff?

        /////////////////////////
        dataPtr->yieldType = MGENERIC_YIELD_DEQUEUE;
        dataPtr->state = GENERICMAC_YIELD;
        enterGenericMacYield(node, dataPtr, msg);
        return ;
        /////////////////////////
    }


    //UserCodeEndRetransmit

    if (dequeue) {
        if (backoff ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_DEQUEUEPACKET;
        enterGenericMacDequeuePacket(node, dataPtr, msg);
        return;
    } else if (backoff) {
        if (dequeue ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = GENERICMAC_SETTIMER;
        enterGenericMacSetTimer(node, dataPtr, msg);
        return;
    }
    else if (yield){
        dataPtr->state = GENERICMAC_YIELD;
        enterGenericMacYield(node,dataPtr,msg);
    }
    else {
        dataPtr->state = GENERICMAC_IDLE;
        enterGenericMacIdle(node, dataPtr, msg);
        return;
    }
}

static void enterGenericMacSendPacket(Node* node,
                                      GenericMacData* dataPtr,
                                      Message *msg) {

    //UserCodeStartSendPacket
    BOOL ack = dataPtr->ackMode;
    //BOOL rtsCts = dataPtr->rtsCtsMode;

    Mac802Address destAddr;

    char clockStr[100];
    clocktype delayAir;

    ctoa(node->getNodeTime(), clockStr);
    MAC_CopyMac802Address(&destAddr , &dataPtr->currentNextHopAddress);

    ERROR_Assert(dataPtr->currentMessage != NULL,
                 "GenericMacTransmitDataFrame: "
                 "There is no message in the local buffer.\n");


      if (MAC_IsBroadcastMac802Address(&destAddr)) {
        Message* dequeuedPacket;
        GenericMacFrameHdr* hdr;

        dequeuedPacket = dataPtr->currentMessage;

        MESSAGE_AddHeader(node,
                          dequeuedPacket,
                          sizeof(GenericMacFrameHdr),
                          TRACE_GENERICMAC);

        hdr = (GenericMacFrameHdr*) MESSAGE_ReturnPacket(dequeuedPacket);

        hdr->frameType = MGENERIC_DATA;

        MAC_CopyMac802Address(&hdr->destAddr , &destAddr );
        MAC_CopyMac802Address(&hdr->sourceAddr , &dataPtr->selfAddr);

        hdr->duration = 0;

        dataPtr->currentMessage = NULL;

        memcpy(&dataPtr->currentNextHopAddress , ANY_MAC802 ,sizeof(Mac802Address));
        memcpy(&dataPtr->ipNextHopAddr , ANY_MAC802 , sizeof(Mac802Address));
        dataPtr->ipDestAddr = ANY_DEST;
        dataPtr->ipSourceAddr = ANY_DEST;
        dataPtr->csma_ca_counter = 0;
        dataPtr->csma_ca_idle_start = 0;

        GenericMacChangeStatus(node, dataPtr, MGENERIC_X_BROADCAST);

        if (MAC_GENERIC_DEBUG) {
            printf("*SENDINGDATA* (BROADCAST), node %d, time %s\n", node->nodeId, clockStr);
            //printf("**** Message Packet : %d\n", MESSAGE_ReturnPacket(dequeuedPacket));
        }
        delayAir = genericMacGetDelayUntilAirborne(node, dataPtr);



        PHY_StartTransmittingSignal(node, dataPtr->myMacData->phyNumber,
                                    dequeuedPacket, TRUE, delayAir);

    } else {
        GenericMacSeqNoEntry *entry;
        GenericMacFrameHdr* hdr;
        Message* pktToPhy = MESSAGE_Duplicate(node, dataPtr->currentMessage);

        MESSAGE_AddHeader(node,
                          pktToPhy,
                          sizeof(GenericMacFrameHdr),
                          TRACE_GENERICMAC);

        hdr = (GenericMacFrameHdr*) MESSAGE_ReturnPacket(pktToPhy);

        hdr->frameType = MGENERIC_DATA;
        MAC_CopyMac802Address(&hdr->destAddr, &destAddr );
        MAC_CopyMac802Address(&hdr->sourceAddr , &dataPtr->selfAddr);
        //end

        if (!ack) {

            MESSAGE_Free(node, dataPtr->currentMessage);

            dataPtr->currentMessage = NULL;


            memcpy(&dataPtr->currentNextHopAddress , ANY_MAC802 ,
                                                      sizeof(Mac802Address));
            memcpy(&dataPtr->ipNextHopAddr , ANY_MAC802 ,
                                                      sizeof(Mac802Address));
            dataPtr->ipDestAddr = ANY_DEST;
            dataPtr->ipSourceAddr = ANY_DEST;
            hdr->duration = 0;
            dataPtr->csma_ca_counter = 0;
            dataPtr->csma_ca_idle_start = 0;
        }

        if (ack) {

            MAC_CopyMac802Address(&dataPtr->waitingForAckOrCtsFromAddress ,
                                  &destAddr );
            //end

            entry = GenericMacGetSeqNo(node, dataPtr, destAddr);

            ERROR_Assert(entry,
                         "GenericMacTransmitDataFrame: "
                         "Unable to find or create sequence number entry.\n");

            hdr->seqNo = entry->toSeqNo;

            ERROR_Assert(entry,
                "GenericMacTransmitDataFrame: "
                "Unable to find or create sequence number entry.\n");
                hdr->seqNo = entry->toSeqNo;

            hdr->duration = (int)
                // the duration for ack msg, no need to consider
                // the delay for slot boundary
                (dataPtr->extraPropDelay +
                dataPtr->ctsOrAckTransmissionDuration +
                dataPtr->extraPropDelay);
        }

        GenericMacChangeStatus(node, dataPtr, MGENERIC_X_UNICAST);

        if (MAC_GENERIC_DEBUG) {


            printf("*SENDINGDATA* (UNICAST and/or ACK) at node %d to",node->nodeId);
            MAC_PrintMacAddr(&destAddr);
            printf(", time %s\n",clockStr);
        }

        delayAir = genericMacGetDelayUntilAirborne(node, dataPtr);


        PHY_StartTransmittingSignal(node, dataPtr->myMacData->phyNumber,
                                    pktToPhy, TRUE, delayAir);
    }


    //UserCodeEndSendPacket

    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}


//Statistic Function Definitions
static void GenericMacInitStats(Node* node,
                                GenericMacStatsType *stats,
                                GenericMacData* dataPtr) {
    dataPtr->pktsToSend = 0;
    dataPtr->pktsLost = 0;
    if (node->guiOption) {
    }

}

void GenericMacPrintStats(Node* node,
                          GenericMacData* dataPtr,
                          int interfaceIndex) {
    char buf[MAX_STRING_LENGTH];
    char chan_accessBuf[20];

    if (dataPtr->channelAccess == CSMA){
        sprintf(chan_accessBuf,"%s", "CSMA");
    }else if (dataPtr->channelAccess == CSMA_CA){
        sprintf(chan_accessBuf,"%s", "CSMA-CA");
    }else if (dataPtr->channelAccess == PSMA){
        sprintf(chan_accessBuf,"%s", "PSMA");
    }else if (dataPtr->channelAccess == SL_CSMA){
        sprintf(chan_accessBuf,"%s", "SLOT-CSMA");
    }else if (dataPtr->channelAccess == SL_CSMA_CA){
        sprintf(chan_accessBuf,"%s", "SLOT-CSMA-CA");
    }

    sprintf(buf, "Packets from network = %d",
            dataPtr->pktsToSend);

    IO_PrintStat(node, "GENERICMAC", chan_accessBuf, ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Packets lost = %d",
            dataPtr->pktsLost);
    IO_PrintStat(node, "GENERICMAC", chan_accessBuf, ANY_DEST, interfaceIndex, buf);

}

void GenericMacRunTimeStat(Node* node, GenericMacData* dataPtr) {
}

// this function returns the delay time required to delay transmission so
// that in slotted csma mode, packets always transmit at start of slot
// if genericmac protocols, delay = 0
// if slot genericmac protocols, delay = time_difference_with_next_slot
static clocktype genericMacGetDelayUntilAirborne(
                            Node* node,
                            GenericMacData* dataPtr)
{
    clocktype additionalDelay = 0;
    clocktype nclk = node->getNodeTime();
    // if not run in slotted mode, no additional delay
    if (dataPtr->slot == 0)
    {
        // Do not have extra delay
        // GenericMac always call PHY_StartTransmittingSignal()
        // specifiying a mac-controlled delay. Further, this protocol
        // has set a minimum lookahead time that we must obey. So,
        // return delay of TURNAROUND_TIME
        additionalDelay = PHY_ABSTRACT_RX_TX_TURNAROUND_TIME;
    }
    else
    {
        //shift to the boundary of timeslot
        additionalDelay = dataPtr->slot - (nclk % dataPtr->slot);

#ifdef PARALLEL //Parallel
        if (additionalDelay < PHY_ABSTRACT_RX_TX_TURNAROUND_TIME)
        {
            additionalDelay = additionalDelay + dataPtr->slot;
        }

        ERROR_Assert(additionalDelay >= PHY_ABSTRACT_RX_TX_TURNAROUND_TIME,
            "Parallel requires minimum lookahead time, may need to delay one more slot");
#endif //endParallel
    }

    return additionalDelay;
}

// based on the input simulation time, return a new time at
// the boundary of the specified simulation time
static
clocktype GenericMacGetTimeSlot(GenericMacData* dataPtr, clocktype _simulT)
{
    if (dataPtr->slot){
        return (clocktype)ceil((double)_simulT/dataPtr->slot) * dataPtr->slot;
    }else return _simulT;
}


void
GenericMacInitScheduler(Node* node, GenericMacData* dataPtr)
{
    //if not running in slotted mode, or user entered 0 for slot duration,
    //skip timer update to avoid infinite loop
    if (dataPtr->slot == 0)
    {
        return;
    }

    Message *newMsg;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_GENERICMAC,
                           MSG_GenericMacSlotTimerExpired);

    MESSAGE_SetInstanceId(newMsg, (short) dataPtr->myMacData->interfaceIndex);

    MESSAGE_Send(node, newMsg, dataPtr->slot);

#ifdef PARALLEL //Parallel
    if (dataPtr->slot != 0)
    {
        PARALLEL_SetLookaheadHandleEOT(node, dataPtr->lookaheadHandle,
            node->getNodeTime() + dataPtr->slot);
    }



    //printf ("node %d intf %d set lookaheadtime %d\n",
    //    node->nodeId, dataPtr->myMacData->interfaceIndex, dataPtr->slot);
    //fflush(stdout);
#endif //endParallel

    if (MAC_GENERIC_SCHEDULER) {
        printf("node %d initialize scheduler at %f, next slot at %f\n",
            node->nodeId, (double)node->getNodeTime()/SECOND,
            (double)(node->getNodeTime() + dataPtr->slot)/SECOND);
    }
}

static void
GenericMacUpdateScheduler(Node* node, GenericMacData* dataPtr, Message* msg)
{
    MESSAGE_Send(node, msg, dataPtr->slot);

#ifdef PARALLEL //Parallel
    if (dataPtr->slot != 0)
    {
        PARALLEL_SetLookaheadHandleEOT(node, dataPtr->lookaheadHandle,
            node->getNodeTime() + dataPtr->slot);
    }



    //printf ("node %d intf %d set lookaheadtime %d\n",
    //    node->nodeId, dataPtr->myMacData->interfaceIndex, dataPtr->slot);
    //fflush(stdout);
#endif //endParallel

    if (MAC_GENERIC_SCHEDULER) {
        printf("node %d updated timer at %f, next slot at %f\n",
            node->nodeId, (double)node->getNodeTime()/SECOND,
            (double)(node->getNodeTime() + dataPtr->slot)/SECOND);
    }
}
void
GenericMacInit(Node* node, int interfaceIndex,
               const NodeInput* nodeInput, const int nodesInSubnet) {

    GenericMacData *dataPtr = NULL;
    Message *msg = NULL;

    //UserCodeStartInit
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    const unsigned char Invalid_mac_addr[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xfe};


    dataPtr = (GenericMacData*) MEM_malloc(sizeof(GenericMacData));
    memset(dataPtr, 0, sizeof(GenericMacData));

    RANDOM_SetSeed(dataPtr->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_GENERICMAC,
                   interfaceIndex);

    IO_ReadString(node, node->nodeId, interfaceIndex, nodeInput, "CHANNEL-ACCESS", &wasFound, buf);
    if (!wasFound) {
        dataPtr->channelAccess = PSMA;
    } else {
        if (!strcmp(buf, "CSMA")) {
#ifdef ADDON_BOEINGFCS
           ERROR_Assert(!node->partitionData->isRunningInParallel(),
            "CSMA not supported on multiple partitions. Use SLOT-CSMA");
#endif
            dataPtr->channelAccess = CSMA;
        } else if (!strcmp(buf,"CSMA-CA")) {
#ifdef ADDON_BOEINGFCS
           ERROR_Assert(!node->partitionData->isRunningInParallel(),
            "CSMA-CA not supported on multiple partitions. Use SLOT-CSMA-CA");
#endif
            dataPtr->channelAccess = CSMA_CA;
        } else if (!strcmp(buf, "SLOT-CSMA")) {
            dataPtr->channelAccess = SL_CSMA;
        } else if (!strcmp(buf, "SLOT-CSMA-CA")) {
            dataPtr->channelAccess = SL_CSMA_CA;
        }
        else {
            dataPtr->channelAccess = PSMA;
        }
    }
    dataPtr->p_access = 0.0;

    dataPtr->tsbWinLength = 0;
    dataPtr->maxTranxTime = 0;


    // -1: for initialization
    dataPtr->waitTimeInTSB = -1;
    // 0: for initialization
    dataPtr->phyIdle = 0;

    if ((dataPtr->channelAccess == SL_CSMA)
        || (dataPtr->channelAccess == SL_CSMA_CA)){

        // read the time-slot definition
        IO_ReadTime(node,node->nodeId, interfaceIndex, nodeInput, "SLOT",
                    &wasFound, &(dataPtr->slot));

        if (!wasFound){
            // use the default value
            dataPtr->slot = MGENERIC_SLOT;
        }
        ERROR_Assert(dataPtr->slot > 0, "Genericmac: SLOT should be > 0.");
        dataPtr->backoff_mul = dataPtr->slot;
    }
    else {
        // the other generic mac protocols
        dataPtr->backoff_mul = MGENERIC_BO_MULTIPLIER;
        dataPtr->slot = 0;
    }

    IO_ReadString(node,node->nodeId, interfaceIndex, nodeInput, "RTS-CTS", &wasFound, buf);

    if (!wasFound) {
        strcpy(buf, "FALSE");
    }

    if (!strcmp(buf, "YES")) {
       dataPtr->rtsCtsMode = TRUE;
    } else {
       dataPtr->rtsCtsMode = FALSE;
    }

    IO_ReadString(node,node->nodeId, interfaceIndex, nodeInput, "ACK", &wasFound, buf);

    if (!wasFound) {
        strcpy(buf, "FALSE");
    }

    if (!strcmp(buf, "YES")) {
       dataPtr->ackMode = TRUE;
    } else {
       dataPtr->ackMode = FALSE;
    }


    dataPtr->myMacData = node->macData[interfaceIndex];
    dataPtr->myMacData->macVar = (void*) dataPtr;

    ConvertVariableHWAddressTo802Address(node,
            &node->macData[dataPtr->myMacData->interfaceIndex]->macHWAddr,
            &dataPtr->selfAddr);

    GenericMacChangeStatus(node, dataPtr, MGENERIC_S_IDLE);
    //Backoff
    dataPtr->BO = 0;
    dataPtr->CW = MGENERIC_CW_MIN;

    dataPtr->csma_ca_counter = 0;
    dataPtr->csma_ca_idle_start = 0;
    dataPtr->csma_ca_timer_on = FALSE;

    //Ack Variables
    dataPtr->RC = 0;
    dataPtr->ctsOrAckTransmissionDuration = 0;

    memcpy(&dataPtr->waitingForAckOrCtsFromAddress,
           Invalid_mac_addr, sizeof(Mac802Address));


    //Sequence Vars
    dataPtr->timerSequenceNumber = 0;
    dataPtr->seqNoHead = NULL;

    //Delays and Durations
    dataPtr->extraPropDelay = dataPtr->myMacData->propDelay;
    dataPtr->bandwidth = dataPtr->myMacData->bandwidth;
    dataPtr->ctsOrAckTransmissionDuration =
                                PHY802_11b_SYNCHRONIZATION_TIME +
                                PHY_GetTransmissionDuration(
                                    node, dataPtr->myMacData->phyNumber,
                                    PHY_GetTxDataRateType(node, dataPtr->myMacData->phyNumber),
                                    sizeof(GenericMacControlFrame));
                                //((sizeof(GenericMacControlFrame) * SECOND) / (dataPtr->bandwidth));


    dataPtr->dataTransmissionDuration = PHY802_11b_SYNCHRONIZATION_TIME +
                                PHY_GetTransmissionDuration(
                                                node, dataPtr->myMacData->phyNumber,
                                                PHY_GetTxDataRateType(node, dataPtr->myMacData->phyNumber),
                                                sizeof(GenericMacFrameHdr));
                                //((sizeof(GenericMacFrameHdr) * SECOND) / (dataPtr->bandwidth));

    //printf("bandwidth = %d\n", dataPtr->bandwidth);
    //printf("ctsorAckTransmissionDuration = %d\n", dataPtr->ctsOrAckTransmissionDuration);
    //printf("extrapopdelay = %d\n", dataPtr->extraPropDelay);
    //printf("control frame size = %d\n", sizeof(GenericMacControlFrame));
    //printf("PHY802_11b_SYNCHRONIZATION_TIME = %d\n", PHY802_11b_SYNCHRONIZATION_TIME);

    //dataPtr->dataTransmissionDuration =
    //                            GenericMacCalcFragmentTransmissionDuration(dataPtr->bandwidth, 1);
    dataPtr->noResponseTimeoutDuration = 0;


    //Message Variables
    dataPtr->currentMessage = NULL;

    memcpy(&dataPtr->currentNextHopAddress , ANY_MAC802 ,
                                            sizeof(Mac802Address));
    memcpy(&dataPtr->ipNextHopAddr , ANY_MAC802 , sizeof(Mac802Address));
    dataPtr->ipDestAddr = ANY_DEST;
    dataPtr->ipSourceAddr = ANY_DEST;

    IO_ReadInt(node,node->nodeId, interfaceIndex, nodeInput,
            "RETRY-LIMIT", &wasFound, &(dataPtr->retryLimit));
    if (!wasFound) {
        // use the default values
        dataPtr->retryLimit = MGENERIC_RETRY_LIMIT;
    }

#if 0
    if ((dataPtr->channelAccess == SL_CSMA) ||(dataPtr->channelAccess == SL_CSMA_CA)){
        dataPtr->retryLimit = MGENERIC_SLOT_RETRY_LIMIT;
    }
    else {
        // for the other protocol
        dataPtr->retryLimit = MGENERIC_RETRY_LIMIT;
    }
#endif

    dataPtr->initStats = TRUE;

    if (MAC_GENERIC_DEBUG) {
        printf("INIT STATE for node %d, *complete*\n", node->nodeId);
    }

    //UserCodeEndInit
    if (dataPtr->initStats) {
        GenericMacInitStats(node, &(dataPtr->stats), dataPtr);
    }
#ifdef PARALLEL //Parallel
    // note: the 5000 might have to adjusted for this protocol
    //PARALLEL_SetProtocolIsNotEOTCapable(node);
    if (dataPtr->slot != 0)
    {
        // Slots aren't being used, so we can't use EOT - for example,
        // we don't know when our first event would be scheduled for
        dataPtr->lookaheadHandle = PARALLEL_AllocateLookaheadHandle (node);
        PARALLEL_AddLookaheadHandleToLookaheadCalculator(
            node, dataPtr->lookaheadHandle, 0);
        PARALLEL_SetLookaheadHandleEOT(node, dataPtr->lookaheadHandle,
            PHY_ABSTRACT_RX_TX_TURNAROUND_TIME);
    }
    else
    {
        PARALLEL_SetProtocolIsNotEOTCapable(node);
    }
    // Also sepcific a minimum lookahead, in case EOT can't be used in the sim.
    PARALLEL_SetMinimumLookaheadForInterface(node,
         PHY_ABSTRACT_RX_TX_TURNAROUND_TIME);

    //printf ("node %d intf %d set lookaheadtime %d\n",
    //    node->nodeId, dataPtr->myMacData->interfaceIndex, PHY_ABSTRACT_RX_TX_TURNAROUND_TIME);
    //fflush(stdout);

#endif //endParallel

    // Init the slot Scheduler
    GenericMacInitScheduler(node, dataPtr);
    dataPtr->state = GENERICMAC_IDLE;
    enterGenericMacIdle(node, dataPtr, msg);
}

// Genericmac interface: to change the channel access mode & protocol
void MacGenericMacSetChanAccess(Node* node,
                                int interfaceIndex,
                                BOOL nl_short) {

    ERROR_Assert(interfaceIndex < node->numberInterfaces,
        "MacGenericMac incorrect interfaceIndex");
    GenericMacData* dataPtr = (GenericMacData*) node->macData[interfaceIndex]->macVar;

    ERROR_Assert(dataPtr, "MacGenericMacSetChanAccess");

    //ERROR_Assert(dataPtr, "macData for node %d interface %d NULL",
    //    node->nodeId, interfaceIndex);

    if (nl_short == FALSE)
    {
        // initialize NORMAL
        if (dataPtr->channelAccess != SL_CSMA_CA){
            printf("Node %d interface %d will change Channl Access to SLOT-CSMA-CA \n",
                node->nodeId, interfaceIndex);
        }
        dataPtr->channelAccess = SL_CSMA_CA;

        if (dataPtr->tsbWinLength == 0){
            // Use the default value
            dataPtr->tsbWinLength = TSB_WINDOW_LENGTH;
        }
        if (dataPtr->maxTranxTime == 0){
            // Use the default value
            dataPtr->maxTranxTime = MAXIMUM_TRANSMIT_TIME;
        }

    }else {
        if (dataPtr->channelAccess != SL_CSMA){
            printf("Node %d interface %d will change Channl Access to SLOT-CSMA \n",
                node->nodeId, interfaceIndex);
        }
        dataPtr->channelAccess = SL_CSMA;

        if (dataPtr->p_access == 0){
            // Use the default value
            dataPtr->p_access = SLOT_CSMA_ACCESS_PROBABILITY;
        }
    }
    if (dataPtr->slot == 0){
        dataPtr->slot = MGENERIC_SLOT;
    }
    dataPtr->backoff_mul = dataPtr->slot;
}

void
GenericMacFinalize(Node* node, int interfaceIndex) {

    GenericMacData* dataPtr = (GenericMacData*) node->macData[interfaceIndex]->macVar;

    //UserCodeStartFinalize
    dataPtr->printStats = TRUE;

    //UserCodeEndFinalize

    if (dataPtr->printStats) {
        GenericMacPrintStats(node, dataPtr, interfaceIndex);
    }

    if (dataPtr->currentInMessage != NULL)
    {
        MESSAGE_Free(node, dataPtr->currentInMessage);
    }
}


void
GenericMacProcessEvent(Node* node, int interfaceIndex, Message* msg) {

    GenericMacData* dataPtr;

    int event;
    dataPtr = (GenericMacData*) node->macData[interfaceIndex]->macVar;
    event = msg->eventType;

    //don't want to have this in switch statement, since sloT
    //update timer message do not set state,
    if (event ==  MSG_GenericMacSlotTimerExpired)
    {
        // Update scheduler for next slot
        GenericMacUpdateScheduler(node, dataPtr, msg);

        return;
    }
    else
    {
        int timerSequenceNumber = *((int*)MESSAGE_ReturnInfo(msg));
        if ((unsigned int)timerSequenceNumber != dataPtr->timerSequenceNumber) {
            if (MAC_GENERIC_DEBUG) {
                printf("TIMEOUT *CANCELLED* at node %d\n", node->nodeId);
            }
            MESSAGE_Free(node, msg);
            msg = NULL;
            return;
        }
    }


    switch (dataPtr->state) {
        case GENERICMAC_CHANNELPOLICYBRANCH:
            assert(FALSE);
            break;
        case GENERICMAC_CSMA_CSMACA_BRANCH:
            assert(FALSE);
            break;
        case GENERICMAC_PSMABRANCH:
            assert(FALSE);
            break;
        case GENERICMAC_CTSFINISHED:
            assert(FALSE);
            break;
        case GENERICMAC_ACKFINISHED:
            assert(FALSE);
            break;
        case GENERICMAC_BUSYTOIDLE:
            assert(FALSE);
            break;
        case GENERICMAC_SENDCTS:
            assert(FALSE);
            break;
        case GENERICMAC_SENDACK:
            assert(FALSE);
            break;
        case GENERICMAC_ACKPROCESS:
            assert(FALSE);
            break;
        case GENERICMAC_DATAFINISHED:
            assert(FALSE);
            break;
        case GENERICMAC_CTSPROCESS:
            assert(FALSE);
            break;
        case GENERICMAC_IDLE:
            switch (event) {
                case MSG_MAC_TimerExpired:
                    dataPtr->state = GENERICMAC_HANDLETIMEOUT;
                    enterGenericMacHandleTimeout(node, dataPtr, msg);
                    MESSAGE_Free(node, msg);
                    break;
                default:
                    assert(FALSE);
            }
            break;
        case GENERICMAC_RTSPROCESS:
            assert(FALSE);
            break;
        case GENERICMAC_IDLETOBUSY:
            assert(FALSE);
            break;
        case GENERICMAC_SENDRTS:
            assert(FALSE);
            break;
        case GENERICMAC_RTSFINISHED:
            assert(FALSE);
            break;
        case GENERICMAC_SETTIMER:
            assert(FALSE);
            break;
        case GENERICMAC_YIELD:
            assert(FALSE);
            break;
        case GENERICMAC_HANDLETIMEOUT:
            assert(FALSE);
            break;
        case GENERICMAC_DEQUEUEPACKET:
            assert(FALSE);
            break;
        case GENERICMAC_PACKETPROCESS:
            assert(FALSE);
            break;
        case GENERICMAC_RETRANSMIT:
            assert(FALSE);
            break;
        case GENERICMAC_SENDPACKET:
            assert(FALSE);
            break;
        default:
            ERROR_ReportError("Illegal transition");
            break;
    }

}

