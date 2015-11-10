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

#ifndef MAC_DOT11S_HWMP_H
#define MAC_DOT11S_HWMP_H


// ------------------------------------------------------------------
/*
HWMP, Hybrid Wireless Mesh Protocol, is the default path protocol
for 802.11s based mesh networks. Others, mentioned in the draft,
are OLSR-RA and NULL protocol.

HWMP combines on-demand and pro-active routing. The on-demand mechanism
is based on AODV (RFC 3561). Modification relate to proxy of BSS
stations (for MPs that are APs) and AirTime Metrics. The on-demand code
closely follows the QualNet AODV code (see routing_aodv.(h/cpp)).

Pro-active routing applies when an MP is configured as a Root. Root
announcements propagate through the mesh and result in formation
of a minimal-cost spanning tree. MPs may use this tree to forward
packets while on-demand routes are being discovered. Tree
based routing (TBR) complements on-demand routing.

The frame formats and procedures, both for on-demand and TBR, have
undergone change across revisions. Further specification change is
likely particularly related to BSS station proxy under reassociation
and proactive routing.

Note: Most of the section references in the code comments refer to
the AODV draft.

*/
// ------------------------------------------------------------------

typedef struct struct_hwmp HWMP_Data;


/**
DEFINE      :: HWMP_NET_DIAMETER
DESCRIPTION :: Value of Net diameter. Same as mesh diameter, to be
                consistent across path protocols.
**/
#define HWMP_NET_DIAMETER                       (dot11->mp->netDiameter)

/**
DEFINE      :: HWMP_NODE_TRAVERSAL_TIME
DESCRIPTION :: Value of node traversal time. Configured at the
                MP level to be consistent across path protocols.
**/
#define HWMP_NODE_TRAVERSAL_TIME                (dot11->mp->nodeTraversalTime)

/**
DEFINE      :: HWMP_NET_TRAVERSAL_TIME
DESCRIPTION :: Derived value of Net traversal time.
**/
#define HWMP_NET_TRAVERSAL_TIME             \
    (2 * HWMP_NET_DIAMETER * HWMP_NODE_TRAVERSAL_TIME)

/**
DEFINE      :: HWMP_ACTIVE_ROUTE_TIMEOUT_DEFAULT
DESCRIPTION :: Default timeout for an active route.
                The default for AODV is 3 seconds.
**/
#define HWMP_ACTIVE_ROUTE_TIMEOUT_DEFAULT       (5000 * MILLI_SECOND)

/**
DEFINE      :: HWMP_ACTIVE_ROUTE_TIMEOUT
DESCRIPTION :: Configured timeout for an active route.
**/
#define HWMP_ACTIVE_ROUTE_TIMEOUT               (hwmp->activeRouteTimeout)

/**
DEFINE      :: HWMP_MY_ROUTE_TIMEOUT
DESCRIPTION :: Configured timeout for reply by destination
**/
#define HWMP_MY_ROUTE_TIMEOUT                   (hwmp->myRouteTimeout)

/**
DEFINE      :: HWMP_ROUTE_DELETION_CONSTANT_DEFAULT
DESCRIPTION :: Default value of route deletion constant.
**/
#define HWMP_ROUTE_DELETION_CONSTANT_DEFAULT    (5)

/**
DEFINE      :: HWMP_ROUTE_DELETION_CONSTANT
DESCRIPTION :: Configured value of route deletion constant.
**/
#define HWMP_ROUTE_DELETION_CONSTANT            (hwmp->routeDeletionConstant)

/**
DEFINE      :: HWMP_DELETION_PERIOD
DESCRIPTION :: Period for route deletion.
**/
#define HWMP_DELETION_PERIOD                    \
        (HWMP_ROUTE_DELETION_CONSTANT *         \
        HWMP_ACTIVE_ROUTE_TIMEOUT)

