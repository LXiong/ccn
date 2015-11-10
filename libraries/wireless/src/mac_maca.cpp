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
 *
 * MACA (multiple access with collision avoidance)
 *
 * reference: C. L Fuller and J. J. Garcia paper MACA specification
 * using RTS /CTS control frames to reserve medium
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "mac_maca.h"
#include "network_ip.h"
#include "partition.h"

#define MACA_DELAY                     (2 * MICRO_SECOND)
#define MACA_PHY_DELAY                 (5 * MICRO_SECOND)

static /*inline*/
PhyStatusType PhyStatus(Node* node, MacDataMaca* maca)
{
   return PHY_GetStatus(node, maca->myMacData->phyNumber);
}


/*
 * NAME:        MacMacaPrintStats
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
void MacMacaPrintStats(Node *node, MacDataMaca* maca, int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Packets from network = %d", maca->pktsToSend);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Packets lost due to buffer overflow = %d",
            maca->pktsLostOverflow);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "UNICAST packets sent to channel = %d",
            maca->pktsSentUnicast);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "BROADCAST packets sent to channel = %d",
            maca->pktsSentBroadcast);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);
    sprintf(buf, "UNICAST packets received = %d",
            maca->pktsGotUnicast);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "BROADCAST packets received = %d",
            maca->pktsGotBroadcast);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "RTS Packets sent = %d", maca->RtsPacketSent);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "CTS Packets sent = %d", maca->CtsPacketSent);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "RTS Packets received = %d", maca->RtsPacketGot);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "CTS Packets received = %d", maca->CtsPacketGot);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Noisy Packets received = %d", maca->NoisyPacketGot);
    IO_PrintStat(node, "MAC", "MACA", ANY_DEST, interfaceIndex, buf);
}




/*
 * NAME:        MacMacaSetTimer.
 *
 * PURPOSE:     Set a timer for node to expire at time timerValue.
 *
 * PARAMETERS:  node, node setting the timer.
 *              timerType, what type of timer is being set.
 *              timerValue, when timer is to expire.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaSetTimer(Node *node,
                            MacDataMaca *maca,
                            int timerType,
                            clocktype timerValue)
{
    Message      *newMsg;
    int         *timerSeq;

    maca->timer.flag = (unsigned char)(MACA_TIMER_ON | timerType);
    maca->timer.seq++;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_TimerExpired);
    MESSAGE_SetInstanceId(newMsg, (short)maca->myMacData->interfaceIndex);
    MESSAGE_InfoAlloc(node, newMsg, sizeof(maca->timer.seq));
    timerSeq  = (int *) MESSAGE_ReturnInfo(newMsg);
    *timerSeq = maca->timer.seq;

    MESSAGE_Send(node, newMsg, timerValue);
}


/*
 * NAME:        MacMacaMacMacaCancelTimer.
 *
 * PURPOSE:     Cancel a timer that was already set.
 *
 * PARAMETERS:  node, node cancelling the timer.
 *              timerType, what type of timer is being cancelled.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaCancelTimer(Node *node, MacDataMaca *maca, int timerType)
{
    if (timerType == MACA_T_UNDEFINED)
    {
        maca->timer.flag = MACA_TIMER_OFF | MACA_T_UNDEFINED;
    }
    else if ((maca->timer.flag & MACA_TIMER_TYPE) == timerType)
    {
        maca->timer.flag = MACA_TIMER_OFF | MACA_T_UNDEFINED;
    }
    else
    {
               assert(FALSE);
    }
}


/*
 * NAME:        MacMacaMacMacaResetTimer.
 *
 * PURPOSE:     Resets backoff timers to default values.
 *
 * PARAMETERS:  node, node resetting backoff timers.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static void MacMacaResetTimer(Node *node, MacDataMaca *maca)
{
    maca->BOmin = MACA_BO_MIN;
    maca->BOmax = MACA_BO_MAX;
    maca->BOtimes = 0;
}


/*
 * NAME:        MacMacaSetState.
 *
 * PURPOSE:     Set the state of a node.
 *
 * PARAMETERS:  node, node setting the state.
 *              state, state to set to.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaSetState(Node *node, MacDataMaca *maca, int state)
{
    maca->state = state;
}


/*
 * NAME:        MacMacaYield.
 *
 * PURPOSE:     Yield so neighboring nodes can transmit or receive.
 *
 * PARAMETERS:  node, node that is yielding.
 *              vacation, how long to yield for.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaYield(Node *node, MacDataMaca *maca, clocktype vacation)
{
    assert(maca->state == MACA_S_YIELD);

    MacMacaSetTimer(node, maca, MACA_T_YIELD,
                    vacation + RANDOM_nrand(maca->yieldSeed) % 20);
}

/*
 * NAME:        MacMacaMacMacaSendCts.
 *
 * PURPOSE:     Send CTS to neighboring nodes.
 *
 * PARAMETERS:  node, node sending CTS frame.
 *              fromNodeAddr, node that CTS frame is intended for.
 *              payloadSize, size of the data to be sent.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaSendCts(Node *node,
                    MacDataMaca *maca,
                    Mac802Address fromNodeAddr,
                    int payloadSize)
{
    int macFrameSize;
    MacaHeader* hdr;
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, msg, sizeof(MacaHeader), TRACE_MACA);

    hdr = (MacaHeader *) MESSAGE_ReturnPacket(msg);

    ConvertVariableHWAddressTo802Address(
                  node,
                  &node->macData[maca->myMacData->interfaceIndex]->macHWAddr,
                  &hdr->sourceAddr);

    MAC_CopyMac802Address(&hdr->destAddr, &fromNodeAddr);

    hdr->frameType   = MACA_CTS;
    hdr->payloadSize = payloadSize;
    hdr->priority = maca->currentPriority;

    maca->payloadSizeExpected = payloadSize;

    /* Size of CTS frame is simply the size of frame header. */

    macFrameSize = sizeof(MacaHeader);

    MacMacaSetState(node, maca, MACA_S_IN_XMITING_CTS);

    PHY_StartTransmittingSignal(
        node, maca->myMacData->phyNumber,
        msg, FALSE, 0);

    maca->CtsPacketSent++;

}

