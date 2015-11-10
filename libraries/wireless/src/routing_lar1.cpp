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

#include "api.h"
#include "network_ip.h"
#include "routing_lar1.h"

/*
 * Notes:
 *   Implementation followed the specification of Mobicom 98 paper by
 *   Ko and Vaidya. Other details followed based on discussions with
 *   Mr. Youngbae Ko of Texas A & M.
 *
 *   We assume that underlying MAC protocol sends a signal when the packet
 *   cannot be reached to the next hop (after retransmissions). MAC protocols
 *   such as IEEE 802.11 and MACAW have these functionality.
 *   Nodes detect link breaks by receiving a signal from the IEEE 802.11 MAC
 *   Protocol. If other MAC protocol is used, users need to modify the LAR
 *   code so that it can detect link breaks (for instance, using passive acks(
 */


//----- Static Parameter Definitions -----//

#define LAR1_SEND_BUFFER_SIZE        (128)
#define LAR1_RREQ_JITTER             (10 * MILLI_SECOND)
#define LAR1_REQ_TIMEOUT             (2 * SECOND)
#define LAR1_MAX_ROUTE_LENGTH        (9)
#define LAR1_MAX_SEQ_NUM             (1024)
#define LAR1_REQUEST_SEEN_LIFETIME   (30 * SECOND)

//----- Packet Definitions -----//

typedef enum {
    LAR1_ROUTE_REQUEST,
    LAR1_ROUTE_REPLY,
    LAR1_ROUTE_ERROR
} LAR1_PacketType;

typedef struct lar1_routerequest {
    LAR1_PacketType packetType;
    NodeAddress sourceAddr;
    NodeAddress destAddr;
    BOOL flooding;
    int currentHop;
    int seqNum;
    LAR1_Zone requestZone;
} LAR1_RouteRequest;

typedef struct lar1_routereply {
    LAR1_PacketType packetType;
    NodeAddress sourceAddr;
    NodeAddress destAddr;
    int segmentLeft;
    double destVelocity;
    clocktype locationTimestamp;
    LAR1_Location destLocation;
} LAR1_RouteReply;

typedef struct lar1_routeerror {
    LAR1_PacketType packetType;
    NodeAddress sourceAddr;
    NodeAddress destAddr;
    NodeAddress fromHop;
    NodeAddress nextHop;
    int segmentLeft;
} LAR1_RouteError;

//----- Static Function Declarations -----//

//
// FUNCTION     Lar1NodeInZone()
// PURPOSE      Returns TRUE if node is within the zone coordinates
// PARAMETERS   zone            - structure containing corner coordinates
//
static BOOL
Lar1NodeInZone(
    Node *node,
    LAR1_Zone *zone);

//
// FUNCTION     Lar1HandleRouteErrorPacket()
// PURPOSE      Handle received LAR1 Route Error control packets
// PARAMETERS   msg             - the control packet
//
static void
Lar1HandleRouteErrorPacket(
    Node *node,
    Message *msg);

//
// FUNCTION     Lar1HandleRouteReply()
// PURPOSE      Handle received LAR1 Route Reply control packets
// PARAMETERS   msg             - the control packet
//
static void
Lar1HandleRouteReply(
    Node *node,
    Message *msg);

//
// FUNCTION     Lar1TransmitData()
// PURPOSE      Retrieve route from route cache, transmit data packet
// PARAMETERS   outMsg          - the packet to be sent
//
static void
Lar1TransmitData(
    Node *node,
    Message *outMsg);

//
// FUNCTION     Lar1RetrieveSendBuf()
// PURPOSE      Retrieve next data packet for transmission for specified
//              destination
// PARAMETERS   destAddr        - destination node
//
static Message *Lar1RetrieveSendBuf(
    Lar1Data *lar1,
    NodeAddress destAddr);

//
// FUNCTION     Lar1HandleRouteRequest()
// PURPOSE      Determine course of action for LAR RREQ packet
// PARAMETERS   msg     - the packet
//
static void
Lar1HandleRouteRequest(
    Node *node,
    Message *msg);


//
// FUNCTION     Lar1InitiateRouteReply()
// PURPOSE      Create and transmit LAR Route Reply packet
// PARAMETERS   oldMsg     - the original LAR Route Request packet
//
static void
Lar1InitiateRouteReply(
    Node *node,
    Message *oldMsg);


//
// FUNCTION     Lar1NodeInReqPath()
// PURPOSE      Return TRUE if node address appears in path array
// PARAMETERS   path    - array of NodeAddress
//              hopcount- number of entries in path array
//
static BOOL
Lar1NodeInReqPath(
    Node *node,
    NodeAddress *path,
    int hopcount);

//
// FUNCTION     Lar1FlushRequestSeenCache()
// PURPOSE      Remove Request Seen Cache entries older than
//              LAR1_REQUEST_SEEN_LIFETIME
// PARAMETERS   lar1            - LAR1 variable space
//

static void
Lar1FlushRequestSeenCache(
    Node* node,
    Lar1Data *lar1);

//
// FUNCTION     Lar1InsertRequestSeen()
// PURPOSE      Insert Request source address and sequence num into cache
// PARAMETERS   sourceAddr      - source of LAR Request Packet
//              seqNum          - sequence number assigned by source
//
static void
Lar1InsertRequestSeen(
    Node* node,
    Lar1Data *lar1,
    NodeAddress sourceAddr,
    int seqNum);


//
// FUNCTION     Lar1InsertRequestSent()
// PURPOSE      Insert destination address for locally generated
//              LAR Request Packet into cache
// PARAMETERS   destAddr        - destination address
//
static void
Lar1InsertRequestSent(
    Lar1Data *lar1,
    NodeAddress destAddr);


//
// FUNCTION     Lar1RemoveRequestSent()
// PURPOSE      Remove destination address for locally generated
//              LAR Request Packet from cache (Reply received)
// PARAMETERS   destAddr        - destination address
//
static void
Lar1RemoveRequestSent(
    Lar1Data *lar1,
    NodeAddress destAddr);

//
// FUNCTION     Lar1LookupRequestSeen(
// PURPOSE      Return TRUE if the (source addr, seq num) appears in cache
// PARAMETERS   sourceAddr      - source of LAR Request Packet
//              seqNum          - sequence number of LAR Request Packet
//
static BOOL
Lar1LookupRequestSeen(
    LAR1_RequestSeenEntry *reqSeen,
    NodeAddress sourceAddr,
    int seqNum);


//
// FUNCTION     Lar1PropagateRouteRequest()
// PURPOSE      Propagate a received LAR Route Request Packet
// PARAMETERS   oldMsg          - the received LAR Route Request Packet
//
static void
Lar1PropagateRouteRequest(
    Node *node,
    Message *oldMsg);


//
// FUNCTION     Lar1InitiateRouteRequest()
// PURPOSE      Initiate a new LAR Route Request Packet
// PARAMETERS   destAddr        - the destination for which route is needed
//
static void
Lar1InitiateRouteRequest(
    Node *node,
    NodeAddress destAddr);

//
// FUNCTION     Lar1CalculateReqZone()
// PURPOSE      Calculate and set the request zone in a LAR Request Packet
//              for a given destination
// PARAMETERS   msg             - the LAR Request Packet to be sent
//              destAddr        - the destination for which route is needed
//
static BOOL
Lar1CalculateReqZone(
    Node *node,
    Message *msg,
                                 NodeAddress destAddr);


//
// FUNCTION     Lar1BufferPacket()
// PURPOSE      Place packet into send buffer, awaiting valid path
//              and return TRUE if buffering is successful
// PARAMETERS   msg             - the data packet to be buffered
//              destAddr        - the destination of this data packet
//
static BOOL
Lar1BufferPacket(
    Node *node,
    Message *msg,
    NodeAddress destAddr);


//
// FUNCTION     Lar1PendingRouteReq()
// PURPOSE      Return TRUE if this node has sent a LAR Route Request Packet
//              for the given destination.
// PARAMETERS   destAddr        - the destination to check
//
static BOOL
Lar1PendingRouteReq(
    Node *node,
    NodeAddress destAddr);

//
// FUNCTION     Lar1RouteExists()
// PURPOSE      Return TRUE if this node has a valid route to the destination
// PARAMETERS   destAddr        - the destination to check
//
static BOOL
Lar1RouteExists(
    LAR1_RouteCacheEntry *cacheEntry,
    NodeAddress destAddr);


//
// FUNCTION     Lar1RetrieveCacheEntry()
// PURPOSE      Return the Route Cache Entry for the given destination if
//              one exists.
// PARAMETERS   cacheEntry      - the head pointer of the route cache
//              destAddr        - the destination to check
//
static LAR1_RouteCacheEntry
*Lar1RetrieveCacheEntry(
    LAR1_RouteCacheEntry *cacheEntry,
    NodeAddress destAddr);


//
// FUNCTION     Lar1InsertRoute()
// PURPOSE      Extract route information from a LAR Route Reply Packet
//              and insert info into the Route Cache.
// PARAMETERS   pkt             - the LAR Route Reply packet
//              path            - the path given in the route reply
//              numEntries      - the number of entries in the path array
//
static void
Lar1InsertRoute(
    Lar1Data *lar1,
    LAR1_RouteReply *pkt,
    NodeAddress *path,
    int numEntries);


