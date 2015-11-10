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
#include <math.h>

#ifdef ADDON_IPv6
#include "dualip.h"
#endif // ADDON_IPv6

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "mac_dot11.h"
#include "mac_dot11s-frames.h"
#include "mac_dot11s.h"
#include "mac_dot11s-hwmp.h"


#define HWMP_DebugRouteTable 0
#define HWMP_TraceComments 0


static
void HwmpTbr_SetState(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_TbrState state);

static
void Hwmp_SendTriggeredRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo);


//-------------------------------------------------------------------------
// FUNCTION  : Hwmp_MemoryChunkAlloc
// PURPOSE   : Function to allocate a chunk of memory
// ARGUMENTS : hwmp, pointer to Hwmp main data structure
// RETURN    : void
//-------------------------------------------------------------------------

static
void Hwmp_MemoryChunkAlloc(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    int i = 0;
    HWMP_MemPoolEntry* freeList = NULL;

    hwmp->freeList = (HWMP_MemPoolEntry*)
        MEM_malloc(HWMP_MEM_UNIT * sizeof(HWMP_MemPoolEntry));
    ERROR_Assert(hwmp->freeList != NULL,
        "Hwmp_MemoryChunkAlloc: Cannot allocate memory");
    // Uncomment to free memory in Finalize function.
    // hwmp->refFreeList = hwmp->freeList;

    freeList = hwmp->freeList;
    for (i = 0; i < HWMP_MEM_UNIT - 1; i++)
    {
        freeList[i].next = &freeList[i + 1];
    }
    freeList[HWMP_MEM_UNIT - 1].next = NULL;
}


//-------------------------------------------------------------------------
// FUNCTION  : Hwmp_MemoryMalloc
// PURPOSE   : Allocate a single cell of memory from the memory chunk
// ARGUMENTS : hwmp, pointer to Hwmp main data structure
// RETURN    : Address of free memory cell
//-------------------------------------------------------------------------

static
HWMP_RouteEntry* Hwmp_MemoryMalloc(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    HWMP_RouteEntry* temp = NULL;

    if (!hwmp->freeList)
    {
        Hwmp_MemoryChunkAlloc(node, dot11, hwmp);
    }

    temp = (HWMP_RouteEntry*)hwmp->freeList;
    hwmp->freeList = hwmp->freeList->next;
    return temp;
}


//-------------------------------------------------------------------------
// FUNCTION  : Hwmp_MemoryFree
// PURPOSE   : Function to return a memory cell to the memory pool
// ARGUMENTS : hwmp, pointer to Hwmp main data structure,
//             ptr, pointer to route entry
// RETURN    : void
//-------------------------------------------------------------------------

static
void Hwmp_MemoryFree(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* ptr)
{
    HWMP_MemPoolEntry* temp = (HWMP_MemPoolEntry*) ptr;
    temp->next = hwmp->freeList;
    hwmp->freeList = temp;
}


//-----------------------------------------------------------------------
// FUNCTION  : Hwmp_GetRreqId
// PURPOSE   : Obtains the broadcast ID for the outgoing RREQ
// ARGUMENTS : hwmp, HWMP main data structure.
// RETURN    : The flooding Id
// NOTE      : This function increments the flooding ID by 1
//-----------------------------------------------------------------------

static
unsigned int Hwmp_GetRreqId(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    hwmp->rreqId++;
    return ((unsigned)hwmp->rreqId);
}


//-----------------------------------------------------------------------
// FUNCTION  : Hwmp_IncreaseTtl
// PURPOSE   : Increase the TTL value of a destination to which RREQ
//             is being sent
// ARGUMENTS : hwmp, Hwmp main structure
//             current, sent table entry for which TTL is incremented
// RETURN    : None
//-----------------------------------------------------------------------

static
void Hwmp_IncreaseTtl(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RreqSentNode* current)
{
    current->ttl += HWMP_RREQ_TTL_INCREMENT;

    if (current->ttl > HWMP_RREQ_TTL_THRESHOLD)
    {
        // Beyond TTL threshold, value is net diameter.
        current->ttl = HWMP_NET_DIAMETER;
    }
    return;
}


//-----------------------------------------------------------------------
// FUNCTION  : Hwmp_GetLastHopCount
// PURPOSE   : Get last known hop count known to the destination node
// ARGUMENTS : destAddr, address for which the next hop is required
// RETURN    : Last hop count if found, initial TTL otherwise.
//-----------------------------------------------------------------------

static
int Hwmp_GetLastHopCount(
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_RoutingTable* routeTable = &hwmp->routeTable;
    HWMP_RouteEntry* current =
        routeTable->routeHash[destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE)];

    // Skip entries with smaller destination address.
    while (current && current->addrSeqInfo.addr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->addrSeqInfo.addr == destAddr)
    {
        ERROR_Assert(current->lastHopCount > 0,
            "Hwmp_GetLastHopCount: invalid last hop count.\n");

        // Got matching destination, return the hop count.
        return current->lastHopCount;
    }
    else
    {
        // No match found, return initial value.
        return HWMP_RREQ_INITIAL_TTL;
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpPktBuffer_Insert
// PURPOSE   : Insert a packet into the buffer when no route is available
// ARGUMENTS : frameInfo, details of frame waiting for route to destination
// RETURN    : TRUE if insert is successful, FALSE if buffer overflows
//-----------------------------------------------------------------------

static
BOOL HwmpPktBuffer_Insert(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo)
{
    HWMP_MsgBuffer* buffer = &(hwmp->msgBuffer);

    HWMP_BufferNode* current = NULL;
    HWMP_BufferNode* previous = NULL;
    HWMP_BufferNode* newNode = NULL;

    Dot11s_TraceFrameInfo(node, dot11, frameInfo, "PktBuffer_Insert");

    // If no buffer size limit (in bytes) is configured,
    // check for limit of packet count.
    // If buffer size is exceeded, silently drop the packet.

    if (hwmp->bufferSizeInByte == 0)
    {
        if (buffer->size == hwmp->bufferSizeInNumPacket)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node,
                              dot11,
                              NULL,
                              "!! HwmpPktBuffer_Insert: "
                              "buffer size in packets exceeded");
            }

#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                frameInfo->msg,
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex,
                MAC_Drop,
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            hwmp->stats->dataDroppedForOverlimit++;
            Dot11s_MemFreeFrameInfo(node, &frameInfo, TRUE);
            return FALSE;
        }
    }
    else
    {
        if ((buffer->numByte + MESSAGE_ReturnPacketSize(frameInfo->msg))
            > hwmp->bufferSizeInByte)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node,
                              dot11,
                              NULL,
                              "!! HwmpPktBuffer_Insert: "
                              "buffer size in bytes exceeded");
            }

#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                frameInfo->msg,
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex,
                MAC_Drop,
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            hwmp->stats->dataDroppedForOverlimit++;
            Dot11s_MemFreeFrameInfo(node, &frameInfo, TRUE);
            return FALSE;
        }
    }

    // Find insertion point. Insert after all addresses match.
    // This is to maintain a sorted list in ascending order
    // of destination address.
    previous = NULL;
    current = buffer->head;

    while (current && current->frameInfo->DA <= frameInfo->DA)
    {
        previous = current;
        current = current->next;
    }

    // Allocate space for the new message.
    Dot11s_Malloc(HWMP_BufferNode, newNode);

    // Store the message details along with insertion time.
    newNode->frameInfo = frameInfo;
    newNode->timestamp = node->getNodeTime();

    newNode->next = current;

    // Increase the size of the buffer.
    ++(buffer->size);
    buffer->numByte += MESSAGE_ReturnPacketSize(frameInfo->msg);

    // Got the insertion point.
    if (previous == NULL)
    {
        // The is the first message in the buffer
        // or to be inserted as the first.
        buffer->head = newNode;
    }
    else
    {
        // This is an intermediate node in the list.
        previous->next = newNode;
    }

    return TRUE;
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpPktBuffer_GetPacket
// PURPOSE   : Get a packet that was buffered
// ARGUMENTS : destAddr, destination address of packet to be retrieved
//             prevHopAddr, previous hop address of packet
// RETURN    : Frame details of packet for this destination
//-----------------------------------------------------------------------

static
DOT11s_FrameInfo* HwmpPktBuffer_GetPacket(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    Mac802Address* prevHopAddr)
{
    HWMP_MsgBuffer* buffer = &(hwmp->msgBuffer);

    HWMP_BufferNode* current = buffer->head;
    DOT11s_FrameInfo* frameInfo = NULL;
    HWMP_BufferNode* toFree = NULL;

    if (!current)
    {
        // Buffer is empty; do nothing.
    }
    else if (current->frameInfo->DA == destAddr)
    {
        // The first packet is the desired packet.
        toFree = current;
        buffer->head = toFree->next;

        frameInfo = toFree->frameInfo;
        *prevHopAddr = toFree->frameInfo->TA;

        buffer->numByte -= MESSAGE_ReturnPacketSize(toFree->frameInfo->msg);
        MEM_free(toFree);
        --(buffer->size);
    }
    else
    {
        while (current->next && current->next->frameInfo->DA < destAddr)
        {
            current = current->next;
        }

        if (current->next && current->next->frameInfo->DA == destAddr)
        {
            // Destination matches, return this packet.
            toFree = current->next;

            *prevHopAddr = toFree->frameInfo->TA;
            frameInfo = toFree->frameInfo;
            buffer->numByte -=
                MESSAGE_ReturnPacketSize(toFree->frameInfo->msg);
            current->next = toFree->next;
            MEM_free(toFree);
            --(buffer->size);
        }
    }

    if (frameInfo != NULL)
    {
        Dot11s_TraceFrameInfo(node, dot11, frameInfo, "PktBuffer_GetPkt: ");
    }

    return frameInfo;
}

//-----------------------------------------------------------------------
// FUNCTION  : HwmpPrecursorTable_DeleteAll
// PURPOSE   : Free the precursors list of a route
// ARGUMENTS : rtEntry, target route entry
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpPrecursorTable_DeleteAll(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* rtEntry)
{
    HWMP_Precursors* precursors = &(rtEntry->precursors);

    HWMP_PrecursorsNode* current = precursors->head;
    HWMP_PrecursorsNode* next = NULL;

    // Delete all the entries in the precursors table.
    while (current != NULL)
    {
        next = current->next;
        MEM_free(current);
        current = next;
    }

    // Reinitialize the precursors table.
    precursors->head = NULL;
    precursors->size = 0;
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpPrecursorTable_DeleteEntry
// PURPOSE   : Delete a particular destination from the precursors table
// ARGUMENTS : address, address to be deleted
//             rtEntry, route entry for precursor table
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpPrecursorTable_DeleteEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* rtEntry,
    Mac802Address address)
{
    HWMP_Precursors* precursors = &(rtEntry->precursors);

    HWMP_PrecursorsNode* toFree = NULL;
    HWMP_PrecursorsNode* current = precursors->head;

    if (!current)
    {
        // The precursors table is empty; do nothing.
    }
    else if (current->destAddr == address)
    {
        // The first entry is the desired entry.
        toFree = current;
        precursors->head = toFree->next;
        MEM_free(toFree);
        --(precursors->size);
    }
    else
    {
        // Search for the entry.
        while (current->next && current->next->destAddr < address)
        {
            current = current->next;
        }

        if (current->next && current->next->destAddr == address)
        {
            // Got the desired entry, delete the entry
            toFree = current->next;
            current->next = toFree->next;
            MEM_free(toFree);
            --(precursors->size);
        }
    }
}

//-----------------------------------------------------------------------
// FUNCTION  : HwmpPrecursorTable_Insert
// PURPOSE   : Adding a destination in the precursors list
// ARGUMENTS : destAddr, the address to insert
//             rtEntry, route entry for precursor table
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpPrecursorTable_Insert(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* rtEntry,
    Mac802Address destAddr)
{
    HWMP_Precursors* precursors = &(rtEntry->precursors);

    HWMP_PrecursorsNode* current = NULL;
    HWMP_PrecursorsNode* previous = NULL;

    HWMP_PrecursorsNode* newNode = NULL;

    // Find Insertion point. Insert after all address matches.
    previous = NULL;
    current = precursors->head;

    while (current && current->destAddr < destAddr)
    {
        previous = current;
        current = current->next;
    }

    if (current && current->destAddr == destAddr)
    {
        // The entry exists; exit without adding a new entry
    }
    else
    {
        // At this point, add node into the precursors list.

        ++(precursors->size);

        newNode =
            (HWMP_PrecursorsNode *) MEM_malloc(sizeof(HWMP_PrecursorsNode));

        newNode->destAddr = destAddr;
        newNode->next = current;

        if (previous == NULL)
        {
            precursors->head = newNode;
        }
        else
        {
            previous->next = newNode;
        }
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_InsertExpiredEntry
// PURPOSE   : Insert route entry in expiry list.
// ARGUMENTS : routeEntry, pointer to route entry to be
//                  inserted into expiry list
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpRouteTable_InsertExpiredEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* routeEntry)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    if (routeTable->routeExpireTail == NULL)
    {
        routeTable->routeExpireTail = routeEntry;
        routeTable->routeExpireHead = routeEntry;
        routeEntry->expirePrev = NULL;
        routeEntry->expireNext = NULL;
    }
    else if (routeTable->routeExpireTail == routeTable->routeExpireHead)
    {
        if (routeTable->routeExpireTail->lifetime > routeEntry->lifetime)
        {
            routeEntry->expireNext = routeTable->routeExpireTail;
            routeEntry->expirePrev = NULL;
            routeTable->routeExpireHead = routeEntry;
            routeTable->routeExpireTail->expirePrev = routeEntry;
        }
        else
        {
            routeEntry->expireNext = NULL;
            routeEntry->expirePrev = routeTable->routeExpireHead;
            routeTable->routeExpireTail = routeEntry;
            routeTable->routeExpireHead->expireNext = routeEntry;
        }
    }
    else
    {
        HWMP_RouteEntry* current = routeTable->routeExpireTail;
        HWMP_RouteEntry* previous = current;

        while (current && current->lifetime > routeEntry->lifetime)
        {
            previous = current;
            current = current->expirePrev;
        }

        routeEntry->expirePrev = current;

        if (routeTable->routeExpireTail == current)
        {
            routeTable->routeExpireTail = routeEntry;
            routeEntry->expireNext = NULL;
        }
        else
        {
            previous->expirePrev = routeEntry;
            routeEntry->expireNext = previous;

        }

        if (previous == routeTable->routeExpireHead)
        {
            routeTable->routeExpireHead = routeEntry;
        }
        else
        {
            current->expireNext = routeEntry;
        }
    }
}

