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

/*
 * TDMA has not completed the QA process.  The current model is provided
 * "as-is".
 */

/*
 *
 * TDMA (Time Division Multiple Access) Protocol
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "mac_tdma.h"
#include "network_ip.h"
#include "partition.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

#ifdef CYBER_LIB
#include "routing_anodr.h"
#endif // CYBER_LIB

#include "mac_dot11.h"

#define DEBUG 0

static const clocktype DefaultTdmaSlotDuration = (10 * MILLI_SECOND);
static const clocktype DefaultTdmaGuardTime = 0;
static const clocktype DefaultTdmaInterFrameTime = (1 * MICRO_SECOND);

/*
 * NAME:        MacTdmaHandlePromiscuousMode.
 *
 * PURPOSE:     Supports promiscuous mode sending remote packets to
 *              upper layers.
 *
 * PARAMETERS:  node, node using promiscuous mode.
 *              frame, packet to send to upper layers.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */
static
void MacTdmaHandlePromiscuousMode(
    Node* node,
    MacDataTdma* tdma,
    Message* frame,
    Mac802Address prevHop,
    Mac802Address destAddr)
{
    MacHWAddress prevHopHWAddr;
    MacHWAddress destHWAddr;

    if (frame->headerProtocols[frame->numberOfHeaders-1] == TRACE_TDMA)
    {
        MESSAGE_RemoveHeader(node, frame, sizeof(TdmaHeader), TRACE_TDMA);

        Convert802AddressToVariableHWAddress(node, &prevHopHWAddr, &prevHop);
        Convert802AddressToVariableHWAddress(node, &destHWAddr, &destAddr);

        MAC_SneakPeekAtMacPacket(node,
                             tdma->myMacData->interfaceIndex,
                             frame,
                             prevHopHWAddr,
                             destHWAddr);


        MESSAGE_AddHeader(node, frame, sizeof(TdmaHeader), TRACE_TDMA);
    }
}


static
void MacTdmaInitializeTimer(Node* node, MacDataTdma* tdma) {
    int i;

    if (DEBUG) {
        printf("partition %d, node %d in tdmainittimer\n", node->partitionData->partitionId, node->nodeId);
        fflush(stdout);
    }
    int initialStatus = (int)tdma->frameDescriptor[0];

    if (initialStatus == TDMA_STATUS_TX) {
        i = 0;
    }
    else {
        for (i = 0; i < tdma->numSlotsPerFrame; i++) {
            if ((int)tdma->frameDescriptor[i] != initialStatus) {
                break;
            }
        }
    }

    if (i == tdma->numSlotsPerFrame) {
#ifdef PARALLEL //Parallel
        // This node has no transmit slots
        PARALLEL_SetLookaheadHandleEOT(node,
            tdma->lookaheadHandle,
            CLOCKTYPE_MAX);
#endif //endParallel
        //
        // Schedule no timer
        //

        tdma->currentStatus = initialStatus;

        if (DEBUG) {
            printf("No initial timer sent for node %d.  "
                   "Initial Status matches all slots in frame\n", node->nodeId);
        }
    }
    else {
        Message *timerMsg;
        clocktype delay;

        delay = (tdma->slotDuration + tdma->guardTime) * i + tdma->interFrameTime;

        timerMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_TimerExpired);

        tdma->currentStatus = initialStatus;
        tdma->currentSlotId = 0;
        tdma->nextStatus = tdma->frameDescriptor[i];
        tdma->nextSlotId = i;

        tdma->timerMsg = timerMsg;
        tdma->timerExpirationTime = delay + node->getNodeTime();

        MESSAGE_SetInstanceId(timerMsg, (short)tdma->myMacData->interfaceIndex);
        MESSAGE_Send(node, timerMsg, delay);

#ifdef PARALLEL //Parallel
        PARALLEL_SetLookaheadHandleEOT(node,
            tdma->lookaheadHandle,
            node->getNodeTime() + delay + 5000);
#endif //endParallel
        if (DEBUG) {
            char clockStr[MAX_CLOCK_STRING_LENGTH];
            char clockStr2[MAX_CLOCK_STRING_LENGTH];

            ctoa(delay, clockStr);
            ctoa(node->getNodeTime(), clockStr2);
            printf("Initial timer sent for node [%d] with a delay of %s at %s\n\n", node->nodeId,
                    clockStr, clockStr2);
            printf("Slot duration, guard time, and interframe time are %"
                    TYPES_64BITFMT "d, %" TYPES_64BITFMT "d, and %"
                    TYPES_64BITFMT "d \n",
                   tdma->slotDuration, tdma->guardTime, tdma->interFrameTime);
            fflush(stdout);
        }
    }
}


