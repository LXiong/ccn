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
// This is a beta release of Fisheye. QA testing has not yet completed.
//

//
// Fisheye Routing Protocol
//        file: fisheye.c
//  objectives: simulate fisheye routing protocol
//   reference: 1) Fisheye State Routing Protocol (FSR) for Ad Hoc Networks
//                 IETF Internet Draft <draft-ietf-manet-fsr-03.txt>
//                 Mario Gerla, Xiaoyan Hong, Guangyu Pei, June. 2002
//              2) Scalable Routing Strategies for Ad-hoc
//                 Wireless Networks
//                 JSAC special issue on Ad-hoc Aug '99 by UCLA
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "app_util.h"
#include "routing_fisheye.h"


// ---------------------------------------------------------------------------
//    DEFINES
// ---------------------------------------------------------------------------

//
// Fisheye value for infinite distance
//

#define FISHEYE_INFINITY         127

//
// We use UDP and IP for the control packet
// The maximum allowable packet size is
// MAX_NW_PKT_SIZE - IP_HEADER - UDP_HEADER
//

#define MAX_TT_SIZE (MAX_NW_PKT_SIZE - 20 - 8 - sizeof(RoutingFisheyeHeader))

//
// Used to avoid synchronization of TT updates.
//

#define FISHEYE_TIMER_JITTER     (100 * MILLI_SECOND)


//
// The default value of fisheye scope
//

#define FISHEYE_SCOPE            2

//
// The default values of neighbor timeout, inter and intra update
//
#define FISHEYE_INTRA_UPDATE_INTERVAL        2  * SECOND
#define FISHEYE_INTER_UPDATE_INTERVAL        4 * SECOND
#define FISHEYE_NEIGHBOR_TIMEOUT_INTERVAL    6 * SECOND

// Pirnt out fisheye parameters
#define FISHEYE_DEBUG_PM   0
// Print out topology table when changed
#define FISHEYE_DEBUG_TT   0
// Print out neighbor list when changed
#define FISHEYE_DEBUG_NB   0
// Print out the update packet received
#define FISHEYE_DEBUG_PK   0


//---------------------------------------------------------------------------
//     STRUCTS, ENUMS, AND TYPEDEFS
//---------------------------------------------------------------------------

//
// One entry of the Neighbor List
//

typedef struct struct_fisheye_heard_neighbor_info
{
    NodeAddress nodeAddress;
    clocktype lastHeardTime;
    struct struct_fisheye_heard_neighbor_info* next;
} FisheyeHeardNeighborInfo;

typedef struct struct_fisheye_neighbor_info
{
    NodeAddress nodeAddress;
    struct struct_fisheye_neighbor_info* next;
} FisheyeNeighborInfo;

//
// One entry of the topology table
//
// Note: The topology table and the routing table
// mentioned in the IETF internet draft are merged
// into one table
//

typedef struct struct_fisheye_tt_row
{
    NodeAddress destAddress;
    NodeAddress nextHop;
    NodeAddress prevHop;

    short distance;

    clocktype lastModifyTime;
    BOOL alive;                   // after timeout, set alive to FALSE
    int sequenceNumber;

    short numNeighbors;
    FisheyeNeighborInfo* neighborInfo;
} FisheyeTTRow;

//
// Topology table
//

typedef struct struct_fisheye_tt
{
    // one row for each destination
    FisheyeTTRow *row;
    int numRows;
} FisheyeTT;

//
// Statistics information
//

typedef struct struct_fisheye_stats
{
    // Total number of in scope TT updates
    int intraScopeUpdate;

    //* Total number of out scope TT updates
    int interScopeUpdate;

    // Total number of fisheye control pkts got from transport Udp
    int pktNumFromUdp;

    // Total Control OH in bytes
    int controlOH;
} FisheyeStats;

//
// Control packet header
//

typedef struct struct_routing_fisheye_header
{
    NodeAddress sourceAddress;
    NodeAddress destAddress;

    // size of the payload in bytes
    unsigned short payloadSize;

    // The Reserved field according to the IETF draft
    // Should always be 0
    short reserved;
} RoutingFisheyeHeader;

//
// Configurable parameters
//
typedef struct struct_fisheye_parameter
{
    // Fisheye scope
    short scope;

    clocktype neighborTOInterval;
    clocktype intraUpdateInterval;
    clocktype interUpdateInterval;
} FisheyeParameter;

//
// Fisheye state.
//

typedef struct struct_fisheye_data
{
    // Random Seed
    RandomSeed jitterSeed;

    // Topology table
    FisheyeTT topologyTable;

    // Neighbor list
    FisheyeHeardNeighborInfo *heardNeighborInfo;

    // Statistics information
    FisheyeStats stats;

    // Configurable parameters
    FisheyeParameter parameter;
} FisheyeData;


//---------------------------------------------------------------------------
//   PROTOTYPES FOR FUNCTIONS WITH INTERNAL LINKAGE
//---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//   Initialization functions
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeInitTT(Node* node,FisheyeData* fisheye);

static void //inline//
RoutingFisheyeInitStats(Node* node, FisheyeData* fisheye);


// ---------------------------------------------------------------------------
//   Handler functions for fisheye internal messages
// ---------------------------------------------------------------------------
static void //inline//
RoutingFisheyeHandleIntraUpdateMsg(Node* node,
                                   FisheyeData* fisheye);

static void //inline//
RoutingFisheyeHandleInterUpdateMsg(Node* node,
                                   FisheyeData* fisheye);

static void //inline//
RoutingFisheyeHandleNeighborTOMsg(Node* node,
                                  FisheyeData* fisheye);


// ---------------------------------------------------------------------------
//   Handler functions for messages from the transport layer
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeUpdateNeighborInfo(Node* node,
                                 FisheyeData* fisheye,
                                 RoutingFisheyeHeader* header);

