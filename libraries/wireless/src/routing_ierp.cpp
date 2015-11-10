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
#include "routing_ierp.h"

//UserCodeStartBodyData
#include <limits.h>
#include "routing_brp.h"

#define IERP_INITIAL_ROUTE_TABLE_SIZE   8
#define IERP_HEADER_SIZE   8
#define IERP_INITIAL_QUERY_PACKET_SIZE   8
#define IERP_INITIAL_QUERY_ID   0
#define IERP_DEFAULT_ROUTE_REQ_TIMER   (30 * SECOND)
#define IERP_DEFAULT_ROUTE_AGE_OUT_TIME   (2 * MINUTE)
#define IERP_DEBUG_QUERY   0
#define IERP_DEBUG_REPLY   0
#define NetworkIpGetRoutingProtocol(n,p)   IerpGetRoutingProtocol(n,p)
#define IERP_DEFAULT_MAX_MESSAGE_BUFFER_SIZE   100


//UserCodeEndBodyData

//Statistic Function Declarations
static void IerpInitStats(Node* node, IerpStatsType *stats);
void IerpPrintStats(Node *node);

//Utility Function Declarations
static void IerpPrintQueryPacket(Node* node,IerpGeneralHeader ierpHeader,char* queryPacket);
static void IerpScheduleRouteQueryTimer(Node* node,IerpData* data,NodeAddress nodeAddress);
static void IerpInitializeRoutingTable(IerpRoutingTable* ierpRouteTable);
static void IerpScheduleRouteFlushOutTimer(Node* node,IerpData* data);
static IerpRoutingTableEntry* IerpAddRouteToRoutingTable(Node* node,IerpRoutingTable* ierpRouteTable,NodeAddress destinationAddress,NodeAddress subnetMask,NodeAddress* path,int length);
static void IerpDeleteRouteFromRoutingTable(IerpRoutingTable* ierpRouteTable,NodeAddress destAddress);
static IerpRoutingTableEntry* IerpSearchRouteInRoutingTable(IerpRoutingTable* ierpRoutingTable,NodeAddress destAddress);
static void IerpRegisterIarpUpdate(Node* node,NodeAddress destinationAddress,NodeAddress subnetMask,NodeAddress path[],int length);
static IerpSeqTable* IerpSearchLinkSourceInSeqTable(IerpSeqTable* root,NodeAddress linkSource);
static IerpSeqTable* IerpInsertIntoSeqTable(IerpSeqTable* root,NodeAddress linkSource,unsigned short seqNum);
static unsigned short IerpIncrementQueryId(IerpData* ierpData);
static void IerpAdvancePacketPtr(char* * packetPtr,int numBytes);
static void IerpEncapsulatePacketInTheMessage(Node* node,Message* * msg,PacketBuffer* packetBuffer);
static void IerpBuildHeader(IerpData* ierpData,char* packet,int initialLength);
static void IerpBuildInitialQueryPacket(Node* node,IerpData* iarpData,char* packet,NodeAddress destInSearch);
static void IerpCreateRouteRequestPacket(Node* node,IerpData* ierpData,PacketBuffer* * packetBuffer,NodeAddress nodeAddress);
static void IerpSendRouteQueryPacket(Node* node,IerpData* ierpData,NodeAddress nodeAddressToSearch,NodeAddress prevHop);
static void IerpGetDestinationQueried(NodeAddress* destAddressQueried,IerpGeneralHeader queryPacketHeader,char* queryPacket);
static void IerpForwardQueryPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,NodeAddress destAddress,char* queryPacket);
static NodeAddress ExtractPreviousHopAddress(NodeAddress route[],unsigned int nodePtr);
static void IerpGenerateReplyPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,char* packet,NodeAddress* path,int pathLength);
static void IerpForwardReplyPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,char* packet,NodeAddress previousHop);
static void IerpReturnPathAndPathLengthAndPreviousHop(IerpGeneralHeader ierpHeader,char* queryPacket,NodeAddress* * route,NodeAddress* prevHopAddress,unsigned int* length);
static void IerpReadHeaderInformation(IerpGeneralHeader* iarpHeader,Message* msg);
static void IerpHandlePacketDrop(Node* node,const Message* msg);
static void IerpReadZoneRadiusFromConfigurationFile(Node* node,const NodeInput* nodeInput,unsigned int* zoneRadiusRead);
static void IerpInsertBuffer(Node* node,Message* msg,NodeAddress destAddr,IerpMessageBuffer* buffer,int maxBuffSize);
static Message* IerpGetBufferedPacket(NodeAddress destAddr,IerpMessageBuffer* buffer);