static
void MacTdmaUpdateTimer(Node* node, MacDataTdma* tdma) {
    int i;
    int slotId = tdma->currentSlotId;
    clocktype delay = 0;

    ERROR_Assert(tdma->currentStatus == tdma->frameDescriptor[tdma->currentSlotId],
                 "TDMA: Current status is incorrect for the current time slot.");

    for (i = 0; i < tdma->numSlotsPerFrame; i++) {
        if (slotId == tdma->numSlotsPerFrame - 1) {
            tdma->nextSlotId = 0;
        }
        else {
            tdma->nextSlotId = slotId + 1;
        }
        tdma->nextStatus = tdma->frameDescriptor[tdma->nextSlotId];

        delay += tdma->slotDuration + tdma->guardTime;

        if (tdma->nextSlotId == 0) {
            delay += tdma->interFrameTime;
        }

        if ((tdma->nextStatus != tdma->currentStatus) ||
            (tdma->nextStatus == TDMA_STATUS_TX &&
             tdma->currentStatus == TDMA_STATUS_TX))
        {
            if (DEBUG) {
                printf("node [%d], Next Timer Being Sent, Current Status :"
                       "%d, Next Status : %d\n",
                       node->nodeId, tdma->currentStatus, tdma->nextStatus);

            }
            break;
        }

        slotId++;

        if (slotId == tdma->numSlotsPerFrame) {
            slotId = 0;
        }
    }
    assert(i != tdma->numSlotsPerFrame);

    tdma->timerExpirationTime = delay + node->getNodeTime();

    MESSAGE_Send(node, tdma->timerMsg, delay);
#ifdef PARALLEL //Parallel
        PARALLEL_SetLookaheadHandleEOT(node,
            tdma->lookaheadHandle,
            node->getNodeTime() + delay + 5000);
#endif //endParallel
}


/*
 * NAME:        MacTdmaNetworkLayerHasPacketToSend.
 *
 * PURPOSE:     In passive mode, start process to send data; else return;
 *
 * RETURN:      None.
 *
 */
void MacTdmaNetworkLayerHasPacketToSend(Node* node, MacDataTdma* tdma) {
}