/**
DEFINE      :: HWMP_REVERSE_ROUTE_TIMEOUT
DESCRIPTION :: Lifetime for a route under discovery.
**/
#define HWMP_REVERSE_ROUTE_TIMEOUT              (hwmp->reverseRouteTimeout)

/**
DEFINE      :: HWMP_RREQ_INITIAL_TTL_DEFAULT
DESCRIPTION :: Default start value of TTL in expanding ring search.
**/
#define HWMP_RREQ_INITIAL_TTL_DEFAULT       (2)

/**
DEFINE      :: HWMP_RREQ_INITIAL_TTL
DESCRIPTION :: Configured start of TTL in expanding ring search.
**/
#define HWMP_RREQ_INITIAL_TTL               (hwmp->rreqTtlInitial)

/**
DEFINE      :: HWMP_RREQ_TTL_INCREMENT_DEFAULT
DESCRIPTION :: Default for TTL increment in expanding ring search.
**/
#define HWMP_RREQ_TTL_INCREMENT_DEFAULT     (2)

/**
DEFINE      :: HWMP_RREQ_TTL_INCREMENT
DESCRIPTION :: Configured TTL increment in expanding ring search.
**/
#define HWMP_RREQ_TTL_INCREMENT             (hwmp->rreqTtlIncrement)

/**
DEFINE      :: HWMP_RREQ_TTL_THRESHOLD_DEFAULT
DESCRIPTION :: Default for TTL threshold in expanding ring search.
**/
#define HWMP_RREQ_TTL_THRESHOLD_DEFAULT     (5)

/**
DEFINE      :: HWMP_RREQ_TTL_THRESHOLD
DESCRIPTION :: Configured TTL threshold in expanding ring search.
**/
#define HWMP_RREQ_TTL_THRESHOLD             (hwmp->rreqTtlThreshold)

/**
DEFINE      :: HWMP_RREQ_ATTEMPTS_EXPANDING_RING_DEFAULT
DESCRIPTION :: Default value of RREQ attempts at max TTL when
                using expanding ring search.
**/
#define HWMP_RREQ_ATTEMPTS_EXPANDING_RING_DEFAULT    (2)

/**
DEFINE      :: HWMP_RREQ_ATTEMPTS_FULL_TTL_DEFAULT
DESCRIPTION :: Default value of RREQ attempts when using full
                TTL based route discovery.
**/
#define HWMP_RREQ_ATTEMPTS_FULL_TTL_DEFAULT  (3)

/**
DEFINE      :: HWMP_RREQ_ATTEMPTS
DESCRIPTION :: Configured value of RREQ attempts.
**/
#define HWMP_RREQ_ATTEMPTS                   (hwmp->rreqAttempts)

/**
DEFINE      :: HWMP_DO_FLAG_DEFAULT
DESCRIPTION :: Default for 'Destination Only' flag used in RREQs.
**/
#define HWMP_RREQ_DO_FLAG_DEFAULT           (TRUE)

/**
DEFINE      :: HWMP_RF_FLAG_DEFAULT
DESCRIPTION :: Default for Reply & Forward flag used in RREQs.
**/
#define HWMP_RREQ_RF_FLAG_DEFAULT           (TRUE)

/**
DEFINE      :: HWMP_RREQ_FLOOD_RECORD_TIME
DESCRIPTION :: Timout for recording RREQ flooded frames in seen table.
**/
#define HWMP_RREQ_FLOOD_RECORD_TIME         \
        (2 * HWMP_NET_TRAVERSAL_TIME)

/**
DEFINE      :: HWMP_ACTIVE_ROOT_TIMEOUT
DESCRIPTION :: Timeout value for lifetime of root announcement.
**/
#define HWMP_ACTIVE_ROOT_TIMEOUT            (hwmp->activeRootTimeout)

/**
DEFINE      :: HWMP_RANN_PERIOD_DEFAULT
DESCRIPTION :: Default interval between root announcements.
**/
#define HWMP_RANN_PERIOD_DEFAULT            (4 * SECOND)

