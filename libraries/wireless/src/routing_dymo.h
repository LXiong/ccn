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

//* dymo.h */
// PROTOCOL :: DYMO (DYNAMIC MANET ON DEMAND ROUTING PROTOCOL )

// SUMMARY ::

//The Dynamic MANET On-demand (DYMO) routing protocol enables reactive,
// multihop routing between participating nodes that wish to
//communicate.  The basic operations of the DYMO protocol are route
// discovery and route management.  During route discovery the
//originating node initiates dissemination of a Route Request (RREQ)
// throughout the network to find the target node.  During this
//dissemination process, each intermediate node records a route to the
//originating node.  When the target node receives the RREQ, it
//responds with a Route Reply (RREP) unicast toward the originating
//node.  Each node that receives the RREP records a route to the target
//node, and then the RREP is unicast toward the originating node.  When
//the originating node receives the RREP, routes have then been
//established between the originating node and the target node in both
//directions.In order to react to changes in the network topology nodes
//maintain
//their routes and monitor their links.  When a data packet is received
//  for a route or link that is no longer available the source of the
//packet is notified.  A Route Error (RERR) is sent to the packet
//source to indicate the current route is broken.  Once the source
//receives the RERR, it can perform route discovery if it still has
//packets to deliver.DYMO uses sequence numbers as they have been proven to
//ensure loop freedom .

// LAYER :: Network Layer

// OMITTED_FEATURES :: (Title) : (Description) (list)
// Security Considerations

// ASSUMPTIONS ::
// + In DYMO ,we are assuming that all RERR message (Control Message)
//                is multicast messages .
// + DYMO can use any mean to monitor link breakage on active routes
//    through e.g.Link layer feedback , Hello messages , Neighbor discovery
//    ,  and Route timeout , but we are using hello messages to monitor link
//        breakage between neighboring nodes.


// STANDARD :: Coding standard follows the following Coding Guidelines
//             1. SNT C Coding Guidelines for QualNet 3.2

// **/


/* Data Structures used for the Dynamic On-Demand Manet Routing Protocol (
                                                                DYMO) for*/
/* Mobile Ad-Hoc Networking (MANET)*/

/* Protect against multiple includes.*/

#ifndef DYMO_H
#define DYMO_H


typedef struct struct_network_dymo_str  DymoData ;
class D_DymoPrint : public D_Command
{
    private:
        DymoData *dymo;

    public:
        D_DymoPrint(DymoData *newDymo) { dymo = newDymo; }

        virtual void ExecuteAsString(const std::string& in, std::string &out);
};



#define DYMO_IPv6_PREFIX_LENGTH 64

// These default values are user configurable. If not configured by the
// user the protocol will run with these specified default values.
#define DYMO_UNKNOWN_HOP_COUNT                  0

#define DYMO_UNKOWN_SEQ_NUM                     0

#define DYMO_DEFAULT_VALID_ROUTE_TIMEOUT        (5000 * MILLI_SECOND)

#define DYMO_DEFAULT_NEW_ROUTE_TIMEOUT         (DYMO_DEFAULT_VALID_ROUTE_TIMEOUT)

#define DYMO_DEFAULT_USED_ROUTE_TIMEOUT         (DYMO_DEFAULT_VALID_ROUTE_TIMEOUT)

#define DYMO_DEFAULT_WAIT_TIME                  (1000 * MILLI_SECOND)

#define DYMO_DEFAULT_ALLOWED_HELLO_LOSS         (4)

#define DYMO_DEFAULT_HELLO_INTERVAL             (1000 * MILLI_SECOND)

#define DYMO_DEFAULT_HOP_LIMIT                  (10)

#define DYMO_DEFAULT_NODE_TRAVERSAL_TIME    (90 * MILLI_SECOND)

#define DYMO_DEFAULT_NET_TRAVERSAL_TIME      (1000 * MILLI_SECOND)

#define DYMO_DEFAULT_RREQ_RETRIES           (3)

#define DYMO_DEFAULT_ROUTE_DELETE_CONST     (5)