/*
 * NAME:        MacMacaMacMacaGetData.
 *
 * PURPOSE:     Sends packet to upper layer.
 *
 * PARAMETERS:  node, node handling the data packet.
 *              msg, packet to send to upper layers.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaGetData(Node *node, MacDataMaca *maca, Message *msg)
{
    MacaHeader *hdr = (MacaHeader *) msg->packet;
    
    MacHWAddress srcmacHWAddr;
    Convert802AddressToVariableHWAddress(node, &srcmacHWAddr,
                                                           &hdr->sourceAddr);

    Mac802Address destinationAddress;
    MAC_CopyMac802Address(&destinationAddress, &hdr->destAddr);

    MESSAGE_RemoveHeader(node, msg, sizeof(MacaHeader), TRACE_MACA);

    MAC_HandOffSuccessfullyReceivedPacket(node,
        maca->myMacData->interfaceIndex, msg, &srcmacHWAddr);


    if (MAC_IsBroadcastMac802Address(&destinationAddress))
    {
        maca->pktsGotBroadcast++;
    }
    else
    {
        maca->pktsGotUnicast++;
    }
}


/*
 * NAME:        MacMacaHandlePromiscuousMode.
 *
 * PURPOSE:     Supports promiscuous mode sending remote packets to
 *              upper layers.
 *
 * PARAMETERS:  node, node using promiscuous mode.
 *              msg, packet to send to upper layers.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaHandlePromiscuousMode(
    Node *node, MacDataMaca *maca, Message *msg, Mac802Address prevHop,
    Mac802Address destAddr)
{
    MESSAGE_RemoveHeader(node, msg, sizeof(MacaHeader), TRACE_MACA);

   MacHWAddress destHWAddr;
   MacHWAddress prevHopHWAddr;
   Convert802AddressToVariableHWAddress(node, &destHWAddr, &destAddr);
   Convert802AddressToVariableHWAddress(node, &prevHopHWAddr, &prevHop);

    MAC_SneakPeekAtMacPacket(node,
                             maca->myMacData->interfaceIndex,
                             msg,
                             prevHopHWAddr,
                             destHWAddr);


    MESSAGE_AddHeader(node, msg, sizeof(MacaHeader), TRACE_MACA);
}


/*
 * NAME:        MacMacaRts.
 *
 * PURPOSE:     Send RTS frame to intended destination.
 *
 * PARAMETERS:  node, node sending the RTS frame.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaRts(Node *node, MacDataMaca *maca)
{
    MacaHeader *hdr;
    Message *msg;

    assert(maca->state == MACA_S_RTS);

//    MAC_OutputQueueTopPacketForAPriority(
//       node, maca->myMacData->interfaceIndex, maca->currentPriority,
//       &tmpPktPtr, &nextHopAddress);

//    if (tmpPktPtr == NULL)
    if (maca->currentMessage == NULL)
    {
        #ifdef QDEBUG
            printf("MACA: Queue should not be empty...\n");
        #endif

        return;
    }


    /* Send RTS. */
    msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, msg, sizeof(MacaHeader), TRACE_MACA);

    hdr = (MacaHeader *) MESSAGE_ReturnPacket(msg);


    ConvertVariableHWAddressTo802Address(
                  node,
                  &node->macData[maca->myMacData->interfaceIndex]->macHWAddr,
                  &hdr->sourceAddr);

    hdr->destAddr    = maca->currentNextHopAddress;
    hdr->frameType   = MACA_RTS;
    hdr->payloadSize = MESSAGE_ReturnPacketSize(maca->currentMessage);
    hdr->priority = maca->currentPriority;

    maca->payloadSizeExpected =
        MESSAGE_ReturnPacketSize(maca->currentMessage);

    MacMacaSetState(node, maca, MACA_S_IN_XMITING_RTS);

    PHY_StartTransmittingSignal(
        node, maca->myMacData->phyNumber,
        msg, FALSE, 0);

    maca->RtsPacketSent++;
}