//State Function Declarations
static void enterIerpProcessIerpQueryTimerExpired(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpSendQueryPacket(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpRoutePacket(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpIdleState(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpProcessReplyPacket(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpProcessQueryPacket(Node* node, IerpData* dataPtr, Message* msg);
static void enterIerpProcessFlushTimerExpired(Node* node, IerpData* dataPtr, Message* msg);

//Utility Function Definitions
//UserCodeStart
// FUNCTION IerpPrintQueryPacket
// PURPOSE  Printing the query packet content
// Parameters:
//  node:   node which is printing the header
//  ierpHeader: Ierp Header
//  queryPacket:    the query packet without header
static void IerpPrintQueryPacket(Node* node,IerpGeneralHeader ierpHeader,char* queryPacket) {
    unsigned int i = 0;
    NodeAddress* nodeAddress = NULL;
    unsigned int totNumRt = (ierpHeader.length - (IERP_HEADER_SIZE / 4));
    FILE* fd = NULL;
    fd = fopen("ierpDebug.out","a");
    fprintf(fd, "-------------------------------------------------\n"
           "node = %u\n"
           "-------------------------------------------------\n"
           "type                     = %u\n"
           "length (in 32 bit words) = %u\n"
           "nodePtr                  = %u\n"
           "reserved1                = %u\n"
           "query Id                 = %u\n"
           "resrved2                 = %u\n"
           "-------------------------------------------------\n",
           node->nodeId,
           ierpHeader.type,
           ierpHeader.length,
           ierpHeader.nodePtr,
           ierpHeader.reserved1,
           ierpHeader.queryId,
           ierpHeader.reserved2);
    nodeAddress = (NodeAddress*) queryPacket;
    for (i = 0; i < totNumRt; i++)
    {
        char ipString[MAX_STRING_LENGTH] = {0};
        IO_ConvertIpAddressToString(nodeAddress[i], ipString);
        fprintf(fd, "%20s\n", ipString);
    }
    fprintf(fd, "-------------------------------------------------\n");
    fclose(fd);
}


// FUNCTION IerpScheduleRouteQueryTimer
// PURPOSE  Scheduling a route query timer
// Parameters:
//  node:   node which is scheduling the query
//  data:   pointee to ierp data structure
//  nodeAddress:    destination for which timer is set.
static void IerpScheduleRouteQueryTimer(Node* node,IerpData* data,NodeAddress nodeAddress) {
    Message* msg = MESSAGE_Alloc(
                       node,
                       NETWORK_LAYER,
                       ROUTING_PROTOCOL_IERP,
                       ROUTING_IerpRouteRequestTimeExpired);
    MESSAGE_InfoAlloc(node, msg, sizeof(NodeAddress));
    *((NodeAddress*) MESSAGE_ReturnInfo(msg)) = nodeAddress;
    MESSAGE_Send(node, msg, data->ierpRouteRequestTimer);
}


// FUNCTION IerpInitializeRoutingTable
// PURPOSE  Initializing Ierp routing table
// Parameters:
//  ierpRouteTable: pointer to ierp route table
static void IerpInitializeRoutingTable(IerpRoutingTable* ierpRouteTable) {
    ierpRouteTable->maxEntries = IERP_INITIAL_ROUTE_TABLE_SIZE;
    ierpRouteTable->numEntries = 0;
    ierpRouteTable->ierpRoutingTableEntry = (IerpRoutingTableEntry*)
        MEM_malloc(ierpRouteTable->maxEntries
            * sizeof(IerpRoutingTableEntry));
    memset(ierpRouteTable->ierpRoutingTableEntry, 0,
        (ierpRouteTable->maxEntries * sizeof(IerpRoutingTableEntry)));
}


// FUNCTION IerpScheduleRouteFlushOutTimer
// PURPOSE  Scheduling a route flush out timer
// Parameters:
//  node:   node which is scheduling the query
//  data:   pointee to ierp data structure
static void IerpScheduleRouteFlushOutTimer(Node* node,IerpData* data) {
    Message* msg = MESSAGE_Alloc(
                       node,
                       NETWORK_LAYER,
                       ROUTING_PROTOCOL_IERP,
                       ROUTING_IerpFlushTimeOutRoutes);
    MESSAGE_Send(node, msg, MINUTE);
}


// FUNCTION IerpAddRouteToRoutingTable
// PURPOSE  node which is adding data to route table
// Parameters:
//  node:   node which is adding data to route table
//  ierpRouteTable: pointer to ierp route table
//  destinationAddress: the destination address
//  subnetMask: subnet mask of the dest address
//  path:   the path list to be stored
//  length: length of the path (in hop count)
static IerpRoutingTableEntry* IerpAddRouteToRoutingTable(Node* node,IerpRoutingTable* ierpRouteTable,NodeAddress destinationAddress,NodeAddress subnetMask,NodeAddress* path,int length) {
    if (ierpRouteTable->numEntries == ierpRouteTable->maxEntries)
    {
        unsigned int newSize = (ierpRouteTable->maxEntries * 2);
        IerpRoutingTableEntry* newIerpRoutingTableEntry =
            (IerpRoutingTableEntry*)
            MEM_malloc(newSize * sizeof(IerpRoutingTableEntry));
        memset(newIerpRoutingTableEntry, 0,
            newSize * sizeof(IerpRoutingTableEntry));
        memcpy(newIerpRoutingTableEntry,
               ierpRouteTable->ierpRoutingTableEntry,
               (ierpRouteTable->numEntries
                   * sizeof(IerpRoutingTableEntry)));
        MEM_free(ierpRouteTable->ierpRoutingTableEntry);
        ierpRouteTable->ierpRoutingTableEntry = newIerpRoutingTableEntry;
        ierpRouteTable->maxEntries = newSize;
    } // end if (ierpRouteTable->numEntries == ierpRouteTable->maxEntries)
    ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
        destinationAddress = destinationAddress;
    ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
        subnetMask = subnetMask;
    if ((length > 0) &&  (path != NULL))
    {
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            pathList = (NodeAddress*) MEM_malloc(
                length * sizeof(NodeAddress));
        memcpy(ierpRouteTable->ierpRoutingTableEntry
                  [ierpRouteTable->numEntries].pathList,
               path,
               (length * sizeof(NodeAddress)));
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            lastUsed = node->getNodeTime();
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            hopCount = length;
    }
    else
    {
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            pathList = NULL;
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            lastUsed = 0;
        ierpRouteTable->ierpRoutingTableEntry[ierpRouteTable->numEntries].
            hopCount = 0;
    }
    (ierpRouteTable->numEntries)++;
    return &(ierpRouteTable->ierpRoutingTableEntry
               [ierpRouteTable->numEntries - 1]);
}


// FUNCTION IerpDeleteRouteFromRoutingTable
// PURPOSE  Deleting a route from route table.
// Parameters:
//  ierpRouteTable: pointer to ierp routing table
//  destAddress:    destination address to be deleted.
static void IerpDeleteRouteFromRoutingTable(IerpRoutingTable* ierpRouteTable,NodeAddress destAddress) {
    int i = 0;
    for (i = 0; i <(signed)ierpRouteTable->numEntries; i++)
    {
        if (ierpRouteTable->ierpRoutingTableEntry[i].destinationAddress ==
            destAddress)
        {
            MEM_free(ierpRouteTable->ierpRoutingTableEntry[i].pathList);

            memmove(&(ierpRouteTable->ierpRoutingTableEntry[i]),
                    &(ierpRouteTable->ierpRoutingTableEntry[
                        ierpRouteTable->numEntries - 1]),
                    sizeof(IerpRoutingTableEntry));
            memset(&(ierpRouteTable->ierpRoutingTableEntry[
                      ierpRouteTable->numEntries - 1]), 0,
                   sizeof(IerpRoutingTableEntry));
            (ierpRouteTable->numEntries)--;
            break;
        }
    }
}


// FUNCTION IerpSearchRouteInRoutingTable
// PURPOSE  Searching for a entry in the routing table of Ierp
// Parameters:
//  ierpRoutingTable:   pointer to routing table entry
//  destAddress:    destination address to be searched
static IerpRoutingTableEntry* IerpSearchRouteInRoutingTable(IerpRoutingTable* ierpRoutingTable,NodeAddress destAddress) {
    int i = 0;
    for (i = 0; i < (signed)ierpRoutingTable->numEntries; i++)
    {
        if (ierpRoutingTable->ierpRoutingTableEntry[i].destinationAddress
            == destAddress)
        {
            return &(ierpRoutingTable->ierpRoutingTableEntry[i]);
        }
    }
    return NULL;
}


// FUNCTION IerpRegisterIarpUpdate
// PURPOSE  Updating the ierp routing table with iarp update
// Parameters:
//  node:   node which is adding data to route table
//  destinationAddress: the destination address
//  subnetMask: subnet mask of the dest address
//  path[]: the path list to be stored
//  length: ength of the path (in hop count)
static void IerpRegisterIarpUpdate(Node* node,NodeAddress destinationAddress,NodeAddress subnetMask,NodeAddress path[],int length) {
    IerpRoutingTableEntry* routingEntry = NULL;
    IerpRoutingTable* ierpRoutingTable = NULL;
    IerpData* dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_IERP);
    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IERP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IERP dataPtr should not be NULL !!!");
        return;
    }
    ierpRoutingTable = &(dataPtr->ierpRoutingTable);
    routingEntry = IerpSearchRouteInRoutingTable(
                       ierpRoutingTable,
                       destinationAddress);
    if (routingEntry == NULL)
    {
        //IerpRoutingTableEntry* pathAdded =
        IerpAddRouteToRoutingTable(
            node,
            ierpRoutingTable,
            destinationAddress,
            subnetMask,
            path,
            length);
    }
    else
    {
         // Path exists and got updated
         routingEntry->destinationAddress = destinationAddress;
         routingEntry->subnetMask = subnetMask;
         routingEntry->lastUsed = node->getNodeTime();
         routingEntry->hopCount = length;
         MEM_free(routingEntry->pathList);
         routingEntry->pathList
             = (NodeAddress *) MEM_malloc(sizeof(NodeAddress)
                                          * routingEntry->hopCount);
         memcpy(routingEntry->pathList, path,
             (sizeof(NodeAddress) * routingEntry->hopCount));
    }
}


// FUNCTION IerpReturnRouteTableUpdateFunction
// PURPOSE  Returning routing table update function pointer
// Parameters:
//  node:   node which will return the function pointer
IerpUpdateFunc IerpReturnRouteTableUpdateFunction(Node* node) {
    return &(IerpRegisterIarpUpdate);
}


// FUNCTION IerpSearchLinkSourceInSeqTable
// PURPOSE  Searching information about a link source address in the
//          link state sequence number table.
// Parameters:
//  root:   pointer to the root of the sequence table.
//  linkSource: linkSource address whos information is to be searched
static IerpSeqTable* IerpSearchLinkSourceInSeqTable(IerpSeqTable* root,NodeAddress linkSource) {
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


// FUNCTION IerpInsertIntoSeqTable
// PURPOSE  Inserting information in sequence table.
// Parameters:
//  root:   pointer to the root of the sequence table.
//  linkSource: linkSource address whos information is to be searched
//  seqNum: ast seqnumber seen.
static IerpSeqTable* IerpInsertIntoSeqTable(IerpSeqTable* root,NodeAddress linkSource,unsigned short seqNum) {
    if (root == NULL)
    {
        IerpSeqTable* seqTable = (IerpSeqTable*)
            MEM_malloc(sizeof(IerpSeqTable));
        seqTable->linkSource = linkSource;
        seqTable->seqNum = seqNum;
        seqTable->leftChild = NULL;
        seqTable->rightChild = NULL;
        return seqTable;
    }
    else if (root->linkSource > linkSource)
    {
        // insert the information in left
        root->leftChild = IerpInsertIntoSeqTable(
                              root->leftChild,
                              linkSource,
                              seqNum);
    }
    else if (root->linkSource < linkSource)
    {
        // insert the information in right
        root->rightChild = IerpInsertIntoSeqTable(
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


// FUNCTION IerpIncrementQueryId
// PURPOSE  Incrementing the ierp query sequence number
// Parameters:
//  ierpData:   pointer to ierp data
static unsigned short IerpIncrementQueryId(IerpData* ierpData) {
    if (ierpData->queryId == USHRT_MAX)
    {
        ierpData->queryId = IERP_INITIAL_QUERY_ID;
    }
    else
    {
        (ierpData->queryId)++;
    }
    return ierpData->queryId;
}


// FUNCTION IerpAdvancePacketPtr
// PURPOSE  advancing the packet ptr by specified number of bytes
// Parameters:
//  packetPtr:  packet pointer to advance
//  numBytes:   number of bytes to advance
static void IerpAdvancePacketPtr(char* * packetPtr,int numBytes) {
    (*packetPtr) = (*packetPtr) + numBytes;
}


// FUNCTION IerpEncapsulatePacketInTheMessage
// PURPOSE  Encapsulate the packet in the message
// Parameters:
//  node:   node which is encasulating the message
//  msg:    message where packet is to be encapsulated
//  packetBuffer:   the buffer that contains the packet
static void IerpEncapsulatePacketInTheMessage(Node* node,Message* * msg,PacketBuffer* packetBuffer) {
    *msg = MESSAGE_Alloc(
               node,
               NETWORK_LAYER,
               ROUTING_PROTOCOL_IERP,
               MSG_DEFAULT);
    MESSAGE_PacketAlloc(node, *msg,
        BUFFER_GetCurrentSize(packetBuffer),
        TRACE_IARP);
    memcpy(MESSAGE_ReturnPacket((*msg)),
           BUFFER_GetData(packetBuffer),
           BUFFER_GetCurrentSize(packetBuffer));
}


// FUNCTION IerpBuildHeader
// PURPOSE  Building header of the Ierp query packet
// Parameters:
//  ierpData:   pointer to ierp data
//  packet: the packet it is building up.
//  initialLength:  initial length of the packet
static void IerpBuildHeader(IerpData* ierpData,char* packet,int initialLength) {
    char* packetPtr = packet;
    char* packetPtrFirst = packet;
    unsigned char packetType = IERP_QUERY_PACKET;
    unsigned char packetLength = (unsigned char)initialLength;
    unsigned short queryId = IerpIncrementQueryId(ierpData);
    // Copy packet type in the header.
    memcpy(packetPtr, &packetType, sizeof(unsigned char));
    IerpAdvancePacketPtr(&packetPtr, sizeof(unsigned char));
    // Copy packet length in the header.
    memcpy(packetPtr, &packetLength, sizeof(unsigned char));
    IerpAdvancePacketPtr(&packetPtr, sizeof(unsigned char));
    // Skip node ptr field and skip "reserved1" field.
    IerpAdvancePacketPtr(&packetPtr,
        (sizeof(unsigned char) + sizeof(unsigned char)));
    // Copy QueryId in the Query Id field of the header.
    memcpy(packetPtr, &queryId, sizeof(unsigned short));
    IerpAdvancePacketPtr(&packetPtr, sizeof(unsigned short));
    // Skip "Reserved2 field of the header"
    IerpAdvancePacketPtr(&packetPtr, sizeof(unsigned short));
    if ((packetPtr - packetPtrFirst) != IERP_HEADER_SIZE)
    {
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Error in building IERP header");
        ERROR_Assert(FALSE, errStr);
        abort();
    }
}


// FUNCTION IerpBuildInitialQueryPacket
// PURPOSE  Building an initial query packet
// Parameters:
//  node:   node which is sending query packet
//  iarpData:   pointer to ierp data
//  packet: pointer to data packet
//  destInSearch:   destination address to be searched
static void IerpBuildInitialQueryPacket(Node* node,IerpData* iarpData,char* packet,NodeAddress destInSearch) {
    char* packetPtr = packet;
    char* packetPtrFirst = packet;
    // Initialy this will only contain following two fields
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |                  Query/Route Source Address                   |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // : (no route data in between...)                                 :
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |                Query/Route Destination Address                |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    NodeAddress sourceAddress = NetworkIpGetInterfaceAddress(
                                    node,
                                    DEFAULT_INTERFACE);
    NodeAddress destinationAddressQueried = destInSearch;
    // Copy query source address in the "Query/Route Source Address"
    // field of the route query packet.
    memcpy(packetPtr, &sourceAddress, sizeof(NodeAddress));
    IerpAdvancePacketPtr(&packetPtr, sizeof(NodeAddress));
    // Copy destination address to be queried "Query/Route Destination
    // Address" field of the route query packet.
    memcpy(packetPtr, &destinationAddressQueried, sizeof(NodeAddress));
    IerpAdvancePacketPtr(&packetPtr, sizeof(NodeAddress));
    if ((packetPtr - packetPtrFirst) != IERP_INITIAL_QUERY_PACKET_SIZE)
    {
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Error in building IERP initial query packet");
        ERROR_Assert(FALSE, errStr);
        abort();
    }
}


// FUNCTION IerpCreateRouteRequestPacket
// PURPOSE  Creating a route request packet.
// Parameters:
//  node:   node which is sending query packet
//  ierpData:   pointer to ierp data.
//  packetBuffer:   packet buffer  used to build up route
//  nodeAddress:    node address to be searched.
static void IerpCreateRouteRequestPacket(Node* node,IerpData* ierpData,PacketBuffer* * packetBuffer,NodeAddress nodeAddress) {
    char* dataPtr = NULL;
    char* headerPtr = NULL;
    *packetBuffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                        IERP_INITIAL_QUERY_PACKET_SIZE,
                        IERP_HEADER_SIZE,
                        IERP_HEADER_SIZE,
                        FALSE,
                        &dataPtr,
                        &headerPtr);
    if ((headerPtr != NULL) && (dataPtr != NULL))
    {
        // Initialise all field of the header with 0
        // type      = 0;
        // length    = 0;
        // nodePtr   = 0;
        // reserved1 = 0;
        // queryId   = 0;
        // reserved2 = 0;
        memset(headerPtr, 0, IERP_HEADER_SIZE);
        IerpBuildHeader(
            ierpData,
            headerPtr,
            (IERP_INITIAL_QUERY_PACKET_SIZE + IERP_HEADER_SIZE) / 4);
        IerpBuildInitialQueryPacket(
            node,
            ierpData,
            dataPtr,
            nodeAddress);
    }
    else
    {
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Error in allocating IERP packet buffer");
        ERROR_Assert(FALSE, errStr);
        abort();
    }
}


// FUNCTION IerpSendRouteQueryPacket
// PURPOSE  Building and sending a route query packet
// Parameters:
//  node:   node which is sending query packet
//  ierpData:   pointer to ierp data.
//  nodeAddressToSearch:    node address to be searched.
//  prevHop:    Previous Hop Address
static void IerpSendRouteQueryPacket(Node* node,IerpData* ierpData,NodeAddress nodeAddressToSearch,NodeAddress prevHop) {

    PacketBuffer* packetBuffer = NULL;
    Message* msg = NULL;
    // Create route request packet
    IerpCreateRouteRequestPacket(
        node,
        ierpData,
        &packetBuffer,
        nodeAddressToSearch);
    // Encapsulate the packet in the message.
    IerpEncapsulatePacketInTheMessage(
        node,
        &msg,
        packetBuffer);
    BUFFER_FreePacketBuffer(packetBuffer);
    if (ierpData->enableBrp)
    {
         // BRP is enabled in this node so handling the packet over to BRP
        BrpSend(node,
                ierpData,
                msg,
                nodeAddressToSearch,
                BRP_NULL_CACHE_ID);
    }
    else
    {
        // BRP is not enabled in this node so start usual route discovery
        // by flooding route request.
        // sending packet to Mac layer
        NetworkIpSendRawMessageToMacLayer(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
            ANY_DEST,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_IERP,
            IPDEFTTL,
            DEFAULT_INTERFACE,
            ANY_DEST);
    }
    // Schudule another query message.
    IerpScheduleRouteQueryTimer(node, ierpData, nodeAddressToSearch);
}


// FUNCTION IerpGetDestinationQueried
// PURPOSE  Extracting the destination that is queried
// Parameters:
//  destAddressQueried: destination address that has been queried
//  queryPacketHeader:  header of the query packet
//  queryPacket:    query packet
static void IerpGetDestinationQueried(NodeAddress* destAddressQueried,IerpGeneralHeader queryPacketHeader,char* queryPacket) {
    char* packetPtr = queryPacket;
    unsigned int lengthInByte = (queryPacketHeader.length * 4);
    // length of route field is calculated by substracting the length
    // of header and length of " Query/Route Source Address" from the
    // total length field.
    unsigned int lengthOfRouteField =  lengthInByte - (IERP_HEADER_SIZE
                                        + sizeof(NodeAddress));
    // advance the pointer upto the last element of the route field
    IerpAdvancePacketPtr(&packetPtr,
        (lengthOfRouteField - sizeof(NodeAddress)));
    // Copy destination address in the formal parameter
    memcpy(destAddressQueried, packetPtr, sizeof(NodeAddress));
}


// FUNCTION IerpForwardQueryPacket
// PURPOSE  Forwarding a query packet.
// Parameters:
//  node:   node which is forwarding the query packet
//  ierpData:   Pointer to ierp data
//  ierpHeader: the ierp header
//  destAddress:    destination for which qury to be forwarded
//  queryPacket:    query packet received excuding header.
static void IerpForwardQueryPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,NodeAddress destAddress,char* queryPacket) {

    Message* msg = NULL;
    char* queryPacketPtr = queryPacket;
    char* dataPtr = NULL;
    char* headerPtr = NULL;
    // sizeof query packet excluding ierp header
    unsigned int queryRouteSize = ((ierpHeader.length - 2) * 4);
    NodeAddress destQueried = destAddress;
    NodeAddress myIP = NetworkIpGetInterfaceAddress(
                           node,
                           DEFAULT_INTERFACE);
    PacketBuffer* packetBuffer =
        BUFFER_AllocatePacketBufferWithInitialHeader(
            ((ierpHeader.length + 1) * sizeof(NodeAddress)),
            IERP_HEADER_SIZE,
            IERP_HEADER_SIZE,
            FALSE,
            &dataPtr,
            &headerPtr);
    if ((headerPtr != NULL) && (dataPtr != NULL))
    {
        // copy all the routes excluding destination address.
        memcpy(dataPtr, queryPacketPtr,
            (queryRouteSize - sizeof(NodeAddress)));
        IerpAdvancePacketPtr(&dataPtr,
            (queryRouteSize - sizeof(NodeAddress)));
        IerpAdvancePacketPtr(&queryPacketPtr,
            (queryRouteSize - sizeof(NodeAddress)));
        // copy your own IP address
        memcpy(dataPtr, &myIP, sizeof(NodeAddress));
        IerpAdvancePacketPtr(&dataPtr, sizeof(NodeAddress));
        // Copy destination address.
        memcpy(dataPtr, &destQueried, sizeof(NodeAddress));
        IerpAdvancePacketPtr(&dataPtr, sizeof(NodeAddress));
        // copy header.
        // Increment the node ptr field of the header.
        ierpHeader.nodePtr++;
        // Increment the length in header.
        ierpHeader.length++;
        memcpy(headerPtr, &ierpHeader, IERP_HEADER_SIZE);
    }
    else
    {
        ERROR_Assert(FALSE, "Buffer Allocation error");
        abort();
    }
    IerpEncapsulatePacketInTheMessage(node, &msg, packetBuffer);
    BUFFER_FreePacketBuffer(packetBuffer);
    if (!ierpData->enableBrp)
    {
        NetworkIpSendRawMessageToMacLayer(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
            ANY_DEST,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_IERP,
            IPDEFTTL,
            DEFAULT_INTERFACE,
            ANY_DEST);
    }
    else
    {
           // BRP is enabled in this node. So deliver the query to BRP for
        // bordercasting
        BrpSend(
            node,
            ierpData,
            msg,
            destAddress,
            0);
    }
}


// FUNCTION ExtractPreviousHopAddress
// PURPOSE  Extracting previous hop address from qurey
//          reply packet
// Parameters:
//  route[]:    pointer ot the "route" part of the query packet
//  nodePtr:    position of the previous hop.
static NodeAddress ExtractPreviousHopAddress(NodeAddress route[],unsigned int nodePtr) {
    return route[nodePtr];
}


// FUNCTION IerpGenerateReplyPacket
// PURPOSE  Generating Reply packet and send it back to query source
// Parameters:
//  node:   the node which is receiving the query packet
//  ierpData:   pointer to the ierp data.
//  ierpHeader: The ierp header
//  packet: pointer to data packet (after removing the header)
//  path:   path that is to be attached with the query packet
//  pathLength: length of the path (in hop count)
static void IerpGenerateReplyPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,char* packet,NodeAddress* path,int pathLength) {
    char *packetPtr = packet;
    unsigned int nodePtr = ierpHeader.nodePtr;
    PacketBuffer* packetBuffer = NULL;
    char* dataPtr = NULL;
    char* headerPtr = NULL;
    Message* msg = NULL;
    NodeAddress previousHopAddress = ExtractPreviousHopAddress(
                                          (NodeAddress*) packetPtr,
                                          nodePtr);
    if ((path != NULL) && (pathLength != 0))
    {
        // estimate the reply packet size (excluding header)
        NodeAddress myAddress = NetworkIpGetInterfaceAddress(
                                     node,
                                     DEFAULT_INTERFACE);
        unsigned int replyPacketSize =
            (ierpHeader.length - (IERP_HEADER_SIZE / 4));
        packetBuffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                           ((replyPacketSize + pathLength)
                              * sizeof(NodeAddress)),
                           IERP_HEADER_SIZE,
                           IERP_HEADER_SIZE,
                           FALSE,
                           &dataPtr,
                           &headerPtr);
        if (nodePtr != (replyPacketSize - 2))
        {
            // Calculation mismatch. possiability pf error.
            // this if() statement is mainly used for debugging.
            char errStr[MAX_STRING_LENGTH] = {0};
            sprintf(errStr, "Error: In building reply packet, packet size"
                    "calculation mismatch");
            ERROR_Assert(FALSE, errStr);
            abort();
        }
        // Copy the entire the query packet in the reply packet.
        // (Except the destination queried)
        memcpy(dataPtr, packetPtr,
            ((replyPacketSize - 1) * sizeof(NodeAddress)));
        // skip the source node field in the query packet
        IerpAdvancePacketPtr(&packetPtr,
            ((replyPacketSize - 1) * sizeof(NodeAddress)));
        IerpAdvancePacketPtr(&dataPtr,
            ((replyPacketSize - 1) * sizeof(NodeAddress)));
        memcpy(dataPtr, &myAddress, sizeof(NodeAddress));
        IerpAdvancePacketPtr(&dataPtr, sizeof(NodeAddress));
        // Add the path in the packet (data ptr).
        memcpy(dataPtr, path, pathLength * sizeof(NodeAddress));
        // Copy the header
        // Following steps does that.
        // Change type ofthe packet
        ierpHeader.type = IERP_REPLY_PACKET;
        // Change the header length field
        ierpHeader.length = (unsigned char)((replyPacketSize + pathLength)
            + IERP_HEADER_SIZE / 4);
        // Do not change the nodPtr field
        // ierpHeader.nodePtr = ierpHeader.nodePtr
        ierpHeader.nodePtr--;
        // Do not chance the query Id
        // ierpHeader.queryId = ierpHeader.queryId
        // Copy the header in the reply packet
        memcpy(headerPtr, &ierpHeader, IERP_HEADER_SIZE);
    }
    else
    {
        unsigned int replyPacketSize = ierpHeader.length;
        packetBuffer = BUFFER_AllocatePacketBufferWithInitialHeader(
                           (replyPacketSize * sizeof(NodeAddress)),
                           IERP_HEADER_SIZE,
                           IERP_HEADER_SIZE,
                           FALSE,
                           &dataPtr,
                           &headerPtr);
        // Copy the entire route request packet in the reply packet.
        memcpy(dataPtr, packetPtr,
            (replyPacketSize * sizeof(NodeAddress)));
        IerpAdvancePacketPtr(&packetPtr,
            (replyPacketSize * sizeof(NodeAddress)));
        // Copy the header
        // Following steps does that.
        // Change type ofthe packet
        ierpHeader.type = IERP_REPLY_PACKET;
        // Do not change the header length field
        // ierpHeader.length = ierpHeader.length;
        // change the nodPtr field
        ierpHeader.nodePtr--;
        // Do not chance the query Id
        // ierpHeader.queryId = ierpHeader.queryId
        memcpy(headerPtr, &ierpHeader, IERP_HEADER_SIZE);
    }
    // Encapsulate the packet in the message
    IerpEncapsulatePacketInTheMessage(
        node,
        &msg,
        packetBuffer);
    // clear packet buffer
    BUFFER_FreePacketBuffer(packetBuffer);
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
        previousHopAddress,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_IERP,
        IPDEFTTL,
        DEFAULT_INTERFACE,
        previousHopAddress);
    if (IERP_DEBUG_REPLY)
    {
        FILE* fd = NULL;
        fd = fopen("ierpDebug.out","a");
        fprintf(fd, "Node %d initiating Reply\n", node->nodeId);
        fclose(fd);
        // If debug query is defined then print query message.
    }
}


// FUNCTION IerpForwardReplyPacket
// PURPOSE  Forwarding the reply packet
// Parameters:
//  node:   node node which is forwording the query packet
//  ierpData:   pointer to ierp data.
//  ierpHeader: ierp header
//  packet: query packet (excluding header)
//  previousHop:    previous where reply is to be forworded.
static void IerpForwardReplyPacket(Node* node,IerpData* ierpData,IerpGeneralHeader ierpHeader,char* packet,NodeAddress previousHop) {
    unsigned char* dataPtr = NULL;
    unsigned char* headerPtr = NULL;
    Message* msg = NULL;
    unsigned int replyPacketSize =
       (ierpHeader.length - (IERP_HEADER_SIZE / 4));
    PacketBuffer* packetBuffer =
        BUFFER_AllocatePacketBufferWithInitialHeader(
            (replyPacketSize * sizeof(NodeAddress)),
            IERP_HEADER_SIZE,
            IERP_HEADER_SIZE,
            FALSE,
            (char**) &dataPtr,
            (char**) &headerPtr);
    if ((dataPtr != NULL) && (headerPtr != NULL))
    {
        // copy entire reply packet in the data part.
        memcpy(dataPtr, packet, (replyPacketSize * sizeof(NodeAddress)));
        // Copy the header
        // Following steps does that.
        // Do not hange type ofthe packet
        // ierpHeader.type = IERP_REPLY_PACKET;
        // Do not change the header length field
        // ierpHeader.length = ierpHeader.length;
        // change the nodPtr field
        ierpHeader.nodePtr--;
        // Do not chance the query Id
        // ierpHeader.queryId = ierpHeader.queryId
        memcpy(headerPtr, &ierpHeader, IERP_HEADER_SIZE);
    }
    // Encapsulate the packet in the message
    IerpEncapsulatePacketInTheMessage(
        node,
        &msg,
        packetBuffer);
    // clear packet buffer
    BUFFER_FreePacketBuffer(packetBuffer);
    NetworkIpSendRawMessageToMacLayer(
        node,
        msg,
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
        previousHop,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_IERP,
        IPDEFTTL,
        DEFAULT_INTERFACE,
        previousHop);
}


// FUNCTION IerpReturnPathAndPathLengthAndPreviousHop
// PURPOSE  Returning Path and path length
// Parameters:
//  ierpHeader: query packet header
//  queryPacket:    query Packet received (excluding header)
//  route:  Pointer to route pointer
//  prevHopAddress: previous hop address to be returned
//  length: path length to pe returned
static void IerpReturnPathAndPathLengthAndPreviousHop(IerpGeneralHeader ierpHeader,char* queryPacket,NodeAddress* * route,NodeAddress* prevHopAddress,unsigned int* length) {
    unsigned int totalPathLength =
       ((ierpHeader.length - IERP_HEADER_SIZE / 4) - 1);
    NodeAddress* routeArray = (NodeAddress*) queryPacket;
    if ((signed char) ierpHeader.nodePtr < 0)
    {
        ierpHeader.nodePtr = 0;
        *route = &(routeArray[ierpHeader.nodePtr + 1]);
        *prevHopAddress = (NodeAddress) NETWORK_UNREACHABLE;
        *length = (ierpHeader.length - 3);
    }
    else
    {
        *route = &(routeArray[ierpHeader.nodePtr + 2]);
        *prevHopAddress = routeArray[ierpHeader.nodePtr];
        *length = (totalPathLength - (ierpHeader.nodePtr + 1));
    }
}


// FUNCTION IerpReadHeaderInformation
// PURPOSE  Reading ierp general message header
// Parameters:
//  iarpHeader: header that has been read from the message
//  msg:    the messgae that has been received.
static void IerpReadHeaderInformation(IerpGeneralHeader* iarpHeader,Message* msg) {
    memcpy(iarpHeader, MESSAGE_ReturnPacket(msg), IERP_HEADER_SIZE);
}


// FUNCTION IerpHandlePacketDrop
// PURPOSE  Handling packet drop due to non avilability of next hop
// Parameters:
//  node:   node which is handling packet drop
//  msg:    the lost message
static void IerpHandlePacketDrop(Node* node,const Message* msg) {
    IerpData* ierpData = (IerpData*) NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_IERP);
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    NodeAddress destAddr = ipHeader->ip_dst;
    IerpRoutingTableEntry* ierpRoutingTableEntry = NULL;
    if (ierpData == NULL)
    {
        //
        //  UNUSUAL SITUATION: IERP data pointer should not be NULL
        //
        ERROR_Assert(ierpData, "IERP dataPtr should not be NULL !!!");
        return;
    }
    ierpRoutingTableEntry = IerpSearchRouteInRoutingTable(
                                &(ierpData->ierpRoutingTable),
                                destAddr);
    if (ierpRoutingTableEntry != NULL)
    {
        IerpDeleteRouteFromRoutingTable(
            &(ierpData->ierpRoutingTable),
            destAddr);
    }
}


// FUNCTION IerpReadZoneRadiusFromConfigurationFile
// PURPOSE  Reading the value of zone radius from the configuration
//          file. If no zone radius is given a default zone radius
//          value is assumed
// Parameters:
//  node:   node which is reading the zone radius
//  nodeInput:  input configuration to the node
//  zoneRadiusRead: value of the zone radius read from
static void IerpReadZoneRadiusFromConfigurationFile(Node* node,const NodeInput* nodeInput,unsigned int* zoneRadiusRead) {
    *zoneRadiusRead = 0;
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


// FUNCTION IerpRegisterZrp
// PURPOSE  Build a co-ordination system with zrp.
// Parameters:
//  node:   node which is initializing.
//  dataPtr:    pointer to the ierp data structure.
//  nodeInput:  pointer to node input
//  interfaceIndex: interface index which is initializing
//  ierpRouterFunc: pointer to ierp router function
//  macStatusHandlar:   pointer to mac layer function handler
void IerpRegisterZrp(Node* node,IerpData* * dataPtr,const NodeInput* nodeInput,int interfaceIndex,RouterFunctionType* ierpRouterFunc,MacLayerStatusEventHandlerFunctionType* macStatusHandlar) {
    IerpInit(node, dataPtr, nodeInput, interfaceIndex);
    *ierpRouterFunc = IerpRouterFunction;
    *macStatusHandlar = IerpMacLayerStatusHandler;
}


// FUNCTION IerpGetRoutingProtocol
// PURPOSE  Get routing protocol structure associated with routing protocol
// Parameters:
//  node:   Pointer to node
//  routingProtocolType:    Routing protocol to retrieve
void* IerpGetRoutingProtocol(Node* node,NetworkRoutingProtocolType routingProtocolType) {
    void* protocolData = NULL;
    int i = 0;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    // Protocol Data must be IERP
    ERROR_Assert(routingProtocolType == ROUTING_PROTOCOL_IERP,
                 "Protocol Must be IERP");
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
        protocolData = (IerpData*) ZrpReturnProtocolData(
                                      node,
                                      ROUTING_PROTOCOL_IERP);
    }
    ERROR_Assert(protocolData, "Protocol Data Must not be NULL");
    return protocolData;
}


// FUNCTION IerpInsertBuffer
// PURPOSE  Insert a packet into the buffer if no route is available
// Parameters:
//  node:   node pointer
//  msg:    The message waiting for a route to destination
//  destAddr:   The destination of the packet
//  buffer: The buffer to store the message
//  maxBuffSize:    Max Buffer Size
static void IerpInsertBuffer(Node* node,Message* msg,NodeAddress destAddr,IerpMessageBuffer* buffer,int maxBuffSize) {

//warning: Don't remove the opening and the closing brace
//add your code here:
    IerpBufferNode* current = NULL;
    IerpBufferNode* previous = NULL;
    IerpBufferNode* newNode = NULL;
    IerpData* dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(
                                     node,
                                     ROUTING_PROTOCOL_IERP);
    // if the buffer exceeds silently drop the packet
    // if no buffer size is specified in bytes it will only check for
    // number of packet.
    if (buffer->size == maxBuffSize)
    {
        MESSAGE_Free(node, msg);
        dataPtr->stats.numDataDroppedForBufferOverFlow++;
        return;
    }
    // Find Insertion point.  Insert after all address matches.
    // This is to maintain a sorted list in ascending order of the
    // destination address
    previous = NULL;
    current = buffer->head;
    while (current && current->destAddr <= destAddr)
    {
        previous = current;
        current = current->next;
    }
    // Allocate space for the new message
    newNode = (IerpBufferNode*) MEM_malloc(sizeof(IerpBufferNode));
    // Store the allocate message along with the destination number and
    // the time at which the packet has been inserted
    newNode->destAddr = destAddr;
    newNode->msg = msg;
    newNode->next = current;
    // Increase the size of the buffer
    ++(buffer->size);
    // Got the insertion point
    if (previous == NULL)
    {
        // The is the first message in the buffer or to be
        // inserted in the first
        buffer->head = newNode;
    }
    else
    {
        // This is an intermediate node in the list
        previous->next = newNode;
    }
}


// FUNCTION IerpGetBufferedPacket
// PURPOSE  Extract the packet that was buffered
// Parameters:
//  destAddr:   he destination address of the packet to be retrieved
//  buffer: the message buffer
static Message* IerpGetBufferedPacket(NodeAddress destAddr,IerpMessageBuffer* buffer) {
//warning: Don't remove the opening and the closing brace
//add your code here:
IerpBufferNode* current = buffer->head;
    Message* pktToDest = NULL;
    IerpBufferNode* toFree = NULL;
    if (!current)
    {
        // No packet in the buffer so nothing to do
    }
    else if (current->destAddr == destAddr)
    {
        // The first packet is the desired packet
        toFree = current;
        buffer->head = toFree->next;
        pktToDest = toFree->msg;
        MEM_free(toFree);
        --(buffer->size);
    }
    else
    {
        while (current->next && current->next->destAddr < destAddr)
        {
            current = current->next;
        }
        if (current->next && current->next->destAddr == destAddr)
        {
            // Got the matched destination so return the packet
            toFree = current->next;
            pktToDest = toFree->msg;
            current->next = toFree->next;
            MEM_free(toFree);
            --(buffer->size);
        }
    }
    return pktToDest;
}



//UserCodeEnd

//State Function Definitions
void IerpMacLayerStatusHandler(Node* node, const Message* msg,
                               const NodeAddress nextHopAddress, const int interfaceIndex) {
        //UserCodeStartMacEvent
    IerpData* dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_IERP);
    Message *nullMsg;

    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IERP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IERP dataPtr should not be NULL !!!");
        return;
    }

    switch(msg->eventType)
    {
        case MSG_NETWORK_PacketDropped:
        {
            IerpHandlePacketDrop(node, msg);
            break;
        }

        default:
        {
            printf("#%d: Cannot handle message type #%d\n",
               node->nodeId, msg->eventType);
            assert(FALSE);
            break;
        }
    }

    // Set the message pointer to NULL. This will not effect
    // the original pointer passed to this function, but it will prevent
    // the  message from being freed at the idle state.
    nullMsg = NULL;

    //UserCodeEndMacEvent

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, NULL);
}


void IerpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                              NodeAddress destAddr, int interfaceIndex, unsigned ttl) {
        //UserCodeStartReceivePacketFromNetworkLayer
    IerpData* dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_IERP);
    IerpGeneralHeader ierpHeader = {0};
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    IerpPacketType packetType;

    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IERP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IERP dataPtr should not be NULL !!!");
        return;
    }

    IerpReadHeaderInformation(&ierpHeader, msg);
    IerpAdvancePacketPtr(&packetPtr, IERP_HEADER_SIZE);
    packetType = (IerpPacketType) ierpHeader.type;


    //UserCodeEndReceivePacketFromNetworkLayer

    if (packetType == IERP_REPLY_PACKET) {
        if (packetType == IERP_QUERY_PACKET ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = IERP_PROCESS_REPLY_PACKET;
        enterIerpProcessReplyPacket(node, dataPtr, msg);
        return;
    } else if (packetType == IERP_QUERY_PACKET) {
        if (packetType == IERP_REPLY_PACKET ) {
            ERROR_ReportError("Multiple true guards found in transitions from same state.");
        }
        dataPtr->state = IERP_PROCESS_QUERY_PACKET;
        enterIerpProcessQueryPacket(node, dataPtr, msg);
        return;
    }
}


void IerpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                        NodeAddress previousHopAddr, BOOL* packetWasRouted) {
        //UserCodeStartIerpRouterFunction
    IerpRoutingTableEntry* ierpRoutingTableEntry = NULL;
    IerpData* dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_IERP);

    if (dataPtr == NULL)
    {
        //
        //  UNUSUAL SITUATION: IERP data pointer should not be NULL
        //
        ERROR_Assert(dataPtr, "IERP dataPtr should not be NULL !!!");
        return;
    }

    if ((destAddr == ANY_DEST) ||
        (NetworkIpIsMyIP(node, destAddr) == TRUE))
    {
        *packetWasRouted = FALSE;
        return;
    }

    ierpRoutingTableEntry = IerpSearchRouteInRoutingTable(
                                &(dataPtr->ierpRoutingTable),
                                destAddr);

    if (ierpRoutingTableEntry != NULL)
    {
        if ((ierpRoutingTableEntry->pathList == NULL) &&
            (ierpRoutingTableEntry->hopCount == 0))
        {
            // This path has been queried earlier
            // but not established yet
            // keep the packet into the buffer
            IerpInsertBuffer(
                node,
                msg,
                destAddr,
                &dataPtr->msgBuffer,
                dataPtr->maxBufferSize);
            *packetWasRouted = true;
            return;
        }
        else
        {
            // Ierp route the packet
            NodeAddress nextHop = ierpRoutingTableEntry->pathList[0];

            if ((nextHop == (NodeAddress) NETWORK_UNREACHABLE)/* ||
                (nextHop == previousHopAddr)*/)
        /*Commented During ARP task, to remove previous hop dependency*/
            {
                 *packetWasRouted = FALSE;
                 return;
            }


            NetworkIpSendPacketToMacLayer(
                node,
                msg,
                DEFAULT_INTERFACE,
                nextHop);

            ierpRoutingTableEntry->lastUsed = node->getNodeTime();
            *packetWasRouted = TRUE;

            // route the packet and update stat
            dataPtr->stats.numPacketRouted++;
        }
    }
    else
    {
        *packetWasRouted = true;

        IerpAddRouteToRoutingTable(
            node,
            &(dataPtr->ierpRoutingTable),
            destAddr,
            0xFFFFFFFF,
            NULL,
            0);

        IerpInsertBuffer(
            node,
            msg,
            destAddr,
            &dataPtr->msgBuffer,
            dataPtr->maxBufferSize);

        // Ierp bordercast route query TBD (this is broadcast).
        if (IERP_DEBUG_QUERY)
        {
            static int i = 0;
            FILE* fd = NULL;

            if (i == 0)
            {
                i++;
                fd = fopen("ierpDebug.out","w");
            }
            else
            {
                fd = fopen("ierpDebug.out","a");
            }

            fprintf(fd, "Node %d initiating Query\n", node->nodeId);
            fclose(fd);
            // If debug query is defined then print query message.
        }

        IerpSendRouteQueryPacket(
            node,
            dataPtr,
            destAddr,
            previousHopAddr);

        // sending query packet and update stat.
        dataPtr->stats.numQueryPacketSend++;
    }

    //UserCodeEndIerpRouterFunction

    if (ierpRoutingTableEntry != NULL) {
        dataPtr->state = IERP_ROUTE_PACKET;
        enterIerpRoutePacket(node, dataPtr, msg);
        return;
    } else {
        dataPtr->state = IERP_SEND_QUERY_PACKET;
        enterIerpSendQueryPacket(node, dataPtr, msg);
        return;
    }
}


