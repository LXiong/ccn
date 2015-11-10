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
#include "mac_aloha.h"
#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel


//UserCodeStartBodyData
#define ALOHA_NEXT_PKT_DELAY   50 * MILLI_SECOND
#define ALOHA_ACK_SIZE   sizeof(AlohaHeader)
#define ALOHA_MAX_TIMEOUTS   5
#define ALOHA_PROPAGATION_DELAY   150 * MICRO_SECOND
#define ALOHA_DELAY_TIMER   5 * MILLI_SECOND
#define DEBUG   0


//UserCodeEndBodyData

//Statistic Function Declarations
static void AlohaInitStats(Node* node, AlohaStatsType *stats);
void AlohaPrintStats(Node *node, AlohaData* dataPtr, int interfaceIndex);

//Utility Function Declarations

//State Function Declarations
static void enterAlohaIdleState(Node* node, AlohaData* dataPtr, Message* msg);
static void enterAlohaHandleData(Node* node, AlohaData* dataPtr, Message* msg);
static void enterAlohaRetransmit(Node* node, AlohaData* dataPtr, Message* msg);
static void enterAlohaReceivePacket(Node* node, AlohaData* dataPtr, Message* msg);
static void enterAlohaTransmit(Node* node, AlohaData* dataPtr, Message* msg);
static BOOL enterAlohaCheckTransmit(Node* node, AlohaData* dataPtr, Message* msg);
static void enterAlohaReceiveACK(Node* node, AlohaData* dataPtr, Message* msg);

//Utility Function Definitions
//UserCodeStart

//UserCodeEnd

//State Function Definitions
void MacAlohaNetworkLayerHasPacketToSend(Node* node, AlohaData* dataPtr) {
    //UserCodeStartNetworkLayerHasPacketToSend
    char clockStr[100];

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s]: NetworkLayerHasPacketToSend!\n", node->nodeId, clockStr);
    }


    //UserCodeEndNetworkLayerHasPacketToSend

    if ((dataPtr->mode == ALOHA_IDLE) && (dataPtr->txMode == ALOHA_TX_IDLE)) {
        dataPtr->state = ALOHA_TRANSMIT;
        enterAlohaTransmit(node, dataPtr, NULL);
        return;
    } else {
        dataPtr->state = ALOHA_IDLE_STATE;
        enterAlohaIdleState(node, dataPtr, NULL);
        return;
    }
}


