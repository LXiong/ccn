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
#include "routing_iarp.h"

//UserCodeStartBodyData
#include <limits.h>

#define IARP_DEFAULT_ZONE_RADIUS   2
#define IARP_DEFAULT_BROADCAST_TIMER   (10 * SECOND)
#define IARP_DEFAULT_ROUTE_REFRSH_TIMER   (20 * SECOND)
#define IARP_LINK_STATE_TIMER   (40 * SECOND)
#define IARP_STARTUP_DELAY   (10 * MILLI_SECOND)
#define IARP_HEADER_SIZE   12
#define IARP_NEIGHBOR_INFO_SIZE   12
#define IARP_INITIAL_SEQ_NUM   1
#define MAX_NODE_ENTRY   1024
#define NO_NEXT_HOP   -1
#define IARP_DEBUG_UPDATE   0
#define IARP_DEBUG_LINK_STATE_TABLE   0
#define IARP_DEBUG_NDP   0
#define IARP_DEBUG_TABLE   0
#define IARP_DEBUG_FORWARD   0
#define IARP_DEBUG_PERIODIC   0
#define IARP_DEBUG_SHORTEST_PATH   0
#define NetworkIpGetRoutingProtocol(N,P)   IarpGetRoutingProtocol(N,P)
#define IARP_INITIAL_ROUTE_TABLE_ENTRY   8


//UserCodeEndBodyData

//Statistic Function Declarations
static void IarpInitStats(Node* node, IarpStatsType *stats);
void IarpPrintStats(Node *node);

//Utility Function Declarations
static void IarpUpdateIerpRoutingTable(Node* node,IarpData* iarpData);
static void IarpReceiveNdpUpdate(Node* node,NodeAddress neighborAddress,NodeAddress neighborSubnetMaskAddress,clocktype lastHard,int interfaceIndex,BOOL isNeighborReachable);
static void IarpInitializeRoutingTable(IarpRoutingTable* iarpRoutingTable);
static void IarpScheduleBroadcastTimer(Node* node,IarpData* iarpData,clocktype timeDelay);
static void IarpDeleteRouteFromRoutingTable(Node* node,IarpData* iarpData,NodeAddress destinationAddress);
static void IarpReadZoneRadiusFromConfigurationFile(Node* node,const NodeInput* nodeInput,unsigned* zoneRadiusRead);
static void IarpInitializePrivateDataStructures(Node* node,IarpData* iarpData);
static void IarpProcessNdpOrIarpUpdatePacket(Node* node,IarpData* iarpData,IarpRoutingEntry* iarpRoutingInfo,IarpUpdatePacketType iarpUpdatePacketType,NodeAddress linkSource,unsigned int zoneRadius,IarpMetric metric,IarpLinkStateTableEntry* iarpLinkStateTableEntry);
static void IarpReadStatisticsOptionFromConfigurationFile(Node* node,IarpData* iarpData,const NodeInput* nodeInput);
static BOOL IarpCheckForInZoneNode(IarpData* iarpData,unsigned int hopCount);
static void IarpUpdateIarpRouteTable(Node* node,IarpData* iarpData,IarpRoutingEntry* iarpRoutingEntry,IarpUpdatePacketType iarpUpdatePacketType,NodeAddress linkSource,unsigned int zoneRadius,IarpMetric iarpMetric,IarpLinkStateTableEntry* linkStateEntryPtr,BOOL* isRouteTableUpdated);
static void IarpInitializeLinkStateTable(IarpData* iarpData);
static IarpLinkStateTableEntry* IarpAddLinkSourceToLinkStateTable(IarpData* iarpData,NodeAddress linkSourceAddress,unsigned int zoneRadius,NodeAddress linkStateId,clocktype insertTime);
static IarpLinkStateTableEntry* IarpFindPlaceForLinkSourceEntry(IarpData* iarpData,NodeAddress linkSource,unsigned int zoneRadius,NodeAddress linkStateId,clocktype insertTime);
static IarpLinkStateInfo* IarpAddLinkStateInfo(IarpLinkStateTableEntry* iarpLinkStateTableEntry,IarpLinkStateInfo* linkStateInfo);
static void IarpPrintLinkStateTable(Node* node,IarpData* iarpData);
static void IarpPrintRoutingTable(Node* node,IarpData* iarpData);
static void IarpBuildUpdatePacket(IarpData* iarpData,PacketBuffer* * packetBuffer);
static void IarpAdvancePacketPtr(char* * packetPtr,int numBytes);
static unsigned short IarpIncrementLinkStateSeqNum(IarpData* iarpData);
static void IarpBuildHeader(IarpData* iarpData,char* headerPtr);
static void IarpIncludeLinkStateInformationInPacket(IarpData* iarpData,char* dataPacket);
static void IarpEncapsulatePacketInTheMessage(Node* node,Message* * msg,PacketBuffer* packetBuffer);
static void IarpReadHeaderInformation(IarpGeneralHeader* iarpHeader,char* packet);
static void IarpScheduleRefreshTimer(Node* node,IarpData* iarpData);
static void IarpReBroadcastUpdatePacket(Node* node,IarpData* iarpData,Message* msg);
static void IarpFindShortestPathBFS(Node* node,IarpData* iarpData);
static void IarpFlushRoutingTable(Node* node,IarpData* iarpData);
static SeqTable* SearchLinkSourceInSeqTable(SeqTable* root,NodeAddress linkSource);
static SeqTable* InsertIntoSeqTable(SeqTable* root,NodeAddress linkSource,unsigned short seqNum);

//State Function Declarations
static void enterIarpIdle(Node* node, IarpData* dataPtr, Message* msg);
static void enterIarpProcessAgedOutRoutes(Node* node, IarpData* dataPtr, Message* msg);
static void enterIarpProcessBroadcastTimerExpired(Node* node, IarpData* dataPtr, Message* msg);

//Utility Function Definitions
//UserCodeStart
// FUNCTION IarpUpdateIerpRoutingTable
// PURPOSE  Updating Ierp routing table with Ierp information
// Parameters:
//  node:   node which is updating ierp routing table
//  iarpData:   pointer to iarp data structure
static void IarpUpdateIerpRoutingTable(Node* node,IarpData* iarpData) {
    if ((iarpData->ierpUpdateFunc) != NULL)
    {
        int i = 0;
        IarpRoutingTable* iarpRoutingTable = &(iarpData->routeTable);
        IarpRoutingEntry* iarpRoutingTableEntry =
            iarpRoutingTable->routingTable;
        for (i = 0; i < (signed)iarpRoutingTable->numEntries; i++)
        {
            // Update ierp routing table thru ugly
            // function pointer call
            iarpData->ierpUpdateFunc(
                node,
                iarpRoutingTableEntry[i].destinationAddress,
                iarpRoutingTableEntry[i].subnetMask,
                iarpRoutingTableEntry[i].path,
                iarpRoutingTableEntry[i].hopCount);
        }
    }
}


// FUNCTION IarpReceiveNdpUpdate
// PURPOSE  Receiving and processing NDP update.
// Parameters:
//  node:   node which is receiving the ndp update
//  neighborAddress:    IP address of neighbor
//  neighborSubnetMaskAddress:  subnet address of the neighbor.
//  lastHard:   time when last hard from the neighbor
//  interfaceIndex: interface index thru which ndp update is received.
//  isNeighborReachable:    informs the iarp if the neighbor is reachable
static void IarpReceiveNdpUpdate(Node* node,NodeAddress neighborAddress,NodeAddress neighborSubnetMaskAddress,clocktype lastHard,int interfaceIndex,BOOL isNeighborReachable) {
    IarpData *iarpData = (IarpData*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IARP);
    IarpRoutingEntry iarpRoutingInfo = {0};
    NodeAddress linkSource = ANY_DEST;
    IarpLinkStateTableEntry* iarpLinkStateTableEntry = NULL;
    // If this is NULL check if ZRP is running. Because the entire
    // ierp protocol data is within ZRP.
    if (iarpData == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(iarpData, "IARP dataPtr should not be NULL !!!");
        return;
    }
    linkSource = iarpData->myLinkSourceAddress;
    iarpRoutingInfo.destinationAddress = neighborAddress;
    if (isNeighborReachable == TRUE)
    {
        iarpRoutingInfo.nextHopAddress = neighborAddress;
    }
    else
    {
        iarpRoutingInfo.nextHopAddress = (NodeAddress) NETWORK_UNREACHABLE;
    }
    iarpRoutingInfo.subnetMask = neighborSubnetMaskAddress;
    iarpRoutingInfo.hopCount = 1;
    iarpRoutingInfo.outGoingInterface = interfaceIndex;
    iarpRoutingInfo.lastHard = lastHard;
    iarpRoutingInfo.path = NULL;
    iarpLinkStateTableEntry = IarpFindPlaceForLinkSourceEntry(
                                  iarpData,
                                  linkSource,
                                  iarpData->zoneRadius,
                                  linkSource,
                                  node->getNodeTime());
    // Update IARP link state table and take Accessory actions.
    IarpProcessNdpOrIarpUpdatePacket(
        node,
        iarpData,
        &iarpRoutingInfo,
        NDP_PERIODIC_UPDATE,
        linkSource,
        iarpData->zoneRadius,
        IARP_METRIC_HOP_COUNT,
        iarpLinkStateTableEntry);
    // Flush out IARP routing table
    IarpFlushRoutingTable(node, iarpData);
    // Calculate Shortest path and update route table
    IarpFindShortestPathBFS(node, iarpData);
    // Update IERP route table
    IarpUpdateIerpRoutingTable(node, iarpData);
}


// FUNCTION IarpInitializeRoutingTable
// PURPOSE  initializing routing table
// Parameters:
//  iarpRoutingTable:   pointer to the routing table
static void IarpInitializeRoutingTable(IarpRoutingTable* iarpRoutingTable) {
    // initializing IARP routing table
    iarpRoutingTable->routingTable =
        (IarpRoutingEntry *)
        MEM_malloc(sizeof(IarpRoutingEntry)
            * IARP_INITIAL_ROUTE_TABLE_ENTRY);
    memset(iarpRoutingTable->routingTable, 0,
    (sizeof(IarpRoutingEntry) * IARP_INITIAL_ROUTE_TABLE_ENTRY));
    iarpRoutingTable->maxEntries = IARP_INITIAL_ROUTE_TABLE_ENTRY;
    iarpRoutingTable->numEntries = 0;
}


