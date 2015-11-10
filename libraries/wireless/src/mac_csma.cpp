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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "mac_csma.h"
#include "network_ip.h"
#include "partition.h"


static /*inline*/
PhyStatusType PhyStatus(Node* node, MacDataCsma* csma)
{
   return PHY_GetStatus(node, csma->myMacData->phyNumber);
}


/*
 * NAME:        MacCsmaHandlePromiscuousMode.
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
void MacCsmaHandlePromiscuousMode(Node *node,
                                  MacDataCsma* csma,
                                  Message *frame,
                                  Mac802Address prevHop,
                                  Mac802Address destAddr)
{
    MacHWAddress prevHopHWAddr;
    MacHWAddress destHWAddr;

    MESSAGE_RemoveHeader(node, frame, sizeof(CsmaHeader), TRACE_CSMA);

    Convert802AddressToVariableHWAddress(node, &prevHopHWAddr, &prevHop);

    Convert802AddressToVariableHWAddress(node, &destHWAddr, &destAddr);

    MAC_SneakPeekAtMacPacket(node,
                             csma->myMacData->interfaceIndex,
                             frame,
                             prevHopHWAddr,
                             destHWAddr);


    MESSAGE_AddHeader(node, frame, sizeof(CsmaHeader), TRACE_CSMA);
}




/*
 * NAME:        MacCsmaDataXmit.
 *
 * PURPOSE:     Sending data frames to destination.
 *
 * PARAMETERS:  node, node sending the data frame.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaXmit(Node *node, MacDataCsma *csma)
{
    Message *msg;
    MacHWAddress destHWAddr;
    int networkType;
    TosType priority;

    CsmaHeader         *hdr;

    assert(csma->status == CSMA_STATUS_XMIT);

    /*
     * Dequeue packet which was received from the
     * network layer.
     */

    MAC_OutputQueueDequeuePacket(
        node, csma->myMacData->interfaceIndex,
        &msg, &destHWAddr, &networkType, &priority);

    if (msg == NULL)
    {
        #ifdef QDEBUG
            printf("CSMA: Queue should not be empty...\n");
        #endif


        // The Queue can be empty if the packet was dropped forcefully by

        // routing protocol. Set the correct CSMA state in this case

        if(csma->BOtimes >0)

        {
            csma->status = CSMA_STATUS_BACKOFF;

        }
        else
        {
            csma->status = CSMA_STATUS_PASSIVE;

        }

        return;
    }

    csma->status = CSMA_STATUS_IN_XMITING;
    csma->timer.flag = CSMA_TIMER_OFF | CSMA_TIMER_UNDEFINED;

    /*
     * Assign other fields to packet to be sent
     * to phy layer.
     */

    MESSAGE_AddHeader(node, msg, sizeof(CsmaHeader), TRACE_CSMA);

    hdr = (CsmaHeader *) msg->packet;

    ConvertVariableHWAddressTo802Address(node, &destHWAddr, &hdr->destAddr);

    ConvertVariableHWAddressTo802Address(
                  node,
                  &node->macData[csma->myMacData->interfaceIndex]->macHWAddr,
                  &hdr->sourceAddr);

    hdr->priority = priority;

    PHY_StartTransmittingSignal(
        node, csma->myMacData->phyNumber,
        msg, FALSE, 0);


    if (MAC_IsBroadcastMac802Address(&hdr->destAddr)) {
        csma->pktsSentBroadcast++;
    }
    else {
        csma->pktsSentUnicast++;
    }
}




/*
 * NAME:        MacCsmaSetTimer.
 *
 * PURPOSE:     Set a timer for node to expire at time timerValue.
 *
 * PARAMETERS:  node, node setting the timer.
 *              timerType, what type of timer is being set.
 *              delay, when timer is to expire.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaSetTimer(
    Node *node, MacDataCsma* csma, int timerType, clocktype delay)
{
    Message      *newMsg;
    int         *timerSeq;

    csma->timer.flag = (unsigned char)(CSMA_TIMER_ON | timerType);
    csma->timer.seq++;

    assert((timerType == CSMA_TIMER_BACKOFF) ||
           (timerType == CSMA_TIMER_YIELD));

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_TimerExpired);
    MESSAGE_SetInstanceId(newMsg, (short)csma->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(csma->timer.seq));
    timerSeq  = (int *) MESSAGE_ReturnInfo(newMsg);
    *timerSeq = csma->timer.seq;

    MESSAGE_Send(node, newMsg, delay);
}


/*
 * NAME:        MacCsmaYield.
 *
 * PURPOSE:     Yield so neighboring nodes can transmit or receive.
 *
 * PARAMETERS:  node, node that is yielding.
 *              holding, how int to yield for.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaYield(Node *node, MacDataCsma *csma, clocktype holding)
{
    assert(csma->status == CSMA_STATUS_YIELD);

    MacCsmaSetTimer(node, csma, CSMA_TIMER_YIELD, holding);
}


/*
 * NAME:        MacCsmaBackoff.
 *
 * PURPOSE:     Backing off sending data at a later time.
 *
 * PARAMETERS:  node, node that is backing off.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaBackoff(Node *node, MacDataCsma *csma)
{
    clocktype randTime;
    assert(csma->status == CSMA_STATUS_BACKOFF);

    randTime = (RANDOM_nrand(csma->seed) % csma->BOmin) + 1;
    csma->BOmin = csma->BOmin * 2;

    if (csma->BOmin > csma->BOmax) {
        csma->BOmin = csma->BOmax;
    }

    csma->BOtimes++;

    MacCsmaSetTimer(node, csma, CSMA_TIMER_BACKOFF, randTime);
}


static //inline//
void CheckPhyStatusAndSendOrBackoff(Node* node, MacDataCsma* csma) {
    /* Carrier sense response from phy. */

    if ((PhyStatus(node, csma) == PHY_IDLE) &&
        (csma->status != CSMA_STATUS_IN_XMITING))
    {
        csma->status = CSMA_STATUS_XMIT;
        MacCsmaXmit(node, csma);
    }
    else {
        if (!MAC_OutputQueueIsEmpty(
                node, csma->myMacData->interfaceIndex))
        {
            csma->status = CSMA_STATUS_BACKOFF;
            MacCsmaBackoff(node, csma);
        }
    }
}



