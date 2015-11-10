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
 * Name: dvmrp.h
 * Purpose: To simulate Distance Vector Multicast Routing Protocol (DVMRP)
 *
 */


#ifndef _DVMRP_H_
#define _DVMRP_H_

#include "network_ip.h"
#include "main.h"
#include "list.h"

#define ALL_DVMRP_ROUTER    0xE0000004

/* Jitter transmission to avoid synchronization */
#define ROUTING_DVMRP_BROADCAST_JITTER    (20 * MILLI_SECOND)

/*
 * How dynamic the routing environment is effects the following rates.
 * A lower rate will allow quicker adaptation to a change in the
 * environment, at the cost of wasting network bandwidth.
 */

/* How often probe message containg list of discovered neighbors are sent */
#define DVMRP_PROBE_MSG_INTERVAL  (10 * SECOND)

/* How often routing messages containing complete routing tables are sent. */
#define DVMRP_FULL_UPDATE_RATE (60 * SECOND)

/* How often triggered routing messages may be sent out. */
#define DVMRP_TRIGGERED_UPDATE_RATE  (5 * SECOND)


/*
 * Increasing the following timeouts will increase the stability of the
 * routing algorithm, at the cost of slower reactions to changes in the
 * routing environment.
 */

/*
 * How long a neighbor is considered up without confirmation.
 *     This is important for timing out routes, and for setting
 *     the children and leaf flags.
 */
//#define DVMRP_NEIGHBOR_TIMEOUT (4 * DVMRP_FULL_UPDATE_RATE)
// Set this value according to draft
#define DVMRP_NEIGHBOR_TIMEOUT (35 * SECOND)

/*
 * How long a route is considered valid without confirmation.
 *     When this timeout expires, packets will no longer be
 *     forwarded on the route, and routing updates will consider
 *     this route to have a metric of infinity.
 */
#define DVMRP_EXPIRATION_TIMEOUT (2 * DVMRP_FULL_UPDATE_RATE)

/*
 * How long a route exists without confirmation.  When this
 *     timeout expires, routing updates will no longer contain any
 *     information on this route, and the route will be deleted.
 */
#define DVMRP_GARBAGE_TIMEOUT (4 * DVMRP_FULL_UPDATE_RATE)

#define ROUTING_DVMRP_INFINITY    32


#define DVMRP_INITIAL_TABLE_SIZE    10

#define DVMRP_MAX_PACKET_LENGTH     512

#define DVMRP_FIXED_LENGTH_HEADER_SIZE \
        sizeof(RoutingDvmrpCommonHeaderType)

#define DVMRP_DEFAULT_PRUNE_LIFETIME    2 * HOUR

#define DVMRP_GRAFT_RTMXT_INTERVAL      5 * SECOND

typedef enum
{
    ROUTING_DVMRP_PROBE = 1,
    ROUTING_DVMRP_REPORT = 2,
    ROUTING_DVMRP_PRUNE = 7,
    ROUTING_DVMRP_GRAFT = 8,
    ROUTING_DVMRP_GRAFT_ACK = 9
} RoutingDvmrpPacketCode;


typedef enum
{
    ROUTING_DVMRP_PHYSICAL,
    ROUTING_DVMRP_TUNNEL
} RoutingDvmrpInterfaceType;


typedef enum
{
    ROUTING_DVMRP_ONE_WAY_NEIGHBOR,
    ROUTING_DVMRP_TWO_WAY_NEIGHBOR
} NeighborRelationship;


typedef enum
{
    ROUTING_DVMRP_VALID,
    ROUTING_DVMRP_HOLD_DOWN,
    ROUTING_DVMRP_EXPIRE
} RoutingDvmrpRouteState;


typedef enum
{
    ROUTING_DVMRP_INSERT_TO_LIST,
    ROUTING_DVMRP_REMOVE_FROM_LIST
} RoutingDvmrpListActionType;

/* Information about various timer */
typedef struct routing_dvmrp_timer_info_str
{
    NodeAddress     neighborAddr;
    int             interfaceIndex;
} RoutingDvmrpTimerInfo;


/* Timer information related prune */
typedef struct routing_dvmrp_prune_timer_info_str
{
    NodeAddress srcAddr;
    NodeAddress grpAddr;
    NodeAddress nbrAddr;
} RoutingDvmrpPruneTimerInfo;