// FUNCTION IarpScheduleBroadcastTimer
// PURPOSE  Scheduling broadcast timer. A routing update will be
//          broadcasted after the expiration of the timer
// Parameters:
//  node:   node which is firing the broadcast timer
//  iarpData:   pointer to IARP data structure
//  timeDelay:  the delay-time value
static void IarpScheduleBroadcastTimer(Node* node,IarpData* iarpData,clocktype timeDelay) {
    Message* timer = MESSAGE_Alloc(
                         node,
                         NETWORK_LAYER,
                         ROUTING_PROTOCOL_IARP,
                         ROUTING_IarpBroadcastTimeExpired);
    MESSAGE_Send(node, timer, timeDelay);
}


// FUNCTION IarpEnterRouteInRoutingTable
// PURPOSE  EnteringRoute In the routing table
// Parameters:
//  node:   node which is adding data into the IARP routing table
//  iarpData:   pointer to Iarp data structure
//  iarpRoutingTable:
//  iarpRoutingEntry:   iarp routing table entry (a row in the routing table)
IarpRoutingEntry* IarpEnterRouteInRoutingTable(Node* node,IarpData* iarpData,IarpRoutingTable* iarpRoutingTable,IarpRoutingEntry* iarpRoutingEntry) {

   if (iarpRoutingTable->numEntries == iarpRoutingTable->maxEntries)
    {
        unsigned int newExtendedSize = (sizeof(IarpRoutingEntry)
            * (iarpRoutingTable->maxEntries
               + IARP_INITIAL_ROUTE_TABLE_ENTRY));
        IarpRoutingEntry* newRoutingTable = (IarpRoutingEntry*)
            MEM_malloc(newExtendedSize);
        memset(newRoutingTable, 0, newExtendedSize);
        memcpy(newRoutingTable, iarpRoutingTable->routingTable,
            (sizeof(IarpRoutingEntry) * iarpRoutingTable->numEntries));
        MEM_free(iarpRoutingTable->routingTable);
        iarpRoutingTable->routingTable = newRoutingTable;
        iarpRoutingTable->maxEntries += IARP_INITIAL_ROUTE_TABLE_ENTRY;
    }
    memcpy(&(iarpRoutingTable->routingTable[iarpRoutingTable->numEntries]),
        iarpRoutingEntry,
        sizeof(IarpRoutingEntry));
    iarpRoutingTable->numEntries++;
    return &(iarpRoutingTable->routingTable
             [iarpRoutingTable->numEntries - 1]);
}


// FUNCTION IarpSearchRouteInRoutingTableEntry
// PURPOSE  searching a route in routing table
// Parameters:
//  iarpData:   pointer to IarpData
//  iarpRoutingTable:
//  destinationAddress: entry to be searched in the routing table
IarpRoutingEntry* IarpSearchRouteInRoutingTableEntry(IarpData* iarpData,IarpRoutingTable* iarpRoutingTable,NodeAddress destinationAddress) {

    int i = 0;
    IarpRoutingEntry* iarpRoutingTableEntry =
        iarpRoutingTable->routingTable;
    for (i = 0; i < (signed)iarpRoutingTable->numEntries; i++)
    {
        if (iarpRoutingTableEntry[i].destinationAddress ==
                destinationAddress)
        {
            return &(iarpRoutingTableEntry[i]);
        }
    }
    return NULL;
}


// FUNCTION IarpDeleteRouteFromRoutingTable
// PURPOSE  removing a given route entry from routing table
// Parameters:
//  node:   node which is deleting the route
//  iarpData:   pointer to iarp data structure
//  destinationAddress: routing entry to be deleted
static void IarpDeleteRouteFromRoutingTable(Node* node,IarpData* iarpData,NodeAddress destinationAddress) {
    int i = 0;
    IarpRoutingTable* iarpRoutingTable = &(iarpData->routeTable);
    IarpRoutingEntry* iarpRoutingTableEntry =
        iarpRoutingTable->routingTable;
    for (i = 0; i < (signed)iarpRoutingTable->numEntries; i++)
    {
        if (iarpRoutingTableEntry[i].destinationAddress ==
               destinationAddress)
        {
            MEM_free(iarpRoutingTableEntry[i].path);
            memcpy(&iarpRoutingTableEntry[i],
                &iarpRoutingTableEntry[iarpRoutingTable->numEntries - 1],
                sizeof(IarpRoutingEntry));
            memset(&iarpRoutingTableEntry[
                       iarpRoutingTable->numEntries - 1],
                   0, sizeof(IarpRoutingEntry));
            iarpRoutingTable->numEntries--;
            break;
        }
    }
}


// FUNCTION IarpReadZoneRadiusFromConfigurationFile
// PURPOSE  Reading the value of zone radius from the configuration file.
//          If no zone radius is given a default zone radius value is assumed
// Parameters:
//  node:   node which is reading the zone radius
//  nodeInput:  input configuration to the node
//  zoneRadiusRead: value of the zone radius read from the configuration file
static void IarpReadZoneRadiusFromConfigurationFile(Node* node,const NodeInput* nodeInput,unsigned* zoneRadiusRead) {
    if (nodeInput != NULL)
    {
        BOOL wasFound = FALSE;
        char valueRead[MAX_STRING_LENGTH] = {0};
        IO_ReadString(node->nodeId,
                      NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
                      nodeInput,
                      "ZONE-RADIUS",
                      &wasFound,
                      valueRead);
        if (wasFound == TRUE)
        {
            if (strcmp(valueRead, "INFINITY") == 0)
            {
                (*zoneRadiusRead) = UINT_MAX;
            }
            else
            {
                (*zoneRadiusRead) = atoi(valueRead);
            }
        }
    }
}


// FUNCTION IarpInitializePrivateDataStructures
// PURPOSE  Initializing IARP private data structure
// Parameters:
//  node:   node which is initializing IARP data structure
//  iarpData:   pointer Iarp data to be initialized
static void IarpInitializePrivateDataStructures(Node* node,IarpData* iarpData) {
    iarpData->zoneRadius = IARP_DEFAULT_ZONE_RADIUS;
    iarpData->iarpBroadcastTimer = IARP_DEFAULT_BROADCAST_TIMER;
    iarpData->iarpRefreashTimer = IARP_DEFAULT_ROUTE_REFRSH_TIMER;
    iarpData->linkStateLifeTime = IARP_LINK_STATE_TIMER;
    iarpData->numOfMyNeighbor = 0;
    iarpData->myLinkSourceAddress = NetworkIpGetInterfaceAddress(
                                        node,
                                        DEFAULT_INTERFACE);
    iarpData->linkStateSeqNum = (IARP_INITIAL_SEQ_NUM - 1);
    // set link state sequence table to NULL;
    iarpData->seqTable = NULL;
    // Initialize statistical options.
    iarpData->initStats = FALSE;
    iarpData->printStats = FALSE;
    iarpData->statsPrinted = FALSE;
}


// FUNCTION IarpProcessNdpOrIarpUpdatePacket
// PURPOSE  processing ndp or iarp update infromation
// Parameters:
//  node:   node which is processing the update packet
//  iarpData:   pointer to iarp data structure
//  iarpRoutingInfo:    iarp routing information extracted from ndp
//  iarpUpdatePacketType:   the update packet type is it NDP_PERIODIC_UPDATE
//  linkSource: link source address which is the source of
//  zoneRadius: the zone radius of the node
//  metric: iarp metric type
//  iarpLinkStateTableEntry:    pointer to iarp link state entry, that need to be updated
static void IarpProcessNdpOrIarpUpdatePacket(Node* node,IarpData* iarpData,IarpRoutingEntry* iarpRoutingInfo,IarpUpdatePacketType iarpUpdatePacketType,NodeAddress linkSource,unsigned int zoneRadius,IarpMetric metric,IarpLinkStateTableEntry* iarpLinkStateTableEntry) {
    BOOL routeTableUpdated = FALSE;
    IarpUpdateIarpRouteTable(
        node,
        iarpData,
        iarpRoutingInfo,
        iarpUpdatePacketType,
        linkSource,
        zoneRadius,
        metric,
        iarpLinkStateTableEntry,
        &routeTableUpdated);
    if (routeTableUpdated == TRUE)
    {
        if ((iarpUpdatePacketType == NDP_PERIODIC_UPDATE) &&
            (linkSource == iarpData->myLinkSourceAddress) &&
            (iarpRoutingInfo->nextHopAddress != (NodeAddress) NETWORK_UNREACHABLE))
        {
            // At this point ONLY INCREMENT IN NUMBER OF
            // NEIGHBOR is possible.
            if (IARP_DEBUG_NDP)
            {
                char destinationAddr[MAX_STRING_LENGTH] = {0};
                IO_ConvertIpAddressToString(
                    iarpRoutingInfo->destinationAddress,
                    destinationAddr);
                printf("Node %d Adding %s As NEIGHBOR LSTE %p\n",
                       node->nodeId,
                       destinationAddr,
                       iarpLinkStateTableEntry);
            }
            iarpData->numOfMyNeighbor++;
        }
    }
}


// FUNCTION IarpReadStatisticsOptionFromConfigurationFile
// PURPOSE  confirming that if user wants to print the statistics or not
// Parameters:
//  node:   Node which is reading the statistics
//  iarpData:   pointer to iarp data
//  nodeInput:  node input
static void IarpReadStatisticsOptionFromConfigurationFile(Node* node,IarpData* iarpData,const NodeInput* nodeInput) {
    if (nodeInput != NULL)
    {
        BOOL wasFound = FALSE;
        char valueRead[MAX_STRING_LENGTH] = {0};
        IO_ReadString(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "ROUTING-STATISTICS",
                      &wasFound,
                      valueRead);
        if ((wasFound) && (strcmp(valueRead, "YES") == 0))
        {
            iarpData->initStats = TRUE;
            iarpData->printStats = TRUE;
        }
    }
}