//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_MoveExpiredEntry
// PURPOSE   : Move entry into expiry table
// ARGUMENTS : routeEntry, route entry to move into expiry list
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpRouteTable_MoveExpiredEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* routeEntry)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    if (routeTable->routeExpireTail != routeEntry)
    {
        if (routeTable->routeExpireHead == routeEntry)
        {
            routeTable->routeExpireHead = routeEntry->expireNext;
        }
        else
        {
            routeEntry->expirePrev->expireNext = routeEntry->expireNext;
        }
        routeEntry->expireNext->expirePrev = routeEntry->expirePrev;

        HwmpRouteTable_InsertExpiredEntry(node, dot11, hwmp, routeEntry);
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_Print
// PURPOSE   : Print routing table.
// ARGUMENTS : hwmp, pointer to HWMP data structure
// RETURN    : None
// NOTES     : To print table to console, change
//              #define HWMP_DebugRouteTable at the top of this file
//              to a non-zero value
//-----------------------------------------------------------------------

static
void HwmpRouteTable_Print(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* rtEntry = NULL;
    int i = 0;
    printf("The Routing Table of Node %u is:\n"
        "      Dest       DestSeq  HopCount  Metric  Intf     NextHop    "
        "is Active   lifetime    precursors\n"
        "----------------------------------------------------------------"
        "-----------------------------------\n",
        node->nodeId);
    for (i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (rtEntry = routeTable->routeHash[i]; rtEntry != NULL;
            rtEntry = rtEntry->hashNext)
        {
            char clockStr[MAX_STRING_LENGTH];
            char dest[MAX_STRING_LENGTH];
            char nextHop[MAX_STRING_LENGTH];
            char trueOrFalse[6];
            Dot11s_AddrAsDotIP(dest, &rtEntry->addrSeqInfo.addr);
            Dot11s_AddrAsDotIP(nextHop, &rtEntry->nextHop);
            TIME_PrintClockInSecond(rtEntry->lifetime, clockStr);

            if (rtEntry->isActive)
            {
                strcpy(trueOrFalse, "TRUE");
            }
            else
            {
                strcpy(trueOrFalse, "FALSE");
            }

            printf("%15s  %5u  %5d  %12u  %5d  %15s  %5s    %9s  ",
                dest,
                rtEntry->addrSeqInfo.seqNo,
                rtEntry->hopCount, rtEntry->metric,
                rtEntry->interfaceIndex, nextHop, trueOrFalse, clockStr);

            if (rtEntry->precursors.size == 0)
            {
                printf("%s\n", "NULL");
            }
            else
            {
                HWMP_PrecursorsNode* precursors = NULL;
                char precursorStr[MAX_STRING_LENGTH];

                for (precursors = rtEntry->precursors.head;
                    precursors != NULL; precursors = precursors->next)
                {
                    Dot11s_AddrAsDotIP(precursorStr, &precursors->destAddr);

                    printf("%s ", precursorStr);
                }
                printf("\n");
            }
        }
    }
    printf("-------------------------------------------------------------"
        "-------------------------------------\n\n");
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_GetSequenceNumber
// PURPOSE   : Obtain the sequence number of the destination node if the
//             destination has an entry in the routing table. If there is
//             no entry for the destination, a sequence number 0 is returned
// ARGUMENTS : destAddr, Destination address whose sequence number is wanted
// RETURN    : sequence number, if there is an existing, expired or invalid
//             route, 0 otherwise.
//-----------------------------------------------------------------------

static
unsigned int HwmpRouteTable_GetSequenceNumber(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* current =
       routeTable->routeHash[destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE)];

    // Skip entries with smaller destination address.
    while (current && current->addrSeqInfo.addr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->addrSeqInfo.addr == destAddr)
    {
        // Got the desired destination.
         return ((unsigned)current->addrSeqInfo.seqNo);
    }
    else
    {
        // No entry for destination.
        return 0;
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_DeletePrecursorFromAllEntries
// PURPOSE   : Delete an address from the precursor's list of
//             all the routes
// ARGUMENTS : address, address to be deleted
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpRouteTable_DeletePrecursorFromAllEntries(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address address)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* current = NULL;
    int i = 0;

    for (i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = routeTable->routeHash[i];
             current != NULL;
             current = current->hashNext)
        {
            // For all the entries in the routing table
            // delete the address from its precursors list.
            HwmpPrecursorTable_DeleteEntry(node, dot11, hwmp,
                current, address);
        }
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_DisableRoute
// PURPOSE   : Disable an active route.
// ARGUMENTS : current, route entry to be disabled
//             incrementSeq, TRUE if sequence number should be incremented
// RETURN    : destination sequence number,
//             0 if the route does not exist
//-----------------------------------------------------------------------

static
unsigned int HwmpRouteTable_DisableRoute(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* current,
    BOOL incrementSeq)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    ERROR_Assert(current->isActive == TRUE,
        "HwmpRouteTable_DisableRoute: "
        "Route should be active, but it is not.\n");

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &current->addrSeqInfo.addr);
        Dot11s_AddrAsDotIP(nextHopStr, &current->nextHop);
        sprintf(traceStr, "HwmpRouteTable_DisableRoute: "
            "dest %s, nextHop %s", destStr, nextHopStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    // Empty the precursors table for the route.
    HwmpPrecursorTable_DeleteAll(node, dot11, hwmp, current);

    // Disable it by setting the hop count and
    // metric to maximum.

    // Record hop count field.
    current->lastHopCount = current->hopCount;
    current->hopCount = HWMP_HOPCOUNT_MAX;

    // Record metric field.
    current->lastMetric = current->metric;
    current->metric = HWMP_METRIC_MAX;

    current->isActive = FALSE;

    // Set timer to delete the route after delete period.
    current->lifetime = node->getNodeTime() + HWMP_DELETION_PERIOD;

    if (incrementSeq)
    {
        // Increment the destination sequence number field
        // There is an implicit assumption here that routes
        // are formed as a result of RREQ route discovery. With
        // path creation, disabling from neighbor association
        // or portal announcements, this increment to sequence
        // number needs attention for side effects.
        current->addrSeqInfo.seqNo++;
    }

    if (routeTable->routeExpireHead == routeTable->routeExpireTail)
    {
        routeTable->routeExpireHead = NULL;
        routeTable->routeExpireTail = NULL;
    }
    else if (routeTable->routeExpireHead == current)
    {
        routeTable->routeExpireHead = current->expireNext;
        routeTable->routeExpireHead->expirePrev = NULL;
    }
    else if (routeTable->routeExpireTail == current)
    {
        routeTable->routeExpireTail = current->expirePrev;
        routeTable->routeExpireTail->expireNext = NULL;
    }
    else
    {
        current->expireNext->expirePrev = current->expirePrev;
        current->expirePrev->expireNext = current->expireNext;
    }

    current->expireNext = NULL;
    current->expirePrev = NULL;

    if (routeTable->routeDeleteTail == NULL
        && routeTable->routeDeleteHead == NULL)
    {
        routeTable->routeDeleteHead = current;
        routeTable->routeDeleteTail = current;
        current->deleteNext = NULL;
        current->deletePrev = NULL;
    }
    else
    {
        current->deletePrev = routeTable->routeDeleteTail;
        current->deleteNext = NULL;
        routeTable->routeDeleteTail->deleteNext = current;
        routeTable->routeDeleteTail = current;
    }

    // An expired routing table entry SHOULD NOT be expunged before
    // (current_time + DELETE_PERIOD) (see section 8.13).  Otherwise,
    // the soft state corresponding to the route (e.g., Last Hop
    // Count) will be lost. Sec: 8.4

    if (hwmp->isDeleteTimerSet == FALSE)
    {
        hwmp->isDeleteTimerSet = TRUE;

        DOT11s_TimerInfo timerInfo(current->addrSeqInfo.addr);
        Dot11s_StartTimer(node, dot11, &timerInfo,
            (clocktype) HWMP_DELETION_PERIOD,
            MSG_MAC_DOT11s_HwmpDeleteRouteTimeout);
    }

    if (HWMP_DebugRouteTable)
    {
        char clockStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &current->addrSeqInfo.addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %u disabled route to %s at %s\n", node->nodeId,
            addrStr, clockStr);
        HwmpRouteTable_Print(node, dot11, hwmp);
    }

    return ((unsigned)current->addrSeqInfo.seqNo);
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_GetNextHop
// PURPOSE   : Look-up routing table to obtain next hop to destination
// ARGUMENTS : destAddr, The destination address for which next hop is wanted
//             nextHop, pointer to be assigned the next hop address
//             interfaceIndex, interface index through which the message
//                  is to be sent
// RETURN    : TRUE, if a route is found
//             FALSE, otherwise
//-----------------------------------------------------------------------

static
BOOL HwmpRouteTable_GetNextHop(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    Mac802Address* nextHop,
    int* interfaceIndex)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* current =
        routeTable->routeHash[destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE)];

    *interfaceIndex = -1;
    while (current && current->addrSeqInfo.addr < destAddr)
    {
        current = current->hashNext;
    }

    if (current &&
        (current->addrSeqInfo.addr == destAddr) &&
        (current->isActive == TRUE))
    {
        // Got a route that is valid.
        // Assign next hop and interface index.
        *nextHop = current->nextHop;
        *interfaceIndex = current->interfaceIndex;
        return TRUE;
    }
    else
    {
        // The entry does not exist.
        return FALSE;
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_FindEntry
// PURPOSE   : Check if a route to a destination exists.
// ARGUMENTS : destAddr, destination address of the packet
//             isValid, TRUE if the route is active
// RETURN    : pointer to the route if it exists in the routing table,
//                  NULL otherwise
//-----------------------------------------------------------------------

static
HWMP_RouteEntry* HwmpRouteTable_FindEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    BOOL* isValid)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* current =
        routeTable->routeHash[destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE)];

    *isValid = FALSE;

    while (current && current->addrSeqInfo.addr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->addrSeqInfo.addr == destAddr)
    {
        // Found the entry
        if (current->isActive == TRUE)
        {
            // The entry is a valid route
            *isValid = TRUE;
        }
        return current;
    }
    else
    {
        // No existing entry.
        return NULL;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_UpdateEntry
// PURPOSE   : Insert/Update an entry into the route table
// ARGUMENTS : destAddr, destination Address
//             destSeqNo, destination sequence number
//             hopCount, number of hops to the destination
//             metric, metric to the destination
//             nextHop, next hop towards the destination
//             lifetime, lifetime of the route
//             isActive, TRUE if an active route
//             interfaceIndex, the interface through the message has been
//                  received (i.e.. the interface in which to direct
//                  packet to reach the destination)
// RETURN   : route pointer to modified or created route
//--------------------------------------------------------------------

static
HWMP_RouteEntry* HwmpRouteTable_UpdateEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    unsigned int destSeqNo,
    int hopCount,
    unsigned int metric,
    Mac802Address nextHop,
    clocktype lifetime,
    BOOL isActive,
    int interfaceIndex)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* theNode = NULL;
    HWMP_RouteEntry* previous = NULL;
    int queueNo =  destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE);
    // Find Insertion point.
    previous = NULL;
    HWMP_RouteEntry* current =
            routeTable->routeHash[queueNo];

    previous = current;

    while (current && current->addrSeqInfo.addr < destAddr)
    {
        previous = current;
        current = current->hashNext;
    }

    if (current && current->addrSeqInfo.addr == destAddr)
    {
        if (current->isActive)
        {
            if (current->lifetime < lifetime)
            {
                current->lifetime = lifetime;
                HwmpRouteTable_MoveExpiredEntry(node, dot11, hwmp, current);
            }
        }
        else
        {
            current->isActive = isActive;
            current->lifetime = lifetime;

            if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
            {
                routeTable->routeDeleteHead = NULL;
                routeTable->routeDeleteTail = NULL;
            }
            else if (routeTable->routeDeleteHead == current)
            {
                routeTable->routeDeleteHead = current->deleteNext;
                routeTable->routeDeleteHead->deletePrev = NULL;
            }
            else if (routeTable->routeDeleteTail == current)
            {
                routeTable->routeDeleteTail = current->deletePrev;
                routeTable->routeDeleteTail->deleteNext = NULL;
            }
            else
            {
                current->deleteNext->deletePrev = current->deletePrev;
                current->deletePrev->deleteNext = current->deleteNext;
            }
            current->deleteNext = NULL;
            current->deletePrev = NULL;
            HwmpRouteTable_InsertExpiredEntry(node, dot11, hwmp, current);
        }
        theNode = current;
    }
    else
    {
        ++(routeTable->size);

        // Adding a new entry here.
        theNode = Hwmp_MemoryMalloc(node, dot11, hwmp);
        memset(theNode, 0, sizeof(HWMP_RouteEntry));

        theNode->deleteNext = NULL;
        theNode->deletePrev = NULL;

        theNode->lifetime = lifetime;
        theNode->isActive = isActive;
        theNode->addrSeqInfo.addr = destAddr;

        theNode->lastHopCount = (unsigned)hopCount;
        ERROR_Assert(theNode->lastHopCount > 0,
            "HwmpRouteTable_UpdateEntry: "
            " Last hopcount should be greater than 0.\n");

        theNode->lastMetric = metric;
        ERROR_Assert(theNode->lastMetric > 0,
            "HwmpRouteTable_UpdateEntry: "
            " Last metric is negative.\n");

        // Initialize precursors list.
        (&theNode->precursors)->head = NULL;
        (&theNode->precursors)->size = 0;

        if (current == routeTable->routeHash[queueNo])
        {
           // First entry in the queue.
           theNode->hashNext = current;
           theNode->hashPrev = NULL;
           routeTable->routeHash[queueNo] = theNode;

           if (current)
           {
               current->hashPrev = theNode;
           }
        }
        else
        {
           theNode->hashNext = current;
           theNode->hashPrev = previous;
           previous->hashNext = theNode;
           if (current)
           {
                current->hashPrev = theNode;
           }
        }
        HwmpRouteTable_InsertExpiredEntry(node, dot11, hwmp, theNode);
    }

    theNode->locallyRepairable = FALSE;
    if (destSeqNo > 0)
    {
        theNode->addrSeqInfo.seqNo = destSeqNo;
    }
    theNode->hopCount = (unsigned)hopCount;
    theNode->metric = metric;
    theNode->nextHop = nextHop;
    theNode->interfaceIndex = (unsigned)interfaceIndex;

    if (HWMP_DebugRouteTable)
    {
        char addrStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        Dot11s_AddrAsDotIP(addrStr, &destAddr);
        printf("After adding or updating entry of %s at %s\n",
            addrStr, clockStr);
        HwmpRouteTable_Print(node, dot11, hwmp);
    }

    return theNode;
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_DeleteEntry
// PURPOSE   : Remove an entry from the route table
// ARGUMENTS : toFree, the routing table entry to delete
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpRouteTable_DeleteEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* toFree)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    int queueNo = toFree->addrSeqInfo.addr.hash(HWMP_ROUTE_HASH_TABLE_SIZE);
    if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
    {
        routeTable->routeDeleteHead = NULL;
        routeTable->routeDeleteTail = NULL;
    }
    else
    {
        routeTable->routeDeleteHead = toFree->deleteNext;
        routeTable->routeDeleteHead->deletePrev = NULL;
    }

    if (routeTable->routeHash[queueNo] == toFree)
    {
        routeTable->routeHash[queueNo] = toFree->hashNext;
        if (routeTable->routeHash[queueNo])
        {
            routeTable->routeHash[queueNo]->hashPrev = NULL;
        }
    }
    else
    {
        toFree->hashPrev->hashNext = toFree->hashNext;

        if (toFree->hashNext)
        {
            toFree->hashNext->hashPrev = toFree->hashPrev;
        }
    }

    if (HWMP_DebugRouteTable)
        {
            char clockStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &toFree->addrSeqInfo.addr);
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("Node %u deleted entry to %s at %s\n", node->nodeId,
                 addrStr, clockStr);
            HwmpRouteTable_Print(node, dot11, hwmp);
        }

    // Before deleting the entry from the routing table,
    // free the precursors list of this route.
    HwmpPrecursorTable_DeleteAll(node, dot11, hwmp, toFree);
    Hwmp_MemoryFree(node, dot11, hwmp, toFree);
    --(routeTable->size);

}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpRouteTable_UpdateLifetime
// PURPOSE   : Update the lifetime field of route to destination.
//             Update hopcount and metric, if applicable.
// ARGUMENTS : destAddr, destination address
//             hopCount, revised hop count, HWMP_HOPCOUNT_MAX for no change
//             metric, revised metric, HWMP_METRIC_MAX for no change
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpRouteTable_UpdateLifetime(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    int hopCount,
    unsigned int metric)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);

    HWMP_RouteEntry* current =
        routeTable->routeHash[destAddr.hash(HWMP_ROUTE_HASH_TABLE_SIZE)];

    // Skip entries with smaller destination address.
    while (current && current->addrSeqInfo.addr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->addrSeqInfo.addr == destAddr)
    {
        clocktype currentTime = node->getNodeTime();

        if (current->isActive == TRUE)
        {
            if (current->lifetime < currentTime + HWMP_ACTIVE_ROUTE_TIMEOUT)
            {
                current->lifetime = currentTime + HWMP_ACTIVE_ROUTE_TIMEOUT;

                HwmpRouteTable_MoveExpiredEntry(node, dot11, hwmp, current);
            }
        }
        else
        {
            current->isActive = TRUE;
            current->lifetime = currentTime + HWMP_ACTIVE_ROUTE_TIMEOUT;

            if (routeTable->routeDeleteHead == routeTable->routeDeleteTail)
            {
                routeTable->routeDeleteHead = NULL;
                routeTable->routeDeleteTail = NULL;
            }
            else if (routeTable->routeDeleteHead == current)
            {
                routeTable->routeDeleteHead = current->deleteNext;
                routeTable->routeDeleteHead->deletePrev = NULL;
            }
            else if (routeTable->routeDeleteTail == current)
            {
                routeTable->routeDeleteTail = current->deletePrev;
                routeTable->routeDeleteTail->deleteNext = NULL;
            }
            else
            {
                current->deleteNext->deletePrev = current->deletePrev;
                current->deletePrev->deleteNext = current->deleteNext;
            }

            if (hopCount != HWMP_HOPCOUNT_MAX)
            {
                current->hopCount = (unsigned)hopCount;
            }
            else
            {
                current->hopCount = current->lastHopCount;
            }

            if (metric != HWMP_METRIC_MAX)
            {
                current->metric = metric;
            }
            else
            {
                current->metric = current->lastMetric;
            }

            current->deleteNext = NULL;
            current->deletePrev = NULL;

            HwmpRouteTable_InsertExpiredEntry(node, dot11, hwmp, current);
        }
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpBlacklistTable_InsertEntry
// PURPOSE   : Add a destination in black list nodes table
//             The blacklist table works when a node fails to send RREP to a
//             particular node. The node marks the next hop towards the
//             destination of the RREP as black-listed and ignores all the
//             RREQs received from any node in its blacklist.
// ARGUMENTS : destAddr, The address to insert
// RETURN    : None
// NOTES     : Currently, the blacklist table is not used.
//-----------------------------------------------------------------------

static
void HwmpBlacklistTable_InsertEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_BlacklistTable* blacklistTable = &(hwmp->blacklistTable);

    HWMP_BlacklistNode* current = blacklistTable->head;
    HWMP_BlacklistNode* previous = NULL;

    HWMP_BlacklistNode* newNode = NULL;

    // Find Insertion point. Insert after all address matches.
    previous = NULL;
    current = blacklistTable->head;

    while (current && current->destAddr < destAddr)
    {
        previous = current;
        current = current->next;
    }

    if (current && current->destAddr == destAddr)
    {
        // The entry is already there in the precursors list.
    }
    else
    {
        // At this point we are adding the node into the blacklist.
        ++(blacklistTable->size);

        newNode =
            (HWMP_BlacklistNode *) MEM_malloc(sizeof(HWMP_BlacklistNode));

        newNode->destAddr = destAddr;
        newNode->next = current;

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "HwmpBlacklistTable_InsertEntry: "
                "Setting timer HwmpBlacklistTimeout");
        }

        DOT11s_TimerInfo timerInfo(destAddr);
        Dot11s_StartTimer(node, dot11, &timerInfo,
            (clocktype) HWMP_BLACKLIST_TIMEOUT,
            MSG_MAC_DOT11s_HwmpBlacklistTimeout);

        if (previous == NULL)
        {
            blacklistTable->head = newNode;
        }
        else
        {
            previous->next = newNode;
        }
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpBlacklistTable_DeleteEntry
// PURPOSE   : Delete an entry from blacklist table
// ARGUMENTS : address, The address to be deleted
// RETURN    : None
// NOTES     : Currently, the blacklist table is not used.
//-----------------------------------------------------------------------

static
void HwmpBlacklistTable_DeleteEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address address)
{
    HWMP_BlacklistTable* blacklistTable = &(hwmp->blacklistTable);

    HWMP_BlacklistNode* toFree = NULL;
    HWMP_BlacklistNode* current = blacklistTable->head;

    if (!current)
    {
        // Nothing to do, no entry in the table
    }
    else if (current->destAddr == address)
    {
        // The first entry in the list is the matching
        toFree = blacklistTable->head;
        blacklistTable->head = toFree->next;
        MEM_free(toFree);
        --(blacklistTable->size);
    }
    else
    {
        // Search for the address and delete it.
        while (current->next && current->next->destAddr < address)
        {
            current = current->next;
        }

        if (current->next && current->next->destAddr == address)
        {
            toFree = current->next;
            current->next = toFree->next;
            MEM_free(toFree);
            --(blacklistTable->size);
        }
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpBlacklistTable_FindEntry
// PURPOSE   : Lookup entry for destination in blacklist
// ARGUMENTS : destAddr, destination address to search
// RETURN    : TRUE, if entry exists
//             FALSE, otherwise
// NOTES     : Currently, the blacklist table is not used.
//-----------------------------------------------------------------------

static
BOOL HwmpBlacklistTable_FindEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_BlacklistTable* blacklistTable = &(hwmp->blacklistTable);

    HWMP_BlacklistNode* current = blacklistTable->head;

    while (current && current->destAddr < destAddr)
    {
        current = current->next;
    }

    if (current && current->destAddr == destAddr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpSentTable_FindEntry
// PURPOSE   : Check if RREQ has been sent to a destination
// ARGUMENTS : destAddr, destination address of the packet
// RETURN    : pointer to the sent node if exists, NULL otherwise
//-----------------------------------------------------------------------

static
HWMP_RreqSentNode* HwmpSentTable_FindEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_RreqSentTable* sentTable = &(hwmp->sentTable);

    HWMP_RreqSentNode* current =
            sentTable->sentHash[destAddr.hash(HWMP_SENT_HASH_TABLE_SIZE)];

    while (current && current->destAddr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->destAddr == destAddr)
    {
        return current;
    }
    else
    {
        return NULL;
    }
}


//-----------------------------------------------------------------------
// FUNCTION  : HwmpSentTable_InsertEntry
// PURPOSE   : Insert an entry into the sent table if RREQ is sent
// ARGUMENTS : destAddr, destination address for which the rreq has been sent
//             ttl, TTL of the rreq
// RETURN    : The node just inserted
//-----------------------------------------------------------------------

static
HWMP_RreqSentNode* HwmpSentTable_InsertEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    int ttl)
{
    HWMP_RreqSentTable* sentTable = &(hwmp->sentTable);

    int queueNo = destAddr.hash(HWMP_SENT_HASH_TABLE_SIZE);

    HWMP_RreqSentNode* current = sentTable->sentHash[queueNo];

    HWMP_RreqSentNode* previous = NULL;

    // Find Insertion point.  Insert after all address matches.
    // This sorts the list in ascending order

    while (current && current->destAddr < destAddr)
    {
        previous = current;
        current = current->hashNext;
    }

    if (current && current->destAddr == destAddr)
    {
        // The entry already exists.
        return current;
    }
    else
    {
        HWMP_RreqSentNode* newNode = (HWMP_RreqSentNode *)
            MEM_malloc(sizeof(HWMP_RreqSentNode));

        newNode->destAddr = destAddr;
        newNode->ttl = ttl;
        newNode->times = 0;
        newNode->hashNext = current;

        (sentTable->size)++;

        if (previous == NULL)
        {
            sentTable->sentHash[queueNo] = newNode;
        }
        else
        {
            previous->hashNext = newNode;
        }
        return newNode;
    }
}

//-----------------------------------------------------------------------
// FUNCTION  : HwmpSentTable_DeleteEntry
// PURPOSE   : Remove an entry from the sent table
// ARGUMENTS : destAddr, address to be deleted from sent table
// RETURN    : None
//-----------------------------------------------------------------------

static
void HwmpSentTable_DeleteEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_RreqSentTable* sentTable = &(hwmp->sentTable);

    int queueNo = destAddr.hash(HWMP_SENT_HASH_TABLE_SIZE);

    HWMP_RreqSentNode* toFree = NULL;

    HWMP_RreqSentNode* current = sentTable->sentHash[queueNo];

    if (!current)
    {
        // Table is empty.
    }
    else if (current->destAddr == destAddr)
    {
        toFree = current;
        sentTable->sentHash[queueNo] = toFree->hashNext;
        MEM_free(toFree);
        --(sentTable->size);
    }
    else
    {
        while (current->hashNext && current->hashNext->destAddr < destAddr)
        {
            current = current->hashNext;
        }

        if (current->hashNext && current->hashNext->destAddr == destAddr)
        {
            toFree = current->hashNext;
            current->hashNext = toFree->hashNext;
            MEM_free(toFree);
            --(sentTable->size);
        }
    }
}

//-----------------------------------------------------------------------
// FUNCTION  : Hwmp_GetSentCountWithMaxTtl
// PURPOSE   : Obtains the number of times the RREQ was sent with
//             TTL = NET_DIAMETER to a given destination
// ARGUMENTS : destAddr, destination address
// RETURN    : int, number of times
//-----------------------------------------------------------------------

static
int HwmpSentTable_GetSentCountWithMaxTtl(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr)
{
    HWMP_RreqSentTable* sentTable = &(hwmp->sentTable);

    HWMP_RreqSentNode* current =
        sentTable->sentHash[destAddr.hash(HWMP_SENT_HASH_TABLE_SIZE)];

    // Skip smaller destinations.
    while (current && current->destAddr < destAddr)
    {
        current = current->hashNext;
    }

    if (current && current->destAddr == destAddr)
    {
        // Got the destination, return the count.
        return current->times;
    }
    else
    {
        // No entry for destination.
        return 0;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpSeenTable_InsertEntry
// PURPOSE   : Insert an entry into the seen table
// ARGUMENTS : sourceAddr, the source address of RREQ packet
//             rreqId, the flooding Id in the RREQ from the source
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpSeenTable_InsertEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address sourceAddr,
    unsigned int rreqId)
{
    HWMP_RreqSeenTable* seenTable = &(hwmp->seenTable);

    // Add to the rear of list and set a timer for expiry.
    // Deletion will be always from the front.
    if (seenTable->size == 0)
    {
        seenTable->rear = (HWMP_RreqSeenNode *)
            MEM_malloc(sizeof(HWMP_RreqSeenNode));
        seenTable->rear->previous = NULL;
        seenTable->front = seenTable->rear;
    }
    else
    {
        seenTable->rear->next = (HWMP_RreqSeenNode *)
            MEM_malloc(sizeof(HWMP_RreqSeenNode));
        seenTable->rear->next->previous = seenTable->rear;
        seenTable->rear = seenTable->rear->next;
    }

    seenTable->rear->sourceAddr = sourceAddr;
    seenTable->rear->rreqId = rreqId;
    seenTable->rear->next = NULL;

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &sourceAddr);
        sprintf(traceStr,
            "Adding to seen table(%d), source: %s, RreqId: %d",
            hwmp->seenTable.size, addrStr, rreqId);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ++(seenTable->size);

    // Keep track of the seen tables max size
    //hwmp->stats->numMaxSeenTable =
    //    MAX((unsigned int) seenTable->size, hwmp->stats->numMaxSeenTable);

    DOT11s_TimerInfo timerInfo;
    Dot11s_StartTimer(node, dot11, &timerInfo,
        (clocktype) HWMP_RREQ_FLOOD_RECORD_TIME,
        MSG_MAC_DOT11s_HwmpSeenTableTimeout);
}

//--------------------------------------------------------------------
// FUNCTION  : HwmpSeenTable_FindEntry
// PURPOSE   : Lookup table of processed broadcast RREQs.
// ARGUMENTS : sourceAddr, source of RREQ
//             rreqId, rreqId id in the received RREQ
// RETURN    : None
//--------------------------------------------------------------------