//
// FUNCTION     Lar1SetTimer()
// PURPOSE      Set a timer to expire just in case a route reply is not
//              received in the allotted time.
// PARAMETERS   eventType       - the event that is triggered by the timer
//              destAddr        - the destination node that the timer is
//                                interested in
//              delay           - the delay between now and timer expiration
//
static void
Lar1SetTimer(
    Node *node,
    Int32 eventType,
    NodeAddress destAddr,
    clocktype delay);


//
// FUNCTION     Lar1FreeCacheEntry()
// PURPOSE      Free the memory used by a route rache entry
// PARAMETERS   cacheEntry      - the entry to free
//
static void
Lar1FreeCacheEntry(
    LAR1_RouteCacheEntry *cacheEntry);


//
// FUNCTION     Lar1InvalidateRoutesThroughBrokenLink()
// PURPOSE      Mark as unusable routes in the cache which contain the
//              given node pair
// PARAMETERS   fromHop         - the first node in the node pair
//              toHop           - the receiving node of the node pair
//
static void
Lar1InvalidateRoutesThroughBrokenLink(
    Node *node,
    NodeAddress fromHop,
    NodeAddress toHop);


//
// FUNCTION     Lar1DeleteRoute()
// PURPOSE      Remove route to the given destination from the route cache
// PARAMETERS   destAddr        - the given destination
//
static void
Lar1DeleteRoute(
    Node *node,
    NodeAddress destAddr);


//
// FUNCTION     Lar1TransmitErrorPacket()
// PURPOSE      Given a packet which MAC 802.11 was unable to transmit to
//              the neighbor node listed in the source route, this function
//              will transmit to the data packet's source node, a LAR
//              Route Error indicating that the route is broken at this link.
// PARAMETERS   oldMsg          - the data packet
//              nextHop         - the nextHop to which the data packet should
//                                have gone
//
static void
Lar1TransmitErrorPacket(
    Node *node,
    const Message *oldMsg,
    NodeAddress nextHop);

//
// FUNCTION     Lar1HandleBrokenLink()
// PURPOSE      Handle message from MAC 802.11 regarding broken link
// PARAMETERS   msg             - the packet returned by MAC 802.11
//
static void
Lar1HandleBrokenLink(
    Node *node,
    const Message *msg);

//----- End Static Function Declarations -----//


//
// FUNCTION     Lar1Init()
// PURPOSE      Initialize LAR1 Routing Protocol Dataspace
// PARAMETERS   lar1            - pointer for dataspace
//              nodeInput
//

void Lar1Init(
   Node *node,
   Lar1Data** lar1,
   const NodeInput *nodeInput,
   int interfaceIndex)
{
    int i;
    BOOL wasFound;
    char buf[80];

    if (MAC_IsWiredNetwork(node, interfaceIndex))
    {
        ERROR_Assert(FALSE, "LAR1 can only support wireless interfaces");
    }

    *lar1 = (Lar1Data *) MEM_malloc(sizeof(Lar1Data));

    RANDOM_SetSeed((*lar1)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_LAR1,
                   interfaceIndex);

    (*lar1)->routeCacheHead = NULL;
    (*lar1)->reqSeenHead = NULL;
    (*lar1)->reqSentHead = NULL;

    (*lar1)->sendBufHead = (*lar1)->sendBufTail = 0;

    (*lar1)->sendBuf =
        (LAR1_SendBufferEntry **)
        MEM_malloc(sizeof(LAR1_SendBufferEntry *) * LAR1_SEND_BUFFER_SIZE);

    for (i=0; i<LAR1_SEND_BUFFER_SIZE; i++)
    {
        (*lar1)->sendBuf[i] = NULL;
    }

    (*lar1)->seqNum = 0;
    (*lar1)->DataPacketsSentAsSource = 0;
    (*lar1)->DataPacketsRelayed = 0;
    (*lar1)->RouteRequestsSentAsSource = 0;
    (*lar1)->RouteRepliesSentAsRecvr = 0;
    (*lar1)->RouteErrorsSentAsErrorSource = 0;
    (*lar1)->RouteRequestsRelayed = 0;
    (*lar1)->RouteRepliesRelayed = 0;
    (*lar1)->RouteErrorsRelayed = 0;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &wasFound,
        buf);

    if ((wasFound == FALSE) || (strcmp(buf, "NO") == 0)) {
        (*lar1)->statsCollected = FALSE;
    }
    else if (strcmp(buf, "YES") == 0) {
        (*lar1)->statsCollected = TRUE;
    }

    (*lar1)->localAddress = NetworkIpGetInterfaceAddress(node, interfaceIndex);

    NetworkIpSetRouterFunction(node, &Lar1RouterFunction, interfaceIndex);
    NetworkIpSetMacLayerStatusEventHandlerFunction(node,
        &Lar1MacLayerStatusHandler, interfaceIndex);
#ifdef DEBUG
    printf("#%d: Lar1Init()\n", node->nodeId);
#endif
}


//
// FUNCTION     Lar1Finalize(Node* node)
// PURPOSE      Finalize statistics Collection
//

void Lar1Finalize(Node *node)
{
    Lar1Data* lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);
    char buf[MAX_STRING_LENGTH];

#ifdef DEBUG
    printf("#%d: Lar1Finalize(Node* node)\n", node->nodeId);
#endif

    if (lar1->statsCollected) {
        sprintf(buf, "Data Packets Sent As Data Source = %d",
                      lar1->DataPacketsSentAsSource);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);

        sprintf(buf, "Data Packets Relayed = %d",
                      lar1->DataPacketsRelayed);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);

        sprintf(buf, "Route Requests Sent As Data Source = %d",
                      lar1->RouteRequestsSentAsSource);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Route Replies Sent As Data Receiver = %d",
                      lar1->RouteRepliesSentAsRecvr);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);

        sprintf(buf, "Route Error Packets Sent As Source of Error = %d",
                      lar1->RouteErrorsSentAsErrorSource);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Route Requests Relayed As Intermediate Node = %d",
                      lar1->RouteRequestsRelayed);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Route Replies Relayed As Intermediate Node = %d",
                      lar1->RouteRepliesRelayed);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Route Error Packets Relayed As Intermediate Node = %d",
                      lar1->RouteErrorsRelayed);
        IO_PrintStat(
            node,
            "Network",
            "LAR1",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
    }
}


//
// FUNCTION     Lar1RouterFunction()
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              previousHopAddr - previous hop of packet
//              PacketWasRouted - variable that indicates to IP that it
//                                no longer needs to act on this data packet
//

void Lar1RouterFunction(Node *node,
                        Message *msg,
                        NodeAddress destAddr,
                        NodeAddress previousHopAddr,
                        BOOL* PacketWasRouted)
{
    Lar1Data *lar1;
    IpHeaderType *ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    if (destAddr == lar1->localAddress) {
        *PacketWasRouted = FALSE;
        return;
    }
#ifdef DEBUG
    printf("#%d:    Lar1HandleDatapacket()\n",node->nodeId);
#endif

    if (ipHeader->ip_src != lar1->localAddress) {
#ifdef DEBUG
        printf("        Source Routing this packet.\n");
#endif
        lar1->DataPacketsRelayed++;
        *PacketWasRouted = FALSE;
        return;
    }

    /* Check if Route In Cache */
    if (Lar1RouteExists(lar1->routeCacheHead, destAddr))
    {
        *PacketWasRouted = TRUE;
#ifdef DEBUG
        printf("        Route In Cache.\n");
#endif
        Lar1TransmitData(node, msg);
    }
    else /* No Route In Cache */
    {
        *PacketWasRouted = TRUE;

#ifdef DEBUG
        printf("        No Route in Cache to #%d.\n", destAddr);
#endif
        /* Check if there is already a pending route request for this dest */
        if (Lar1PendingRouteReq(node, destAddr))
        {
#ifdef DEBUG
            printf("            Already pending request.  Buffer pkt.\n");
#endif

            if (Lar1BufferPacket(node, msg, destAddr))
            {
#ifdef DEBUG
                printf("                Buffered size %d.\n",
                    MESSAGE_ReturnPacketSize(msg));
#endif
            }
            else
            {
#ifdef DEBUG
                printf("                Not enough space in buffer.  ",
                       "Dropped.\n");
#endif
                MESSAGE_Free(node, msg);
            }
        }
        else
        {
#ifdef DEBUG
            printf("            Initiate route request and buffer pkt.\n");
#endif
            if (Lar1BufferPacket(node, msg, destAddr))
            {
#ifdef DEBUG
                printf("                Buffered.  Initiate Route Request.\n");
                printf("                    size %d\n",
                    MESSAGE_ReturnPacketSize(msg));
#endif
                Lar1InsertRequestSeen(node, lar1, lar1->localAddress, lar1->seqNum);
                Lar1InitiateRouteRequest(node, destAddr);
            }
            else
            {
#ifdef DEBUG
                printf("                Not enough space in buffer.  ",
                       "Dropped.\n");
#endif
                MESSAGE_Free(node, msg);
            }
        }
    }
}