/*
 * NAME:        MacCsmaNetworkLayerHasPacketToSend.
 *
 * PURPOSE:     In passive mode, start process to send data; else return;
 *
 * RETURN:      None.
 *
 */

void MacCsmaNetworkLayerHasPacketToSend(Node *node, MacDataCsma *csma)
{
    if (csma->status == CSMA_STATUS_PASSIVE) {
        CheckPhyStatusAndSendOrBackoff(node, csma);
    }//if//
}




/*
 * NAME:        MacCsmaPassive.
 *
 * PURPOSE:     In passive mode, check whether there is a local packet.
 *              If YES, send data; else return;
 *
 * PARAMETERS:  node, node that is in passive state.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaPassive(Node *node, MacDataCsma *csma)
{
    if ((csma->status == CSMA_STATUS_PASSIVE) &&
        (!MAC_OutputQueueIsEmpty(node, csma->myMacData->interfaceIndex)))
    {
        MacCsmaNetworkLayerHasPacketToSend(node, csma);
    }

}


/*
 * NAME:        MacCsmaPrintStats
 *
 * PURPOSE:     Print MAC layer statistics.
 *
 * PARAMETERS:  node.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacCsmaPrintStats(Node *node, MacDataCsma* csma, int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Packets from network = %d",
            csma->pktsToSend);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Packets lost due to buffer overflow = %d",
            csma->pktsLostOverflow);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "UNICAST packets sent to channel = %d",
            csma->pktsSentUnicast);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "BROADCAST packets sent to channel = %d",
            csma->pktsSentBroadcast);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);
    sprintf(buf, "UNICAST packets received = %d",
            csma->pktsGotUnicast);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "BROADCAST packets received = %d",
            csma->pktsGotBroadcast);
    IO_PrintStat(node, "Mac", "CSMA", ANY_DEST, interfaceIndex, buf);
}




/*
 * FUNCTION    MacCsmaInit
 * PURPOSE     Initialization function for CSMA protocol of MAC layer.

 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */

void MacCsmaInit(
   Node *node, int interfaceIndex, const NodeInput *nodeInput)
{
    MacDataCsma *csma = (MacDataCsma *) MEM_malloc(sizeof(MacDataCsma));

    assert(csma != NULL);

    memset(csma, 0, sizeof(MacDataCsma));
    csma->myMacData = node->macData[interfaceIndex];
    csma->myMacData->macVar = (void *)csma;

    csma->timer.flag = CSMA_TIMER_OFF | CSMA_TIMER_UNDEFINED;
    csma->timer.seq = 0;

    csma->status = CSMA_STATUS_PASSIVE;
    csma->BOmin = CSMA_BO_MIN;
    csma->BOmax = CSMA_BO_MAX;
    csma->BOtimes = 0;

    csma->pktsToSend = 0;
    csma->pktsLostOverflow = 0;

    csma->pktsSentUnicast = 0;
    csma->pktsSentBroadcast = 0;

    csma->pktsGotUnicast = 0;
    csma->pktsGotBroadcast = 0;

    RANDOM_SetSeed(csma->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_CSMA,
                   interfaceIndex);

#ifdef PARALLEL //Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif //endParallel
}