static
BOOL HwmpSeenTable_FindEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address sourceAddr,
    unsigned int rreqId)
{
    HWMP_RreqSeenTable* seenTable = &(hwmp->seenTable);

    HWMP_RreqSeenNode* current = NULL;

    if (seenTable->size == 0)
    {
        return FALSE;
    }

    // Check if the last found node from table matches.
    if ((seenTable->lastFound ) &&
        (seenTable->lastFound->sourceAddr == sourceAddr) &&
        (seenTable->lastFound->rreqId == rreqId))
    {
        //hwmp->stats->numLastFoundHits++;
        return TRUE;
    }

    // Traverse the list from the rear to check
    // the most recent entries first.
    for (current = seenTable->rear;
         current != NULL;
         current = current->previous)
    {
        if ((current->sourceAddr == sourceAddr) &&
            (current->rreqId == rreqId))
        {
            // Remember the last found node.
            seenTable->lastFound = current;
            return TRUE;
        }
    }

    return FALSE;
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpSeenTable_DeleteEntry
// PURPOSE   : Remove an entry from the seen table, deletion will be always
//             from the front of the table and insertion from the rear.
// ARGUMENTS :
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpSeenTable_DeleteEntry(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    HWMP_RreqSeenTable* seenTable = &(hwmp->seenTable);

    HWMP_RreqSeenNode* toFree = NULL;

    toFree = seenTable->front;
    seenTable->front = toFree->next;

    // Clean up lastfound if it is going to be freed.
    if (seenTable->lastFound == toFree){
        seenTable->lastFound = NULL;
    }

    MEM_free(toFree);
    --(seenTable->size);

    if (seenTable->size == 0)
    {
        seenTable->rear = NULL;
    }
    else
    {
        // Update the previous node link to NULL
        // from front of the list.
        seenTable->front->previous = NULL;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_CalculateNumInactiveRouteByDest
// PURPOSE   : On receiving an RERR message, check destination in the
//             list of the RERR that should be forwarded. Also remove
//             unreachable destination from precursor list of other routes
//             and then disable the route(s).
// ARGUMENTS : rerrData, data of received RERR
//             sourceAddr, previous hop address
//             ifOneUpstream, True if there is only one node to which the
//                  RREQ should be forwarded back.
//             upstreamAddr, if there is only one neighbor to which the RERR
//                  message needs to be forwarded
//             unreachableDestList, list of addresses which should be
//                  included in the forwarded RERR message along with their
//                  sequence number
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_CalculateNumInactiveRouteByDest(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RerrData* rerrData,
    Mac802Address sourceAddr,
    BOOL* ifOneUpstream,
    Mac802Address* upstreamAddr,
    LinkedList* unreachableDestList)
{
    HWMP_AddrSeqInfo* rxDestAddrSeq = NULL;
    BOOL isNumUpstreamUnique = TRUE;
    Mac802Address previousPrecursorNode;

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Received RERR with %u unreachable dests",
            ListGetSize(node, rerrData->destInfoList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    // Traverse the unreachable destination list.
    ListItem* listItem = rerrData->destInfoList->first;
    HWMP_AddrSeqInfo* txDestAddrSeq;

    while (listItem != NULL)
    {
        HWMP_RouteEntry* current = NULL;
        BOOL rtIsValid = FALSE;
        rxDestAddrSeq = (HWMP_AddrSeqInfo*) listItem->data;

        Mac802Address destAddr = rxDestAddrSeq->addr;
        unsigned int seqNo = (unsigned)rxDestAddrSeq->seqNo;

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char destStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(destStr, &destAddr);
            sprintf(traceStr, "dest %s, seqNo %u",
                destStr, seqNo);
        }

        // Check if a valid route entry exists in the routing table
        // for the unreachable destination.

        current = HwmpRouteTable_FindEntry(node, dot11, hwmp,
                      destAddr, &rtIsValid);

        if (rtIsValid)
        {
            // Valid route entry exists.
            if (current->nextHop == sourceAddr)
            {
                // Last forwarding node is the next hop of the route.

                if (current->precursors.size)
                {
                    // Precursors list is not empty
                    Dot11s_Malloc(HWMP_AddrSeqInfo, txDestAddrSeq);

                    if (isNumUpstreamUnique
                        && (current->precursors.size > 1
                            || (ListGetSize(node, unreachableDestList)
                                && (previousPrecursorNode !=
                                    current->precursors.head->destAddr))))
                    {
                        isNumUpstreamUnique = FALSE;
                    }

                    previousPrecursorNode =
                        current->precursors.head->destAddr;

                    current->addrSeqInfo.seqNo = seqNo;
                    txDestAddrSeq->addr = current->addrSeqInfo.addr;
                    txDestAddrSeq->seqNo = seqNo;

                    // Add the destination and destination sequence
                    // to list to be forwarded upstream.
                    ListAppend(node, unreachableDestList, 0, txDestAddrSeq);
                }
                else
                {
                    // The precursors list is empty, no need to include
                    // the destination in the RERR to be forwarded.
                    // Just disable the route entry.

                    current->addrSeqInfo.seqNo = seqNo;
                }

                HwmpRouteTable_DisableRoute(node, dot11, hwmp,
                    current, FALSE);
            }
        }

        listItem = listItem->next;
    }

    *ifOneUpstream = isNumUpstreamUnique;
    *upstreamAddr = previousPrecursorNode;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_CalculateNumInactiveRouteByNextHop
// PURPOSE   : Find number of routes using a given next hop
// ARGUMENTS : nextHop, nextHop that is not reachable
//             numberDestinations, number of routes using the next hop
//             isUniqueUpstream, TRUE if there a single node to send
//                  an RERR
//             upstreamAddr, if single upstream node, its address.
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_CalculateNumInactiveRouteByNextHop(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address nextHop,
    int* numberDestinations,
    BOOL* isUniqueUpstream,
    Mac802Address* upstreamAddr)
{
    HWMP_RoutingTable* routeTable = &(hwmp->routeTable);
    HWMP_RouteEntry* current = NULL;
    *isUniqueUpstream = TRUE;

    // Delete the disabled route from all routes precursor list.
    HwmpRouteTable_DeletePrecursorFromAllEntries(
        node, dot11, hwmp, nextHop);

    for (int i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = routeTable->routeHash[i];
             current != NULL;
             current = current->hashNext)
        {
            if ((current->nextHop == nextHop)
                && (current->isActive == TRUE))
            {
                if (current->precursors.size)
                {
                    // Precursors list is not null.
                    // Need to advertise the failure of this route.
                    (*numberDestinations)++;

                    // Check if there is only one upstream to which
                    // the route error should be sent.
                    if (isUniqueUpstream
                        && (current->precursors.size > 1
                            || ((*numberDestinations > 1)
                                && *upstreamAddr
                                    != current->precursors.head->destAddr)))
                    {
                        *isUniqueUpstream = FALSE;
                    }

                    *upstreamAddr = current->precursors.head->destAddr;
                }
            }
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RelayDataFrame
// PURPOSE   : Forward the data packet to the next hop
// ARGUMENTS : rtEntryToDest, entry in routing table to destinatin
//             frameInfo, data frame message and related info
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RelayDataFrame(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RouteEntry* rtEntryToDest,
    DOT11s_FrameInfo* frameInfo)
{
    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &rtEntryToDest->addrSeqInfo.addr);
        Dot11s_AddrAsDotIP(nextHopStr, &rtEntryToDest->nextHop);
        sprintf(traceStr,
            "Hwmp_RelayDataFrame: Sending packet to dest %s, next hop %s ",
            addrStr, nextHopStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    // Each time a route is used to forward a data packet,
    // its Lifetime field is updated to be no less than
    // the current time plus ACTIVE_ROUTE_TIMEOUT.

    HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
        frameInfo->DA,
        HWMP_HOPCOUNT_MAX, HWMP_METRIC_MAX);

    // Since the route between each source and destination pair
    // are expected to be symmetric, the Lifetime for the previous hop,
    // along the reverse path back to the source, is also updated
    // to be no less than the current time plus ACTIVE_ROUTE_TIMEOUT.

    if (Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->SA) == FALSE
        && frameInfo->fwdControl.GetExMesh() == FALSE)
    {
        HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
            frameInfo->TA,
            1,
            Dot11s_ComputeLinkMetric(node, dot11, frameInfo->TA));

        // Update lifetime of source.
        if (frameInfo->TA != frameInfo->SA)
        {
            HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
                frameInfo->SA,
                HWMP_HOPCOUNT_MAX,      // Does not alter hopcount
                HWMP_METRIC_MAX);       // Does not alter metric
        }

    }

    // Update the lifetime of next hop towards the destination.
    HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
        rtEntryToDest->nextHop,
        1,
        Dot11s_ComputeLinkMetric(node, dot11, rtEntryToDest->nextHop));


    frameInfo->frameType = DOT11_MESH_DATA;
    frameInfo->RA = rtEntryToDest->nextHop;
    frameInfo->TA = dot11->selfAddr;

    MESSAGE_AddHeader(node,
        frameInfo->msg,
        DOT11s_DATA_FRAME_HDR_SIZE,
        TRACE_DOT11);
    Dot11s_SetFieldsInDataFrameHdr(node, dot11,
        MESSAGE_ReturnPacket(frameInfo->msg),
        frameInfo);

    // Enqueue the mesh broadcast for transmission
    Dot11sDataQueue_EnqueuePacket(node, dot11, frameInfo);

}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbr_RelayDataFrame
// PURPOSE   : Forward the data packet using proactive routing
// ARGUMENTS : frameInfo, data frame message and related info
// RETURN    : None
//--------------------------------------------------------------------

static
BOOL HwmpTbr_RelayDataFrame(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo)
{
    ERROR_Assert(hwmp->fwdState == HWMP_TBR_FWD_FORWARDING,
        "HwmpTbr_RelayDataFrame: "
        "Not in forwarding state; cannot send data frame.\n");

    Mac802Address nextHopAddr = INVALID_802ADDRESS;
    BOOL sendDownstream = FALSE;
    DOT11s_FwdItem* fwdItem = NULL;
    DOT11s_ProxyItem* proxyItem = NULL;

    // The frame could be
    // - from local BSS, could go upstream or downstream
    // - from current parent, should go downstream
    // - from downstream, should relay to parent.

    proxyItem = Dot11sProxyList_Lookup(node, dot11, frameInfo->DA);
    if (proxyItem != NULL)
    {
        fwdItem =
            Dot11sFwdList_Lookup(node, dot11, proxyItem->proxyAddr);
        if (fwdItem == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "!! HwmpTbr_RelayDataFrame: "
                    "Proxy Item does not have a corresponding Fwd Item.\n");
            }
            return FALSE;
        }

        sendDownstream = (proxyItem->inMesh) ? TRUE : FALSE;
    }
    else
    {
        sendDownstream = FALSE;
    }

    if (sendDownstream)
    {
        nextHopAddr = fwdItem->nextHopAddr;
    }
    else
    {
        if (hwmp->currentParent != NULL)
        {
            nextHopAddr = hwmp->currentParent->addr;

            // Sanity check
            if (frameInfo->TA == nextHopAddr)
            {
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "!! HwmpTbr_RelayDataFrame: "
                        "Frame from parent should have "
                        "destination in proxy list.\n");
                }
                return FALSE;
            }
        }
        else
        {
            if (hwmp->isRoot)
            {
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "!! HwmpTbr_RelayDataFrame: "
                        "Destination is not registered with Root");
                }

                if (HwmpPktBuffer_Insert(node, dot11, hwmp, frameInfo))
                {
                    // Discover route on behalf of MP/station.
                    DOT11s_FrameInfo newFrameInfo;
                    newFrameInfo.DA = frameInfo->DA;
                    newFrameInfo.SA = dot11->selfAddr;
                    Hwmp_SendTriggeredRreq(node, dot11, hwmp,
                        &newFrameInfo);
                }

                return TRUE;
            }
            else
            {
                MacDot11Trace(node, dot11, NULL,
                    "!! HwmpTbr_RelayDataFrame: "
                    "MP's parent is not valid in forwarding state.");
            }
            return FALSE;
        }
    }

    if (nextHopAddr != INVALID_802ADDRESS)
    {
        frameInfo->frameType = DOT11_MESH_DATA;
        frameInfo->RA = nextHopAddr;
        frameInfo->TA = dot11->selfAddr;
        // No change to source / dest
        // No change to E2E sequence number
        frameInfo->fwdControl.SetUsesTbr(TRUE);

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char destAddrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(destAddrStr, &frameInfo->DA);
            char nextHopAddrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(nextHopAddrStr, &frameInfo->RA);
            sprintf(traceStr,
                "Forwarding TBR frame: dest=%s, nextHop=%s",
                destAddrStr, nextHopAddrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        MESSAGE_AddHeader(node,
            frameInfo->msg,
            DOT11s_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);
        Dot11s_SetFieldsInDataFrameHdr(node, dot11,
            MESSAGE_ReturnPacket(frameInfo->msg),
            frameInfo);

        // Enqueue the mesh frame for transmission
        Dot11sDataQueue_EnqueuePacket(node, dot11, frameInfo);
    }
    else
    {
        ERROR_Assert(
            hwmp->isRoot
            && (proxyItem == NULL
                || proxyItem->inMesh == FALSE),
            "HwmpTbr_RelayDataFrame: "
            "Unexpected setting in proxy item.\n");

        // Assumes portal
        // Pass to upper layer to forward outside mesh
        MAC_HandOffSuccessfullyReceivedPacket(node,
            dot11->myMacData->interfaceIndex,
            frameInfo->msg,
            &frameInfo->SA);
    }

    return TRUE;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRreqFrame
// PURPOSE   : Send RREQ frame
// ARGUMENTS : rreqData, data to create the RREQ frame
//             nextHopAddr, address of next hop, typically broadcast
//             replyTimeout, delay time for reply
//             isRelay, TRUE if RREQ is being relayed
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRreqFrame(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RreqData* rreqData,
    Mac802Address nextHopAddr,
    clocktype replyTimeout,
    BOOL isRelay)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    Dot11s_CreateRreqFrame(node, dot11, rreqData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(
        node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = nextHopAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_RREQ;

    Dot11s_SetFieldsInMgmtFrameHdr(
        node, dot11, txFrame, newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

    // If this is a relay, the seen table entry has
    // already been added in the RREQ receive function.
    if (isRelay == FALSE)
    {
        HwmpSeenTable_InsertEntry(node, dot11, hwmp,
            rreqData->sourceInfo.addr, (unsigned)rreqData->rreqId);
    }

    hwmp->lastBroadcastSent = node->getNodeTime();

    if (replyTimeout > 0)
    {
        DOT11s_TimerInfo timerInfo;
        timerInfo.addr = rreqData->destInfo.addr;
        timerInfo.addr2 = rreqData->sourceInfo.addr;
        Dot11s_StartTimer(node, dot11, &timerInfo,
            (clocktype) replyTimeout,
            MSG_MAC_DOT11s_HwmpRreqReplyTimeout);
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRrepFrame
// PURPOSE   : Send RREP frame.
// ARGUMENTS : rrepData, data to create the RREP frame
//             nextHopAddr, address of next hop
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRrepFrame(
    Node* node,
    MacDataDot11* dot11,
    const HWMP_RrepData* const rrepData,
    Mac802Address nextHopAddr)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    Dot11s_CreateRrepFrame(node, dot11, rrepData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(
        node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;

    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = nextHopAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_RREP;

    Dot11s_SetFieldsInMgmtFrameHdr(
        node, dot11, txFrame, newFrameInfo);

    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
}



//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRerrFrame
// PURPOSE   : Send route error message to other nodes if route is not
//             not available to a destination.
// ARGUMENTS : rerrData, details of the RERR
//             ifOneUpstream, true if there is only one upstream
//             upstreamAddr, address of upstream if only one upstream
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRerrFrame(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const HWMP_RerrData* const rerrData,
    BOOL ifOneUpstream,
    Mac802Address upstreamAddr)
{
    Mac802Address nextHopAddr;
    int interfaceIndex;

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];

        sprintf(traceStr, "Sending RERR to ");

        if (ifOneUpstream)
        {
            Dot11s_AddrAsDotIP(destStr, &upstreamAddr);
            strcat(traceStr, destStr);
        }
        else
        {
            strcat(traceStr, "all neighbors");
        }
        MacDot11Trace(node, dot11, NULL, traceStr);

        sprintf(traceStr, "Dest count = %u",
            ListGetSize(node, rerrData->destInfoList));
        MacDot11Trace(node, dot11, NULL, traceStr);

        HWMP_AddrSeqInfo* destInfo;
        ListItem *item = rerrData->destInfoList->first;
        while (item)
        {
            destInfo = (HWMP_AddrSeqInfo*) item->data;
            Dot11s_AddrAsDotIP(destStr, &destInfo->addr);
            sprintf(traceStr, "Dest %s, seqNo %u",
                destStr, destInfo->seqNo);
            MacDot11Trace(node, dot11, NULL, traceStr);

            item = item->next;
        }
    }

    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    Dot11s_CreateRerrFrame(node, dot11, rerrData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(
        node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_RERR;

    // If one upstream then send a unicast else broadcast the RERR.
    if (ifOneUpstream
        && HwmpRouteTable_GetNextHop(node, dot11, hwmp,
                upstreamAddr, &nextHopAddr, &interfaceIndex))
    {
        newFrameInfo->RA = nextHopAddr;
        newFrameInfo->TA = dot11->selfAddr;

        Dot11s_SetFieldsInMgmtFrameHdr(
            node, dot11, txFrame, newFrameInfo);

        Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
    }
    else
    {
        newFrameInfo->RA = ANY_MAC802;
        newFrameInfo->TA = dot11->selfAddr;

        Dot11s_SetFieldsInMgmtFrameHdr(
            node, dot11, txFrame, newFrameInfo);

        Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRerrForUnreachableDest
// PURPOSE   : Send route error for link failure to a given destination
// ARGUMENTS : destAddr, unreachable destination
//             previousHopAddr, upstream towards the source if
//                  isOneUpstream is true
//             isOneUpstream, true if there is one upstream towards source
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRerrForUnreachableDest(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    Mac802Address previousHopAddr,
    BOOL isOneUpstream)
{
    HWMP_RerrData rerrData;
    ListInit(node, &rerrData.destInfoList);

    HWMP_AddrSeqInfo* addrSeq = NULL;
    BOOL isValidRoute = FALSE;

    HWMP_RouteEntry* rtToDest =
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
        destAddr, &isValidRoute);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &destAddr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "Route broken to %s, send RERR at %s", destStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    Dot11s_Malloc(HWMP_AddrSeqInfo, addrSeq);
    addrSeq->addr = destAddr;
    addrSeq->seqNo = rtToDest ? rtToDest->addrSeqInfo.seqNo : hwmp->seqNo;

    ListAppend(node, rerrData.destInfoList, 0, addrSeq);

    Hwmp_SendRerrFrame(node, dot11, hwmp,
        &rerrData, isOneUpstream, previousHopAddr);

    ListFree(node, rerrData.destInfoList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRerrForLinkFailure
// PURPOSE   : Processing procedure when the node fails to deliver a data
//             packet to a neighbor.
// ARGUMENTS : nextHopAddr, next hop for the destination
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRerrForLinkFailure(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address nextHopAddr)
{
    BOOL isUpstreamUnique = TRUE;
    Mac802Address upstreamAddr;
    int numberRouteDestinations = 0;
    HWMP_AddrSeqInfo* destAddrSeq = NULL;
    HWMP_RouteEntry* current = NULL;

    // Calculate number of destination that are not reachable
    // to the lost link toward this next hop.
    Hwmp_CalculateNumInactiveRouteByNextHop(node, dot11, hwmp,
        nextHopAddr,
        &numberRouteDestinations,
        &isUpstreamUnique,
        &upstreamAddr);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &nextHopAddr);
        sprintf(traceStr, "next hop to destination is %s", destStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
        sprintf(traceStr, "number of destinations using route is %u",
            numberRouteDestinations);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    if (numberRouteDestinations)
    {
        // There are unreachable destinations. Allocate and send a RERR.
        HWMP_RerrData rerrData;
        ListInit(node, &rerrData.destInfoList);

        hwmp->stats->rerrInitiated++;

        // Allocate the destinations in the inactive destination field
        for (int i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
        {
            for (current = (&hwmp->routeTable)->routeHash[i];
                 current != NULL;
                 current = current->hashNext)
            {
                if ((current->nextHop == nextHopAddr)
                    && (current->isActive == TRUE))
                {
                    if (current->precursors.size)
                    {
                        Dot11s_Malloc(HWMP_AddrSeqInfo, destAddrSeq);
                        destAddrSeq->addr = current->addrSeqInfo.addr;
                        destAddrSeq->seqNo = current->addrSeqInfo.seqNo + 1;
                        ListAppend(node, rerrData.destInfoList, 0, destAddrSeq);
                    }
                    if (HWMP_TraceComments)
                    {
                        char traceStr[MAX_STRING_LENGTH];
                        char destStr[MAX_STRING_LENGTH];
                        Dot11s_AddrAsDotIP(
                            destStr, &current->addrSeqInfo.addr);
                        sprintf(traceStr,
                            "disabling route to dest %s", destStr);
                        MacDot11Trace(node, dot11, NULL, traceStr);
                    }
                }
            }
        }

        Hwmp_SendRerrFrame(node, dot11, hwmp, &rerrData,
            isUpstreamUnique, upstreamAddr);

        ListFree(node, rerrData.destInfoList, FALSE);
    }

    for (int i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
    {
        for (current = (&hwmp->routeTable)->routeHash[i];
             current != NULL;
             current = current->hashNext)
        {

            if ((current->nextHop == nextHopAddr) &&
                (current->isActive == TRUE))
            {
                HwmpRouteTable_DisableRoute(node, dot11, hwmp,
                    current, TRUE);
            }
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendTriggeredRreq
// PURPOSE   : Initiate a Route Request packet when no route to
//             destination is known.
// ARGUMENTS : frameInfo, data frame message and related info
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendTriggeredRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo)
{
    // A node floods a RREQ when it determines that it needs a route to
    // a destination and does not have one available. This can happen
    // if the destination is previously unknown to the node, or if a
    // previously valid route to the destination expires or is broken.

    // To prevent unnecessary network-wide floods of RREQs,
    // the source node SHOULD use an expanding ring search
    // technique as an optimization.
    // Sec: 8.4

    int ttl = hwmp->rreqTtlInitial;
    if (hwmp->rreqType == HWMP_RREQ_EXPANDING_RING)
    {
        ttl = Hwmp_GetLastHopCount(hwmp, frameInfo->DA);
    }
    ERROR_Assert(ttl >= 1,
        "Hwmp_SendTriggeredRreq: TTL needs to be greater than 0.\n");
    ERROR_Assert(ttl < 256,
        "Hwmp_SendTriggeredRreq: TTL needs to be less than 256.\n");

    // Increase own sequence number before flooding route request.
    hwmp->seqNo++;

    HWMP_RreqData rreqData;

    rreqData.portalRole = FALSE;
    rreqData.isBroadcast = TRUE;
    rreqData.rrepRequested = FALSE;
    rreqData.staReassocFlag = FALSE;
    rreqData.hopCount = 0;
    rreqData.ttl = (unsigned char) ttl;
    // For a station, use MAP's value
    rreqData.rreqId = Hwmp_GetRreqId(node, dot11, hwmp);
    rreqData.sourceInfo.addr = frameInfo->SA;
    // For a station, use MAP's value
    rreqData.sourceInfo.seqNo = hwmp->seqNo;
    rreqData.lifetime = HWMP_REVERSE_ROUTE_TIMEOUT;
    rreqData.metric = 0;
    rreqData.DO_Flag = hwmp->rreqDoFlag;
    rreqData.RF_Flag = hwmp->rreqRfFlag;
    rreqData.destInfo.addr = frameInfo->DA;
    rreqData.destInfo.seqNo =
        HwmpRouteTable_GetSequenceNumber(node, dot11, hwmp, frameInfo->DA);

    clocktype replyTimeout = 2 * ttl * HWMP_NODE_TRAVERSAL_TIME;

    Hwmp_SendRreqFrame(node, dot11, hwmp,
        &rreqData,
        ANY_MAC802,
        replyTimeout,
        FALSE);                     // isRelay

    HWMP_RreqSentNode* sentNode =
        HwmpSentTable_InsertEntry(node, dot11, hwmp, frameInfo->DA, ttl);

    Hwmp_IncreaseTtl(node, dot11, hwmp, sentNode);
    if (ttl >= HWMP_NET_DIAMETER) {
        sentNode->times++;
    }

    hwmp->stats->rreqBcInitiated++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RetryRreq
// PURPOSE   : Resend triggered RREQ if RREP has not been received
// ARGUMENTS : destAddr, destination address
//             sourceAddr, source address
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RetryRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address destAddr,
    Mac802Address sourceAddr)
{
    int ttl = 0;

    HWMP_RreqSentNode* sentNode =
        HwmpSentTable_FindEntry(node, dot11, hwmp, destAddr);
    ERROR_Assert(sentNode != NULL,
        "Hwmp_RetryRreq: sent node must have destination entry.\n");

    ttl = sentNode->ttl;
    ERROR_Assert(ttl >= 1,
        "Hwmp_RetryRreq: TTL needs to be greater than 0.\n");
    ERROR_Assert(ttl < 256,
        "Hwmp_RetryRreq: TTL needs to be less than 256.\n");

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &destAddr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Retry no %d RREQ to %s at %s",
            sentNode->times, addrStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    HWMP_RreqData rreqData;

    rreqData.portalRole = FALSE;
    rreqData.isBroadcast = TRUE;
    rreqData.rrepRequested = FALSE;
    rreqData.staReassocFlag = FALSE;
    rreqData.hopCount = 0;
    rreqData.ttl = (unsigned char) ttl;
    rreqData.rreqId = Hwmp_GetRreqId(node, dot11, hwmp);
    rreqData.sourceInfo.addr = sourceAddr;
    rreqData.sourceInfo.seqNo = hwmp->seqNo;
    rreqData.lifetime = HWMP_REVERSE_ROUTE_TIMEOUT;
    rreqData.metric = 0;

    rreqData.DO_Flag = hwmp->rreqDoFlag;
    rreqData.RF_Flag = TRUE;
    rreqData.destInfo.addr = destAddr;
    rreqData.destInfo.seqNo =
        HwmpRouteTable_GetSequenceNumber(node, dot11, hwmp, destAddr);

    clocktype replyDelay = 2 * ttl * HWMP_NODE_TRAVERSAL_TIME;
    if (ttl >= HWMP_NET_DIAMETER)
    {
        replyDelay *= (sentNode->times + 1);
    }

    Hwmp_SendRreqFrame(node, dot11, hwmp,
        &rreqData,
        ANY_MAC802,
        replyDelay,
        FALSE);                     // isRelay

    Hwmp_IncreaseTtl(node, dot11, hwmp, sentNode);
    if (ttl >= HWMP_NET_DIAMETER) {
        sentNode->times++;
    }

    hwmp->stats->rreqBcInitiated++;
    hwmp->stats->rreqResent++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RelayRreq
// PURPOSE   : Forward (re-broadcast) the RREQ
// ARGUMENTS : rxFrameInfo, info about received frame
//             rxRreq, received RRREQ packet data
//             destSeqNo, RREQ destination sequence number
//             ttl, time to live of the message
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RelayRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const DOT11s_FrameInfo* const rxFrameInfo,
    const HWMP_RreqData* const rxRreq,
    unsigned int destSeqNo,
    int ttl)
{
    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &rxRreq->destInfo.addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Relaying RREQ to %s at %s",
            addrStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ERROR_Assert(ttl >= 1,
        "Hwmp_RelayRreq: TTL needs to be greater than 0.\n");
    ERROR_Assert(ttl < 256,
        "Hwmp_RelayRreq: TTL needs to be less than 256.\n");

    HWMP_RreqData rreqData;
    rreqData.portalRole = rxRreq->portalRole;
    rreqData.isBroadcast = rxRreq->isBroadcast;
    rreqData.rrepRequested = rxRreq->rrepRequested;
    rreqData.staReassocFlag = rxRreq->staReassocFlag;
    rreqData.hopCount =  (unsigned char)(rxRreq->hopCount + 1);
    rreqData.ttl = (unsigned char) ttl;
    rreqData.rreqId = rxRreq->rreqId;
    rreqData.sourceInfo.addr = rxRreq->sourceInfo.addr;
    rreqData.sourceInfo.seqNo = rxRreq->sourceInfo.seqNo;
    rreqData.lifetime = rxRreq->lifetime;
    rreqData.metric =
        rxRreq->metric
        + Dot11s_ComputeLinkMetric(node, dot11, rxFrameInfo->TA);
    rreqData.DO_Flag = rxRreq->DO_Flag;
    rreqData.RF_Flag = rxRreq->RF_Flag;
    rreqData.destInfo.addr = rxRreq->destInfo.addr;
    rreqData.destInfo.seqNo = destSeqNo;

    Mac802Address nextHopAddr = ANY_MAC802;
    if (rreqData.isBroadcast == FALSE) {
        // Get route entry and send unicast RREQ
        BOOL isValidDest;
        HWMP_RouteEntry* rtToDest =
            HwmpRouteTable_FindEntry(node, dot11, hwmp,
            rreqData.destInfo.addr, &isValidDest);

        if (isValidDest == FALSE)
        {
                hwmp->stats->rerrInitiated++;

                Hwmp_SendRerrForUnreachableDest(node, dot11, hwmp,
                    rreqData.destInfo.addr, rxFrameInfo->TA, TRUE);

                return;
        }
        else
        {
           nextHopAddr = rtToDest->nextHop;
        }
    }

    Hwmp_SendRreqFrame(node, dot11, hwmp,
        &rreqData,
        nextHopAddr,
        0,
        TRUE);

    hwmp->stats->rreqRelayed++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRrepAsDest
// PURPOSE   : Initiate route reply message as destination
// ARGUMENTS : rxRreq, received RRREQ packet data
//             nextHopAddr, The node to which Route reply should be sent
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRrepAsDest(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const HWMP_RreqData* const rxRreq,
    Mac802Address nextHopAddr)
{
    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));

    rrepData.isGratuitous = FALSE;
    rrepData.hopCount = 0;
    rrepData.ttl = HWMP_NET_DIAMETER;
    rrepData.destInfo.addr = rxRreq->destInfo.addr;

    // If the generating node is the destination itself;
    // it MUST increment its own sequence number by one
    // if the sequence number in the RREQ packet is
    // equal to that incremented value.

    if (rxRreq->destInfo.seqNo > hwmp->seqNo)
    {
        hwmp->seqNo = rxRreq->destInfo.seqNo;
    }
    rrepData.destInfo.seqNo = hwmp->seqNo;

    rrepData.lifetime = HWMP_MY_ROUTE_TIMEOUT;
    rrepData.metric = 0;

    HWMP_AddrSeqInfo* sourceInfo;
    Dot11s_Malloc(HWMP_AddrSeqInfo, sourceInfo);
    sourceInfo->addr = rxRreq->sourceInfo.addr;
    sourceInfo->seqNo = rxRreq->sourceInfo.seqNo;
    ListAppend(node, rrepData.sourceInfoList, 0, sourceInfo);

    Hwmp_SendRrepFrame(node, dot11,
        &rrepData,
        nextHopAddr);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char sourceStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(sourceStr, &sourceInfo->addr);
        Dot11s_AddrAsDotIP(nextHopStr, &nextHopAddr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Sending RREP to %s, nextHop %s, at %s",
            sourceStr, nextHopStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, rrepData.sourceInfoList, FALSE);

    hwmp->stats->rrepInitiatedAsDest++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RelayRREP
// PURPOSE   : Forward the RREP packet
// ARGUMENTS : rxRrepData, received RREP data
//             destRouteEntry, pointer to the destination route
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RelayRrep(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RrepData* rxRrepData,
    HWMP_RouteEntry* destRouteEntry)
{
    BOOL existsRtToSource = FALSE;
    HWMP_AddrSeqInfo* rxSourceInfo =
        (HWMP_AddrSeqInfo*)(rxRrepData->sourceInfoList->first->data);
    HWMP_RouteEntry* rtToSource =
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
        rxSourceInfo->addr, &existsRtToSource);

    // If the current node is not the source node as indicated by the Source
    // address in the RREP message AND a forward route has been created
    // or updated as described before, the node consults its route table
    // entry for the source node to determine the next hop for the RREP
    // packet, and then forwards the RREP towards the source with its Hop
    // Count incremented by one. Sec: 8.7

    if (existsRtToSource == FALSE)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! No route to source, not relaying RREP");
        }
        return;
    }

    unsigned int metricToPrevHopAddr =
        Dot11s_ComputeLinkMetric(node, dot11, destRouteEntry->nextHop);

    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));

    rrepData.isGratuitous = rxRrepData->isGratuitous;
    rrepData.hopCount =  (unsigned char)(rxRrepData->hopCount + 1);
    rrepData.ttl =  (unsigned char)(rxRrepData->ttl - 1);
    rrepData.destInfo.addr = rxRrepData->destInfo.addr;
    rrepData.destInfo.seqNo = rxRrepData->destInfo.seqNo;
    rrepData.lifetime = rxRrepData->lifetime;
    rrepData.metric = rxRrepData->metric + metricToPrevHopAddr;

    ListItem* item = rxRrepData->sourceInfoList->first;
    HWMP_AddrSeqInfo* addrInfo;
    HWMP_AddrSeqInfo* rxAddrInfo;

    do
    {
        rxAddrInfo = (HWMP_AddrSeqInfo*) item->data;
        Dot11s_Malloc(HWMP_AddrSeqInfo, addrInfo);
        *addrInfo = *rxAddrInfo;

        ListAppend(node, rrepData.sourceInfoList, 0, addrInfo);

        item = item->next;

    } while (item != NULL);

    Hwmp_SendRrepFrame(node, dot11,
        &rrepData, rtToSource->nextHop);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char sourceStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(sourceStr, &rxSourceInfo->addr);
        Dot11s_AddrAsDotIP(nextHopStr, &rtToSource->nextHop);
        Dot11s_AddrAsDotIP(destStr, &rrepData.destInfo.addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "Relaying RREP to %s, nextHop %s, dest %s, at %s",
            sourceStr, nextHopStr, destStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    // When any node generates or forwards a RREP, the precursor list for
    // the corresponding destination node is updated by adding to it the
    // next hop node to which the RREP is forwarded. Sec: 8.7

    HwmpPrecursorTable_Insert(node, dot11, hwmp,
        destRouteEntry, rtToSource->nextHop);

    HwmpPrecursorTable_Insert(node, dot11, hwmp,
        rtToSource, destRouteEntry->nextHop);

    // The precursor list for the next hop towards the destination
    // is updated to contain the next hop towards the source.
    BOOL existsRtToPrevHop = FALSE;
    HWMP_RouteEntry* rtToPrevHop =
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
        destRouteEntry->nextHop, &existsRtToPrevHop);
    if (existsRtToPrevHop)
    {
        HwmpPrecursorTable_Insert(node, dot11, hwmp,
            rtToPrevHop, rtToSource->nextHop);
    }

    hwmp->stats->rrepForwarded++;

    // Also, at each node the (reverse) route used to forward a RREP has
    // its lifetime changed to current time plus ACTIVE_ROUTE_TIMEOUT.

    HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
        rtToSource->nextHop,
        1,
        Dot11s_ComputeLinkMetric(node, dot11, rtToSource->nextHop));

    ListFree(node, rrepData.sourceInfoList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRrepByIntermediate
// PURPOSE   : Transmit of RREP by an intermediate node that has a
//             valid route to destination.
// ARGUMENTS : rxRreq, received RREQ data
//             lastHopAddress, last hop address in routing table for the
//                  destination
//             rtToDest, route entry to RREQ destination
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRrepByIntermediate(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const HWMP_RreqData* const rxRreq,
    Mac802Address lastHopAddr,
    HWMP_RouteEntry* rtToDest)
{
    if (rtToDest->lifetime <= node->getNodeTime())
    {
        return;
    }

    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));
    rrepData.isGratuitous = FALSE;
    rrepData.destInfo.addr = rxRreq->destInfo.addr;

    // If node generating the RREP is not the destination node, but
    // instead is an intermediate hop along the path from the source to the
    // destination, it copies the last known destination sequence number in
    // the Destination Sequence Number field in the RREP message. Sec: 8.6.2

    rrepData.destInfo.seqNo = rtToDest->addrSeqInfo.seqNo;

    // The Lifetime field of the RREP is calculated by subtracting
    // the current time from the expiration time in its route table entry.
    // Sec: 8.6.2

    rrepData.lifetime = rtToDest->lifetime - node->getNodeTime();

    // If node generating the RREP is not the destination node, but
    // instead is an intermediate hop along the path from the source to the
    // destination, it copies the last known destination sequence number in
    // the Destination Sequence Number field in the RREP message.

    rrepData.hopCount =
         (unsigned char)(rxRreq->hopCount + rtToDest->hopCount);
    rrepData.ttl = HWMP_NET_DIAMETER;

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &rrepData.destInfo.addr);
        sprintf(traceStr,
            "Send RREP by intermediate node "
            "destination=%s, seqNo=%u, hopcount= %u",
            destStr, rrepData.destInfo.seqNo, rrepData.hopCount);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    rrepData.metric = rtToDest->metric;

    HWMP_AddrSeqInfo* sourceInfo;
    Dot11s_Malloc(HWMP_AddrSeqInfo, sourceInfo);
    sourceInfo->addr = rxRreq->sourceInfo.addr;
    sourceInfo->seqNo = rxRreq->sourceInfo.seqNo;
    ListAppend(node, rrepData.sourceInfoList, 0, sourceInfo);

    Hwmp_SendRrepFrame(node, dot11,
        &rrepData,
        lastHopAddr);

    hwmp->stats->rrepInitiatedAsIntermediate++;

    // Also, at each node the (reverse) route used to forward a RREP has
    // its lifetime changed to current time plus ACTIVE_ROUTE_TIMEOUT.

    BOOL isValidSource;
    HWMP_RouteEntry* rtToSource =
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
        lastHopAddr, &isValidSource);
    HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
        lastHopAddr, rtToSource->hopCount, rtToSource->metric);

    ListFree(node, rrepData.sourceInfoList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ParseMeshFrame
// PURPOSE   : Parse a mesh action frame
// ARGUMENTS : frameInfo, frame related information
//             fieldId, mesh field ID
//             ieData, IE data filled in after parsing
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ParseMeshFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo,
    DOT11s_MeshFieldId fieldId,
    void* ieData)
{
    DOT11_FrameHdr* frameHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(frameInfo->msg);
    int rxFrameSize = MESSAGE_ReturnPacketSize(frameInfo->msg);

    unsigned char* rxFrame = (unsigned char *) frameHdr;
    int offset = sizeof(DOT11_FrameHdr);

    offset++;

    unsigned char rxFrameActionFieldId = *(rxFrame + offset);
    ERROR_Assert(rxFrameActionFieldId == fieldId,
        "Hwmp_ParseMeshFrame: Field ID does not match parameter.\n");
    offset++;

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;

    ieHdr = rxFrame + offset;
    Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);

    switch (rxFrameActionFieldId)
    {
        case DOT11_MESH_RREQ:
        {
            ERROR_Assert(ieHdr.id == DOT11_IE_RREQ,
                "Hwmp_ParseMeshFrame: Invalid RREQ IE ID.\n");

            Dot11s_ParseRreqElement(
                node, &element, (HWMP_RreqData*) ieData);

            break;
        }
        case DOT11_MESH_RREP:
        {
            ERROR_Assert(ieHdr.id == DOT11_IE_RREP,
                "Hwmp_ParseMeshFrame: Invalid RREP IE ID.\n");

            Dot11s_ParseRrepElement(
                node, &element, (HWMP_RrepData*) ieData);

            break;
        }
        case DOT11_MESH_RERR:
        {
            ERROR_Assert(ieHdr.id == DOT11_IE_RERR,
                "Hwmp_ParseMeshFrame: Invalid RERR IE ID.\n");

            Dot11s_ParseRerrElement(
                node, &element, (HWMP_RerrData*) ieData);

            break;
        }
        case DOT11_MESH_RANN:
        {
            ERROR_Assert(ieHdr.id == DOT11_IE_RANN,
                "Hwmp_ParseMeshFrame: Invalid RANN IE ID.\n");

            Dot11s_ParseRannElement(
                node, &element, (HWMP_RannData*) ieData);

            break;
        }
        case DOT11_MESH_RREP_ACK:
        default:
        {
            ERROR_ReportError("Hwmp_ParseMeshFrame: "
                "Unexpected field ID.\n");
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReceiveRreq
// PURPOSE   : Processing procedure when an RREQ is received.
// ARGUMENTS : frameInfo, Info related to received RREQ frame
//             interfaceIndex, interface index of received frame
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReceiveRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    int interfaceIndex)
{
    HWMP_RreqData rreqData;
    Hwmp_ParseMeshFrame(node, dot11, frameInfo,
        DOT11_MESH_RREQ, &rreqData);

    hwmp->stats->rreqReceived++;

    Mac802Address prevHopAddr = frameInfo->TA;
    int ttl = rreqData.ttl;

    // Do not process a request if the hop count recorded in it
    // is greater than HWMP net diameter.
    if (rreqData.hopCount > (unsigned int) HWMP_NET_DIAMETER)
    {
        //hwmp->stats->numMaxHopExceed++;

        return;
    }

    BOOL createRt = FALSE;
    BOOL updateRt = FALSE;
    BOOL dropRreq = FALSE;
    HWMP_RouteEntry* rtToSource = NULL;
    BOOL isValidSource = FALSE;
    HWMP_RouteEntry* rtToDest = NULL;
    BOOL replyByIntermediate = FALSE;

    // Check if MP is source of RREQ (itself or stations it is proxying)
    if (Dot11s_IsSelfOrBssStationAddr(
        node, dot11, rreqData.sourceInfo.addr))
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "RREQ from self, ignoring");
        }
        hwmp->stats->rreqDuplicate++;
        return;
    }

    // Check if MP is destination for itself or stations it is proxying
    BOOL isDestination =
        Dot11s_IsSelfOrBssStationAddr(node, dot11, rreqData.destInfo.addr);

    clocktype revRtLifetime;
    if (isDestination)
    {
        revRtLifetime = rreqData.lifetime;
    }
    else
    {
        revRtLifetime = rreqData.lifetime
            - rreqData.hopCount * HWMP_NODE_TRAVERSAL_TIME;
    }

    unsigned int metricToPrevHopAddr =
        Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr);

    if (rreqData.sourceInfo.addr != prevHopAddr)
    {
        HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
            prevHopAddr,
            0,                          // seq no.
            1,                          // hop count
            metricToPrevHopAddr,
            prevHopAddr,                // next hop
            node->getNodeTime() + revRtLifetime,
            TRUE,                       // isActive
            interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(prevHopAddr);
            Dot11s_StartTimer(node, dot11, &timerInfo, revRtLifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }

    rtToSource = HwmpRouteTable_FindEntry(node, dot11, hwmp,
        rreqData.sourceInfo.addr, &isValidSource);

    if (rtToSource == NULL)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Create new route");
        }

        // Need to allocate route entry
        createRt = TRUE;
    }
    else
    {   // Route entry exists
        if (isValidSource == FALSE
            || rtToSource->addrSeqInfo.seqNo == 0)
        {
            updateRt = TRUE;
        }
        else
        {   // Route entry is valid had a non-zero sequence number
            if (rtToSource->addrSeqInfo.seqNo > rreqData.sourceInfo.seqNo)
            {
                dropRreq = TRUE;
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL, "Old seq no.");
                }
            }
            else
            {   // RREQ source seq no or metric is better
                if ((rtToSource->addrSeqInfo.seqNo
                        < rreqData.sourceInfo.seqNo)
                    || (rtToSource->addrSeqInfo.seqNo
                            == rreqData.sourceInfo.seqNo
                        && rtToSource->metric
                            > rreqData.metric + metricToPrevHopAddr))
                {
                    updateRt = TRUE;
                }
                else
                {
                    if (HwmpSeenTable_FindEntry(node, dot11, hwmp,
                        rreqData.sourceInfo.addr,
                        (unsigned)rreqData.rreqId) == TRUE)
                    {
                        hwmp->stats->rreqDuplicate++;

                        dropRreq = TRUE;
                        if (HWMP_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "RREQ seen before");
                        }
                    }
                    else
                    {
                        updateRt = TRUE;
                    }
                }
            }
        }
    }

    if (dropRreq)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Dropping RREQ");
        }
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, frameInfo->msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        return;
    }

    if (frameInfo->RA == ANY_MAC802
        && (HwmpSeenTable_FindEntry(node, dot11, hwmp,
            rreqData.sourceInfo.addr,
            (unsigned)rreqData.rreqId) == FALSE))
    {
        // Insert the source and RREQ ID in seen table
        // to protect processing duplicates.
        HwmpSeenTable_InsertEntry(node, dot11, hwmp,
            rreqData.sourceInfo.addr, (unsigned)rreqData.rreqId);
    }

    if (createRt || updateRt)
    {
        rtToSource =
            HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
                rreqData.sourceInfo.addr,
                (unsigned)rreqData.sourceInfo.seqNo,      // dest seq no
                rreqData.hopCount + 1,
                rreqData.metric + metricToPrevHopAddr,
                prevHopAddr,
                node->getNodeTime() + revRtLifetime,
                TRUE,                           // isActive
                interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(rreqData.sourceInfo.addr);
            Dot11s_StartTimer(node, dot11, &timerInfo, revRtLifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }
    else
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "No new/update of route, dropping");
        }
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, frameInfo->msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        return;
    }

    if (isDestination)
    {
        hwmp->stats->rreqReceivedAsDest++;

        if (rreqData.staReassocFlag == FALSE)
        {
            // The node itself the destination so send back an RREP.

            if (HWMP_TraceComments) {
                MacDot11Trace(node, dot11, NULL,
                    "RREQ destination, sending RREP");
            }

            // Route Reply Generation by the Destination

            // If the generating node is the destination itself, it MUST
            // update its own sequence number to the maximum of its current
            // sequence number and the destination sequence number in the
            // RREQ packet.  The destination node places the value zero in
            // the Hop Count field of the RREP.
            // Sec: 8.6.1

            hwmp->seqNo = MAX(hwmp->seqNo, rreqData.destInfo.seqNo);

            Hwmp_SendRrepAsDest(node, dot11, hwmp,
                &rreqData, prevHopAddr);
        }
        else
        {
            // Handle custom station reassociation indication.

            if (HWMP_TraceComments) {
                MacDot11Trace(node, dot11, NULL,
                    "RREQ destination, station reassociation");
            }

            Dot11s_StationDisassociateEvent(node, dot11,
                rreqData.destInfo.addr);
        }
    }
    else
    {
        // The node is not the destination for the packet.
        // Check whether it should relay or reply.

        BOOL isValidDest = FALSE;
        unsigned int destSeqNo = 0;
        rtToDest = HwmpRouteTable_FindEntry(node, dot11, hwmp,
            rreqData.destInfo.addr, &isValidDest);

        if (rreqData.DO_Flag == FALSE)
        {
            if (rtToDest
                && isValidDest
                && rtToDest->addrSeqInfo.seqNo >= rreqData.destInfo.seqNo)
            {
                replyByIntermediate = TRUE;

                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "RREQ intermediate, will reply");
                }
            }
            else
            {
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "RREQ intermediate, no (fresh) route, will relay");
                }
            }
        }

        if (!replyByIntermediate)
        {
            // Relay the packet only if TTL is not zero
            if (ttl > 1)
            {
                ttl--;

                destSeqNo = (unsigned)rreqData.destInfo.seqNo;
                if (rtToDest)
                {
                    if (rtToDest->addrSeqInfo.seqNo
                        > rreqData.destInfo.seqNo)
                    {
                        if (HWMP_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "Route Dest SeqNo is larger than in RREQ");
                        }
                        destSeqNo = (unsigned)rtToDest->addrSeqInfo.seqNo;
                    }
                }

                if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 1)
                {
                    Hwmp_RelayRreq(node, dot11, hwmp,
                        frameInfo, &rreqData, destSeqNo, ttl);
                }
            }
            else
            {
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "RREQ intermediate, TTL expired");
                }

                hwmp->stats->rreqTtlExpired++;

                return;
            }
        }
        else
        {   // Reply by intermediate.

            // Furthermore, the intermediate
            // node puts the next hop towards the destination in the
            // precursor list for the reverse route entry -- i.e.,
            // the entry for the Source address field of the RREQ
            // message data. Sec: 8.6.2
            HwmpPrecursorTable_Insert(node, dot11, hwmp,
                rtToSource, rtToDest->nextHop);

            // Do not send a reply as intermediate node if
            // summation of hop count to source and hop count
            // to destination exceeds HWMP net diameter.
            if ((rreqData.hopCount + rtToDest->hopCount)
                > (unsigned int) HWMP_NET_DIAMETER)
            {
                //hwmp->stats->numMaxHopExceed++;

                return;
            }

            // When the intermediate node updates its route table for
            // the source of the RREQ, it puts the last hop node into
            // the precursor list for the forward path route entry --
            // i.e. the entry for the Destination Address.

            HwmpPrecursorTable_Insert(node, dot11, hwmp,
                rtToDest, prevHopAddr);

            Hwmp_SendRrepByIntermediate(node, dot11, hwmp,
                &rreqData, prevHopAddr, rtToDest);

            if (rreqData.RF_Flag == TRUE)
            {
                rreqData.DO_Flag = TRUE;
                rreqData.RF_Flag = FALSE;
                rreqData.lifetime = revRtLifetime;

                destSeqNo = (unsigned)rreqData.destInfo.seqNo;

                if (rtToDest->addrSeqInfo.seqNo
                    > rreqData.destInfo.seqNo)
                {
                    if (HWMP_TraceComments)
                    {
                        MacDot11Trace(node, dot11, NULL,
                            "Route Dest SeqNo is larger than in RREQ");
                    }
                    destSeqNo = (unsigned)rtToDest->addrSeqInfo.seqNo;
                }

                Hwmp_RelayRreq(node, dot11, hwmp,
                    frameInfo, &rreqData,
                    destSeqNo, ttl);
            }
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RelayTbrGratuitousRrep
// PURPOSE   : Relay of gratuitous RREP by intermediate node to root portal.
// ARGUMENTS : frameInfo, Info related to received RREQ frame
//             rxRrepData, Values of received RREP
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RelayTbrGratuitousRrep(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    HWMP_RrepData* rxRrepData)
{
    if (hwmp->currentParent == NULL)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! No parent to root, not relaying gratuitious RREP");
        }
        return;
    }

    HWMP_AddrSeqInfo* destInfo =
        (HWMP_AddrSeqInfo*) (rxRrepData->sourceInfoList->first->data);

    unsigned int metricToPrevHopAddr =
        Dot11s_ComputeLinkMetric(node, dot11, frameInfo->TA);

    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));

    rrepData.isGratuitous = rxRrepData->isGratuitous;
    rrepData.hopCount = (unsigned char)(rxRrepData->hopCount + 1);
    rrepData.ttl = (unsigned char)(rxRrepData->ttl - 1);
    rrepData.destInfo.addr = rxRrepData->destInfo.addr;
    rrepData.destInfo.seqNo = rxRrepData->destInfo.seqNo;
    rrepData.lifetime = rxRrepData->lifetime;
    rrepData.metric = rxRrepData->metric + metricToPrevHopAddr;

    // Copy source list to relayed RREP
    ListItem* item = rxRrepData->sourceInfoList->first;
    HWMP_AddrSeqInfo* addrInfo;
    HWMP_AddrSeqInfo* rxAddrInfo;

    do
    {
        rxAddrInfo = (HWMP_AddrSeqInfo*) item->data;
        Dot11s_Malloc(HWMP_AddrSeqInfo, addrInfo);
        *addrInfo = *rxAddrInfo;

        ListAppend(node, rrepData.sourceInfoList, 0, addrInfo);

        item = item->next;

    } while (item != NULL);

    Hwmp_SendRrepFrame(node, dot11,
        &rrepData, hwmp->currentParent->addr);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        char sourceStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &rrepData.destInfo.addr);
        Dot11s_AddrAsDotIP(nextHopStr, &hwmp->currentParent->addr);
        Dot11s_AddrAsDotIP(sourceStr, &destInfo->addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "Relaying gratuious RREP to %s, nextHop %s, from %s, at %s",
            destStr, nextHopStr, sourceStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    // The precursor list for the next hop towards the destination
    // is updated to contain the next hop towards the source.
    BOOL existsRtToPrevHop = FALSE;
    HWMP_RouteEntry* rtToPrevHop =
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
            frameInfo->TA, &existsRtToPrevHop);
    if (existsRtToPrevHop)
    {
        HwmpPrecursorTable_Insert(node, dot11, hwmp,
            rtToPrevHop,
            hwmp->currentParent->addr);
    }

    // Also, at each node the (reverse) route used to forward a RREP has
    // its lifetime changed to current time plus ACTIVE_ROUTE_TIMEOUT.

    HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
        hwmp->currentParent->addr,
        1,
        Dot11s_ComputeLinkMetric(node, dot11, hwmp->currentParent->addr));

    ListFree(node, rrepData.sourceInfoList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReceiveTbrGratuitousRrep
// PURPOSE   : Relay of gratuitous RREP by intermediate node to root portal.
// ARGUMENTS : frameInfo, Info related to received RREQ frame
//             rxRrepData, Values of received RREP
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReceiveTbrGratuitousRrep(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    HWMP_RrepData* rrepData,
    int interfaceIndex)
{
    ERROR_Assert(rrepData->isGratuitous == TRUE,
        "Hwmp_ReceiveTbrGratuitousRrep: "
        "Registration RREP does not have G flag set.\n");

    Mac802Address prevHopAddr = frameInfo->TA;
    HWMP_AddrSeqInfo* destInfo = &(rrepData->destInfo);

    ListItem* listItem = rrepData->sourceInfoList->first;
    HWMP_AddrSeqInfo* sourceInfo = (HWMP_AddrSeqInfo*) listItem->data;

    // When a node receives a RREP message, it first increments the hop
    // count value in the RREP by one, to account for the new hop through
    // the intermediate node. Sec: 8.7
    int hopCount = rrepData->hopCount + 1;

    unsigned int metric =
        rrepData->metric
        + Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr);

    clocktype lifetime = rrepData->lifetime;

    HWMP_RouteEntry* prevRtPtr = NULL;
    HWMP_RouteEntry* newRtPtr = NULL;
    HWMP_RouteEntry* rtToDest = NULL;

    BOOL newRtAdded = FALSE;
    BOOL routeUpdated = FALSE;
    BOOL rtIsValid = FALSE;

    // Don't process a reply packet if the hop count in the packet is
    // greater than hwmp net diameter.

    if (hopCount > HWMP_NET_DIAMETER)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Rrep hopcount exceeds net diameter");
        }

        //hwmp->stats->numMaxHopExceed++;

        return;
    }

    if (lifetime == 0)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Rrep lifetime is zero");
        }
        return;
    }

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char sourceStr[MAX_STRING_LENGTH];
        char prevHopStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(sourceStr, &sourceInfo->addr);
        Dot11s_AddrAsDotIP(prevHopStr, &prevHopAddr);
        Dot11s_AddrAsDotIP(destStr, &destInfo->addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "Registration gratuitous RREP from %s, "
            "prevHopAddr %s, to %s, at %s",
            sourceStr, prevHopStr, destStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    hwmp->stats->rrepGratReceived++;

    if (sourceInfo->addr != prevHopAddr)
    {
        HWMP_RouteEntry* ptrToPrevHop = NULL;

        ptrToPrevHop =
            HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
                prevHopAddr,
                0,                          // seq no.
                1,                          // hop count
                Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr),
                prevHopAddr,
                node->getNodeTime() + lifetime,
                TRUE,                       // isActive
                interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(prevHopAddr);
            Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }

    prevRtPtr = HwmpRouteTable_FindEntry(node, dot11, hwmp,
        sourceInfo->addr, &rtIsValid);

    rtToDest = prevRtPtr;

    // The forward route for this destination is created or updated only if
    // (i) the Destination Sequence Number in the RREP is greater than the
    //     node's copy of the destination sequence number, or
    // (ii) the sequence numbers are the same, but the route is no longer
    //      active or the incremented Hop Count in RREP is smaller than
    //      the hop count in route table entry. Sec: 8.7

    if (!prevRtPtr ||
        (prevRtPtr->addrSeqInfo.seqNo < sourceInfo->seqNo) ||
        ((prevRtPtr->addrSeqInfo.seqNo == sourceInfo->seqNo) &&
        (!rtIsValid || prevRtPtr->metric > metric)))
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Adding/Updating route");
        }

        // Add the forward route entry in the routing table
        newRtPtr =
            HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
                sourceInfo->addr,
                (unsigned)sourceInfo->seqNo,
                hopCount,
                metric,
                prevHopAddr,
                node->getNodeTime() + lifetime,
                TRUE,
                interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(sourceInfo->addr);
            Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }

        //hwmp->stats->numRoutes++;
        //hwmp->stats->numHops += rrepData->hopCount + 1;

        if (!prevRtPtr)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "First RREP received");
            }

            newRtAdded = TRUE;
            rtToDest = newRtPtr;
        }
        else
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL, "Update route");
            }

            routeUpdated = TRUE;
            rtToDest = prevRtPtr;
        }
    }

    // Add registration entry of source.
    Dot11sProxyList_Insert(node, dot11,
        sourceInfo->addr, TRUE, FALSE, sourceInfo->addr);

    // Add forwarding entry.
    Dot11sFwdList_Insert(node, dot11,
        sourceInfo->addr, prevHopAddr, DOT11s_FWD_IN_MESH_NEXT_HOP);

    Dot11sProxyList_DeleteItems(node, dot11, sourceInfo->addr);

    HWMP_AddrSeqInfo* staInfo;
    listItem = listItem->next;
    while (listItem != NULL)
    {
        staInfo = (HWMP_AddrSeqInfo*) listItem->data;

        // Add registration entries to the proxy list.
        Dot11sProxyList_Insert(node, dot11,
            staInfo->addr, TRUE, TRUE, sourceInfo->addr);

        // Send any buffered frames.
        DOT11s_FrameInfo* newFrameInfo = NULL;
        Mac802Address pktPrevHopAddr;

        newFrameInfo = HwmpPktBuffer_GetPacket(node, dot11, hwmp,
            sourceInfo->addr, &pktPrevHopAddr);

        // Send any buffered packets to the destination
        while (newFrameInfo)
        {
            hwmp->stats->dataInitiated++;

            Hwmp_RelayDataFrame(node, dot11, hwmp,
                rtToDest, newFrameInfo);

            newFrameInfo = HwmpPktBuffer_GetPacket(node, dot11, hwmp,
                sourceInfo->addr, &pktPrevHopAddr);
        }

        listItem = listItem->next;

    };

    // Check if the MP is source, or is proxying a station.
    BOOL isSource =
        Dot11s_IsSelfOrBssStationAddr(node, dot11, destInfo->addr);

    if (isSource)
    {
        ERROR_Assert(hwmp->isRoot,
            "Hwmp_ReceiveTbrGratuitousRrep: "
            "Destination is not a root.\n");

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Gratuitous RREQ destination. Dropping RREP");
        }
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, frameInfo->msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
    }
    else
    {   // Intermediate node of the route

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Processing gratuitous RREP as intermediate");
        }

        Hwmp_RelayTbrGratuitousRrep(node, dot11, hwmp,
            frameInfo, rrepData);

        hwmp->stats->rrepGratForwarded++;

    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReceiveRrep
