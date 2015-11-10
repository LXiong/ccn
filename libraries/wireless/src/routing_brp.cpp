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

// Note: This is BRP implementation following draft
//       draft-ietf-manet-zone-brp-02.txt. This is not a separate protocol
//       but a library that IERP can use to optimize route discovery.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "routing_iarp.h"
#include "routing_ierp.h"
#include "routing_brp.h"

#define DEBUG_BRP 0


//-------------------------------------------------------------------------
// FUNCTION : BrpDeleteRoutesFromTable
// PURPOSE  : Free memory of Previous bordercast node's ARP Routing table
//              which is temporary created.
// ARGUMENTS: Pointer to ARP routing table data structure
// RETURN   : void
//-------------------------------------------------------------------------
static
void BrpDeleteRoutesFromTable(IarpRoutingTable* routeTable)
{
    MEM_free(routeTable->routingTable);
}

//-------------------------------------------------------------------------
// FUNCTION : BrpConstructPrevBordercasterRoutingTable
// PURPOSE  : Create Previous bordercast node's ARP Routing table
//              for temporary.
// ARGUMENTS: Pointer to node data structure
//          : Pointer to iarpData data structure
//          : Pointer to routeTable data structure
//          : prevBordercaster NodeAddress
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpConstructPrevBordercasterRoutingTable(
    Node* node,
    IarpData* iarpData,
    IarpRoutingTable* routeTable,
    NodeAddress prevBordercaster)
{
    IarpLinkStateTableEntry* linkStateTableEntry = NULL;
    IarpLinkStateInfo* linkStateNeighbor = NULL;
    NodeAddress queue[BRP_MAX_NODE_ENTRY] = {0};
    NodeAddress source = 0;
    int current = 0;
    int last = 0;

    routeTable->routingTable =
        (IarpRoutingEntry *)
        MEM_malloc(sizeof(IarpRoutingEntry)
            * IARP_INITIAL_ROUTE_TABLE_ENTRY);

    memset(routeTable->routingTable, 0,
        (sizeof(IarpRoutingEntry) * IARP_INITIAL_ROUTE_TABLE_ENTRY));
    routeTable->maxEntries = IARP_INITIAL_ROUTE_TABLE_ENTRY;
    routeTable->numEntries = 0;

    // Find the position of the source vertex in the link state table
    linkStateTableEntry = IarpSearchLinkSourceInLinkStateTable(
                              iarpData ,
                              prevBordercaster);

    if (linkStateTableEntry == NULL)
    {
        NodeAddress selfIp = NetworkIpGetInterfaceAddress(
                                 node,
                                 DEFAULT_INTERFACE);

        // check if we are directly connected
        // (only in case the zone radious is 1)
        linkStateTableEntry = IarpSearchLinkSourceInLinkStateTable(
                                  iarpData ,
                                  selfIp);
        if (linkStateTableEntry)
        {
            linkStateNeighbor = linkStateTableEntry->linkStateinfo;

            while (linkStateNeighbor)
            {
                // check if it the previous hop
                source = linkStateNeighbor->destinationAddress;
                if (source == prevBordercaster)
                {
                    IarpRoutingEntry iarpRoutingEntry = {0};

                    iarpRoutingEntry.destinationAddress = selfIp;
                    iarpRoutingEntry.subnetMask =
                        linkStateNeighbor->subnetMask;
                    iarpRoutingEntry.outGoingInterface = DEFAULT_INTERFACE;
                    iarpRoutingEntry.nextHopAddress = selfIp;
                    iarpRoutingEntry.hopCount = 1;
                    IarpEnterRouteInRoutingTable(
                        node,
                        iarpData,
                        routeTable,
                        &iarpRoutingEntry);

                    break;
                }

                linkStateNeighbor = linkStateNeighbor->next;
            }
        }
        return;
    }

    // insert itself into queue
    queue[last] = prevBordercaster;

    while (current <= last && linkStateTableEntry != NULL)
    {
        NodeAddress prevNode = linkStateTableEntry->linkSource;
        linkStateNeighbor = linkStateTableEntry->linkStateinfo;

        // Enter all neighbors in queue
        while (linkStateNeighbor)
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

                 if (prevNode == prevBordercaster)
                 {
                     iarpRoutingEntry.nextHopAddress = source;
                     iarpRoutingEntry.hopCount = 1;
                 }
                 else
                 {
                     IarpRoutingEntry *routingEntry =
                         IarpSearchRouteInRoutingTableEntry(
                             iarpData,
                             routeTable,
                             prevNode);

                     if (!routingEntry)
                     {
                         ERROR_Assert(FALSE, "No ARP Route Entry");
                     }
                     memcpy(&iarpRoutingEntry,
                            routingEntry,
                            sizeof(IarpRoutingEntry));
                     iarpRoutingEntry.hopCount += 1;
                }

                iarpRoutingEntry.destinationAddress = source;
                iarpRoutingEntry.subnetMask =
                    linkStateNeighbor->subnetMask;
                iarpRoutingEntry.outGoingInterface = DEFAULT_INTERFACE;

                if(!IarpSearchRouteInRoutingTableEntry(
                        iarpData,
                        routeTable,
                        source))
                {
                    IarpEnterRouteInRoutingTable(
                        node,
                        iarpData,
                        routeTable,
                        &iarpRoutingEntry);


                    if (DEBUG_BRP)
                    {
                        printf("\nAdding route entry: dest %u nexthop"
                                "%u hopcount %u\n",
                            iarpRoutingEntry.destinationAddress,
                            iarpRoutingEntry.nextHopAddress,
                            iarpRoutingEntry.hopCount);
                    }
                }
            }  // if(!isSourcePresentInQueue(queue, source, last))

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

//-------------------------------------------------------------------------
// FUNCTION : BrpExtractPacket
// PURPOSE  : Extract the BRP packet
// ARGUMENTS: Pointer to node data structure
//          : Pointer to message data structure
//          : Pointer to NodeAddress data structure for Source node
//          : Query ID
//          : Pointer to NodeAddress data structure for Destination node
//          : prevBordercaster NodeAddress
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpExtractPacket(
    Node* node,
    Message* msg,
    NodeAddress* src,
    unsigned int* queryId,
    NodeAddress* dest,
    NodeAddress* prevBordercast)
{
    // Extract the value of the arguments from BRP packet
    BrpHeader* brpHeader;
    brpHeader = (BrpHeader*) MESSAGE_ReturnPacket(msg);

    // Extract fields of BRP header
    *src = brpHeader->querySrcAddr;
    *dest = brpHeader->queryDestAddr;
    *queryId = brpHeader->queryId;
    *prevBordercast = brpHeader->prevBordercastAddr;

    // Form BRP query packet encapsulating IERP query
    MESSAGE_RemoveHeader(node, msg, sizeof(BrpHeader), TRACE_BRP);
}

//-------------------------------------------------------------------------
// FUNCTION : BrpBordercastQueryPacket
// PURPOSE  : Bordercasting query package.
// ARGUMENTS: Pointer to node data structure
//          : Pointer to message data structure
//          : Next Hop Address
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpBordercastQueryPacket(
    Node* node,
    Message* msg,
    NodeAddress nextHop)
{
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,                         // packet to deliver
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE), // src
        nextHop,                     // destination address
        IPTOS_PREC_INTERNETCONTROL,  // priority
        IPPROTO_BRP,                 // protocol
        1,                           // ttl
        DEFAULT_INTERFACE,           // outgoing interface
        nextHop);                    // next hop
}