/**
DEFINE      :: HWMP_RANN_PERIOD
DESCRIPTION :: Configured interval between root announcements.
**/
#define HWMP_RANN_PERIOD                    (hwmp->rannPeriod)

/**
DEFINE      :: HWMP_RANN_PROPAGATION_DELAY
DESCRIPTION :: Propagation delay for retransmit of RANN.
                This is a minimum delay; a random value is added.
**/
#define HWMP_RANN_PROPAGATION_DELAY         (20 * MILLI_SECOND)

/**
DEFINE      :: HWMP_ROOT_MAINTENANCE_PERIOD
DESCRIPTION :: Period between route updates by an MP to root.
**/
#define HWMP_ROOT_MAINTENANCE_PERIOD        (4 * SECOND)

/**
DEFINE      :: HWMP_TBR_BETTER_PARENT_FACTOR
DESCRIPTION :: Factor by which the metric of a potential new parent
                should be better than existing parent.
                Value of 1.1 indicates 10% better metric.
**/
#define HWMP_TBR_BETTER_PARENT_FACTOR       (1.1)

/**
DEFINE      :: HWMP_REGISTRATION_DEFAULT
DESCRIPTION :: Default registration setting of root.
**/
#define HWMP_ROOT_REGISTRATION_DEFAULT      (TRUE)

/**
DEFINE      :: HWMP_BLACKLIST_TIMEOUT
DESCRIPTION :: Timeout for blacklist entries
                Not used.
**/
#define HWMP_BLACKLIST_TIMEOUT              \
        (HWMP_RREQ_ATTEMPTS * HWMP_NET_TRAVERSAL_TIME)

/**
DEFINE      :: HWMP_HOPCOUNT_MAX
DESCRIPTION :: Maximum value of hopcount.
**/
#define HWMP_HOPCOUNT_MAX                   (0xFF)

/**
DEFINE      :: HWMP_METRIC_MAX
DESCRIPTION :: Maximum value of metric.
**/
#define HWMP_METRIC_MAX                     (0xFFFFFFFF)

/**
DEFINE      :: HWMP_DEFAULT_MESSAGE_BUFFER_IN_PKT
DESCRIPTION :: Default message buffer size by count.
**/
#define HWMP_DEFAULT_MESSAGE_BUFFER_IN_PKT  (100)

/**
DEFINE      :: HWMP_MEM_UNIT
DESCRIPTION :: Default number of memory units allocated.
**/
#define HWMP_MEM_UNIT                       100

/**
DEFINE      :: HWMP_ROUTE_HASH_TABLE_SIZE
DESCRIPTION :: Hash size of routing table.
**/
#define HWMP_ROUTE_HASH_TABLE_SIZE          100

/**
DEFINE      :: HWMP_SENT_HASH_TABLE_SIZE
DESCRIPTION :: Hash size of sent table
**/
#define HWMP_SENT_HASH_TABLE_SIZE           20


// ------------------------------------------------------------------

/**
ENUM        :: HWMP_RreqType
DESCRIPTION :: Adhoc Route Discovery Type.
**/
enum HWMP_RreqType {
    HWMP_RREQ_EXPANDING_RING,
    HWMP_RREQ_FULL_TTL
};

/**
ENUM        :: HWMP_TbrState
DESCRIPTION :: Tree based routing states.
                - Init : initial state, reset values
                - Topology Forwarding : Start forwarding to root
**/
enum HWMP_TbrState {
    HWMP_TBR_S_INIT,
    HWMP_TBR_S_FORWARDING
};

/**
ENUM        :: HWMP_TbrFwdState
DESCRIPTION :: Tree based forwarding states.
**/
enum HWMP_TbrFwdState {
    HWMP_TBR_FWD_LEARNING,
    HWMP_TBR_FWD_FORWARDING
};