//
// FUNCTION     Lar1MacLayerStatusHandler()
// PURPOSE      Handle internal messages from MAC Layer
// PARAMETERS   msg             - the packet returned by MAC
//

void Lar1MacLayerStatusHandler(Node *node,
                               const Message* msg,
                               const NodeAddress nextHopAddress,
                               const int interfaceIndex)
{
#ifdef DEBUG
    printf("#%d: Lar1MacLayerStatusHandler()\n", node->nodeId);
#endif
    switch(msg->eventType) {
    case MSG_NETWORK_PacketDropped: {
#ifdef DEBUG
        printf("    PacketDropped.\n");
#endif
        Lar1HandleBrokenLink(node, msg);
        break;
    }
    default:
        printf("#%d: Cannot handle message type #%d\n",
               node->nodeId, msg->eventType);
        assert(FALSE);
    }
}


//
// FUNCTION     Lar1HandleProtocolPacket(Node* node, Message* msg,  NodeAddress sourceAddress)
// PURPOSE      Process a LAR1 generated control packet
// PARAMETERS   msg             - The packet
//

void Lar1HandleProtocolPacket(Node* node, Message* msg)
{
    LAR1_PacketType* larHeader = (LAR1_PacketType*)MESSAGE_ReturnPacket(msg);
#ifdef DEBUG
    printf("#%d:         LAR packet.\n",node->nodeId);
#endif

    switch (*larHeader) {
    case LAR1_ROUTE_REQUEST: {
#ifdef DEBUG
        int numEntries;

        /* Calculate the number of node addresses that are included in this
           packet by taking the full size of the packet, and subtracting the
           size of the LAR1 Route Reply Header, and then dividing by the
           size of a node address */

        numEntries = MESSAGE_ReturnPacketSize(msg);
        numEntries -= sizeof(LAR1_RouteRequest);
        numEntries = numEntries / sizeof(NodeAddress);
        printf("            Route Request Packet %d entries.\n",
               numEntries);
#endif
        Lar1HandleRouteRequest(node, msg);
        MESSAGE_Free(node, msg);
        break;
    }
    case LAR1_ROUTE_REPLY: {
#ifdef DEBUG
        int numEntries;

        /* Calculate the number of node addresses that are included in this
           packet by taking the full size of the packet, and subtracting the
           size of the LAR1 Route Reply Header, and then dividing by the
           size of a node address */

        numEntries = MESSAGE_ReturnPacketSize(msg);
        numEntries -= sizeof(LAR1_RouteReply);
        numEntries = numEntries / sizeof(NodeAddress);
        printf("            Route Reply Packet %d entries.\n",
               numEntries);
#endif
        Lar1HandleRouteReply(node, msg);
        break;
    }
    case LAR1_ROUTE_ERROR: {
#ifdef DEBUG
        int numEntries;

        /* Calculate the number of node addresses that are included in this
           packet by taking the full size of the packet, and subtracting the
           size of the LAR1 Route Reply Header, and then dividing by the
           size of a node address */

        numEntries = MESSAGE_ReturnPacketSize(msg);
        numEntries -= sizeof(LAR1_RouteError);
        numEntries = numEntries / sizeof(NodeAddress);
        printf("            Route Error Packet %d entries.\n",
               numEntries);
#endif
        Lar1HandleRouteErrorPacket(node, msg);
        break;
    }
    default:
        assert(FALSE);
    }//switch//
}

//
// FUNCTION     Lar1HandleCheckTimeoutAlarm()
// PURPOSE      Process timeouts sent by LAR1 to itself
// PARAMETERS   msg             - the timer
//

void Lar1HandleCheckTimeoutAlarm(Node* node, Message* msg) {

    NodeAddress *info = (NodeAddress *)MESSAGE_ReturnInfo(msg);
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


#ifdef DEBUG
    printf("#%d: Check for route reply for dest %d\n", node->nodeId,
           *info);
#endif

    /* Check if Route In Cache */
    if (Lar1RouteExists(lar1->routeCacheHead, *info))
    {
        /* Already received the anticipated Route Reply packet */
        MESSAGE_Free(node, msg);
    }
    else
    {
       /* Remove route and related information from route cache */
       Lar1DeleteRoute(node, *info);

       /* Need to retransmit a Route Request packet */
       Lar1InitiateRouteRequest(node, *info);
       MESSAGE_Free(node, msg);
    }
}



//
// FUNCTION     Lar1NodeInZone()
// PURPOSE      Returns TRUE if node is within the zone coordinates
// PARAMETERS   zone            - structure containing corner coordinates
//

static BOOL Lar1NodeInZone(Node *node, LAR1_Zone *zone)
{
    Coordinates coordinates;

    MOBILITY_ReturnCoordinates(node, &coordinates);

#ifdef DEBUG
    printf("        GPS = (%f, %f)\n",
           coordinates.common.c1,
           coordinates.common.c2);
#endif
    if (coordinates.common.c1 >= zone->bottomLeft.x &&
        coordinates.common.c1 <= zone->topRight.x + 1&&
        coordinates.common.c2 >= zone->bottomLeft.y &&
        coordinates.common.c2 <= zone->topRight.y + 1)
    {
        return TRUE;
    }

    return FALSE;
}

//
// FUNCTION     Lar1HandleRouteErrorPacket()
// PURPOSE      Handle received LAR1 Route Error control packets
// PARAMETERS   msg             - the control packet
//

static void Lar1HandleRouteErrorPacket(Node *node, Message *msg)
{
    LAR1_RouteError *pkt = (LAR1_RouteError *) MESSAGE_ReturnPacket(msg);
    char *pktptr;
    NodeAddress *path;
    NodeAddress fromHop, nextHop;
    int left,
        numEntries;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


#ifdef DEBUG
    printf("    HandleRouteErrorPacket()\n");
#endif

    /* Calculate the number of node addresses that are included in this
       packet by taking the full size of the packet, and subtracting the
       size of the LAR1 Route Reply Header, and then dividing by the
       size of a node address */

    numEntries = MESSAGE_ReturnPacketSize(msg);
    numEntries -= sizeof(LAR1_RouteError);
    numEntries = numEntries / sizeof(NodeAddress);

    /* Record the segmentLeft */
    left = pkt->segmentLeft;

    /* Record the node pair involved in the link break */
    fromHop = pkt->fromHop;
    nextHop = pkt->nextHop;

    /* Delete routes in cache that use the broken link */
    Lar1InvalidateRoutesThroughBrokenLink(node, fromHop,
                                             nextHop);

    /* Position the path pointer onto the array of node addresses */
    pktptr = (char *) MESSAGE_ReturnPacket(msg) + sizeof(LAR1_RouteError);
    path = (NodeAddress *) pktptr;
#ifdef DEBUG
    printf("        destAddr = %d, path[left] = %d\n", pkt->destAddr,
           path[left]);
#endif

    /* Decrease the segment left */
    pkt->segmentLeft = pkt->segmentLeft - 1;

    if ((pkt->destAddr == lar1->localAddress) &&
        (path[left] == lar1->localAddress))
    { /* This error notification has reached its destination (i.e., the source
         of the broken route */
#ifdef DEBUG
        printf("    I am destination for this route error packet.\n");
#endif
        MESSAGE_Free(node, msg);
    }
    else if (path[left] == lar1->localAddress)
    { /* This error notification has reached an intermediate node of the broken
         route */
        NodeAddress nextHop = path[pkt->segmentLeft];

#ifdef DEBUG
        printf("    I am relay for this route error packet.\n");
        printf("        Send it on to %d\n", nextHop);
#endif

        lar1->RouteErrorsRelayed++;
        /* Propagate this control message towards its destination */
        NetworkIpSendRawMessageToMacLayer(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
            nextHop,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_LAR1,
            LAR1_MAX_ROUTE_LENGTH,
            DEFAULT_INTERFACE,
            nextHop);

    }
    else
    {
        /* This packet should not have reached this node, because it should
           have been unicasted to the next node in the path towards the
           data source. */
        MESSAGE_Free(node, msg);
        assert(FALSE);
    }
}

//
// FUNCTION     Lar1HandleRouteReply()
// PURPOSE      Handle received LAR1 Route Reply control packets
// PARAMETERS   msg             - the control packet
//

