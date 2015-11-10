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
//-------------------------------------------------------------------------
#ifndef EIGRP_H
#define EIGRP_H

#include "list.h"
#include "buffer.h"
#include "network_access_list.h"

#define MAX_NUM_OF_TOS                        1
#define DIRECTLY_CONNECTED_NETWORK            0
#define DEFAULT_TOS                           1
#define MAX_ANTICIPATED_SIZE                  8
#define EIGRP_HEADER_SIZE                     sizeof(EigrpHeader)
#define EIGRP_HOLD_TIME_SIZE                  sizeof(unsigned int)
#define EIGRP_MAX_ROUTE_ADVERTISED            83
#define EIGRP_MAX_CACHE_SIZE                  8
#define EIGRP_VERSION                         1
#define EIGRP_DESTINATION_INACCESSABLE        0xFFFFFFFF
#define EIGRP_MAX_HOP_COUNT                   15
#define EIGRP_SLEEP_TIME                      (5 * SECOND)
#define HELLO_INTERVAL_ON_LOW_BANDWIDTH_LINK  (60 * SECOND)
#define HELLO_INTERVAL_ON_HIGH_BANDWIDTH_LINK (5 * SECOND)
#define EIGRP_DEFAULT_SLEEP_TIME              (5 * SECOND)
#define EIGRP_ROUTER_MULTICAST_ADDRESS        0xE000000A /* 224.0.0.10 */
#define EIGRP_MIN_SEQ_NUM                     1
#define ONE_HOP_TTL                           1
#define MAX_QUERY_PACKET_SIZE                 256
#define EIGRP_DEFAULT_HOLD_TIMER  (3 * \
        HELLO_INTERVAL_ON_LOW_BANDWIDTH_LINK)
#define EIGRP_NULL0 ANY_INTERFACE


#define EigrpGetOpcode(msg) ((EigrpHeader*) msg->packet)->opcode
#define EigrpSetNeighborState(x, y) ((x)->state = (y))
#define GET_IP_LAYER(node)  ((NetworkDataIp*) node->networkData.networkVar)
#define GET_EIGRP_ACCESS_LIST_NUMBERED(x) ((char*)(&(x)))
#define GET_EIGRP_ACCESS_LIST_NAMED(x) ((char*)(x))

#define SET_DEFAULT_ROUTE_FLAG(x, y)  \
    (((EigrpTopologyTableEntry*) (x->data))->isDefaultRoute = y)

#define SET_SUMMARY_FLAG(x, y) \
    (((EigrpTopologyTableEntry*) (x->data))-> \
    isSummary = y)

#define SET_DEFINED_IN_EIGRP_PROCESS_FLAG(x, y) \
    (((EigrpTopologyTableEntry*) (x->data))-> \
    isDefinedInEigrpProcess = y)

// Define type of access-list supported by EIGRP.
// "EIGRP_ACCESS_LIST_NONE" is kept only for debugging purpose.
typedef enum
{
    EIGRP_ACCESS_LIST_NONE     = 0,
    EIGRP_ACCESS_LIST_NUMBERED = 1,
    EIGRP_ACCESS_LIST_NAMED    = 2
} EigrpAccessListType;


// Define eigrp external route  flag type that will be used in
// "flags" field in "EigrpExternMetric" structure.
typedef enum
{
    EIGRP_DEFAULT_ROUTE_FLAG = 02,
    EIGRP_EXTERN_ROUTE_FLAG  = 01,
} EigrpExternRouteFlagType;

// Define external protocol ID constants used by EIGRP
typedef enum
{
    EIGRP_EXTERN_IGRP  = 0x01,
    EIGRP_EXTERN_EIGRP = 0x02,
    EIGRP_EXTERN_RIP   = 0x04,
    EIGRP_EXTERN_OSPF  = 0x06,
    EIGRP_EXTERN_BGP   = 0x09
} EigrpExternProtocolId;


// Eigrp uses five opcode types
typedef enum
{
    EIGRP_ANY_OPCODE          = -1, // Used in erronious conditions.
    EIGRP_UPDATE_PACKET       =  1,
    EIGRP_QUERY_PACKET        =  3,
    EIGRP_REPLY_PACKET        =  4,
    EIGRP_HELLO_OR_ACK_PACKET =  5,
    EIGRP_REQUEST_PACKET
} EigrpOpcodeType;