/*
 * NAME:        MacMacaDataXmit.
 *
 * PURPOSE:     Sending data frames to destination.
 *
 * PARAMETERS:  node, node sending the data frame.
 *              tag, type of data frame to send.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaDataXmit(Node *node, MacDataMaca *maca, int tag)
{
    Message      *msg;
    MacaHeader *hdr;

    assert(maca->state == MACA_S_XMIT);

//    MAC_OutputQueueDequeuePacketForAPriority(
//       node, maca->myMacData->interfaceIndex, maca->currentPriority,
//       &msg, &nextHopAddress);

//    if (msg == NULL)
    if (maca->currentMessage == NULL)
    {
        #ifdef QDEBUG
            printf("MACA: Queue should not be empty...\n");
        #endif

        return;
    }

    msg = maca->currentMessage;

    MESSAGE_AddHeader(node, msg, sizeof(MacaHeader), TRACE_MACA);

    hdr  = (MacaHeader *) MESSAGE_ReturnPacket(msg);
    hdr->frameType = tag;
    hdr->payloadSize = MESSAGE_ReturnPacketSize(msg) - sizeof(MacaHeader);

    
    ConvertVariableHWAddressTo802Address(
                  node,
                  &node->macData[maca->myMacData->interfaceIndex]->macHWAddr,
                  &hdr->sourceAddr);

    MAC_CopyMac802Address(&hdr->destAddr, &maca->currentNextHopAddress);
    hdr->priority = maca->currentPriority;

    maca->currentPriority = (unsigned char)( -1);
    maca->currentMessage = NULL;

     memcpy(&maca->currentNextHopAddress, ANY_MAC802, sizeof(Mac802Address));

    if (tag == MACA_UNICAST)
    {
        
        maca->pktsSentUnicast++;
        MacMacaSetState(node, maca, MACA_S_IN_XMITING_UNICAST);
    }
    else if (tag == MACA_BROADCAST)
    {
        maca->pktsSentBroadcast++;
        MacMacaSetState(node, maca, MACA_S_IN_XMITING_BROADCAST);
    }
    PHY_StartTransmittingSignal(
        node, maca->myMacData->phyNumber,
        msg, FALSE, 0);
}


/*
 * NAME:        MacMacaBackoff.
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
void MacMacaBackoff(Node *node, MacDataMaca *maca)
{
    clocktype randomTime;

    assert(maca->state == MACA_S_BACKOFF);

    randomTime = (RANDOM_nrand(maca->backoffSeed) % maca->BOmin) + 1;

    maca->BOmin = maca->BOmin * 2;

    if (maca->BOmin > maca->BOmax)
    {
        maca->BOmin = maca->BOmax;
    }

    maca->BOtimes++;

    MacMacaSetTimer(node, maca, MACA_T_BACKOFF, randomTime);

}


/*
 * NAME:        MacMacaPassive.
 *
 * PURPOSE:     In passive mode, check whether there is a local packet.
 *              If YES, send RTS; else return;
 *
 * PARAMETERS:  node, node that is in passive state.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaPassive(Node *node, MacDataMaca *maca)
{
    if (maca->state != MACA_S_PASSIVE)
    {
        
        
        return;
    }

    if ( (MAC_OutputQueueIsEmpty(node, maca->myMacData->interfaceIndex)) &&
         (maca->currentMessage == NULL))
    {
        maca->currentPriority = (unsigned char)(-1);
        memcpy(&maca->currentNextHopAddress, ANY_MAC802,
                                                      sizeof(Mac802Address));

        return;
    }

    if (maca->currentMessage == NULL)
    {
        int networkType;

        MacHWAddress nextHopAddr;
        
        MAC_OutputQueueTopPacket(
            node,
            maca->myMacData->interfaceIndex,
            &(maca->currentMessage),
            &nextHopAddr,
            &networkType,\
            &(maca->currentPriority));

        MAC_OutputQueueDequeuePacketForAPriority(
           node, maca->myMacData->interfaceIndex, maca->currentPriority,
           &(maca->currentMessage),  &nextHopAddr,
           &networkType);

        ConvertVariableHWAddressTo802Address(node, &nextHopAddr,
                                               &maca->currentNextHopAddress);


    }


    if (!MAC_IsBroadcastMac802Address(&maca->currentNextHopAddress))
    {
        
        MacMacaSetState(node, maca, MACA_S_RTS);
        MacMacaRts(node, maca);
    }
    else
    {
        if (PhyStatus(node, maca) == PHY_IDLE)
        {
            MacMacaSetState(node, maca, MACA_S_XMIT);
            MacMacaDataXmit(node, maca, MACA_BROADCAST);
        }
        else
        {
            
            MacMacaSetState(node, maca, MACA_S_BACKOFF);
            MacMacaBackoff(node, maca);
        }
    }
}

/*
 * NAME:        MacMacaNetworkLayerHasPacketToSend.
 *
 * PURPOSE:     Starts process to send a packet.
 *
 * RETURN:      None.
 *
 */