#define DYMO_DEFAULT_MESSAGE_BUFFER_IN_PKT  (50)

#define DYMO_ALLOWED_HELLO_LOSS             (dymo->allowedHelloLoss)

#define DYMO_HELLO_INTERVAL                 (dymo->helloInterval)

#define DYMO_MAX_HOP_LIMIT                      (dymo->hopLimit)

#define DYMO_TTL_THRESHOLD                  (dymo->ttlMax)

#define DYMO_NODE_TRAVERSAL_TIME            (dymo->nodeTraversalTime)

#define DYMO_RREQ_RETRIES                   (dymo->rreqRetries)

#define DYMO_DEFAULT_HOP_COUNT              (0)

#define DYMO_ALLOWED_HELLO_LOSS             (dymo->allowedHelloLoss)

#define DYMO_DELETE_ROUTE_TIMEOUT           (dymo->deleteRouteTimeout)

#define DYMO_NEXT_HOP_WAIT                  (DYMO_NODE_TRAVERSAL_TIME + 10)

// This is what is stated in the DYMO spec for Net Traversal Time.Fields
// which are not given
//we have taken default value for those parameter from dymo.

#define DYMO_NET_TRAVERSAL_TIME        (DYMO_NODE_TRAVERSAL_TIME * \
                                                    (DYMO_MAX_HOP_LIMIT+1))

#define DYMO_DEFAULT_DELETE_ROUTE_TIMEOUT   MAX(2 * DYMO_DEFAULT_VALID_ROUTE_TIMEOUT, \
                                                DYMO_ALLOWED_HELLO_LOSS * DYMO_HELLO_INTERVAL)

#define DYMO_FLOOD_RECORD_TIME             (2 * DYMO_NET_TRAVERSAL_TIME)


#define DYMO_LOCAL_ADD_TTL                      (2)

#define DYMO_DEFAULT_TTL_START                  (1)

#define DYMO_DEFAULT_TTL_INCREMENT              (2)

#define DYMO_DEFAULT_TTL_THRESHOLD              (7)

#define DYMO_TTL_START                          (dymo->ttlStart)

#define DYMO_TTL_INCREMENT                      (dymo->ttlIncrement)

#define DYMO_HOP_COUNT_THRESHOLD                (dymo->hopCntMax)

#define DYMO_INFINITY                           (254)

#define DYMO_BROADCAST_JITTER                   (10 * MILLI_SECOND)


// Performance Issue

#define DYMO_MEM_UNIT                           100

#define DYMO_ROUTE_HASH_TABLE                   100

#define DYMO_SENT_HASH_TABLE                    20

#define DYMO_MESSAGE_SEMANTICS_ADD_MSG_SEQ_ID   0x00
#define DYMO_MESSAGE_SEMANTICS                  0x01

#define DYMO_ADD_TLV_SEMANTICS                  0x11//0x0E
#define DYMO_ADD_TLV_SEMANTICS_BIT_0 0x01
#define DYMO_ADD_TLV_SEMANTICS_BIT_1 0x02
#define DYMO_ADD_TLV_SEMANTICS_BIT_2 0x04
#define DYMO_ADD_TLV_SEMANTICS_BIT_3 0x08
#define DYMO_ADD_TLV_SEMANTICS_BIT_4 0x10

#define DYMO_HELLO_MESSAGE_HOP_COUNT            0
#define DYMO_HELLO_MESSAGE_TTL                  1

#define DYMO_Ip6HostBit     interfaceAddr.ipv6.s6_addr32[3]

#define DYMO_ADDRBLK_SEMANTICS 0x02 // 0x04
#define DYMO_ADDRBLK_SEMANTICS_BIT_0 0x01 
#define DYMO_ADDRBLK_SEMANTICS_BIT_1 0x02 
#define DYMO_ADDRBLK_SEMANTICS_BIT_2 0x04 

//-----------------------------------------------------------------------
// STRUCT         ::   DymoAddrSeqInfo
// DESCRIPTION    ::   Address and sequence number for Dymo for IPv4 to
//                       be used in Packets.
//-----------------------------------------------------------------------