static void //inline//
RoutingFisheyeUpdateTT(Node* node,
                       FisheyeData* fisheye,
                       RoutingFisheyeHeader* header,
                       char* payload);


static void //inline//
RoutingFisheyeHandleControlPacket(Node *node,
                                  FisheyeData *fisheye,
                                  RoutingFisheyeHeader *header,
                                  char *payload);


// ---------------------------------------------------------------------------
//   Routing table manipulation functions
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeSendScopeTT(Node* node,
                          FisheyeData* fisheye,
                          short lower,
                          short upper);

static void //inline//
RoutingFisheyeCopyHeardToTable(Node* node,
                               FisheyeData* fisheye);

static int //inline//
RoutingFisheyeFindNodeInTT(FisheyeData* fisheye,
                           NodeAddress destAddress);

static int //inline//
RoutingFisheyeInsertNodeInTT(Node* node,
                             FisheyeData* fisheye,
                             NodeAddress destAddress);

static void //inline//
RoutingFisheyeCheckTTTimeout(Node* node,
                             FisheyeData* fisheye);

static void //inline//
RoutingFisheyeFindSP(Node* node, FisheyeData* fisheye);


// ---------------------------------------------------------------------------
//   Statistics
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintRoutingStats(Node* node);


// ---------------------------------------------------------------------------
//   Debug output functions
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintParameters(Node* node, FisheyeData* fisheye);

static void //inline//
RoutingFisheyePrintNeighborInfo(Node* node,
                                FisheyeData* fisheye);

static void //inline//
RoutingFisheyePrintTT(Node* node, FisheyeData* fisheye);

static void //inline//
RoutingFisheyePrintTTPkt(Node* node,
                         RoutingFisheyeHeader* header,
                         char* payload);


// ---------------------------------------------------------------------------
//   FUNCTIONS WITH EXTERNAL LINKAGE
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//   Init function
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FUNCTION   RoutingFisheyeInit
// PURPOSE    Initialize Fisheye variables, being called once for each node
//            at the beginning.
// PARAMETERS node - information associated with the node.
//
// RETURN     None.
// ---------------------------------------------------------------------------

void
RoutingFisheyeInit(Node* node, const NodeInput* nodeInput)
{
    FisheyeData* fisheye;
    clocktype randomDelay;
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;
    int scope;

    Message* newMsg;

    fisheye = (FisheyeData* ) MEM_malloc(sizeof(FisheyeData));
    //Initialize the buffer values to zero.
    memset(fisheye, 0,sizeof(FisheyeData));

    node->appData.routingVar = (void* )fisheye;

    RANDOM_SetSeed(fisheye->jitterSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_ROUTING_FISHEYE);

    // Init the configurable parameters by reading configuration file
    IO_ReadInt(node->nodeId,
               NetworkIpGetInterfaceAddress(node, 0),
               nodeInput,
               "FISHEYE-SCOPE",
               &wasFound,
               &scope);

    if (wasFound) {
        fisheye->parameter.scope = (short) scope;
    }
    else {
        fisheye->parameter.scope = FISHEYE_SCOPE;
    }

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, 0),
                  nodeInput,
                  "FISHEYE-INTRA-UPDATE-INTERVAL",
                  &wasFound,
                  buf);

    if (!wasFound) {
        fisheye->parameter.intraUpdateInterval =
            FISHEYE_INTRA_UPDATE_INTERVAL;
    }
    else {
        fisheye->parameter.intraUpdateInterval =
            TIME_ConvertToClock(buf);
    }

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, 0),
                  nodeInput,
                  "FISHEYE-INTER-UPDATE-INTERVAL",
                  &wasFound,
                  buf);

    if (!wasFound) {
        fisheye->parameter.interUpdateInterval =
            FISHEYE_INTER_UPDATE_INTERVAL;
    }
    else {
        fisheye->parameter.interUpdateInterval =
            TIME_ConvertToClock(buf);
    }

    IO_ReadString(node->nodeId,
                  NetworkIpGetInterfaceAddress(node, 0),
                  nodeInput,
                  "FISHEYE-NEIGHBOR-TIMEOUT-INTERVAL",
                  &wasFound,
                  buf);

    if (!wasFound) {
        fisheye->parameter.neighborTOInterval =
            FISHEYE_NEIGHBOR_TIMEOUT_INTERVAL;
    }
    else {
        fisheye->parameter.neighborTOInterval =
            TIME_ConvertToClock(buf);
    }

    if (FISHEYE_DEBUG_PM) {
        RoutingFisheyePrintParameters(node, fisheye);
    }

    // Init the Topology Table
    RoutingFisheyeInitTT(node,fisheye);

    // Init Neighbor List
    fisheye->heardNeighborInfo = NULL;

    // Init statistics information
    RoutingFisheyeInitStats(node,fisheye);


    // start TT exchange
    randomDelay = (clocktype) (RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeIntraUpdate);
    MESSAGE_Send(node,newMsg,randomDelay);

    randomDelay = (clocktype) (RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeInterUpdate);
    MESSAGE_Send(node,newMsg,randomDelay);

    randomDelay = (clocktype) (RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeNeighborTimeout);
    MESSAGE_Send(node,newMsg,randomDelay +
                 fisheye->parameter.neighborTOInterval);


    // registering RoutingFisheyeHandleAddressChangeEvent function
   NetworkIpAddAddressChangedHandlerFunction(node,
                    &RoutingFisheyeHandleChangeAddressEvent);
}


// ---------------------------------------------------------------------------
// FUNCTION   RoutingFisheyeLayer
// PURPOSE    Simulate Fisheye protocol, being called whenever a message is
//            for Fisheye routing layer.
// PARAMETERS
//            node - information associated with the node.
//            message - message header indentifying type of message.
//
// RETURN     None.
// --------------------------------------------------------------------------