/**
ENUM        :: HWMP_TbrStateEvent
DESCRIPTION :: Tree based state events
**/
enum HWMP_TbrStateEvent{
    HWMP_TBR_S_EVENT_ENTER_STATE,
    HWMP_TBR_S_EVENT_RANN_TIMER,
    HWMP_TBR_S_EVENT_ROOT_MAINTENANCE_TIMER,
    HWMP_TBR_S_EVENT_RREP_TIMER,
    HWMP_TBR_S_EVENT_RAAN_RECEIVED,
    HWMP_TBR_S_EVENT_RREP_RECEIVED,
    HWMP_TBR_S_EVENT_LAST
};


// ------------------------------------------------------------------

/**
STRUCT      :: HWMP_PrecursorsNode
DESCRIPTION :: List of neighboring nodes that have generated
                or forwarded RREPs the destination for which
                there will be a route entry in the routing table.
**/
struct HWMP_PrecursorsNode
{
    Mac802Address destAddr;
    HWMP_PrecursorsNode *next;
};

/**
STRUCT      :: HWMP_Precursors
DESCRIPTION :: List to store precursor nodes
**/
struct HWMP_Precursors
{
    HWMP_PrecursorsNode *head;
    int size;
};

/**
STRUCT      :: HWMP_BlacklistNode
DESCRIPTION :: Entry in blacklist table.
                Not used.
**/
struct HWMP_BlacklistNode
{
    Mac802Address destAddr;
    HWMP_BlacklistNode *next;
};

/**
STRUCT      :: HWMP_BlacklistTable
DESCRIPTION :: List to store blacklisted nodes whose
                RREQ are to be discarded. Not used.
**/
struct HWMP_BlacklistTable
{
    HWMP_BlacklistNode *head;
    int size;
};

/**
STRUCT      :: HWMP_RouteEntry
DESCRIPTION :: Entry in routing table.
**/
struct HWMP_RouteEntry
{
    // Destination node and sequence number
    HWMP_AddrSeqInfo addrSeqInfo;

     // Next hop address to forward a packet for a destination
    Mac802Address nextHop;

    // Interface through which the packet should be forwarded
    // to reach the destination
    unsigned int interfaceIndex;

    // Number of hops to traverse to reach destination
    unsigned int hopCount;

    // Cumulative metric from this node to the destination
    unsigned int metric;

    // List of neighbors that have generated or forwarded
    // RREPs for this destination
    HWMP_Precursors precursors;

    // The life time of the route
    clocktype lifetime;

    // Holder for hop count after deletion of a route
    unsigned int lastHopCount;

    // Holder for last known metric after deletion
    unsigned int lastMetric;

    // Flag to mark if the route is active
    BOOL isActive;

    // If the link is locally repairable in case of link failure
    // Not used.
    BOOL locallyRepairable;

    // Pointer to next/previous route entries
    HWMP_RouteEntry* hashNext;
    HWMP_RouteEntry* hashPrev;
    HWMP_RouteEntry* expireNext;
    HWMP_RouteEntry* expirePrev;
    HWMP_RouteEntry* deleteNext;
    HWMP_RouteEntry* deletePrev;

};

/**
STRUCT      :: HWMP_RoutingTable
DESCRIPTION :: Routing table
**/
struct HWMP_RoutingTable
{
    HWMP_RouteEntry* routeHash[HWMP_ROUTE_HASH_TABLE_SIZE];
    HWMP_RouteEntry* routeExpireHead;
    HWMP_RouteEntry* routeExpireTail;
    HWMP_RouteEntry* routeDeleteHead;
    HWMP_RouteEntry* routeDeleteTail;
    int size;
};


/**
STRUCT      :: HWMP_RreqSeenNode
DESCRIPTION :: Store for <source, rreqId> pair for which an RREQ
                has been seen. This is to eliminate duplicate
                RREQs with the same signature.
**/
struct HWMP_RreqSeenNode
{
    // Source address of RREQ
    Mac802Address sourceAddr;

    // Broadcast ID of RREQ
    unsigned int rreqId;