/* Timer information related graft */
typedef struct routing_dvmrp_graft_timer_info_str
{
    int count;
    NodeAddress srcAddr;
    NodeAddress grpAddr;
} RoutingDvmrpGraftTimerInfo;


/* DVMRP packet header */
typedef struct routing_dvmrp_common_header_str
{
    unsigned char           type;
    unsigned char           pktCode;
    unsigned short          checksum;
    unsigned short          reserved;
    unsigned char           minorVersion;
    unsigned char           majorVersion;
} RoutingDvmrpCommonHeaderType;


/* DVMRP Probe packet */
typedef struct routing_dvmrp_probe_packet_str
{
    RoutingDvmrpCommonHeaderType   commonHeader;
    unsigned int                   generationId;

    /* Neighbor will be allocated dynamically */
} RoutingDvmrpProbePacketType;


/* DVMRP Route report packet */
typedef struct routing_dvmrp_route_report_packet_str
{
    RoutingDvmrpCommonHeaderType    commonHeader;

    /* This part will be allocated dynamically */
} RoutingDvmrpRouteReportPacketType;


/* DVMRP Prune packet */
typedef struct routing_dvmrp_prune_packet_str
{
    RoutingDvmrpCommonHeaderType    commonHeader;
    NodeAddress                     sourceAddr;
    NodeAddress                     groupAddr;
    unsigned int                    pruneLifetime;
} RoutingDvmrpPrunePacketType;


/* DVMRP Graft packet */
typedef struct routing_dvmrp_graft_packet_str
{
    RoutingDvmrpCommonHeaderType    commonHeader;
    NodeAddress                     sourceAddr;
    NodeAddress                     groupAddr;
} RoutingDvmrpGraftPacketType;


/* DVMRP Graft acknowledgement packet */
typedef struct routing_dvmrp_graft_ack_packet_str
{
    RoutingDvmrpCommonHeaderType    commonHeader;
    NodeAddress                     sourceAddr;
    NodeAddress                     groupAddr;
} RoutingDvmrpGraftAckPacketType;


/* Item associated with each child interface */
typedef struct routing_dvmrp_child_list_item_str
{
    int                 interfaceIndex;
    NodeAddress         ipAddr;
    NodeAddress         designatedForwarder;
    unsigned char       advertiseMetric;
    BOOL                isLeaf;
    LinkedList*         dependentNeighbor;

    /* Not used in simulation */
    unsigned char       ttl;
} RoutingDvmrpChildListItem;


/* Routing table row */
typedef struct routing_dvmrp_routing_table_row_str
{
    NodeAddress               destAddr;
    NodeAddress               subnetMask;
    NodeAddress               nextHop;
    int                       interfaceId;

    /* Route may be in valid, expire or hold-down state */
    RoutingDvmrpRouteState    state;
    clocktype                 lastModified;
    LinkedList*               child;
    unsigned char             metric;
    clocktype                 routeExpirationTimer;
    BOOL                      triggeredUpdateScheduledFlag;
} RoutingDvmrpRoutingTableRow;


/* Routing table */
typedef struct routing_dvmrp_routing_table_str
{
    unsigned int                    numEntries;
    unsigned int                    numRowsAllocated;
    clocktype                       nextBroadcastTime;
    RoutingDvmrpRoutingTableRow*    rowPtr;
} RoutingDvmrpRoutingTable;


/* Item associated with each dependent neighbor */
typedef struct routing_dvmrp_neighbor_list_item
{
    NodeAddress         addr;
    BOOL                pruneActive;
    clocktype           lastPruneReceived;
    clocktype           pruneLifetime;
} RoutingDvmrpDependentNeighborListItem;


/* Item associated with each downstream */
typedef struct routing_dvmrp_downstream_list_item_str
{
    int              interfaceIndex;
    unsigned char    ttl;
    BOOL             isPruned;
    clocktype        whenPruned;    //Don't need
    LinkedList*      dependentNeighbor;
} RoutingDvmrpDownstreamListItem;


/* Source list item */
typedef struct routing_dvmrp_source_list_str
{
    NodeAddress         srcAddr;
    int                 upstream;
    NodeAddress         prevHop;
    BOOL                graftRtmxtTimer;
    BOOL                pruneActive;
    clocktype           lastDownstreamPruned;
    clocktype           pruneLifetime;
    LinkedList*         downstream;
} RoutingDvmrpSourceList;