// FUNCTION IarpCheckForInZoneNode
// PURPOSE  check if  incoming routing information for the node,
//          is in the zone or not
// Parameters:
//  iarpData:   pointer to iarp data
//  hopCount:   nunber of "hops" that is separating THIS Node
static BOOL IarpCheckForInZoneNode(IarpData* iarpData,unsigned int hopCount) {
     return iarpData->zoneRadius >= hopCount;
}


// FUNCTION IarpUpdateIarpRouteTable
// PURPOSE  updating Iarp routing table
// Parameters:
//  node:   node which is updating the routing table
//  iarpData:   pointer to iarp data
//  iarpRoutingEntry:   pointher to an entry of type "IarpRoutingEntry"
//  iarpUpdatePacketType:   update packet type NDP_PERIODIC_UPDATE or
//  linkSource: Address of the link state update source
//  zoneRadius: zone radius of the link source
//  iarpMetric: IARP metric type currently only one type of
//  linkStateEntryPtr:  pointer to link state table entry
//  isRouteTableUpdated:    This function will set this value to
static void IarpUpdateIarpRouteTable(Node* node,IarpData* iarpData,IarpRoutingEntry* iarpRoutingEntry,IarpUpdatePacketType iarpUpdatePacketType,NodeAddress linkSource,unsigned int zoneRadius,IarpMetric iarpMetric,IarpLinkStateTableEntry* linkStateEntryPtr,BOOL* isRouteTableUpdated) {
    *isRouteTableUpdated = TRUE;
    if ((NetworkIpIsMyIP(node, linkSource) == TRUE) &&
        (iarpUpdatePacketType == IARP_PERIODIC_UPDATE))
    {
        *isRouteTableUpdated = FALSE;
        return;
    }
    // This means NDP has reported that suddenly a neighbor became
    // unreachable. So Delete neighbor and all the nodes
    // reachable via neighbor.
    if ((iarpUpdatePacketType == NDP_PERIODIC_UPDATE) &&
        (iarpRoutingEntry->nextHopAddress == (NodeAddress) NETWORK_UNREACHABLE))
    {
        IarpLinkStateInfo* linkStateinfo = NULL;
        IarpLinkStateTableEntry* linkStateEntryToDelete =
            IarpSearchLinkSourceInLinkStateTable(
                iarpData,
                iarpRoutingEntry->destinationAddress);
        if (linkStateEntryToDelete == NULL)
        {
            return;
        }
        if (linkStateEntryToDelete ==
            iarpData->iarpLinkStateTable.first)
        {
            iarpData->iarpLinkStateTable.first =
                linkStateEntryToDelete->next;
            linkStateEntryToDelete->next->prev = NULL;
        }
        else if (linkStateEntryToDelete ==
                 iarpData->iarpLinkStateTable.last)
        {
            iarpData->iarpLinkStateTable.last =
                linkStateEntryToDelete->prev;
            linkStateEntryToDelete->prev->next = NULL;
        }
        else
        {
            linkStateEntryToDelete->prev->next =
                linkStateEntryToDelete->next;
            linkStateEntryToDelete->next->prev =
                linkStateEntryToDelete->prev;
        }
        // Deleate all neighbor information attached
        // with "linkStateEntryToDelete".
        linkStateinfo = linkStateEntryToDelete->linkStateinfo;
        while (linkStateinfo)
        {
            IarpLinkStateInfo* linkStateInfoToDelete =
                linkStateEntryToDelete->linkStateinfo;
            linkStateEntryToDelete->linkStateinfo =
                linkStateInfoToDelete->next;
            if (linkStateEntryToDelete->linkStateinfo)
            {
                linkStateEntryToDelete->linkStateinfo->prev = NULL;
            }
            linkStateinfo = linkStateEntryToDelete->linkStateinfo;
            MEM_free(linkStateInfoToDelete);
        }
        MEM_free(linkStateEntryToDelete);
        *isRouteTableUpdated = TRUE;
        return;
    } // end if
    // Do not accept any IARP update before your neighbor
    // information build up
    if ((iarpUpdatePacketType == IARP_PERIODIC_UPDATE) &&
        (iarpData->numOfMyNeighbor == 0))
    {
        *isRouteTableUpdated = FALSE;
        return;
    }
    // Check if the link state destination is with in the Zone Radius
    // or not. Discard any link state if that is out of the Zone Radius.
    if (IarpCheckForInZoneNode(iarpData, iarpRoutingEntry->hopCount))
    {
        IarpLinkStateInfo* linkStateInfoPtr = NULL;
        IarpLinkStateInfo linkStateInfo = {
            iarpRoutingEntry->destinationAddress,
            iarpRoutingEntry->subnetMask,
            iarpMetric,
            node->getNodeTime(),
            NULL,
            NULL};
        linkStateInfoPtr = IarpAddLinkStateInfo(
                               linkStateEntryPtr,
                               &linkStateInfo);
        if (!(linkStateInfoPtr))
        {
            *isRouteTableUpdated = FALSE;
        }
        else
        {
            *isRouteTableUpdated = TRUE;
        }
    }
    else
    {
        // Routing table will not be updated, for out
        // of the zone routes
        *isRouteTableUpdated = FALSE;
    }
}


// FUNCTION IarpInitializeLinkStateTable
// PURPOSE  initializing iarp link state table
// Parameters:
//  iarpData:   pointer to iarp structure
static void IarpInitializeLinkStateTable(IarpData* iarpData) {
     memset(&(iarpData->iarpLinkStateTable), 0, sizeof(IarpLinkStateTable));
}


// FUNCTION IarpAddLinkSourceToLinkStateTable
// PURPOSE  Addding a link source to a link state table
// Parameters:
//  iarpData:   pointer to iarp data
//  linkSourceAddress:  the ip address of the source of the
//  zoneRadius: the zone radius of the source of the
//  linkStateId:    the link state id of the source of the
//  insertTime: the insert time
static IarpLinkStateTableEntry* IarpAddLinkSourceToLinkStateTable(IarpData* iarpData,NodeAddress linkSourceAddress,unsigned int zoneRadius,NodeAddress linkStateId,clocktype insertTime) {
    IarpLinkStateTableEntry* iarpLinkStateEntry = (IarpLinkStateTableEntry*)
        MEM_malloc(sizeof(IarpLinkStateTableEntry));
    // create link state table entry
    iarpLinkStateEntry->linkSource = linkSourceAddress;
    iarpLinkStateEntry->zoneRadius = zoneRadius;
    iarpLinkStateEntry->linkStateId = linkStateId;
    iarpLinkStateEntry->insertTime = insertTime;
    iarpLinkStateEntry->linkStateinfo = NULL;
    iarpLinkStateEntry->next = NULL;
    iarpLinkStateEntry->prev = NULL;
    if (iarpData->iarpLinkStateTable.first == NULL)
    {
        iarpData->iarpLinkStateTable.first = iarpLinkStateEntry;
        iarpData->iarpLinkStateTable.last = iarpLinkStateEntry;
    }
    else
    {
        iarpLinkStateEntry->prev = iarpData->iarpLinkStateTable.last;
        iarpData->iarpLinkStateTable.last->next = iarpLinkStateEntry;
        iarpData->iarpLinkStateTable.last = iarpLinkStateEntry;
    }
    return iarpLinkStateEntry;
}


// FUNCTION IarpSearchLinkSourceInLinkStateTable
// PURPOSE  Searching a link source address in the link state table.
//          And returning a pointer to the ilnk source entry if
//          exists or NULL otherwise
// Parameters:
//  iarpData:   pointer to iarp data
//  linkSourceAddress:  the ip address of the node
IarpLinkStateTableEntry* IarpSearchLinkSourceInLinkStateTable(IarpData* iarpData,NodeAddress linkSourceAddress) {
    IarpLinkStateTableEntry* iarpLinkStateTableEntry =
        iarpData->iarpLinkStateTable.first;
    while (iarpLinkStateTableEntry)
    {
        if (iarpLinkStateTableEntry->linkSource == linkSourceAddress)
        {
            return iarpLinkStateTableEntry;
        }
        iarpLinkStateTableEntry = iarpLinkStateTableEntry->next;
    }
    return NULL;
}


// FUNCTION IarpFindPlaceForLinkSourceEntry
// PURPOSE  Returning the pointer to the link source  entry in
//          the link state table.  If entry do not exists
//          previously , the  this function creates a new
//          entry and returns a pointer to that.
// Parameters:
//  iarpData:   pointer to iarp data
//  linkSource: link source address to find
//  zoneRadius: zone radius of the advertiser ( i.e the link source)
//  linkStateId:    the link state Id
//  insertTime: time when the entry is created
static IarpLinkStateTableEntry* IarpFindPlaceForLinkSourceEntry(IarpData* iarpData,NodeAddress linkSource,unsigned int zoneRadius,NodeAddress linkStateId,clocktype insertTime) {
    IarpLinkStateTableEntry* existingIarpLinkStateTableEntry =
        IarpSearchLinkSourceInLinkStateTable(iarpData,linkSource);
    if (existingIarpLinkStateTableEntry == NULL)
    {
        existingIarpLinkStateTableEntry =
            IarpAddLinkSourceToLinkStateTable(
                iarpData,
                linkSource,
                zoneRadius,
                linkStateId,
                insertTime);
    }
    else
    {
        // update only insert time.
        existingIarpLinkStateTableEntry->insertTime = insertTime;
    }
    return existingIarpLinkStateTableEntry;
}