// PURPOSE   : Processing procedure when RREP is received
// ARGUMENTS : frameInfo, Info related to received RREQ frame
//             interfaceIndex, the interface at which reply has been
//                  received
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReceiveRrep(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    int interfaceIndex)
{
    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));
    Hwmp_ParseMeshFrame(node, dot11, frameInfo,
        DOT11_MESH_RREP, &rrepData);

    // Process registration gratuitous RREPs.
    if (rrepData.isGratuitous)
    {
        Hwmp_ReceiveTbrGratuitousRrep(node, dot11, hwmp,
            frameInfo, &rrepData, interfaceIndex);

        ListFree(node, rrepData.sourceInfoList, FALSE);
        return;
    }

    hwmp->stats->rrepReceived++;

    Mac802Address prevHopAddr = frameInfo->TA;

    // When a node receives a RREP message, it first increments the hop
    // count value in the RREP by one, to account for the new hop through
    // the intermediate node. Sec: 8.7
    int hopCount = rrepData.hopCount + 1;

    unsigned int metric =
        rrepData.metric
        + Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr);

    clocktype lifetime = rrepData.lifetime;

    HWMP_RouteEntry* prevRtPtr = NULL;
    HWMP_RouteEntry* newRtPtr = NULL;
    HWMP_RouteEntry* rtToDest = NULL;

    BOOL newRtAdded = FALSE;
    BOOL routeUpdated = FALSE;
    BOOL rtIsValid = FALSE;

    // Do not process a reply packet if the hop count in the packet is
    // greater than hwmp net diameter.

    if (hopCount > HWMP_NET_DIAMETER)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! RREP hopcount exceeds net diameter");
        }

        //hwmp->stats->numMaxHopExceed++;

        ListFree(node, rrepData.sourceInfoList, FALSE);
        return;
    }

    if (lifetime == 0)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! RREP lifetime is zero");
        }

        ListFree(node, rrepData.sourceInfoList, FALSE);
        return;
    }

    HWMP_AddrSeqInfo* sourceInfo =
        (HWMP_AddrSeqInfo*) (rrepData.sourceInfoList->first->data);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char prevHopStr[MAX_STRING_LENGTH];
        char sourceStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &rrepData.destInfo.addr);
        Dot11s_AddrAsDotIP(prevHopStr, &prevHopAddr);
        Dot11s_AddrAsDotIP(sourceStr, &sourceInfo->addr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "RREP from dest %s, prevHopAddr %s, to source %s, at %s",
            destStr, prevHopStr, sourceStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    if (rrepData.destInfo.addr != prevHopAddr)
    {
        HWMP_RouteEntry* ptrToPrevHop = NULL;

        ptrToPrevHop =
            HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
                prevHopAddr,
                0,                          // seq no.
                1,                          // hop count
                Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr),
                prevHopAddr,
                node->getNodeTime() + lifetime,
                TRUE,                       // isActive
                interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(prevHopAddr);
            Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }

    prevRtPtr = HwmpRouteTable_FindEntry(node, dot11, hwmp,
        rrepData.destInfo.addr, &rtIsValid);

    rtToDest = prevRtPtr;

    // The forward route for this destination is created or updated only if
    // (i) the Destination Sequence Number in the RREP is greater than the
    //     node's copy of the destination sequence number, or
    // (ii) the sequence numbers are the same, but the route is no longer
    //      active or the incremented Hop Count in RREP is smaller than
    //      the hop count in route table entry. Sec: 8.7

    if (!prevRtPtr ||
        (prevRtPtr->addrSeqInfo.seqNo < rrepData.destInfo.seqNo) ||
        (prevRtPtr->nextHop != prevHopAddr) ||
        ((prevRtPtr->addrSeqInfo.seqNo == rrepData.destInfo.seqNo) &&
        (!rtIsValid || prevRtPtr->metric > metric)))
    {

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Adding/Updating route");
        }

        // Add the forward route entry in the routing table
        newRtPtr =
            HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
                rrepData.destInfo.addr,
                (unsigned)rrepData.destInfo.seqNo,
                hopCount,
                metric,
                prevHopAddr,
                node->getNodeTime() + lifetime,
                TRUE,
               interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(rrepData.destInfo.addr);
            Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }

        //hwmp->stats->numRoutes++;
        //hwmp->stats->numHops += rrepData.hopCount + 1;

        if (!prevRtPtr)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "First RREP received");
            }

            newRtAdded = TRUE;
            rtToDest = newRtPtr;
        }
        else
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Contains better route compared to known one");
            }

            routeUpdated = TRUE;
            rtToDest = prevRtPtr;
        }
    }

    // Check if the MP is source, or is proxying a source station.
    BOOL isSource =
        Dot11s_IsSelfOrBssStationAddr(node, dot11, sourceInfo->addr);

    if (isSource)
    {
        if (HWMP_TraceComments)
        {
            if (dot11->selfAddr == sourceInfo->addr)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Processing my RREP");
            }
            else
            {
                MacDot11Trace(node, dot11, NULL,
                    "Processing my STA's RREP");
            }
        }

        hwmp->stats->rrepReceivedAsSource++;

        // If RREP is from root, call TBR state machine.
        if (rrepData.destInfo.addr
            == hwmp->rootItem.rannData.rootAddr)
        {
            hwmp->tbrStateData.event = HWMP_TBR_S_EVENT_RREP_RECEIVED;
            hwmp->tbrStateData.frameInfo = frameInfo;
            hwmp->tbrStateData.rrepData = &rrepData;
            hwmp->tbrStateData.interfaceIndex = interfaceIndex;
            hwmp->tbrStateFn(node, dot11, hwmp);
        }

        if (routeUpdated || newRtAdded)
        {
            DOT11s_FrameInfo* newFrameInfo = NULL;

            HwmpSentTable_DeleteEntry(node, dot11, hwmp,
                rrepData.destInfo.addr);

            newFrameInfo = HwmpPktBuffer_GetPacket(node, dot11, hwmp,
                rrepData.destInfo.addr, &prevHopAddr);

            // Send any buffered packets to the destination
            while (newFrameInfo)
            {
                hwmp->stats->dataInitiated++;

                Hwmp_RelayDataFrame(node, dot11, hwmp,
                    rtToDest, newFrameInfo);

                newFrameInfo = HwmpPktBuffer_GetPacket(node, dot11, hwmp,
                    rrepData.destInfo.addr, &prevHopAddr);
            }
        }
        else
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "No new route or update. Dropping RREP");
            }
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, frameInfo->msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        }
    } // if source
    else
    {   // Intermediate node of the route

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Processing RREP as intermediate");
        }

        // Forward the packet to the upstream of the route
        Hwmp_RelayRrep(node, dot11, hwmp, &rrepData, rtToDest);
    }

    ListFree(node, rrepData.sourceInfoList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION   : Hwmp_ReceiveRerr
// PURPOSE    : Processing procedure when RERR is received
// ARGUMENTS  : frameInfo,  received frame information
//              interfaceIndex, interface at which message was received
// RETURN     : None
//--------------------------------------------------------------------

static
void Hwmp_ReceiveRerr(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    int interfaceIndex)
{
    HWMP_RerrData rxRerrData;
    ListInit(node, &rxRerrData.destInfoList);
    Hwmp_ParseMeshFrame(node, dot11, frameInfo,
        DOT11_MESH_RERR, &rxRerrData);

    LinkedList* unreachableDestList;
    ListInit(node, &unreachableDestList);

    Mac802Address sourceAddr = frameInfo->TA;
    BOOL ifOneUpstream = TRUE;
    Mac802Address upstreamAddr;
    HWMP_RouteEntry* ptrToPrevHop;

    // When a node receives an HWMP control packet from a neighbor, it
    // checks its route table for an entry for that neighbor.  In the
    // event that there is no corresponding entry for that neighbor, an
    // entry is created.  The sequence number is either determined from
    // the information contained in the control packet (i.e.,
    // the neighbor is the source of a RREQ), or else it is initialized
    // to zero if the sequence number for that node can not be
    // determined. sec 6.2 draft 10

    ptrToPrevHop =
        HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
            sourceAddr,
            0,                   // seq no
            1,                   // hop count
            Dot11s_ComputeLinkMetric(node, dot11, sourceAddr),
            sourceAddr,
            node->getNodeTime() + HWMP_ACTIVE_ROUTE_TIMEOUT,
            TRUE,                // is active
            interfaceIndex);

    if (hwmp->isExpireTimerSet == FALSE)
    {
        hwmp->isExpireTimerSet = TRUE;

        DOT11s_TimerInfo timerInfo(sourceAddr);
        Dot11s_StartTimer(node, dot11, &timerInfo,
            HWMP_ACTIVE_ROUTE_TIMEOUT,
            MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
    }

    hwmp->stats->rerrReceived++;

    // Check destinations in the RERR with non-empty precursors list;
    // those entries should be included in the rerr that is forwarded.
    Hwmp_CalculateNumInactiveRouteByDest(node, dot11, hwmp,
        &rxRerrData, sourceAddr, &ifOneUpstream, &upstreamAddr,
        unreachableDestList);

    if (ListGetSize(node, unreachableDestList) > 0)
    {
        HWMP_RerrData txRerrData;
        ListInit(node, &txRerrData.destInfoList);
        HWMP_AddrSeqInfo* destAddrSeq;
        HWMP_AddrSeqInfo* txAddrSeq = NULL;
        ListItem* txListItem = unreachableDestList->first;

        hwmp->stats->rerrForwarded++;

        while (txListItem != NULL)
        {
            destAddrSeq = (HWMP_AddrSeqInfo*) txListItem->data;
            Dot11s_Malloc(HWMP_AddrSeqInfo, txAddrSeq);
            txAddrSeq->addr = destAddrSeq->addr;
            txAddrSeq->seqNo = destAddrSeq->seqNo;
            ListAppend(node, txRerrData.destInfoList, 0, txAddrSeq);

            Dot11s_DeletePacketsToNode(node, dot11,
                sourceAddr, txAddrSeq->addr);

            txListItem = txListItem->next;
        }

        // RERR packet allocation is complete; send out the packet.
        Hwmp_SendRerrFrame(node, dot11, hwmp,
            &txRerrData, ifOneUpstream, upstreamAddr);

        ListFree(node, txRerrData.destInfoList, FALSE);

    }

    ListFree(node, unreachableDestList, FALSE);

    ListFree(node, rxRerrData.destInfoList, FALSE);

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReceiveData
// PURPOSE   : Processing procedure when data is received. This node is
//             either intermediate hop or destination of the data.
// ARGUMENTS : frameInfo, Info related to received mesh data frame
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReceiveData(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo)
{
    // Check if the node is the final destination
    if (Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->DA))
    {
        hwmp->stats->dataReceived++;

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Packet for self/BSS");
        }

        BOOL isValid;
        HwmpRouteTable_FindEntry(node, dot11, hwmp,
            frameInfo->SA, &isValid);
        BOOL initiateRreq = FALSE;

        if (isValid == FALSE
            && frameInfo->fwdControl.GetUsesTbr())
        {
            // When source within mesh has arrived using
            // TBR, initiate an RREQ to SA.
            if (!HwmpSentTable_FindEntry(node, dot11, hwmp,
                    frameInfo->SA))
            {
                if (HWMP_TraceComments) {
                    MacDot11Trace(node, dot11, NULL,
                        "Frame from inMesh received via TBR, "
                        "sending RREQ.");
                }

                // Create frame info with appropriate SA/DA.
                DOT11s_FrameInfo tempFrameInfo;
                tempFrameInfo.DA = frameInfo->SA;
                tempFrameInfo.SA = frameInfo->DA;
                // RA/TA is set in called function
                Hwmp_SendTriggeredRreq(node, dot11, hwmp, &tempFrameInfo);

                initiateRreq = TRUE;
            }
        }

        HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
            frameInfo->TA,
            1,
            Dot11s_ComputeLinkMetric(node, dot11, frameInfo->TA));

        if (frameInfo->SA != frameInfo->TA
            && initiateRreq == FALSE)
        {
            HwmpRouteTable_UpdateLifetime(node, dot11, hwmp,
                frameInfo->SA,
                HWMP_HOPCOUNT_MAX,      // Does not change hopcount
                HWMP_METRIC_MAX);       // Does not change metric
        }
    }
    else
    {
        // Need to relay data to next hop
        HWMP_RouteEntry* rtToDest = NULL;
        BOOL isValidRoute = FALSE;

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Not my packet, need to route");
        }

        // The node is an intermediate node of the route.
        // Relay the packet to the next hop.
        rtToDest = HwmpRouteTable_FindEntry(node, dot11, hwmp,
                        frameInfo->DA, &isValidRoute);

        if (isValidRoute)
        {
            // There is a valid route towards the destination.
            // Update lifetime for previous hop, destination and
            // source

            hwmp->stats->dataForwarded++;

            Hwmp_RelayDataFrame(node, dot11, hwmp,
                rtToDest, frameInfo);

            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Route exists, queing packet for Tx.");
            }
        }
        else if (hwmp->fwdState == HWMP_TBR_FWD_FORWARDING
                && HwmpTbr_RelayDataFrame(node, dot11, hwmp, frameInfo))
        {
            hwmp->stats->dataForwarded++;
        }
        else
        {
            // There is no valid route for the destination but RREQ has been
            // sent for the destination. Buffer the packet.

            if (HwmpSentTable_FindEntry(node, dot11, hwmp, frameInfo->DA))
            {
                // Data packets waiting for a route (i.e., waiting for a
                // RREP after RREQ has been sent) SHOULD be buffered.
                // The buffering SHOULD be FIFO. Sec: 8.3

                HwmpPktBuffer_Insert(node, dot11, hwmp, frameInfo);
            }
            else
            {
                hwmp->stats->rerrInitiated++;

                Hwmp_SendRerrForUnreachableDest(node, dot11, hwmp,
                    frameInfo->DA, frameInfo->TA, TRUE);

                hwmp->stats->dataDroppedForNoRoute++;

                Dot11s_MemFreeFrameInfo(node, &frameInfo, TRUE);
            }
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RootReceiveRannError
// PURPOSE   : Generate an error if root portal receives RANN
// ARGUMENTS : state, Mesh Point Tree state
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_RootReceiveRannError(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_TbrState state)
{
    char errorStr[MAX_STRING_LENGTH];
    sprintf(errorStr, "Hwmp_RootReceiveRann: "
        "Root receives a Root Announcement.\n"
        "Multiple roots are not currently supported.\n"
        "Root Mesh Point state is %d.\n",
        state);
    ERROR_ReportError(errorStr);
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbrParentList_Lookup
// PURPOSE   : Search for a parent in the Parent list
// ARGUMENTS : parentAddr, address of parent to lookup
// RETURN    : Parent list item or NULL
//--------------------------------------------------------------------

static
HWMP_TbrParentItem* HwmpTbrParentList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address parentAddr)
{
    LinkedList* parentList = hwmp->parentList;

    ListItem* listItem = parentList->first;
    HWMP_TbrParentItem* parentItem = NULL;
    BOOL isFound = FALSE;

    while (listItem != NULL)
    {
        parentItem = (HWMP_TbrParentItem*) listItem->data;
        if (parentItem->addr == parentAddr)
        {
            isFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isFound ? parentItem : NULL);
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbrParentList_Delete
// PURPOSE   : Delete an item from TBR Parent list
// ARGUMENTS : parentAddr, address of parent to lookup
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpTbrParentList_Delete(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address parentAddr)
{
    LinkedList* parentList = hwmp->parentList;

    ListItem* listItem = parentList->first;
    HWMP_TbrParentItem* parentItem = NULL;

    while (listItem != NULL)
    {
        parentItem = (HWMP_TbrParentItem*) listItem->data;
        if (parentItem->addr == parentAddr)
        {
            ListDelete(node, parentList, listItem, FALSE);
            break;
        }
        listItem = listItem->next;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbrParentList_Finalize
// PURPOSE   : Clear parent list and release memory
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpTbrParentList_Finalize(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    LinkedList* parentList = hwmp->parentList;

    if (parentList == NULL)
    {
        return;
    }

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "HwmpTbrParentList_Finalize: "
            "Freed %d items",
            ListGetSize(node, parentList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, parentList, FALSE);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRannFrame
// PURPOSE   : Build and send a root announcement
// ARGUMENTS : rannData, RANN details
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendRannFrame(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RannData* rannData)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    Dot11s_CreateRannFrame(node, dot11,
        rannData,
        &fullFrame,
        &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(
        node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = ANY_MAC802;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_RANN;

    Dot11s_SetFieldsInMgmtFrameHdr(
        node, dot11, txFrame, newFrameInfo);

    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendRann
// PURPOSE   : Send a root announcement
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_SendRann(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    ERROR_Assert(hwmp->isRoot,
        "Hwmp_SendRann: Single root portal currently supported.\n");

    HWMP_RootItem* rootItem;
    HWMP_RannData* rootItemRann;

    rootItem = &(hwmp->rootItem);
    rootItemRann = &(hwmp->rootItem.rannData);

    // Increase sequence number.
    hwmp->seqNo++;

    HWMP_RannData rannData;

    rannData = *rootItemRann;
    rannData.rootSeqNo = hwmp->seqNo;

    if (HWMP_TraceComments)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Sending Rann at %s", clockStr);

        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    Hwmp_SendRannFrame(node, dot11, hwmp, &rannData);

    rootItem->lastRannTime = node->getNodeTime();

    hwmp->stats->rannInitiated++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReceiveRann
// PURPOSE   : Receive a root announcement
// ARGUMENTS : frameInfo, Info related to received RANN frame
//             interfaceIndex, interface on which frame was received
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_ReceiveRann(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    int interfaceIndex)
{
    HWMP_RannData rannData;
    Hwmp_ParseMeshFrame(node, dot11, frameInfo,
        DOT11_MESH_RANN, &rannData);

    Mac802Address prevHopAddr = frameInfo->TA;

    if (rannData.rootAddr == dot11->selfAddr)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Own RANN, dropping.");
        }
        hwmp->stats->rannDuplicate++;

        return;
    }

    // Do not process if the hop count recorded in it is greater
    // than HWMP net diameter.
    if (rannData.hopCount > (unsigned int) HWMP_NET_DIAMETER)
    {
        ERROR_ReportWarning("Hwmp_ReceiveRann: "
            "The hop count to root exceeds HWMP_NET_DIAMETER.\n"
            "Configuration values seem to need change.\n");

        //hwmp->stats->numMaxHopExceed++;

        return;
    }

    BOOL createRt = FALSE;
    BOOL updateRt = FALSE;
    BOOL dropRann = FALSE;
    HWMP_RouteEntry* rtToSource = NULL;
    BOOL isValidSource = FALSE;

    unsigned int metricToPrevHopAddr =
        Dot11s_ComputeLinkMetric(node, dot11, prevHopAddr);

    if (rannData.rootAddr != prevHopAddr)
    {
        HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
            prevHopAddr,
            0,                      // sequence number
            1,                      // hop count
            metricToPrevHopAddr,
            prevHopAddr,
            node->getNodeTime() + rannData.lifetime,
            TRUE,                   // isActive
            interfaceIndex);

        if (hwmp->isExpireTimerSet == FALSE)
        {
            hwmp->isExpireTimerSet = TRUE;

            DOT11s_TimerInfo timerInfo(prevHopAddr);
            Dot11s_StartTimer(node, dot11, &timerInfo,
                rannData.lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }

    rtToSource = HwmpRouteTable_FindEntry(node, dot11, hwmp,
        rannData.rootAddr, &isValidSource);

    if (!rtToSource)
    {
        // Need to allocate route entry
        createRt = TRUE;
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Create new route");
        }
    }
    else
    {   // Route entry exists
        if (isValidSource == FALSE
            || rtToSource->addrSeqInfo.seqNo == 0)
        {
            updateRt = TRUE;
        }
        else
        {   // Route entry is valid had a non-zero sequence number
            if (rtToSource->addrSeqInfo.seqNo > rannData.rootSeqNo)
            {
                dropRann = TRUE;
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL, "Old seq no.");
                }
            }
            else
            {   // Root seq no is better or metric is better
                if ((rtToSource->addrSeqInfo.seqNo
                        < rannData.rootSeqNo)
                    || (rtToSource->addrSeqInfo.seqNo
                            == rannData.rootSeqNo
                        && rtToSource->metric
                            > rannData.metric + metricToPrevHopAddr))
                {
                    updateRt = TRUE;
                }
            }
        }
    }

    if (dropRann)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Old seq no., dropping");
        }
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, frameInfo->msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        return;
    }

    if ((createRt || updateRt) == FALSE)
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "No new/ update of route, dropping");
        }
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, frameInfo->msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        return;
    }

    if (hwmp->isRoot) {
        Hwmp_RootReceiveRannError(node, dot11, hwmp, hwmp->tbrState);

        return;
    }

    rannData.hopCount += 1;
    rannData.metric += metricToPrevHopAddr;

    rtToSource =
        HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
            rannData.rootAddr,
            (unsigned)rannData.rootSeqNo,
            (int)rannData.hopCount,
            rannData.metric,
            prevHopAddr,
            node->getNodeTime()
            + rannData.lifetime,
            TRUE,                   // isActive
            interfaceIndex);

    if (hwmp->isExpireTimerSet == FALSE)
    {
        hwmp->isExpireTimerSet = TRUE;

        DOT11s_TimerInfo timerInfo(rannData.rootAddr);
        Dot11s_StartTimer(node, dot11, &timerInfo,
            rannData.lifetime,
            MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
    }

    hwmp->stats->rannReceived++;

    // Update local root information
    hwmp->tbrStateData.event = HWMP_TBR_S_EVENT_RAAN_RECEIVED;
    hwmp->tbrStateData.frameInfo = frameInfo;
    hwmp->tbrStateData.interfaceIndex = interfaceIndex;
    hwmp->tbrStateData.rannData = &rannData;
    hwmp->tbrStateFn(node, dot11, hwmp);

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RelayRann
// PURPOSE   : Relay a root announcement
// ARGUMENTS : rxRannData, received Root Announcement data
// RETURN    : None
// NOTES     : Relay the latest received RANN data with validated
//              parent metrics. Lifetime not decreased; if required,
//              a value is available in hwmp->rannPropagationDelay.
//--------------------------------------------------------------------

static
void Hwmp_RelayRann(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_RannData* rxRannData)
{
    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Relaying RANN at %s", clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    HWMP_RannData rannData = *rxRannData;

    rannData.ttl--;
    if (rannData.ttl == 0)
    {
        hwmp->stats->rannTtlExpired++;
        return;
    }
    rannData.metric = hwmp->currentParent->rannData.metric;
    rannData.lifetime -=
        node->getNodeTime() - hwmp->currentParent->lastRannTime;

    Hwmp_SendRannFrame(node, dot11, hwmp, &rannData);

    hwmp->stats->rannRelayed++;

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendMaintenanceRreq
// PURPOSE   : Send a TBR Maintenance RREQ
// ARGUMENTS : frameInfo, frame data to send
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_SendMaintenanceRreq(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    DOT11s_FrameInfo* frameInfo,
    HWMP_RreqData* rreqData)
{
    // TTL is filled in from the calling fn.
    ERROR_Assert(rreqData->ttl >= 1,
        "Hwmp_SendMaintenanceRreq: TTL needs to be greater than 0.\n");

    // Increase own sequence number.
    hwmp->seqNo++;

    rreqData->portalRole = FALSE;
    // isBroadcast is filled in
    rreqData->rrepRequested = FALSE;
    rreqData->staReassocFlag = FALSE;
    rreqData->hopCount = 0;
    // ttl is filled in
    rreqData->rreqId = Hwmp_GetRreqId(node, dot11, hwmp);
    rreqData->sourceInfo.addr = frameInfo->SA;
    rreqData->sourceInfo.seqNo = hwmp->seqNo;
    rreqData->metric = 0;
    // lifetime is filled in
    // DO and RF flags are filled in by the calling fn.
    rreqData->destInfo.addr = hwmp->rootItem.rannData.rootAddr;
    rreqData->destInfo.seqNo = hwmp->rootItem.rannData.rootSeqNo;

    Hwmp_SendRreqFrame(node, dot11, hwmp,
        rreqData,
        frameInfo->RA,
        0,                      // Set own state timer
        FALSE);                 // isRelay

    if (frameInfo->RA == ANY_MAC802)
    {
        HwmpSentTable_InsertEntry(node, dot11, hwmp,
            frameInfo->DA, rreqData->ttl);

        hwmp->stats->rreqBcInitiated++;
    }
    else
    {
        hwmp->stats->rreqUcInitiated++;
    }

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_SendTbrGratuitousRrep
// PURPOSE   : Send a gratuitous RREP to root
// ARGUMENTS : DA, final destination address
//             nextHopAddr, address of parent to root
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_SendTbrGratuitousRrep(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Mac802Address DA,
    Mac802Address nextHopAddr)
{
    // Omit sending if there are no associated stations.
    if (Dot11s_GetAssociatedStationCount(node, dot11) == 0)
    {
        return;
    }

    HWMP_RrepData rrepData;
    ListInit(node, &(rrepData.sourceInfoList));

    // Increment sequence number
    hwmp->seqNo++;

    rrepData.isGratuitous = TRUE;
    rrepData.hopCount = 0;
    rrepData.ttl = HWMP_NET_DIAMETER;

    rrepData.lifetime = HWMP_MY_ROUTE_TIMEOUT;
    rrepData.metric = 0;

    // Note: The meaning of destination is inverted here.
    rrepData.destInfo.addr = DA;
    rrepData.destInfo.seqNo = hwmp->rootItem.rannData.rootSeqNo;

    // First item is own address
    HWMP_AddrSeqInfo* sourceItem;
    Dot11s_Malloc(HWMP_AddrSeqInfo, sourceItem);
    sourceItem->addr = dot11->selfAddr;
    sourceItem->seqNo = hwmp->seqNo;
    ListAppend(node, rrepData.sourceInfoList, 0, sourceItem);

    // For an MP, this frame omits dependent MP information
    // as specified in the draft.
    // For an MAP, the source list describes BSS stations.
    if (dot11->isMAP)
    {
    DOT11_ApStationListItem* item = dot11->apVars->apStationList->first;
    while (item)
    {
            Dot11s_Malloc(HWMP_AddrSeqInfo, sourceItem);
        sourceItem->addr = item->data->macAddr;
        // The same sequence number is shared with stations.
        sourceItem->seqNo = hwmp->seqNo;
        ListAppend(node, rrepData.sourceInfoList, 0, sourceItem);

        item = item->next;
    }
    }

    Hwmp_SendRrepFrame(node, dot11,
        &rrepData,
        nextHopAddr);

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char destStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(destStr, &rrepData.destInfo.addr);
        Dot11s_AddrAsDotIP(nextHopStr, &nextHopAddr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr,
            "Sending registration RREP to %s, nextHop %s, at %s",
            destStr, nextHopStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, rrepData.sourceInfoList, FALSE);

    hwmp->stats->rrepGratInitiated++;
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbr_InitState
// PURPOSE   : TBR Init state handler
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpTbr_InitState(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    DOT11s_Data* mp = dot11->mp;
    HWMP_RootItem* rootItem = &(hwmp->rootItem);
    HWMP_RannData* rootItemRann = &(hwmp->rootItem.rannData);

    switch (hwmp->tbrStateData.event)
    {

    case HWMP_TBR_S_EVENT_ENTER_STATE:
    {
        hwmp->fwdState = HWMP_TBR_FWD_LEARNING;
        hwmp->currentParent = NULL;
        hwmp->potentialNewParent = FALSE;

        if (hwmp->isRoot)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Initializing Root TBR variables");
            }

            rootItemRann->isPortal = mp->isMpp;
            rootItemRann->registration = hwmp->rootRegistrationFlag;
            rootItemRann->hopCount = 0;
            rootItemRann->ttl = HWMP_NET_DIAMETER;
            rootItemRann->rootAddr = dot11->selfAddr;
            rootItemRann->lifetime = HWMP_ACTIVE_ROOT_TIMEOUT;
            rootItemRann->metric = 0;

            // Start a small random timer to initiate RANNs.
            hwmp->rannPropagationDelay =
                RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME;

            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                hwmp->rannPropagationDelay,
                MSG_MAC_DOT11s_HwmpTbrRannTimeout);
        }
        else
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Initialize/Reset MP TBR variables");
            }

            rootItem->Reset();

            if (hwmp->parentList != NULL)
            {
                ListFree(node, hwmp->parentList, FALSE);
            }
            ListInit(node, &(hwmp->parentList));
        }

        break;
    }
    case HWMP_TBR_S_EVENT_RANN_TIMER:
    {
        if (hwmp->isRoot)
        {
            // Send first RANN and transition to forwarding state.
            Hwmp_SendRann(node, dot11, hwmp);

            HwmpTbr_SetState(node, dot11, hwmp,
                HWMP_TBR_S_FORWARDING);

            break;
        }

        // Select best parent
        unsigned int metric = HWMP_METRIC_MAX;
        ListItem* item = hwmp->parentList->first;
        HWMP_TbrParentItem* bestParent = NULL;
        HWMP_TbrParentItem* tempParent = NULL;
        while (item != NULL)
        {
            tempParent = (HWMP_TbrParentItem*) item->data;
            if (tempParent->rannData.metric < metric)
            {
                bestParent = tempParent;
                metric = tempParent->rannData.metric;
            }

            item = item->next;
        }

        if (bestParent == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "No RANN propagation; no parent");
            }

            // Do nothing
            break;
        }

        if (bestParent->rannData.registration)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "HwmpTbr_TopologyForwardingState: "
                    "Validate parent/root with unicast RREQ");
            }

            // Validate parent / root by sending a unicast RREQ
            DOT11s_FrameInfo frameInfo;
            frameInfo.RA = bestParent->addr;
            frameInfo.TA = dot11->selfAddr;
            frameInfo.DA = rootItemRann->rootAddr;
            frameInfo.SA = dot11->selfAddr;
            // other values not important

            HWMP_RreqData rreqData;
            rreqData.isBroadcast = FALSE;
            rreqData.ttl = HWMP_NET_DIAMETER;
            // rreqData.ttl = 1;            // if validating parent
            rreqData.lifetime = rootItemRann->lifetime;
            rreqData.DO_Flag = TRUE;        // if validating root
            //rreqData.DO_Flag = FALSE;     // if validating parent
            rreqData.RF_Flag = FALSE;

            Hwmp_SendMaintenanceRreq(node, dot11, hwmp,
                &frameInfo, &rreqData);

            DOT11s_TimerInfo timerInfo(bestParent->addr);
            Dot11s_StartTimer(node, dot11, &timerInfo,
                HWMP_NET_TRAVERSAL_TIME,
                MSG_MAC_DOT11s_HmwpTbrRrepTimeout);

        }
        else
        {
            hwmp->currentParent = bestParent;

            // Send gratuitous RREP
            Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                rootItemRann->rootAddr, bestParent->addr);

            // Enter forwarding state.
            HwmpTbr_SetState(node, dot11, hwmp,
                HWMP_TBR_S_FORWARDING);
        }

        break;
    }

    case HWMP_TBR_S_EVENT_ROOT_MAINTENANCE_TIMER:
    {
        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Unexpected Root maintenance timer in TBR Init state.");
        }

        break;
    }

    case HWMP_TBR_S_EVENT_RREP_TIMER:
    {
        Mac802Address parentAddr = hwmp->tbrStateData.parentAddr;

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char parentStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(parentStr, &parentAddr);
            sprintf(traceStr, "HwmpTbr_InitState: "
                "RREP timeout for parent %s", parentStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        HwmpTbrParentList_Delete(node, dot11, hwmp, parentAddr);

        break;
    }

    case HWMP_TBR_S_EVENT_RAAN_RECEIVED:
    {
        if (hwmp->isRoot)
        {
            // Cannot happen or no action.
            break;
        }

        if (hwmp->tbrStateData.rannData->rootSeqNo
            > rootItemRann->rootSeqNo)
        {
            hwmp->rannPropagationDelay =
                RANDOM_nrand(dot11->seed) % (HWMP_RANN_PROPAGATION_DELAY / 2)
                + HWMP_RANN_PROPAGATION_DELAY;

            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                hwmp->rannPropagationDelay,
                MSG_MAC_DOT11s_HwmpTbrRannTimeout);
        }

        rootItem->lastRannTime = node->getNodeTime();
        rootItem->prevHopAddr = hwmp->tbrStateData.frameInfo->TA;
        *rootItemRann = *(hwmp->tbrStateData.rannData);

        // Add parent, if required.
        HWMP_TbrParentItem* parent;
        parent = HwmpTbrParentList_Lookup(node, dot11, hwmp,
            rootItem->prevHopAddr);

        if (parent == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "New parent added.");
            }

            Dot11s_MallocMemset0(HWMP_TbrParentItem, parent);
            parent->addr = rootItem->prevHopAddr;
            parent->lastValidationTime = 0;

            ListAppend(node, hwmp->parentList, 0, parent);
        }

        parent->rannData = *rootItemRann;
        parent->lastRannTime = rootItem->lastRannTime;

        break;
    }
    case HWMP_TBR_S_EVENT_RREP_RECEIVED:
    {
        if (hwmp->tbrStateData.rrepData->destInfo.addr
            != rootItemRann->rootAddr)
        {
            // Not a response to RREQ to root; ignore
            break;
        }

        Mac802Address prevHopAddr = hwmp->tbrStateData.frameInfo->TA;

        HWMP_TbrParentItem* parent;
        parent = HwmpTbrParentList_Lookup(node, dot11, hwmp, prevHopAddr);

        if (parent != NULL)
        {
            if (HWMP_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(addrStr, &prevHopAddr);
                sprintf(traceStr,
                    "Validated Prev hop %s", addrStr);
                MacDot11Trace(node, dot11, NULL, traceStr);

            }

            parent->lastValidationTime = node->getNodeTime();
            hwmp->currentParent = parent;

            // Send gratuitous RREP
            Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                rootItemRann->rootAddr, parent->addr);

            // Enter forwarding state.
            HwmpTbr_SetState(node, dot11, hwmp,
                HWMP_TBR_S_FORWARDING);
        }

        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "HwmpTbr_InitState: "
            "!! Event ignored or unhandled -- %d",
            hwmp->tbrStateData.event);
        MacDot11Trace(node, dot11, NULL, traceStr);

        break;
    }

    } //switch
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbr_ForwardingState
// PURPOSE   : TBR Forwarding state handler
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpTbr_ForwardingState(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp)
{
    HWMP_RootItem* rootItem = &(hwmp->rootItem);
    HWMP_RannData* rootItemRann = &(hwmp->rootItem.rannData);

    switch (hwmp->tbrStateData.event)
    {

    case HWMP_TBR_S_EVENT_ENTER_STATE:
    {
        hwmp->fwdState = HWMP_TBR_FWD_FORWARDING;

        // For root, start timer for next RANN .
        if (hwmp->isRoot)
        {
            hwmp->rannPropagationDelay =
                 (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
                + HWMP_RANN_PERIOD;

            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                hwmp->rannPropagationDelay,
                MSG_MAC_DOT11s_HwmpTbrRannTimeout);

            break;
        }

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "MP in forwarding state.");
        }

        // For MP, start maintenance timer.
        clocktype delay =
            (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
            + HWMP_ROOT_MAINTENANCE_PERIOD;

        DOT11s_TimerInfo timerInfo;
        Dot11s_StartTimer(node, dot11, &timerInfo, delay,
            MSG_MAC_DOT11s_HwmpTbrMaintenanceTimeout);


        break;
    }

    case HWMP_TBR_S_EVENT_RANN_TIMER:
    {
        // For root, send RANN and start timer for next RANN.
        if (hwmp->isRoot)
        {
            Hwmp_SendRann(node, dot11, hwmp);

            hwmp->rannPropagationDelay =
                 (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
                + HWMP_RANN_PERIOD;

            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                hwmp->rannPropagationDelay,
                MSG_MAC_DOT11s_HwmpTbrRannTimeout);
        }
        else
        {
            // For MP, relay RANN.
            if (hwmp->currentParent != NULL)
            {
                // Forward RAAN frame; TTL checked in relay fn.
                if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 1)
                {
                    Hwmp_RelayRann(node, dot11, hwmp, rootItemRann);
                }
                else
                {
                    if (HWMP_TraceComments)
                    {
                        MacDot11Trace(node, dot11, NULL,
                            "Rann relay omitted due to neighbor count.");
                    }
                }
            }
        }

        break;
    }

    case HWMP_TBR_S_EVENT_ROOT_MAINTENANCE_TIMER:
    {
        char traceStr[MAX_STRING_LENGTH];
        char parentStr[MAX_STRING_LENGTH];

        if (HWMP_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Root maintenance timer.");
        }

        // Housekeep parents.

        ListItem* listItem = hwmp->parentList->first;
        ListItem* tempItem;
        while (listItem != NULL)
        {
            HWMP_RouteEntry* rtToParent = NULL;
            BOOL rtIsValid = FALSE;
            HWMP_TbrParentItem *parent =
                (HWMP_TbrParentItem*) listItem->data;
            rtToParent = HwmpRouteTable_FindEntry(node, dot11, hwmp,
                parent->addr, &rtIsValid);
            if (rtIsValid == FALSE)
            {
                if (hwmp->currentParent == parent)
                {
                    if (HWMP_TraceComments)
                    {
                        Dot11s_AddrAsDotIP(parentStr, &parent->addr);
                        sprintf(traceStr,
                            "HwmpTbr_TopologyForwardingState: "
                            "housekeeping; removing current parent %s",
                            parentStr);
                        MacDot11Trace(node, dot11, NULL, traceStr);
                    }

                    hwmp->currentParent = NULL;

                    HwmpTbr_SetState(node, dot11, hwmp, HWMP_TBR_S_INIT);
                    break;
                }
                else
                {
                    if (HWMP_TraceComments)
                    {
                        Dot11s_AddrAsDotIP(parentStr, &parent->addr);
                        sprintf(traceStr,
                            "HwmpTbr_TopologyForwardingState: "
                            "housekeeping; removing inactive parent %s",
                            parentStr);
                        MacDot11Trace(node, dot11, NULL, traceStr);
                    }
                }

                tempItem = listItem;
                listItem = listItem->next;
                //HwmpTbrParentList_Delete(node, dot11, hwmp, parent->addr);
                ListDelete(node, hwmp->parentList, tempItem, FALSE);

                break;
            }
            else
            {
                listItem = listItem->next;
            }
        }

        if (ListGetSize(node, hwmp->parentList) == 0)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "HwmpTbr_TopologyForwardingState: "
                    "no parent; moving to Init state");
            }

            hwmp->currentParent = NULL;

            HwmpTbr_SetState(node, dot11, hwmp, HWMP_TBR_S_INIT);
            break;
        }


        HWMP_TbrParentItem* bestParent = hwmp->currentParent;

        if (hwmp->potentialNewParent)
        {
            bestParent = NULL;

            unsigned int metric = HWMP_METRIC_MAX;
            ListItem* item = hwmp->parentList->first;
            HWMP_TbrParentItem* tempParent = NULL;

            while (item != NULL)
            {
                tempParent = (HWMP_TbrParentItem*) item->data;
                if (tempParent->rannData.metric < metric)
                {
                    bestParent = tempParent;
                    metric = tempParent->rannData.metric;
                }

                item = item->next;
            }
        }

        if (bestParent == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "HwmpTbr_TopologyForwardingState: "
                    "No parent found, moving to Init state");
            }

            hwmp->currentParent = NULL;

            HwmpTbr_SetState(node, dot11, hwmp, HWMP_TBR_S_INIT);
            break;
        }

        if (bestParent != hwmp->currentParent)
        {
            if (HWMP_TraceComments)
            {
                Dot11s_AddrAsDotIP(parentStr, &bestParent->addr);
                sprintf(traceStr,
                    "HwmpTbr_TopologyForwardingState: "
                    "Found better parent %s", parentStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }
        }
        else
        {
            if (HWMP_TraceComments)
            {
                Dot11s_AddrAsDotIP(parentStr, &bestParent->addr);
                sprintf(traceStr,
                    "HwmpTbr_TopologyForwardingState: "
                    "Current parent %s is unchanged", parentStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }
        }

        if (bestParent->rannData.registration)
        {
            // Send a maintenance RREQ
            DOT11s_FrameInfo frameInfo;
            frameInfo.RA = bestParent->addr;
            frameInfo.TA = dot11->selfAddr;
            frameInfo.DA = rootItemRann->rootAddr;
            frameInfo.SA = dot11->selfAddr;
            // other values not important

            // Send a unicast rreq
            HWMP_RreqData rreqData;
            rreqData.isBroadcast = FALSE;
            rreqData.ttl = HWMP_NET_DIAMETER;   // if validating root
            // rreqData.ttl = 1;            // if validating parent
            rreqData.lifetime = rootItemRann->lifetime;
            rreqData.DO_Flag = TRUE;        // if validating root
            //rreqData.DO_Flag = FALSE;     // if validating parent
            rreqData.RF_Flag = FALSE;

            Hwmp_SendMaintenanceRreq(node, dot11, hwmp,
                &frameInfo, &rreqData);

            DOT11s_TimerInfo timerInfo(bestParent->addr);
            Dot11s_StartTimer(node, dot11, &timerInfo,
                HWMP_NET_TRAVERSAL_TIME,
                MSG_MAC_DOT11s_HmwpTbrRrepTimeout);
        }
        else
        {
            if (hwmp->currentParent != bestParent)
            {
                // Send gratuitous RREP.
                // The RREP does not include dependent MPs.
                // For MAPs, the RREP includes a list of stations.
                Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                    rootItemRann->rootAddr, bestParent->addr);
                hwmp->stationUpdateIsDue = FALSE;
            }
            else
            {
                if (hwmp->stationUpdateIsDue)
                {
                    Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                        rootItemRann->rootAddr, bestParent->addr);
                    hwmp->stationUpdateIsDue = FALSE;
                }
            }


            hwmp->potentialNewParent = FALSE;
            hwmp->currentParent = bestParent;
        }

        // Start next maintenance timer.
        clocktype delay =
           (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
            + HWMP_ROOT_MAINTENANCE_PERIOD;

        DOT11s_TimerInfo timerInfo;
        Dot11s_StartTimer(node, dot11, &timerInfo, delay,
            MSG_MAC_DOT11s_HwmpTbrMaintenanceTimeout);

        break;
    }

    case HWMP_TBR_S_EVENT_RREP_TIMER:
    {
        Mac802Address parentAddr = hwmp->tbrStateData.parentAddr;
        HWMP_TbrParentItem* parent =
            HwmpTbrParentList_Lookup(node, dot11, hwmp, parentAddr);

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char parentStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(parentStr, &parentAddr);
            sprintf(traceStr, "HwmpTbr_ForwardingState: "
                "RREP timeout for parent %s", parentStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        if (parent == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "!! HwmpTbr_ForwardingState: "
                    "RREP timer for NULL parent");
            }
            break;
        }

        HWMP_RouteEntry* rtToParent = NULL;
        BOOL rtIsValid = FALSE;

        if (parent == hwmp->currentParent)
        {
            rtToParent = HwmpRouteTable_FindEntry(node, dot11, hwmp,
                parentAddr, &rtIsValid);
            if (rtIsValid == FALSE)
            {
                HwmpTbr_SetState(node, dot11, hwmp, HWMP_TBR_S_INIT);
                break;
            }
        }

        hwmp->potentialNewParent = FALSE;

        break;
    }

    case HWMP_TBR_S_EVENT_RAAN_RECEIVED:
    {
        if (hwmp->isRoot)
        {
            // Do nothing, should be filtered out in the receive fn.
            break;
        }

        ERROR_Assert(hwmp->currentParent != NULL,
            "HwmpTbr_ForwardingState: "
            "RANN received but current parent is NULL.\n");

        if (hwmp->tbrStateData.rannData->rootSeqNo
            > rootItemRann->rootSeqNo)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Starting RANN propagation timer.");
            }

            hwmp->rannPropagationDelay =
                RANDOM_nrand(dot11->seed) % (HWMP_RANN_PROPAGATION_DELAY / 2)
                + HWMP_RANN_PROPAGATION_DELAY;

            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                hwmp->rannPropagationDelay,
                MSG_MAC_DOT11s_HwmpTbrRannTimeout);
        }

        *rootItemRann = *(hwmp->tbrStateData.rannData);
        rootItem->lastRannTime = node->getNodeTime();
        rootItem->prevHopAddr = hwmp->tbrStateData.frameInfo->TA;

        HWMP_TbrParentItem* parent;
        parent = HwmpTbrParentList_Lookup(node, dot11, hwmp,
            rootItem->prevHopAddr);

        if (parent == NULL)
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "New parent added.");
            }

            Dot11s_MallocMemset0(HWMP_TbrParentItem, parent);
            parent->addr = rootItem->prevHopAddr;
            parent->lastValidationTime = 0;

            ListAppend(node, hwmp->parentList, 0, parent);
        }

        parent->rannData = *rootItemRann;
        parent->lastRannTime = rootItem->lastRannTime;

        // To prevent change of parent with small differences
        // in metric, the new parent metric could be better
        // by a given factor.
        if ((parent->rannData.metric
              < hwmp->currentParent->rannData.metric
                * HWMP_TBR_BETTER_PARENT_FACTOR)
            && (parent->addr != hwmp->currentParent->addr))
        {
            if (HWMP_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Potential better parent.");
            }
            hwmp->potentialNewParent = TRUE;
        }

        break;
    }
    case HWMP_TBR_S_EVENT_RREP_RECEIVED:
    {
        // Root only receives gratituous RREPs.
        // On demand RREPs ignored; this is invalid
        // for multiple roots.
        if (hwmp->isRoot)
        {
            break;
        }

        if (hwmp->tbrStateData.rrepData->destInfo.addr
            != rootItemRann->rootAddr)
        {
            // Not a maintenance RREQ to root; ignore
            break;
        }

        Mac802Address parentAddr = hwmp->tbrStateData.frameInfo->TA;
        HWMP_TbrParentItem* parent =
            HwmpTbrParentList_Lookup(node, dot11, hwmp, parentAddr);
        if (parent == NULL)
        {
            if (HWMP_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char parentStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(parentStr, &parentAddr);
                sprintf(traceStr, "!! HwmpTbr_ForwardingState: "
                    "RREP received from non-existent parent %s",
                    parentStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            break;
        }

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &parentAddr);
            sprintf(traceStr,
                "Validated root/parent %s", addrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        hwmp->potentialNewParent = FALSE;

        if (parent != hwmp->currentParent)
        {
            Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                rootItemRann->rootAddr, parent->addr);
            hwmp->stationUpdateIsDue = FALSE;
        }
        else
        {
            if (hwmp->stationUpdateIsDue)
            {
                Hwmp_SendTbrGratuitousRrep(node, dot11, hwmp,
                    rootItemRann->rootAddr, parent->addr);
                hwmp->stationUpdateIsDue = FALSE;
            }
        }

        hwmp->currentParent = parent;
        parent->lastValidationTime = node->getNodeTime();

        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "!! HwmpTbr_TopologyForwardingState: "
            "Event ignored or unhandled -- %d",
            hwmp->tbrStateData.event);
        MacDot11Trace(node, dot11, NULL, traceStr);

        break;
    }

    } //switch
}