//-------------------------------------------------------------------------
// FUNCTION : BrpLoadPacket
// PURPOSE  : Bordercasting query package.
// ARGUMENTS: Pointer to node data structure
//          : Pointer to message data structure
//          : Source Address
//          : Query Id
//          : Destination Address
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpLoadPacket(
    Node* node,
    Message* msg,
    NodeAddress src,
    unsigned int queryId,
    NodeAddress destAddr)
{
    // Load the BRP packet with the values provided
    BrpHeader* brpHeader;

    // Form BRP query packet encapsulating IERP query
    MESSAGE_AddHeader(node, msg, sizeof(BrpHeader), TRACE_BRP);

    brpHeader = (BrpHeader*) MESSAGE_ReturnPacket(msg);

    // Fill up BRP header
    brpHeader->querySrcAddr = src;
    brpHeader->queryDestAddr = destAddr;
    brpHeader->queryId = (unsigned short)queryId;
    brpHeader->queryExtn = 0;  // Don't know what to do with it
    brpHeader->reserved = 0;
    brpHeader->prevBordercastAddr =
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE);
}

//-------------------------------------------------------------------------
// FUNCTION : BrpGetQueryCoverageEntryByCacheId
// PURPOSE  : search the Query coverage for BRP Cache ID.
// ARGUMENTS: Pointer to BrpData data structure
//          : BRP Query Cache ID
// RETURN   : Pointer to BrpQueryCoverageEntry
//-------------------------------------------------------------------------

