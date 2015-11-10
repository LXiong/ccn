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
 * This model implements an abstract store-and-forward
 * switched ethernet single subnet LAN with uniform
 * bandwidth and propagation delay on all links.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "mac_switched_ethernet.h"
///GuiStart
#include "gui.h"
//GuiEnd
//#define DEBUG

enum {
    IDLE,
    BUSY
};


void
ReturnSwitchedEthernetSubnetParameters(
    Node* node,
    int interfaceIndex,
    const Address* address,
    const NodeInput* nodeInput,
    Int64 *subnetBandwidth,
    clocktype *subnetPropDelay)
{
    BOOL wasFound;
    char addressStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString((Address*)address, addressStr);

    IO_ReadInt64(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "SUBNET-DATA-RATE",
        &wasFound,
        subnetBandwidth);

    if (!wasFound)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Unable to read \"SUBNET-DATA-RATE\" for "
                          "interface address %s", addressStr);
        ERROR_ReportError(errorStr);
    }

    IO_ReadTime(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "SUBNET-PROPAGATION-DELAY",
        &wasFound,
        subnetPropDelay);

    if (!wasFound)
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Unable to read \"SUBNET-PROPAGATION-DELAY\" for "
                          "interface address %s", addressStr);
        ERROR_ReportError(errorStr);
    }
}


void
SwitchedEthernetInit(
    Node *node,
    const NodeInput* nodeInput,
    int interfaceIndex,
    SubnetMemberData* nodeList,
    int numNodesInSubnet)
{
    MacSwitchedEthernetData *eth;

    eth = (MacSwitchedEthernetData *)
          MEM_malloc(sizeof(MacSwitchedEthernetData));

    memset(eth, 0, sizeof(MacSwitchedEthernetData));

    eth->myMacData = node->macData[interfaceIndex];
    eth->myMacData->macVar = (void *) eth;

    eth->status = IDLE;
    eth->busyUntil = 0;

    eth->nodeList = nodeList;
    eth->numNodesInSubnet = numNodesInSubnet;

    eth->stats.packetsSent = 0;
    eth->stats.packetsReceived = 0;

#ifdef PARALLEL //Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node,
        eth->myMacData->propDelay);
#endif //endParallel

#ifdef DEBUG
    printf("#%d/%d: SwitchedEthernetInit()\n", node->nodeId, interfaceIndex);
    printf("\tbandwidth = %lld / delay = %f\n",
           eth->myMacData->bandwidth,
           (double) eth->myMacData->propDelay / SECOND);
#endif
}


void
SwitchedEthernetFinalize(
    Node *node,
    int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macStats == TRUE)
    {
        MacSwitchedEthernetData *eth;
        char buf[MAX_STRING_LENGTH];
        char buf1[MAX_STRING_LENGTH];

        eth = (MacSwitchedEthernetData *)
              node->macData[interfaceIndex]->macVar;

        assert(eth != NULL);

        ctoa(eth->stats.packetsSent, buf1);
        sprintf(buf, "Frames sent = %s", buf1);
        IO_PrintStat(
            node,
            "MAC",
            "EthernetSwitch",
            ANY_DEST,
            interfaceIndex,
            buf);

        ctoa(eth->stats.packetsReceived, buf1);
        sprintf(buf, "Frames received = %s", buf1);
        IO_PrintStat(
            node,
            "MAC",
            "EthernetSwitch",
            ANY_DEST,
            interfaceIndex,
            buf);
    }

    // Free memory allocated for MacSwitchedEthernetData at initialization
    MacSwitchedEthernetData *eth;
    eth = (MacSwitchedEthernetData *)
          node->macData[interfaceIndex]->macVar;
    if (eth != NULL)
    {
        if (eth->hwAddrMap != NULL)
        {
            delete eth->hwAddrMap;
        }
        MEM_free(eth);
    }
}


static void
SwitchedEthernetSend(
    // TX delay based on sender's bandwidth
    clocktype txDelay, 
    Node *destNode,
    int interfaceIndex,
    Message *msgToSend)
{
    MacSwitchedEthernetData *eth;
    clocktype arrivalTime;
    clocktype timeToClearPipe;

    eth = (MacSwitchedEthernetData *)
          destNode->macData[interfaceIndex]->macVar;

    MESSAGE_SetEvent(msgToSend, MSG_MAC_LinkToLink);
    MESSAGE_SetLayer(msgToSend, MAC_LAYER, 0);

    MESSAGE_SetInstanceId(msgToSend, (short)interfaceIndex);

    timeToClearPipe = eth->busyUntil - destNode->getNodeTime();

    if (timeToClearPipe > 0)
    {
        arrivalTime
            = txDelay + MAX(0, eth->myMacData->propDelay - timeToClearPipe);
    }
    else
    {
        arrivalTime = txDelay + eth->myMacData->propDelay;
    }

    eth->busyUntil = MAX(eth->busyUntil + arrivalTime,
                         destNode->getNodeTime() + arrivalTime);

    MESSAGE_Send(destNode, msgToSend, eth->busyUntil - destNode->getNodeTime());

#ifdef DEBUG
    printf("\tSwitchedEthernetSend (to %d/%d)\n", destNode->nodeId,
                                                    interfaceIndex);
#endif

}