void MacAlohaReceivePacketFromPhy(Node* node, AlohaData* dataPtr, Message* msg) {
    //UserCodeStartReceivePacketFromPhy
    AlohaHeader *hdr = (AlohaHeader *) MESSAGE_ReturnPacket(msg);

    MacHWAddress destHW;
    Convert802AddressToVariableHWAddress(node, &destHW, &hdr->destAddr);
    BOOL isUnicastFrame = FALSE;

    if (NetworkIpIsUnnumberedInterface(
                    node, dataPtr->myMacData->interfaceIndex))
    {
        isUnicastFrame = MAC_IsMyAddress(node, &destHW);
    }
    else
    {
       isUnicastFrame = MAC_IsMyHWAddress(node,
                                       dataPtr->myMacData->interfaceIndex,
                                       &destHW);
    }
    BOOL isBroadcastFrame = MAC_IsBroadcastHWAddress(&destHW);



    //UserCodeEndReceivePacketFromPhy

    if ((isUnicastFrame || isBroadcastFrame)) {
        dataPtr->state = ALOHA_HANDLEDATA;
        enterAlohaHandleData(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = ALOHA_IDLE_STATE;
        enterAlohaIdleState(node, dataPtr, msg);
        MESSAGE_Free(node, msg);
        return;
    }
}


void MacAlohaReceivePhyStatusChangeNotification(Node* node, AlohaData* dataPtr, PhyStatusType oldPhyStatus, PhyStatusType newPhyStatus) {
    //UserCodeStartReceivePhyStatusChangeNotification
    BOOL isQueueEmpty = MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex);
    clocktype retransmitDelay;
    Message *ackTimer;
    clocktype bandwith = dataPtr->myMacData->bandwidth;
    char clockStr[100], jitStr[100], retxDelayStr[100];
    clocktype jitter = (clocktype) (RANDOM_erand(dataPtr->seed) * ALOHA_DELAY_TIMER);

    if (oldPhyStatus == PHY_TRANSMITTING) {
        assert(newPhyStatus != PHY_TRANSMITTING);
        if (dataPtr->txMode == ALOHA_TX_DATA) {
            dataPtr->txMode = ALOHA_TX_IDLE;
            dataPtr->mode = ALOHA_EXPECTING_ACK;

            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("ALOHA [%d] [%s]: ReceivePhyStatus - Finished Sending Packet, Now Expecting ACK\n", node->nodeId, clockStr);
            }

            ackTimer = MESSAGE_Alloc(node, MAC_LAYER, 0,
                                     MSG_MAC_TimerExpired);
            dataPtr->retransmitTimer = ackTimer;
            retransmitDelay = (clocktype) ((((2 * ALOHA_PROPAGATION_DELAY) + (ALOHA_ACK_SIZE / bandwith)) * pow(2.0,dataPtr->numTimeouts)) + jitter);

            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                ctoa(jitter, jitStr);
                printf("JITTER - %s\n", jitStr);
                ctoa(retransmitDelay, retxDelayStr);
                printf("ALOHA [%d] [%s]: ReceivePhyStatus - Retransmit delay - %s\n", node->nodeId, clockStr, retxDelayStr);
            }

            MESSAGE_SetInstanceId(ackTimer,
                (short)dataPtr->myMacData->interfaceIndex);
            MESSAGE_Send(node, ackTimer, retransmitDelay);
        } else {
            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("Aloha [%d] [%s]: ReceivePhyStatus - Finished Sending ACK, Now Idle ACK\n", node->nodeId, clockStr);
            }

            assert (dataPtr->txMode == ALOHA_TX_ACK);
            //assert (dataPtr->mode == ALOHA_IDLE);
            dataPtr->txMode = ALOHA_TX_IDLE;

        }
    }




    //UserCodeEndReceivePhyStatusChangeNotification

    if ((oldPhyStatus == PHY_TRANSMITTING) && !isQueueEmpty && (dataPtr->mode == ALOHA_IDLE)) {
        dataPtr->state = ALOHA_TRANSMIT;
        enterAlohaTransmit(node, dataPtr, NULL);
        return;
    } else {
        dataPtr->state = ALOHA_IDLE_STATE;
        enterAlohaIdleState(node, dataPtr, NULL);
        return;
    }
}


static void enterAlohaIdleState(Node* node,
                                AlohaData* dataPtr,
                                Message *msg) {

    //UserCodeStartIdleState

    //UserCodeEndIdleState


}