// FUNCTION IarpAddLinkStateInfo
// PURPOSE  Adding "link state information" in the link state table
// Parameters:
//  iarpLinkStateTableEntry:    pointer to iarp link state table information
//  linkStateInfo:  the link state information that is to be insreted
static IarpLinkStateInfo* IarpAddLinkStateInfo(IarpLinkStateTableEntry* iarpLinkStateTableEntry,IarpLinkStateInfo* linkStateInfo) {
    IarpLinkStateInfo* infoInserted = NULL;
    if(iarpLinkStateTableEntry)
    {
        if (iarpLinkStateTableEntry->linkStateinfo == NULL)
        {
            iarpLinkStateTableEntry->linkStateinfo =
                (IarpLinkStateInfo*) MEM_malloc(sizeof(IarpLinkStateInfo));
            memcpy(iarpLinkStateTableEntry->linkStateinfo,
                   linkStateInfo,
                   sizeof(IarpLinkStateInfo));
            iarpLinkStateTableEntry->linkStateinfo->prev = NULL;
            iarpLinkStateTableEntry->linkStateinfo->next = NULL;
            return iarpLinkStateTableEntry->linkStateinfo;
        }
        else
        {
            IarpLinkStateInfo* tempLinkStateInfo =
                iarpLinkStateTableEntry->linkStateinfo;
            // Search the link state info list, to find if the in coming
            // link state info already exists or not.
            while(tempLinkStateInfo)
            {
                if (tempLinkStateInfo->destinationAddress ==
                    linkStateInfo->destinationAddress)
                {
                    tempLinkStateInfo->insertTime =
                        linkStateInfo->insertTime;
                    return NULL;
                }
                tempLinkStateInfo = tempLinkStateInfo->next;
            }
            // If the incoming link state information do not exists in the
            // link state info list, then add it at the begining of the list.
            infoInserted = (IarpLinkStateInfo*)
                MEM_malloc(sizeof(IarpLinkStateInfo));
            memcpy(infoInserted,
                   linkStateInfo,
                   sizeof(IarpLinkStateInfo));
            infoInserted->prev = NULL;
            infoInserted->next = iarpLinkStateTableEntry->linkStateinfo;
            iarpLinkStateTableEntry->linkStateinfo->prev = infoInserted;
            iarpLinkStateTableEntry->linkStateinfo = infoInserted;
            return infoInserted;
        }
    } // end if (IarpLinkStateTableEntry->linkStateinfo == NULL)
    return NULL;
}


// FUNCTION IarpPrintLinkStateTable
// PURPOSE  printing link state table entries.
// Parameters:
//  node:   node which will print statistics
//  iarpData:   pointer to iarp Data
static void IarpPrintLinkStateTable(Node* node,IarpData* iarpData) {
    char myLinkSourceAddress[MAX_STRING_LENGTH] = {0};
    IarpLinkStateTableEntry* iarpLinkStateTableEntry =
        iarpData->iarpLinkStateTable.first;
    IO_ConvertIpAddressToString(iarpData->myLinkSourceAddress,
         myLinkSourceAddress);
    printf("--------------------------------------------------------------"
           "--------------------------\n"
           "NODE %u has %u neighbors. (Link Source = %s)\n"
           "--------------------------------------------------------------"
           "--------------------------\n"
           "%16s %3s %12s %10s %16s %16s %6s\n"
           "--------------------------------------------------------------"
           "--------------------------\n",
           node->nodeId,
           iarpData->numOfMyNeighbor,
           myLinkSourceAddress,
           "Link-Source-Addr",
           "RAD",
           "LinkSourceId",
           "InsertTime",
           "Destination-Addr",
           "Subnet-Mask-Addr",
           "Metric");
    while (iarpLinkStateTableEntry)
    {
        BOOL isFirstInfoPrinted = FALSE;
        char linkSourceAddr[MAX_STRING_LENGTH] = {0};
        char insertTimeStr[MAX_STRING_LENGTH] = {0};
        IarpLinkStateInfo* linkStateInfo = iarpLinkStateTableEntry->linkStateinfo;
        IO_ConvertIpAddressToString(iarpLinkStateTableEntry->linkSource,
            linkSourceAddr);
        ctoa((iarpLinkStateTableEntry->insertTime / SECOND), insertTimeStr);
        printf("%16s %3u %12u %10s",
               linkSourceAddr,
               iarpLinkStateTableEntry->zoneRadius,
               iarpLinkStateTableEntry->linkStateId,
               insertTimeStr);
        while (linkStateInfo)
        {
            char destinationAddress[MAX_STRING_LENGTH] = {0};
            char subnetMaskStr[MAX_STRING_LENGTH] = {0};
            IO_ConvertIpAddressToString(linkStateInfo->destinationAddress,
                 destinationAddress);
            IO_ConvertIpAddressToString(linkStateInfo->subnetMask,
                 subnetMaskStr);
            if (isFirstInfoPrinted == FALSE)
            {
                printf(" %16s %16s %6u\n",
                       destinationAddress,
                       subnetMaskStr,
                       linkStateInfo->iarpMetric);
                isFirstInfoPrinted = TRUE;
            }
            else
            {
                printf("%61s %16s %6u\n",
                       destinationAddress,
                       subnetMaskStr,
                       linkStateInfo->iarpMetric);
            }
            linkStateInfo = linkStateInfo->next;
        } // end while (linkStateInfo)
        printf("\n");
        iarpLinkStateTableEntry = iarpLinkStateTableEntry->next;
    } // end while (iarpLinkStateTableEntry)
}


// FUNCTION IarpPrintRoutingTable
// PURPOSE  Printing Iarp routing table for debugging purpose
// Parameters:
//  node:   node which is printing routing table
//  iarpData:   ointer to iarp data structure
static void IarpPrintRoutingTable(Node* node,IarpData* iarpData) {
    int numEntriesInRoutingTable = iarpData->routeTable.numEntries;
    int i = 0;
    printf("----------------------------------------------------------\n"
           "Route Table %d\n"
           "----------------------------------------------------------\n"
           "%16s %16s %8s %16s %16s\n",
           node->nodeId,
           "Destination_Addr",
           "NextHop_Address",
           "HopCount",
           "Insert_Time",
           "Path_Addresses");
    for (i = 0; i < numEntriesInRoutingTable; i++)
    {
        unsigned int j = 0;
        char destination[MAX_STRING_LENGTH] = {0};
        char nextHopAddress[MAX_STRING_LENGTH] = {0};
        char insertTime[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(
             iarpData->routeTable.routingTable[i].destinationAddress,
             destination);
        IO_ConvertIpAddressToString(
             iarpData->routeTable.routingTable[i].nextHopAddress,
             nextHopAddress);
        ctoa(iarpData->routeTable.routingTable[i].lastHard, insertTime);
        printf("%16s %16s %u %20s    ", destination, nextHopAddress,
            iarpData->routeTable.routingTable[i].hopCount,
            insertTime);
        if (iarpData->routeTable.routingTable[i].path != NULL)
        {
            for (j = 0;
                 j < iarpData->routeTable.routingTable[i].hopCount;
                 j++)
            {
                printf("  %u,", iarpData->routeTable.routingTable[i]
                       .path[j]);
            }
        }
        printf("\n");
    }
}


// FUNCTION IarpBuildUpdatePacket
// PURPOSE  building update packet
// Parameters:
//  iarpData:   pointer to iarp data
//  packetBuffer:   pointer to packet buffer that will hold the packet
static void IarpBuildUpdatePacket(IarpData* iarpData,PacketBuffer* * packetBuffer) {
    char* dataPtr = NULL;
    char* headerPtr = NULL;
    *packetBuffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                        (IARP_NEIGHBOR_INFO_SIZE
                            * iarpData->numOfMyNeighbor),
                        IARP_HEADER_SIZE,
                        IARP_HEADER_SIZE,
                        FALSE,
                        &dataPtr,
                        &headerPtr);
    if ((headerPtr == NULL) || (dataPtr == NULL))
    {
        ERROR_Assert(FALSE, "Cannot allocate packet !!!");
        abort();
    }
    // Build IARP header.
    IarpBuildHeader(iarpData, headerPtr);
    IarpIncludeLinkStateInformationInPacket(iarpData, dataPtr);
}


// FUNCTION IarpAdvancePacketPtr
// PURPOSE  advancing the packet ptr by specified number of bytes
// Parameters:
//  packetPtr:  packet pointer to advance
//  numBytes:   number of bytes to advance
static void IarpAdvancePacketPtr(char* * packetPtr,int numBytes) {
     (*packetPtr) = (*packetPtr) + numBytes;
}


// FUNCTION IarpIncrementLinkStateSeqNum
// PURPOSE  Incrementing Iarp link state sequence number
// Parameters:
//  iarpData:   pointer to iarp protocol data
static unsigned short IarpIncrementLinkStateSeqNum(IarpData* iarpData) {
    if (iarpData->linkStateSeqNum == USHRT_MAX)
    {
        iarpData->linkStateSeqNum = IARP_INITIAL_SEQ_NUM;
    }
    else
    {
        iarpData->linkStateSeqNum++;
    }
    return iarpData->linkStateSeqNum;
}


// FUNCTION IarpBuildHeader
// PURPOSE  building iarp header
// Parameters:
//  iarpData:   pointer iarp data
//  headerPtr:  header pointer
static void IarpBuildHeader(IarpData* iarpData,char* headerPtr) {
    unsigned short seqNum = IarpIncrementLinkStateSeqNum(iarpData);
    unsigned char numDest = (unsigned char) iarpData->numOfMyNeighbor;
    char* initialPosition = headerPtr;
    unsigned char ttl = (unsigned char)iarpData->zoneRadius - 1;
    unsigned char zoneRadius = (unsigned char) iarpData->zoneRadius;
    // copy Link source address into the header.
    memcpy(headerPtr, (char*) &(iarpData->myLinkSourceAddress),
        sizeof(NodeAddress));
    IarpAdvancePacketPtr(&headerPtr, sizeof(NodeAddress));
    // copy link state seq number into the header.
    memcpy(headerPtr, (char*) &seqNum, sizeof(unsigned short));
    IarpAdvancePacketPtr(&headerPtr, sizeof(unsigned short));
    // copy zone radius into the header.
    memcpy(headerPtr, (char*) &zoneRadius, sizeof(unsigned char));
    IarpAdvancePacketPtr(&headerPtr, sizeof(unsigned char));
    // set the ttl field of the header.
    memcpy(headerPtr, (char*) &ttl, sizeof(unsigned char));
    IarpAdvancePacketPtr(&headerPtr, sizeof(unsigned char));
    // Skip 3-byte reserve field after initilizing them with 0
    memset(headerPtr, 0, (sizeof(unsigned char) * 3));
    IarpAdvancePacketPtr(&headerPtr, (sizeof(unsigned char) * 3));
    // Set the field "link Destination Count" in the header
    memcpy(headerPtr, &numDest, sizeof(unsigned char));
    IarpAdvancePacketPtr(&headerPtr, sizeof(unsigned char));
    if ((headerPtr - initialPosition) != IARP_HEADER_SIZE)
    {
        ERROR_Assert(FALSE, "Header size mismatch !!!");
        abort();
    }
}