static void enterIerpProcessIerpQueryTimerExpired(Node* node,
                                                  IerpData* dataPtr,
                                                  Message *msg) {

    //UserCodeStartProcessIerpQueryTimerExpired
    IerpData* ierpData = dataPtr;
    NodeAddress destAddress = ANY_DEST;
    IerpRoutingTableEntry* ierpRoutingTableEntry = NULL;

    // Get dest address from msg->info field.
    memcpy(&destAddress, MESSAGE_ReturnInfo(msg), sizeof(NodeAddress));

    ierpRoutingTableEntry = IerpSearchRouteInRoutingTable(
                                &(ierpData->ierpRoutingTable),
                                destAddress);

    if ((ierpRoutingTableEntry) &&
        (ierpRoutingTableEntry->pathList == NULL))
    {
        // Ierp bordercast route query.
        // TBD (this is broadcast)
        IerpSendRouteQueryPacket(node, ierpData, destAddress, ANY_IP);

        // sending query packet and update stat.
        ierpData->stats.numQueryPacketSend++;
    }
    else
    {
        // printf("Not doing anything\n");
        // no operation
    }

    //UserCodeEndProcessIerpQueryTimerExpired

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}

static void enterIerpSendQueryPacket(Node* node,
                                     IerpData* dataPtr,
                                     Message *msg) {

    //UserCodeStartSendQueryPacket
    // 1) Query message has been sent in the previous state.
    //    In this state only freeing of message has been prevented.

    // 2) Set the message pointer to NULL. This will not effect
    //    the original pointer passed to this function, but it will
    //    prevent the  message from being freed at the idle state.
    msg = NULL;

    //UserCodeEndSendQueryPacket

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}

