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

#ifndef AODV_H
#define AODV_H

typedef struct struct_network_aodv_str AodvData;

class D_AodvPrint : public D_Command
{
    private:
        AodvData *aodv;

    public:
        D_AodvPrint(AodvData *newAodv) { aodv = newAodv; }

        virtual void ExecuteAsString(const std::string& in, std::string& out);
};

// Aodv default timer and constant values ref:
// draft-ietf-manet-aodv-08.txt section: 12

// These default values are user configurable. If not configured by the
// user the protocol will run with these specified default values.

#define AODV_DEFAULT_ACTIVE_ROUTE_TIMEOUT      (3000 * MILLI_SECOND)

#define AODV_DEFAULT_ALLOWED_HELLO_LOSS        (2)

#define AODV_DEFAULT_HELLO_INTERVAL            (1000 * MILLI_SECOND)

#define AODV_DEFAULT_NET_DIAMETER              (35)

#define AODV_DEFAULT_NODE_TRAVERSAL_TIME       (40 * MILLI_SECOND)

#define AODV_DEFAULT_RREQ_RETRIES              (2)

#define AODV_DEFAULT_ROUTE_DELETE_CONST        (5)

#define AODV_DEFAULT_MESSAGE_BUFFER_IN_PKT     (100)

#define AODV_ACTIVE_ROUTE_TIMEOUT              (aodv->activeRouteTimeout)

#define AODV_DEFAULT_MY_ROUTE_TIMEOUT   (2 * AODV_ACTIVE_ROUTE_TIMEOUT)

#define AODV_ALLOWED_HELLO_LOSS        (aodv->allowedHelloLoss)

#define AODV_HELLO_INTERVAL            (aodv->helloInterval)

#define AODV_NET_DIAMETER              (aodv->netDiameter)

#define AODV_NODE_TRAVERSAL_TIME       (aodv->nodeTraversalTime)

#define AODV_RREQ_RETRIES              (aodv->rreqRetries)

#define AODV_ROUTE_DELETE_CONST        (aodv->rtDeletionConstant)

#define AODV_MY_ROUTE_TIMEOUT          (aodv->myRouteTimeout)

#define AODV_NEXT_HOP_WAIT             (AODV_NODE_TRAVERSAL_TIME + 10)

// This is what is stated in the AODV spec for Net Traversal Time.
#define AODV_NET_TRAVERSAL_TIME_NEW   (2 * AODV_NODE_TRAVERSAL_TIME * \
                                       AODV_NET_DIAMETER)

#define AODV_NET_TRAVERSAL_TIME   (3 * AODV_NODE_TRAVERSAL_TIME * \
                                   AODV_NET_DIAMETER / 2)

#define AODV_BLACKLIST_TIMEOUT  (AODV_RREQ_RETRIES * AODV_NET_TRAVERSAL_TIME)

#define AODV_FLOOD_RECORD_TIME  (2 * AODV_NET_TRAVERSAL_TIME)

#define AODV_DELETE_PERIOD  (aodv->deletePeriod)


#define AODV_LOCAL_ADD_TTL             (2)

#define AODV_MAX_REPAIR_TTL            (0.3 * AODV_NET_DIAMETER)

#define AODV_MIN_REPAIR_TTL            lastHopCountToDest

#define AODV_REV_ROUTE_LIFE            (AODV_NET_TRAVERSAL_TIME)

#define AODV_DEFAULT_TTL_START                 (1)

#define AODV_DEFAULT_TTL_INCREMENT             (2)

#define AODV_DEFAULT_TTL_THRESHOLD             (7)

#define AODV_TTL_START                 (aodv->ttlStart)

#define AODV_TTL_INCREMENT             (aodv->ttlIncrement)

#define AODV_TTL_THRESHOLD             (aodv->ttlMax)

#define AODV_INFINITY                  (-1)

#define AODV_BROADCAST_JITTER          (20 * MILLI_SECOND)