static void Lar1HandleRouteReply(Node *node, Message *msg)
{
    LAR1_RouteReply *pkt = (LAR1_RouteReply *) MESSAGE_ReturnPacket(msg);
    char *pktptr;
    NodeAddress *path;
    int left;
    int numEntries;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    /* Calculate the number of node addresses that are included in this
       packet by taking the full size of the packet, and subtracting the
       size of the LAR1 Route Reply Header, and then dividing by the
       size of a node address */

    numEntries = MESSAGE_ReturnPacketSize(msg);
    numEntries -= sizeof(LAR1_RouteReply);
    numEntries = numEntries / sizeof(NodeAddress);

    /* Decrease the segment left */
    pkt->segmentLeft = pkt->segmentLeft - 1;

    left = pkt->segmentLeft;

    /* Position the path pointer onto the array of node addresses */
    pktptr = (char *) MESSAGE_ReturnPacket(msg) + sizeof(LAR1_RouteReply);
    path = (NodeAddress *) pktptr;

#ifdef DEBUG
    printf("        destAddr = %d, path[left] = %d\n", pkt->destAddr,
           path[left]);
#endif

    if ((pkt->destAddr == lar1->localAddress) &&
        (path[left] == lar1->localAddress) &&
        (!Lar1RouteExists(lar1->routeCacheHead, pkt->destAddr)))
    { /* The node originated the route request received the first rotue reply */
        Message *bufMsg;

        /* Remove this request from the list of outstanding route requests */
        Lar1RemoveRequestSent(lar1, pkt->sourceAddr);
#ifdef DEBUG
        {
            int i;
            printf("        I have received valid and useful path to %d\n",
                   pkt->sourceAddr);
            for (i = 0; i < numEntries; i++)
            {
                printf("            path step #%d = %d\n", i, path[i]);
            }
        }
#endif

        /* Insert the new route into the route cache */
        Lar1InsertRoute(lar1, pkt, path, numEntries);

        /* Retrieve and send all packets in the buffer for this destination */
        bufMsg = Lar1RetrieveSendBuf(lar1, pkt->sourceAddr);
        while (bufMsg != NULL)
        {
#ifdef DEBUG
            printf("        Send Message to %d size %d\n",
                   pkt->sourceAddr, MESSAGE_ReturnPacketSize(bufMsg));
#endif

            /* Transmit the data packet */
            Lar1TransmitData(node, bufMsg);
            bufMsg = Lar1RetrieveSendBuf(lar1, pkt->sourceAddr);
        }

        MESSAGE_Free(node, msg);
    }
    else if (path[left] == lar1->localAddress)
    { /* This node is an intermediate node for the route reply packet */
        NodeAddress nextHop = path[pkt->segmentLeft-1];
#ifdef DEBUG
        printf("       I'm the intended intermediate node.\n");
        printf("       Propagate Route Reply for %d to %d\n",
               pkt->destAddr, nextHop);
#endif

        lar1->RouteRepliesRelayed++;
        /* Propagate this control message towards its destination */
        NetworkIpSendRawMessageToMacLayer(
            node,
            msg,
            NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
            nextHop,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_LAR1,
            LAR1_MAX_ROUTE_LENGTH,
            DEFAULT_INTERFACE,
            nextHop);

    }
    else
    {
        /* This packet should not have reached this node, because it should
           have been unicasted to the next node in the path towards the
           data source. */
        MESSAGE_Free(node, msg);
        assert(FALSE);
    }

}

//
// FUNCTION     Lar1TransmitData()
// PURPOSE      Retrieve route from route cache, transmit data packet
// PARAMETERS   outMsg          - the packet to be sent
//

static void Lar1TransmitData(Node *node, Message *outMsg)
{
    int numRoutes;
    NodeAddress *routes;
    IpHeaderType *ipHdr = (IpHeaderType *)MESSAGE_ReturnPacket(outMsg);
    NodeAddress destAddr = ipHdr->ip_dst;
    LAR1_RouteCacheEntry *cache_entry;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

    IpOptionsHeaderType *ipOption =
                      IpHeaderSourceRouteOptionField(ipHdr);
    if (ipOption)
    {
        memcpy((char *)&destAddr,
               (char *)ipOption + ipOption->len - sizeof(NodeAddress),
                           sizeof(NodeAddress));
    }

    /* Retrieve the cache entry for the desired destination */
    cache_entry = Lar1RetrieveCacheEntry(lar1->routeCacheHead,
                                                destAddr);
    assert(cache_entry);

    /* Extract route information from the cache entry */
    routes = cache_entry->path;
    numRoutes = cache_entry->pathLength;

    if (numRoutes >1)
    {
        NodeAddress* routeAddresses;
        int numRouteAddresses;

        memcpy((char *)&ipHdr->ip_dst, (char *)routes,
                                                       sizeof(NodeAddress));
        routeAddresses = routes;
        numRouteAddresses = numRoutes - 1;
        routeAddresses++;
    BOOL removeExistingRecordedRoute = TRUE;
    if (ipHdr->ip_src != lar1->localAddress)
    {
        removeExistingRecordedRoute = FALSE;
    }
    else
    {
        removeExistingRecordedRoute = TRUE;
    }
    /* Use route information to send source routed IP datagram */
    NetworkIpSendPacketToMacLayerWithNewStrictSourceRoute(
           node, outMsg, routeAddresses, numRouteAddresses,
           removeExistingRecordedRoute);
    }
    else
    {
        int outgoingInterface;
        NodeAddress nextHop;

        //calculate outgoing interface to route the packet
        memcpy((char *)&nextHop, (char *)&ipHdr->ip_dst,
                                                       sizeof(NodeAddress));
        outgoingInterface = NetworkIpGetInterfaceIndexForNextHop(node,
                                                                 nextHop);
        NetworkIpSendPacketOnInterface(node,
                                       outMsg,
                                       CPU_INTERFACE,
                                       outgoingInterface,
                                       nextHop);
    }

    lar1->DataPacketsSentAsSource++;
}


//
// FUNCTION     Lar1RetrieveSendBuf()
// PURPOSE      Retrieve next data packet for transmission for specified
//              destination
// PARAMETERS   destAddr        - destination node
//

static Message *Lar1RetrieveSendBuf(Lar1Data *lar1,
                                    NodeAddress destAddr)
{
    LAR1_SendBufferEntry *entry;
    int bufsize;
    int index;
    int i;

    bufsize = (lar1->sendBufTail + LAR1_SEND_BUFFER_SIZE) - lar1->sendBufHead;
    bufsize = bufsize % LAR1_SEND_BUFFER_SIZE;

    if (bufsize == 0)
        return NULL;

    index = lar1->sendBufHead;

    /* Update Head Pointer */
    entry = lar1->sendBuf[index];
    while ((entry == NULL) &&
           (lar1->sendBufHead != lar1->sendBufTail) &&
           (bufsize > 0))
    {
#ifdef DEBUG
        printf("                Advance Head Pointer over useless entry.\n");
#endif
        lar1->sendBufHead = (lar1->sendBufHead + 1) % LAR1_SEND_BUFFER_SIZE;
        bufsize -= 1;
        index = lar1->sendBufHead;
        entry = lar1->sendBuf[index];
    }

    /* Return and remove first useful entry, if any */
    for (i = 0; i < bufsize; i++)
    {
        index = (lar1->sendBufHead + i) % LAR1_SEND_BUFFER_SIZE;
        entry = lar1->sendBuf[index];
        if (entry != NULL)
        {
            if (entry->destAddr == destAddr)
            {
                Message *outMsg = entry->msg;

                assert(outMsg);
                MEM_free(entry);
                lar1->sendBuf[index] = NULL;
                return outMsg;
            }
        }
    }

    return NULL;
}

//
// FUNCTION     Lar1HandleRouteRequest()
// PURPOSE      Determine course of action for LAR RREQ packet
// PARAMETERS   msg     - the packet
//

static void Lar1HandleRouteRequest(Node *node, Message *msg)
{
    LAR1_RouteRequest *pkt = (LAR1_RouteRequest *) MESSAGE_ReturnPacket(msg);
    char *path;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    /* Increase the hop count */
    pkt->currentHop = pkt->currentHop + 1;

    /* Process packet only if the node is within the request zone */
    /* or the packet is purely flooded */
    if (!(pkt->flooding) && !(Lar1NodeInZone(node, &pkt->requestZone)))
    {
#ifdef DEBUG
        printf("            Not in Zone, Not Flooding Pkt. Discard.\n");
#endif
        return;
    }

    if (pkt->currentHop <= LAR1_MAX_ROUTE_LENGTH)
    {
        /* if not seen before */
        Lar1FlushRequestSeenCache(node, lar1);
        if (!Lar1LookupRequestSeen(lar1->reqSeenHead, pkt->sourceAddr,
                                          pkt->seqNum))
        {
#ifdef DEBUG
            printf("            First Time Seeing This Request.\n");
#endif

            Lar1InsertRequestSeen(node, lar1, pkt->sourceAddr, pkt->seqNum);
            path = MESSAGE_ReturnPacket(msg) + sizeof(LAR1_RouteRequest);

            if (!Lar1NodeInReqPath(node, (NodeAddress *) path,
                                          pkt->currentHop))
            {
                if (pkt->destAddr == lar1->localAddress)
                {
#ifdef DEBUG
                    printf("            I am the destination node.\n");
#endif
                    Lar1InitiateRouteReply(node, msg);
                }
                else
                {
                    /* relay the packet */
#ifdef DEBUG
                    printf("            Relay the packet.\n");
#endif
                    Lar1PropagateRouteRequest(node, msg);
                }
            }
            else
            {
#ifdef DEBUG
                printf("            This node already in traversed path.\n");
#endif
                return;
            }
        }
        else
        {
#ifdef DEBUG
            printf("            Request already seen.  Discard.\n");
#endif
            return;
        }
    }
    else
    {
#ifdef DEBUG
        printf("            Over Hop Limit. Discard.\n");
#endif
        return;
    }
}