static void enterIerpRoutePacket(Node* node,
                                 IerpData* dataPtr,
                                 Message *msg) {

    //UserCodeStartRoutePacket
    // 1) Packet has been routed in the previous state.
    //    In this state only freeing of message has been prevented.

    // 2) Set the message pointer to NULL. This will not effect
    //    the original pointer passed to this function, but it will
    //    prevent the  message from being freed at the idle state.
    msg = NULL;

    //UserCodeEndRoutePacket

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}

static void enterIerpIdleState(Node* node,
                               IerpData* dataPtr,
                               Message *msg) {

    //UserCodeStartIdleState
    if (msg != NULL)
    {
        MESSAGE_Free(node, msg);
    }

    //UserCodeEndIdleState


}

static void enterIerpProcessReplyPacket(Node* node,
                                        IerpData* dataPtr,
                                        Message *msg) {

    //UserCodeStartProcessReplyPacket
    IerpData *ierpData = dataPtr;
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    char* packet = NULL;
    IerpGeneralHeader ierpHeader = {0};
    Message* newMsg = NULL;
    NodeAddress sourceAddress = ANY_DEST;
    NodeAddress destInSearch = ANY_DEST;
    NodeAddress prevHop = ANY_DEST;
    NodeAddress* pathList = NULL;
    unsigned int length = 0;
    int i =0;
    IerpRoutingTableEntry* ierpRoutingTableEntry = NULL;

    //  read and throw out the header.
    IerpReadHeaderInformation(&ierpHeader, msg);
    IerpAdvancePacketPtr(&packetPtr, IERP_HEADER_SIZE);
    packet = packetPtr;

    // Extract the query source Address
    memcpy(&sourceAddress, packetPtr, sizeof(NodeAddress));

    // Get the path from curret from nodePtr position to destination.
    IerpReturnPathAndPathLengthAndPreviousHop(
        ierpHeader,
        packet,
        &pathList,
        &prevHop,
        &length);

    destInSearch = pathList[length - 1];

    // Detect loop - Your own IP should not be in Path list
    // If loop detected discard processing.
    for (i = 0; i < (signed)length; i++)
    {
        if (NetworkIpIsMyIP(node, pathList[i]))
        {
            // GOTO Idle state and Free the packet
            dataPtr->state = IERP_IDLE_STATE;
            enterIerpIdleState(node, dataPtr, msg);
            return;
        }
    }

    if (IERP_DEBUG_REPLY)
    {
        int i = 0;
        char dbgStr[MAX_STRING_LENGTH] = {0};
        FILE* fd = NULL;

        fd = fopen("ierpDebug.out","a");

        IO_ConvertIpAddressToString(destInSearch, dbgStr);

        fprintf(fd, "-------------------------------------------\n"
               "#node %d: has got following paths:\n"
               "-------------------------------------------\n"
               "dest address in search = %16s\n"
               "-------------------------------------------\n",
               node->nodeId,
               dbgStr);

        for (i = 0; i < (signed)length; i++)
        {
            IO_ConvertIpAddressToString(pathList[i], dbgStr);
            fprintf(fd, "%20s\n", dbgStr);
        }

        fprintf(fd, "------------------------------------------\n");
        fclose(fd);

        IerpPrintQueryPacket(node, ierpHeader, packet);
    }

    ierpRoutingTableEntry = IerpSearchRouteInRoutingTable(
                                &(ierpData->ierpRoutingTable),
                                destInSearch);

    // assert(ierpRoutingTableEntry && pathList && length);
    if (ierpRoutingTableEntry != NULL)
    {
        NodeAddress* path = (NodeAddress*) MEM_malloc(
            length * sizeof(NodeAddress));
        memcpy(path, pathList, (length * sizeof(NodeAddress)));

        ierpRoutingTableEntry->pathList = path;
        ierpRoutingTableEntry->hopCount = length;
        newMsg = IerpGetBufferedPacket(
                     destInSearch,
                     &dataPtr->msgBuffer);

        while (newMsg)
        {
            // route the packet and update stat
            dataPtr->stats.numPacketRouted++;

            NetworkIpSendPacketToMacLayer(
                node,
                newMsg,
                DEFAULT_INTERFACE,
                path[0]);

            newMsg = IerpGetBufferedPacket(
                         destInSearch,
                         &dataPtr->msgBuffer);
        }
    }
    else
    {
        //IerpRoutingTableEntry* pathAdded
        IerpAddRouteToRoutingTable(
            node,
            &(ierpData->ierpRoutingTable),
            destInSearch,
            0xFFFFFFFF,
            pathList,
            length);
    }

    if (NetworkIpIsMyIP(node, sourceAddress) == TRUE)
    {
        ierpData->stats.numReplyPacketRecv++;

        // GOTO Idle state and Free the packet
        dataPtr->state = IERP_IDLE_STATE;
        enterIerpIdleState(node, dataPtr, msg);
        return;
    }

    // If reply-processor node is not query source then
    // forword the reply packet to previous hop
    IerpForwardReplyPacket(node, ierpData, ierpHeader, packet, prevHop);

    // update reply packet forwarded statistics.
    ierpData->stats.numReplyPacketRelayed++;

    // update reply packet received statistics
    ierpData->stats.numReplyPacketRecv++;

    //UserCodeEndProcessReplyPacket

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}