static void enterAlohaHandleData(Node* node,
                                 AlohaData* dataPtr,
                                 Message *msg) {

    //UserCodeStartHandleData
    AlohaHeader *hdr = (AlohaHeader *) msg->packet;
    BOOL isACK = hdr->ACK;
    BOOL expectingACK = (dataPtr->mode == ALOHA_EXPECTING_ACK);



    //UserCodeEndHandleData

    if (!isACK && !expectingACK) {
        if (isACK ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = ALOHA_RECEIVEPACKET;
        enterAlohaReceivePacket(node, dataPtr, msg);
        return;
    } else if (isACK) {
        if (!isACK && !expectingACK ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = ALOHA_RECEIVEACK;
        enterAlohaReceiveACK(node, dataPtr, msg);
        MESSAGE_Free(node, msg);
        return;
    } else {
        dataPtr->state = ALOHA_IDLE_STATE;
        enterAlohaIdleState(node, dataPtr, msg);
        MESSAGE_Free(node, msg);
        return;
    }
}

static void enterAlohaRetransmit(Node* node,
                                 AlohaData* dataPtr,
                                 Message *msg) {

    //UserCodeStartRetransmit
    Message* retransmitPkt;
    char clockStr[100];

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s]: Retransmitting Packet\n", node->nodeId, clockStr);
    }

    if (dataPtr->mode != ALOHA_EXPECTING_ACK) {
        if (DEBUG) {
            ctoa(node->getNodeTime(), clockStr);
            printf("ALOHA [%d] [%s]: Retransmitting Failed - not exepcting ACK\n", node->nodeId, clockStr);
        }
    } else {
        assert(dataPtr->currentFrame);
        retransmitPkt = MESSAGE_Duplicate(node, dataPtr->currentFrame);

        dataPtr->retransmitTimer = NULL;

        //ctoa(node->getNodeTime(), clockStr);
        //printf("ALOHA [%d] [%s]: Retransmitting - Checking Timeouts\n", node->nodeId, clockStr);
        if (dataPtr->numTimeouts >= ALOHA_MAX_TIMEOUTS) {
            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("Aloha [%d] [%s]: Retransmitting Packet Failed - too many timeouts\n", node->nodeId, clockStr);
            }
            dataPtr->stats.pktsDropped++;
            dataPtr->numTimeouts = 0;
            MESSAGE_Free(node, dataPtr->currentFrame);
            dataPtr->currentFrame = NULL;
            MESSAGE_Free(node, retransmitPkt);
            retransmitPkt = NULL;
            dataPtr->mode = ALOHA_IDLE;
            dataPtr->txMode = ALOHA_TX_IDLE;

            memset(&dataPtr->ACKAddress, 255, MAC_ADDRESS_LENGTH_IN_BYTE);

        } else {
            //printf("ALOHA [%d] [%s]: Retransmitting - Timeouts Passed\n", node->nodeId, clockStr);
            AlohaHeader* hdr = (AlohaHeader*) MESSAGE_ReturnPacket(retransmitPkt);

            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("ALOHA [%d] : ReTransmit , sending packet from ",node->nodeId);
                MAC_PrintMacAddr(&hdr->sourceAddr);
                printf(" to ");
                MAC_PrintMacAddr(&hdr->destAddr);
                printf("\n");
            }

            dataPtr->txMode = ALOHA_TX_DATA;
            assert(dataPtr->mode == ALOHA_EXPECTING_ACK);
            PHY_StartTransmittingSignal(node,
                                        dataPtr->myMacData->phyNumber,
                                        retransmitPkt,
                                        FALSE,
                                        0);

            if (DEBUG) {
                ctoa(node->getNodeTime(), clockStr);
                printf("Aloha [%d] [%s]: Retransmitting Packet SUCCESFULL - Packet Sent\n", node->nodeId, clockStr);
            }
            dataPtr->numTimeouts++;
        }
    }

    //UserCodeEndRetransmit

    dataPtr->state = ALOHA_IDLE_STATE;
    enterAlohaIdleState(node, dataPtr, msg);
}

static void enterAlohaReceivePacket(Node* node,
                                    AlohaData* dataPtr,
                                    Message *msg) {

    //UserCodeStartReceivePacket
    char clockStr[100];

    AlohaHeader *hdr = (AlohaHeader *) MESSAGE_ReturnPacket(msg);
    AlohaHeader* ackHdr = (AlohaHeader *) MEM_malloc(sizeof(AlohaHeader));
    MacHWAddress sourceHWAddr;


    Message* ACK = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, ACK, sizeof(AlohaHeader), TRACE_ALOHA);

    MAC_CopyMac802Address(&ackHdr->sourceAddr, &hdr->destAddr);
    MAC_CopyMac802Address(&ackHdr->destAddr, &hdr->sourceAddr);

    ackHdr->ACK = TRUE;

    memcpy(MESSAGE_ReturnPacket(ACK), ackHdr, sizeof(AlohaHeader));
    MEM_free(ackHdr);

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s] : ReceivedPacket : Message Received, sending ACK\n",  node->nodeId, clockStr);
        printf("ALOHA [%d] [%s] : ReceivedPacket : ACK Parameters , From ", node->nodeId, clockStr);
        MAC_PrintMacAddr(&hdr->destAddr);
        printf("To");
        MAC_PrintMacAddr(&hdr->sourceAddr);
        printf("\n");
    }

    dataPtr->txMode = ALOHA_TX_ACK;

    PHY_StartTransmittingSignal(
                node,
                dataPtr->myMacData->phyNumber,
                ACK,
                FALSE,
                0);

    MESSAGE_RemoveHeader(node, msg, sizeof(AlohaHeader), TRACE_ALOHA);

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s]: ReceivedPacket : Sending packet to upper Layers\n", node->nodeId, clockStr);
    }

    Convert802AddressToVariableHWAddress(node, &sourceHWAddr,
                                                           &hdr->sourceAddr);

    MAC_HandOffSuccessfullyReceivedPacket(
                            node,
                            dataPtr->myMacData->interfaceIndex,
                            msg,
                            &sourceHWAddr);
    dataPtr->stats.pktsReceived++;

    //UserCodeEndReceivePacket

    dataPtr->state = ALOHA_IDLE_STATE;
    enterAlohaIdleState(node, dataPtr, msg);
}