// Performance Issue

#define AODV_MEM_UNIT                     100

#define AODV_ROUTE_HASH_TABLE_SIZE        100

#define AODV_SENT_HASH_TABLE_SIZE        20

// Aodv Packet Types

#define AODV_RREQ     1   // route request packet type
#define AODV_RREP     2   // route reply packet type
#define AODV_RERR     3   // route error packet type
#define AODV_RREP_ACK 4   // route reply acknowledgement packet type

// /**
// Macro that return type of AODV Message.
// **/
#define AODV_GetType(bits) ((bits & 0xff000000) >> 24)

// /**
// Constant for J-bit.
// **/
#define AODV_J_BIT 0x00800000

// /**
// Constant for D-bit.
// **/
#define AODV_D_BIT 0x00100000

// /**
// Constant for N-bit.
// **/
#define AODV_N_BIT 0x00800000

// /**
// Constant for R-bit.
// **/
#define AODV_R_BIT 0x00800000

// /**
// Constant for R-bit.
// **/
#define AODV_RREQ_R_BIT 0x00400000

// /**
// Constant for R-bit.
// **/
#define AODV_G_BIT 0x00200000

// /**
// Constant for A-bit.
// **/
#define AODV_A_BIT 0x00400000

// /**
// Constant for G-bit.
// **/
#define AODV_G_BIT 0x00200000

// /**
// AODV Hop Count Bits.
// **/
#define AODV_HOP_COUNT_BITS 0x000000ff

// /**
// AODV Destination Count Bits.
// **/
#define AODV_DEST_COUNT_BITS 0x000000ff

// /**
// AODV for IPv4 Prefix Size Bits.
// **/
#define AODV_PREFIX_SIZE_BITS 0x00001f00

// /**
// AODV for IPv4 precursor destination address.
// **/
#define AODV_Ipv4PrecDestAddr precursors.head->destAddr.interfaceAddr.ipv4

// /**
// AODV for IPv4 Destination sequence number.
// **/
//#define AODV_Ipv4DestSeqNum destination.ipv4_destination.seqNum

// /**
// AODV RREQ Hop Count.
// **/
#define AODV_RREQ_HopCount typeBitsHopcounts&0x000000ff

// /**
// AODV RREP Hop Count.
// **/
#define AODV_RREP_HopCount typeBitsPrefixSizeHop&0x000000ff

// /**
// AODV RRERR Hop Count.
// **/
#define AODV_REER_DestCount typeBitsDestCount&0x000000ff

#define AODV_Ip6HostBit interfaceAddr.ipv6.s6_addr32[3]


// /**
// STRUCT         ::    AodvAddrSeqInfo
// DESCRIPTION    ::    Address and sequence number for AODV for IPv4
//                      to be used in Packets.
// **/
typedef struct
{
    NodeAddress address;
    UInt32 seqNum;
} AodvAddrSeqInfo;

// /**
// STRUCT         ::    Aodv6AddrSeqInfo
// DESCRIPTION    ::    Address and sequence number for AODV for IPv6
//                      to be used in Packets.
// **/
typedef struct
{
    in6_addr address;
    UInt32 seqNum;
} Aodv6AddrSeqInfo;

// /**
// STRUCT         ::    AodvRreqPacketInfo
// DESCRIPTION    ::    AODV Route Request Message part which is same for
//                      both IPv6 and IPv4.
// **/
typedef struct
{
    // Next 32-bit variable will be used as
    // First 8-bits for type
    // next bit is J-bit
    // next bit is R-bit
    // next bit is G-bit
    // next bit is D-bit
    // next bit is U-bit
    // next 11-bits are reserved and set to zero
    // last 8-bits is hop count
    UInt32 typeBitsHopcounts;
    UInt32 floodingId;
} AodvRreqPacketInfo;