    // List traversal pointers
    HWMP_RreqSeenNode* next;
    HWMP_RreqSeenNode* previous;
};

/**
STRUCT      :: HWMP_RreqSeenTable
DESCRIPTION :: Table for seen nodes
**/
struct HWMP_RreqSeenTable
{
    HWMP_RreqSeenNode *front;
    HWMP_RreqSeenNode *rear;
    HWMP_RreqSeenNode *lastFound;       // Cache the last found node

    int size;
};

/**
STRUCT      :: HWMP_BufferNode
DESCRIPTION :: Structure to store packets temporarily until a route
                to destination is found.
**/
struct HWMP_BufferNode
{
    // Frame related information
    DOT11s_FrameInfo* frameInfo;

    // Time of insertion in the buffer
    clocktype timestamp;

    // Pointer to the next node
    HWMP_BufferNode* next;
};

/**
STRUCT      :: HWMP_MsgBuffer
DESCRIPTION :: Linked list for message buffer nodes
**/
struct HWMP_MsgBuffer
{
    HWMP_BufferNode *head;
    int size;                       // in number of packets
    int numByte;
};

/**
STRUCT      :: HWMP_RreqSentNode
DESCRIPTION :: Structure to store information about RREQs that
                have been sent.
**/
struct HWMP_RreqSentNode
{
    // Destination for which the RREQ has been sent
    Mac802Address destAddr;

    // Last used TTL to find the route
    int ttl;

    // Number of times RREQ has been sent
    int times;

    // Pointer to next item
    HWMP_RreqSentNode* hashNext;

};

/**
STRUCT      :: HWMP_RreqSentTable
DESCRIPTION :: Table for RREQ send node entries.
**/
struct HWMP_RreqSentTable
{
    HWMP_RreqSentNode* sentHash[HWMP_SENT_HASH_TABLE_SIZE];
    int size;
};

/**
STRUCT      :: HWMP_MemPoolEntry
DESCRIPTION :: Entry for memory pool
**/
struct HWMP_MemPoolEntry
{
    HWMP_RouteEntry routeEntry ;
    HWMP_MemPoolEntry* next;
};


/**
STRUCT      :: HWMP_TbrParentItem
DESCRIPTION :: Entry for TBR Parent list.
**/
struct HWMP_TbrParentItem
{
    Mac802Address addr;
    HWMP_RannData rannData;
    clocktype lastRannTime;
    // Not required if using root
    // and not parent validation.
    clocktype lastValidationTime;
};

/**
STRUCT      :: HWMP_RootItem
DESCRIPTION :: Details of last RANN received.
**/
struct HWMP_RootItem
{
    HWMP_RannData rannData;
    clocktype lastRannTime;
    Mac802Address prevHopAddr;

    HWMP_RootItem()
        :   lastRannTime(0),
            prevHopAddr(INVALID_802ADDRESS)
    {}

    void Reset()
    {
        rannData.Reset();
        lastRannTime = 0;
        prevHopAddr = INVALID_802ADDRESS;
    }
};

/**
TYPEDEF     :: HWMP_TbrStateFn
DESCRIPTION :: Function prototype for the tree state.
**/
typedef
void (*HWMP_TbrStateFn)(
    Node* node,
    MacDataDot11* dot11,
    HWMP_Data* hwmp);


/**
STRUCT      :: HWMP_TbrStateData
DESCRIPTION :: Data for the TBR state machine.
**/
struct HWMP_TbrStateData {
    HWMP_TbrStateEvent event;
    DOT11s_FrameInfo* frameInfo;
    int interfaceIndex;
    HWMP_RannData* rannData;
    HWMP_RrepData* rrepData;
    Mac802Address parentAddr;
};

/**
STRUCT      :: HWMP_Stats
DESCRIPTION :: Statistics of HWMP
**/
struct HWMP_Stats
{
    unsigned int rreqBcInitiated;
    unsigned int rreqUcInitiated;
    unsigned int rreqResent;
    unsigned int rreqRelayed;