//
// FUNCTION     Lar1InitiateRouteReply()
// PURPOSE      Create and transmit LAR Route Reply packet
// PARAMETERS   oldMsg     - the original LAR Route Request packet
//

static void Lar1InitiateRouteReply(Node *node, Message *oldMsg)
{
    Message *newMsg;
    LAR1_RouteRequest *pkt;
    LAR1_RouteReply *reply;
    NodeAddress *n_addr;
    NodeAddress *old_n_addr;
    NodeAddress nextHop;
    Coordinates coordinates;
    char *pktptr;
    int pktSize = sizeof(LAR1_RouteReply);
    Lar1Data *lar1;
    clocktype currentTime = node->getNodeTime();

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    pkt = (LAR1_RouteRequest *) MESSAGE_ReturnPacket(oldMsg);

    pktSize += (sizeof(NodeAddress) * (pkt->currentHop + 1));

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_FromNetwork);
    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_LAR1);

    pktptr = (char *) MESSAGE_ReturnPacket(newMsg);
    reply = (LAR1_RouteReply *) pktptr;
    /* Position the n_addr pointer onto the array of node addresses */
    pktptr += sizeof(LAR1_RouteReply);
    n_addr = (NodeAddress *) pktptr;

    /* Position the old_n_addr pointer onto the array of node addresses */
    pktptr = (char *) MESSAGE_ReturnPacket(oldMsg) +
             sizeof(LAR1_RouteRequest);
    old_n_addr = (NodeAddress *) pktptr;

    MOBILITY_ReturnCoordinates(node, &coordinates);

    reply->packetType = LAR1_ROUTE_REPLY;
    reply->sourceAddr = pkt->destAddr;
    reply->destAddr = pkt->sourceAddr;
    reply->segmentLeft = pkt->currentHop;

    reply->destLocation.x = (Int32)(EARTH_RADIUS * 
                  cos(coordinates.latlonalt.latitude * IN_RADIAN) * 
                  cos(coordinates.latlonalt.longitude * IN_RADIAN));
    reply->destLocation.y = (Int32)(EARTH_RADIUS * 
                  cos(coordinates.latlonalt.latitude * IN_RADIAN) * 
                  sin(coordinates.latlonalt.longitude * IN_RADIAN));

    reply->locationTimestamp = currentTime;
    MOBILITY_ReturnInstantaneousSpeed(node, &(reply->destVelocity));

    memcpy(n_addr, old_n_addr, (sizeof(NodeAddress) * pkt->currentHop));

    n_addr[pkt->currentHop] = lar1->localAddress;

#ifdef DEBUG
    printf("        Transmit Route Reply for %d to %d, i = %d\n",
           reply->destAddr, n_addr[pkt->currentHop-1], pkt->currentHop);
#endif
    nextHop = n_addr[pkt->currentHop-1];
    lar1->RouteRepliesSentAsRecvr++;
    NetworkIpSendRawMessageToMacLayer(
        node,
        newMsg,
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
        nextHop,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_LAR1,
        LAR1_MAX_ROUTE_LENGTH,
        DEFAULT_INTERFACE,
        nextHop);

}


//
// FUNCTION     Lar1NodeInReqPath()
// PURPOSE      Return TRUE if node address appears in path array
// PARAMETERS   path    - array of NodeAddress
//              hopcount- number of entries in path array
//

static BOOL Lar1NodeInReqPath(Node *node, NodeAddress *path, int hopcount)
{
    Lar1Data *lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);
    int i;

    for (i = 0; i < hopcount; i++)
    {
#ifdef DEBUG
        printf("                path[%d] = %d\n", i, path[i]);
#endif
        if (path[i] == lar1->localAddress)
            return TRUE;
    }

    return FALSE;
}


//
// FUNCTION     Lar1FlushRequestSeenCache()
// PURPOSE      Remove Request Seen Cache entries older than
//              LAR1_REQUEST_SEEN_LIFETIME
// PARAMETERS   lar1            - LAR1 variable space
//

static void Lar1FlushRequestSeenCache(Node* node,
                                      Lar1Data *lar1)
{
    LAR1_RequestSeenEntry *head = lar1->reqSeenHead;
    LAR1_RequestSeenEntry *parent = NULL;
    BOOL firstEntry = TRUE;

    while ((head != NULL) && firstEntry)
    {
        if (head->lifetime < node->getNodeTime())
        {
            lar1->reqSeenHead = head->next;
            MEM_free(head);
            head = lar1->reqSeenHead;
        }
        else
        {
            parent = head;
            head = head->next;
            firstEntry = FALSE;
        }
    }

    while (head != NULL)
    {
        if (head->lifetime < node->getNodeTime())
        {
            parent->next = head->next;
            MEM_free(head);
            head = parent->next;
        }
        else
        {
            parent = head;
            head = head->next;
        }
    }
}


//
// FUNCTION     Lar1InsertRequestSeen()
// PURPOSE      Insert Request source address and sequence num into cache
// PARAMETERS   sourceAddr      - source of LAR Request Packet
//              seqNum          - sequence number assigned by source
//

static void Lar1InsertRequestSeen(Node* node,
                                  Lar1Data *lar1,
                                  NodeAddress sourceAddr,
                                  int seqNum)
{
    LAR1_RequestSeenEntry *entry =
        (LAR1_RequestSeenEntry *) MEM_malloc(sizeof(LAR1_RequestSeenEntry));

    assert(entry);

    entry->sourceAddr = sourceAddr;
    entry->seqNum = seqNum;
    entry->lifetime = node->getNodeTime() + LAR1_REQUEST_SEEN_LIFETIME;
    entry->next = lar1->reqSeenHead;

    lar1->reqSeenHead = entry;
}


//
// FUNCTION     Lar1InsertRequestSent()
// PURPOSE      Insert destination address for locally generated
//              LAR Request Packet into cache
// PARAMETERS   destAddr        - destination address
//

static void Lar1InsertRequestSent(Lar1Data *lar1,
                                  NodeAddress destAddr)
{
    LAR1_RequestSentEntry *entry =
        (LAR1_RequestSentEntry *) MEM_malloc(sizeof(LAR1_RequestSentEntry));

    entry->destAddr = destAddr;
    entry->next = lar1->reqSentHead;
    lar1->reqSentHead = entry;
}

//
// FUNCTION     Lar1RemoveRequestSent()
// PURPOSE      Remove destination address for locally generated
//              LAR Request Packet from cache (Reply received)
// PARAMETERS   destAddr        - destination address
//

static void Lar1RemoveRequestSent(Lar1Data *lar1,
                                  NodeAddress destAddr)
{
    LAR1_RequestSentEntry *entry = lar1->reqSentHead;
    LAR1_RequestSentEntry *parent = NULL;
    BOOL firstEntry = TRUE;

    while ((entry != NULL) && firstEntry)
    {
        if (entry->destAddr == destAddr)
        {
            lar1->reqSentHead = entry->next;
            MEM_free(entry);
            entry = lar1->reqSentHead;
        }
        else
        {
            firstEntry = FALSE;
            parent = entry;
            entry = entry->next;
        }
    }

    while (entry != NULL)
    {
        if (entry->destAddr == destAddr)
        {
            parent->next = entry->next;
            MEM_free(entry);
            entry = parent->next;
        }
        else
        {
            parent = entry;
            entry = entry->next;
        }
    }
}


//
// FUNCTION     Lar1LookupRequestSeen()
// PURPOSE      Return TRUE if the (source addr, seq num) appears in cache
// PARAMETERS   sourceAddr      - source of LAR Request Packet
//              seqNum          - sequence number of LAR Request Packet
//

static BOOL Lar1LookupRequestSeen(LAR1_RequestSeenEntry *reqSeen,
                                  NodeAddress sourceAddr,
                                  int seqNum)
{

    while (reqSeen != NULL)
    {
        if ((reqSeen->sourceAddr == sourceAddr) &&
            (reqSeen->seqNum == seqNum))
            return TRUE;
        reqSeen = reqSeen->next;
    }
    return FALSE;
}


//
// FUNCTION     Lar1PropagateRouteRequest()
// PURPOSE      Propagate a received LAR Route Request Packet
// PARAMETERS   oldMsg          - the received LAR Route Request Packet
//