void
RoutingFisheyeLayer(Node* node, Message* msg)
{
    FisheyeData* fisheye;
    char clockStr[MAX_STRING_LENGTH];
    char* payload;

    ctoa(node->getNodeTime(), clockStr);

    fisheye = (FisheyeData* ) node->appData.routingVar;
    if (fisheye == NULL)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    switch(msg->eventType) {
        case  MSG_APP_FromTransport:
        {
            RoutingFisheyeHeader* header;

            fisheye->stats.pktNumFromUdp ++ ;

            header = (RoutingFisheyeHeader* ) msg->packet;

            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(RoutingFisheyeHeader),
                                 TRACE_FISHEYE);

            payload = msg->packet;


RoutingFisheyeHandleControlPacket(node,fisheye,header,payload);
            MESSAGE_Free(node,msg);

            break;
        }
        case MSG_APP_FisheyeNeighborTimeout:
        {
            RoutingFisheyeHandleNeighborTOMsg(node,fisheye);
            MESSAGE_Free(node,msg);

            break;
        }
        case MSG_APP_FisheyeIntraUpdate:
        {
            fisheye->stats.intraScopeUpdate ++ ;
            RoutingFisheyeHandleIntraUpdateMsg(node,fisheye);
            MESSAGE_Free(node,msg);

            break;
        }
        case MSG_APP_FisheyeInterUpdate:
        {
            fisheye->stats.interScopeUpdate ++ ;
            RoutingFisheyeHandleInterUpdateMsg(node,fisheye);
            MESSAGE_Free(node,msg);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Fisheye: Unknown message.\n");
            break;
        }
    }
}


// ---------------------------------------------------------------------------
// FUNCTION   RoutingFisheyeFinalize
// PURPOSE    Print out statistics, being called at the end.
// PARAMETERS
//            node - information associated with the node.
//
// RETURN     None.
// ---------------------------------------------------------------------------

void
RoutingFisheyeFinalize(Node* node)
{
    if (node->appData.routingStats == TRUE)
    {
        RoutingFisheyePrintRoutingStats(node);
    }
}


// ---------------------------------------------------------------------------
// Initialize the Topology table (indexed by nodeId)
// Input parameter: node
// Assumption NONE
// RETURN NONE
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeInitTT(Node* node,FisheyeData* fisheye)
{
    int i;

    // zero out the routing table
    fisheye->topologyTable.row = NULL;
    fisheye->topologyTable.numRows = 0;
    NodeAddress myAddress;
    // allocate all necessary memory at the beginning
    // allocate one extra for the Dijkstra's SP
    fisheye->topologyTable.row =
       (FisheyeTTRow *) MEM_malloc((node->numNodes+1) * sizeof(FisheyeTTRow));
    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    // Add myself into topology table

    fisheye->topologyTable.row[0].destAddress = myAddress;
    fisheye->topologyTable.row[0].sequenceNumber = -1;
    fisheye->topologyTable.row[0].numNeighbors = 0;
    fisheye->topologyTable.row[0].neighborInfo = NULL;
    fisheye->topologyTable.row[0].lastModifyTime =
    node->getNodeTime();fisheye->topologyTable.row[0].alive = TRUE;
    fisheye->topologyTable.row[0].nextHop = myAddress;
    fisheye->topologyTable.row[0].prevHop = myAddress;
    fisheye->topologyTable.row[0].distance = 0;
    fisheye->topologyTable.numRows = 1;

    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_FISHEYE);
    for (i=0; i < fisheye->topologyTable.numRows; i++)
    {
        if (fisheye->topologyTable.row[i].nextHop > 0 )
        {
            NetworkUpdateForwardingTable(
                node,
                fisheye->topologyTable.row[i].destAddress,
                ANY_DEST,
                fisheye->topologyTable.row[i].nextHop,
                DEFAULT_INTERFACE,
                fisheye->topologyTable.row[i].distance,
                ROUTING_PROTOCOL_FISHEYE);
        }
        else
        {
            NetworkUpdateForwardingTable(
                node,
                fisheye->topologyTable.row[i].destAddress,
                ANY_DEST,
                (unsigned) NETWORK_UNREACHABLE,
                DEFAULT_INTERFACE,
                fisheye->topologyTable.row[i].distance,
                ROUTING_PROTOCOL_FISHEYE);
        }
    }
    if (FISHEYE_DEBUG_TT)
    {
        RoutingFisheyePrintTT(node, fisheye);
    }
}
//---------------------------------------------------------------------------
//Initialize the Stats variable
// RETURN: NONE
//---------------------------------------------------------------------------
static void //inline//
RoutingFisheyeInitStats(Node* node, FisheyeData* fisheye)
{
    // Total number of TT updates In the scope
    fisheye->stats.intraScopeUpdate = 0;

    // Total number of TT updates out the scope
    fisheye->stats.interScopeUpdate = 0;

    // Total Control OH in bytes
    fisheye->stats.controlOH = 0;
}