void MacMacaNetworkLayerHasPacketToSend(Node* node, MacDataMaca* maca)
{
    if (maca->currentMessage != NULL)
    {
         
        // Ignore this signal if MACA is holding a packet
        return;
    }

     
    MacMacaPassive(node, maca);
}



/*
 * NAME:        MacMacaRemote.
 *
 * PURPOSE:     Handle incoming frames.
 *
 * PARAMETERS:  node, node handling incoming frame.
 *              msg, the incoming frame.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */

static
void MacMacaRemote(Node *node, MacDataMaca *maca, Message *msg)
{
    Mac802Address sourceAddr, destAddr;
    int frameType, payloadSize;
    clocktype holdForCtsData = 0, holdForData = 0;
    MacaHeader *hdr = (MacaHeader *) msg->packet;

    MAC_CopyMac802Address(&sourceAddr, &hdr->sourceAddr);
    MAC_CopyMac802Address(&destAddr, &hdr->destAddr);

    frameType    = hdr->frameType;
    payloadSize  = hdr->payloadSize;


     MacHWAddress  macHWAddress;
     Convert802AddressToVariableHWAddress(node, &macHWAddress,
                                                             &hdr->destAddr);

    switch (frameType)
    {
        case MACA_RTS:

            if (MAC_IsMyAddress(node, &macHWAddress))
            {
                /* local RTS */
                maca->RtsPacketGot++;
                
                MacMacaSendCts(node, maca, sourceAddr, payloadSize);
            }
            else
            {
                /* Overheard RTS */
                /* Hold until other's CTS and DATA to be finished   */

                 
                holdForCtsData = ((sizeof(MacaHeader) +
                                 payloadSize + sizeof(MacaHeader)) *
                                 SECOND) / maca->myMacData->bandwidth +
                                 (3 * MACA_DELAY) + (2 * MACA_PHY_DELAY) +
                                 (2 *
                                  PHY_GetTransmissionDelay(
                                      node, maca->myMacData->phyNumber, 0)) +
                                 (2 * maca->myMacData->propDelay) +
                                 MACA_EXTRA_DELAY;

                MacMacaSetTimer(node, maca, MACA_T_REMOTE, holdForCtsData);

                maca->NoisyPacketGot++;
            }

            MESSAGE_Free(node, msg);
            break;

        case MACA_CTS:
        {
            /*
             * Determines how long a remote node backoffs to allow
             * other nodes to send and receive data packets.  The
             * bigger the different between this value and the actual
             * time for other nodes to send and receive a data packet,
             * the more unfairness occurs for this node since the other
             * nodes will be allowed to transmit before this node.
             */
            holdForData = ((payloadSize + sizeof(MacaHeader)) *
                          SECOND) / maca->myMacData->bandwidth +
                          MACA_DELAY +
                          PHY_GetTransmissionDelay(
                            node, maca->myMacData->phyNumber, 0) +
                          MACA_PHY_DELAY + maca->myMacData->propDelay +
                          MACA_EXTRA_DELAY;

            MacMacaSetTimer(node, maca, MACA_T_REMOTE, holdForData);

            maca->NoisyPacketGot++;
            MESSAGE_Free(node, msg);

            break;
        }
        case MACA_UNICAST:
        {
            /* yield for some time */

            if (MAC_IsMyAddress(node, &macHWAddress))
            {
                
                MacMacaGetData(node, maca, msg);
            }
            else
            {
                if (maca->myMacData->promiscuousMode == TRUE) {
                    MacMacaHandlePromiscuousMode(node,
                                                 maca,
                                                 msg,
                                                 sourceAddr,
                                                 destAddr);
                }
                MESSAGE_Free(node, msg);
            }

            MacMacaSetState(node, maca, MACA_S_PASSIVE);
            
            MacMacaPassive(node, maca);

            break;
        }
        case MACA_BROADCAST:
        {
            /* yield for some time */

            if (MAC_IsBroadcastHWAddress(&macHWAddress))
            {
                
                MacMacaGetData(node, maca, msg);
            }
            else
            {
                MESSAGE_Free(node, msg);
            }

            MacMacaSetState(node, maca, MACA_S_YIELD);
            MacMacaYield(node, maca, MACA_VACATION);

            break;
        }
        default:
            assert(FALSE);
            break;
    } /* end of switch */
}