    unsigned int rreqReceived;
    unsigned int rreqReceivedAsDest;
    unsigned int rreqDuplicate;
    unsigned int rreqTtlExpired;
    //unsigned int rreqBlacklist;

    unsigned int rrepInitiatedAsDest;
    unsigned int rrepInitiatedAsIntermediate;
    unsigned int rrepForwarded;
    unsigned int rrepReceived;
    unsigned int rrepReceivedAsSource;

    unsigned int rrepGratInitiated;
    unsigned int rrepGratForwarded;
    unsigned int rrepGratReceived;

    unsigned int rerrInitiated;
    unsigned int rerrForwarded;
    unsigned int rerrReceived;
    unsigned int rerrDiscarded;

    unsigned int rannInitiated;
    unsigned int rannRelayed;

    unsigned int rannReceived;
    unsigned int rannDuplicate;
    unsigned int rannTtlExpired;
    //unsigned int rannBlacklist;

    unsigned int dataInitiated;
    unsigned int dataForwarded;
    unsigned int dataReceived;
    unsigned int dataDroppedForNoRoute;
    unsigned int dataDroppedForOverlimit;

    // Uncomment if values are of interest

    //unsigned int numMaxHopExceed;   // composite for all packet types

    //unsigned int numHops;
    //unsigned int numRoutes;
    //unsigned int numBrokenLinks;

    //unsigned int numMaxSeenTable;   // track table's max entries
    //unsigned int numLastFoundHits;  // track LastFound matches

};

/**
STRUCT      :: HWMP_Data (struct_hwmp)
DESCRIPTION :: HWMP main protocol data

**/
struct struct_hwmp
{
    clocktype myRouteTimeout;
    clocktype activeRouteTimeout;
    int routeDeletionConstant;
    clocktype reverseRouteTimeout;

    HWMP_RreqType rreqType;
    int rreqTtlInitial;
    int rreqTtlIncrement;
    int rreqTtlThreshold;
    int rreqAttempts;
    BOOL rreqDoFlag;
    BOOL rreqRfFlag;

    int bufferSizeInNumPacket;
    int bufferSizeInByte;
    HWMP_MsgBuffer msgBuffer;

    HWMP_RoutingTable routeTable;
    HWMP_RreqSentTable sentTable;
    HWMP_RreqSeenTable seenTable;

    // Performance Issue
    HWMP_MemPoolEntry* freeList;
    //HWMP_MemPoolEntry* refFreeList;

    HWMP_BlacklistTable blacklistTable;

    int seqNo;
    int rreqId;
    clocktype lastBroadcastSent;

    BOOL isExpireTimerSet;
    BOOL isDeleteTimerSet;


    // TBR variables
    BOOL isRoot;
    clocktype rannPeriod;
    clocktype activeRootTimeout;

    HWMP_TbrState tbrState;
    HWMP_TbrState prevTbrState;

    HWMP_TbrFwdState fwdState;

    HWMP_TbrStateFn tbrStateFn;
    HWMP_TbrStateData tbrStateData;

    HWMP_RootItem rootItem;
    clocktype rannPropagationDelay;
    LinkedList* parentList;
    HWMP_TbrParentItem* currentParent;
    BOOL potentialNewParent;
    BOOL rootRegistrationFlag;
    BOOL stationUpdateIsDue;

    HWMP_Stats* stats;
    BOOL printStats;
};


// ------------------------------------------------------------------

/**
FUNCTION   :: Hwmp_ReceiveControlFrame
LAYER      :: MAC
PURPOSE    :: Receive a protocol control frame from PHY.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ msg       : Message*      : received message
+ interfaceIndex : int      : interface at which message was received
RETURN     :: void
**/
void Hwmp_ReceiveControlFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_LinkFailureFunction
LAYER      :: MAC
PURPOSE    :: Handle link failure notification resulting from a
                packet drop to neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ nextHopAddr : Mac802Address : neighbor address