/*
 * FUNCTION    MacTdmaInit
 * PURPOSE     Initialization function for TDMA protocol of MAC layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacTdmaInit(Node* node,
                 int interfaceIndex,
                 const NodeInput* nodeInput,
                 const int subnetListIndex,
                 const int numNodesInSubnet)
{
    BOOL wasFound;
    BOOL automaticScheduling = 0;

    char schedulingString[MAX_STRING_LENGTH];

    int i;
    int numSlots;
    int  numChannels = PROP_NumberChannels(node);

    MacDataTdma* tdma = (MacDataTdma *)MEM_malloc(sizeof(MacDataTdma));
    clocktype duration;

    if (DEBUG) {
        printf("partition %d, node %d in tdmainitt\n",
               node->partitionData->partitionId, node->nodeId);
        printf("partition %d, node %d sees %d nodes in subnet\n"
               "subnetListIndex %d\n",
               node->partitionData->partitionId,
               node->nodeId,
               numNodesInSubnet,
               subnetListIndex);
        fflush(stdout);
    }

    ERROR_Assert(tdma != NULL,
                 "TDMA: Unable to allocate memory for TDMA data structure.");

    memset(tdma, 0, sizeof(MacDataTdma));
    tdma->myMacData = node->macData[interfaceIndex];
    tdma->myMacData->macVar = (void *)tdma;

    //
    // TDMA-NUMBER-OF-SLOTS-PER-FRAME: number of slots per frame
    //
    IO_ReadInt(node,node->nodeId, interfaceIndex, nodeInput,
               "TDMA-NUM-SLOTS-PER-FRAME", &wasFound, &numSlots);

    if (wasFound) {
        tdma->numSlotsPerFrame = numSlots;
    }
    else {
        tdma->numSlotsPerFrame = numNodesInSubnet;
    }

    //
    // TDMA-SLOT-DURATION: slot duration
    //
    IO_ReadTime(node,node->nodeId, interfaceIndex, nodeInput,
                "TDMA-SLOT-DURATION", &wasFound, &duration);

    if (wasFound) {
        tdma->slotDuration = duration;
    }
    else {
        tdma->slotDuration = DefaultTdmaSlotDuration;
    }

    //
    // TDMA-GUARD-TIME: to be inserted between slots
    //
    IO_ReadTime(node,node->nodeId, interfaceIndex, nodeInput,
                "TDMA-GUARD-TIME", &wasFound, &duration);

    if (wasFound) {
        tdma->guardTime = duration;
    }
    else {
        tdma->guardTime = DefaultTdmaGuardTime;
    }

    //
    // TDMA-INTER-FRAME-TIME: to be inserted between frames
    //
    IO_ReadTime(node,node->nodeId, interfaceIndex, nodeInput,
                "TDMA-INTER-FRAME-TIME", &wasFound, &duration);

    if (wasFound) {
        tdma->interFrameTime = duration;
    }
    else {
        tdma->interFrameTime = DefaultTdmaInterFrameTime;
    }

    //
    // TDMA-SCHEDULING: type of scheduling (automatic or from file)
    //
    IO_ReadString(node,node->nodeId, interfaceIndex, nodeInput,
                  "TDMA-SCHEDULING", &wasFound, schedulingString);

    if (wasFound) {
        if (strcmp(schedulingString, "AUTOMATIC") == 0) {
            automaticScheduling = TRUE;
        }
        else if (strcmp(schedulingString, "FILE") == 0) {
            automaticScheduling = FALSE;
        }
        else {
            ERROR_ReportError(
                "TDMA-SCHEDULING must be either AUTOMATIC or FILE\n");
        }
    }
    else {
        automaticScheduling = TRUE;
    }

    //
    // Build frameDescriptor
    //
    // If TDMA-SCHEDULING = AUTOMATIC is specified, all nodes
    // other than transmitters listen to the channel.
    //
    // If TDMA-SCHEDULING = FILE is specified, all nodes stay
    // idle unless Rx or Tx is specified in TDMA-SCHEDULING-FILE.
    //

    tdma->frameDescriptor =
        (char*)MEM_malloc(tdma->numSlotsPerFrame * sizeof(char));

    for (i = 0; i < tdma->numSlotsPerFrame; i++) {
        tdma->frameDescriptor[i] = TDMA_STATUS_IDLE;
    }

    if (automaticScheduling == TRUE) {
        for (i = 0; i < tdma->numSlotsPerFrame; i++) {
            if (subnetListIndex == i % numNodesInSubnet) {
                tdma->frameDescriptor[i] = TDMA_STATUS_TX;
            }
            else {
                tdma->frameDescriptor[i] = TDMA_STATUS_RX;
            }
        }
    }
    else {
        NodeInput scheduleInput;

        IO_ReadCachedFile(node,node->nodeId, interfaceIndex, nodeInput,
                          "TDMA-SCHEDULING-FILE", &wasFound, &scheduleInput);

        if (wasFound == FALSE) {
            ERROR_ReportError("TDMA-SCHEDULING-FILE is missing\n");
        }

        for (i = 0; i < scheduleInput.numLines; i++) {
            char token[MAX_STRING_LENGTH];
            char *strPtr = NULL;
            char *returnValue = NULL;
            int slotId;

            ERROR_Assert(i < tdma->numSlotsPerFrame,
                         "TDMA: Too many slots in TDMA Scheduling File.");

            IO_GetToken(token, scheduleInput.inputStrings[i], &strPtr);

            slotId = (int)atoi(token);
            ERROR_Assert(i == slotId,
                         "TDMA: Line on TDMA Scheduling File does not correspond with slot ID");

            returnValue = IO_GetToken(token, strPtr, &strPtr);

            while (returnValue != NULL) {
                char nodeString[MAX_STRING_LENGTH];
                char *typeStr;
                NodeAddress nodeId;
                int numReturnVals = 0;

                numReturnVals = sscanf(token, "%s", nodeString);
                assert(numReturnVals == 1);

                IO_ConvertStringToLowerCase(nodeString);

                typeStr = strrchr(nodeString, '-');
                *typeStr = 0;
                typeStr++; // to the next character

                if (strcmp(nodeString, "all") == 0) {
                    if (strcmp(typeStr, "rx") != 0) {
                        ERROR_ReportError("'All' must be used with '-Rx'\n");
                    }
                    nodeId = node->nodeId;
                }
                else {
                    nodeId = (NodeAddress)atoi(nodeString);
                }

                returnValue = IO_GetToken(token, strPtr, &strPtr);

                if (node->nodeId != nodeId) {
                    continue;
                }

                if (strcmp(typeStr, "rx") == 0) {
                    //
                    // '-Tx' has priority over '-Rx' (likely from 'All-Rx')
                    //
                    if (tdma->frameDescriptor[slotId] != TDMA_STATUS_TX) {
                        tdma->frameDescriptor[slotId] = TDMA_STATUS_RX;
                    }
                }
                else if (strcmp(typeStr, "tx") == 0) {
                    tdma->frameDescriptor[slotId] = TDMA_STATUS_TX;
                }
                else {
                    ERROR_ReportError("Slot type must be either Rx or Tx\n");
                }
            }
        }
        assert(i == tdma->numSlotsPerFrame);
    }

    if (DEBUG) {
        printf("partition %d, node %d: ", node->partitionData->partitionId, node->nodeId);
        for (i = 0; i < tdma->numSlotsPerFrame; i++) {
            printf("%d ", (int)tdma->frameDescriptor[i]);
        }
        printf("\n");
    }

    // Use first listening channel
    for (i = 0; i < numChannels; i++)
    {
        if (PHY_IsListeningToChannel(node, tdma->myMacData->phyNumber, i)
                == TRUE)
        {
            tdma->channelIndex = i;
            break;
        }
    }

    tdma->stats.pktsSentUnicast = 0;
    tdma->stats.pktsSentBroadcast = 0;
    tdma->stats.pktsGotUnicast = 0;
    tdma->stats.pktsGotBroadcast = 0;
    tdma->stats.numTxSlotsMissed = 0;

#ifdef PARALLEL //Parallel
    tdma->lookaheadHandle = PARALLEL_AllocateLookaheadHandle (node);
    PARALLEL_AddLookaheadHandleToLookaheadCalculator(
        node, tdma->lookaheadHandle, 0);

    // Also sepcific a minimum lookahead, in case EOT can't be used in the sim.
    PARALLEL_SetMinimumLookaheadForInterface(node, tdma->slotDuration);
    int index;
    for (index = 0; index < node->numberPhys; index ++ )
    {
        if (node->phyData[index]->phyModel == PHY802_11a)
        {
            PARALLEL_SetMinimumLookaheadForInterface(node,
                DOT11_802_11a_DELAY_UNTIL_SIGNAL_AIRBORN);
        }
        if (node->phyData[index]->phyModel == PHY802_11b)
        {
            PARALLEL_SetMinimumLookaheadForInterface(node,
                DOT11_802_11b_DELAY_UNTIL_SIGNAL_AIRBORN);
        }
    }
#endif //endParallel

    MacTdmaInitializeTimer(node, tdma);

}

void MacTdmaReceivePacketFromPhy(
    Node* node, MacDataTdma* tdma, Message* msg)
{

    if (tdma->currentStatus == TDMA_STATUS_TX ||
        tdma->currentStatus == TDMA_STATUS_IDLE)
    {
        MESSAGE_Free(node, msg);
        return;
    }//if//

    if (DEBUG) {
        //printf("**PRE-RECEIVE** About to check "
        //       "if Receive mode is on at node [%d]\n", node->nodeId);
    }

    switch (tdma->currentStatus) {
        case TDMA_STATUS_RX: {
            int interfaceIndex = tdma->myMacData->interfaceIndex;
            TdmaHeader *hdr = (TdmaHeader*)msg->packet;

            if (DEBUG) {
                char clockStr[MAX_CLOCK_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);
                printf ("***RECEIVE*** Node [%d] finished RECEIVING packet at slot [%d]\n",
                         node->nodeId, tdma->currentSlotId);
            }

            MacHWAddress  macHWAddress;
            Convert802AddressToVariableHWAddress(node, &macHWAddress,
                                                 &hdr->destAddr);
#ifdef CYBER_LIB
            MacHWAddress srcHWAddress;
            Convert802AddressToVariableHWAddress(node, &srcHWAddress,
                                                 &hdr->sourceAddr);
            TosType priority = hdr->priority;

            BOOL macMatched = FALSE;
            if (msg->headerProtocols[msg->numberOfHeaders-1] == TRACE_TDMA)
            {
                macMatched = TRUE;
                MESSAGE_RemoveHeader(node, msg, sizeof(TdmaHeader), TRACE_TDMA);
            }

            if (macMatched && IsMyAnodrDataPacket(node, msg))
            {
                srcHWAddress = GetBroadCastAddress(node, interfaceIndex);
                tdma->stats.pktsGotUnicast++;
                MAC_HandOffSuccessfullyReceivedPacket(
                    node, interfaceIndex, msg, &srcHWAddress);
                ERROR_Assert(tdma->currentReceiveSlotId == tdma->currentSlotId,
                             "TDMA: Finished receiving on a different slot");
            }
            else
            {
             // Not anonymous TDMA packet for ANODR, restore TDMA header
             if (macMatched)
             {
             MESSAGE_AddHeader(node, msg, sizeof(TdmaHeader), TRACE_TDMA);
             hdr = (TdmaHeader*)msg->packet;
             ConvertVariableHWAddressTo802Address(node,
                                                  &macHWAddress,
                                                  &hdr->destAddr);
             ConvertVariableHWAddressTo802Address(node,
                                                  &srcHWAddress,
                                                  &hdr->sourceAddr);
             hdr->priority = priority;
             }
#endif // CYBER_LIB
            if (MAC_IsMyAddress(node ,&macHWAddress))
            {
                tdma->stats.pktsGotUnicast++;
            }
            else if (MAC_IsBroadcastMac802Address(&hdr->destAddr)) {
                tdma->stats.pktsGotBroadcast++;
            }

            if (MAC_IsMyAddress(node, &macHWAddress) ||
                    MAC_IsBroadcastHWAddress(&macHWAddress))
            {

                BOOL macMatched = FALSE;
                if (msg->headerProtocols[msg->numberOfHeaders-1] == TRACE_TDMA)
                {
                    macMatched = TRUE;
                    MESSAGE_RemoveHeader(node, msg, sizeof(TdmaHeader), TRACE_TDMA);
                }
                if (macMatched)
                {
                    MacHWAddress src_macHWAddr;
                    Convert802AddressToVariableHWAddress(node , &src_macHWAddr,
                                                           &hdr->sourceAddr);

                    MAC_HandOffSuccessfullyReceivedPacket(node,
                        tdma->myMacData->interfaceIndex, msg, &src_macHWAddr);

                    ERROR_Assert(tdma->currentReceiveSlotId == tdma->currentSlotId,
                             "TDMA: Finished receiving on a different slot");
                }
            }
            else {
                if (node->macData[interfaceIndex]->promiscuousMode) {
                    MacTdmaHandlePromiscuousMode(node,
                                                 tdma,
                                                 msg,
                                                 hdr->sourceAddr,
                                                 hdr->destAddr);
                }

                MESSAGE_Free(node, msg);
            }
#ifdef CYBER_LIB
            }
#endif // CYBER_LIB

            break;
        }

        default:
            assert(FALSE); abort();
    }//switch//
}


void MacTdmaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataTdma* tdma,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus)
{
   if (oldPhyStatus == PHY_TRANSMITTING) {
      ERROR_Assert(newPhyStatus != PHY_TRANSMITTING,
                   "TDMA: Old and New physical status cannot be the same");
      ERROR_Assert(tdma->currentStatus == TDMA_STATUS_TX,
                    "TDMA: Physical and MAC status do not match during TX");

      tdma->currentStatus = TDMA_STATUS_TX;
   }//if//

   if (newPhyStatus == PHY_RECEIVING) {
       //ERROR_Assert(tdma->currentStatus == TDMA_STATUS_RX,
       //             "TDMA: Physical and MAC status do not match during TX");
   }

   tdma->currentReceiveSlotId = tdma->currentSlotId;

   if (DEBUG) {
       //printf("*** STATUS CHANGE. Node [%d]. OLD:%u, NEW: %u\n",
       //        node->nodeId, oldPhyStatus, newPhyStatus);
   }

}


/*
 * FUNCTION    MacTdmaLayer
 * PURPOSE     Models the behaviour of the MAC layer with the TDMA protocol
 *             on receiving the message enclosed in msg.
 *
 * Parameters:
 *     node:     node which received the message
 *     msg:      message received by the layer
 *
 * Return:      None.
 *
 * Assumption: Regardless of the slot duration time, the basic slot unit
 *             is a packet. This need not be the case.
 *             TBD: Use only slot duration length as determinant of data
 *             to be sent, including fragmentation
 */