static void enterAlohaTransmit(Node* node,
                               AlohaData* dataPtr,
                               Message *msg) {

    //UserCodeStartTransmit
    int networkType;

    MacHWAddress nextHopMacAddr;
    int priority;

    AlohaHeader *hdr;

    Message* transmitFrame;

    if (MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex))
     {
        assert(0);
    }

    MAC_OutputQueueDequeuePacket(
                node,
                dataPtr->myMacData->interfaceIndex,
                &transmitFrame,
                &nextHopMacAddr,
                &networkType,
                &priority);

    assert(transmitFrame != NULL);

    MESSAGE_AddHeader(node, transmitFrame, sizeof(AlohaHeader), TRACE_ALOHA);

    hdr  = (AlohaHeader *) MESSAGE_ReturnPacket(transmitFrame);
    hdr->payloadSize = MESSAGE_ReturnPacketSize(transmitFrame)
                            - sizeof(AlohaHeader);

    ConvertVariableHWAddressTo802Address(
               node,
               &node->macData[dataPtr->myMacData->interfaceIndex]->macHWAddr,
               &hdr->sourceAddr);

    //ctoa(node->getNodeTime(), clockStr);
    //printf("ALOHA [%d] [%s] Transmit, Interface Index - %d\n",
    //          node->nodeId, clockStr, dataPtr->myMacData->interfaceIndex);

    hdr->ACK = FALSE;


    ConvertVariableHWAddressTo802Address(node,
                                         &nextHopMacAddr,
                                         &hdr->destAddr);

    dataPtr->stats.pktsSent++;

    ConvertVariableHWAddressTo802Address(node,
                                         &nextHopMacAddr,
                                         &dataPtr->ACKAddress);


    dataPtr->numTimeouts = 0;

    dataPtr->currentFrame = MESSAGE_Duplicate(node, transmitFrame);

    dataPtr->txMode = ALOHA_TX_DATA;
    assert(dataPtr->mode == ALOHA_IDLE);

    if (DEBUG) {

        printf("ALOHA [%d] : Transmit , sending packet from  ", node->nodeId);

        MAC_PrintMacAddr(&hdr->sourceAddr);
        printf("To ");
        MAC_PrintMacAddr(&hdr->destAddr);
        printf("\n");
    }

    PHY_StartTransmittingSignal(
                node,
                dataPtr->myMacData->phyNumber,
                transmitFrame,
                FALSE,
                0);





    //UserCodeEndTransmit

    dataPtr->state = ALOHA_IDLE_STATE;
    enterAlohaIdleState(node, dataPtr, msg);
}

static BOOL enterAlohaCheckTransmit(Node* node,
                                    AlohaData* dataPtr,
                                    Message *msg) {

    //UserCodeStartCheckTransmit
    BOOL transmit = MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex);
    char clockStr[100];
    BOOL msgReuse = FALSE;

    assert(dataPtr->mode == ALOHA_YIELD);

    if (dataPtr->txMode == ALOHA_TX_ACK) {
        clocktype jitter = (clocktype) (RANDOM_erand(dataPtr->seed) * ALOHA_DELAY_TIMER);
        transmit = FALSE;
        MESSAGE_Send(node, msg, ALOHA_NEXT_PKT_DELAY + jitter);
        msgReuse = TRUE;
    } else {
        transmit = !MAC_OutputQueueIsEmpty(node, dataPtr->myMacData->interfaceIndex);
        dataPtr->mode = ALOHA_IDLE;
    }

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s]: Check Transmit, checking if que has packets to send\n", node->nodeId, clockStr);
    }




    //UserCodeEndCheckTransmit

    if (transmit) {
        dataPtr->state = ALOHA_TRANSMIT;
        enterAlohaTransmit(node, dataPtr, msg);
    } else {
        dataPtr->state = ALOHA_IDLE_STATE;
        enterAlohaIdleState(node, dataPtr, msg);
    }
    return msgReuse;
}