// ---------------------------------------------------------------------------
// Name     :   RoutingFisheyeSendScopeTT
// Purpose  :   Function to send update TT packets from lower to upper
//              distance
// Parameter:   node
// Return   :   none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeSendScopeTT(Node* node,
                          FisheyeData* fisheye,
                          short lower,
                          short upper)
{
    RoutingFisheyeHeader header;
    FisheyeNeighborInfo* tmp;
    FisheyeTTRow* row;
    int numRows;
    char payload[MAX_TT_SIZE];
    int broadcastTTsize;
    int maxEntrySize;
    int TTindex;
    int max;
    int i;

    clocktype delay = 0;

    RoutingFisheyeCheckTTTimeout(node, fisheye);

    row = fisheye->topologyTable.row;
    numRows = fisheye->topologyTable.numRows;

    // Find out the max number of a node in the scope entries
    max = -1;
    for (i = 0; i < numRows; i++)
    {
        if (row[i].distance >= lower &&
           row[i].distance < upper  &&
           row[i].alive)
            {
                if (row[i].numNeighbors > max)
                {
                    max = row[i].numNeighbors;
                }
            }
    }

    maxEntrySize = sizeof(NodeAddress)             // Dest. Addr
                   + sizeof(row[0].sequenceNumber) // seq.
                   + sizeof(row[0].numNeighbors)
                   + sizeof(NodeAddress)*max;      // neighbor list

    // packet the entries to nwDU, multiple nwDUs are sent
    // in the case of size exceeds one packet
    TTindex = 0;
    int maxTTSize = (GetNetworkIPFragUnit(node, (int)DEFAULT_INTERFACE)
                        - 20 - 8 - sizeof(RoutingFisheyeHeader));

    while (TTindex < numRows)
    {
        broadcastTTsize = 0;

        while (((broadcastTTsize + maxEntrySize) <= (signed) maxTTSize) &&
               (TTindex < numRows))
               {
                    if (row[TTindex].distance >= lower &&
                    row[TTindex].distance < upper  &&
                    row[TTindex].alive)
                    {

                        // Destination Address
                        memcpy(&(payload[broadcastTTsize]),
                            &(row[TTindex].destAddress),
                            sizeof(row[TTindex].destAddress));
                        broadcastTTsize += sizeof(row[TTindex].destAddress);

                        // Sequence number
                        memcpy(&(payload[broadcastTTsize]),
                            &(row[TTindex].sequenceNumber),
                            sizeof(row[TTindex].sequenceNumber));
                        broadcastTTsize += sizeof(row[TTindex].sequenceNumber);

                        // number of neighbors
                        memcpy(&(payload[broadcastTTsize]),
                            &(row[TTindex].numNeighbors),
                            sizeof(row[TTindex].numNeighbors));
                        broadcastTTsize += sizeof(row[TTindex].numNeighbors);

                        // the neighbor IDs
                        tmp = row[TTindex].neighborInfo;
                        while (tmp != NULL)
                        {
                            memcpy(&(payload[broadcastTTsize]),
                                &(tmp->nodeAddress),
                                sizeof(tmp->nodeAddress));
                            broadcastTTsize += sizeof(tmp->nodeAddress);
                            tmp = tmp->next;
                        }
                    }

                    TTindex ++;
                }

        if (broadcastTTsize > 0 )
        {

            // create header and send TT to UDP

            header.sourceAddress = NetworkIpGetInterfaceAddress(node,
                                        DEFAULT_INTERFACE);
            //header.sourceAddress = node->nodeId;
            header.destAddress = ANY_DEST;
            header.payloadSize = (unsigned short) broadcastTTsize;
            header.reserved = 0;

            // Send to neighbors
            Message *udpmsg = APP_UdpCreateMessage(
                node,
                ANY_IP,
                APP_ROUTING_FISHEYE,
                ANY_DEST,
                APP_ROUTING_FISHEYE,
                TRACE_FISHEYE,
                IPTOS_PREC_INTERNETCONTROL);

            APP_AddPayload(node, udpmsg, 
                payload, broadcastTTsize);

            APP_AddHeader(node, udpmsg,
                &header, sizeof(RoutingFisheyeHeader));

            APP_UdpSend(node, udpmsg, delay);

            fisheye->stats.controlOH += broadcastTTsize;
        }
    }
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeCopyHeardToTable
// Purpose   :  copy heard list to table list
// Papramers :  node
// Return    :  none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeCopyHeardToTable(Node* node,
                               FisheyeData* fisheye)
{
    FisheyeNeighborInfo* tableTmp;
    FisheyeNeighborInfo* tableNext;
    FisheyeHeardNeighborInfo* heardTmp;
    int myIndex;
    NodeAddress myAddress;

    // delete old list
    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);
    tableTmp = fisheye->topologyTable.row[myIndex].neighborInfo;
    while (tableTmp != NULL)
    {
        tableNext = tableTmp->next;
        MEM_free(tableTmp);
        tableTmp = tableNext;
    }

    // copy over Heard List to neighbor list
    heardTmp = fisheye->heardNeighborInfo;
    tableTmp = NULL;
    while (heardTmp != NULL)
    {
        tableNext = tableTmp;

        tableTmp =(FisheyeNeighborInfo*)
            MEM_malloc(sizeof(FisheyeNeighborInfo));
        tableTmp->nodeAddress = heardTmp->nodeAddress;
        tableTmp->next = tableNext;

        heardTmp = heardTmp->next;
    }

    fisheye->topologyTable.row[myIndex].neighborInfo = tableTmp;
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeFindNodeInTT
// Purpose   :  Find the position of the node in the topology table
// Parameters:  fisheye, destAddress
// Return    :  the index of the node in the table
// ---------------------------------------------------------------------------

static int //inline//
RoutingFisheyeFindNodeInTT(FisheyeData* fisheye,
                           NodeAddress destAddress)
{
    int top;
    int bottom;
    int middle;
    FisheyeTTRow *row;
    int index = -1;

    row = fisheye->topologyTable.row;

    if (fisheye->topologyTable.numRows > 0)
    {
        top = 0;
        bottom = fisheye->topologyTable.numRows - 1;
        middle = (bottom + top) / 2;

        if (destAddress == row[top].destAddress)
        {
            index = top;
        }

        if (destAddress == row[bottom].destAddress)
        {
            index = bottom;
        }

        while ((index < 0) && (middle != top))
        {
            if (destAddress == row[middle].destAddress)
            {
                index = middle;
            }
            else if (destAddress < row[middle].destAddress)
            {
                bottom = middle;
            }
            else
            {
                top = middle;
            }

            middle = (bottom + top) / 2;
        }
    }

    return index;
}


// ---------------------------------------------------------------------------
// Name:      RoutingFisheyeInsertNodeInTT
// Purpose:   Insert one node into the topology table
// Parameter: node, fisheye, destAddress
// Return:    the index of this node in the topology table
// ---------------------------------------------------------------------------