static void
SwitchedEthernetBroadcast(
    // TX delay based on sender's bandwidth
    clocktype txDelay,
    Node *node,
    MacSwitchedEthernetData *eth,
    Message *msgToSend)
{
    int i = 0;
    int remainingCopies = 1;

#ifdef DEBUG
    printf("#%d: SwitchedEthernetBroadcast(%d nodes)\n",
           node->nodeId, eth->numNodesInSubnet);
#endif
//GuiStart
    if (node->guiOption == TRUE){
        GUI_Broadcast(node->nodeId,
                          GUI_MAC_LAYER,
                          GUI_DEFAULT_DATA_TYPE,
                          eth->myMacData->interfaceIndex,
                          node->getNodeTime());
    }
//GuiEnd
    while (1)
    {
        if (eth->nodeList[i].node == node)
        {
            ERROR_Assert(
                remainingCopies == 1,
                "This node appears in the subnet list twice");

            remainingCopies--;
            i++;
        }

        if (i < (eth->numNodesInSubnet - remainingCopies - 1))
        {
            Message *newMsg;

            newMsg = MESSAGE_Duplicate(node, msgToSend);

            SwitchedEthernetSend(
                txDelay,
                eth->nodeList[i].node,
                eth->nodeList[i].interfaceIndex,
                newMsg);

            i++;
        }
        else
        {
            SwitchedEthernetSend(
                txDelay,
                eth->nodeList[i].node,
                eth->nodeList[i].interfaceIndex,
                msgToSend);

            break;
        }
    }
}


static void
SwitchedEthernetFindDestinationAndSendOrBroadcast(
    // TX delay based on sender's bandwidth
    clocktype txDelay,
    Node *node,
    MacSwitchedEthernetData *eth,
    Mac802Address destAddr,
    Message *msgToSend)
{
    // Build hw address -> node* map
    if (eth->hwAddrMap == NULL)
    {
        eth->hwAddrMap = new std::map<Mac802Address, int>;
        for (int i = 0; i < eth->numNodesInSubnet; i++)
        {
            Node* subnetNode = eth->nodeList[i].node;

            Mac802Address tempDestAddr;
            ConvertVariableHWAddressTo802Address(
                    node,
                    &subnetNode->macData[eth->nodeList[i].interfaceIndex]->macHWAddr,
                    &tempDestAddr);

            (*eth->hwAddrMap)[tempDestAddr] = i;
        }
    }

    std::map<Mac802Address, int>::iterator it;

    it = eth->hwAddrMap->find(destAddr);
    if (it != eth->hwAddrMap->end())
    {
        int nodeIndex = it->second;
        SwitchedEthernetSend(
            txDelay,
            eth->nodeList[nodeIndex].node,
            eth->nodeList[nodeIndex].interfaceIndex,
            msgToSend);

        return;
    }

    SwitchedEthernetBroadcast(
        txDelay,
        node,
        eth,
        msgToSend);
}