/* Forwarding table row */
typedef struct routing_dvmrp_forwarding_table_row_str
{
    NodeAddress           groupAddr;
    LinkedList*           srcList;
} RoutingDvmrpForwardingTableRow;


/* Forwarding table */
typedef struct routing_dvmrp_forwarding_table_str
{
    unsigned int                       numEntries;
    unsigned int                       numRowsAllocated;
    RoutingDvmrpForwardingTableRow*    rowPtr;
} RoutingDvmrpForwardingTable;


/* Neighbor list item */
typedef struct routing_dvmrp_neighbor_list_item_str
{
    NodeAddress                 ipAddr;
    unsigned int                lastGenIdReceive;
    clocktype                   lastProbeReceive;
    NeighborRelationship        relation;
} RoutingDvmrpNeighborListItem;


/* Interface specific data structure */
typedef struct routing_dvmrp_interface_str
{
    /* Currently all interfaces will be considered as physical */
    RoutingDvmrpInterfaceType    interfaceType;
    NodeAddress                  ipAddress;
    NodeAddress                  subnetMask;
    clocktype                    probeInterval;
    clocktype                    neighborTimeoutInterval;
    unsigned int                 probeGenerationId;
    int                          neighborCount;
    LinkedList*                  neighborList;
} RoutingDvmrpInterface;


/* Statistic structure of DVMRP */
typedef struct routing_dvmrp_stats_str
{
    int                 numNeighbor;
    int                 totalPacketSent;
    int                 probePacketSent;
    int                 routingUpdateSent;
    int                 pruneSent;
    int                 graftSent;
    int                 graftAckSent;
    int                 totalPacketReceive;
    int                 probePacketReceive;
    int                 routingUpdateReceive;
    int                 pruneReceive;
    int                 graftReceive;
    int                 graftAckReceive;
    int                 numTriggerUpdate;
    int                 numRTUpdate;
    int                 numFTUpdate;
    int                 numOfDataPacketOriginated;
    int                 numOfDataPacketReceived;
    int                 numOfDataPacketForward;
    int                 numOfDataPacketDiscard;

    BOOL                alreadyPrinted;
} RoutingDvmrpStats;


/* This is the main structure of DVMRP */
typedef struct struct_routing_dvmrp_str
{
    RoutingDvmrpRoutingTable       routingTable;
    RoutingDvmrpForwardingTable    forwardingTable;
    RoutingDvmrpInterface          *interface;
    RoutingDvmrpStats              stats;
    BOOL                           showStat;
    RandomSeed                     seed;  // used to generate jitter
} DvmrpData;



/* Function declaration */

void RoutingDvmrpHandleProtocolEvent(Node *node, Message *msg);


void RoutingDvmrpHandleProtocolPacket(Node *node, Message *msg,
        NodeAddress srcAddr, NodeAddress destAddr, int intfId);

void RoutingDvmrpRouterFunction(Node *node,
                                Message *msg,
                                NodeAddress destAddr,
                                int interfaceIndex,
                                BOOL *packetWasRouted,
                                NodeAddress prevHop);

/*
 * FUNCTION     :RoutingDvmrpInit()
 * PURPOSE      :Initialization function for DVMRP multicast protocol
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 * Parameters:
 *     node:       Node being initialized
 *     dvmrpPtr:   Reference to DVMRP data structure.
 *     nodeInput:  Reference to input file.
 */
void RoutingDvmrpInit(Node *node, const NodeInput *nodeInput,
                      int interfaceIndex);

void RoutingDvmrpFinalize(Node *node);


/*
 * FUNCTION     :RoutingDvmrpLocalMembersJoinOrLeave()
 * PURPOSE      :Handle local group membership events
 * ASSUMPTION   :None
 * RETURN VALUE :NULL
 */
void RoutingDvmrpLocalMembersJoinOrLeave(Node *node,
                                         NodeAddress groupAddr,
                                         int interfaceId,
                                         LocalGroupMembershipEventType event);

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingDvmrpHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address for DVMRP
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldAddress         : Address* : current address
// + subnetMask         : NodeAddress : subnetMask
// + networkType        : NetworkType : type of network protocol
// RETURN :: NONE
//---------------------------------------------------------------------------
void RoutingDvmrpHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);
#endif /* _DVMRP_H_ */