typedef enum
{
    EIGRP_INTERNAL_ROUTES  = 0,
    EIGRP_SYSTEM_ROUTE     = 1,
    EIGRP_EXTERIOR_ROUTE   = 2
} EigrpRouteType;


// State of the Distributed Update ALgorithm (DUAL)
typedef enum
{
    DUAL_ACTIVE_STATE  = 0,
    DUAL_PASSIVE_STATE = 1
} EigrpDualStateType;


// Eigrp neighbor state
typedef enum
{
    EIGRP_NEIGHBOR_OFF               = 0,
    EIGRP_NEIGHBOR_CONNECTION_OPENED = 1,
    EIGRP_NEIGHBOR_ACTIVE            = 2
} NeighborState;


// Define EIGRP TLV type
typedef enum
{
    EIGRP_NULL_TLV_TYPE            = 0x0000,
    EIGRP_HELLO_HOLD_TIME_TLV_TYPE = 0x0001,
    EIGRP_INTERNAL_ROUTE_TLV_TYPE  = 0x0102,
    EIGRP_SYSTEM_ROUTE_TLV_TYPE    = 0x0102,
    EIGRP_EXTERN_ROUTE_TLV_TYPE    = 0x0103
} EigrpTLVType;


// Define EIGRP flag type(s). This is used
// in "flags" field of Eigrp header.
typedef enum
{
    EIGRP_FLAG_INITALLIZATION_BIT_ON  = 0x000001,
    EIGRP_FLAG_INITALLIZATION_BIT_OFF = 0x000000
} EigrpInitFlagType;


typedef enum
{
    EIGRP_QUERY_ORIGIN  = 0,
    EIGRP_QUERY_FORWORD = 1
} EigrpQueryPacketType;


typedef enum
{
    EIGRP_STUCK_OFF = 0,
    EIGRP_STUCK_SET = 1,
    EIGRP_STUCK_CLS = 2
}EIGRP_STUCK_STATUS;

// Eigrp metric structure
typedef struct
{
    NodeAddress    nextHop;            // Next hop Address
    unsigned int   delay;              // delay in tens of Micro Second
    unsigned int   bandwidth;          // Inverse Bandwidth
    unsigned char  mtu[3];             // MTU
    unsigned char  hopCount;           // Hop Count
    unsigned short reliability;        // Reliability (not used)
    unsigned char  load;               // Load
    unsigned char  reserved;           // Reserved field (not used)
    unsigned char  ipPrefix;           // IP prefix
    unsigned char  destination[3];     // Three most significant bytes
                                       // of IP address of destination
} EigrpMetric;


// Define EIGRP external metric structure.
typedef struct
{
    NodeAddress    nextHop;                // Next hop Address.

    NodeAddress    originatingRouter;      // Address of originating router.
    unsigned int   originitingAs;          // The originating AS.
    NodeAddress    arbitraryTag;           // used in route tagging.
    unsigned int   externalProtocolMetric; // external protocol identifier.
    unsigned int   externReserved;         // Reserved (not used).
    unsigned char  externProtocolId;       // External protocol Id
    unsigned char  flags;                  // Flag that distingushes default
                                           // and external route.

    unsigned int   delay;                  // delay in tens of Micro Second.
    unsigned int   bandwidth;              // Inverse Bandwidth
    unsigned char  mtu[3];                 // MTU
    unsigned char  hopCount;               // Hop Count
    unsigned short reliability;            // Reliability (not used)
    unsigned char  load;                   // Load
    unsigned char  reserved;               // Reserved field (not used)
    unsigned char  ipPrefix;               // IP prefix
    unsigned char  destination[3];         // Three most significant bytes
                                           // of IP address of destination
} EigrpExternMetric;


typedef struct
{
    NodeAddress    originatingRouter;      // Address of originating router.
    unsigned int   originitingAs;          // The originating AS.
    NodeAddress    arbitraryTag;           // used in route tagging.
    unsigned int   externalProtocolMetric; // external protocol identifier.
    unsigned int   externReserved;         // Reserved (not used).
    unsigned char  externProtocolId;       // External protocol Id
    unsigned char  flags;                  // Flag that distingushes default
                                           // and external route.
} EigrpExtraTopologyMetric;