// FUNCTION IarpIncludeLinkStateInformationInPacket
// PURPOSE  building up link state information
// Parameters:
//  iarpData:   pointer to iarp data
//  dataPacket: data packet to build up
static void IarpIncludeLinkStateInformationInPacket(IarpData* iarpData,char* dataPacket) {
    //int i = 0;
    IarpLinkStateInfo* iarpLinkStateInfo = NULL;
    IarpLinkStateTableEntry* iarpLinkStateTableEntry =
        iarpData->iarpLinkStateTable.first;
    ERROR_Assert(iarpLinkStateTableEntry, "No link state table"
        "entry exists. Error !!");
    while (iarpLinkStateTableEntry->linkSource !=
           iarpData->myLinkSourceAddress)
    {
        iarpLinkStateTableEntry = iarpLinkStateTableEntry->next;
        ERROR_Assert(iarpLinkStateTableEntry, "No link state table"
            "entry exists. Error !!");
    }
    // Program invariant: at this point "iarpLinkStateTableEntry"
    //                    points to node's own link state entry.
    iarpLinkStateInfo = iarpLinkStateTableEntry->linkStateinfo;
    while (iarpLinkStateInfo)
    {
        IarpNeighborInfo neighborInfo = {0};
        // Collect neighbor information
        neighborInfo.linkDestinationAddress = iarpLinkStateInfo->
            destinationAddress;
        neighborInfo.linkDestSubnetMask = iarpLinkStateInfo->
            subnetMask;
        neighborInfo.reserved = 0;
        neighborInfo.metricType =
            (unsigned char)iarpLinkStateInfo->iarpMetric;
        neighborInfo.metricValue = 1;
        // copy neighbor infor mation in ther packet
        memcpy(dataPacket, &neighborInfo, sizeof(IarpNeighborInfo));
        // Go and next neighbor information.
        iarpLinkStateInfo = iarpLinkStateInfo->next;
        IarpAdvancePacketPtr(&dataPacket, sizeof(IarpNeighborInfo));
    } // while (iarpLinkStateInfo)
}


// FUNCTION IarpEncapsulatePacketInTheMessage
// PURPOSE  ncapsulate the packet in the message
// Parameters:
//  node:   node which is encapsulating packet
//  msg:    message where packet is to be encapsulated
//  packetBuffer:   the buffer that contains the packet
static void IarpEncapsulatePacketInTheMessage(Node* node,Message* * msg,PacketBuffer* packetBuffer) {
    *msg = MESSAGE_Alloc(
               node,
               NETWORK_LAYER,
               ROUTING_PROTOCOL_IARP,
               MSG_DEFAULT);
    MESSAGE_PacketAlloc(node, *msg,
        BUFFER_GetCurrentSize(packetBuffer),
        TRACE_IARP);
    memcpy(MESSAGE_ReturnPacket((*msg)),
           BUFFER_GetData(packetBuffer),
           BUFFER_GetCurrentSize(packetBuffer));
}


// FUNCTION IarpReadHeaderInformation
// PURPOSE  Reading the header information of the routing update packet
// Parameters:
//  iarpHeader: value that is read from the header
//  packet: Packet that is received
static void IarpReadHeaderInformation(IarpGeneralHeader* iarpHeader,char* packet) {
    memcpy(iarpHeader, packet, IARP_HEADER_SIZE);
}


// FUNCTION IarpScheduleRefreshTimer
// PURPOSE  scheduling a route refresh timer
// Parameters:
//  node:   node which is scheduling the timer
//  iarpData:   pointer to iarp data
static void IarpScheduleRefreshTimer(Node* node,IarpData* iarpData) {
    Message* timer = MESSAGE_Alloc(
                         node,
                         NETWORK_LAYER,
                         ROUTING_PROTOCOL_IARP,
                         ROUTING_IarpRefraceTimeExpired);
    MESSAGE_Send(node, timer, iarpData->iarpRefreashTimer);
}


// FUNCTION IarpReBroadcastUpdatePacket
// PURPOSE  Rebroadcasting an update packet
// Parameters:
//  node:   node which is rebroadcasting the packet
//  iarpData:   pointer to iarp data
//  msg:    message to be rebroad casted
static void IarpReBroadcastUpdatePacket(Node* node,IarpData* iarpData,Message* msg) {

    // Just find and decrement the value of the ttl field in the header
    // and then rebroadcast the undistorted message.
    char* packet = MESSAGE_ReturnPacket(msg);
    unsigned int ipTTL = ((IarpGeneralHeader*) packet)->ttl;
    if (ipTTL > 0)
    {
        int i = 0;
        clocktype jitter = 0;
        ((IarpGeneralHeader*) packet)->ttl -=1;
        for (i = 0; i < node->numberInterfaces; i++)
        {
           if ((NetworkIpGetUnicastRoutingProtocolType(node, i)
                    != ROUTING_PROTOCOL_IARP && !ZrpIsEnabled(node)))
            {
                continue;
            }

            // Add Ip header and send packet to Mac layer
            NetworkIpSendRawMessageToMacLayerWithDelay(
                node,
                msg,
                NetworkIpGetInterfaceAddress(node, i),
                ANY_DEST,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_IARP,
                ipTTL,
                i,
                ANY_DEST,
                jitter);
        } // end for (i = 0; i < node->numberInterfaces; i++)
        // Statistical update. Update number of updates relayed
        // or rebooadcasted.
        (iarpData->stats.numUpdateRelayed)++;
    } // end if ((*packet) >= 0)
}


// FUNCTION IarpFindShortestPathBFS
// PURPOSE  Finding shortest path for each destination. Input to the algorithm is collected from link state table
// Parameters:
//  node:   node which is executing the shortest path algorithm
//  iarpData:   pointer to iarp data structure
static void IarpFindShortestPathBFS(Node* node,IarpData* iarpData) {
    //IarpRoutingEntry iarpRoutingEntry = {0};
    clocktype currentTime = node->getNodeTime();
    IarpLinkStateTableEntry* linkStateTableEntry = NULL;
    IarpLinkStateInfo* linkStateNeighbor = NULL;
    NodeAddress queue[MAX_NODE_ENTRY] = {0};
    NodeAddress source = 0;
    int current = 0;
    int last = 0;
    // Find the position of the source vertex in the link state table
    linkStateTableEntry = IarpSearchLinkSourceInLinkStateTable(
                              iarpData ,
                              iarpData->myLinkSourceAddress);
    if (linkStateTableEntry == NULL)
    {
        return;
    }
    // insert itself into queue
    queue[last] = linkStateTableEntry->linkSource;
    // insertElementInQueue(queue, linkStateTableEntry->linkSource, &last);
    // check untill queue not empty
    while (current <= last && linkStateTableEntry != NULL)
    {
        NodeAddress PrevNode = linkStateTableEntry->linkSource;
        linkStateNeighbor = linkStateTableEntry->linkStateinfo;
        // Enter all neighbors in queue
        while (linkStateNeighbor)
        {
            if (currentTime - linkStateNeighbor->insertTime <
                IARP_LINK_STATE_TIMER)
            {
                source = linkStateNeighbor->destinationAddress;
                if (!isSourcePresentInQueue(queue, source, last))
                {
                     IarpRoutingEntry iarpRoutingEntry = {0};
                     if (IarpSearchLinkSourceInLinkStateTable(
                            iarpData,
                            source))
                     {
                         insertElementInQueue(
                             queue,
                             source,
                             &last);
                     }
                     if (PrevNode == iarpData->myLinkSourceAddress)
                     {
                         iarpRoutingEntry.nextHopAddress = source;
                         iarpRoutingEntry.hopCount = 1;
                         iarpRoutingEntry.path =(NodeAddress*)
                             MEM_malloc(sizeof(NodeAddress));
                     }
                     else
                     {
                         NodeAddress *path = NULL;
                         IarpRoutingEntry *routingEntry =
                             IarpSearchRouteInRoutingTableEntry(
                                 iarpData,
                                 &iarpData->routeTable,
                                 PrevNode);
                         memcpy(&iarpRoutingEntry,
                                routingEntry,
                                sizeof(IarpRoutingEntry));
                         iarpRoutingEntry.hopCount += 1;
                         path = (NodeAddress*)
                             MEM_malloc(iarpRoutingEntry.hopCount
                                 * sizeof(NodeAddress));
                         memcpy(path, routingEntry->path,
                             (iarpRoutingEntry.hopCount-1)
                                 * sizeof(NodeAddress));
                         iarpRoutingEntry.path = path;
                    }
                    iarpRoutingEntry.destinationAddress = source;
                    iarpRoutingEntry.subnetMask =
                        linkStateNeighbor->subnetMask;
                    iarpRoutingEntry.outGoingInterface = DEFAULT_INTERFACE;
                    iarpRoutingEntry.lastHard =
                        linkStateNeighbor->insertTime;
                    iarpRoutingEntry.path[iarpRoutingEntry.hopCount - 1] =
                        source;
                    if(!IarpSearchRouteInRoutingTableEntry(
                            iarpData,
                            &iarpData->routeTable,
                            source))
                    {
                        IarpEnterRouteInRoutingTable(
                            node,
                            iarpData,
                            &iarpData->routeTable,
                            &iarpRoutingEntry);
                    }
                    else
                    {
                        MEM_free(iarpRoutingEntry.path);
                    }
                }  // if(!isSourcePresentInQueue(queue, source, last))
            }  // if (bad data ?)
            linkStateNeighbor = linkStateNeighbor->next;
        } // while (linkStateNeighbor)
        current++;
        if (current <= last)
        {
            linkStateTableEntry = IarpSearchLinkSourceInLinkStateTable(
                                      iarpData,
                                      queue[current]);
            if(!linkStateTableEntry)
            {
                return;
            }
        }
    } // end while (current =< last)
}


// FUNCTION isSourcePresentInQueue
// PURPOSE  adding a node address into a BFS queue. This function is used calculating shortest path
// Parameters:
//  queue:  pointer to queue
//  source: source node address
//  last:
BOOL isSourcePresentInQueue(NodeAddress* queue,NodeAddress source,int last) {
    int i = 0;
    for (i = 0; i <= last; i++)
    {
        if (queue[i] == source)
        {
            return TRUE;
        }
    }
    return FALSE;
}