+ destAddr  : Mac802Address   : destination address
+ interfaceIndex : int      : interface at which failure occurred
RETURN     :: void
**/
void Hwmp_LinkFailureFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr,
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_LinkUpdateFunction
LAYER      :: MAC
PURPOSE    :: Handle notification that a neighbor link is active.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ nextHopAddr : Mac802Address : neighbor address
+ metric    : unsigned int  : metric to neighbor
+ lifetime  : clocktype     : lifetime
+ interfaceIndex : int      : interface to neighbor
RETURN     :: void
**/
void Hwmp_LinkUpdateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    unsigned int metric,
    clocktype lifetime,
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_LinkCloseFunction
LAYER      :: MAC
PURPOSE    :: Handle link failure notification to neighbor from
                a cause other than a packet drop.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ nextHopAddr : Mac802Address : neighbor address
+ interfaceIndex : int      : interface at which failure occurred
RETURN     :: void
**/
void Hwmp_LinkCloseFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_PathUpdateFunction
LAYER      :: MAC
PURPOSE    :: Handle notification to create/update path to destination.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ destAddr  : Mac802Address : destination of path
+ destSeqNo : unsigned int  : DSN of destination
+ hopCount  : int           : hop count to destinatin
+ metric    : unsigned int  : metric to neighbor
+ lifetime  : clocktype     : lifetime of path
+ nextHopAddr : Mac802Address : neighbor address
+ isActive  : BOOL          : TRUE to add/update path
+ interfaceIndex : int      : interface to neighbor
RETURN     :: void
**/
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
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_StationDisssociateFunction
LAYER      :: MAC
PURPOSE    :: Handle notification of station association.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ staAddr   : Mac802Address : station that has reassociated
+ interfaceIndex : int      : interface to neighbor
RETURN     :: void
**/
void Hwmp_StationDisassociateFunction(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    int interfaceIndex);

/**
FUNCTION   :: Hwmp_Init
LAYER      :: MAC
PURPOSE    :: Initialize protocol values; read user configuration.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 protocol data
+ nodeInput : NodeInput*    : pointer to user configuration data
+ address   : Address*      : pointer to link layer address
RETURN     :: void
NOTES      :: The protocol initialization takes a two-pass approach.
                Here, the defaults are set and user configuration
                is read in. Once the Mesh Point negotiates HWMP as
                the active protocol, Hwmp_InitActiveProtocol() is
                called to initialize working variables.
**/
void Hwmp_Init(
    Node* node,
    MacDataDot11* dot11,
    const NodeInput* nodeInput,
    Address* address,
    NetworkType networkType);

/**
FUNCTION   :: Hwmp_InitActiveProtocol
LAYER      :: MAC
PURPOSE    :: Initialize working variables on selection as active protocol
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
RETURN     :: void
**/
void Hwmp_InitActiveProtocol(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: Hwmp_Finalize
LAYER      :: MAC
PURPOSE    :: End of simulation printing of stats and clean up.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
RETURN     :: void
**/
void Hwmp_Finalize(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: Hwmp_RouterFunction
LAYER      :: MAC
PURPOSE    :: Route the frame to be sent
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ frameInfo : DOT11s_FrameInfo* : frame information
+ packetWasRouted : BOOL*   : Flag set TRUE if frame is routed
RETURN     :: void
NOTES      :: packetWasRouted is set to FALSE if the destination
                of the frame is either self or one of associated
                BSS stations.
**/
void Hwmp_RouterFunction(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo,
    BOOL* packetWasRouted);

/**
FUNCTION   :: Hwmp_HandleTimeout
LAYER      :: MAC
PURPOSE    :: Handle protocol timer events.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ msg       : Message*      : self timer message
RETURN     :: void
**/
void Hwmp_HandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

// ------------------------------------------------------------------

#endif //MAC_DOT11S_HWMP_H