static
BrpQueryCoverageEntry* BrpGetQueryCoverageEntryByCacheId(
    BrpData* brpData, unsigned int brpCacheId)
{
    BrpQueryCoverageTable* coverageTable = &brpData->queryCoverageTable;
    BrpQueryCoverageEntry* coverageEntry = coverageTable->coverageEntry;

    while (coverageEntry)
    {
        if (coverageEntry->brpCacheId == brpCacheId)
        {
            break;
        }
        else
        {
            coverageEntry = coverageEntry->next;
        }
    }
    return coverageEntry;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpScheduleDelete
// PURPOSE  : Schedule timer for Deletion of the Query Coverage Entry
// ARGUMENTS: Pointer to Node data structure
//          : BRP Query Cache ID
// RETURN   : Pointer to BrpQueryCoverageEntry
//-------------------------------------------------------------------------

static
void BrpScheduleDelete(Node* node, unsigned int brpCacheId)
{
    unsigned int* info;

    Message* newMsg = MESSAGE_Alloc(
        node,
        NETWORK_LAYER,
        ROUTING_PROTOCOL_BRP,
        MSG_NETWORK_BrpDeleteQueryEntry);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(unsigned int));

    info = (unsigned int *) MESSAGE_ReturnInfo(newMsg);
    *info = brpCacheId;

    // Schedule the timer after the specified delay
    MESSAGE_Send(node, newMsg, BRP_DEFAULT_REQUEST_TIMEOUT);
}

//-------------------------------------------------------------------------
// FUNCTION : BrpDeleteQueryCoverageEntryByCacheId
// PURPOSE  : Delete the Query Coverage Entry as per BRP Cache ID
// ARGUMENTS: Pointer to BRPData data structure
//          : BRP Query Cache ID
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpDeleteQueryCoverageEntryByCacheId(
    BrpData* brpData, unsigned int brpCacheId)
{
    BrpQueryCoverageTable* coverageTable = &brpData->queryCoverageTable;
    BrpQueryCoverageEntry* coverageEntry = coverageTable->coverageEntry;
    BrpQueryCoverageEntry* prev = NULL;
    BrpQueryCoverageEntry* toFree = NULL;

    while (coverageEntry)
    {
        if (coverageEntry->brpCacheId == brpCacheId)
        {
            toFree = coverageEntry;
            if (prev == NULL)
            {
                // first entry
                coverageTable->coverageEntry = coverageEntry->next;
            }
            else
            {
                prev->next = coverageEntry->next;
            }
            break;
        }
        else
        {
            prev = coverageEntry;
            coverageEntry = coverageEntry->next;
        }
    }

    if (toFree)
    {
        MEM_free(toFree->graph);
        MEM_free(toFree);
        coverageTable->numEntry--;
    }
}

//-------------------------------------------------------------------------
// FUNCTION : BrpAddNewQueryCoverageEntry
// PURPOSE  : Add a new Query Coverage Entry.
// ARGUMENTS: Pointer to BRPData data structure
// RETURN   : pointer to BrpQueryCoverageEntry
//-------------------------------------------------------------------------