//--------------------------------------------------------------------
// FUNCTION  : HwmpTbr_SetState
// PURPOSE   : Set Tree based routing state
// ARGUMENTS : state, new state to set
// RETURN    : None
//--------------------------------------------------------------------

static
void HwmpTbr_SetState(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    HWMP_TbrState state)
{
    hwmp->prevTbrState = hwmp->tbrState;
    hwmp->tbrState = state;

    switch (state)
    {
        case HWMP_TBR_S_INIT:
            hwmp->tbrStateFn = HwmpTbr_InitState;
            break;
        case HWMP_TBR_S_FORWARDING:
            hwmp->tbrStateFn = HwmpTbr_ForwardingState;
            break;
        default:
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr, "HwmpTbr_SetState: Invalid state %d.\n",
                state);
            ERROR_ReportError(errorStr);
            break;
    }

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Entering TBR state %d at %s",
            state, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    hwmp->tbrStateData.event = HWMP_TBR_S_EVENT_ENTER_STATE;
    hwmp->tbrStateFn(node, dot11, hwmp);
}


//--------------------------------------------------------------------
// FUNCTION : Hwmp_ReceiveControlFrame
// PURPOSE  : Called when Hwmp packet is received. The packets
//            may be of following types, Route Request, Route Reply,
//            Route Error
// ARGUMENTS: msg,      The message received
//            interfaceIndex, receiving interface
// RETURN   : None
//--------------------------------------------------------------------