static int //inline//
RoutingFisheyeInsertNodeInTT(Node* node,
                             FisheyeData* fisheye,
                             NodeAddress destAddress)
{
    FisheyeTTRow *row;
    int index;

    row = fisheye->topologyTable.row;
    index = fisheye->topologyTable.numRows;

    while ((index > 0) && (row[index-1].destAddress > destAddress))
    {
        row[index] = row[index-1];
        index --;
    }

    // initialize this row
    row[index].destAddress = destAddress;
    row[index].sequenceNumber = -1;

    row[index].nextHop = (unsigned) (-1);
    row[index].prevHop = (unsigned) (-1);
    row[index].distance = FISHEYE_INFINITY;

    row[index].numNeighbors = 0;
    row[index].neighborInfo = NULL;

    row[index].lastModifyTime = node->getNodeTime();
    row[index].alive = TRUE;

    fisheye->topologyTable.numRows ++;

    ERROR_Assert(fisheye->topologyTable.numRows <= node->numNodes,
                 "Fisheye: topology table is larger than # nodes");

    if (FISHEYE_DEBUG_TT)
    {
        RoutingFisheyePrintTT(node, fisheye);
    }

    return index;
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeCheckTTTimeout
// Purpose   :  Check timeout of the entries of the topology table
// Parameter :  node, fisheye
// Return    :  none.
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeCheckTTTimeout(Node* node, FisheyeData* fisheye)
{
    FisheyeTTRow* row;
    int i;

    row = fisheye->topologyTable.row;

    for (i=0; i<fisheye->topologyTable.numRows; i++)
    {
        if ((node->getNodeTime() - row[i].lastModifyTime) >
            fisheye->parameter.neighborTOInterval)
        {
            row[i].alive = FALSE;
            row[i].sequenceNumber = -1;
        }
    }
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeFindSP
// Purpose   :  Calculate shortest path base
//              on the current link state table
//              using Dijkstra's algorithm
// Papramers :  node
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeFindSP(Node* node, FisheyeData* fisheye)
{
    int** connectionTable;
    FisheyeTTRow* row;
    NodeAddress myAddress;
    int myIndex;
    int nodesInTT;
    int priority;
    int min;
    int i;
    int j;
    int k;

    RoutingFisheyeCheckTTTimeout(node, fisheye);

    row = fisheye->topologyTable.row;
    nodesInTT = fisheye->topologyTable.numRows;

    connectionTable = (int** )MEM_malloc(sizeof(int* ) * nodesInTT);

    for (i=0; i<nodesInTT; i++)
    {
        connectionTable[i] =
            (int* ) MEM_malloc(sizeof(int) * nodesInTT);
    }

    for (i=0; i<nodesInTT; i++)
    {
        for (j=0; j<nodesInTT; j++)
        {
            connectionTable[i][j] = 0 ;
        }
    }

    for (i=0; i<nodesInTT; i++)
    {
        if (row[i].alive)
        {
            FisheyeNeighborInfo* tmp;
            tmp = row[i].neighborInfo;
            while (tmp != NULL)
            {
                k = RoutingFisheyeFindNodeInTT(fisheye, tmp->nodeAddress);
                if (k >= 0)
                {
                    connectionTable[i][k] = 1;
                    connectionTable[k][i] = 1;
                }

                tmp = tmp->next;
            }
        }
    }

    for (i=0; i<nodesInTT; i++)
    {
        row[i].distance = - FISHEYE_INFINITY;
        row[i].prevHop  = (unsigned) (-1);
        row[i].nextHop  = (unsigned) (-1);
    }

    row[nodesInTT].distance = -(FISHEYE_INFINITY + 1);
    //myAddress = It is the Node Interface Address
    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);
    row[myIndex].prevHop = myAddress;
    row[myIndex].nextHop = myAddress;
    row[myIndex].distance = 0;

    k = myIndex;
    min = nodesInTT;
    while (k != nodesInTT)
    {
        row[k].distance = (short) (-row[k].distance);

        for (i=0; i<nodesInTT; i++)
        {
            if (row[i].alive)
            {
                if (row[i].distance < 0)
                {
                    priority = row[k].distance + connectionTable[k][i];
                    if (connectionTable[k][i] &&
                        (row[i].distance < -priority))
                    {
                        row[i].distance = (short) (-priority);
                        row[i].prevHop = row[k].destAddress;

                        if (row[i].prevHop == myAddress)
                        {
                            row[i].nextHop = row[i].destAddress;
                        }
                        else
                        {
                            row[i].nextHop = row[k].nextHop;
                        }
                    }

                    if (row[i].distance > row[min].distance)
                    {
                        min = i;
                    }
                }
            }
        }

        k = min;
        min = nodesInTT;
    }

    if (FISHEYE_DEBUG_TT)
    {
        RoutingFisheyePrintTT(node, fisheye);
    }
    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_FISHEYE);

    for (k=0; k<nodesInTT; k++)
    {
        if ((row[k].nextHop < 0) ||
            (!row[k].alive))
        {
            row[k].nextHop = (unsigned) (-1);
            row[k].distance = FISHEYE_INFINITY;

            NetworkUpdateForwardingTable(
                node, row[k].destAddress,
                ANY_DEST,
                (unsigned) NETWORK_UNREACHABLE,
                DEFAULT_INTERFACE,
                row[k].distance,
                ROUTING_PROTOCOL_FISHEYE);
        }
        else
        {
            NetworkUpdateForwardingTable(
                node, row[k].destAddress,
                ANY_DEST,
                row[k].nextHop,
                DEFAULT_INTERFACE,
                row[k].distance,
                ROUTING_PROTOCOL_FISHEYE);
        }
    }

    // Free memory
    for (i = 0; i< nodesInTT; i++)
    {
        MEM_free(connectionTable[i]);
        connectionTable[i] = NULL;
    }

    MEM_free(connectionTable);
    connectionTable = NULL;
}