//----------------------------------------------------------------------
// Eigrp Topolopgy Table Entry
// The topology table contains all
// destinations advertised by neighboring routers.
//----------------------------------------------------------------------
typedef struct
{
    EigrpMetric metric;
    unsigned short asId;
    unsigned char  entryType;
    int outgoingInterface;
    float reportedDistance;
    float feasibleDistance;
    BOOL isOptimum;

    BOOL isDefaultRoute;
    EigrpExtraTopologyMetric* eigrpExtraTopologyMetric;

    BOOL isSummary;
    BOOL isDefinedInEigrpProcess;

} EigrpTopologyTableEntry;


// Eigrp Routing Table Entry
typedef struct
{
    ListItem* feasibleRoute;           // This is  the pointer to
                                       // Eigrp Topology Table Entry
                                       // pointing to a feasible route
    int outgoingInterface;
    unsigned numOfPacketSend;
    clocktype lastUpdateTime;
} EigrpRoutingTableEntry;


// Eigrp routing table
typedef struct
{
    int tos;  // The type of service

    int k1;   // k1,k2,k3 and k4 are tos parameters that
    int k2;   // are read from the configuration file
    int k3;   // for each type of service  given
    int k4;
    int k5;
    DataBuffer eigrpRoutingTableEntry;
} EigrpRoutingTable;


typedef struct
{
   int tableIndex;
   NodeAddress neighborAddress;
} NeighborSearchInfo;


// Create a neighborinfo structure...
typedef struct eigrp_neighbor_info_struct
{
    NodeAddress neighborAddr;             // IP address of the neighbor(s)

    clocktype neighborHoldTime;           // This variable is used to hold
                                          // the value of hold-time
                                          // advertised by an adjacent
                                          // neighbor.

    BOOL isHelloOrAnyOtherPacketReceived; // Is hello packet received
                                          // from the within hello interval?

    NeighborState state;                  // State of the neighbor.
                                          // Packet sequence number.
    struct eigrp_neighbor_info_struct *next;
} EigrpNeighborInfo;


// Eigrp neighbor table structure.
typedef struct
{
    EigrpNeighborInfo* first;
    EigrpNeighborInfo* last;

    unsigned int seqNo;                   // Packet sequence number.
    clocktype helloInterval;              // My own hello interval period.
    clocktype holdTime;                   // My own holdtime value
} EigrpNeighborTable;


// EIGRP Query cache struct.
typedef struct
{
    NodeAddress nodeAddress;
    unsigned int queryId;
} EigrpQueryEntry;


// EIGRP Query cache.
typedef struct
{
    EigrpQueryEntry* queryCache;
    unsigned int numEntries;
    unsigned int maxEntries;
} EigrpQueryCache;


// Eigrp statistics structure.
typedef struct
{
    BOOL isStatPrinted;
    unsigned int numHelloSend;
    unsigned int numHelloReceived;
    unsigned int numTriggeredUpdates;
    unsigned int numUpdateReceived;
    unsigned int numQuerySend;
    unsigned int numQueryReceived;
    unsigned int numResponsesSend;
    unsigned int numResponsesReceived;
} EigrpStat;


// Define EIGRP filter Structure. This will be used to
// keep track of individual ACL parameters.
typedef struct eigrp_filter_list_struct
{
    EigrpAccessListType type;
    AccessListInterfaceType aclInterface;
    int interfaceIndex;

    char* ACLname;           // for ACL named
    unsigned int ACLnumber;  // for ACL numbered

    struct eigrp_filter_list_struct* prev;
    struct eigrp_filter_list_struct* next;
} EigrpFilterList;


// Define EIGRP filter-list table.
// This will be used as a table of ACL parameters.
typedef struct
{
    unsigned int numList;
    EigrpFilterList* first;
    EigrpFilterList* last;
} EigrpFilterListTable;
//


// Routing Eigrp structure.
typedef struct
{
    EigrpNeighborTable* neighborTable; // pointer to eigrp neighbor table
    DataBuffer eigrpRoutingTable;      // eigrp routing table
    LinkedList* topologyTable;               // eigrp topology table


    BOOL isRouteFilterEnabled;
    EigrpFilterListTable eigrpFilterListTable;

    BOOL isSummaryOn;

    unsigned char edition;             // eigrp version
    float variance;                    // EIGRP varience
    short asId;                        // the as Id
    unsigned char maxHop;              // maximum number of hops

    clocktype ackTime;
    clocktype waitTime;
    clocktype sleepTime;
    clocktype lastUpdateSent;

    BOOL splitHorizonEnabled;          // is split Horizon enabled ?
    BOOL poisonReverseEnabled;         // is posionReverse enabled ?
    BOOL isNextUpdateScheduled;        // is next update scheduled ?
    BOOL isQueryScheduled;             // is query message scheduled ?
    BOOL collectStat;                  // is statistics collection on ?
    EigrpStat* eigrpStat;              // pointer to eigrp stat struct.

    int* isStuckTimerSet;

    RandomSeed seed;
} RoutingEigrp;