typedef struct
{
    NodeAddress      address;
    UInt16           SeqNum;
} DymoAddrSeqInfo;

//-----------------------------------------------------------------------
// STRUCT         ::   Dymo6AddrSeqInfo
// DESCRIPTION    ::    Address and sequence number for Dymo for IPv6
//                        to be used in Packets.
//-----------------------------------------------------------------------

typedef struct
{
    in6_addr    address ;
    UInt16        SeqNum;
}Dymo6AddrSeqInfo;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoBlacklistNode
// DESCRIPTION    ::    DYMO for IPv4/Ipv6 Blacklisted node
//-----------------------------------------------------------------------
typedef struct str_dymo_black_list_neighbors
{
    Address targtAddr;
    struct str_dymo_black_list_neighbors *next;
}DymoBlacklistNode;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRouteEntry
// DESCRIPTION    ::    DYMO for IPv4/IPv6 routing table entry for
//                      a destination
//-----------------------------------------------------------------------
typedef struct dymo_route_table_row
{
    Address destination;  // The IP address of the target node and
    UInt16  SeqNum;       //target sequence number

    Int32      intface;        // The interface through which the packet
                                //should be forwarded to reach the target
    Address     nextHop;        //The next hop address to which to forward a
                                //packet to reach thetarget
    UInt8       hopCount;        // Number of internediate hops to reach the
                                //target.
    UInt8       lastHopCount;    // Place to store hop count after
                                // deletion of a route

    clocktype   UsedRouteTimeout;     //If the current time is after Route.Valid
                                 //Timeout then the routing table entry is
                                 //considerd to be invalid.means the route is
                                 //not active    
    BOOL        activated;   // Whether the route is active or not

    // Dymo Draft 09 
    BOOL         isNewRoute; // true - denotes new route
                             // false - denotes used route
    BOOL         isToDelete; 

    UInt8       Prefix;         // This field specify the size of the subnet
                                //reachablethrough the Route.DestAddress
    BOOL        isGateway;    // 1 bit selector indicate whether the Route.Dest
                                 //Address is a gateaway or not.

    UInt32 helloSeqNum;
    struct dymo_route_table_row* hashNext; // Pointer to next route entry
    struct dymo_route_table_row* hashPrev;
    struct dymo_route_table_row* expireNext;
    struct dymo_route_table_row* expirePrev;
    struct dymo_route_table_row* deleteNext;
    struct dymo_route_table_row* deletePrev;
} DymoRouteEntry;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRoutingTable
// DESCRIPTION    ::    DYMO for IPv4/IPv6 routing table
//-----------------------------------------------------------------------
typedef struct
{
    DymoRouteEntry* routeHashTable[DYMO_ROUTE_HASH_TABLE];
    DymoRouteEntry* routeExpireHead;
    DymoRouteEntry* routeExpireTail;
    DymoRouteEntry* routeDeleteHead;
    DymoRouteEntry* routeDeleteTail;
    Int32   size;
} DymoRoutingTable;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRreqSeenNode
// DESCRIPTION    ::    DYMO IPv4/IPv6 Structure to store <source,
//                        msg_seq_num> for which one RREQ has been seen.
//                        (i.e. processed for one source and destination
//                        pair then successive RREQs for same source and
//                        destination pair if received will be discarded
//                        silently).
//-----------------------------------------------------------------------
typedef struct str_dymo_rreq_seen
{
    Address srcAddr;             // Source address of RREQ
    UInt16 msgSeqId;         // multicast address of RREQ
    struct str_dymo_rreq_seen *next;
    struct str_dymo_rreq_seen *previous;    // Added to traverse the list
                                            // from the rear to the front.
}DymoRreqSeenNode;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoBufferNode
// DESCRIPTION    ::    DYMO for IPv4/IPv6 Structure to store packets
//                      temporarily until one route to the destination
//                      of the packet is found or the packets are timed
//                        out to find a route.
//-----------------------------------------------------------------------
typedef struct str_dymo_fifo_buffer
{
    Address     targtAddr;       // target address of the packet
    Address     lastHopAddress; // The node from which it has got the packet
    clocktype   timestamp;     // The time when the packet was inserted in
                                // the buffer
    Address     previousHop;    // The last hop which sent the data
    Message*    msg;               // The packet to be sent
    struct str_dymo_fifo_buffer *next; // Pointer to the next message
}DymoBufferNode;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoMessageBuffer
// DESCRIPTION    ::    Link list for message buffer
//-----------------------------------------------------------------------
typedef struct
{
    DymoBufferNode  *head;
    Int32           size;                   // in Number of packets
    Int32           numByte;
} DymoMessageBuffer;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRreqSentNode
// DESCRIPTION    ::    DYMO IPv4/IPv6 Structure to store information
//                        about nodes for which RREQ has been sent.
//                      These information are necessary until a route
//                        is found for the destination.
//-----------------------------------------------------------------------
typedef struct str_dymo_sent_node
{
    Address targtAddr;          // target for which the RREQ has been sent
    UInt8   ttl;               // Last used TTL to find the route
    clocktype waitTime;        // Last used rreq wait time
    Int32   times;             // Number of times RREQ has been sent
    struct  str_dymo_sent_node* hashNext;
}DymoRreqSentNode;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRreqSeenTable
// DESCRIPTION    ::    DYMO IPv4/IPv6 structure for node which have already
//                        sent the Routing Message
//-----------------------------------------------------------------------
typedef struct
{
    DymoRreqSeenNode *front;
    DymoRreqSeenNode *rear;
    DymoRreqSeenNode *lastFound;    // Cache the last found node
    Int32 size;
} DymoRreqSeenTable;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoRreqSentTable
// DESCRIPTION    ::    DYMO IPv4/IPv6 structure for Sent node entries
//-----------------------------------------------------------------------
typedef struct
{
    DymoRreqSentNode* sentHashTable[DYMO_SENT_HASH_TABLE];
    Int32 size;
} DymoRreqSentTable;