static void enterAlohaReceiveACK(Node* node,
                                 AlohaData* dataPtr,
                                 Message *msg) {

    //UserCodeStartReceiveACK
    AlohaHeader *hdr = (AlohaHeader *) msg->packet;

    Message *sendTimer;
    char clockStr[100];
    char delayStr[100];
    clocktype jitter = (clocktype) (RANDOM_erand(dataPtr->seed) * ALOHA_DELAY_TIMER);

    if (dataPtr->retransmitTimer != NULL) {
        MESSAGE_CancelSelfMsg(node, dataPtr->retransmitTimer);
        dataPtr->retransmitTimer = NULL;
    }

    if (DEBUG) {
        ctoa(node->getNodeTime(), clockStr);
        printf("ALOHA [%d] [%s]: ACK Received\n", node->nodeId, clockStr);
    }

    if ((dataPtr->mode == ALOHA_EXPECTING_ACK) &&
            MAC_IsIdenticalMac802Address(&dataPtr->ACKAddress,
                                         &hdr->sourceAddr)) {

        if (DEBUG) {
            ctoa(node->getNodeTime(), clockStr);
            printf("ALOHA [%d] [%s]: expecting source",
                                    node->nodeId,clockStr);
            MAC_PrintMacAddr(&dataPtr->ACKAddress);
            printf("actual source -");
            MAC_PrintMacAddr(&hdr->sourceAddr);
        }

        //assert(dataPtr->ACKAddress == lastHopAddress);
        memset(&dataPtr->ACKAddress, 255, sizeof(Mac802Address));
        dataPtr->numTimeouts = 0;
        if (dataPtr->currentFrame != NULL)
        {
            MESSAGE_Free(node, dataPtr->currentFrame);
            dataPtr->currentFrame = NULL;
        }

        //Initiate sending again.
        dataPtr->mode = ALOHA_YIELD;
        sendTimer = MESSAGE_Alloc(node, MAC_LAYER, 0,
                              MSG_MAC_CheckTransmit);

        MESSAGE_SetInstanceId(sendTimer,
            (short)dataPtr->myMacData->interfaceIndex);

        if (DEBUG) {
            ctoa(ALOHA_NEXT_PKT_DELAY, delayStr);
            ctoa(node->getNodeTime(), clockStr);
            printf("ALOHA [%d] [%s]: Initiating Sending after received ACK, delay - %s\n", node->nodeId, clockStr, delayStr);
        }

        //MESSAGE_Free(node, msg);
        MESSAGE_Send(node, sendTimer, ALOHA_NEXT_PKT_DELAY + jitter);

    } else {
        if (DEBUG) {
            ctoa(node->getNodeTime(), clockStr);
            printf("ALOHA [%d] [%s]: Not Expecting ACK from this address!\n", node->nodeId, clockStr);
        }
    }





    //UserCodeEndReceiveACK

    dataPtr->state = ALOHA_IDLE_STATE;
    enterAlohaIdleState(node, dataPtr, msg);
}


//Statistic Function Definitions
static void AlohaInitStats(Node* node, AlohaStatsType *stats) {
    if (node->guiOption) {
        stats->pktsSentId = GUI_DefineMetric("Aloha: Number of Packets Sent", node->nodeId, GUI_MAC_LAYER, 0, GUI_INTEGER_TYPE,GUI_CUMULATIVE_METRIC);
        stats->pktsReceivedId = GUI_DefineMetric("Aloha: Number of Packets Received", node->nodeId, GUI_MAC_LAYER, 0, GUI_INTEGER_TYPE,GUI_CUMULATIVE_METRIC);
        stats->pktsDroppedId = GUI_DefineMetric("Aloha: Number of Packets Dropped", node->nodeId, GUI_MAC_LAYER, 0, GUI_INTEGER_TYPE,GUI_CUMULATIVE_METRIC);
    }

    stats->pktsSent = 0;
    stats->pktsReceived = 0;
    stats->pktsDropped = 0;
}

void AlohaPrintStats(Node* node, AlohaData* dataPtr, int interfaceIndex) {
    AlohaStatsType *stats = &(dataPtr->stats);
    char buf[MAX_STRING_LENGTH];

    char statProtocolName[MAX_STRING_LENGTH];
    strcpy(statProtocolName,"Aloha");
    sprintf(buf, "Packets Sent to Channel = %d", stats->pktsSent);
    IO_PrintStat(node, "Mac", statProtocolName, ANY_DEST, interfaceIndex, buf);
    sprintf(buf, "Packets Received from Channel = %d", stats->pktsReceived);
    IO_PrintStat(node, "Mac", statProtocolName, ANY_DEST, interfaceIndex, buf);
    sprintf(buf, "Packets Dropped = %d", stats->pktsDropped);
    IO_PrintStat(node, "Mac", statProtocolName, ANY_DEST, interfaceIndex, buf);
}