void
SwitchedEthernetNetworkLayerHasPacketToSend(
    Node *node,
    MacSwitchedEthernetData *eth)
{
    Message *newMsg = NULL;
    Message *txFinishedMsg;
    clocktype txDelay;
    MacHWAddress nextHop;
    int networkType;
    TosType priority;
    MacSwitchedEthernetFrameHeader *header;
    MacData *thisMac = eth->myMacData;
    int interfaceIndex = thisMac->interfaceIndex;
    NodeAddress ipDestAddr = 0;

    if (eth->status == BUSY)
    {
        return;
    }

    assert(eth->status == IDLE);

    MAC_OutputQueueDequeuePacket(
       node, interfaceIndex, &newMsg, &nextHop, &networkType, &priority);
    assert(newMsg != NULL);

    char *payload = MESSAGE_ReturnPacket(newMsg);
    payload = MacSkipLLCandMPLSHeader(node, payload);

    IpHeaderType *ipHeader = (IpHeaderType *)payload;
    ipDestAddr = ipHeader->ip_dst;

    MESSAGE_AddHeader(node,
                      newMsg,
                      sizeof(MacSwitchedEthernetFrameHeader),
                      TRACE_SWITCHED_ETHERNET);

    header = (MacSwitchedEthernetFrameHeader *) MESSAGE_ReturnPacket(newMsg);


    ConvertVariableHWAddressTo802Address(node,
                &node->macData[eth->myMacData->interfaceIndex]->macHWAddr,
                &header->sourceAddr);
    
    ConvertVariableHWAddressTo802Address(node, &nextHop, &header->destAddr);


    txDelay = (clocktype) (MESSAGE_ReturnPacketSize(newMsg) * 8 * SECOND
                           / thisMac->bandwidth);

    MESSAGE_SetEvent(newMsg, MSG_MAC_LinkToLink);
    MESSAGE_SetLayer(newMsg, MAC_LAYER, 0);

    BOOL isBroadCast = MAC_IsBroadcastMac802Address(&header->destAddr);

    if (isBroadCast)
    {
        // is MPLS enabled?
        if (node->macData[interfaceIndex]->mplsVar != NULL)
        {
            SwitchedEthernetBroadcast(
                txDelay,
                node,
                eth,
                newMsg);
        }
        else // we can look at the IP header because we would have stored
             // the MAC address for local LAN nodes in the switch's cache
        {

            Mac802Address dest;
            IPv4AddressToDefaultMac802Address(node, interfaceIndex,
                                              ipDestAddr, &dest);
            SwitchedEthernetFindDestinationAndSendOrBroadcast(
                txDelay,
                node,
                eth,
                dest,
                newMsg);

        }
    }
    else
    {
        SwitchedEthernetFindDestinationAndSendOrBroadcast(
            txDelay,
            node,
            eth,
            header->destAddr,
            newMsg);
    }

    eth->status = BUSY;

    txFinishedMsg = MESSAGE_Alloc(
                        node,
                        MAC_LAYER,
                        0,
                        MSG_MAC_TransmissionFinished);

    MESSAGE_SetInstanceId(txFinishedMsg, (short)interfaceIndex);
    MESSAGE_Send(node, txFinishedMsg, txDelay);

#ifdef DEBUG
    printf("#%d: SwitchedEthernetNetworkLayerHasPacketToSend(to ",
           node->nodeId);
    MAC_PrintMacAddr(&header->destAddr);
    printf(" )\n");
#endif
}


static void
SwitchedEthernetMessageFromWire(
    Node *node,
    MacSwitchedEthernetData *eth,
    int interfaceIndex,
    Message *msg)
{
    MacSwitchedEthernetFrameHeader *header;

    header = (MacSwitchedEthernetFrameHeader *) MESSAGE_ReturnPacket(msg);

    //assert(header->destAddr == ANY_DEST
    //       || header->destAddr
    //          == NetworkIpGetInterfaceAddress(node, interfaceIndex)
    //       || header->destAddr
    //          == NetworkIpGetInterfaceBroadcastAddress(node, interfaceIndex));

    MacHWAddress sourceAddr;
    Convert802AddressToVariableHWAddress(node, &sourceAddr, &header->sourceAddr);
    
    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(MacSwitchedEthernetFrameHeader),
                         TRACE_SWITCHED_ETHERNET);

#ifdef DEBUG
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %ld received packet from node",node->nodeId);
        MAC_PrintMacAddr(&header->sourceAddr);
        printf(" at time %s\n", clockStr);
        printf("    handing off packet to network layer\n");
    }
#endif

    MAC_HandOffSuccessfullyReceivedPacket(
        node,
        interfaceIndex,
        msg,
        &sourceAddr);
    eth->stats.packetsReceived++;
}


static void
SwitchedEthernetTransmissionFinished(
    Node *node,
    MacSwitchedEthernetData *eth,
    int interfaceIndex,
    Message *msg)
{
    assert(eth->status == BUSY);

    eth->status = IDLE;
    eth->stats.packetsSent++;

#ifdef DEBUG
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %ld finished transmitting packet at time %s\n",
                node->nodeId, clockStr);
    }
#endif

    if (MAC_OutputQueueIsEmpty(node, interfaceIndex) != TRUE)
    {
#ifdef DEBUG
        printf("    there's more packets in the queue, "
               "so try to send the next packet\n");
#endif

        SwitchedEthernetNetworkLayerHasPacketToSend(node, eth);
    }

    MESSAGE_Free(node, msg);
}


void
SwitchedEthernetLayer(
    Node *node,
    int interfaceIndex,
    Message *msg)
{
    MacSwitchedEthernetData *eth =
        (MacSwitchedEthernetData *) node->macData[interfaceIndex]->macVar;

    assert(eth != NULL);

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_MAC_FromNetwork:
        {
#ifdef DEBUG
            {
                char clockStr[20];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                printf("Node %ld received packet to send from network layer "
                       "at time %s\n",
                        node->nodeId, clockStr);
            }
#endif

            SwitchedEthernetNetworkLayerHasPacketToSend(node, eth);
            break;
        }
        case MSG_MAC_LinkToLink:
        {
            SwitchedEthernetMessageFromWire(
                node,
                eth,
                interfaceIndex,
                msg);
            break;
        }
        case MSG_MAC_TransmissionFinished:
        {
            SwitchedEthernetTransmissionFinished(
                node,
                eth,
                interfaceIndex,
                msg);
            break;
        }
        default:
        {
            assert(FALSE); abort();
        }
    }
}