static void enterIerpProcessQueryPacket(Node* node,
                                        IerpData* dataPtr,
                                        Message *msg) {

    //UserCodeStartProcessQueryPacket
    IerpData* ierpData = dataPtr;
    char* packetPtr = MESSAGE_ReturnPacket(msg);
    IerpGeneralHeader ierpHeader = {0};
    char* packet = NULL;
    IerpSeqTable* seqTable = NULL;
    //unsigned short queryId = ierpHeader.queryId;
    NodeAddress sourceAddress = ANY_DEST;
    NodeAddress destAddressQueried = ANY_DEST;
    IerpRoutingTableEntry* ierpRoutingTableEntry = NULL;

    //  read and throw out the header.
    IerpReadHeaderInformation(&ierpHeader, msg);
    IerpAdvancePacketPtr(&packetPtr, IERP_HEADER_SIZE);
    packet = packetPtr;

    // Read the query source from link state table
    memcpy(&sourceAddress, packet, sizeof(NodeAddress));
    IerpAdvancePacketPtr(&packetPtr, sizeof(NodeAddress));

    IerpGetDestinationQueried(&destAddressQueried, ierpHeader, packetPtr);

    if (NetworkIpIsMyIP(node, sourceAddress) == TRUE)
    {
        // GOTO Idle state and Free the packet
        dataPtr->state = IERP_IDLE_STATE;
        enterIerpIdleState(node, dataPtr, msg);
        return;
    }
    if (NetworkIpIsMyIP(node, destAddressQueried) == TRUE)
    {
        IerpGenerateReplyPacket(
            node,
            ierpData,
            ierpHeader,
            packet,
            NULL,
            0);

        ierpData->stats.numQueryPacketRecv++;
        ierpData->stats.numReplyPacketSend++;

        // GOTO Idle state and Free the packet
        dataPtr->state = IERP_IDLE_STATE;
        enterIerpIdleState(node, dataPtr, msg);
        return;
    }

    seqTable = IerpSearchLinkSourceInSeqTable(
                   ierpData->seqTable,
                   sourceAddress);

    if ((seqTable == NULL) ||
        (seqTable->seqNum <  ierpHeader.queryId))
    {
        ierpData->seqTable = IerpInsertIntoSeqTable(
                                 ierpData->seqTable,
                                 sourceAddress,
                                 ierpHeader.queryId);
    }
    else
    {
        // GOTO Idle state and Free the packet
        dataPtr->state = IERP_IDLE_STATE;
        enterIerpIdleState(node, dataPtr, msg);
        return;
    }

    ierpRoutingTableEntry = IerpSearchRouteInRoutingTable(
                                &(ierpData->ierpRoutingTable),
                                destAddressQueried);

    if (IERP_DEBUG_QUERY)
    {
        // If debug query is defined then print query message.
        IerpPrintQueryPacket(node, ierpHeader, packet);
    }

    if (ierpRoutingTableEntry == NULL)
    {
        IerpForwardQueryPacket(
            node,
            ierpData,
            ierpHeader,
            destAddressQueried,
            packet);
        // update query packet forwarded statistics
        ierpData->stats.numQueryPacketRelayed++;
    }
    else
    {
        if ((ierpRoutingTableEntry->pathList == NULL) &&
            (ierpRoutingTableEntry->hopCount == 0))
        {
            IerpForwardQueryPacket(
                node,
                ierpData,
                ierpHeader,
                destAddressQueried,
                packet);
            // update query packet forwarded statistics
            ierpData->stats.numQueryPacketRelayed++;
        }
        else
        {
            IerpGenerateReplyPacket(
                node,
                ierpData,
                ierpHeader,
                packet,
                ierpRoutingTableEntry->pathList,
                ierpRoutingTableEntry->hopCount);

            ierpData->stats.numReplyPacketSend++;
        }
    }

    // update the statistics query packet received
    ierpData->stats.numQueryPacketRecv++;

    //UserCodeEndProcessQueryPacket

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}