//-----------------------------------------------------------------------
// STRUCT         ::   DymoMemPollEntry
// DESCRIPTION    ::   DYMO IPv4/IPv6 Memory Pool
//-----------------------------------------------------------------------
typedef struct str_dymo_mem_poll
{
    DymoRouteEntry routeEntry ;
    struct str_dymo_mem_poll* next;
} DymoMemPollEntry;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoStats
// DESCRIPTION    ::    DYMO IPv4/IPv6 Structure to store the
//                statistical information.
//-----------------------------------------------------------------------
typedef struct
{
    UInt32 numRequestInitiated;
    UInt32 numRequestRetried;
    UInt32 numRequestRelayed;

    UInt32 numRequestRecved;
    UInt32 numRequestDuplicate;
    UInt32 numRequestTtlExpired;
    UInt32 numRequestRecvedAsTargt;

    UInt32 numReplyInitiatedAsTargt;
    UInt32 numReplyInitiatedAsIntermediate;
    UInt32 numReplyForwarded;
    UInt32 numGratReplySent;

    UInt32 numReplyRecved;
    UInt32 numReplyRecvedAsTargt;

    UInt32 numHelloSent;
    UInt32 numHelloRecved;

    UInt32 numRerrInitiated;
    UInt32 numRerrForwarded;
    UInt32 numRerrRecved;
    UInt32 numRerrDiscarded;

    UInt32 numDataInitiated;
    UInt32 numDataForwarded;
    UInt32 numDataRecved;

    UInt32 numDataDroppedForNoRoute;
    UInt32 numDataDroppedForBufferOverflow;
    UInt32 numMaxHopExceed;
    UInt32 numHops;

    UInt32 numBrokenLinks;     //total number of broken links
    UInt32  numMaxSeenTable;   // Added to track table's max entries
    UInt32  numLastFoundHits;
} DymoStats;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoInterfaceInfo
// DESCRIPTION    ::    DYMO IPv4/IPv6 InterfaceInfo also used for dual
//                        IP case.
//-----------------------------------------------------------------------
typedef struct str_dymo_interface_info
{
    Address address;
    UInt32 ip_version;
    BOOL dymo4eligible;
    BOOL dymo6eligible;
} DymoInterfaceInfo;