static void Lar1PropagateRouteRequest(Node *node, Message *oldMsg)
{
    Message *newMsg;
    LAR1_RouteRequest *rreq,
                      *pkt;
    NodeAddress *n_addr,
              *old_n_addr;
    char *pktptr;
    int pktSize = sizeof(LAR1_RouteRequest);
    clocktype delay;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

    delay = (clocktype) (RANDOM_erand(lar1->seed) * LAR1_RREQ_JITTER);

    pkt = (LAR1_RouteRequest *) MESSAGE_ReturnPacket(oldMsg);
    pktSize += (sizeof(NodeAddress) * (pkt->currentHop + 1));

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_FromNetwork);
    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_LAR1);

    pktptr = (char *) MESSAGE_ReturnPacket(newMsg);
    rreq = (LAR1_RouteRequest *) pktptr;
    /* Position the n_addr pointer onto the array of node addresses */
    pktptr += sizeof(LAR1_RouteRequest);
    n_addr = (NodeAddress *) pktptr;

    /* Position the old_n_addr pointer onto the array of node addresses */
    pktptr = (char *) MESSAGE_ReturnPacket(oldMsg) +
             sizeof(LAR1_RouteRequest);
    old_n_addr = (NodeAddress *) pktptr;

    rreq->packetType = LAR1_ROUTE_REQUEST;
    rreq->sourceAddr = pkt->sourceAddr;
    rreq->destAddr = pkt->destAddr;
    rreq->seqNum = pkt->seqNum;
    rreq->currentHop = pkt->currentHop;
    rreq->flooding = pkt->flooding;
    memmove(&(rreq->requestZone), &(pkt->requestZone), sizeof(LAR1_Zone));

    memcpy(n_addr, old_n_addr, sizeof(NodeAddress) * pkt->currentHop);

    n_addr[pkt->currentHop] = lar1->localAddress;

    lar1->RouteRequestsRelayed++;
    /* Transmit Route Request */
    NetworkIpSendRawMessageWithDelay(
                node,
                newMsg,
                ANY_IP,
                ANY_DEST,
                ANY_INTERFACE,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_LAR1,
                0,
                delay);
}


//
// FUNCTION     Lar1InitiateRouteRequest()
// PURPOSE      Initiate a new LAR Route Request Packet
// PARAMETERS   destAddr        - the destination for which route is needed
//

static void Lar1InitiateRouteRequest(Node *node, NodeAddress destAddr)
{
    Message *newMsg;
    LAR1_RouteRequest *rreq;
    NodeAddress *n_addr;
    char *pktptr;
    int pktSize = sizeof(LAR1_RouteRequest) + sizeof(NodeAddress);
    clocktype delay;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    delay = (clocktype) (RANDOM_erand(lar1->seed) * LAR1_RREQ_JITTER);

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_FromNetwork);
    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_LAR1);

    pktptr = (char *) MESSAGE_ReturnPacket(newMsg);
    rreq = (LAR1_RouteRequest *) pktptr;
    /* Position the n_addr pointer onto the array of node addresses */
    pktptr += sizeof(LAR1_RouteRequest);
    n_addr = (NodeAddress *) pktptr;

    rreq->packetType = LAR1_ROUTE_REQUEST;
    rreq->sourceAddr = lar1->localAddress;
    rreq->destAddr = destAddr;
    rreq->currentHop = 0;
    rreq->seqNum = lar1->seqNum;
    lar1->seqNum = (lar1->seqNum + 1) % LAR1_MAX_SEQ_NUM;

    /* Calculate Request Zone */
    if (Lar1CalculateReqZone(node, newMsg, destAddr))
        rreq->flooding = FALSE;
    else
        rreq->flooding = TRUE;

    n_addr[0] = lar1->localAddress;

    lar1->RouteRequestsSentAsSource++;
    /* Transmit Route Request */
    NetworkIpSendRawMessageWithDelay(
                node,
                newMsg,
                ANY_IP,
                ANY_DEST,
                ANY_INTERFACE,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_LAR1,
                0,
                delay);

    Lar1SetTimer(node, MSG_NETWORK_CheckTimeoutAlarm, destAddr,
                        LAR1_REQ_TIMEOUT);

    Lar1InsertRequestSent(lar1, destAddr);
}


//
// FUNCTION     Lar1CalculateReqZone()
// PURPOSE      Calculate and set the request zone in a LAR Request Packet
//              for a given destination
// PARAMETERS   msg             - the LAR Request Packet to be sent
//              destAddr        - the destination for which route is needed
//

static BOOL Lar1CalculateReqZone(Node *node, Message *msg,
                                 NodeAddress destAddr)
{
    LAR1_Zone zone;
    double radius;
    double velocity = 0.0;
    Int32 xSource = 0;
    Int32 ySource = 0;
    Int32 xDestination = 0;
    Int32 yDestination = 0 ;
    Coordinates coordinates;
    clocktype destinationTime = 0;
    BOOL found = FALSE;
    LAR1_RouteCacheEntry *current;
    LAR1_RouteRequest *rreq;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    rreq = (LAR1_RouteRequest *) MESSAGE_ReturnPacket(msg);
    /* Get source location info */
    MOBILITY_ReturnCoordinates(node, &coordinates);

    xSource = (Int32)(EARTH_RADIUS * 
                  cos(coordinates.latlonalt.latitude * IN_RADIAN) * 
                  cos(coordinates.latlonalt.longitude * IN_RADIAN));
    ySource = (Int32)(EARTH_RADIUS * 
                  cos(coordinates.latlonalt.latitude * IN_RADIAN) * 
                  sin(coordinates.latlonalt.longitude * IN_RADIAN));


    /* Get destination location info */
    current = lar1->routeCacheHead;
    while (current != NULL)
    {
        if (current->destAddr == destAddr)
        {
            found = TRUE;
            velocity = current->destVelocity;
            destinationTime = current->locationTimestamp;
            xDestination = current->destLocation.x;
            yDestination = current->destLocation.y;

            break;
        }
        current = current->next;
    }

    if (!found){
#ifdef DEBUG
        printf("                No location data on node %d\n", destAddr);
#endif
        memset(&(rreq->requestZone), 0, sizeof(LAR1_Zone));
        return FALSE;
    }
    else {
#ifdef DEBUG
        printf("            Location data found for node %d\n", destAddr);
#endif
        /* Calculate radius to find the request zone */
        radius = velocity * (double)((node->getNodeTime() - destinationTime) / SECOND);
#ifdef DEBUG
        printf("            velocity = %f  radius = %f\n",velocity,radius);
        printf("    Source = (%d,%d), Dest = (%d, %d)\n",
            xSource, ySource, xDestination, yDestination);
#endif

        /* Calculate reqest zone if source node is within expected zone */
        if ((xSource >= (xDestination - radius)) &&
            (xSource <= (xDestination + radius)) &&
            (ySource >= (yDestination - radius)) &&
            (ySource <= (yDestination + radius)))
        {
#ifdef DEBUG
            printf("            Source Node is Within Expected Zone.\n");
#endif
            zone.bottomLeft.x = (Int32)(xDestination - radius);
            zone.bottomLeft.y = (Int32)(yDestination - radius);
            zone.topLeft.x = (Int32)(xDestination - radius);
            zone.topLeft.y = (Int32)(yDestination + radius);
            zone.topRight.x = (Int32)(xDestination + radius);
            zone.topRight.y = (Int32)(yDestination + radius);
            zone.bottomRight.x = (Int32)(xDestination + radius);
            zone.bottomRight.y =(Int32)( yDestination - radius);
        }
        /* Calculate request zone if source node is outside expected zone */
        else
        {
#ifdef DEBUG
            printf("            Source Node is Outside Expected Zone.\n");
#endif
            if (xSource < xDestination-radius)
            {
                if (ySource < yDestination-radius)
                {
                    zone.bottomLeft.x = xSource;
                    zone.bottomLeft.y = ySource;
                    zone.topLeft.x = xSource;
                    zone.topLeft.y = (Int32)(yDestination + radius);
                    zone.topRight.x = (Int32)(xDestination + radius);
                    zone.topRight.y = (Int32)(yDestination + radius);
                    zone.bottomRight.x = (Int32)(xDestination + radius);
                    zone.bottomRight.y = ySource;
                }
                else if (ySource >= yDestination-radius &&
                         ySource <= yDestination+radius)
                {
                    zone.bottomLeft.x = xSource;
                    zone.bottomLeft.y = (Int32)(yDestination-radius);
                    zone.topLeft.x = xSource;
                    zone.topLeft.y = (Int32)(yDestination + radius);
                    zone.topRight.x = (Int32)(xDestination + radius);
                    zone.topRight.y = (Int32)(yDestination + radius);
                    zone.bottomRight.x = (Int32)(xDestination + radius);
                    zone.bottomRight.y = (Int32)(yDestination - radius);
                }
                else if (ySource > yDestination+radius)
                {
                    zone.bottomLeft.x = xSource;
                    zone.bottomLeft.y = (Int32)(yDestination - radius);
                    zone.topLeft.x = xSource;
                    zone.topLeft.y = ySource;
                    zone.topRight.x = (Int32)(xDestination + radius);
                    zone.topRight.y = ySource;
                    zone.bottomRight.x = (Int32)(xDestination + radius);
                    zone.bottomRight.y = (Int32)(yDestination - radius);
                }
            }
            else if (xSource >= xDestination-radius &&
                     xSource <= xDestination+radius)
            {
                if (ySource < yDestination-radius)
                {
                    zone.bottomLeft.x = (Int32)(xDestination - radius);
                    zone.bottomLeft.y = ySource;
                    zone.topLeft.x = (Int32)(xDestination - radius);
                    zone.topLeft.y = (Int32)(yDestination + radius);
                    zone.topRight.x = (Int32)(xDestination + radius);
                    zone.topRight.y = (Int32)(yDestination + radius);
                    zone.bottomRight.x = (Int32)(xDestination + radius);
                    zone.bottomRight.y = ySource;
                }
                else if (ySource > yDestination+radius)
                {
                    zone.bottomLeft.x = (Int32)(xDestination - radius);
                    zone.bottomLeft.y = (Int32)(yDestination - radius);
                    zone.topLeft.x = (Int32)(xDestination - radius);
                    zone.topLeft.y = ySource;
                    zone.topRight.x = (Int32)(xDestination + radius);
                    zone.topRight.y = ySource;
                    zone.bottomRight.x = (Int32)(xDestination + radius);
                    zone.bottomRight.y = (Int32)(yDestination - radius);
                }
            }
            else if (xSource > xDestination+radius)
            {
                if (ySource < yDestination-radius)
                {
                    zone.bottomLeft.x = (Int32)(xDestination - radius);
                    zone.bottomLeft.y = ySource;
                    zone.topLeft.x = (Int32)(xDestination - radius);
                    zone.topLeft.y = (Int32)(yDestination + radius);
                    zone.topRight.x = xSource;
                    zone.topRight.y = (Int32)(yDestination + radius);
                    zone.bottomRight.x = xSource;
                    zone.bottomRight.y = ySource;
                }
                else if (ySource >= yDestination-radius &&
                         ySource <= yDestination+radius)
                {
                    zone.bottomLeft.x = (Int32)(xDestination - radius);
                    zone.bottomLeft.y = (Int32)(yDestination - radius);
                    zone.topLeft.x = (Int32)(xDestination - radius);
                    zone.topLeft.y = (Int32)(yDestination + radius);
                    zone.topRight.x = xSource;
                    zone.topRight.y = (Int32)(yDestination + radius);
                    zone.bottomRight.x = xSource;
                    zone.bottomRight.y = (Int32)(yDestination - radius);
                }
                else if (ySource > yDestination+radius)
                {
                    zone.bottomLeft.x = (Int32)(xDestination - radius);
                    zone.bottomLeft.y = (Int32)(yDestination - radius);
                    zone.topLeft.x = (Int32)(xDestination - radius);
                    zone.topLeft.y = ySource;
                    zone.topRight.x = xSource;
                    zone.topRight.y = ySource;
                    zone.bottomRight.x = xSource;
                    zone.bottomRight.y = (Int32)(yDestination - radius);
                }
            }
        }
#ifdef DEBUG
        printf("        zone.bottomLeft = (%d, %d)\n",
            zone.bottomLeft.x, zone.bottomLeft.y);
        printf("        zone.topLeft = (%d, %d)\n",
            zone.topLeft.x, zone.topLeft.y);
        printf("        zone. topRight = (%d, %d)\n",
            zone.topRight.x, zone.topRight.y);
        printf("        zone.bottomRight = (%d, %d)\n",
            zone.bottomRight.x, zone.bottomRight.y);
#endif
        memmove(&(rreq->requestZone), &zone, sizeof(LAR1_Zone));
        return TRUE;
    }
}