// Eigrp TLV encoding structure.
typedef struct
{
    EigrpTLVType   type;               // TLV type see EigrpTLVType
    unsigned short length;             // Size of TLV in bytes.
} EigrpTlv;


// Eigrp header format
typedef struct
{
    unsigned char  version;            // Eigrp version number
    unsigned char  opcode;             // Eigrp Opcode Type
    unsigned short checksum;           // Checksum
    unsigned int   flags;              // Eigrp flags
    unsigned short sequenceNumber;     // Packet sequence number
    unsigned short asNum;              // AS number
} EigrpHeader;


// Eigrp update message structure
typedef struct
{
    EigrpMetric  metric;
} EigrpUpdateMessage;


// Define Eigrp Reply message and Query message structure
// ASSUMPTION : They assumed to have same format as
//              update message.
typedef EigrpUpdateMessage EigrpReplyMessage;


// Define Eigrp query message type
typedef struct
{
    unsigned char destination[3]; // Destination address to be quired.
    unsigned char ipPrefix;       // IP prefix
} EigrpQueryMessage;

//-------------------------------------------------------------------------
// FUNCTION     : EigrpInit
//
// PURPOSE      : initializing EIGRP protocol.
//
// PARAMETERS   : node - the node which is initializing
//                eigrp - the routing eigrp structure
//                nodeInput - the input configuration file
//                interfaceIndex - interface index.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void EigrpInit(
    Node* node,
    RoutingEigrp** eigrp,
    const NodeInput* nodeInput,
    int interfaceIndex);


//-------------------------------------------------------------------------
// FUNCTION     : EigrpHandleProtocolEvent()
//
// PURPOSE      : Handling protocol related timer events
//
// PARAMETERS   : node - node which is handling the packet
//                msg - the event it is handling
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void EigrpHandleProtocolEvent(
    Node* node,
    Message* msg);


//-------------------------------------------------------------------------
// FUNCTION     : EigrpHandleProtocolPacket()
//
// PURPOSE      : Handling protocol related control packets
//
// PARAMETERS   : node - node which is handling the packet
//                msg - the packet it is handling
//                sourceAddr - immediate next hop from which
//                             message being received
//                interfaceIndex - the interface index through which
//                                 the message is being received.
//                previousHopAddress - The previous Hop Address
// RETURN VALUE : void
//-------------------------------------------------------------------------
void EigrpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddr,
    int interfaceIndex,
    NodeAddress previousHopAddress);


//-------------------------------------------------------------------------
// FUNCTION     : EigrpFinalize()
//
// PURPOSE      : Printing eigrp statistics
//
// PARAMETERS   : node - node which is printing statistics
//
// RETURN VALUE : none
//-------------------------------------------------------------------------
void EigrpFinalize(Node* node);

//-------------------------------------------------------------------------
// FUNCTION     : EigrpRouterFunction()
//
// PURPOSE      : Snoop at the eigrp reply packet and updating its metric
//                if the packet is the reply packet.
//
// PARAMETERS   : node            - node which is routing the packet
//                msg             - the data packet
//                destAddr        - the destination node of this data packet
//                previouHopAddress - notUsed
//                PacketWasRouted   - variable that indicates to IP that it
//                                    no longer needs to act on this
//                                    data packet
//
//-------------------------------------------------------------------------
void EigrpRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previouHopAddress,
    BOOL* packetWasRouted);

//--------------------------------------------------------------------------
// FUNCTION     : EigrpInterfaceStatusHandler()
//
// PURPOSE      : Handling Mac layer interface status information
//
// PARAMETERS   : node - node which is handling information
//                interfaceIndex - interface Index who's status is being
//                                 handled.
//                state - state ACTIVE of FAILED
//
// RETURN VALUE : none
//--------------------------------------------------------------------------
void EigrpInterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state);

// Hooking function for Route Redistribution
void EigrpHookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* eigrpMetric);
#endif