// FUNCTION insertElementInQueue
// PURPOSE  Inserting an element in the queue. An "Element" hehe is node's IP address.
// Parameters:
//  queue:  the queue of node addresses
//  source: the IP address of source node
//  last:   last position (last index) in the queue
void insertElementInQueue(NodeAddress* queue,NodeAddress source,int* last) {
    *last += 1;
    queue[*last] = source;
}


// FUNCTION IarpFlushRoutingTable
// PURPOSE  Flushing out Routing Table
// Parameters:
//  node:   node which is running the Dijkstra algorithm
//  iarpData:   pointer to iarp data
static void IarpFlushRoutingTable(Node* node,IarpData* iarpData) {
    int i = 0;
    IarpRoutingEntry* routingTable = iarpData->routeTable.routingTable;
    unsigned int numEntries = iarpData->routeTable.numEntries;
    for (i = 0; i < (signed)numEntries; i++)
    {
        if (routingTable[i].path != NULL)
        {
            MEM_free(routingTable[i].path);
            routingTable[i].path = NULL;
        }
    }
    memset(routingTable, 0, (numEntries * sizeof(IarpRoutingEntry)));
    iarpData->routeTable.numEntries = 0;
}


// FUNCTION IarpFindNextHop
// PURPOSE  Finding a next hop for given destination
// Parameters:
//  iarpData:   pointer to iarp data
//  destAddress:    destination address for which next hop to be found
NodeAddress IarpFindNextHop(IarpData* iarpData,NodeAddress destAddress) {
    IarpRoutingEntry* routingTable = iarpData->routeTable.routingTable;
    unsigned int numEntries = iarpData->routeTable.numEntries;
    while (numEntries > 0 )
    {
        numEntries--;
        if (routingTable[numEntries].destinationAddress == destAddress)
        {
            return  routingTable[numEntries].nextHopAddress;
        }
    }
    return (NodeAddress) NETWORK_UNREACHABLE;
}


// FUNCTION SearchLinkSourceInSeqTable
// PURPOSE  Searching information about a link source address in the link state sequence number table.
// Parameters:
//  root:   linkSource address whos information is to be
//  linkSource: linkSource address whos information is to be
static SeqTable* SearchLinkSourceInSeqTable(SeqTable* root,NodeAddress linkSource) {
    while (( root != NULL) && (root->linkSource != linkSource))
    {
        if (root->linkSource > linkSource)
        {
            root = root->leftChild;
        }
        else if (root->linkSource < linkSource)
        {
            root = root->rightChild;
        }
    }
    return root;
}


// FUNCTION InsertIntoSeqTable
// PURPOSE  Inserting information in sequence table.
// Parameters:
//  root:   pointer to the root of the sequence table
//  linkSource: link source address.
//  seqNum: last seqnumber seen.
static SeqTable* InsertIntoSeqTable(SeqTable* root,NodeAddress linkSource,unsigned short seqNum) {
    if (root == NULL)
    {
        SeqTable* seqTable = (SeqTable*)
            MEM_malloc(sizeof(SeqTable));
        seqTable->linkSource = linkSource;
        seqTable->seqNum = seqNum;
        seqTable->leftChild = NULL;
        seqTable->rightChild = NULL;
        return seqTable;
    }
    else if (root->linkSource > linkSource)
    {
        // insert the information in left
        root->leftChild = InsertIntoSeqTable(
                              root->leftChild,
                              linkSource,
                              seqNum);
    }
    else if (root->linkSource < linkSource)
    {
        // insert the information in right
        root->rightChild = InsertIntoSeqTable(
                               root->rightChild,
                               linkSource,
                               seqNum);
    }
    else // if (root->linkSource == linkSource)
    {
        // entry exists in the table update
        // seq number...
        root->seqNum = seqNum;
    }
    return root;
}


// FUNCTION IarpRegisterZrp
// PURPOSE  Registering Zrp with iarp
// Parameters:
//  node:   node which is initializing
//  dataPtr:    pointer to the iarp data structure
//  nodeInput:  pointer to node input structure
//  interfaceIndex: interface index which is initializing
//  iarpRouterFunc: pointer to iarp router function
void IarpRegisterZrp(Node* node,IarpData* * dataPtr,const NodeInput* nodeInput,int interfaceIndex,RouterFunctionType* iarpRouterFunc) {
    // Call Initialize Iarp
    IarpInit(node, dataPtr, nodeInput, interfaceIndex);
    *iarpRouterFunc = IarpRouterFunction;
}


// FUNCTION IarpSetIerpUpdeteFuncPtr
// PURPOSE  Accept pointer to "IERP routing table update function" and storing ththe pointer in it's
//          internal data structure. This will be used to update IERP routing table from IARP
// Parameters:
//  node:   node which is performing above operation.
//  updateFunc: pointer to IERP routing table update function
void IarpSetIerpUpdeteFuncPtr(Node* node,IerpUpdateFunc updateFunc) {
     IarpData *dataPtr = (IarpData*) NetworkIpGetRoutingProtocol(
                                         node,
                                         ROUTING_PROTOCOL_IARP);
     // If this is NULL check if ZRP is running. Because the entire
     // ierp protocol data is within ZRP.
     if (dataPtr == NULL)
     {
         //
         // UNUSUAL SITUATION: IARP data pointer should not be NULL
         //
         ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
         return;
     }
     dataPtr->ierpUpdateFunc = updateFunc;
     assert(dataPtr->ierpUpdateFunc);
}


// FUNCTION IarpGetRoutingProtocol
// PURPOSE  Get routing protocol structure associated with routing protocol
// Parameters:
//  node:   Pointer to node
//  routingProtocolType:    Routing protocol to retrieve
void* IarpGetRoutingProtocol(Node* node,NetworkRoutingProtocolType routingProtocolType) {
    void* protocolData = NULL;
    int i = 0;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    // Protocol Data must be IERP
    ERROR_Assert(routingProtocolType == ROUTING_PROTOCOL_IARP,
                 "Protocol Must be IARP");
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, i)
            == routingProtocolType)
        {
            protocolData = ip->interfaceInfo[i]->routingProtocol;
        }
    }
    if (protocolData == NULL)
    {
        // Protocol data not found check iz ZRP is running.
        protocolData = (IarpData*) ZrpReturnProtocolData(
                                      node,
                                      ROUTING_PROTOCOL_IARP);
    }
    ERROR_Assert(protocolData, "Protocol Data Must not be NULL");
    return protocolData;
}



//UserCodeEnd