//
// FUNCTION     Lar1BufferPacket()
// PURPOSE      Place packet into send buffer, awaiting valid path
//              and return TRUE if buffering is successful
// PARAMETERS   msg             - the data packet to be buffered
//              destAddr        - the destination of this data packet
//

static BOOL Lar1BufferPacket(Node *node, Message *msg,
                             NodeAddress destAddr)
{
    LAR1_SendBufferEntry *entry;
    int bufsize;
    int index;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

    bufsize = (lar1->sendBufTail + LAR1_SEND_BUFFER_SIZE) - lar1->sendBufHead;
    bufsize = bufsize % LAR1_SEND_BUFFER_SIZE;

    if (bufsize >= LAR1_SEND_BUFFER_SIZE)
        return FALSE;

    index = lar1->sendBufTail;

    if (lar1->sendBuf[index] == NULL)
    {
        lar1->sendBuf[index] =
            (LAR1_SendBufferEntry *) MEM_malloc(sizeof(LAR1_SendBufferEntry));
        assert (lar1->sendBuf[index]);
    }
    entry = lar1->sendBuf[index];

    entry->destAddr = destAddr;
    entry->msg = msg;
    entry->reTx = FALSE;
    entry->times = node->getNodeTime();

    lar1->sendBufTail = (lar1->sendBufTail + 1) % LAR1_SEND_BUFFER_SIZE;

    return TRUE;
}


//
// FUNCTION     Lar1PendingRouteReq()
// PURPOSE      Return TRUE if this node has sent a LAR Route Request Packet
//              for the given destination.
// PARAMETERS   destAddr        - the destination to check
//

static BOOL Lar1PendingRouteReq(Node *node,
                                NodeAddress destAddr)
{
    Lar1Data *lar1;
    LAR1_RequestSentEntry *entry;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    entry = lar1->reqSentHead;
    while (entry != NULL)
    {
        if (entry->destAddr == destAddr)
            return TRUE;
        else
            entry = entry->next;
    }

    return FALSE;
}


//
// FUNCTION     Lar1RouteExists()
// PURPOSE      Return TRUE if this node has a valid route to the destination
// PARAMETERS   destAddr        - the destination to check
//

static BOOL Lar1RouteExists(LAR1_RouteCacheEntry *cacheEntry,
                            NodeAddress destAddr)
{
    while (cacheEntry != NULL)
    {
        if ((cacheEntry->destAddr == destAddr) &&
            (cacheEntry->valid))
            return TRUE;
        cacheEntry = cacheEntry->next;
    }
    return FALSE;
}


//
// FUNCTION     Lar1RetrieveCacheEntry()
// PURPOSE      Return the Route Cache Entry for the given destination if
//              one exists.
// PARAMETERS   cacheEntry      - the head pointer of the route cache
//              destAddr        - the destination to check
//

static LAR1_RouteCacheEntry *Lar1RetrieveCacheEntry(
    LAR1_RouteCacheEntry *cacheEntry,
    NodeAddress destAddr)
{
    while (cacheEntry != NULL)
    {
        if ((cacheEntry->destAddr == destAddr) &&
            (cacheEntry->valid))
            return cacheEntry;
        cacheEntry = cacheEntry->next;
    }
    return NULL;
}


//
// FUNCTION     Lar1InsertRoute()
// PURPOSE      Extract route information from a LAR Route Reply Packet
//              and insert info into the Route Cache.
// PARAMETERS   pkt             - the LAR Route Reply packet
//              path            - the path given in the route reply
//              numEntries      - the number of entries in the path array
//

static void Lar1InsertRoute(Lar1Data *lar1,
                            LAR1_RouteReply *pkt,
                            NodeAddress *path,
                            int numEntries)
{
    LAR1_RouteCacheEntry *entry =
        (LAR1_RouteCacheEntry *) MEM_malloc(sizeof(LAR1_RouteCacheEntry));

    assert(entry);
    entry->destAddr = pkt->sourceAddr;
    entry->inUse = TRUE;
    entry->valid = TRUE;
    memmove(&(entry->destVelocity), &(pkt->destVelocity), sizeof(double));

    entry->path = (NodeAddress *)
                  MEM_malloc(sizeof(NodeAddress) * (numEntries - 1));
    assert(entry->path);
    memcpy(entry->path, &(path[1]), (sizeof(NodeAddress) * (numEntries - 1)));

    entry->pathLength = numEntries - 1;
    memmove(&(entry->locationTimestamp), &(pkt->locationTimestamp),
           sizeof(clocktype));
    memmove(&(entry->destLocation), &(pkt->destLocation),
            sizeof(LAR1_Location));
    entry->next = lar1->routeCacheHead;

    lar1->routeCacheHead = entry;

}


//
// FUNCTION     Lar1SetTimer()
// PURPOSE      Set a timer to expire just in case a route reply is not
//              received in the allotted time.
// PARAMETERS   eventType       - the event that is triggered by the timer
//              destAddr        - the destination node that the timer is
//                                interested in
//              delay           - the delay between now and timer expiration
//

static void Lar1SetTimer(Node *node, Int32 eventType, NodeAddress destAddr,
                         clocktype delay)
{
    Message *newMsg;
    NodeAddress *info;

    newMsg = MESSAGE_Alloc(node,
                            NETWORK_LAYER,
                            ROUTING_PROTOCOL_LAR1,
                            eventType);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(NodeAddress));
    info = (NodeAddress *) MESSAGE_ReturnInfo(newMsg);
    *info = destAddr;
    MESSAGE_Send(node, newMsg, delay);
}


//
// FUNCTION     Lar1FreeCacheEntry()
// PURPOSE      Free the memory used by a route rache entry
// PARAMETERS   cacheEntry      - the entry to free
//

static void Lar1FreeCacheEntry(LAR1_RouteCacheEntry *cacheEntry)
{
    MEM_free(cacheEntry->path);
    MEM_free(cacheEntry);
}


//
// FUNCTION     Lar1InvalidateRoutesThroughBrokenLink()
// PURPOSE      Mark as unusable routes in the cache which contain the
//              given node pair
// PARAMETERS   fromHop         - the first node in the node pair
//              toHop           - the receiving node of the node pair
//