//-----------------------------------------------------------------------
// STRUCT         ::    DymoData
// DESCRIPTION    ::    DYMO IPv4/IPv6 structure to store all necessary
//                      information.
//-----------------------------------------------------------------------
typedef struct struct_network_dymo_str
{
    // set of user configurable parameters
    Int32        hopLimit;
    clocktype    nodeTraversalTime;
    clocktype    deleteRouteTimeout;
    Int32        allowedHelloLoss;
    clocktype    UsedRouteTimeout;    
    
    Int32        rreqRetries;
    clocktype    helloInterval;

    RandomSeed  dymoJitterSeed;

    // set of Dymo protocol dependent parameters
    DymoRoutingTable routeTable;
    DymoRreqSentTable sent;
    DymoRreqSeenTable seenTable;
    DymoMessageBuffer msgBuffer;

    Int32 bufferSizeInNumPacket;
    Int32 bufferSizeInByte;

    DymoStats* stats;
    BOOL statsCollected;
    BOOL statsPrinted;
    BOOL processHello;
    
    Int32       ttlStart;
    Int8        msgttl;
    Int32  ttlIncrement;
    Int32  ttlMax;

    UInt16  seqNumber;
    UInt16  msgSeqId;
    clocktype lastBroadcastSent;

     // Performance Issue
    DymoMemPollEntry* freeList;

    BOOL isExpireTimerSet;
    BOOL isDeleteTimerSet;

    // Point to DYMO for IPv6 data if the node is dual IP
    struct_network_dymo_str* dymo6;
    int mainInterface;
    Address mainInterfaceAddr;
    DymoInterfaceInfo* intface;
    Address multicastAddr;

    // Gateway variable
    BOOL isGatewayEnabled;
    UInt8 gatewayPrfixLength;
    
    BOOL Dflag;     
    BOOL Aflag;
    BOOL Iflag;
    BOOL Eflag;
}DymoData;


void DymoInit(
    Node *node,
    DymoData **DymoPtr,
    const NodeInput *nodeInput,
    Int32 interfaceIndex,
    NetworkRoutingProtocolType dymoProtocolType);


void DymoFinalize(
    Node *node,
    Int32 interfaceId,
    NetworkType networkType);

void
Dymo4MacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const NodeAddress nextHopAddress,
    const int incommingInterfaceIndex);

void
Dymo6MacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const in6_addr nextHopAddress,
    const int incommingInterfaceIndex);

void DymoHandleProtocolPacket(
    Node *node,
    Message *msg,
    Address srcAddr,
    Address targtAddr,
    Int32 interfaceIndex);


void
DymoHandleProtocolEvent(
    Node *node,
    Message *msg);


void DymoInitiateRREQ(
    Node* node,
    Address targtAddr);


void DymoHandleRequest(
    Node* node,
    Message* msg,
    Address srcAddr,
    Int8 ttl,
    Int32 interfaceIndex);


void
Dymo4RouterFunction(
    Node* node,
    Message* msg,
    NodeAddress targtAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted);


BOOL DymoIsPrefixMatch(
    Address* targtAddr,
    Address* gateWayAddr,
    UInt8 prefixLength);

void DymoGetTunnelTypeAndInterface(
    Node* node,
    const NodeInput* nodeInput,
    DymoData* dymo);

void
Dymo6RouterFunction(
    Node* node,
    Message* msg,
    in6_addr targtAddr,
    in6_addr previousHopAddress,
    BOOL* packetWasRouted);
//--------------------------------------------------------------------------
// FUNCTION   :: RoutingDymoHandleChangeAddressEvent
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
void RoutingDymoHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);

#endif// Header file DYMO_H