void Hwmp_ReceiveControlFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    int interfaceIndex)
{
    DOT11s_Data* mp = dot11->mp;

    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_ReceiveControlFrame: HWMP is not the active protocol.\n");
    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

    unsigned char* rxFrame = (unsigned char *) frameHdr;
    int offset = sizeof(DOT11_FrameHdr);

    unsigned char rxFrameCategory = *(rxFrame + offset);
    ERROR_Assert(rxFrameCategory == DOT11_ACTION_MESH,
        "Hwmp_ReceiveControlFrame: Not a mesh category frame.\n");
    offset++;
    unsigned char rxFrameActionFieldId = *(rxFrame + offset);

    DOT11s_FrameInfo frameInfo;
    frameInfo.msg = msg;
    frameInfo.RA = frameHdr->destAddr;
    frameInfo.TA = frameHdr->sourceAddr;
    frameInfo.DA = frameHdr->address3;
    // Other frameInfo values at default.

    switch (rxFrameActionFieldId)
    {
        case DOT11_MESH_RREQ:
        {
            Hwmp_ReceiveRreq(node, dot11, hwmp,
                &frameInfo, interfaceIndex);

            break;
        }

        case DOT11_MESH_RREP:
        {
            if (HWMP_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char clockStr[MAX_STRING_LENGTH];
                char sourceStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                Dot11s_AddrAsDotIP(sourceStr, &frameHdr->sourceAddr);
                sprintf(traceStr, "Got RREP from %s at time %s",
                    sourceStr, clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            Hwmp_ReceiveRrep(node, dot11, hwmp, &frameInfo, interfaceIndex);

            break;
        }

        case DOT11_MESH_RERR:
        {
            Hwmp_ReceiveRerr(node, dot11, hwmp,
                &frameInfo, interfaceIndex);

            break;
        }

        case DOT11_MESH_RREP_ACK:
        {
            ERROR_ReportError(
                "Hwmp_ReceiveControlFrame: RREP-ACK deprecated.\n");
            break;
        }

        case DOT11_MESH_RANN:
        {
            Hwmp_ReceiveRann(node, dot11, hwmp,
                &frameInfo, interfaceIndex);

            break;
        }

        default:
        {
            ERROR_ReportError(
                "Hwmp_ReceiveControlFrame: Received unknown packet type.\n");
           break;
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_LinkFailureFunction
// PURPOSE   : Handle link failure notification resulting from a packet
//             drop.
// ARGUMENTS : nextHopAddr, neighbor address
//             destAddr, destination address
//             interfaceIndex, the interface at which message was sent
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_LinkFailureFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr,
    int interfaceIndex)
{
    DOT11s_Data* mp = dot11->mp;

    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_LinkFailureFunction: HWMP is not the active protocol.\n");
    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);

    if (nextHopAddr == ANY_MAC802)
    {
        return;
    }

    if (HWMP_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(nextHopStr, &nextHopAddr);

        if (destAddr == ANY_MAC802
            || destAddr == INVALID_802ADDRESS
            || destAddr == DOT11s_MESH_BSS_ID)
        {
            sprintf(traceStr,
                "Failed nexthop %s", nextHopStr);
        }
        else
        {
            char destStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(destStr, &destAddr);
            sprintf(traceStr,
                "Failed to deliver frame to dest %s using nexthop %s",
                destStr, nextHopStr);
        }
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    //hwmp->stats->numBrokenLinks++;

    // Removes messages from queue destined to the next hop to which
    // message transmission failed.
    Dot11s_DeletePacketsToNode(node, dot11, nextHopAddr, ANY_MAC802);

    Hwmp_SendRerrForLinkFailure(node, dot11, hwmp, nextHopAddr);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_LinkUpdateFunction
// PURPOSE   : Handle notification that a neighbor is alive.
// ARGUMENTS : neighborAddr, neighbor address
//             metric, cost metric to neighbor link
//             lifetime, lifetime of link
//             interfaceIndex, the interface at which message was sent
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_LinkUpdateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr,
    unsigned int metric,
    clocktype lifetime,
    int interfaceIndex)
{
    ERROR_Assert(
        dot11->mp->activeProtocol.pathProtocol ==
            DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_LinkUpdateFunction: HWMP is not the active protocol.\n");

    return;

    // For 802.11s, neighbor link state exchanges could serve as
    // hello packets. This may be useful for LWMPs.
    //HWMP_Data* hwmp = (HWMP_Data*)(dot11->mp->activeProtocol.data);

    //HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
    //    neighborAddr,
    //    0,
    //    1,
    //    metric,
    //    neighborAddr,
    //    node->getNodeTime() + lifetime,
    //    TRUE,
    //    interfaceIndex);

    //if (hwmp->isExpireTimerSet == FALSE)
    //{
    //    hwmp->isExpireTimerSet = TRUE;

    //    DOT11s_TimerInfo timerInfo(neighborAddr);
    //    Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
    //        MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
    //}
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_LinkCloseFunction
// PURPOSE   : Handle link failure notification resulting from cause
//             other than a packet drop
// ARGUMENTS : nextHopAddr, neighbor address
//             interfaceIndex, the interface at which message was sent
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_LinkCloseFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    int interfaceIndex)
{
    Hwmp_LinkFailureFunction(node, dot11,
        nextHopAddr, ANY_MAC802, interfaceIndex);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_PathUpdateFunction
// PURPOSE   : Handle notification to create/update path to destination
// ARGUMENTS : destAddr, destination address of path
//             destSeqNo, DSN of destination, 0 if unknown
//             hopCount, hop count to destination
//             metric, cost metric to neighbor link
//             lifetime, lifetime of link
//             nextHopAddr, next hop address
//             isActive, TRUE to add path
//             interfaceIndex, the interface at which message was sent
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_PathUpdateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    unsigned int destSeqNo,
    int hopCount,
    unsigned int metric,
    clocktype lifetime,
    Mac802Address nextHopAddr,
    BOOL isActive,
    int interfaceIndex)
{
    ERROR_Assert(
        dot11->mp->activeProtocol.pathProtocol ==
            DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_PathUpdateFunction: HWMP is not the active protocol.\n");

    HWMP_Data* hwmp = (HWMP_Data*)(dot11->mp->activeProtocol.data);

    HwmpRouteTable_UpdateEntry(node, dot11, hwmp,
        destAddr,
        destSeqNo,
        hopCount,
        metric,
        nextHopAddr,
        node->getNodeTime() + lifetime,
        isActive,
        interfaceIndex);

    if (hwmp->isExpireTimerSet == FALSE)
    {
        hwmp->isExpireTimerSet = TRUE;

        DOT11s_TimerInfo timerInfo(destAddr);

        if (isActive == TRUE)
        {
            Dot11s_StartTimer(node, dot11, &timerInfo, lifetime,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
        else
        {
            Dot11s_StartTimer(node, dot11, &timerInfo, 1,
                MSG_MAC_DOT11s_HwmpActiveRouteTimeout);
        }
    }
}

//--------------------------------------------------------------------
// FUNCTION  : Hwmp_StationAssociateFunction
// PURPOSE   : Handle notification of station association
// ARGUMENTS : staAddr, address of station that has associated
//             prevApAddr, address of previuos AP
//             interfaceIndex, the interface index
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_StationAssociateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    Mac802Address prevApAddr,
    int interfaceIndex)
{
    ERROR_Assert(
        dot11->mp->activeProtocol.pathProtocol ==
            DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_StationAssociateFunction: HWMP is not the active protocol.\n");
    HWMP_Data* hwmp = (HWMP_Data*)(dot11->mp->activeProtocol.data);

    // Set flag for proxy update.
    if (hwmp->fwdState == HWMP_TBR_FWD_FORWARDING)
    {
        hwmp->stationUpdateIsDue = TRUE;
    }

    // For an association, do nothing
    if (prevApAddr == INVALID_802ADDRESS
        || prevApAddr == dot11->selfAddr)
    {
        return;
    }

    // Notify previous AP by sending a custom RREQ
    // with station reassociation flag set.
    int ttl = HWMP_NET_DIAMETER;
    hwmp->seqNo++;
    HWMP_RreqData rreqData;

    rreqData.portalRole = FALSE;
    rreqData.isBroadcast = TRUE;
    rreqData.rrepRequested = FALSE;
    rreqData.staReassocFlag = TRUE;
    rreqData.hopCount = 0;
    rreqData.ttl = (unsigned char) ttl;
    rreqData.rreqId = Hwmp_GetRreqId(node, dot11, hwmp);
    rreqData.sourceInfo.addr = dot11->selfAddr;
    // For a station, use MAP's value
    rreqData.sourceInfo.seqNo = hwmp->seqNo;
    rreqData.lifetime = HWMP_REVERSE_ROUTE_TIMEOUT;
    rreqData.metric = 0;
    rreqData.DO_Flag = TRUE;
    rreqData.RF_Flag = FALSE;
    rreqData.destInfo.addr = staAddr;
    rreqData.destInfo.seqNo = 0;

    Hwmp_SendRreqFrame(node, dot11, hwmp,
        &rreqData,
        ANY_MAC802,
        0,                          // No reply expected
        FALSE);                     // isRelay

    hwmp->stats->rreqBcInitiated++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_StationDisssociateFunction
// PURPOSE   : Handle notification of station association
// ARGUMENTS : staAddr, address of station that has disassociated
//             interfaceIndex, the interface index
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_StationDisassociateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    int interfaceIndex)
{
    ERROR_Assert(
        dot11->mp->activeProtocol.pathProtocol ==
            DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_StationAssociateFunction: HWMP is not the active protocol.\n");
    HWMP_Data* hwmp = (HWMP_Data*)(dot11->mp->activeProtocol.data);

    // Set flag for proxy update.
    if (hwmp->fwdState == HWMP_TBR_FWD_FORWARDING)
    {
        hwmp->stationUpdateIsDue = TRUE;
    }

    // Send route error for station.
    HWMP_RerrData rerrData;
    ListInit(node, &rerrData.destInfoList);

    HWMP_AddrSeqInfo* destAddrSeq;
    Dot11s_Malloc(HWMP_AddrSeqInfo, destAddrSeq);
    destAddrSeq->addr = staAddr;
    destAddrSeq->seqNo = hwmp->seqNo;
    ListAppend(node, rerrData.destInfoList, 0, destAddrSeq);

    Hwmp_SendRerrFrame(node, dot11, hwmp, &rerrData,
        FALSE, INVALID_802ADDRESS);

    ListFree(node, rerrData.destInfoList, FALSE);

    hwmp->stats->rerrInitiated++;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_RouterFunction
// PURPOSE   : Determine the routing action to take for a given data packet
//             set packetWasRouted variable to TRUE if no further handling
//             of this packet is necessary
// ARGUMENTS : frameInfo, Details of frame to be routed
//             packetWasRouted, set to FALSE if no route based
//                  forwarding is required, otherwise TRUE
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_RouterFunction(
    Node *node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo,
    BOOL *packetWasRouted)
{
    DOT11s_Data* mp = dot11->mp;

    // Ensure that HWMP is the active protocol.
    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_RouterFunction: HWMP is not the active protocol.\n");

    // Should be a data packet, not a control packet.
    ERROR_Assert(
        frameInfo->frameType == DOT11_MESH_DATA
        || frameInfo->frameType == DOT11_DATA,
        "Hwmp_RouterFunction: Not a data frame.\n");

    BOOL isLocalDA =
        Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->DA);
    BOOL isLocalSA =
        Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->SA);
    if (isLocalDA && isLocalSA)
    {
        // This could occur as reassociation does not
        // use a request-response mechanism. The
        // reassociation notification may not have
        // reached this MAP.
        if (HWMP_TraceComments) {
            char traceStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &frameInfo->SA);
            sprintf(traceStr, "!! Hwmp_RouterFunction: "
                "Station %s is not within BSS.", addrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
        Dot11s_StationDisassociateEvent(node, dot11, frameInfo->SA);
    }

    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);
    HWMP_RouteEntry* rtToDest = NULL;
    BOOL rtIsValid = FALSE;
    BOOL treatAsIntermediateOrDest;

    *packetWasRouted =
        (Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->DA)
        == FALSE);

    treatAsIntermediateOrDest =
        (Dot11s_IsSelfOrBssStationAddr(node, dot11, frameInfo->SA)
        == FALSE);

    if (treatAsIntermediateOrDest)
    {
        Hwmp_ReceiveData(node, dot11, hwmp, frameInfo);
    }
    else
    {
        rtToDest =
            HwmpRouteTable_FindEntry(node, dot11, hwmp,
            frameInfo->DA, &rtIsValid);

        if (rtIsValid)
        {
            if (HWMP_TraceComments) {
                MacDot11Trace(node, dot11, NULL,
                    "Found valid route, send immediately");
            }

            Hwmp_RelayDataFrame(node, dot11, hwmp,
                rtToDest, frameInfo);

            hwmp->stats->dataInitiated++;

        }
        else if (hwmp->fwdState == HWMP_TBR_FWD_FORWARDING
                && HwmpTbr_RelayDataFrame(node, dot11, hwmp, frameInfo))
        {
            if (HWMP_TraceComments) {
                MacDot11Trace(node, dot11, NULL,
                    "No route, forwarding along tree");
            }

            hwmp->stats->dataInitiated++;
        }
        else if (HwmpSentTable_FindEntry(node, dot11, hwmp,
                 frameInfo->DA) == FALSE)
        {
            if (HwmpPktBuffer_Insert(node, dot11, hwmp, frameInfo))
            {
                if (HWMP_TraceComments) {
                    MacDot11Trace(node, dot11, NULL,
                        "No route, buffer packet & send RREQ.");
                }
                Hwmp_SendTriggeredRreq(node, dot11, hwmp, frameInfo);
            }
        }
        else
        {
            if (HWMP_TraceComments) {
                MacDot11Trace(node, dot11, NULL,
                    "RREQ already sent, buffer packet.");
            }

            HwmpPktBuffer_Insert(node, dot11, hwmp, frameInfo);
        }
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_InitActiveProtocol
// PURPOSE   : Initialize HWMP as the active protocol
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_InitActiveProtocol(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_RouterFunction: HWMP is not the active protocol.\n");

    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);

    Hwmp_MemoryChunkAlloc(node, dot11, hwmp);

    // Initialize message buffer
    (&hwmp->msgBuffer)->head = NULL;
    (&hwmp->msgBuffer)->size = 0;
    (&hwmp->msgBuffer)->numByte = 0;

    // Initialize routing table
    for (int i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
    {
        (&hwmp->routeTable)->routeHash[i] = NULL;
    }

    (&hwmp->routeTable)->routeExpireHead = NULL;
    (&hwmp->routeTable)->routeExpireTail = NULL;
    (&hwmp->routeTable)->routeDeleteHead = NULL;
    (&hwmp->routeTable)->routeDeleteTail = NULL;

    (&hwmp->routeTable)->size = 0;

    // Initialize store for RREQ related info
    (&hwmp->seenTable)->front = NULL;
    (&hwmp->seenTable)->rear = NULL;
    (&hwmp->seenTable)->lastFound = NULL;
    (&hwmp->seenTable)->size = 0;

    // Initialize buffer of destinations to which RREQ has been sent
    for (int i = 0; i < HWMP_SENT_HASH_TABLE_SIZE; i++)
    {
        (&hwmp->sentTable)->sentHash[i] = NULL;
    }
    (&hwmp->sentTable)->size = 0;

    // Initialize Hwmp sequence number
    hwmp->seqNo = 1;

    // Initialize Hwmp Broadcast id
    hwmp->rreqId = 1;

    // Initialize when last broadcast was sent
    hwmp->lastBroadcastSent = (clocktype) 0;

    // Initialize black listed neighbors
    //(&hwmp->blacklistTable)->head = NULL;
    //(&hwmp->blacklistTable)->size = 0;

    Dot11s_MallocMemset0(HWMP_Stats, hwmp->stats);

    // Set the router function
    mp->activeProtocol.routerFunction = Hwmp_RouterFunction;

    // Set the active protocol interface functions
    mp->activeProtocol.linkFailureFunction = Hwmp_LinkFailureFunction;
    mp->activeProtocol.linkUpdateFunction = Hwmp_LinkUpdateFunction;
    mp->activeProtocol.linkCloseFunction = Hwmp_LinkCloseFunction;
    mp->activeProtocol.pathUpdateFunction = Hwmp_PathUpdateFunction;
    mp->activeProtocol.stationAssociationFunction =
        Hwmp_StationAssociateFunction;
    mp->activeProtocol.stationDisassociationFunction =
        Hwmp_StationDisassociateFunction;

    // Start the proactive routing state machine.
    ListInit(node, &(hwmp->parentList));
    HwmpTbr_SetState(node, dot11, hwmp, HWMP_TBR_S_INIT);
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReadUserConfigurationForRreqExpandingRing
// PURPOSE   : Read and initialize the user configurable parameters
//             for expanding ring search.
// ARGUMENTS : nodeInput, pointer to user configuration data
//             address, pointer to Address structure
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReadUserConfigurationForRreqExpandingRing(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const NodeInput* nodeInput,
    Address* address)
{
    BOOL wasFound = FALSE;
    int inputInt;
    BOOL isZeroValid = FALSE;
    BOOL isNegativeValid = FALSE;
    // Read Route Discovery Initial RREQ TTL.
    //       MAC-DOT11s-HWMP-RREQ-TTL-INITIAL <integer>
    // Default is HWMP_RREQ_INITIAL_TTL_DEFAULT

    hwmp->rreqTtlInitial = HWMP_RREQ_INITIAL_TTL_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-TTL-INITIAL",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {

        if (inputInt > HWMP_NET_DIAMETER)
        {
            ERROR_ReportError(
                "Hwmp_ReadUserConfigurationForRreqExpandingRing.\n"
                "Value of RREQ inital TTL exceeds net diameter.\n"
                "Check MAC-DOT11s-HWMP-RREQ-TTL-INITIAL.\n");
        }

        hwmp->rreqTtlInitial = inputInt;
    }

    // Read Route Discovery RREQ TTL increment.
    //       MAC-DOT11s-HWMP-RREQ-TTL-INCREMENT <integer>
    // Default is HWMP_RREQ_TTL_INCREMENT_DEFAULT
    hwmp->rreqTtlIncrement = HWMP_RREQ_TTL_INCREMENT_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-TTL-INCREMENT",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {
        hwmp->rreqTtlIncrement = inputInt;
    }

    // Read Route Discovery RREQ TTL threshold.
    //       MAC-DOT11s-HWMP-RREQ-TTL-THRESHOLD <integer>
    // Default is HWMP_RREQ_TTL_THRESHOLD_DEFAULT

    hwmp->rreqTtlThreshold =
        MIN(HWMP_RREQ_TTL_THRESHOLD_DEFAULT, HWMP_NET_DIAMETER);

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-TTL-THRESHOLD",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {


        if (inputInt < HWMP_RREQ_INITIAL_TTL
            || inputInt > HWMP_NET_DIAMETER)
        {
            ERROR_ReportError(
                "Hwmp_ReadUserConfigurationForRreqExpandingRing.\n"
                "Invalid value of RREQ TTL threshold.\n"
                "Value should be within initial TTL and net diameter..\n");
        }

        hwmp->rreqTtlThreshold = inputInt;
    }

    // Read Route Discovery RREQ attempt at max TTL.
    //       MAC-DOT11s-HWMP-RREQ-MAX-TTL-ATTEMPTS <integer>
    // Default is HWMP_RREQ_ATTEMPTS_EXPANDING_RING_DEFAULT
    hwmp->rreqAttempts = HWMP_RREQ_ATTEMPTS_EXPANDING_RING_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-MAX-TTL-ATTEMPTS",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {
        hwmp->rreqAttempts = inputInt;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReadUserConfigurationForRreqFullTtl
// PURPOSE   : Read and initialize the user configurable parameters
//             for full TTL search.
// AUGUMENTS : nodeInput, pointer to user configuration data
//             address, pointer to Address structure
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReadUserConfigurationForRreqFullTtl(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    const NodeInput* nodeInput,
    Address* address)
{
    BOOL wasFound = FALSE;
    int inputInt;
    BOOL isZeroValid = FALSE;
    BOOL isNegativeValid = FALSE;

    hwmp->rreqTtlInitial = HWMP_NET_DIAMETER;
    hwmp->rreqTtlIncrement = 1;
    hwmp->rreqTtlThreshold = HWMP_NET_DIAMETER;

    // Read Route Discovery RREQ attempts at max TTL.
    //       MAC-DOT11s-HWMP-RREQ-MAX-TTL-ATTEMPTS <integer>
    // Default is HWMP_RREQ_ATTEMPTS_EXPANDING_RING_DEFAULT
    hwmp->rreqAttempts = HWMP_RREQ_ATTEMPTS_FULL_TTL_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-MAX-TTL-ATTEMPTS",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {
        hwmp->rreqAttempts = inputInt;
    }
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_ReadUserConfiguration
// PURPOSE   : Read and initialize the user configurable parameters
// ARGUMENTS : nodeInput, pointer to user configuration data
//             address, pointer to Address structure
// RETURN    : None
//--------------------------------------------------------------------

static
void Hwmp_ReadUserConfiguration(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    HWMP_Data* hwmp,
    Address* address)
{
    BOOL wasFound = FALSE;
    char inputStr[MAX_STRING_LENGTH];
    int inputInt;
    clocktype inputTime;
    BOOL isZeroValid = FALSE;
    BOOL isNegativeValid = FALSE;

    // Read HWMP-ACTIVE-ROUTE-TIMEOUT
    //       MAC-DOT11s-HWMP-ACTIVE-ROUTE-TIMEOUT <time>
    // Default is HWMP_ACTIVE_ROUTE_TIMEOUT_DEFAULT.
    hwmp->activeRouteTimeout = HWMP_ACTIVE_ROUTE_TIMEOUT_DEFAULT;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-ACTIVE-ROUTE-TIMEOUT",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        if (inputTime < HWMP_NET_TRAVERSAL_TIME)
        {
            ERROR_ReportWarning("Hwmp_ReadUserConfiguration: "
                "Value of HWMP Active Route Timeout is low.\n"
                "Should be >= 2 * Net Diameter * Node Traversal Time.\n"
                "Check MAC-DOT11s-HWMP-ACTIVE-ROUTE-TIMEOUT.\n");
        }

        hwmp->activeRouteTimeout = inputTime;
    }

    // Read HWMP-MY-ROUTE-TIMEOUT
    //       MAC-DOT11s-HWMP-MY-ROUTE-TIMEOUT <time>
    // Default is 2 * Active Route Timeout
    hwmp->myRouteTimeout = 2 * HWMP_ACTIVE_ROUTE_TIMEOUT;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-MY-ROUTE-TIMEOUT",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        if (inputTime < HWMP_ACTIVE_ROUTE_TIMEOUT)
        {
            ERROR_ReportWarning("Hwmp_ReadUserConfiguration: "
                "Value of HWMP My Route Timeout is low.\n"
                "Should be >= Active Route Timeout.\n"
                "Check MAC-DOT11s-HWMP-MY-ROUTE-TIMEOUT.\n");
        }

        hwmp->myRouteTimeout = inputTime;
    }

    // Read Route Deletion Constant
    //       MAC-DOT11s-HWMP-ROUTE-DELETION-CONSTANT <integer>
    // Default is HWMP_ROUTE_DELETION_CONSTANT_DEFAULT
    hwmp->routeDeletionConstant = HWMP_ROUTE_DELETION_CONSTANT_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-ROUTE-DELETION-CONSTANT",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {
        hwmp->routeDeletionConstant = inputInt;
    }

    // Read HWMP-REVERSE-ROUTE-TIMEOUT
    //       MAC-DOT11s-HWMP-REVERSE-ROUTE-TIMEOUT <time>
    // Default is Active Route Timeout
    hwmp->reverseRouteTimeout = HWMP_ACTIVE_ROUTE_TIMEOUT;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-HWMP-REVERSE-ROUTE-TIMEOUT",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        if (inputTime < HWMP_NET_TRAVERSAL_TIME)
        {
            ERROR_ReportWarning("Hwmp_ReadUserConfiguration: "
                "Value of HWMP Reverse Route Timeout is low.\n"
                "Should be >= 2 * Net Diameter * Node Traversal Time.\n"
                "Check MAC-DOT11s-HWMP-REVERSE-ROUTE-TIMEOUT.\n");
        }

        hwmp->reverseRouteTimeout = inputTime;
    }

    // Read Adhoc Route Discovery Type
    //       MAC-DOT11s-HWMP-ROUTE-DISCOVERY-TYPE <string>
    // Default is HWMP_RREQ_EXPANDING_RING
    hwmp->rreqType = HWMP_RREQ_EXPANDING_RING;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-HWMP-ROUTE-DISCOVERY-TYPE",
        &wasFound,
        inputStr);

    if (wasFound)
    {
        if (strlen(inputStr) == 0)
        {
            ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-HWMP-ROUTE-DISCOVERY-TYPE "
                "in configuration file.\n"
                "Value is missing.\n");
        }

        if (strcmp(inputStr, "EXPANDING-RING") == 0)
        {
            hwmp->rreqType = HWMP_RREQ_EXPANDING_RING;
        }
        else if (strcmp(inputStr, "FULL-TTL") == 0)
        {
            hwmp->rreqType = HWMP_RREQ_FULL_TTL;
        }
        else
        {
            ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-HWMP-ROUTE-DISCOVERY-TYPE "
                "in configuration file.\n"
                "Options are EXPANDING-RING and FULL-TTL.\n");
        }
    }

    if (hwmp->rreqType == HWMP_RREQ_EXPANDING_RING)
    {
        Hwmp_ReadUserConfigurationForRreqExpandingRing(
            node, dot11, hwmp, nodeInput, address);
    }
    else if (hwmp->rreqType == HWMP_RREQ_FULL_TTL)
    {
        Hwmp_ReadUserConfigurationForRreqFullTtl(
            node, dot11, hwmp, nodeInput, address);
    }

    // Read RREQ destination only flag
    //       MAC-DOT11s-HWMP-RREQ-DESTINATION-ONLY <YES | NO>
    // Default is HWMP_RREQ_DO_FLAG_DEFAULT
    hwmp->rreqDoFlag = HWMP_RREQ_DO_FLAG_DEFAULT;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-DESTINATION-ONLY",
        &wasFound,
        inputStr);

    if (wasFound) {
        if (!strcmp(inputStr, "YES"))
        {
            hwmp->rreqDoFlag = TRUE;
        }
        else if (!strcmp(inputStr, "NO"))
        {
            hwmp->rreqDoFlag = FALSE;
        }
        else
        {
            ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-HWMP-RREQ-DESTINATION-ONLY "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    // Read RREQ reply-and-forward flag
    //       MAC-DOT11s-HWMP-RREQ-REPLY-AND-FORWARD <YES | NO>
    // Default is HWMP_RREQ_RF_FLAG_DEFAULT
    hwmp->rreqRfFlag = HWMP_RREQ_RF_FLAG_DEFAULT;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-HWMP-RREQ-REPLY-AND-FORWARD",
        &wasFound,
        inputStr);

    if (wasFound) {
        if (!strcmp(inputStr, "YES"))
        {
            hwmp->rreqRfFlag = TRUE;
        }
        else if (!strcmp(inputStr, "NO"))
        {
            hwmp->rreqRfFlag = FALSE;
        }
        else
        {
            ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-HWMP-REPLY-AND-FORWARD "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    // Read HWMP Root configuration
    //       MAC-DOT11s-HWMP-ROOT <YES | NO>
    // Default is NO.
    hwmp->isRoot = FALSE;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-HWMP-ROOT",
        &wasFound,
        inputStr);

    if (wasFound) {
        if (!strcmp(inputStr, "YES"))
        {
            hwmp->isRoot = TRUE;
        }
        else if (!strcmp(inputStr, "NO"))
        {
            hwmp->isRoot = FALSE;
        }
        else
        {
            ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-HWMP-ROOT "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    if (hwmp->isRoot)
    {
        // Read Root announcement period.
        //       MAC-DOT11s-HWMP-ROOT-ANNOUNCEMENT-PERIOD <time>
        // Default is HWMP_RANN_PERIOD_DEFAULT
        hwmp->rannPeriod = HWMP_RANN_PERIOD_DEFAULT;

        Dot11sIO_ReadTime(
            node,
            node->nodeId,
            address,
            nodeInput,
            "MAC-DOT11s-HWMP-ROOT-ANNOUNCEMENT-PERIOD",
            &wasFound,
            &inputTime,
            isZeroValid);

        if (wasFound)
        {
            hwmp->rannPeriod = inputTime;
        }

        // Read Active Root timeout.
        //       MAC-DOT11s-HWMP-ROOT-TIMEOUT <time>
        // Default is same as on-demand route timeout.

        hwmp->activeRootTimeout = hwmp->myRouteTimeout;

        Dot11sIO_ReadTime(
            node,
            node->nodeId,
            address,
            nodeInput,
            "MAC-DOT11s-HWMP-ROOT-TIMEOUT",
            &wasFound,
            &inputTime,
            isZeroValid);

        if (wasFound)
        {
            if (inputTime <= hwmp->rannPeriod)
            {
                ERROR_ReportWarning("Hwmp_ReadUserConfiguration: "
                    "Value of HWMP Root Timeout is low.\n"
                    "Should be more than Root Announcement Period.\n"
                    "Check MAC-DOT11s-HWMP-ROOT-TIMEOUT.\n");
            }

            hwmp->activeRootTimeout = inputTime;
        }

        // Read root registration setting
        //       MAC-DOT11s-HWMP-ROOT-REGISTRATION <YES | NO>
        // Default is HWMP_ROOT_REGISTRATION_DEFAULT
        hwmp->rootRegistrationFlag = HWMP_ROOT_REGISTRATION_DEFAULT;

        IO_ReadString(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11s-HWMP-ROOT-REGISTRATION",
            &wasFound,
            inputStr);

        if (wasFound) {
            if (!strcmp(inputStr, "YES"))
            {
                hwmp->rootRegistrationFlag = TRUE;
            }
            else if (!strcmp(inputStr, "NO"))
            {
                hwmp->rootRegistrationFlag = FALSE;
            }
            else
            {
                ERROR_ReportError("Hwmp_ReadUserConfiguration: "
                    "Invalid value for MAC-DOT11s-HWMP-ROOT-REGISTRATION "
                    "in configuration file.\n"
                    "Expecting YES or NO.\n");
            }
        }
    }

    // HWMP packet buffer limit uses number of packets.
    hwmp->bufferSizeInNumPacket = HWMP_DEFAULT_MESSAGE_BUFFER_IN_PKT;
    hwmp->bufferSizeInByte = 0;

    hwmp->printStats = TRUE;

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_Init
// PURPOSE   : Initialization function for HWMP protocol
// ARGUMENTS : nodeInput, pointer to user configuration data
//             address, pointer to link layer address
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_Init(
    Node* node,
    MacDataDot11* dot11,
    const NodeInput* nodeInput,
    Address* address,
    NetworkType networkType)
{
    DOT11s_Data* mp = dot11->mp;
    HWMP_Data* hwmp;
    Dot11s_MallocMemset0(HWMP_Data, hwmp);

    // Read user configurable parameters or
    // initialize them with default values.
    Hwmp_ReadUserConfiguration(node, nodeInput, dot11, hwmp, address);

    mp->activeProtocol.data = hwmp;
}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_Finalize
// PURPOSE   : Called at the end of the simulation to collect the results
// ARGUMENTS : None
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;

    // Invalid in a multi-profile situation.
    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_Finalize: HWMP is not the active protocol.\n");

    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);

    // If HWMP was not the selected path protocol,
    // free minimal memory, do not print stats.
    if (mp->activeProtocol.isInitialized == FALSE)
    {
        MEM_free(hwmp);
        return;
    }

    int interfaceIndex = dot11->myMacData->interfaceIndex;

    char buf[MAX_STRING_LENGTH];

    if (hwmp->printStats)
    {
        sprintf(buf, "HWMP RREQ broadcasts initiated = %u",
            hwmp->stats->rreqBcInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQ unicasts initiated = %u",
            hwmp->stats->rreqUcInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs retried = %u",
            hwmp->stats->rreqResent);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs forwarded = %u",
            hwmp->stats->rreqRelayed);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs received = %u",
            hwmp->stats->rreqReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs received (destination) = %u",
            hwmp->stats->rreqReceivedAsDest);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs discarded (duplicate) = %u",
            hwmp->stats->rreqDuplicate);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREQs discarded (TTL expired) = %u",
            hwmp->stats->rreqTtlExpired);
        DOT11s_STATS_PRINT;

        //sprintf(buf, "HWMP RREQs discarded (blacklist) = %u",
        //    hwmp->stats->rreqBlacklist);
        //DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREPs initiated (destination) = %u",
            hwmp->stats->rrepInitiatedAsDest);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREPs initiated (intermediate) = %u",
            hwmp->stats->rrepInitiatedAsIntermediate);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREPs forwarded = %u",
            hwmp->stats->rrepForwarded);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREPs received = %u",
            hwmp->stats->rrepReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RREPs received (source) = %u",
            hwmp->stats->rrepReceivedAsSource);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Gratuitous RREPs initiated = %u",
            hwmp->stats->rrepGratInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Gratuitous RREPs forwarded = %u",
            hwmp->stats->rrepGratForwarded);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Gratuitous RREPs received = %u",
            hwmp->stats->rrepGratReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RERRs initiated = %u",
            hwmp->stats->rerrInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RERRs forwarded = %u",
            hwmp->stats->rerrForwarded);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RERRs received = %u",
            hwmp->stats->rerrReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RERRs discarded = %u",
            hwmp->stats->rerrDiscarded);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RANNs initiated = %u",
            hwmp->stats->rannInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RANNs forwarded = %u",
            hwmp->stats->rannRelayed);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RANNs received = %u",
            hwmp->stats->rannReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RANNs discarded (duplicate) = %u",
            hwmp->stats->rannDuplicate);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP RANNs discarded (TTL expired) = %u",
            hwmp->stats->rannTtlExpired);
        DOT11s_STATS_PRINT;

        //sprintf(buf, "HWMP RANNs discarded (blacklist) = %u",
        //    hwmp->stats->rannBlacklist);
        //DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Data packets sent (source or BSS) = %u",
            hwmp->stats->dataInitiated);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Data packets forwarded = %u",
            hwmp->stats->dataForwarded);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Data packets received = %u",
            hwmp->stats->dataReceived);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Data packets dropped (no route) = %u",
            hwmp->stats->dataDroppedForNoRoute);
        DOT11s_STATS_PRINT;

        sprintf(buf, "HWMP Data packets dropped (buffer overflow) = %u",
            hwmp->stats->dataDroppedForOverlimit);
        DOT11s_STATS_PRINT;

        //sprintf(buf, "HWMP Packets dropped (max hop count) = %u",
        //    hwmp->stats->numMaxHopExceed);
        //DOT11s_STATS_PRINT;

        //sprintf(buf,
        //    "HWMP Total hop counts for all routes = %u",
        //    hwmp->stats->numHops);
        //DOT11s_STATS_PRINT;

        //sprintf(buf, "HWMP Number of routes selected = %u",
        //    hwmp->stats->numRoutes);
        //DOT11s_STATS_PRINT;

        //sprintf(buf, "HWMP Number of times link broke = %u",
        //    hwmp->stats->numBrokenLinks);
        //DOT11s_STATS_PRINT;

        //if (HWMP_TraceComments) {
        //    sprintf(buf,
        //        "HWMP Max number of seen RREQs cached at once = %u",
        //        hwmp->stats->numMaxSeenTable);
        //    DOT11s_STATS_PRINT;

        //    sprintf(buf, "HWMP Max number of last found hits = %u",
        //        hwmp->stats->numLastFoundHits);
        //    DOT11s_STATS_PRINT;
        //}

        if (HWMP_DebugRouteTable)
        {
            printf("Routing table at the end of simulation\n");
            HwmpRouteTable_Print(node, dot11, hwmp);
        }
    }

    // Free allocated stats
    MEM_free(hwmp->stats);

    // Blacklist table
    /*
    {
        HWMP_BlacklistTable* blacklistTable = &(hwmp->blacklistTable);
        HWMP_BlacklistNode* current = blacklistTable->head;
        HWMP_BlacklistNode* temp;
        int count = 0;

        while (current)
        {
            temp = current;
            current = current ->next;
            MEM_free(temp);
            count++;
        }

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "Blacklist table finalize: "
                "freed %d items", count);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }
    */

    // Sent table
    {
        HWMP_RreqSentTable* sentTable = &(hwmp->sentTable);
        HWMP_RreqSentNode* current;
        HWMP_RreqSentNode* temp;
        int count = 0;

        for (int i = 0; i < HWMP_SENT_HASH_TABLE_SIZE; i++)
        {
            current = sentTable->sentHash[i];
            while (current)
            {
                temp = current;
                current = current->hashNext;
                MEM_free(temp);
                count++;
            }
        }

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "Sent table finalize: "
                "freed %d items", count);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    // Seen table
    {
        HWMP_RreqSeenTable* seenTable = &(hwmp->seenTable);
        HWMP_RreqSeenNode* current = seenTable->front;
        HWMP_RreqSeenNode* temp;
        int count = 0;

        while (current)
        {
            temp = current;
            current = current->next;
            MEM_free(temp);
            count++;
        }

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "Seen table finalize: "
                "freed %d items", count);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    // Route table
    {
        HWMP_RoutingTable* routeTable = &(hwmp->routeTable);
        HWMP_RouteEntry* current;
        HWMP_RouteEntry* temp;

        for (int i = 0; i < HWMP_ROUTE_HASH_TABLE_SIZE; i++)
        {
            current = routeTable->routeHash[i];
            while (current)
            {
                temp = current;
                current = current->hashNext;
                HwmpPrecursorTable_DeleteAll(node, dot11, hwmp, temp);
                Hwmp_MemoryFree(node, dot11, hwmp, temp);
            }
        }

        //MEM_free(hwmp->refFreeList);
    }


    // Message buffer
    {
        HWMP_MsgBuffer* buffer = &(hwmp->msgBuffer);
        HWMP_BufferNode* current = buffer->head;
        HWMP_BufferNode* temp;
        DOT11s_FrameInfo* frameInfo = NULL;
        int count = 0;

        while (current)
        {
            temp = current;
            frameInfo = current->frameInfo;
            Dot11s_MemFreeFrameInfo(node, &frameInfo, TRUE);
            current = current->next;
            MEM_free(temp);
            count++;
        }

        if (HWMP_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "HWMP message buffer finalize: "
                "freed %d items", count);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    // TBR Parent list
    HwmpTbrParentList_Finalize(node, dot11, hwmp);

    MEM_free(hwmp);

}