static void Lar1InvalidateRoutesThroughBrokenLink(
    Node *node,
    NodeAddress fromHop,
    NodeAddress toHop)
{
    Lar1Data *lar1;
    LAR1_RouteCacheEntry *cacheEntry;
    NodeAddress *path;
    int pathCount;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

    cacheEntry = lar1->routeCacheHead;

    while (cacheEntry != NULL)
    {
        path = cacheEntry->path;
        if ((lar1->localAddress == fromHop) && (path[0] == toHop))
            cacheEntry->valid = FALSE;
        else
        {
            for (pathCount = 0; pathCount < cacheEntry->pathLength;
                 pathCount++)
            {
                if (path[pathCount] == fromHop)
                {
                    if (pathCount < (cacheEntry->pathLength - 1))
                        if (path[pathCount+1] == toHop)
                        {
                            cacheEntry->valid = FALSE;
                            pathCount = cacheEntry->pathLength;
                        }
                }
            }
        }
        cacheEntry = cacheEntry->next;
    }
}


//
// FUNCTION     Lar1DeleteRoute()
// PURPOSE      Remove route to the given destination from the route cache
// PARAMETERS   destAddr        - the given destination
//

static void Lar1DeleteRoute(Node *node, NodeAddress destAddr)
{
    Lar1Data *lar1;
    LAR1_RouteCacheEntry *cacheEntry,
                         *tempCacheEntry = NULL;
    BOOL firstEntry = TRUE;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

    cacheEntry = lar1->routeCacheHead;

#ifdef DEBUG
    printf("    Lar1DeleteRoute(%d)\n", destAddr);
#endif

    if (cacheEntry == NULL)
        return;

    while ((cacheEntry != NULL)  && (firstEntry == TRUE))
    {
        if (cacheEntry->destAddr == destAddr)
        {
            lar1->routeCacheHead = cacheEntry->next;
            Lar1FreeCacheEntry(cacheEntry);
            cacheEntry = lar1->routeCacheHead;
        }
        else
        {
            firstEntry = FALSE;
            tempCacheEntry = cacheEntry;
            if (tempCacheEntry != NULL)
                cacheEntry = cacheEntry->next;
            else
                cacheEntry = NULL;
        }
    }

    while (cacheEntry != NULL)
    {
        if (cacheEntry->destAddr == destAddr)
        {
            tempCacheEntry->next = cacheEntry->next;
            Lar1FreeCacheEntry(cacheEntry);
            cacheEntry = tempCacheEntry->next;
        }
        else
        {
            tempCacheEntry = cacheEntry;
            cacheEntry = cacheEntry->next;
        }
    }
}


//
// FUNCTION     Lar1TransmitErrorPacket()
// PURPOSE      Given a packet which MAC 802.11 was unable to transmit to
//              the neighbor node listed in the source route, this function
//              will transmit to the data packet's source node, a LAR
//              Route Error indicating that the route is broken at this link.
// PARAMETERS   oldMsg          - the data packet
//              nextHop         - the nextHop to which the data packet should
//                                have gone
//

static void Lar1TransmitErrorPacket(Node *node, const Message *oldMsg,
                                    NodeAddress nextHop)
{
    Message *newMsg;
    LAR1_RouteError *error;
    NodeAddress *n_addr;
    NodeAddress prevHop;
    char *pktptr;
    int numHops;
    int pktSize = sizeof(LAR1_RouteError);
    IpHeaderType *ipHeader = (IpHeaderType *)MESSAGE_ReturnPacket(oldMsg);
    IpOptionsHeaderType* ipOptions =
        (IpOptionsHeaderType*) (MESSAGE_ReturnPacket(oldMsg)
        + sizeof(IpHeaderType));
    int ptr = 4;
    char* routeRawAddressBytes = (char*)ipOptions + ptr - 1;
    NodeAddress nHop = ANY_DEST;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);

/* Determine number of hops back to source node */
    numHops = 0;
#ifdef DEBUG
    printf("        Determine path from source to here\n");
#endif
    while ((ptr < ipOptions->len) && (nHop != lar1->localAddress))
    {
        memcpy(&nHop, routeRawAddressBytes, sizeof(NodeAddress));
        ptr += 4;
        routeRawAddressBytes += 4;
        numHops++;
    }
    assert(nHop == lar1->localAddress);

    pktSize += (sizeof(NodeAddress) * (numHops+1));
    newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0,
                            MSG_MAC_FromNetwork);
    MESSAGE_PacketAlloc(node, newMsg, pktSize, TRACE_LAR1);

    pktptr = (char *) MESSAGE_ReturnPacket(newMsg);
    error = (LAR1_RouteError *) pktptr;
    /* Position the n_addr pointer onto the array of node addresses */
    pktptr += sizeof(LAR1_RouteError);
    n_addr = (NodeAddress *) pktptr;
    error->packetType = LAR1_ROUTE_ERROR;
    error->sourceAddr = ipHeader->ip_dst;
    error->destAddr = ipHeader->ip_src;
    error->fromHop = lar1->localAddress;
    error->nextHop = nextHop;
    error->segmentLeft = numHops-1;

    routeRawAddressBytes = (char*)ipOptions + 4 - 1;
    memcpy(&(n_addr[1]), routeRawAddressBytes,
           (sizeof(NodeAddress) * (numHops-1)));
    nHop = ipHeader->ip_src;
    memcpy(&(n_addr[0]), &nHop, sizeof(NodeAddress));

#ifdef DEBUG
    {
        int i;
        for (i=0; i<numHops; i++)
            printf("    --- hop %d = %d\n", i+1, n_addr[i]);
    }
#endif
    prevHop = n_addr[error->segmentLeft];

#ifdef DEBUG
    printf("    Send Route Error packet To Next Hop %d\n", prevHop);
#endif

    lar1->RouteErrorsSentAsErrorSource++;
    NetworkIpSendRawMessageToMacLayer(
        node,
        newMsg,
        NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE),
        prevHop,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_LAR1,
        LAR1_MAX_ROUTE_LENGTH,
        DEFAULT_INTERFACE,
        prevHop);

}


//
// FUNCTION     Lar1HandleBrokenLink()
// PURPOSE      Handle message from MAC 802.11 regarding broken link
// PARAMETERS   msg             - the packet returned by MAC 802.11
//

static void Lar1HandleBrokenLink(Node *node, const Message *msg)
{
    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    IpOptionsHeaderType* ipOptions =
        IpHeaderSourceRouteOptionField(ipHeader);
    NodeAddress nextHop = 0, destAddr;
    Lar1Data *lar1;

    lar1 = (Lar1Data *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_LAR1);


    /* Return if the IP packet is not a data packet */
    if (ipHeader->ip_p  == IPPROTO_LAR1)
        return;

    if (ipOptions && ipOptions->ptr < (ipOptions->len+4) ) {

       // Extract Next Address
       // The definition of "ptr" seems to number 1 as the
       // first byte of the the options field.

       char* routeRawAddressBytes = (char*)ipOptions + ipOptions->ptr - 1;

       routeRawAddressBytes -= 4;
       memcpy(&nextHop, routeRawAddressBytes, sizeof(NodeAddress));

    }

    ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    IpOptionsHeaderType *ipOption =
                      IpHeaderSourceRouteOptionField(ipHeader);
    if (ipOption)
    {
        memcpy((char *)&destAddr,
           (char *)ipOption + ipOption->len - sizeof(NodeAddress),
                       sizeof(NodeAddress));
    }

#ifdef DEBUG
    printf("    HandleBrokenLink (%d - %d)\n",node->nodeId, nextHop);
#endif

    /* If Node is the source of broken route */
    if (lar1->localAddress == ipHeader->ip_src)
    {
        Message *bufMsg = MESSAGE_Duplicate(node, msg);
#ifdef DEBUG
        printf("    Node is source of dropped packet.\n");
        printf("            Initiate route request and buffer pkt.\n");
#endif
        if (Lar1BufferPacket(node, bufMsg, destAddr))
        {
            /* Check if there is already a pending route request
               for this dest */
            if ((!Lar1PendingRouteReq(node, destAddr)) ||
                (Lar1RouteExists(lar1->routeCacheHead, destAddr)))
            {
#ifdef DEBUG
                printf("                Buffered.  Initiate Route Request.\n");
#endif
                Lar1InsertRequestSeen(node, lar1, lar1->localAddress, lar1->seqNum);
                Lar1InitiateRouteRequest(node, destAddr);
            }
#ifdef DEBUG
            else
            {
                printf("                Buffered.  Pending Route Request.\n");
            }
#endif
        }
        else
        {
#ifdef DEBUG
            printf("                Not enough space in buffer.  ",
                   "Dropped.\n");
#endif
            MESSAGE_Free(node, bufMsg);
        }

        /* Delete Routes that Use the Broken Link */
        Lar1InvalidateRoutesThroughBrokenLink(
            node, lar1->localAddress, nextHop);
    }
    else
    {
#ifdef DEBUG
        printf("    Node is intermediate.  Build error packet.\n");
#endif
        /* Build Error Packet */
        Lar1TransmitErrorPacket(node, msg, nextHop);
    }
}