static void enterIerpProcessFlushTimerExpired(Node* node,
                                              IerpData* dataPtr,
                                              Message *msg) {

    //UserCodeStartProcessFlushTimerExpired
    int i = 0;
    clocktype currentTime = node->getNodeTime();
    IerpRoutingTable* ierpRoutingTable = &(dataPtr->ierpRoutingTable);

    while (i < (signed)ierpRoutingTable->numEntries)
    {
        if ((currentTime - IERP_DEFAULT_ROUTE_AGE_OUT_TIME) >
            ierpRoutingTable->ierpRoutingTableEntry[i].lastUsed)
        {
            MEM_free(ierpRoutingTable->ierpRoutingTableEntry[i].pathList);

            memcpy(&ierpRoutingTable->ierpRoutingTableEntry[i],
                   &ierpRoutingTable->ierpRoutingTableEntry
                       [ierpRoutingTable->numEntries - 1],
                   sizeof(IerpRoutingTableEntry));

            memset(&ierpRoutingTable->ierpRoutingTableEntry
                       [ierpRoutingTable->numEntries - 1], 0,
                   sizeof(IerpRoutingTableEntry));

            ierpRoutingTable->numEntries--;
            continue;
        }
        i++;
    }
    IerpScheduleRouteFlushOutTimer(node, dataPtr);

    //UserCodeEndProcessFlushTimerExpired

    dataPtr->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, dataPtr, msg);
}