//--------------------------------------------------------------------
// FUNCTION  : Hwmp_HandleProtocolEvent
// PURPOSE   : Handle protocol events
// ARGUMENTS : msg,  message containing the event type
// RETURN    : None
//--------------------------------------------------------------------

void Hwmp_HandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;

    ERROR_Assert(
        mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP,
        "Hwmp_HandleTimeout: HWMP is not the active protocol.\n");
    HWMP_Data* hwmp = (HWMP_Data*)(mp->activeProtocol.data);

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_MAC_DOT11s_HwmpSeenTableTimeout:
        {
            // Remove an entry from the RREQ Seen Table

            if (hwmp->seenTable.front != NULL)
            {
                if (HWMP_TraceComments)
                {
                    char traceStr[MAX_STRING_LENGTH];
                    char sourceStr[MAX_STRING_LENGTH];
                    Dot11s_AddrAsDotIP(sourceStr,
                        &hwmp->seenTable.front->sourceAddr);
                    sprintf(traceStr, "Deleting from seen table(%d), "
                        "source: %s, RREQ ID: %d",
                        hwmp->seenTable.size,
                        sourceStr,
                        hwmp->seenTable.front->rreqId);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                HwmpSeenTable_DeleteEntry(node, dot11, hwmp);
            }
            else
            {
                if (HWMP_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "!! Hwmp_HandleTimeout: "
                        "Seen table entry not found");
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_MAC_DOT11s_HwmpActiveRouteTimeout:
        {
            // Remove the route that has not been used for a while

            HWMP_RoutingTable* routeTable = &hwmp->routeTable;
            HWMP_RouteEntry* current = routeTable->routeExpireHead;
            HWMP_RouteEntry* rtPtr = current;

            while (current && current->lifetime <= node->getNodeTime())
            {
                rtPtr = current;
                current = current->expireNext;

                // Disable route and delete after delete period
                HwmpRouteTable_DisableRoute(node, dot11, hwmp,
                    rtPtr, TRUE);
            }

            if (current == NULL)
            {
                hwmp->isExpireTimerSet = FALSE;

                MESSAGE_Free(node, msg);
            }
            else
            {
                MESSAGE_Send(node, msg,
                    (current->lifetime - node->getNodeTime()));
            }

            break;
        }

        case MSG_MAC_DOT11s_HwmpDeleteRouteTimeout:
        {
            HWMP_RoutingTable* routeTable = &hwmp->routeTable;
            HWMP_RouteEntry* current = routeTable->routeDeleteHead;
            HWMP_RouteEntry* rtPtr = current;

            while (current && current->lifetime <= node->getNodeTime())
            {
                rtPtr = current;
                current = current->deleteNext;

                if (HWMP_TraceComments)
                {
                    char traceStr[MAX_STRING_LENGTH];
                    char destStr[MAX_STRING_LENGTH];
                    char nextHopStr[MAX_STRING_LENGTH];
                    Dot11s_AddrAsDotIP(destStr, &rtPtr->addrSeqInfo.addr);
                    Dot11s_AddrAsDotIP(nextHopStr, &rtPtr->nextHop);
                    sprintf(traceStr,
                        "Deleting route to dest %s, nextHop %s",
                        destStr, nextHopStr);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                HwmpRouteTable_DeleteEntry(node, dot11, hwmp, rtPtr);
            }

            if (current == NULL)
            {
                hwmp->isDeleteTimerSet = FALSE;

                MESSAGE_Free(node, msg);
            }
            else
            {
                MESSAGE_Send(node, msg,
                    (current->lifetime - node->getNodeTime()));
            }

            break;
        }

        case MSG_MAC_DOT11s_HwmpRreqReplyTimeout:
        {
            DOT11s_TimerInfo* timerInfo =
                (DOT11s_TimerInfo*) (MESSAGE_ReturnInfo(msg));
            Mac802Address destAddr = timerInfo->addr;

            if (HwmpSentTable_FindEntry(node, dot11, hwmp, destAddr))
            {
                BOOL rtIsValid = FALSE;
                HWMP_RouteEntry* rtEntry = NULL;

                rtEntry = HwmpRouteTable_FindEntry(node, dot11, hwmp,
                    destAddr, &rtIsValid);

                if (HwmpSentTable_GetSentCountWithMaxTtl(
                            node, dot11, hwmp, destAddr)
                    < HWMP_RREQ_ATTEMPTS)
                {
                    // If the RREP is not received within
                    // NET_TRAVERSAL_TIME milliseconds, the node MAY try
                    // again to flood the RREQ, up to a maximum of
                    // RREQ_ATTEMPTS times.  Each new attempt MUST
                    // increment the Flooding ID field. Sec: 8.3

                    Mac802Address sourceAddr = timerInfo->addr2;
                    Hwmp_RetryRreq(node, dot11, hwmp,
                        destAddr, sourceAddr);
                }
                else
                {
                    if (HWMP_TraceComments) {
                        MacDot11Trace(node, dot11, NULL,
                            "Hwmp_HandleTimeout: "
                            "RREQ reply timeout; aborting");
                    }

                    DOT11s_FrameInfo* frameInfo = NULL;
                    Mac802Address prevHopAddr;

                    // Remove all the messages for destination
                    frameInfo = HwmpPktBuffer_GetPacket(node, dot11, hwmp,
                        destAddr, &prevHopAddr);

                    while (frameInfo)
                    {
                        hwmp->stats->dataDroppedForNoRoute++;

                        Dot11s_MemFreeFrameInfo(node, &frameInfo, TRUE);

                        frameInfo = HwmpPktBuffer_GetPacket(
                            node, dot11, hwmp, destAddr, &prevHopAddr);
                    }

                    // Remove from sent table.
                    HwmpSentTable_DeleteEntry(node, dot11, hwmp,
                        destAddr);
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_MAC_DOT11s_HwmpBlacklistTimeout:
        {
            ERROR_ReportError(
                "Hwmp_HandleTimeout: Blacklist not implemented.\n");

            break;
        }
        case MSG_MAC_DOT11s_HwmpTbrRannTimeout:
        {
            hwmp->tbrStateData.event = HWMP_TBR_S_EVENT_RANN_TIMER;
            hwmp->tbrStateFn(node, dot11, hwmp);

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_MAC_DOT11s_HwmpTbrMaintenanceTimeout:
        {
            hwmp->tbrStateData.event =
                HWMP_TBR_S_EVENT_ROOT_MAINTENANCE_TIMER;
            hwmp->tbrStateFn(node, dot11, hwmp);

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_MAC_DOT11s_HmwpTbrRrepTimeout:
        {
            DOT11s_TimerInfo timerInfo =
                *(DOT11s_TimerInfo*) (MESSAGE_ReturnInfo(msg));
            Mac802Address parentAddr = timerInfo.addr;

            hwmp->tbrStateData.event = HWMP_TBR_S_EVENT_RREP_TIMER;
            hwmp->tbrStateData.parentAddr = parentAddr;
            hwmp->tbrStateFn(node, dot11, hwmp);

            MESSAGE_Free(node, msg);
            break;
        }
        default:
        {
            ERROR_ReportError(
                "Hwmp_HandleTimeout: Unknown message type.\n");
            break;
        }
    }
}

//=============================================================