// ---------------------------------------------------------------------------
// Name     :   RoutingFisheyeHandleIntraUpdateMsg
// Purpose  :   Function to handle the intra update message
// Parameter:   node, fisheye
// Return   :   none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeHandleIntraUpdateMsg(Node* node, FisheyeData* fisheye)
{
    clocktype randomTime;
    Message* newMsg;
    int myIndex;
    NodeAddress myAddress;

    // increase the sequence number of its own neighbor list
    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);
    fisheye->topologyTable.row[myIndex].sequenceNumber ++;
    fisheye->topologyTable.row[myIndex].lastModifyTime = node->getNodeTime();

    RoutingFisheyeSendScopeTT(node, fisheye, 0,
                              fisheye->parameter.scope);
    // Invoke next intra update
    randomTime = (clocktype)(RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER
                             - FISHEYE_TIMER_JITTER/2);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeIntraUpdate);
    MESSAGE_Send(node,newMsg,
                 randomTime + fisheye->parameter.intraUpdateInterval);
}


// ---------------------------------------------------------------------------
// Name     :   RoutingFisheyeHandleInterUpdateMsg
// Purpose  :   Function to handle the inter update message
// Parameter:   node
// Return   :   none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeHandleInterUpdateMsg(Node* node, FisheyeData* fisheye)
{
    clocktype randomTime;
    Message* newMsg;

    RoutingFisheyeSendScopeTT(node,
                              fisheye,
                              fisheye->parameter.scope,
                              FISHEYE_INFINITY);

    // Invoke next inter update
    randomTime = (clocktype)(RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER
                    -  FISHEYE_TIMER_JITTER / 2);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeInterUpdate);
    MESSAGE_Send(node,newMsg, randomTime +
                 fisheye->parameter.interUpdateInterval);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeHandleNeighborTOMsg
// Purpose   :  Timeout stale neighbors
// Papramers :  node, routing
// Return    :  none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeHandleNeighborTOMsg(Node* node, FisheyeData* fisheye)
{
    FisheyeTTRow* row;
    FisheyeHeardNeighborInfo* trash;
    FisheyeHeardNeighborInfo* prev;
    FisheyeHeardNeighborInfo* current;
    int myIndex;
    clocktype randomTime;
    BOOL changed;
    Message* newMsg;
    NodeAddress myAddress;

    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);

    row = fisheye->topologyTable.row;
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);
    current = fisheye->heardNeighborInfo;
    prev = current;

    changed = FALSE;
    while (current != NULL)
    {
        trash = NULL;
        if ((node->getNodeTime() - current->lastHeardTime) >
            fisheye->parameter.neighborTOInterval)
        {
            changed = TRUE;

            // stale neighbor and should be removed
            if (current == fisheye->heardNeighborInfo)
            {
                fisheye->heardNeighborInfo = fisheye->heardNeighborInfo->next;
                prev = fisheye->heardNeighborInfo;
            }
            else
            {
                prev->next = current->next;
            }

            trash = current ;
            current = current->next ;
            MEM_free(trash) ;
            trash = NULL;

            row[myIndex].numNeighbors -- ;
        }
        else
        {
            prev = current ;
            current = current->next ;
        }
    }

    if (changed)
    {
        RoutingFisheyeCopyHeardToTable(node,fisheye);
        RoutingFisheyeFindSP(node, fisheye);

        if (FISHEYE_DEBUG_NB)
        {
            RoutingFisheyePrintNeighborInfo(node, fisheye);
        }
    }

    // schedule next TO check
    randomTime = (clocktype)(RANDOM_nrand(fisheye->jitterSeed) % FISHEYE_TIMER_JITTER
                 - FISHEYE_TIMER_JITTER / 2);
    newMsg = MESSAGE_Alloc(node,
                           APP_LAYER,
                           APP_ROUTING_FISHEYE,
                           MSG_APP_FisheyeNeighborTimeout);

    MESSAGE_Send(node,newMsg, randomTime +
                 fisheye->parameter.neighborTOInterval);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeUpdateNeighbor
// Purpose   :  Update Neighbor Information based on the
//              control packet received
// Papramers :  node and packet
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeUpdateNeighborInfo(Node* node,
                                 FisheyeData* fisheye,
                                 RoutingFisheyeHeader* header)
{
    FisheyeHeardNeighborInfo* tmp;
    int myIndex;

    NodeAddress myAddress;
    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);

    if (fisheye->heardNeighborInfo == NULL)
    {

        // I don't have any neighbour yet
        ERROR_Assert(fisheye->topologyTable.row[myIndex].numNeighbors == 0,
                     "Fisheye: wrong number of neighbors");

        tmp = (FisheyeHeardNeighborInfo* )
              MEM_malloc(sizeof(FisheyeHeardNeighborInfo));

        tmp->nodeAddress = header->sourceAddress;
        tmp->lastHeardTime = node->getNodeTime();
        tmp->next = fisheye->heardNeighborInfo;
        fisheye->heardNeighborInfo = tmp;

        fisheye->topologyTable.row[myIndex].numNeighbors ++;

        RoutingFisheyeCopyHeardToTable(node,fisheye);
        RoutingFisheyeFindSP(node, fisheye);
    }
    else
    {

        // Update neighbor info if header->sourceAddress in the list
        // otherwise insert it

        BOOL Find = FALSE;
        FisheyeHeardNeighborInfo* prev = FALSE;
        tmp =fisheye->heardNeighborInfo ;
        while (!Find && tmp != NULL)
        {
            if (tmp->nodeAddress == header->sourceAddress)
            {
                Find = TRUE;
                tmp->lastHeardTime = node->getNodeTime();
            }

            prev = tmp;
            tmp = tmp->next;
        }

        if (!Find)
        {
            tmp = (FisheyeHeardNeighborInfo* )
                MEM_malloc(sizeof(FisheyeHeardNeighborInfo));

            tmp->nodeAddress = header->sourceAddress;
            tmp->lastHeardTime = node->getNodeTime();

            tmp->next = prev->next;
            prev->next = tmp ;

            fisheye->topologyTable.row[myIndex].numNeighbors ++;

            RoutingFisheyeCopyHeardToTable(node,fisheye);
            RoutingFisheyeFindSP(node, fisheye);
        }

        if (FISHEYE_DEBUG_NB)
        {
            RoutingFisheyePrintNeighborInfo(node, fisheye);
        }
    }
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeUpdateTT
// Purpose   :  Update Topology Table based on the
//              TTupdate Message Received
// Papramers :  node and packet header and payload
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeUpdateTT(Node* node,
                       FisheyeData* fisheye,
                       RoutingFisheyeHeader* header,
                       char* payload)
{
    FisheyeTTRow* row;
    FisheyeNeighborInfo* tmp;
    FisheyeNeighborInfo* next;
    NodeAddress* linkList = NULL;
    NodeAddress destAddress;
    int TTsize;
    int TTindex;
    int sequenceNumber;
    short numNeighbors;
    int i;

    if (FISHEYE_DEBUG_PK)
    {
        RoutingFisheyePrintTTPkt(node, header, payload);
    }

    TTsize = 0;
    row = fisheye->topologyTable.row;

    while (TTsize < header->payloadSize)
    {

        // Entry index
        memcpy(&destAddress,&(payload[TTsize]),sizeof(destAddress));
        TTsize += sizeof(destAddress);

        // Sequence number
        memcpy(&sequenceNumber,&(payload[TTsize]), sizeof(sequenceNumber));
        TTsize += sizeof(sequenceNumber);

        // number of neighbors
        memcpy(&numNeighbors,&(payload[TTsize]), sizeof(numNeighbors));
        TTsize += sizeof(numNeighbors);

        // the neighbor IDs

        if (numNeighbors > 0)
        {
            linkList = (NodeAddress* )
                       MEM_malloc(numNeighbors*sizeof(NodeAddress));
            memcpy(linkList,
                   &(payload[TTsize]),
                   numNeighbors * sizeof(NodeAddress));
            TTsize += numNeighbors * sizeof(NodeAddress);
        }

        // update correspond TT database if the sequence number is bigger
        TTindex = RoutingFisheyeFindNodeInTT(fisheye, destAddress);
        if (TTindex < 0)
        {
            TTindex = RoutingFisheyeInsertNodeInTT(node, fisheye, destAddress);
        }

        if (sequenceNumber > row[TTindex].sequenceNumber)
        {
            row[TTindex].sequenceNumber = sequenceNumber;
            row[TTindex].numNeighbors = numNeighbors;

            // free the old memory
            tmp = row[TTindex].neighborInfo;
            while (tmp != NULL)
            {
                next = tmp->next;
                MEM_free(tmp);
                tmp = next ;
            }
            row[TTindex].neighborInfo = NULL;

            tmp = NULL;
            next = NULL;
            for (i=0; i< numNeighbors; i++)
            {
                next = tmp;
                tmp = (FisheyeNeighborInfo *)
                    MEM_malloc(sizeof(FisheyeNeighborInfo));
                tmp->nodeAddress = linkList[numNeighbors -1-i];
                tmp->next = next;
            }
            row[TTindex].neighborInfo = tmp;

            row[TTindex].lastModifyTime = node->getNodeTime();
            if (!row[TTindex].alive)
            {
                row[TTindex].alive = TRUE;
            }
         }
         if (linkList != NULL)
         {
             MEM_free(linkList);
             linkList = NULL;
         }
    }

    RoutingFisheyeFindSP(node, fisheye);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyeHandleControlPacket
// Purpose   :  handle the control packet received from UDP
// Papramers :  node, header, payload
// Return    :  none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyeHandleControlPacket(Node *node,
                                  FisheyeData *fisheye,
                                  RoutingFisheyeHeader *header,
                                  char *payload)
{
    RoutingFisheyeUpdateNeighborInfo(node, fisheye, header);
    RoutingFisheyeUpdateTT(node, fisheye, header, payload);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyePrintParameters
// Purpose   :  Print out fisheye parameters
// Parameters:  node, fisheye
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintParameters(Node* node, FisheyeData* fisheye)
{
    char buf[MAX_STRING_LENGTH];

    printf("Fisheye: protocol parameters\n");

    printf("FISHEYE-SCOPE: %d\n", fisheye->parameter.scope);

    ctoa(fisheye->parameter.intraUpdateInterval, buf);
    printf("FISHEYE-INTRA-UPDATE-INTERVAL: %s\n", buf);

    ctoa(fisheye->parameter.interUpdateInterval, buf);
    printf("FISHEYE-INTER-UPDATE-INTERVAL: %s\n", buf);

    ctoa(fisheye->parameter.neighborTOInterval, buf);
    printf("FISHEYE-NEIGHBOR-TIMEOUT-INTERVAL: %s\n", buf);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyePrintRoutingStats
// Purpose   :  Print Network Protocol Stats
// Papramers :  node
// Return    :  none
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintRoutingStats(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    FisheyeData* fisheye;

    fisheye = (FisheyeData* ) node->appData.routingVar;

    sprintf(buf, "Number of intra scope updates sent = %d",
            fisheye->stats.intraScopeUpdate);
    IO_PrintStat(node,
                 "Application",
                 "Fisheye",
                 ANY_DEST,
                 -1,             // instance Id
                 buf);

    sprintf(buf, "Number of inter scope updates sent = %d",
            fisheye->stats.interScopeUpdate);
    IO_PrintStat(node,
                 "Application",
                 "Fisheye",
                 ANY_DEST,
                 -1,             // instance Id
                 buf);
    sprintf(buf, "Number of control packets received = %d",
            fisheye->stats.pktNumFromUdp);
    IO_PrintStat(node,
                 "Application",
                 "Fisheye",
                 ANY_DEST,
                 -1,             // instance Id
                 buf);

    sprintf(buf, "Control Overhead in Bytes = %d",
            fisheye->stats.controlOH);
    IO_PrintStat(node,
                 "Application",
                 "Fisheye",
                 ANY_DEST,
                 -1,            // instance Id
                 buf);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyePrintNeighborInfo
// Purpose   :  Print Neighbor Information of the node
// Papramers :  node
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintNeighborInfo(Node* node,
                                FisheyeData* fisheye)
{
    char clockStr[MAX_STRING_LENGTH];
    FisheyeHeardNeighborInfo* tmp;
    int myIndex;
    NodeAddress myAddress;

    ctoa(node->getNodeTime(), clockStr);

    myAddress = NetworkIpGetInterfaceAddress(node,DEFAULT_INTERFACE);
    myIndex = RoutingFisheyeFindNodeInTT(fisheye, myAddress);

    //myIndex = RoutingFisheyeFindNodeInTT(fisheye, node->nodeId);
    printf("Fisheye: %s node%d's NeighorInfo\n", clockStr, node->nodeId);
    printf("Total number of neighbor %d\n",
           fisheye->topologyTable.row[myIndex].numNeighbors);

    tmp = fisheye->heardNeighborInfo ;
    while (tmp != NULL)
    {
        ctoa(tmp->lastHeardTime, clockStr);
        printf("Neighbor ID %d, LastHeard Time %s\n",
               tmp->nodeAddress, clockStr);
        tmp = tmp->next;
    }
    printf("----------------------------------------\n");
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyePrintTT
// Purpose   :  Print the topology table of a node
// Papramers :  node
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintTT(Node* node, FisheyeData* fisheye)
{
    char clockStr[MAX_STRING_LENGTH];
    FisheyeNeighborInfo* tmp;
    FisheyeTTRow* row;
    int i;

    ctoa(node->getNodeTime(), clockStr);
    row = fisheye->topologyTable.row;

    printf("Fisheye: %s, topoplogy table of node%d:\n",
           clockStr, node->nodeId);
    printf("----------------------------------------------\n");

    for (i=0; i< fisheye->topologyTable.numRows; i++)
    {
        if (row[i].alive)
        {
            ctoa(row[i].lastModifyTime, clockStr);
            printf("%d %d %d %hd %s %d %hd:(",
                   row[i].destAddress, row[i].nextHop,
                   row[i].prevHop, row[i].distance,
                   clockStr, row[i].sequenceNumber,
                   row[i].numNeighbors);

            tmp = row[i].neighborInfo;
            while (tmp != NULL)
            {
                printf("%d ", tmp->nodeAddress);
                tmp = tmp->next;
            }

            printf(")\n");
        }
    }

    printf("----------------------------------------------\n");
    fflush(stdout);
}


// ---------------------------------------------------------------------------
// Name      :  RoutingFisheyePrintTTPkt
// Purpose   :  Print the topology info from a TT pkt
// Papramers :  pkt
// Return    :  None
// ---------------------------------------------------------------------------

static void //inline//
RoutingFisheyePrintTTPkt(Node* node,
                         RoutingFisheyeHeader* header,
                         char* payload)
{
    int TTsize;
    NodeAddress destAddress;
    int sequenceNumber;
    short numNeighbors;
    NodeAddress neighbor;
    int i;

    printf("Fisheye: Node%d Receive from Node%d, payload size=%d\n",
           node->nodeId,
           header->sourceAddress,
           header->payloadSize);

    TTsize = 0;
    while (TTsize < header->payloadSize)
    {

        // Entry index
        memcpy(&destAddress, &(payload[TTsize]), sizeof(destAddress));
        TTsize += sizeof(destAddress);
        printf("destAddress: %d, ", destAddress);

        // Sequence number
        memcpy(&sequenceNumber, &(payload[TTsize]),
               sizeof(sequenceNumber));
        TTsize += sizeof(sequenceNumber);
        printf("sequenceNumber: %d, ",sequenceNumber);

        // number of neighbors
        memcpy(&numNeighbors, &(payload[TTsize]),
               sizeof(numNeighbors));
        TTsize += sizeof(numNeighbors);
        printf("numNeighbors: %d\n", numNeighbors);

        // the neighbor IDs
        printf("Neighbor ID List:\n");
        for (i = 0; i < numNeighbors; i++)
        {
            memcpy(&(neighbor), &(payload[TTsize]),
                   sizeof(NodeAddress));
            TTsize += sizeof(NodeAddress);
            printf("%d ",neighbor);

            if (((i+1)%5) == 0)
            {
                printf("\n");
            }
        }
        printf("\n");
    }

    printf("\n");
    fflush(stdout);
}

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingFisheyeHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node*   : Pointer to Node structure
// + interfaceIndex          : Int32   : interface index
// + old_addr              : Address*: old address
// + NetworkType networkType : type of network protocol
// RETURN :: void : NULL
//--------------------------------------------------------------------------
void RoutingFisheyeHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* old_addr,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    NetworkDataIp* ip = node->networkData.networkVar;
    IpInterfaceInfoType* interfaceInfo =
        ip->interfaceInfo[interfaceIndex];
    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
        != ROUTING_PROTOCOL_FISHEYE)
    {
        return;
    }
    if (networkType == NETWORK_IPV6)
    {
        return;
    }
    FisheyeData* fisheye;
    fisheye = (FisheyeData* )node->appData.routingVar;
    for (int i = 0; i < fisheye->topologyTable.numRows; i++)
    {
        if (fisheye->topologyTable.row[i].destAddress ==
            old_addr->interfaceAddr.ipv4)
        {
            fisheye->topologyTable.row[i].destAddress =
                NetworkIpGetInterfaceAddress(node, 0);
            fisheye->topologyTable.row[0].lastModifyTime = node->getNodeTime();
            break;
        }
    }
    RoutingFisheyeHandleIntraUpdateMsg(node, fisheye);
    RoutingFisheyeHandleInterUpdateMsg(node, fisheye);
}