//State Function Definitions
void IarpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                              NodeAddress destAddr, int interfaceIndex, unsigned ttl) {
        //UserCodeStartProcessReceivedPacket
    // Iarp process update packet. It is the  only packet that
    // IARP receives form IP so go for direct processing no
    // packet type checking is needed.
    int i = 0;
    NodeAddress incomingLinkSource = ANY_DEST;
    NodeAddress linkStateId = ANY_DEST;
    unsigned int zoneRadius = UCHAR_MAX;
    unsigned int IpTTL = UCHAR_MAX;
    unsigned int linkDestCnt = UCHAR_MAX;

    SeqTable* seqTable = NULL;
    IarpLinkStateTableEntry* iarpLinkStateTableEntry = NULL;
    char* iarpUpdatePacket = MESSAGE_ReturnPacket(msg);
    IarpGeneralHeader iarpHeader = {0};
    Message* newMessage =  NULL;
    //int flag = 0;

    // Extract IARP protocol data from node structure.
    IarpData* dataPtr = (IarpData*) NetworkIpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IARP);

    // If this is NULL check if ZRP is running. Because the entire
    // ierp protocol data is within ZRP.
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
        return;
    }

    IarpReadHeaderInformation(&iarpHeader, iarpUpdatePacket);
    IarpAdvancePacketPtr(&iarpUpdatePacket,IARP_HEADER_SIZE);

    // Extract various fields from IARP Header
    incomingLinkSource = iarpHeader.linkSourceAddress;
    zoneRadius = iarpHeader.zoneRadius;
    IpTTL = iarpHeader.ttl;
    linkDestCnt = iarpHeader.linkDestCnt;
    linkStateId = incomingLinkSource; // FOR WIRED NEED CHANGE

    if (incomingLinkSource == dataPtr->myLinkSourceAddress)
    {
        dataPtr->state = IARP_IDLE;
        MESSAGE_Free(node, msg);
        return;
    }

    if (IARP_DEBUG_UPDATE)
    {
        char linkSource[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(incomingLinkSource, linkSource);

        printf("Node %d received a message from %s \n"
               "Zone radius = %u IpTTL = %u link dest cnt = %u"
               "link state id = %u  Seq Num = %d\n",
               node->nodeId,
               linkSource,
               zoneRadius,
               IpTTL,
               linkDestCnt,
               linkStateId,
               iarpHeader.linkStateSeqNum);
    }

    if (zoneRadius <= 1)
    {
        (dataPtr->stats.numUpdateReceived)++;
        dataPtr->state = IARP_IDLE;
        MESSAGE_Free(node, msg);
        return;
    }

    // Search for a entry inthe sequence table for a sequence
    // number for the incoming link source.
    seqTable = SearchLinkSourceInSeqTable(
                   dataPtr->seqTable,
                   incomingLinkSource);

    if (seqTable == NULL)
    {
        dataPtr->seqTable = InsertIntoSeqTable(
                                dataPtr->seqTable,
                                incomingLinkSource,
                                iarpHeader.linkStateSeqNum);

        ERROR_Assert(dataPtr->seqTable, "Nothing ia added in the"
            "Sequence table");
    }
    else if ((seqTable->seqNum == IARP_INITIAL_SEQ_NUM) ||
             (seqTable->seqNum == USHRT_MAX) ||
             (seqTable->seqNum < iarpHeader.linkStateSeqNum))
    {
        seqTable->seqNum = iarpHeader.linkStateSeqNum;
    }
    else
    {
        // Packet with a duplicate sequence number is
        // seen. Do not process this packet.
        dataPtr->state = IARP_IDLE;
        MESSAGE_Free(node, msg);
        return;
    }

    // Find the link source entry in the link state table for
    // link source == incoming link source.
    iarpLinkStateTableEntry = IarpFindPlaceForLinkSourceEntry(
                                  dataPtr,
                                  incomingLinkSource,
                                  zoneRadius,
                                  linkStateId,
                                  node->getNodeTime());

    for (i = 0; i < (signed)linkDestCnt; i++)
    {
        IarpRoutingEntry routingEntry = {0};
        IarpNeighborInfo neighborInfo = {0};
        //NodeAddress* pathList = NULL;

        // Read a neighbor info from the update packet
        memcpy(&neighborInfo, iarpUpdatePacket, sizeof(IarpNeighborInfo));
        IarpAdvancePacketPtr(&iarpUpdatePacket, sizeof(IarpNeighborInfo));

        // Build IARP routing Entry
        routingEntry.destinationAddress =
            neighborInfo.linkDestinationAddress;

        routingEntry.nextHopAddress = linkStateId; // MAY NEED CHANGE
        routingEntry.subnetMask = neighborInfo.linkDestSubnetMask;
        routingEntry.hopCount = (zoneRadius - IpTTL);
        routingEntry.outGoingInterface = DEFAULT_INTERFACE; // MAY NEED CHANGE
        routingEntry.lastHard = node->getNodeTime();
        routingEntry.path = NULL;

        if (IARP_DEBUG_UPDATE)
        {
            char dests[MAX_STRING_LENGTH] = {0};
            char subnetMsk[MAX_STRING_LENGTH] = {0};

            IO_ConvertIpAddressToString(
                neighborInfo.linkDestinationAddress,

                dests);

            IO_ConvertIpAddressToString(
                neighborInfo.linkDestSubnetMask,
                subnetMsk);

            printf("\t\t##dest address = %s\n\t\t##subnet mask = %s\n"
                   "\t\t##metric value = %u\n"
                   "\t\t##metric type  = %u\n"
                   "\t\t##---------------------------------\n",
                   dests,
                   subnetMsk,
                   neighborInfo.metricValue,
                   neighborInfo.metricType);
        }

        IarpProcessNdpOrIarpUpdatePacket(
            node,
            dataPtr,
            &routingEntry,
            IARP_PERIODIC_UPDATE,
            incomingLinkSource,
            zoneRadius,
            IARP_METRIC_HOP_COUNT,
            iarpLinkStateTableEntry);
    } // end for (i = 0; i < linkDestCnt; i++)

    // Flash out IARP routing table
    IarpFlushRoutingTable(node, dataPtr);

    // Calculate shortest path and update IARP route table
    IarpFindShortestPathBFS(node, dataPtr);

    // Update Ierp Routing Table
    IarpUpdateIerpRoutingTable(node, dataPtr);

    if (IARP_DEBUG_FORWARD)
    {
        #define IP_ADDR_LSU_SOURCE 1
        if (incomingLinkSource == IP_ADDR_LSU_SOURCE)
        {
            FILE* fd = NULL;
            static int i = 0;

            if(i == 0)
            {
                i++;
                fd = fopen("lsuForward.out","w");
            }
            else
            {
                fd = fopen("lsuForward.out","a");
            }
            // fprintf(fd,"\n Node %u receive LSU of node %u",
            //        node->nodeId, incomingLinkSource);

            fprintf(fd, "\n Node %u receive LSU of node %u having IpTTL = %u",
                    node->nodeId,
                    incomingLinkSource, IpTTL);

            fclose(fd);
        }
        #undef IP_ADDR_LSU_SOURCE
    }

    if (IpTTL > 1)
    {
        // If TTL is greater than zero then only rebroadcast
        newMessage =  MESSAGE_Duplicate(node, msg);
        IarpReBroadcastUpdatePacket(node, dataPtr, newMessage);
    }

    // Statiscal update. Increment the number of update
    // packet received
    (dataPtr->stats.numUpdateReceived)++;

    //UserCodeEndProcessReceivedPacket

    dataPtr->state = IARP_IDLE;
    enterIarpIdle(node, dataPtr, msg);
}


void IarpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                        NodeAddress previousHopAddr, BOOL* packetWasRouted) {
        //UserCodeStartRoutePacket
    NodeAddress nextHop = ANY_DEST;

    // Extract IARP protocol data from node structure.
    IarpData* dataPtr = (IarpData*) NetworkIpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IARP);

    // If this is NULL check if ZRP is running. Because the entire
    // ierp protocol data is within ZRP.
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
        return;
    }

    nextHop = IarpFindNextHop(dataPtr, destAddr);

    if (IARP_DEBUG_SHORTEST_PATH)
    {
        #define IP_PROTOCOL_TYPE      17 // for UDP application

        IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;
        int protocol = ipHeader->ip_p;
        FILE *fd = NULL;
        static int i = 0;

        if (i == 0)
        {
            i++;
            fd = fopen("shortestPath.out","w");
            fprintf(fd, "%s","Routing Path is :\n");
        }
        else
        {
            fd = fopen("shortestPath.out","a");
        }

        if (protocol == IP_PROTOCOL_TYPE)
        {
            if (node->nodeId == destAddr)
            {
                fprintf(fd, "%d \n",node->nodeId);
            }
            else
            {
                fprintf(fd, "%d ->",node->nodeId);
            }
        }
        fclose(fd);
        #undef IP_PROTOCOL_TYPE
    }

    if ((nextHop == (NodeAddress) NETWORK_UNREACHABLE) ||
        /*Commented During ARP task, to remove previous hop dependency*/
        /*(nextHop == previousHopAddr)     ||*/
        (destAddr == ANY_DEST)           ||
        (NetworkIpIsMyIP(node, destAddr) == TRUE))
    {
        *packetWasRouted = FALSE;
        return;
    }

    NetworkIpSendPacketToMacLayer(
        node,
        msg,
        DEFAULT_INTERFACE,
        nextHop);

    *packetWasRouted = TRUE;

    // Statiscal update. Number of packets routed.
    (dataPtr->stats.numPacketRouted)++;

    // Set the message pointer to NULL. This will not effect
    // the original pointer passed to this function, but it will prevent
    // the  message from being freed at the idle state.
    msg = NULL;

    //UserCodeEndRoutePacket

    dataPtr->state = IARP_IDLE;
    enterIarpIdle(node, dataPtr, msg);
}


static void enterIarpIdle(Node* node,
                          IarpData* dataPtr,
                          Message *msg) {

    //UserCodeStartIdle
    if (msg != NULL)
    {
        MESSAGE_Free(node, msg);
    }

    //UserCodeEndIdle


}

static void enterIarpProcessAgedOutRoutes(Node* node,
                                          IarpData* dataPtr,
                                          Message *msg) {

    //UserCodeStartProcessAgedOutRoutes
    // This function deleates all the "aged out routes." All
    // those routes are deleted that has not been updated for
    // the duration "IARP_LINK_STATE_TIMER".
    IarpLinkStateTableEntry* linkStateTableEntry = NULL;

    // If this is NULL check if ZRP is running. Because the entire
    // ierp protocol data is within ZRP.
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
        return;
    }

    linkStateTableEntry = dataPtr->iarpLinkStateTable.first;

    while (linkStateTableEntry)
    {
        clocktype currentTime = node->getNodeTime();

        IarpLinkStateInfo* iarpLinkStateInfo =
            linkStateTableEntry->linkStateinfo;

        NodeAddress linkSourceAdderss = linkStateTableEntry->linkSource;
        IarpLinkStateInfo** head = &(linkStateTableEntry->linkStateinfo);

        while (iarpLinkStateInfo)
        {
            IarpLinkStateInfo* linkStateInfoToDelele = NULL;

            if ((currentTime - iarpLinkStateInfo->insertTime) >
                 IARP_LINK_STATE_TIMER)
            {
                linkStateInfoToDelele = iarpLinkStateInfo;

                if ((*head) == linkStateInfoToDelele)
                {
                    (*head) = linkStateInfoToDelele->next;

                    if ((*head) != NULL)
                    {
                        (*head)->prev = NULL;
                    }
                }
                else
                {
                    if (linkStateInfoToDelele->prev)
                    {
                        linkStateInfoToDelele->prev->next =
                            linkStateInfoToDelele->next;
                    }
                    else
                    {
                        ERROR_Assert(FALSE, "Here the prev pointer"
                            "Should not be NULL !!");
                    }

                    if (linkStateInfoToDelele->next != NULL)
                    {
                        linkStateInfoToDelele->next->prev =
                            linkStateInfoToDelele->prev;
                    }
                }
                iarpLinkStateInfo = linkStateInfoToDelele->next;

                if (linkSourceAdderss == dataPtr->myLinkSourceAddress)
                {
                    char destinationAddr[MAX_STRING_LENGTH] = {0};

                    IO_ConvertIpAddressToString(
                        linkStateInfoToDelele->destinationAddress,
                        destinationAddr);

                    (dataPtr->numOfMyNeighbor)--;

                    ERROR_Assert(dataPtr->numOfMyNeighbor >= 0,
                                 "Number of my neighbor become nagative !!");
                }
                MEM_free(linkStateInfoToDelele);
            }
            else
            {
                iarpLinkStateInfo = iarpLinkStateInfo->next;
            }
        } // end while (iarpLinkStateInfo)

        if (currentTime - linkStateTableEntry->insertTime >
              IARP_LINK_STATE_TIMER)
        {
            IarpLinkStateTableEntry* linkStateEntryToDelete =
                linkStateTableEntry;

            if (dataPtr->iarpLinkStateTable.first == linkStateEntryToDelete)
            {
                dataPtr->iarpLinkStateTable.first =
                    linkStateEntryToDelete->next;

                if (dataPtr->iarpLinkStateTable.first)
                {
                    dataPtr->iarpLinkStateTable.first->prev = NULL;
                }
            }
            else
            {
                if (linkStateEntryToDelete->prev)
                {
                    linkStateEntryToDelete->prev->next =
                        linkStateEntryToDelete->next;
                }
                else
                {
                    ERROR_Assert(FALSE, "Here the prev pointer"
                        "Should not be NULL !!");
                }

                if (linkStateEntryToDelete->next)
                {
                    linkStateEntryToDelete->next->prev =
                        linkStateEntryToDelete->prev;
                }
            }

            if (dataPtr->iarpLinkStateTable.last ==
                linkStateEntryToDelete)
            {
                dataPtr->iarpLinkStateTable.last =
                    linkStateEntryToDelete->prev;
            }

            ERROR_Assert(linkStateEntryToDelete->linkStateinfo == NULL,
                "There is some link state information\n And link source"
                "is deleted\n");

            linkStateTableEntry = linkStateEntryToDelete->next;
            MEM_free(linkStateEntryToDelete);
        }
        else
        {
            linkStateTableEntry = linkStateTableEntry->next;
        }
    } // while (linkStateTableEntry)

    // Schedule a route refrese timer
    IarpScheduleRefreshTimer(node, dataPtr);

    //UserCodeEndProcessAgedOutRoutes

    dataPtr->state = IARP_IDLE;
    enterIarpIdle(node, dataPtr, msg);
}