// /**
// STRUCT         ::    AodvRreqPacket
// DESCRIPTION    ::    AODV for IPv4 route request message format.
// **/
typedef struct
{
    AodvRreqPacketInfo info;

    // destination address and sequence
    AodvAddrSeqInfo destination;

    // source address and sequence
    AodvAddrSeqInfo source;
} AodvRreqPacket;

// /**
// STRUCT         ::    Aodv6RreqPacket
// DESCRIPTION    ::    AODV for IPv6 route request message format.
// **/
typedef struct
{
    AodvRreqPacketInfo info;

    // destination address and sequence
    Aodv6AddrSeqInfo destination;

    // source address and sequence
    Aodv6AddrSeqInfo source;
} Aodv6RreqPacket;

// /**
// STRUCT         ::    AodvRrepPacket
// DESCRIPTION    ::    AODV for IPv4 route reply message format.
// **/
typedef struct
{
    // Next 32-bit variable will be used as
    // First 8-bits for type
    // next bit is R-bit
    // next bit is A-bit
    // if IPv4 then next 9-bits are reserved and set to zero
    // else if IPv6 then next 7-bits are reserved and set to zero
    // if IPv4 then next 5-bit is prefix size
    // else if IPv6 then next 8-bits is prefix size
    // last 8-bits is hop count
    UInt32 typeBitsPrefixSizeHop;

    // destination address and sequence
    AodvAddrSeqInfo  destination;

    // address of the source node which issued the request
    NodeAddress sourceAddr;
    UInt32 lifetime;
} AodvRrepPacket;

// /**
// STRUCT         ::    Aodv6RrepPacket
// DESCRIPTION    ::    AODV for IPv6 route reply message format.
// **/
typedef struct
{
    // Next 32-bit variable will be used as
    // First 8-bits for type
    // next bit is R-bit
    // next bit is A-bit
    // if IPv4 then next 9-bits are reserved and set to zero
    // else if IPv6 then next 7-bits are reserved and set to zero
    // if IPv4 then next 5-bit is prefix size
    // else if IPv6 then next 8-bits is prefix size
    // last 8-bits is hop count
    UInt32 typeBitsPrefixSizeHop;

    // destination address and sequence
    Aodv6AddrSeqInfo destination;

    // address of the source node which issued the request
    in6_addr sourceAddr;
    UInt32 lifetime;
} Aodv6RrepPacket;

// /**
// STRUCT         ::    AodvRerrPacket
// DESCRIPTION    ::    AODV for IPv4/IPv6 route error message format.
// **/
typedef struct
{
    // Next 32-bit variable will be used as below
    // First 8-bits for type
    // next bit is N-bit
    // next 15-bits are reserved and set to zero
    // next 8-bits are destination count
    UInt32 typeBitsDestCount;

    // unreachable destination list will be implemented dynamically.

} AodvRerrPacket;

// /**
// STRUCT         ::    AodvRackPacket
// DESCRIPTION    ::    AODV for IPv4/IPv6 route acknowledgement
//                      message format.
// **/
typedef struct
{
    UInt8 type;
    UInt8 reserved;
} AodvRackPacket;

// /**
// STRUCT         ::    AodvPrecursorsNode
// DESCRIPTION    ::    AODV for IPv4/IPv6 List of neighboring nodes which
//                      have generated or forwarded Route Reply to the
//                      destination for which there will be one route entry
//                      in the routing table
// **/
typedef struct str_aodv_precursors
{
    Address destAddr;
    struct str_aodv_precursors *next;
} AodvPrecursorsNode;

// /**
// STRUCT         ::    AodvPrecursors
// DESCRIPTION    ::    AODV for IPv4/IPv6 List to store precursor nodes
// **/
typedef struct
{
    AodvPrecursorsNode *head;
    int size;
} AodvPrecursors;

// /**
// STRUCT         ::    AodvBlacklistNode
// DESCRIPTION    ::    AODV for IPv4/Ipv6 Blacklisted node
// **/
typedef struct str_aodv_black_list_neighbors
{
    Address destAddr;
    struct str_aodv_black_list_neighbors *next;
} AodvBlacklistNode;