/*
 * FUNCTION    MacMacaInit
 * PURPOSE     Initialization function for MACA protocol of MAC layer.

 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacMacaInit(
   Node *node, int interfaceIndex, const NodeInput *nodeInput)
{
    MacDataMaca *maca = (MacDataMaca *) MEM_malloc(sizeof(MacDataMaca));

    assert(maca != NULL);

    memset(maca, 0, sizeof(MacDataMaca));
    maca->myMacData = node->macData[interfaceIndex];
    maca->myMacData->macVar = (void *)maca;

    maca->state = MACA_S_PASSIVE;

    maca->BOmin = MACA_BO_MIN;
    maca->BOmax = MACA_BO_MAX;
    maca->BOtimes = 0;

    maca->timer.flag = MACA_TIMER_OFF | MACA_T_UNDEFINED;
    maca->timer.seq = 0;

    maca->pktsToSend = 0;
    maca->pktsLostOverflow = 0;

    maca->pktsSentUnicast = 0;
    maca->pktsSentBroadcast = 0;

    maca->pktsGotUnicast = 0;
    maca->pktsGotBroadcast = 0;

    maca->RtsPacketSent = 0;
    maca->CtsPacketSent = 0;

    maca->RtsPacketGot = 0;
    maca->CtsPacketGot = 0;
    maca->NoisyPacketGot = 0;

    maca->currentPriority = (unsigned char)(-1);
    maca->currentMessage = NULL;
    
    memcpy(&maca->currentNextHopAddress, ANY_MAC802, sizeof(Mac802Address));

    RANDOM_SetSeed(maca->backoffSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_MACA,
                   interfaceIndex);
    RANDOM_SetSeed(maca->yieldSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_MACA,
                   interfaceIndex + 1);

#ifdef PARALLEL //Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif //endParallel
}

/*
 * FUNCTION    MacMacaFinalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the MAC layer MACA protocol.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */

void MacMacaFinalize(Node *node, int interfaceIndex)
{
    MacDataMaca *maca =
                (MacDataMaca *)node->macData[interfaceIndex]->macVar;
    if (maca->myMacData->macStats == TRUE)
    {
        MacMacaPrintStats(node, maca, interfaceIndex);
    }
}



void MacMacaReceivePacketFromPhy(
    Node* node, MacDataMaca* maca, Message* msg)
{
    MacaHeader *hdr = (MacaHeader *) msg->packet;
    int frameType = hdr->frameType;


     MacHWAddress  macHWAddress;
     Convert802AddressToVariableHWAddress(node, &macHWAddress,
                                                             &hdr->destAddr);


     if (MAC_IsMyAddress(node, &macHWAddress) ||
         MAC_IsBroadcastMac802Address(&hdr->destAddr))
    {
        maca->currentPriority = (unsigned char)(hdr->priority);
    }

    switch(maca->state) {
    case MACA_S_RTS:
    {
       
        if (MAC_IsMyAddress(node, &macHWAddress)&& (frameType == MACA_CTS))
        {
            /*  Local CTS  */
            maca->CtsPacketGot++;

            MacMacaCancelTimer(node, maca, MACA_T_RTS);
            MacMacaSetState(node, maca, MACA_S_XMIT);
            
            MacMacaDataXmit(node, maca, MACA_UNICAST);
            MESSAGE_Free(node, msg);
        }
            /* Fall through */
        else
        {
            MacMacaCancelTimer(node, maca, MACA_T_RTS);
            MacMacaSetState(node, maca, MACA_S_REMOTE);
            MacMacaRemote(node, maca, msg);

            maca->NoisyPacketGot++;
        }

        break;
    }
    case MACA_S_YIELD:
    case MACA_S_PASSIVE:
    case MACA_S_BACKOFF:
    case MACA_S_REMOTE:
    {
        MacMacaCancelTimer(node, maca, MACA_T_UNDEFINED);
        MacMacaSetState(node, maca, MACA_S_REMOTE);
        MacMacaRemote(node, maca, msg);

        break;
    }
    case MACA_S_IN_XMITING_RTS:
    {
        /*
         * Receives mesg from phy even though in transmit mode.
         * This can possibly happen due to simultaneous messages.
         * The phy layer receives a mesg from the channel and
         * sends it up to the MAC layer at the same time as the
         * MAC layer sends down the RTS message
         */

        /*
         * Since the messages are simultaneous, we can assume that
         * the RTS message is sent before the incoming message
         * is received.  Thus we lose the incoming packet.
         */
        MESSAGE_Free(node, msg);
        break;
    }
    case MACA_S_XMIT:
    case MACA_S_IN_XMITING_CTS:
    case MACA_S_IN_XMITING_UNICAST:
    case MACA_S_IN_XMITING_BROADCAST:
        MESSAGE_Free(node, msg);
        assert(FALSE);
        break;

    default:
        MESSAGE_Free(node, msg);
        break;
    }/*switch*/
}

void MacMacaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataMaca* maca,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus)
{
   if (oldPhyStatus == PHY_TRANSMITTING) {
      assert(newPhyStatus != PHY_TRANSMITTING);

      switch (maca->state) {
      case MACA_S_IN_XMITING_UNICAST:
      {
          MacMacaResetTimer(node, maca);
          MacMacaSetState(node, maca, MACA_S_PASSIVE);
         
          MacMacaPassive(node, maca);
          break;
      }
      case MACA_S_IN_XMITING_BROADCAST:
      {
          MacMacaSetState(node, maca, MACA_S_YIELD);
          MacMacaYield(node, maca, MACA_VACATION);
          break;
      }
      case MACA_S_IN_XMITING_RTS:
      {
          clocktype holdForCts;

          holdForCts = (3 * MACA_DELAY) + (2 * MACA_PHY_DELAY) +
              (2 * PHY_GetTransmissionDelay(
                            node, maca->myMacData->phyNumber, 0)) +
              (2 * maca->myMacData->propDelay) +
              (sizeof(MacaHeader) * SECOND) /
              maca->myMacData->bandwidth +
              MACA_EXTRA_DELAY;

          MacMacaSetState(node, maca, MACA_S_RTS);

          
          MacMacaSetTimer(node, maca, MACA_T_RTS, holdForCts);

          break;
      }
      case MACA_S_IN_XMITING_CTS:
      {
          clocktype holdForData;

          MacMacaSetState(node, maca, MACA_S_REMOTE);

          holdForData = (3 * MACA_DELAY) + (2 * MACA_PHY_DELAY) +
                        (2 *
                         PHY_GetTransmissionDelay(
                            node, maca->myMacData->phyNumber, 0)) +
                        (2 * maca->myMacData->propDelay) +
                        ((maca->payloadSizeExpected +
                        sizeof(MacaHeader)) * SECOND) /
                        maca->myMacData->bandwidth +
                        MACA_EXTRA_DELAY;

          MacMacaSetTimer(node, maca, MACA_T_REMOTE, holdForData);

          break;
      }
      default:
          assert(FALSE); abort();
          break;
      }/*switch*/
   }//if//
}



/*
 * FUNCTION    MacMacaLayer
 * PURPOSE     Models the behaviour of the MAC layer with the MACA protocol
 *             on receiving the message enclosed in msg.
 *
 * Parameters:
 *     node:     node which received the message
 *     msg:      message received by the layer
 */

void MacMacaLayer(Node *node, int interfaceIndex, Message *msg)
{
    /*
     * Retrieve the pointer to the data portion which relates
     * to the MACA protocol.
     */
    MacDataMaca *maca = (MacDataMaca *) node->macData[interfaceIndex]->macVar;
    int seqNo;

    assert(msg->eventType == MSG_MAC_TimerExpired);

    seqNo = *((int *) MESSAGE_ReturnInfo(msg));
    MESSAGE_Free(node, msg);

    /* this timer has already been canceled */
    if ((seqNo < maca->timer.seq) ||
        ((maca->timer.flag &MACA_TIMER_SWITCH) == MACA_TIMER_OFF))
    {
        return;
    }

    assert(seqNo <= maca->timer.seq);

    switch (maca->timer.flag & MACA_TIMER_TYPE) {
    case MACA_T_RTS:
    {
        MacMacaSetState(node, maca, MACA_S_BACKOFF);
        MacMacaBackoff(node, maca);

        break;
    }
    case MACA_T_BACKOFF:
    {
        MacMacaSetState(node, maca, MACA_S_PASSIVE);
        
        MacMacaPassive(node, maca);

        break;
    }
    case MACA_T_REMOTE:
    {
        MacMacaSetState(node, maca, MACA_S_PASSIVE);
        
        MacMacaPassive(node, maca);

        break;
    }
    case MACA_T_YIELD:
    {
        MacMacaCancelTimer(node, maca, MACA_T_UNDEFINED);
        MacMacaSetState(node, maca, MACA_S_PASSIVE);
         
        MacMacaPassive(node, maca);

        break;
    }
    default:
        break;
    }/*switch*/
}