static
BrpQueryCoverageEntry* BrpAddNewQueryCoverageEntry(BrpData* brpData)
{
    BrpQueryCoverageTable* coverageTable = &brpData->queryCoverageTable;
    BrpQueryCoverageEntry* coverageEntry = coverageTable->coverageEntry;
    BrpQueryCoverageEntry* prevEntry = coverageEntry;

    if (coverageTable->numEntry == 0)
    {
        // This is the first entry in the list
        coverageTable->coverageEntry = (BrpQueryCoverageEntry*)
            MEM_malloc(sizeof(BrpQueryCoverageEntry));

        coverageEntry =
            coverageTable->coverageEntry;
    }
    else
    {
        // Already there are some entry in the query
        // coverage table. Go to the last entry and add a
        // new entry there.

        while (coverageEntry != NULL)
        {
            prevEntry = coverageEntry;
            coverageEntry = coverageEntry->next;
        }

        ERROR_Assert(prevEntry != NULL, "Strange!!");

        prevEntry->next = (BrpQueryCoverageEntry*)
            MEM_malloc(sizeof(BrpQueryCoverageEntry));

        coverageEntry = prevEntry->next;
    }

    coverageTable->numEntry++;

    return coverageEntry;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpNewNetGraph
// PURPOSE  : Add a new Connecting graph.
// ARGUMENTS: Pointer to IarpData data structure
//          : Pointer to IerpData data structure
//          : Pointer to BrpData data structure
//          : Pointer to BrpQueryCoverageEntry data structure
// RETURN   : pointer to BrpNetGraph
//-------------------------------------------------------------------------

static
BrpNetGraph BrpNewNetGraph(
    IarpData* iarpData,
    IerpData* ierpData,
    BrpData* brpData,
    BrpQueryCoverageEntry* coverageEntry)
{
    IarpRoutingEntry* routingTable = iarpData->routeTable.routingTable;
    unsigned int numEntries = iarpData->routeTable.numEntries;
    unsigned int numPeripheralNode = 0;
    BrpNetGraph graph = NULL;
    int counter = 0;

    while (numEntries > 0)
    {
        numEntries--;


        if (DEBUG_BRP)
        {
            printf("\nIARP table dest: %u, nexthop: %u hopCount: %u\n",
                routingTable[numEntries].destinationAddress,
                routingTable[numEntries].nextHopAddress,
                routingTable[numEntries].hopCount);
        }

        if (routingTable[numEntries].hopCount == ierpData->zoneRadius)
        {
            // This is a peripheral node
            numPeripheralNode++;
        }
    }

    coverageEntry->numPeripheralNode = numPeripheralNode;

    graph = (BrpNetGraph) MEM_malloc(numPeripheralNode *
                sizeof(BrpPeripheralEntry));

    // Now fill up peripheral nodes information
    numEntries = iarpData->routeTable.numEntries;

    while (numEntries > 0)
    {
        numEntries--;

        if (routingTable[numEntries].hopCount ==
            ierpData->zoneRadius)
        {
            if (DEBUG_BRP)
            {
                printf("Adding dest %u nextHop %u to net graph\n",
                    routingTable[numEntries].destinationAddress,
                    routingTable[numEntries].nextHopAddress);
            }

            graph[counter].nextHop =
                routingTable[numEntries].nextHopAddress;
            graph[counter].peripheralNode =
                routingTable[numEntries].destinationAddress;
            graph[counter].isCovered = FALSE;
            counter++;
        }
    }

    return graph;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpIsCovered
// PURPOSE  : Whether this node is already covered or not.
// ARGUMENTS: Pointer to BrpQueryCoverageEntry data structure
//          : Destination Node Address
// RETURN   : boolean
//-------------------------------------------------------------------------

static
BOOL BrpIsCovered(
    BrpQueryCoverageEntry* coverageEntry,
    NodeAddress destAddr)
{
    if (coverageEntry->isCovered)
    {
        return TRUE;
    }
    else
    {
        unsigned int index = 0;
        BrpNetGraph coverage = coverageEntry->graph;
        for (index = 0; index < coverageEntry->numPeripheralNode; index++)
        {
            if (coverage[index].peripheralNode == destAddr)
            {
                return coverage[index].isCovered;
            }
        }
        return FALSE;
    }
}

//-------------------------------------------------------------------------
// FUNCTION : BrpMarkCovered
// PURPOSE  : Mark the node as it is covered by query message.
// ARGUMENTS: Pointer to IerpData data structure
//          : Pointer to IarpRoutingTable data structure
//          : Pointer to BrpQueryCoverageEntry data structure
//          : Previous Border Cast Address
// RETURN   : void
//-------------------------------------------------------------------------

static
void BrpMarkCovered(
    IerpData* ierpData,
    IarpRoutingTable* prevRouteTable,
    BrpQueryCoverageEntry* coverageEntry,
    NodeAddress prevBordercaster)
{
    BrpNetGraph coverage = coverageEntry->graph;
    unsigned int numPeripheralNode = coverageEntry->numPeripheralNode;
    unsigned int numEntries = prevRouteTable->numEntries;
    unsigned int index = 0;
    unsigned int numCoveredPeripheral = 0;
    IarpRoutingEntry* routeEntry = prevRouteTable->routingTable;

    while (numEntries > 0)
    {
        numEntries--;

        if (routeEntry[numEntries].hopCount ==
            ierpData->zoneRadius)
        {
            for (index = 0; index < numPeripheralNode; index++)
            {
                if (coverage[index].nextHop ==
                    routeEntry[numEntries].nextHopAddress)
                {
                    // Mark this entry as covered
                    coverage[index].isCovered = TRUE;
                    numCoveredPeripheral++;
                }
            }
        }
    }

    for (index = 0; index < numPeripheralNode; index++)
    {
        // Obviously not going to use prevBordercaster as next hop
        if (coverage[index].nextHop == prevBordercaster)
        {
            // Mark this entry as covered
            coverage[index].isCovered = TRUE;
            numCoveredPeripheral++;
        }
    }

    coverageEntry->numUncoveredPeripheral =
        coverageEntry->numPeripheralNode - numCoveredPeripheral;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpSend
// PURPOSE  : used by IERP to request packet transmission.
// ARGUMENTS: Pointer to Node data structure
//          : Pointer to IerpData data structure
//          : Pointer to Message data structure
//          : Destination Node Address
//          : Previous hop Address
//          : BRP Cache ID
// RETURN   : void
//-------------------------------------------------------------------------

void BrpSend(
    Node* node,
    IerpData* ierpData,
    Message* msg,
    NodeAddress destAddr,
    unsigned int brpCacheId)
{
    IarpData* iarpData = (IarpData*) IarpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IARP);

    BrpData* brpData = (BrpData*) ierpData->brpData;

    NodeAddress nextHop = ANY_DEST;

    NodeAddress src;
    unsigned int queryId;
    BrpQueryCoverageEntry* coverageEntry;
    NodeAddress prevBordercaster = 0;

    BrpNetGraph coverage = NULL;

    if (DEBUG_BRP)
    {
        printf("\nNode %u: Received query packet from Ierp\n",
                                                node->nodeId);
        printf("\tdestAddr: %u\n", destAddr);
    }

    if (brpCacheId != BRP_NULL_CACHE_ID)
    {
        // This is a query to be relayed
        brpCacheId = brpData->brpCacheId - 1;
        coverageEntry =
            BrpGetQueryCoverageEntryByCacheId(brpData, brpCacheId);
        if(coverageEntry == NULL)
        {
            return;
        }
        src = coverageEntry->nodeId;
        queryId = coverageEntry->queryId;

        if (DEBUG_BRP)
        {
            printf("Node is relaying query to find previous bordercaster\n");
            printf("\tSrc: %u\n", src);
            printf("\tqueryId: %u", queryId);
            printf("\tprevBordercaster: %u\n\n", prevBordercaster);
        }
    }
    else
    {
        // Query is originated at this node
        src = NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE);
        queryId = brpData->queryId++;
        coverageEntry = BrpAddNewQueryCoverageEntry(brpData);

        coverageEntry->nodeId = src;
        coverageEntry->queryId = queryId;
        coverageEntry->brpCacheId = brpData->brpCacheId++;
        coverageEntry->insertionTime = node->getNodeTime();
        coverageEntry->isCovered = FALSE;
        coverageEntry->prevBordercaster = 0;
        coverageEntry->next = NULL;

        coverageEntry->graph = BrpNewNetGraph(
            iarpData,
            ierpData,
            brpData,
            coverageEntry);

        BrpScheduleDelete(node, coverageEntry->brpCacheId);

        if (DEBUG_BRP)
        {
            printf("The query is originated at this node\n");
        }
    }
    prevBordercaster = coverageEntry->prevBordercaster;
    coverage = coverageEntry->graph;

    nextHop = IarpFindNextHop(iarpData, destAddr);

    if ((nextHop != (NodeAddress) NETWORK_UNREACHABLE) &&
        (nextHop != prevBordercaster)                  &&
        (destAddr != ANY_DEST)                         &&
        (NetworkIpIsMyIP(node, destAddr) == TRUE))
    {
        // A valid intrazone route exists.
        // Check if it already has relayed the query. If not relay the
        // the query packet to the destination.

        if (DEBUG_BRP)
        {
            printf("Valid intrazone route exists\n");
        }

        ERROR_Assert(FALSE, "This packet should not return back to BRP"
            "from IERP\n");

        if (!BrpIsCovered(coverageEntry, destAddr))
        {
            // Has not already relayed the query
            // Send the query packet to the next hop to the query
            // destination

            // next hop is already assigned
        }
        else
        {
            // drop the packet
            MESSAGE_Free(node, msg);
        }


        // This condition is handled by IERP so return without
        // doing anything
        return;
    }
    else
    {
        // no intrazone route exists
        // 1. Construct Bordercast Tree spanning all uncovered
        // peripheral nodes
        IarpRoutingTable routeTable;

        if (DEBUG_BRP)
        {
            printf("No intrazone route exists\n");
        }

        nextHop = ANY_DEST;

        if (prevBordercaster)
        {
            BrpConstructPrevBordercasterRoutingTable(
                node,
                iarpData,
                &routeTable,
                prevBordercaster);

            // Mark all nodes in the coverage graph that are coverd
            // by previous bordercaster as covered.
            BrpMarkCovered(
                ierpData,
                &routeTable,
                coverageEntry,
                prevBordercaster);

            BrpDeleteRoutesFromTable(&routeTable);

            if (!coverageEntry->numUncoveredPeripheral)
            {
                // No periphral node left uncovered

                if (DEBUG_BRP)
                {
                    printf("No peripheral node left to be covered, so drop "
                        "the query\n");
                }

                coverageEntry->isCovered = TRUE;
                MESSAGE_Free(node, msg);
                return;
            }
        }
    }

    // 2. Forward the query packet to the tree neighbors
    BrpLoadPacket(node, msg, src, queryId, destAddr);

    if (DEBUG_BRP)
    {
        printf("Bordercasting query packet to nextHop: %u\n",
            nextHop);
    }

    BrpBordercastQueryPacket(node, msg, nextHop);
    brpData->stat.numQuerySent++;

    // 3. Mark all its routing zone nodes as covered
    coverageEntry->isCovered = TRUE;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpCheckIfIntendedReceiver
// PURPOSE  : Wheteher this node is intended receiver .
// ARGUMENTS: Pointer to IerpData data structure
//          : Pointer to IarpRoutingTable data structure
//          : Node's own Ip
// RETURN   : BOOL
//-------------------------------------------------------------------------

static
BOOL BrpCheckIfIntendedReceiver(
    IerpData* ierpData,
    IarpRoutingTable* routeTable,
    NodeAddress selfIp)
{
    unsigned int numEntries = routeTable->numEntries;
    BOOL found = FALSE;
    IarpRoutingEntry* routeEntry = routeTable->routingTable;

    while ((numEntries > 0) && (!found))
    {
        numEntries--;

        if ((routeEntry[numEntries].hopCount ==
            ierpData->zoneRadius)
            && (routeEntry[numEntries].nextHopAddress == selfIp))
        {
            found = TRUE;
        }
    }

    return found;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpCheckQueryCoverageEntry
// PURPOSE  : Get the Query Coverage Entry by Query ID ans Source
// ARGUMENTS: Pointer to BrpQueryCoverageTable data structure
//          : Source Node Address
//          : Query ID
// RETURN   : BrpQueryCoverageEntry
//-------------------------------------------------------------------------

static
BrpQueryCoverageEntry* BrpCheckQueryCoverageEntry(
        BrpQueryCoverageTable* queryCoverageTable,
        NodeAddress src,
        unsigned int queryId)
{
    BrpQueryCoverageEntry* coverageEntry = queryCoverageTable->coverageEntry;

    while (coverageEntry)
    {
        if (coverageEntry->nodeId == src
            && coverageEntry->queryId == queryId)
        {
            return coverageEntry;
        }
        coverageEntry = coverageEntry->next;
    }
    return NULL;
}

//-------------------------------------------------------------------------
// FUNCTION : BrpDeliver
// PURPOSE  : used by IP to deliver packet to BRP
// ARGUMENTS: Pointer to Node data structure
//          : Pointer to Message data structure
//          : Source Node Address
//          : Destination Node Address
//          : Query ID
//          : Interface Index
//          : TTL (Time To Live)
// RETURN   : void
//-------------------------------------------------------------------------

void BrpDeliver(
    Node* node,
    Message* msg,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    unsigned int interfaceIndex,
    unsigned int ttl)
{
    IarpData* iarpData = (IarpData*) IarpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IARP);

    IerpData* ierpData = (IerpData*) IerpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IERP);

    BrpData* brpData = (BrpData*) ierpData->brpData;

    IarpRoutingTable routeTable;
    NodeAddress src;
    unsigned int queryId;
    NodeAddress dest;
    NodeAddress prevBordercaster;
    NodeAddress selfIp =
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE);
    BrpQueryCoverageEntry* coverageEntry;

    if (brpData == NULL)
    {
        // not enabled on this node, do nothing
        MESSAGE_Free(node, msg);
        return;
    }

    brpData->stat.numQueryReceived++;

    BrpExtractPacket(
        node,
        msg,
        &src,
        &queryId,
        &dest,
        &prevBordercaster);

    if (DEBUG_BRP)
    {
        printf("\nNode: %u recevied query packet from IP\n", node->nodeId);
        printf("\tIP src: %u\n", srcAddr);
        printf("\tIP dest: %u\n", destAddr);
        printf("\tBRP dest: %u\n", dest);
        printf("\tPrevious Bordercaster: %u\n", prevBordercaster);
        printf("\tquery id: %u\n\n", queryId);
    }


    coverageEntry = BrpCheckQueryCoverageEntry(
        &brpData->queryCoverageTable,
        src,
        queryId);

    if (!coverageEntry)
    {
        coverageEntry = BrpAddNewQueryCoverageEntry(brpData);

        coverageEntry->nodeId = src;
        coverageEntry->queryId = queryId;
        coverageEntry->brpCacheId = brpData->brpCacheId++;
        coverageEntry->insertionTime = node->getNodeTime();
        coverageEntry->isCovered = FALSE;
        coverageEntry->prevBordercaster = prevBordercaster;
        coverageEntry->next = NULL;

        coverageEntry->graph = BrpNewNetGraph(
            iarpData,
            ierpData,
            brpData,
            coverageEntry);

        BrpScheduleDelete(node, coverageEntry->brpCacheId);

    }
    else
    {
        prevBordercaster = coverageEntry->prevBordercaster;
    }

    BrpConstructPrevBordercasterRoutingTable(
        node,
        iarpData,
        &routeTable,
        prevBordercaster);

    // Mark all nodes in the coverage graph that are coverd
    // by previous bordercaster as covered.
    BrpMarkCovered(
        ierpData,
        &routeTable,
        coverageEntry,
        prevBordercaster);

    if (BrpCheckIfIntendedReceiver(ierpData, &routeTable, selfIp))
    {
        // 1. Record the BRP state
        // 2. Deliver the packet to IERP

        if (DEBUG_BRP)
        {
            printf("Intended receiver of the query, so hand"
                                        " it over to IERP\n");
        }

        IerpHandleProtocolPacket(
            node,
            msg,
            srcAddr,
            destAddr,
            interfaceIndex,
            ttl);
    }
    else
    {
        // 1. Mark its own routing zone as covered
        coverageEntry->isCovered = TRUE;

        if (DEBUG_BRP)
        {
            printf("Not intended receiver, so discard the query\n");
        }

        // 2. Discard the packet
        MESSAGE_Free(node, msg);
    }

    BrpDeleteRoutesFromTable(&routeTable);
}