// /**
// STRUCT         ::    AodvBlacklistTable
// DESCRIPTION    ::    AODV for IPv4/Ipv6 List to store blacklisted nodes whose
//                      RREQ are to be discarded
// **/
typedef struct
{
    AodvBlacklistNode *head;
    int size;
} AodvBlacklistTable;

// /**
// STRUCT         ::    AodvRouteEntry
// DESCRIPTION    ::    Aodv for IPv4/IPv6 routing table entry for
//                      a destination
// **/
typedef struct route_table_row
{
    // Destination node and destination sequence number
    Address destination;
    UInt32 destSeqNum;

    // The interface through which the packet should be forwarded to reach
    // the destination
    UInt32 outInterface;

    // Number of hops to traverse to reach the destination
    Int32 hopCount;

    // Place to store hop count after deletion of a route
    Int32 lastHopCount;

    // The next hop address to forward a packet to reach the destination.
    Address nextHop;

    // List of neighbors which has generated or forwarded RREP for this
    // destination
    AodvPrecursors precursors;

    // The life time of the route
    clocktype lifetime;

    // Not used now, to be used to chose among equal distant routes.
    clocktype latency;

    UInt32 helloSeqNum;

    // Whether the route is active
    BOOL activated;

    // If the link is locally repairable in case of link failure.
    BOOL locallyRepairable;

    // Pointer to next route entry.
    struct route_table_row* hashNext;
    struct route_table_row* hashPrev;
    struct route_table_row* expireNext;
    struct route_table_row* expirePrev;
    struct route_table_row* deleteNext;
    struct route_table_row* deletePrev;
} AodvRouteEntry;

// /**
// STRUCT         ::    AodvRoutingTable
// DESCRIPTION    ::    Aodv for IPv4/IPv6 routing table
// **/
typedef struct
{
    AodvRouteEntry* routeHashTable[AODV_ROUTE_HASH_TABLE_SIZE];
    AodvRouteEntry* routeExpireHead;
    AodvRouteEntry* routeExpireTail;
    AodvRouteEntry* routeDeleteHead;
    AodvRouteEntry* routeDeleteTail;
    int   size;
} AodvRoutingTable;

// /**
// STRUCT         ::    AodvRreqSeenNode
// DESCRIPTION    ::    AODV IPv4/IPv6 Structure to store <source, floodingId>
//                      pair for which one RREQ has been seen. The purpose
//                      of it is if one RREQ has been seen (ie. processed for
//                      one source and destination pair then successive RREQs
//                      for same source and destination pair if received will
//                      be discarded silently)
// **/
typedef struct str_aodv_rreq_seen
{
    // Source address of RREQ
    Address srcAddr;

    // Broadcast address of RREQ
    unsigned int floodingId;
    struct str_aodv_rreq_seen *next;
    struct str_aodv_rreq_seen *previous;
} AodvRreqSeenNode;

// /**
// STRUCT         ::    AodvRreqSeenTable
// DESCRIPTION    ::    AODV for IPv4/IPv6 Route Request Seen Table
// **/
typedef struct
{
    AodvRreqSeenNode *front;
    AodvRreqSeenNode *rear;
    AodvRreqSeenNode *lastFound;    // Cache the last found node
    int size;
} AodvRreqSeenTable;

// /**
// STRUCT         ::    AodvBufferNode
// DESCRIPTION    ::    AODV for IPv4/IPv6 Structure to store packets
//                      temporarily until one route to the destination
//                      of the packet is found or the packets are timed out
//                      to find a route
// **/
typedef struct str_aodv_fifo_buffer
{
   // Destination address of the packet
   Address destAddr;

    // The time when the packet was inserted in the buffer
    clocktype timestamp;

    // The last hop which sent the data
    Address previousHop;

    // The packet to be sent
    Message *msg;

    // Pointer to the next message.
    struct str_aodv_fifo_buffer *next;
} AodvBufferNode;

