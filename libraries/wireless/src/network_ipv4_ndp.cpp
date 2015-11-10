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
//
//--------------------------------------------------------------------------
//
// FILE       : ndp.c
//
// PURPOSE    : Implementing a neighbor discovery protocol for IARP.
//
// ASSUMPTION : This is a component protocol of IARP. See the file iarp.c.
//              This version of NDP transmits HELLO beacons at regular
//              intervals. Upon receiving a beacon, the neighbor table is
//              updated. Neighbors, for which no beacon has been received
//              within a specified time, are removed from the table.
//              IARP relies on this NDP to provide current information
//              about a node's neighbors. At the least, this information
//              must include the IP addresses of all the neighbors.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "api.h"
#include "network_ip.h"
#include "main.h"
#include "network.h"
#include "network_ipv4_ndp.h"
#include "fileio.h"

// The #define statements below are for ndp debugging.
#define NDP_DEBUG                         0
#define DEBUG_NDP_TEST_NDP_NEIGHBOR_TABLE 0
#define MUM_OF_TIMER_FIRED                15
#define DEBUG_DELAY                       (4 * MINUTE)
#define NDP_DEBUG_PERIODIC                0

//-------------------------------------------------------------------------
// FUNCTION     : NdpPrintHelloPacketContent()
//
// PURPOSE      : Printing the content of hello message. This is mainly
//                required for debugging purpose
//
// PARAMETERS   : node - Node which is printing the content of the packet
//                helloPacket - The hello packet to be printed. Pointer
//                              to the first byte of the packet
//                              is expected.
//                interfaceIndex - interface index thru which sending or
//                                 receiving the hello message
//                end - state sendin end or receiving end. state SENDING
//                      or RECEIVING
//
// RETURN VALUE : None.
//-------------------------------------------------------------------------
static
void NdpPrintHelloPacketContent(
    Node* node,
    char* helloPacket,
    NodeAddress senderOrReceiver,
    int interfaceIndex,
    const char* end)
{
    char subnetIpAddressStr[MAX_STRING_LENGTH] = {0};
    char ipAddressStr[MAX_STRING_LENGTH] = {0};
    NodeAddress subnetMask = (unsigned) NETWORK_UNREACHABLE;
    unsigned int helloInterval = 0;

    memcpy(&subnetMask, &helloPacket[0], sizeof(NodeAddress));
    memcpy(&helloInterval, &helloPacket[4], sizeof(unsigned int));
    IO_ConvertIpAddressToString(subnetMask, subnetIpAddressStr);
    IO_ConvertIpAddressToString(senderOrReceiver, ipAddressStr);

    printf("------------------------------------------------------------\n"
           "Node %u is %s a Hello message thru interface index %d\n"
           "%s node %s\n"
           "------------------------------------------------------------\n"
           " Subnet Mask = %s\n"
           " Hello Interval = %3u Second\n"
           "------------------------------------------------------------\n",
           node->nodeId,
           end,
           interfaceIndex,
           ((strcmp(end, "SENDING") == 0) ? "TO" :
               ((strcmp(end, "RECEIVING") == 0) ? "FROM" : " ")),
           ipAddressStr,
           subnetIpAddressStr,
           helloInterval);
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpPrintNeighborTable()
//
// PURPOSE      : Printing the neighbor table of the node
//
// PARAMETERS   : node - Node which  is printing neighbor table
//                ndpData - pointer to NdpData
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpPrintNeighborTable(Node* node, NdpData* ndpData)
{
    static int j = 0;
    int i = 0;
    FILE * fd = NULL;
    char time[MAX_STRING_LENGTH] = {0};

    if (j == 0)
    {
        j++;
        fd = fopen("neighborTable.out","w");
    }
    else
    {
        fd = fopen("neighborTable.out","a");
    }
    TIME_PrintClockInSecond(node->getNodeTime(), time);

    fprintf(fd,"------------------------------------------------------------"
           "-------------\n"
           "Simulation Time = %16s\n"
           "NEIGHBOR TABLE OF NODE : %u numEntry: %10u maxEntry: %10u\n"
           "------------------------------------------------------------"
           "-------------\n"
           "%16s %16s %17s %16s %3s\n"
           "------------------------------------------------------------"
           "-------------\n",
           time,
           node->nodeId,
           ndpData->ndpNeighborTable.numEntry,
           ndpData->ndpNeighborTable.maxEntry,
           "Neighbor_Address",
           "SubnetMask_Addr",
           "Hello_Interval(S)",
           "Time_Last_Hard",
           "NDX");

    for (i = 0; i < (signed)ndpData->ndpNeighborTable.numEntry; i++)
    {
        char ipAddressStr[MAX_STRING_LENGTH] = {0};
        char subnetIpAddressStr[MAX_STRING_LENGTH] = {0};
        char clockStr[MAX_STRING_LENGTH] = {0};

        IO_ConvertIpAddressToString(ndpData->ndpNeighborTable.
            neighborTable[i].source, ipAddressStr);

        IO_ConvertIpAddressToString(ndpData->ndpNeighborTable.
            neighborTable[i].subnetMask, subnetIpAddressStr);

        ctoa(ndpData->ndpNeighborTable.neighborTable[i].lastHard, clockStr);

        fprintf(fd,"%16s %16s %17u %16s %3d\n",
               ipAddressStr,
               subnetIpAddressStr,
               ndpData->ndpNeighborTable.neighborTable[i].
                  neighborHelloInterval,
               clockStr,
               ndpData->ndpNeighborTable.neighborTable[i].
                   interfaceIndex);
    }
    fprintf(fd,"------------------------------------------------------------"
           "-------------\n\n\n");
    fclose(fd);
}

//-------------------------------------------------------------------------
// FUNCTION     : NdpRegisterUpdateFunctionHandler()
//
// PURPOSE      : Registering a update function in the update function list.
//
// PARAMETERS   : ndpData - pointer to ndpData
//                updateFunction - update function to be registered.
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void NdpRegisterUpdateFunctionHandler(
    NdpData* ndpData,
    NdpUpdateFunctionType updateFunction)
{
    NdpUpdateFuncHandlar* updateFuncHandlerNode = (NdpUpdateFuncHandlar*)
        MEM_malloc(sizeof(NdpUpdateFuncHandlar));

    updateFuncHandlerNode->updateFunction = updateFunction;
    updateFuncHandlerNode->next = NULL;

    if (ndpData->updateFuncHandlerList.first == NULL)
    {
        ndpData->updateFuncHandlerList.first = updateFuncHandlerNode;
        ndpData->updateFuncHandlerList.last = updateFuncHandlerNode;
    }
    else
    {
        ndpData->updateFuncHandlerList.last->next = updateFuncHandlerNode;
        ndpData->updateFuncHandlerList.last = updateFuncHandlerNode;
    }
    ndpData->updateFuncHandlerList.numEntry++;
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpExecuteCallBackFunctions()
//
// PURPOSE      : Executing all the call back functions that are registered
//                thru "NdpSetUpdateFunction()" function.
//
// PARAMETERS   : node - node for which call function is executed.
//                ndpData - pointer to ndp data structure.
//                neighborAddress - neighbor joined/updated/deleated,
//                neighborSubnetMaskAddress - subnet mask address of
//                                            the neighbor.
//                lastHardTime - time when last hard from the neighbor
//                interfaceIndex - interface index thru which packet is
//                                 received
//                isNeighborReachable - is neighbor currently
//                                      reachable now?
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpExecuteCallBackFunctions(
    Node* node,
    NdpData* ndpData,
    NodeAddress neighborAddress,
    NodeAddress neighborSubnetMaskAddress,
    clocktype lastHardTime,
    int interfaceIndex,
    BOOL isNeighborReachable)
{
    if ((ndpData) && (ndpData->updateFuncHandlerList.numEntry > 0))
    {
        NdpUpdateFuncHandlar* updateFuncHandlerNode =
            ndpData->updateFuncHandlerList.first;

        while (updateFuncHandlerNode != NULL)
        {
            if (updateFuncHandlerNode->updateFunction != NULL)
            {
                // Call the update function thru function pointer.
                (updateFuncHandlerNode->updateFunction)(
                    node,
                    neighborAddress,
                    neighborSubnetMaskAddress,
                    lastHardTime,
                    interfaceIndex,
                    isNeighborReachable);
            }
            updateFuncHandlerNode = updateFuncHandlerNode->next;
        } // end while (updateFuncHandlerNode != NULL)
    }// end if ((ndpData) && (ndpData->updateFuncHandlerList.numEntry > 0))
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpSetUpdateFunction()
//
// PURPOSE      : Setting protocol defined update function handler that
//                will be called from NDP.
//
// PARAMETERS   : node - Node which is setting update function
//                updateFunction - update function pointer
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
void NdpSetUpdateFunction(
    Node* node,
    const NodeInput* nodeInput,
    NdpUpdateFunctionType updateFunction)
{
    NetworkDataIp *ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NdpData *ndpData = NULL;

    if (ipLayer->isNdpEnable == FALSE)
    {
        ipLayer->isNdpEnable = TRUE;
        NdpInit(node, &ipLayer->ndpData, nodeInput, ANY_INTERFACE);
    }

    ndpData = (NdpData *) ipLayer->ndpData;
    ERROR_Assert(ndpData, "NDP structure is null !!!");
    NdpRegisterUpdateFunctionHandler(ndpData, updateFunction);
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpInitNeighborCahce()
//
// PURPOSE      : initializing a neighbor cache
//
// PARAMETERS   : neighborCache - pointer to neighbor cache
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpInitNeighborCahce(NdpNeighborTable* neighborCache)
{
    neighborCache->neighborTable = (NdpNeighborStruct*)
        MEM_malloc(sizeof(NdpNeighborStruct) * NDP_INITIAL_CACHE_ENTRY);

    memset(neighborCache->neighborTable, 0,
        (sizeof(NdpNeighborStruct) * NDP_INITIAL_CACHE_ENTRY));

    neighborCache->numEntry = 0;
    neighborCache->maxEntry = NDP_INITIAL_CACHE_ENTRY;
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpAddDataToNeighborCache()
//
// PURPOSE      : Adding data to neighbor cache
//
// PARAMETERS   : neighborCache - pointer to neighbor cache
//                neighborData - neighbor data to be added
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpAddDataToNeighborCache(
    NdpNeighborTable* neighborCache,
    NdpNeighborStruct* neighborData)
{
    if (neighborCache && neighborData)
    {
        if (neighborCache->numEntry == neighborCache->maxEntry)
        {
            unsigned int newNeighborCacheSize = (unsigned int)
                ((neighborCache->numEntry + NDP_INITIAL_CACHE_ENTRY)
                    * sizeof(NdpNeighborStruct));

            NdpNeighborStruct* tempCache = (NdpNeighborStruct*)
                MEM_malloc(newNeighborCacheSize);
            memset(tempCache, 0, newNeighborCacheSize);

            memcpy(tempCache, neighborCache->neighborTable,
                (sizeof(NdpNeighborStruct) * neighborCache->numEntry));

            MEM_free(neighborCache->neighborTable);

            neighborCache->neighborTable = tempCache;

            neighborCache->maxEntry = neighborCache->numEntry
                                      + NDP_INITIAL_CACHE_ENTRY;
        }
        memcpy(&(neighborCache->neighborTable[neighborCache->numEntry]),
            neighborData, sizeof(NdpNeighborStruct));

        neighborCache->numEntry++;
    }// end if (neighborCache && neighborData)
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpDeleateDataFromNeighborCache()
//
// PURPOSE      : Deleating data From neighbor cache
//
// PARAMETERS   : neighborCache - pointer to neighbor cache
//                neighborData - neighbor data to be deleated
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpDeleateDataFromNeighborCache(
    NdpNeighborTable* neighborCache,
    NdpNeighborStruct* neighborData)
{
    if (neighborCache && neighborData)
    {
        unsigned int  i = 0;

        for (i = 0; i < neighborCache->numEntry; i++)
        {
            if (neighborCache->neighborTable[i].source ==
                neighborData->source)
            {
                memcpy(&(neighborCache->neighborTable[i]),
                       &(neighborCache->neighborTable[
                           neighborCache->numEntry - 1]),
                       sizeof(NdpNeighborStruct));

                memset(&(neighborCache->neighborTable[
                           neighborCache->numEntry - 1]), 0,
                       sizeof(NdpNeighborStruct));

                neighborCache->numEntry--;
                break;
            }
        }
    } // end if (neighborCache && neighborData)
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpSearchDataInNeighborCache()
//
// PURPOSE      : Searching data in neighbor cache
//
// PARAMETERS   : neighborCache - pointer to neighbor cache
//                neighborData - neighbor data to be deleated
//
// RETURN VALUE : Pointer to the entry found in the cache
//                or NULL otherwise.
//-------------------------------------------------------------------------
static
NdpNeighborStruct* NdpSearchDataInNeighborCache(
    NdpNeighborTable* neighborCache,
    NdpNeighborStruct* neighborData)
{
    unsigned int i = 0;
    for (i = 0; i < neighborCache->numEntry; i++)
    {
        ERROR_Assert(neighborCache->neighborTable[i].source,
            "Source Address cannot be NULL!!");

        if (neighborCache->neighborTable[i].source ==
            neighborData->source)
        {
            return &(neighborCache->neighborTable[i]);
        }
    }
    return NULL;
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpExtractInfoFromHelloPacket()
//
// PURPOSE      : Extracting hello information from hello packet.
//
// PARAMETERS   : node - node which is extracting the info from hello msg.
//                ndpNeighborInfo - Put neighbor info here.
//                sourceAddress - sender of the hello packet
//                interfaceIndex - interface index thru which packet
//                                 is received
//                packet - hello packet received.
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpExtractInfoFromHelloPacket(
    Node* node,
    NdpNeighborStruct* ndpNeighborInfo,
    NodeAddress sourceAddress,
    int interfaceIndex,
    char packet[])
{
    memcpy(&(ndpNeighborInfo->source), &sourceAddress, sizeof(NodeAddress));
    memcpy(&(ndpNeighborInfo->subnetMask), &packet[0], sizeof(NodeAddress));
    memcpy(&(ndpNeighborInfo->neighborHelloInterval), &packet[4],
        sizeof(unsigned int));
    memcpy(&(ndpNeighborInfo->interfaceIndex), &interfaceIndex,
        sizeof(int));
    ndpNeighborInfo->lastHard = node->getNodeTime();
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpUpdateNeighborTable()
//
// PURPOSE      : Updating Neighbor table with neighbor info
//
// PARAMETERS   : node - Node which is updating the neighbor table
//                ndpData - pointer to NdpData
//                ndpNeighborInfo - new incomong neighbor info
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpUpdateNeighborTable(
    Node* node,
    NdpData* ndpData,
    NdpNeighborStruct* ndpNeighborInfo,
    BOOL* isNewNeighbor)
{
    NdpNeighborStruct* existingNeighborInfo =
        NdpSearchDataInNeighborCache(&(ndpData->ndpNeighborTable),
            ndpNeighborInfo);

    *isNewNeighbor = FALSE;
    if (existingNeighborInfo != NULL)
    {
        // If neighbor already exist just update last heard time
        existingNeighborInfo->lastHard = node->getNodeTime();
    }
    else
    {
        NdpAddDataToNeighborCache(&(ndpData->ndpNeighborTable),
            ndpNeighborInfo);
        *isNewNeighbor = TRUE;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpSendHelloPacket()
//
// PURPOSE      : Sending hello packets to  all neighbors.
//
// PARAMETERS   : node - node which is sending the hello packet
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpSendHelloPacket(Node* node, NdpData* ndpData)
{
    int i = 0;
    Message* msg = NULL;
    unsigned char helloPacket[NDP_HELLO_PACKET_SIZE] = {0};

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress submetMask = NetworkIpGetInterfaceSubnetMask(node, i);
        unsigned int helloTime =
            (unsigned int)(ndpData->ndpPeriodicHelloTimer / SECOND);
        memcpy(&helloPacket[0], &submetMask, sizeof(NodeAddress));
        memcpy(&helloPacket[4], &helloTime, sizeof(unsigned int));

        msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_NDP,
                            MSG_DEFAULT);

        MESSAGE_PacketAlloc(node, msg, NDP_HELLO_PACKET_SIZE, TRACE_NDP);

        memcpy(MESSAGE_ReturnPacket(msg), helloPacket,
            NDP_HELLO_PACKET_SIZE);

        if (NDP_DEBUG)
        {
            NdpPrintHelloPacketContent(node, MESSAGE_ReturnPacket(msg),
                NetworkIpGetInterfaceBroadcastAddress(node, i), i,
                "SENDING");
        }

        if (NDP_DEBUG_PERIODIC)
        {
            static int i = 0;
            FILE * fd = NULL;
            char time[MAX_STRING_LENGTH] = {0};

            if (i == 0)
            {
                i++;
                fd = fopen("helloPeriodic.out","w");
            }
            else
            {
                fd = fopen("helloPeriodic.out","a");
            }
            TIME_PrintClockInSecond(node->getNodeTime(), time);
            fprintf(fd,"\n node %u send Hello at time %s",node->nodeId, time);
            fclose(fd);
        }

        NetworkIpSendRawMessageToMacLayer(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, i),
            // NetworkIpGetInterfaceBroadcastAddress(node, i),
            ANY_DEST,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_NDP,
            NDP_CONSTANT_TTL_VALUE,
            i,
            // NetworkIpGetInterfaceBroadcastAddress(node, i),
            ANY_DEST);

        // Increment Hello packet sending statics.
        ndpData->stats.numHelloSend++;
    } // end for (i = 0; i < node->numberInterfaces; i++)
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpGetNdpData()
//
// PURPOSE      : Getting Ndp information from node
//
// PARAMETERS   : node - the node structure
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
NdpData* NdpGetNdpData(Node* node)
{
    NetworkDataIp *ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    if (ipLayer->isNdpEnable == TRUE)
    {
        return ((NdpData*) ipLayer->ndpData);
    }
    else
    {
        return NULL;
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpScheduleHelloTimer()
//
// PURPOSE      : Scheduling a hello timer. When this timer will expire
//                NDP will send a hello packet.
//
// PARAMETERS   : node - Node which is scheduling the hello timer.
//                timer - the delay value
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
static
void NdpScheduleHelloTimer(Node* node, clocktype timer)
{
    clocktype scheduleTimer;

    NdpData* ndpData = NdpGetNdpData(node);

    // Apply ramdom JITTER not greater than 1000 MS
    scheduleTimer = RANDOM_nrand(ndpData->seed) % NDP_MAX_JITTER;

    Message* msg = MESSAGE_Alloc(
                       node,
                       NETWORK_LAYER,
                       NETWORK_PROTOCOL_NDP,
                       NETWORK_NdpPeriodicHelloTimerExpired);

    MESSAGE_Send(
        node,
        msg,
        scheduleTimer);
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpSetHoldTimerForTheNeighbor()
//
// PURPOSE      : This function sets a hold timer for a given neighbor
//
// PARAMETERS   : node - node which is setting the hold timer.
//                ndpData - pointer to NdpData
//                neighborInfo - neighbor for which hold timer is to be set.
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpSetHoldTimerForTheNeighbor(
    Node* node,
    NdpData* ndpData,
    NdpNeighborStruct* neighborInfo)
{

    clocktype delay = (ndpData->numHelloLossAllowed
        + neighborInfo->neighborHelloInterval) * SECOND;

    if (neighborInfo->neighborHelloInterval > 0)
    {
        Message* msg = MESSAGE_Alloc(
                           node,
                           NETWORK_LAYER,
                           NETWORK_PROTOCOL_NDP,
                           NETWORK_NdpHoldTimerExpired);

        MESSAGE_InfoAlloc(node, msg, sizeof(NdpNeighborStruct));

        memcpy(MESSAGE_ReturnInfo(msg), neighborInfo,
            sizeof(NdpNeighborStruct));

        MESSAGE_Send(node, msg, delay);
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpStatInit()
//
// PURPOSE      : Initializing statistical variables
//
// PARAMETERS   : ndpStatType - pointer to Ndp statistics structure
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
static
void NdpStatInit(NdpStatType* ndpStatType)
{
    ndpStatType->numHelloSend = 0;
    ndpStatType->numHelloReceived = 0;
    ndpStatType->isStatPrinted = FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpInit()
//
// PURPOSE      : Initializing NDP protocol.
//
// PARAMETERS   : node - Node which is initializing NDP
//                ndpData - Pointer to NdpData pointer
//                nodeInput - Node input
//                interfaceIndex - Interface index
//
// ASSUMPTION   : This function will be called from iarp.c.
//-------------------------------------------------------------------------
void NdpInit(
    Node* node,
    void** ndpData,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    NdpData* myNdp = NULL;
    BOOL retVal = FALSE;
    char buf[MAX_STRING_LENGTH] = {0};

    *ndpData =  MEM_malloc(sizeof(NdpData));
    memset(*ndpData, 0, sizeof(NdpData));

    myNdp = (NdpData*) *ndpData;
    NdpInitNeighborCahce(&(myNdp->ndpNeighborTable));
    myNdp->ndpPeriodicHelloTimer = NDP_PERIODIC_HELLO_TIMER;
    myNdp->numHelloLossAllowed = NDP_NUM_HELLO_LOSS_ALLOWED;

    RANDOM_SetSeed(myNdp->seed,
                   node->globalSeed,
                   node->nodeId,
                   NETWORK_PROTOCOL_NDP,
                   interfaceIndex);

    NdpStatInit(&(myNdp->stats));

    if (nodeInput != NULL)
    {
        // Read statistics collection option from configuration file.
        IO_ReadString(node->nodeId, ANY_DEST, nodeInput,
             "NDP-STATISTICS", &retVal, buf);
    }

    if ((retVal == TRUE) && (strcmp(buf, "YES") == 0))
    {
        myNdp->collectStats = TRUE;
    }
    else
    {
        myNdp->collectStats = FALSE;

    }

    if (NDP_DEBUG)
    {
        // DEBUG statement: Print nodeId of the node
        //                  that is initializing.
        printf("NDP protocol is Initializing in Node : %u\n",
            node->nodeId);
    }

    // Initialize update function pointer (linked) list.
    memset(&(myNdp->updateFuncHandlerList), 0,
        sizeof(NdpUpdateFuncHandlarList));

    // Start Sending hello messages immediately
    NdpScheduleHelloTimer(node, PROCESS_IMMEDIATELY);

    // This statements are for debugging of NDP.
    if (DEBUG_NDP_TEST_NDP_NEIGHBOR_TABLE)
    {
        int i = 0;

        for (i = 1; i <= MUM_OF_TIMER_FIRED; i++)
        {
            Message *dbg = MESSAGE_Alloc(node,
                                         NETWORK_LAYER,
                                         NETWORK_PROTOCOL_NDP,
                                         MSG_NETWORK_PrintRoutingTable);

            MESSAGE_Send(node, dbg, (i * DEBUG_DELAY));
        }
    }
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpHandleProtocolEvent(Node* node, Message* msg)
//
// PURPOSE      : Handling NDP timer related events.
//
// PARAMETER    : node - The node that is handling the event.
//                msg - the event that is being handled
//
// ASSUMPTION   : his function will be called from NetworkIpLayer()
//-------------------------------------------------------------------------
void NdpHandleProtocolEvent(Node* node, Message* msg)
{
    NdpData* ndpData = NdpGetNdpData(node);

    if (ndpData == NULL)
    {
        return;
    }

    switch (msg->eventType)
    {
        case NETWORK_NdpPeriodicHelloTimerExpired:
        {
            // Build and send Hello packet
            NdpSendHelloPacket(node, ndpData);

            // Schedule timer for next hello packet.
            NdpScheduleHelloTimer(node, ndpData->ndpPeriodicHelloTimer);
            break;
        }

        case NETWORK_NdpHoldTimerExpired:
        {
            NdpNeighborStruct* existingNeighborInfo =
                NdpSearchDataInNeighborCache(&(ndpData->ndpNeighborTable),
                    (NdpNeighborStruct*) MESSAGE_ReturnInfo(msg));

            if (existingNeighborInfo != NULL)
            {
                clocktype allowedDelay = (ndpData->numHelloLossAllowed
                    + existingNeighborInfo->neighborHelloInterval) * SECOND;

                clocktype currentTime = node->getNodeTime();

                if (currentTime > (existingNeighborInfo->lastHard
                     + allowedDelay))
                {
                    NdpNeighborStruct deletedNeighbor = {0};

                    // Keep a record of the deleated neighbor.
                    memcpy(&deletedNeighbor, existingNeighborInfo,
                        sizeof(NdpNeighborStruct));

                    // Neighbors Hold time expired. So delete the neighbor.
                    if (NDP_DEBUG)
                    {
                        char ipAddressStr[MAX_STRING_LENGTH] = {0};
                        IO_ConvertIpAddressToString(
                            existingNeighborInfo->source,
                            ipAddressStr);

                        printf("Neighbor to be deleated is %s\n",
                            ipAddressStr);
                        NdpPrintNeighborTable(node, ndpData);
                    }

                    NdpDeleateDataFromNeighborCache(
                        &(ndpData->ndpNeighborTable),
                        existingNeighborInfo);

                    if (NDP_DEBUG)
                    {
                        printf("Neighbor table after deletion:\n");
                        NdpPrintNeighborTable(node, ndpData);
                    }

                    NdpExecuteCallBackFunctions(
                        node,
                        ndpData,
                        deletedNeighbor.source,
                        deletedNeighbor.subnetMask,
                        deletedNeighbor.lastHard,
                        deletedNeighbor.interfaceIndex,
                        FALSE);
                }
                else
                {
                    // Set HoldTime for the neighbor a new.
                    NdpSetHoldTimerForTheNeighbor(
                        node,
                        ndpData,
                        existingNeighborInfo);
                }
            }
            break;
        }

        case MSG_NETWORK_PrintRoutingTable:
        {
            // This case statement is for debugging purpose only
            if (DEBUG_NDP_TEST_NDP_NEIGHBOR_TABLE)
            {
                NdpPrintNeighborTable(node, ndpData);
            }
            break;
        }

        default:
        {
            // Control of the code must not reach here.
            ERROR_Assert(FALSE, "Unknown event in NDP");
            break;
        }
    } // end switch (msg->eventType)
    MESSAGE_Free(node, msg);
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpHandleProtocolPacket(Node* node, Message* msg,  NodeAddress sourceAddress)
//
// PURPOSE      : Processing receipt of an NDP hello.
//
// PARAMETERS   : node - Node receiving the packet
//                msg - The packet that has been received.
//                sourceAddress - Source address of the packet
//                interfaceIndex - interface index of the packet.
//
// ASSUMPTION   : This function will be called from "DeliverPacket()"
//                in ip.c
//-------------------------------------------------------------------------
void NdpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    int interfaceIndex)
{
    NdpNeighborStruct ndpNeighborInfo = {0};
    NdpData* ndpData = NdpGetNdpData(node);
    char* packet = MESSAGE_ReturnPacket(msg);
    BOOL isNewNeighbor = FALSE;

    if (ndpData != NULL)
    {
        if (NDP_DEBUG)
        {
            NdpPrintHelloPacketContent(node, packet, sourceAddress,
                interfaceIndex, "RECEIVING");
        }

        // Constract NdpNeighborStructure from hello packet.
        NdpExtractInfoFromHelloPacket(
            node,
            &ndpNeighborInfo,
            sourceAddress,
            interfaceIndex,
            packet);

        // Update Neghbor Table with newly received information.
        if (NDP_DEBUG)
        {
            // DEBUG statement : print neignbor table before Update.
            printf("Before updating neighbor table\n");
            NdpPrintNeighborTable(node, ndpData);
        }

        NdpUpdateNeighborTable(
            node,
            ndpData,
            &ndpNeighborInfo,
            &isNewNeighbor);

        if (NDP_DEBUG)
        {
            // DEBUG statement : print neignbor table after Update.
            printf("After updating neighbor table\n");
            NdpPrintNeighborTable(node, ndpData);
        }
        NdpExecuteCallBackFunctions(
            node,
            ndpData,
            ndpNeighborInfo.source,
            ndpNeighborInfo.subnetMask,
            node->getNodeTime(),
            ndpNeighborInfo.interfaceIndex,
            TRUE);

        if (isNewNeighbor == TRUE)
        {
            // Set HoldTime for the newly joined neighbor.
            NdpSetHoldTimerForTheNeighbor(node, ndpData, &ndpNeighborInfo);
        }

        // Increment Hello packet receiving statics
        ndpData->stats.numHelloReceived++;
    }

    MESSAGE_Free(node, msg);
}


//-------------------------------------------------------------------------
// FUNCTION     : NdpFinalize(Node* node)
//
// PURPOSE      : Printing the statisdics collected during simulation.
//
// PARAMETERS   : node - Node which is printing the statistics.
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
void NdpFinalize(Node* node)
{
    NdpData* ndpData = NdpGetNdpData(node);
    char buf[MAX_STRING_LENGTH] = {0};

    if ((ndpData) && (ndpData->collectStats == TRUE) &&
        (ndpData->stats.isStatPrinted == FALSE))
    {
        // Print number of hello message send
        sprintf(buf, "Number of Hello Message Send = %u",
                ndpData->stats.numHelloSend);
        IO_PrintStat(node, "Network", "NDP", ANY_DEST,
            ANY_INTERFACE, buf);

        // Print number of hello message received
        sprintf(buf, "Number of Hello Message Reseived = %u",
                ndpData->stats.numHelloReceived);
        IO_PrintStat(node, "Network", "NDP", ANY_DEST,
            ANY_INTERFACE, buf);

        if (NDP_DEBUG)
        {
            // DEBUG statement.: print neignbor table
            NdpPrintNeighborTable(node, ndpData);
        }
        ndpData->stats.isStatPrinted = TRUE;
    }
}