void AlohaRunTimeStat(Node* node, AlohaData* dataPtr) {
    clocktype now = node->getNodeTime();

    if (node->guiOption) {
        GUI_SendIntegerData(node->nodeId, dataPtr->stats.pktsSentId, dataPtr->stats.pktsSent, now);
        GUI_SendIntegerData(node->nodeId, dataPtr->stats.pktsReceivedId, dataPtr->stats.pktsReceived, now);
        GUI_SendIntegerData(node->nodeId, dataPtr->stats.pktsDroppedId, dataPtr->stats.pktsDropped, now);
    }
}

void
AlohaInit(Node* node, int interfaceIndex, const NodeInput* nodeInput, const int nodesInSubnet) {

    AlohaData *dataPtr = NULL;
    Message *msg = NULL;

    //UserCodeStartInit
    dataPtr = (AlohaData *) MEM_malloc(sizeof(AlohaData));

    assert(dataPtr != NULL);

    memset(dataPtr, 0, sizeof(AlohaData));
    dataPtr->myMacData = node->macData[interfaceIndex];
    dataPtr->myMacData->macVar = (void *)dataPtr;
    dataPtr->initStats = TRUE;
    dataPtr->mode = ALOHA_IDLE;
    dataPtr->txMode = ALOHA_TX_IDLE;

    memset(&dataPtr->ACKAddress, 255, sizeof(Mac802Address));
    dataPtr->numTimeouts = 0;
    dataPtr->currentFrame = NULL;
    dataPtr->retransmitTimer = NULL;
    dataPtr->stats.pktsDropped = 0;

    RANDOM_SetSeed(dataPtr->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_ALOHA,
                   interfaceIndex);

    //UserCodeEndInit
    if (dataPtr->initStats) {
        AlohaInitStats(node, &(dataPtr->stats));
    }
    dataPtr->state = ALOHA_IDLE_STATE;
    enterAlohaIdleState(node, dataPtr, msg);

#ifdef PARALLEL //Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif //endParallel
}


void
AlohaFinalize(Node* node, int interfaceIndex) {

    AlohaData* dataPtr = (AlohaData*) node->macData[interfaceIndex]->macVar;

    //UserCodeStartFinalize
    dataPtr->printStats = dataPtr->myMacData->macStats;




    //UserCodeEndFinalize

    if (dataPtr->printStats) {
        AlohaPrintStats(node, dataPtr, interfaceIndex);
    }
}


void
AlohaProcessEvent(Node* node, int interfaceIndex, Message* msg) {

    AlohaData* dataPtr;

    int event;
    dataPtr = (AlohaData*) node->macData[interfaceIndex]->macVar;
    event = msg->eventType;
    BOOL msgReuse = FALSE;
    switch (dataPtr->state) {
        case ALOHA_IDLE_STATE:
            switch (event) {
                case MSG_MAC_TimerExpired:
                    dataPtr->state = ALOHA_RETRANSMIT;
                    enterAlohaRetransmit(node, dataPtr, msg);
                    break;
                case MSG_MAC_CheckTransmit:
                    dataPtr->state = ALOHA_CHECKTRANSMIT;
                    msgReuse = enterAlohaCheckTransmit(node, dataPtr, msg);
                    break;
                default:
                    assert(FALSE);
            }
            break;
        case ALOHA_HANDLEDATA:
            assert(FALSE);
            break;
        case ALOHA_RETRANSMIT:
            assert(FALSE);
            break;
        case ALOHA_RECEIVEPACKET:
            assert(FALSE);
            break;
        case ALOHA_TRANSMIT:
            assert(FALSE);
            break;
        case ALOHA_CHECKTRANSMIT:
            assert(FALSE);
            break;
        case ALOHA_RECEIVEACK:
            assert(FALSE);
            break;
        default:
            ERROR_ReportError("Illegal transition");
            break;
    }
    if (msgReuse == FALSE)
    {
        MESSAGE_Free(node, msg);
    }
}