// /**
// STRUCT         ::    AodvMessageBuffer
// DESCRIPTION    ::    Link list for message buffer
// **/
typedef struct
{
    AodvBufferNode *head;
    int size;
    int numByte;
} AodvMessageBuffer;

// /**
// STRUCT         ::    AodvRreqSentNode
// DESCRIPTION    ::    AODV IPv4/IPv6 Structure to store information about
//                      messages for which RREQ has been sent.
//                      These information are necessary until a route is
//                      found for the destination
// **/
typedef struct str_aodv_sent_node
{
    // Destination for which the RREQ has been sent
    Address destAddr;

    // Last used TTL to find the route
    int ttl;

    // Number of times RREQ has been sent
    int times;
    struct str_aodv_sent_node* hashNext;
}AodvRreqSentNode;

// /**
// STRUCT         ::    AodvRreqSentTable
// DESCRIPTION    ::    AODV IPv4/IPv6 structure for Sent node entries
// **/
typedef struct
{
    AodvRreqSentNode* sentHashTable[AODV_SENT_HASH_TABLE_SIZE];
    int size;
} AodvRreqSentTable;

// /**
// STRUCT         ::    AodvMemPollEntry
// DESCRIPTION    ::    AODV IPv4/IPv6 Memory Pool
// **/
typedef struct str_aodv_mem_poll
{
    AodvRouteEntry routeEntry ;
    struct str_aodv_mem_poll* next;
} AodvMemPollEntry;

// /**
// STRUCT         ::    AodvStats
// DESCRIPTION    ::    AODV IPv4/IPv6 Structure to store the statistical
//                      informations.
// **/
typedef struct
{
    D_UInt32 numRequestInitiated;
    UInt32 numRequestResent;
    UInt32 numRequestRelayed;
    UInt32 numRequestForLocalRepair;
    UInt32 numRequestForAlternateRt;
    UInt32 numRequestRecved;
    UInt32 numRequestDuplicate;
    UInt32 numRequestTtlExpired;
    UInt32 numSenderInBlacklist;
    UInt32 numRequestRecvedAsDest;
    UInt32 numReplyInitiatedAsDest;
    UInt32 numReplyInitiatedAsIntermediate;
    UInt32 numReplyForwarded;
    UInt32 numGratReplySent;
    UInt32 numReplyRecved;
    UInt32 numReplyRecvedForLocalRepair;
    UInt32 numReplyRecvedAsSource;
    UInt32 numHelloSent;
    UInt32 numHelloRecved;
    UInt32 numRerrInitiated;
    UInt32 numRerrInitiatedWithNFlag;
    UInt32 numRerrForwarded;
    UInt32 numRerrForwardedWithNFlag;
    UInt32 numRerrRecved;
    UInt32 numRerrRecvedWithNFlag;
    UInt32 numRerrDiscarded;
    UInt32 numDataInitiated;
    UInt32 numDataForwarded;
    UInt32 numDataRecved;
    UInt32 numDataDroppedForNoRoute;
    UInt32 numDataDroppedForOverlimit;
    UInt32 numMaxHopExceed;
    UInt32 numHops;
    UInt32 numRoutes;
    UInt32 numBrokenLinks;

    // Added to track table's max entries
    UInt32 numMaxSeenTable;

    // Added to track LastFound matches
    UInt32 numLastFoundHits;
} AodvStats;

// /**
// STRUCT         ::    AodvInterfaceInfo
// DESCRIPTION    ::    AODV IPv4/IPv6 InterfaceInfo
// **/
typedef struct str_aodv_interface_info
{
    Address address;
    UInt32 ip_version;
    BOOL aodv4eligible;
    BOOL aodv6eligible;
    BOOL AFlag;
} AodvInterfaceInfo;