void MacCsmaReceivePacketFromPhy(
    Node* node, MacDataCsma* csma, Message* msg)
{
    if (csma->status == CSMA_STATUS_IN_XMITING) {
        MESSAGE_Free(node, msg);
        return;
    }//if//

    switch (csma->status) {
    case CSMA_STATUS_PASSIVE:
    case CSMA_STATUS_CARRIER_SENSE:
    case CSMA_STATUS_BACKOFF:
    case CSMA_STATUS_YIELD: {
        int interfaceIndex = csma->myMacData->interfaceIndex;
        CsmaHeader *hdr = (CsmaHeader *) msg->packet;

        
        MacHWAddress  destHWAddress;

        Convert802AddressToVariableHWAddress(node, &destHWAddress,
                                                             &hdr->destAddr);


       if (MAC_IsMyAddress(node, &destHWAddress)) {
            csma->pktsGotUnicast++;
        }
        else if (MAC_IsBroadcastMac802Address(&hdr->destAddr))
        {
            csma->pktsGotBroadcast++;
        }

       if (MAC_IsMyAddress(node, &destHWAddress) ||
                            MAC_IsBroadcastHWAddress(&destHWAddress))
        {

             MacHWAddress srcHWAddress;
             Convert802AddressToVariableHWAddress(node, &srcHWAddress,
                                                           &hdr->sourceAddr);

            MESSAGE_RemoveHeader(node, msg, sizeof(CsmaHeader), TRACE_CSMA);

            MAC_HandOffSuccessfullyReceivedPacket(node,
               csma->myMacData->interfaceIndex, msg, &srcHWAddress);
        }
        else {
            if (node->macData[interfaceIndex]->promiscuousMode) {
                MacCsmaHandlePromiscuousMode(node,
                                             csma,
                                             msg,
                                             hdr->sourceAddr,
                                             hdr->destAddr);
            }
            MESSAGE_Free(node, msg);
        }
        break;
    }
    default:
        MESSAGE_Free(node, msg);
        printf("MAC_CSMA: Error with node %u, status %d.\n",
               node->nodeId, csma->status);
        assert(FALSE); abort();
    }//switch//
}



void MacCsmaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataCsma* csma,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus)
{
   if (oldPhyStatus == PHY_TRANSMITTING) {
      assert(newPhyStatus != PHY_TRANSMITTING);
      assert(csma->status == CSMA_STATUS_IN_XMITING);

      csma->BOmin = CSMA_BO_MIN;
      csma->BOmax = CSMA_BO_MAX;
      csma->BOtimes = 0;
      csma->status = CSMA_STATUS_YIELD;
      MacCsmaYield(node, csma, (clocktype)CSMA_TX_DATA_YIELD_TIME);
   }//if//
}





/*
 * FUNCTION    MacCsmaLayer
 * PURPOSE     Models the behaviour of the MAC layer with the CSMA protocol
 *             on receiving the message enclosed in msg.
 *
 * Parameters:
 *     node:     node which received the message
 *     msg:      message received by the layer
 */

void MacCsmaLayer(Node *node, int interfaceIndex, Message *msg)
{
    /*
     * Retrieve the pointer to the data portion which relates
     * to the CSMA protocol.
     */

    MacDataCsma *csma = (MacDataCsma *)node->macData[interfaceIndex]->macVar;
    int seq_num;

    if (msg->eventType != MSG_MAC_TimerExpired) {
        printf("at ""%15" TYPES_64BITFMT "d"", node %d executing wrong event %d on int %d\n",
               node->getNodeTime(), node->nodeId, msg->eventType,
               interfaceIndex);
        fflush(stdout);
    }
    assert(msg->eventType == MSG_MAC_TimerExpired);

    seq_num = *((int *) MESSAGE_ReturnInfo(msg));

    MESSAGE_Free(node, msg);

    if ((seq_num < csma->timer.seq) ||
        ((csma->timer.flag & CSMA_TIMER_SWITCH) == CSMA_TIMER_OFF)) {
        return;
    }

    if (seq_num > csma->timer.seq) {
        assert(FALSE);
    }

    assert(((csma->timer.flag & CSMA_TIMER_TYPE) ==
            CSMA_TIMER_BACKOFF) ||
           ((csma->timer.flag & CSMA_TIMER_TYPE) == CSMA_TIMER_YIELD));

    switch(csma->timer.flag & CSMA_TIMER_TYPE) {
    case CSMA_TIMER_BACKOFF:
    {
        csma->timer.flag = CSMA_TIMER_OFF | CSMA_TIMER_UNDEFINED;

        CheckPhyStatusAndSendOrBackoff(node, csma);

        break;
    }

    case CSMA_TIMER_YIELD:
        csma->timer.flag = CSMA_TIMER_OFF | CSMA_TIMER_UNDEFINED;
        csma->status = CSMA_STATUS_PASSIVE;
        MacCsmaPassive(node, csma);
        break;

    default:
        assert(FALSE); abort();
        break;
    }/*switch*/
}




/*
 * FUNCTION    MAC_Finalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the CSMA protocol of MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */

void MacCsmaFinalize(Node *node, int interfaceIndex)
{
    MacDataCsma* csma = (MacDataCsma *)node->macData[interfaceIndex]->macVar;

    if (node->macData[interfaceIndex]->macStats == TRUE) {
        MacCsmaPrintStats(node, csma, interfaceIndex);
    }
}