static void enterIarpProcessBroadcastTimerExpired(Node* node,
                                                  IarpData* dataPtr,
                                                  Message *msg) {

    //UserCodeStartProcessBroadcastTimerExpired
    // If this is NULL check if ZRP is running. Because the entire
    // ierp protocol data is within ZRP.
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
        return;
    }

    if (dataPtr->zoneRadius > 0)
    {
        if (dataPtr->numOfMyNeighbor > 0)
        {
            unsigned int i = 0;
            PacketBuffer* packetBuffer = NULL;
            clocktype jitter = RANDOM_nrand(dataPtr->seed) % (10 * MILLI_SECOND);

            // Build a routing update packet.
            IarpBuildUpdatePacket(dataPtr, &packetBuffer);

            for (i = 0; i < (unsigned)node->numberInterfaces; i++)
            {
                Message* msgNew = NULL;

                if ((NetworkIpGetUnicastRoutingProtocolType(node, i)
                    != ROUTING_PROTOCOL_IARP && !ZrpIsEnabled(node)))
                {
                    continue;
                }


                // Encapsulate the packet in the message
                IarpEncapsulatePacketInTheMessage(
                    node,
                    &msgNew,
                    packetBuffer);

                if (IARP_DEBUG_PERIODIC)
                {
                    char time[MAX_STRING_LENGTH] = {0};
                    static int i = 0;
                    FILE* fd = NULL;

                    if (i == 0)
                    {
                        i++;
                        fd = fopen("lsuPeriodic.out","w");
                    }
                    else
                    {
                        fd = fopen("lsuPeriodic.out","a");
                    }

                    TIME_PrintClockInSecond(node->getNodeTime(), time);

                    fprintf(fd, "\n Node %u sends LSU at time %s",
                            node->nodeId,
                            time);

                    fclose(fd);
                }

                // Add Ip header and send the message.
                NetworkIpSendRawMessageToMacLayerWithDelay(
                    node,
                    msgNew,
                    NetworkIpGetInterfaceAddress(node, i),
                    ANY_DEST,
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_IARP,
                    dataPtr->zoneRadius,
                    i,
                    ANY_DEST,
                    jitter);

                // Statistical update. Update the number of Regular
                // update message sent.
                (dataPtr->stats.numRegularUpdate)++;
            } // end for (i = 0; i < node->numberInterfaces; i++)

            // Clear packet buffer data if not null
            if (packetBuffer != NULL)
            {
                BUFFER_FreePacketBuffer(packetBuffer);
                //BUFFER_ClearPacketBufferData(packetBuffer);
            }
        } // end if (dataPtr->numOfMyNeighbor)
    } // if ((dataPtr->zoneRadius - 1) >= 0)

    // Re-schedule the broadcast timer again
    IarpScheduleBroadcastTimer(
        node,
        dataPtr,
        dataPtr->iarpBroadcastTimer);

    //UserCodeEndProcessBroadcastTimerExpired

    dataPtr->state = IARP_IDLE;
    enterIarpIdle(node, dataPtr, msg);
}


//Statistic Function Definitions
static void IarpInitStats(Node* node, IarpStatsType *stats) {
    if (node->guiOption) {
        stats->numTriggeredUpdateId = GUI_DefineMetric("IARP: Number of Triggered Update Messages Sent", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numRegularUpdateId = GUI_DefineMetric("IARP: Number of Regular Update Messages Sent", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numUpdateRelayedId = GUI_DefineMetric("IARP: Number of Update Messages Relayed", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numUpdateReceivedId = GUI_DefineMetric("IARP: Number of Update Messages Received", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numPacketRoutedId = GUI_DefineMetric("IARP: Number of Packets Routed", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
    }

    stats->numTriggeredUpdate = 0;
    stats->numRegularUpdate = 0;
    stats->numUpdateRelayed = 0;
    stats->numUpdateReceived = 0;
    stats->numPacketRouted = 0;
}

void IarpPrintStats(Node* node) {
    IarpStatsType *stats = &((IarpData *) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IARP))->stats;
    char buf[MAX_STRING_LENGTH];

    char statProtocolName[MAX_STRING_LENGTH];
    strcpy(statProtocolName,"IARP");
    sprintf(buf, "Number of triggered updates sent = %u", stats->numTriggeredUpdate);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of regular updates sent = %u", stats->numRegularUpdate);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of update messages relayed = %u", stats->numUpdateRelayed);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of update messages received = %u", stats->numUpdateReceived);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of packets routed = %u", stats->numPacketRouted);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
}

void IarpRunTimeStat(Node* node, IarpData* dataPtr) {
    clocktype now = node->getNodeTime();

    if (node->guiOption) {
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numTriggeredUpdateId, dataPtr->stats.numTriggeredUpdate, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numRegularUpdateId, dataPtr->stats.numRegularUpdate, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numUpdateRelayedId, dataPtr->stats.numUpdateRelayed, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numUpdateReceivedId, dataPtr->stats.numUpdateReceived, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numPacketRoutedId, dataPtr->stats.numPacketRouted, now);
    }
}

void
IarpInit(Node* node, IarpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex) {

    Message *msg = NULL;

    //UserCodeStartInit
    BOOL wasFound = FALSE;
    char valueRead[MAX_STRING_LENGTH];
    (*dataPtr) = (IarpData*) MEM_malloc(sizeof(IarpData));
    memset((*dataPtr), 0, sizeof(IarpData));

    // IARP initialize private data structure.
    IarpInitializePrivateDataStructures(node, *dataPtr);

    RANDOM_SetSeed((*dataPtr)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_IARP,
                   interfaceIndex);

    // Read the value of zone radius from the configuration file
    IarpReadZoneRadiusFromConfigurationFile(
        node,
        nodeInput,
        &((*dataPtr)->zoneRadius));

    IO_ReadString(
        node->nodeId,
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
        nodeInput,
        "IERP-USE-BRP",
        &wasFound,
        valueRead);

    if (wasFound && !strcmp(valueRead, "YES"))
    {
        (*dataPtr)->zoneRadius = 2 * (*dataPtr)->zoneRadius - 1;
    }

    // Initialize IARP Routing Table
    IarpInitializeRoutingTable(&((*dataPtr)->routeTable));

    // Initialize topology table
    IarpInitializeLinkStateTable(*dataPtr);

    // Read from configuration file if user wants to get the stats
    IarpReadStatisticsOptionFromConfigurationFile(
        node,
        *dataPtr,
        nodeInput);

    // By the function call below IARP register itself to
    // use the service of NDP
    NdpSetUpdateFunction(
        node,
        nodeInput,
        IarpReceiveNdpUpdate);

    // Schedule a broadcast timer.
    IarpScheduleBroadcastTimer(node, (*dataPtr), IARP_STARTUP_DELAY);

    // Schedule a route refresh timer
    IarpScheduleRefreshTimer(node, (*dataPtr));

    if (!ZrpIsEnabled(node))
    {
        // Set router function.
        NetworkIpSetRouterFunction(
            node,
            &IarpRouterFunction,
            interfaceIndex);
    }

    // Set IerpUpdate function pointer to NULL
    (*dataPtr)->ierpUpdateFunc = NULL;

    // ZRP currently will handle wireless interface only. So if
    // wired interface is given abort execution forcefully
    if (MAC_IsWiredNetwork(node, interfaceIndex))
    {
        // Report error and abort
        ERROR_ReportError("ZRP can handle wireless interface only");
        abort();
    }

    //UserCodeEndInit
    if ((*dataPtr)->initStats) {
        IarpInitStats(node, &((*dataPtr)->stats));
    }
    (*dataPtr)->state = IARP_IDLE;
    enterIarpIdle(node, *dataPtr, msg);
}


void
IarpFinalize(Node* node) {

    IarpData *dataPtr = (IarpData*) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IARP);

    //UserCodeStartFinalize
    // If this is NULL check if ZRP is running. Because the entire
    // iarp protocol data is within ZRP.
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IARP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IARP dataPtr should not be NULL !!!");
        return;
    }

    if (IARP_DEBUG_TABLE)
    {
        // This will be printed for the purpose of debugging.
        IarpPrintLinkStateTable(node, dataPtr);
        IarpPrintRoutingTable(node, dataPtr);
    }

    //UserCodeEndFinalize

    if (dataPtr->statsPrinted) {
        return;
    }

    if (dataPtr->printStats) {
        dataPtr->statsPrinted = TRUE;
        IarpPrintStats(node);
    }
}


void
IarpProcessEvent(Node* node,   Message* msg) {

    IarpData* dataPtr = (IarpData*)  NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IARP);

    int event = msg->eventType;
    switch (dataPtr->state) {
        case IARP_IDLE:
            switch (event) {
                case ROUTING_IarpBroadcastTimeExpired:
                    dataPtr->state = IARP_PROCESS_BROADCAST_TIMER_EXPIRED;
                    enterIarpProcessBroadcastTimerExpired(node, dataPtr, msg);
                    break;
                case ROUTING_IarpRefraceTimeExpired:
                    dataPtr->state = IARP_PROCESS_AGED_OUT_ROUTES;
                    enterIarpProcessAgedOutRoutes(node, dataPtr, msg);
                    break;
                default:
                    assert(FALSE);
            }
            break;
        case IARP_PROCESS_AGED_OUT_ROUTES:
            assert(FALSE);
            break;
        case IARP_PROCESS_BROADCAST_TIMER_EXPIRED:
            assert(FALSE);
            break;
        default:
            ERROR_ReportError("Illegal transition");
            break;
    }

}