// /**
// STRUCT         ::    AodvData
// DESCRIPTION    ::    AODV IPv4/IPv6 structure to store all necessary
//                      informations.
// **/
typedef struct struct_network_aodv_str
{
    // set of user configurable parameters
    Int32 netDiameter;
    clocktype nodeTraversalTime;
    clocktype myRouteTimeout;
    Int32 allowedHelloLoss;
    clocktype activeRouteTimeout;
    Int32 rreqRetries;
    clocktype helloInterval;
    clocktype deletePeriod;
    Int32 rtDeletionConstant;

    RandomSeed  aodvJitterSeed;

    // D- Flag set
    BOOL Dflag;

    // set of aodv protocol dependent parameters
    AodvRoutingTable routeTable;
    AodvRreqSeenTable seenTable;
    AodvRreqSentTable sent;
    AodvMessageBuffer msgBuffer;

    Int32 bufferSizeInNumPacket;
    Int32 bufferSizeInByte;
    AodvBlacklistTable blacklistTable;
    AodvStats stats;
    BOOL statsCollected;
    BOOL statsPrinted;
    BOOL processHello;
    BOOL processAck;
    BOOL localRepair;
    BOOL findAlternateRtIfNSet;
    BOOL biDirectionalConn;
    Int32 ttlStart;
    Int32 ttlIncrement;
    Int32 ttlMax;
    UInt32 seqNumber;
    UInt32 floodingId;
    clocktype lastBroadcastSent;

    // Performance Issue
    AodvMemPollEntry* freeList;

    BOOL isExpireTimerSet;
    BOOL isDeleteTimerSet;

    // Point to AODV for IPv6 data if the node is dual IP
    struct_network_aodv_str* aodv6;
    BOOL isDualIpNode;
    AodvInterfaceInfo* iface;
    Address broadcastAddr;
    int defaultInterface;
    Address defaultInterfaceAddr;
} AodvData;

void AodvInit(
    Node *node,
    AodvData **aodvPtr,
    const NodeInput *nodeInput,
    int interfaceIndex,
    NetworkRoutingProtocolType aodvProtocolType);

void AodvFinalize(Node *node, int i, NetworkType networkType);

void AodvRouterFunction(
    Node *node,
    Message *msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL *packetWasRouted);

void Aodv4RouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted);

void Aodv6RouterFunction(
    Node* node,
    Message* msg,
    in6_addr destAddr,
    in6_addr previousHopAddress,
    BOOL* packetWasRouted);

void
Aodv4MacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const NodeAddress nextHopAddress,
    const int incommingInterfaceIndex);

void
Aodv6MacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const in6_addr nextHopAddress,
    const int incommingInterfaceIndex);

void
AodvMacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const Address nextHopAddress,
    const int incommingInterfaceIndex);

void AodvHandleProtocolPacket(
    Node *node,
    Message *msg,
    Address srcAddr,
    Address destAddr,
    int ttl,
    int interfaceIndex);

void
AodvHandleProtocolEvent(
    Node *node,
    Message *msg);

BOOL
AodvConfigureAFlag(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex);

void AodvGetTunnelTypeAndInterface(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex,
    AodvInterfaceInfo* iface);
//--------------------------------------------------------------------------
// FUNCTION   :: RoutingAodvHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
//               due to DHCP feature
// PARAMETERS ::
// + node                    : Node*       : Pointer to Node structure
// + interfaceIndex          : Int32       : interface index
// + oldAddress              : Address*    : old address
// + subnetMask              : NodeAddress : subnetMask
// + NetworkType networkType : type of network protocol
// RETURN     ::             : void        : NULL
//--------------------------------------------------------------------------
void RoutingAodvHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingAodvIPv6HandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
//               due to autoconfiguration feature
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldGlobalAddress   : in6_addr* old global address
// RETURN :: void : NULL
//---------------------------------------------------------------------------
void RoutingAodvIPv6HandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    in6_addr* oldGlobalAddress);

#endif