void MacTdmaLayer(Node* node, int interfaceIndex, Message* msg) {
    MacDataTdma* tdma =
        (MacDataTdma*)node->macData[interfaceIndex]->macVar;
    int previousStatus = tdma->currentStatus;
    BOOL phyIsListening;

    //
    // Currently, the only message this function receives is
    // "MSG_MAC_TimerExpired" message sent by this protocol itself.
    //
    ERROR_Assert(msg->eventType == MSG_MAC_TimerExpired,
                 "TDMA: Only acceptable event type is timer expiration");
    ERROR_Assert(node->getNodeTime() == tdma->timerExpirationTime,
                 "TDMA: Simulation time differs from expected timer expiration");

    tdma->currentStatus = tdma->nextStatus;
    tdma->currentSlotId = tdma->nextSlotId;

    assert((tdma->currentStatus != previousStatus) ||
           (tdma->currentStatus == TDMA_STATUS_TX &&
            previousStatus == TDMA_STATUS_TX));

    phyIsListening =
        PHY_IsListeningToChannel(
            node, tdma->myMacData->phyNumber, tdma->channelIndex);

    switch (tdma->currentStatus) {
        case TDMA_STATUS_IDLE: {
            if (phyIsListening) {
                PHY_StopListeningToChannel(
                    node,
                    tdma->myMacData->phyNumber,
                    tdma->channelIndex);
            }

            break;
        }

        case TDMA_STATUS_RX: {
            if (DEBUG) {
                //printf("Node %ld in RECEIVE slot %d\n",
                //       node->nodeId, tdma->currentSlotId);
            }

            if (!phyIsListening) {
                PHY_StartListeningToChannel(
                    node,
                    tdma->myMacData->phyNumber,
                    tdma->channelIndex);
            }

            break;
        }

        case TDMA_STATUS_TX: {
            Message *msg;
            MacHWAddress nextHopHWAddr;
            int networkType;
            TosType priority;
            TdmaHeader *hdr;
            clocktype transmissionDelay;

            if (DEBUG) {
                //printf("Node %ld in TRANSMIT slot %d\n",
                //       node->nodeId, tdma->currentSlotId);
            }

            if (previousStatus == TDMA_STATUS_TX) {
                // Previous transmission should have ended at this point
                assert(PHY_GetStatus(node, tdma->myMacData->phyNumber) !=
                       PHY_TRANSMITTING);
            }

            if (MAC_OutputQueueIsEmpty(node,
                        tdma->myMacData->interfaceIndex)) {
                tdma->stats.numTxSlotsMissed++;

                break;
            }

            //
            // Dequeue packet from the network queue.
            //
            MAC_OutputQueueDequeuePacket(
                node, tdma->myMacData->interfaceIndex,
                &msg, &nextHopHWAddr, &networkType, &priority);


            assert(msg != NULL);

            if (DEBUG) {
                char clockStr[100];
                //char buf[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);
                printf("at %s: ***TRANSMITTING*** node [%d] to node",clockStr, node->nodeId);
                MAC_PrintHWAddr(&nextHopHWAddr);
                printf(" during SLOT [%d]\n",tdma->currentSlotId);
                fflush(stdout);
            }

            /*
             * Assign other fields to packet to be sent
             * to phy layer.
             */
            MESSAGE_AddHeader(node, msg, sizeof(TdmaHeader), TRACE_TDMA);

            hdr = (TdmaHeader*)msg->packet;


#ifdef CYBER_LIB
            if (NetworkIpGetUnicastRoutingProtocolType(node, interfaceIndex)
                == ROUTING_PROTOCOL_ANODR)
            {
                // Anonymous TDMA for ANODR
                memset(&hdr->sourceAddr, 0xff, sizeof(Mac802Address));
                memset(&hdr->destAddr, 0xff, sizeof(Mac802Address));
            }
            else
            {
#endif // CYBER_LIB
            ConvertVariableHWAddressTo802Address(node,
                                                 &nextHopHWAddr,
                                                 &hdr->destAddr);

            ConvertVariableHWAddressTo802Address(
                  node,
                  &node->macData[tdma->myMacData->interfaceIndex]->macHWAddr,
                  &hdr->sourceAddr);
#ifdef CYBER_LIB
            }
#endif // CYBER_LIB

            hdr->priority = priority;

            transmissionDelay = PHY_GetTransmissionDelay(node,
                                                         tdma->myMacData->phyNumber,
                                                         MESSAGE_ReturnPacketSize(msg));

            ERROR_Assert(transmissionDelay <= tdma->slotDuration,
                         "TDMA: Message size is greater than slot duration");


            PHY_StartTransmittingSignal(
                node, tdma->myMacData->phyNumber,
                msg, FALSE, 0);

            if (MAC_IsBroadcastMac802Address(&hdr->destAddr)) {
                tdma->stats.pktsSentBroadcast++;
            }
            else {
                tdma->stats.pktsSentUnicast++;
            }

            break;
        }

        default: {
            abort();
        }
    }
    MacTdmaUpdateTimer(node, tdma);
}


/*
 * FUNCTION    MAC_Finalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the TDMA protocol of MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void MacTdmaFinalize(Node *node, int interfaceIndex) {
    MacDataTdma* tdma = (MacDataTdma *)node->macData[interfaceIndex]->macVar;

    if (node->macData[interfaceIndex]->macStats == TRUE) {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "UNICAST packets sent to the channel = %d",
                tdma->stats.pktsSentUnicast);
        IO_PrintStat(node, "MAC", "TDMA", ANY_DEST, interfaceIndex, buf);

        sprintf(buf, "BROADCAST packets sent to the channel = %d",
                tdma->stats.pktsSentBroadcast);
        IO_PrintStat(node, "MAC", "TDMA", ANY_DEST, interfaceIndex, buf);
        sprintf(buf, "UNICAST packets received = %d",
                tdma->stats.pktsGotUnicast);
        IO_PrintStat(node, "MAC", "TDMA", ANY_DEST, interfaceIndex, buf);

        sprintf(buf, "BROADCAST packets received = %d",
                tdma->stats.pktsGotBroadcast);
        IO_PrintStat(node, "MAC", "TDMA", ANY_DEST, interfaceIndex, buf);
    }
}