//-------------------------------------------------------------------------
// FUNCTION : BrpInit
// PURPOSE  : BRP Initialization
// ARGUMENTS: Pointer to Node data structure
//          : Pointer to BrpData data structure
// RETURN   : void
//-------------------------------------------------------------------------

void BrpInit(Node* node, BrpData* brpData)
{
    // Initialize Query Coverage Table
    brpData->queryCoverageTable.numEntry = 0;
    brpData->queryCoverageTable.coverageEntry = NULL;

    // Initialize queryId
    brpData->queryId = 0;

    // Initialize brp cache id
    brpData->brpCacheId = 0;

    // Initialize statistical variables
    memset(&brpData->stat, 0, sizeof(BrpStat));
}

//-------------------------------------------------------------------------
// FUNCTION : BrpProcessEvent
// PURPOSE  : BRP Process Event
// ARGUMENTS: Pointer to Node data structure
//          : Pointer to Message data structure
// RETURN   : void
//-------------------------------------------------------------------------

void BrpProcessEvent(Node* node, Message* msg)
{
    IerpData* ierpData = (IerpData*) IerpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IERP);

    BrpData* brpData = (BrpData*) ierpData->brpData;

    // Fuction to process timers
    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_NETWORK_BrpDeleteQueryEntry:
        {
            unsigned int* brpCacheId
                            = (unsigned int*) MESSAGE_ReturnInfo(msg);

            BrpDeleteQueryCoverageEntryByCacheId(brpData, *brpCacheId);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Delete query is the only event now");
        }
    }
}