//Statistic Function Definitions
static void IerpInitStats(Node* node, IerpStatsType *stats) {
    if (node->guiOption) {
        stats->numQueryPacketSendId = GUI_DefineMetric("IARP: Number of Query Packets Sent", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numQueryPacketRecvId = GUI_DefineMetric("IARP: Number of Query Packets Received", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numQueryPacketRelayedId = GUI_DefineMetric("IARP: Number of Query Packets Relayed", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numReplyPacketSendId = GUI_DefineMetric("IARP: Number of Reply Packets Sent", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numReplyPacketRecvId = GUI_DefineMetric("IARP: Number of Reply Packets Received", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numReplyPacketRelayedId = GUI_DefineMetric("IARP: Number of Reply Packets Relayed", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numPacketRoutedId = GUI_DefineMetric("IARP: Number of Packets Routed via IERP", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
        stats->numDataDroppedForBufferOverFlowId = GUI_DefineMetric("IARP: Number of Data Packets Dropped due to Buffer Overflow", node->nodeId, GUI_NETWORK_LAYER, 0, GUI_UNSIGNED_TYPE,GUI_CUMULATIVE_METRIC);
    }

    stats->numQueryPacketSend = 0;
    stats->numQueryPacketRecv = 0;
    stats->numQueryPacketRelayed = 0;
    stats->numReplyPacketSend = 0;
    stats->numReplyPacketRecv = 0;
    stats->numReplyPacketRelayed = 0;
    stats->numPacketRouted = 0;
    stats->numDataDroppedForBufferOverFlow = 0;
}

void IerpPrintStats(Node* node) {
    IerpStatsType *stats = &((IerpData *) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IERP))->stats;
    char buf[MAX_STRING_LENGTH];

    char statProtocolName[MAX_STRING_LENGTH];
    strcpy(statProtocolName,"IERP");
    sprintf(buf, "Number of query packets sent = %u", stats->numQueryPacketSend);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of query packets received = %u", stats->numQueryPacketRecv);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of query packets relayed = %u", stats->numQueryPacketRelayed);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of reply packets sent = %u", stats->numReplyPacketSend);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of reply packets received  = %u", stats->numReplyPacketRecv);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of reply packets relayed = %u", stats->numReplyPacketRelayed);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of packets routed via IERP = %u", stats->numPacketRouted);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
    sprintf(buf, "Number of data packets dropped because of buffer overflow = %u", stats->numDataDroppedForBufferOverFlow);
    IO_PrintStat(node, "Network", statProtocolName, ANY_DEST, -1, buf);
}

void IerpRunTimeStat(Node* node, IerpData* dataPtr) {
    clocktype now = node->getNodeTime();

    if (node->guiOption) {
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numQueryPacketSendId, dataPtr->stats.numQueryPacketSend, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numQueryPacketRecvId, dataPtr->stats.numQueryPacketRecv, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numQueryPacketRelayedId, dataPtr->stats.numQueryPacketRelayed, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numReplyPacketSendId, dataPtr->stats.numReplyPacketSend, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numReplyPacketRecvId, dataPtr->stats.numReplyPacketRecv, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numReplyPacketRelayedId, dataPtr->stats.numReplyPacketRelayed, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numPacketRoutedId, dataPtr->stats.numPacketRouted, now);
        GUI_SendUnsignedData(node->nodeId, dataPtr->stats.numDataDroppedForBufferOverFlowId, dataPtr->stats.numDataDroppedForBufferOverFlow, now);
    }
}

void
IerpInit(Node* node, IerpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex) {

    Message *msg = NULL;

    //UserCodeStartInit
    // The following line of code should be executed only
    // for the *first* initialization of an interface that
    // shares this data structure.
    int parameterValue;
    BOOL wasFound = FALSE;
    char valueRead[MAX_STRING_LENGTH];
    (*dataPtr) = (IerpData*) MEM_malloc(sizeof(IerpData));
    memset((*dataPtr), 0, sizeof(IerpData));
    (*dataPtr)->printStats =  FALSE;
    (*dataPtr)->statsPrinted = FALSE;

    (*dataPtr)->seqTable = NULL;
    (*dataPtr)->ierpRouteRequestTimer = IERP_DEFAULT_ROUTE_REQ_TIMER;

    IerpReadZoneRadiusFromConfigurationFile(
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
        (*dataPtr)->enableBrp = TRUE;
        (*dataPtr)->brpData = MEM_malloc(sizeof(BrpData));
        BrpInit(node, (BrpData*) ((*dataPtr)->brpData));
    }
    else
    {
        (*dataPtr)->enableBrp = FALSE;
        (*dataPtr)->brpData = NULL;
    }

    // Initialize Ierp statistics structure.
    IerpInitStats(node, &((*dataPtr)->stats));

    IerpInitializeRoutingTable(&((*dataPtr)->ierpRoutingTable));

    IO_ReadInt(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "IERP-MAX-MESSAGE-BUFFER-SIZE",
        &wasFound,
        &parameterValue);

    if((wasFound == TRUE) &&  (parameterValue >= 0))
    {
        (*dataPtr)->maxBufferSize = parameterValue;
    }
    else
    {
        (*dataPtr)->maxBufferSize = IERP_DEFAULT_MAX_MESSAGE_BUFFER_SIZE;
    }

    // Read if statistics is need to be printed
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &wasFound,
        valueRead);

    if ((wasFound == TRUE) && (strcmp(valueRead, "YES") == 0))
    {
        (*dataPtr)->printStats = TRUE;
    }

    // Set Ierp router function.
    if (!ZrpIsEnabled(node))
    {
        NetworkIpSetRouterFunction(
            node,
            IerpRouterFunction,
            interfaceIndex);

        NetworkIpSetMacLayerStatusEventHandlerFunction(
            node,
            &IerpMacLayerStatusHandler,
            interfaceIndex);
    }
    IerpScheduleRouteFlushOutTimer(node, (*dataPtr));

    //UserCodeEndInit
    if ((*dataPtr)->initStats) {
        IerpInitStats(node, &((*dataPtr)->stats));
    }
    (*dataPtr)->state = IERP_IDLE_STATE;
    enterIerpIdleState(node, *dataPtr, msg);
}


void
IerpFinalize(Node* node) {

    IerpData *dataPtr = (IerpData*) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IERP);

    //UserCodeStartFinalize

    // If protocol data pointer is NULL give error Message.
    ERROR_Assert(dataPtr != NULL,
       "Pointer to the protocol Data Cannot be NULL");
    if (dataPtr->enableBrp)
    {
        BrpPrintStats(node);
    }


    //UserCodeEndFinalize

    if (dataPtr->statsPrinted) {
        return;
    }

    if (dataPtr->printStats) {
        dataPtr->statsPrinted = TRUE;
        IerpPrintStats(node);
    }
}


void
IerpProcessEvent(Node* node,   Message* msg) {

    IerpData* dataPtr = (IerpData*)  NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_IERP);

    int event = msg->eventType;
    switch (dataPtr->state) {
        case IERP_PROCESS_IERP_QUERY_TIMER_EXPIRED:
            assert(FALSE);
            break;
        case IERP_SEND_QUERY_PACKET:
            assert(FALSE);
            break;
        case IERP_ROUTE_PACKET:
            assert(FALSE);
            break;
        case IERP_IDLE_STATE:
            switch (event) {
                case ROUTING_IerpRouteRequestTimeExpired:
                    dataPtr->state = IERP_PROCESS_IERP_QUERY_TIMER_EXPIRED;
                    enterIerpProcessIerpQueryTimerExpired(node, dataPtr, msg);
                    break;
                case ROUTING_IerpFlushTimeOutRoutes:
                    dataPtr->state = IERP_PROCESS_FLUSH_TIMER_EXPIRED;
                    enterIerpProcessFlushTimerExpired(node, dataPtr, msg);
                    break;
                default:
                    assert(FALSE);
            }
            break;
        case IERP_PROCESS_REPLY_PACKET:
            assert(FALSE);
            break;
        case IERP_PROCESS_QUERY_PACKET:
            assert(FALSE);
            break;
        case IERP_PROCESS_FLUSH_TIMER_EXPIRED:
            assert(FALSE);
            break;
        default:
            ERROR_ReportError("Illegal transition");
            break;
    }

}