//-------------------------------------------------------------------------
// FUNCTION : BrpPrintStats
// PURPOSE  : Printing BRP Status
// ARGUMENTS: Pointer to Node data structure
// RETURN   : void
//-------------------------------------------------------------------------

void BrpPrintStats(Node* node)
{
    // Ha ha now this function is empty
    IerpData* ierpData = (IerpData*) IerpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IERP);

    BrpData* brpData = (BrpData*) ierpData->brpData;

    char buf[MAX_STRING_LENGTH];

    char statProtocolName[MAX_STRING_LENGTH];
    strcpy(statProtocolName,"BRP");
    sprintf(buf, "Number of query packets sent = %u",
                            brpData->stat.numQuerySent);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of query packets received = %u",
                            brpData->stat.numQueryReceived);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
}

void BrpFinalize(Node* node)
{
    BrpData *dataPtr = (BrpData*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_BRP);

    // If protocol data pointer is NULL give error Message.
    ERROR_Assert(dataPtr != NULL,
       "Pointer to the protocol Data Cannot be NULL");

    if (dataPtr->statsPrinted) {
        return;
    }

    if (dataPtr->printStats) {
        dataPtr->statsPrinted = TRUE;
        BrpPrintStats(node);
    }
    BrpPrintStats(node);
}
