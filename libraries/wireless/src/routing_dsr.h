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

#ifndef DSR_H
#define DSR_H

// Draft specified symbolic constants
#define DSR_BROADCAST_JITTER            (100 * MILLI_SECOND)

#define DSR_MAX_REQUEST_PERIOD          (10 * SECOND)

#define DSR_REQUEST_PERIOD              (500 * MILLI_SECOND)

#define DSR_ROUTE_CACHE_TIMEOUT         (300 * SECOND)

#define DSR_SEND_BUFFER_TIMEOUT         (30 * SECOND)

#define DSR_REQUEST_TABLE_SIZE           64

#define DSR_REQUEST_TABLE_IDS            16

#define DSR_MAX_REQUEST_REXMT            16

#define DSR_NON_PROP_REQUEST_TIMEOUT    (30 * MILLI_SECOND)

#define DSR_REXMT_BUFFER_SIZE            50

#define DSR_MAIN_THOLDOFF_TIME          (250 * MILLI_SECOND)

#define DSR_MAX_MAIN_TREXMT              2

#define DSR_TRY_PASSIVE_ACKS             1

#define DSR_PASSIVE_ACK_TIMEOUT         (100 * MILLI_SECOND)

#define DSR_GRAT_REPLY_HOLDOFF          (1 * SECOND)

#define DSR_MAX_SALVAGE_COUNT            15

#define DSR_NON_PROPAGATING_TTL          1

#define DSR_PROPAGATING_TTL              65

#define DSR_NO_MORE_HEADER               0

#define DSR_ROUTE_TIMER                  (30 * SECOND)

#define DSR_MEM_UNIT                     100

#define DSR_ROUTE_HASH_TABLE_SIZE        100

#define DSR_REPLY_HASH_TABLE_SIZE        20

#define DSR_EXPANDING_RING               0

typedef enum
{
    DSR_FORWARD_DIRECTION,
    DSR_BACKWARD_DIRECTION,
    DSR_BOTH_DIRECTION
} DsrPathDirection;

// Dsr packet formats according to draft-ietf-manet-dsr-07.txt


// Fixed header format of Dsr
typedef struct
{
    unsigned char nextHeader;
    unsigned char reserved;
    unsigned short payloadLength;
    // here should come variable length options
} DsrHeader;

#define DSR_SIZE_OF_HEADER 4


// Dsr option formats: Possible options are
// Route Request Option (2)
// Route Reply Option (3)
// Route Error Option (4)
// Acknowledgement Request Option (5)
// Acknowledgement Option (6)
// Dsr Source Route Option (7)
// Pad 1 Option (0)
// Pad N Option (1)
typedef enum
{
    DSR_PAD_1 = 0,
    DSR_PAD_N = 1,
    DSR_ROUTE_REQUEST = 2,
    DSR_ROUTE_REPLY = 3,
    DSR_ROUTE_ERROR = 4,
    DSR_ACK_REQUEST = 5,
    DSR_ACK = 6,
    DSR_SRC_ROUTE = 7
} DsrPacketType;

// Format of route request option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    unsigned short identification;
    NodeAddress   targetAddr;
    // After this it contains a path list
} DsrRouteRequest;

#define DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH 8

#define DSR_SIZE_OF_ROUTE_REQUEST_WITHOUT_PATH_TYPE_LEN 6

// Format of route reply option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    unsigned char dsrRrep;//L : 1,
                  //reserved : 7;
    // After this it contains a path list
} DsrRouteReply;

//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteReplySetL()
// PURPOSE      : Set the value of L for DsrRouteReply
// ARGUMENTS    : dsrRrep, The variable containing the value of reserved and
//                         L
//                L, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrRouteReplySetL(unsigned char *dsrRrep, BOOL L)
{
    unsigned char L_char = (unsigned char)L;

    //masks L within boundry range
    L_char = L_char & maskChar(8, 8);

    //clears the first bit
    *dsrRrep = *dsrRrep & maskChar(2, 8);

    //setting the value of L in dsrRrep
    *dsrRrep = *dsrRrep | LshiftChar(L_char, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteReplySetResv()
// PURPOSE      : Set the value of reserved for DsrRouteReply
// ARGUMENTS    : dsrRrep, The variable containing the value of reserved and
//                         L
//                reserved, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrRouteReplySetResv(unsigned char *dsrRrep,
                                 unsigned char reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskChar(2, 8);

    //clears last seven bits
    *dsrRrep = *dsrRrep & maskChar(1, 1);

    //setting the value of reserved in dsrRrep
    *dsrRrep = *dsrRrep | reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteReplyGetL()
// PURPOSE      : Returns the value of L for DsrRouteReply
// ARGUMENTS    : dsrRrep, The variable containing the value of reserved and
//                         L
// RETURN       : BOOL
//-------------------------------------------------------------------------
static BOOL DsrRouteReplyGetL(unsigned char dsrRrep)
{
    unsigned char L = dsrRrep;

    //clears last 7 bits
    L = L & maskChar(1, 1);

    //right shift 7 bits
    L = RshiftChar(L, 1);

    return (BOOL)L;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteReplyGetResv()
// PURPOSE      : Returns the value of reserved for DsrRouteReply
// ARGUMENTS    : dsrRrep, The variable containing the value of reserved and
//                         L
// RETURN       : unsigned char
//-------------------------------------------------------------------------
static unsigned char DsrRouteReplyGetResv(unsigned char dsrRrep)
{
    unsigned char reserved = dsrRrep;

    //clears first bit
    reserved = reserved & maskChar(2, 8);

    return reserved;
}


#define DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH 3

#define DSR_SIZE_OF_ROUTE_REPLY_WITHOUT_PATH_TYPE_LEN 1

// Format of route error option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    unsigned char errorType;
    unsigned char dsrErr;//reserved : 4,
                  //salvage : 4;
    NodeAddress   errSrcAddr;
    NodeAddress   errDestAddr;
    // After this it contains unreachable dest list
} DsrRouteError;

//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteErrorSetResv()
// PURPOSE      : Set the value of reserved for DsrRouteError
// ARGUMENTS    : dsrErr, The variable containing the value of reserved and
//                        salvage
//                reserved, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrRouteErrorSetResv(unsigned char *dsrErr,
                                 unsigned char reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskChar(5, 8);

    //clears first four bits
    *dsrErr = *dsrErr & maskChar(5, 8);

    //setting the value of reserved in dsrErr
    *dsrErr = *dsrErr | LshiftChar(reserved, 4);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteErrorSetSalvage()
// PURPOSE      : Set the value of salvage for DsrRouteError
// ARGUMENTS    : dsrErr, The variable containing the value of reserved and
//                        salvage
//                salvage, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrRouteErrorSetSalvage(unsigned char *dsrErr,
                                    unsigned char salvage)
{
    //masks salvage within boundry range
    salvage = salvage & maskChar(5, 8);

    //clears last four bits
    *dsrErr = *dsrErr & maskChar(1, 4);

    //setting the value of salvage in dsrErr
    *dsrErr = *dsrErr | salvage;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteErrorGetResv()
// PURPOSE      : Returns the value of reserved for DsrRouteError
// ARGUMENTS    : dsrErr, The variable containing the value of reserved and
//                        salvage
// RETURN       : unsigned char
//-------------------------------------------------------------------------
static unsigned char DsrRouteErrorGetResv(unsigned char dsrErr)
{
    unsigned char reserved = dsrErr;

    //clears last 4 bits
    reserved = reserved & maskChar(1, 4);

    //right shift 4 bits
    reserved = RshiftChar(reserved, 4);

    return reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrRouteErrorGetSalvage()
// PURPOSE      : Returns the value of salvage for DsrRouteError
// ARGUMENTS    : dsrErr, The variable containing the value of reserved and
//                        salvage
// RETURN       : unsigned char
//-------------------------------------------------------------------------
static unsigned char DsrRouteErrorGetSalvage(unsigned char dsrErr)
{
    unsigned char salvage = dsrErr;

    //clears first 4 bits
    salvage = salvage & maskChar(5, 8);

    return salvage;
}



#define DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_PATH_TYPE_LEN 10

#define DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_PATH 12

#define DSR_SIZE_OF_ROUTE_ERROR_WITHOUT_TYPE_LEN 14

#define DSR_ERROR_NODE_UNREACHABLE 1

#define DSR_SIZE_OF_ROUTE_ERROR 16

// Format of acknowledgement request option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    unsigned short identification;
} DsrAckRequest;

// Format of Acknowledgement option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    unsigned short identification;
    NodeAddress ackSrcAddr;
    NodeAddress ackDestAddr;
} DsrAck;

// Format of source route option.
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    UInt16 dsrSr;  //F : 1,
                   //L : 1,
                   //reserved : 4,
                   //salvage : 4,
                   //segLeft : 6;
    // After this it contains path list
} DsrSourceRoute;

//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteSetF()
// PURPOSE      : Set the value of F for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
//                F, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrSourceRouteSetF(UInt16 *dsrSr, UInt32 F)
{
    UInt16 F_short = (UInt16)F;

    //masks F within boundry range
    F_short = F_short & maskShort(16, 16);

    //clears the first bit
    *dsrSr = *dsrSr & maskShort(2, 16);

    //setting the value of F_short in dsrSr
    *dsrSr = *dsrSr | LshiftShort(F_short, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteSetL()
// PURPOSE      : Set the value of L for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
//                L, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrSourceRouteSetL(UInt16 *dsrSr, UInt32 L)
{
    UInt16 L_short = (UInt16)L;

    //masks L within boundry range
    L_short = L_short & maskShort(16, 16);

    //clears the second bit
    *dsrSr = *dsrSr & (~(maskShort(2, 2)));

    //setting the value of L_short in dsrSr
    *dsrSr = *dsrSr | LshiftShort(L_short, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteSetResv()
// PURPOSE      : Set the value of reserved for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
//                reserved, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrSourceRouteSetResv(UInt16 *dsrSr, UInt32 reserved)
{
    UInt16 reserved_short = (UInt16)reserved;

    //masks reserved within boundry range
    reserved_short = reserved_short & maskShort(13, 16);

    //clears the 3-6 bit
    *dsrSr = *dsrSr & (~(maskShort(3, 6)));

    //setting the value of reserved_short in dsrSr
    *dsrSr = *dsrSr | LshiftShort(reserved_short, 6);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteSetSalvage()
// PURPOSE      : Set the value of salvage for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
//                salvage, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrSourceRouteSetSalvage(UInt16 *dsrSr, UInt32 salvage)
{
    UInt16 salvage_short = (UInt16)salvage;

    //masks salvage within boundry range
    salvage = salvage & maskShort(13, 16);

    //clears the 7-10 bit
    *dsrSr = *dsrSr & (~(maskShort(7, 10)));

    //setting the value of salvage_short in dsrSr
    *dsrSr = *dsrSr | LshiftShort(salvage_short, 10);
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteSetSegLeft()
// PURPOSE      : Set the value of segLeft for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
//                segLeft, Input value for set operation
// RETURN       : void
//-------------------------------------------------------------------------
static void DsrSourceRouteSetSegLeft(UInt16 *dsrSr, UInt32 segLeft)
{
    UInt16 segLeft_short = (UInt16)segLeft;

    //masks segLeft within boundry range
    segLeft = segLeft & maskShort(11, 16);

    //clears the 7-10 bit
    *dsrSr = *dsrSr & (~(maskShort(11, 16)));

    //setting the value of segLeft_short in dsrSr
    *dsrSr = *dsrSr | segLeft_short;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteGetF()
// PURPOSE      : Returns the value of F for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
// RETURN       : UInt32
//-------------------------------------------------------------------------
static UInt32 DsrSourceRouteGetF(UInt16 dsrSr)
{
    UInt16 F = (UInt16)dsrSr;

    //clears all the bits except first
    F = F & maskShort(1, 1);

    //right shifts so that last bit represents F
    F = RshiftShort(F, 1);

    return (UInt32)F;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteGetL()
// PURPOSE      : Returns the value of L for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
// RETURN       : UInt32
//-------------------------------------------------------------------------
static UInt32 DsrSourceRouteGetL(UInt16 dsrSr)
{
    UInt16 L = (UInt16)dsrSr;

    //clears all the bits except second
    L = L & maskShort(1, 1);

    //right shifts so that last bit represents L
    L = RshiftShort(L, 1);

    return (UInt32)L;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteGetResv()
// PURPOSE      : Returns the value of reserved for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
// RETURN       : UInt32
//-------------------------------------------------------------------------
static UInt32 DsrSourceRouteGetResv(UInt16 dsrSr)
{
    UInt16 reserved = (UInt16)dsrSr;

    //clears all the bits except 3-6
    reserved = reserved & maskShort(3, 6);

    //right shifts so that last 4 bits represents resv
    reserved = RshiftShort(reserved, 6);

    return (UInt32)reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteGetSalvage()
// PURPOSE      : Returns the value of salvage for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
// RETURN       : UInt32
//-------------------------------------------------------------------------
static UInt32 DsrSourceRouteGetSalvage(UInt16 dsrSr)
{
    UInt16 salvage = (UInt16)dsrSr;

    //clears all the bits except 7-10
    salvage = salvage & maskShort(7, 10);

    //right shifts so that last 4 bits represents salvage
    salvage = RshiftShort(salvage, 10);

    return (UInt32)salvage;
}


//-------------------------------------------------------------------------
// FUNCTION     : DsrSourceRouteGetSegLeft()
// PURPOSE      : Returns the value of segLeft for DsrSourceRoute
// ARGUMENTS    : dsrSr, The variable containing the value of F,L,reserved,
//                       salvage and segLeft
// RETURN       : UInt32
//-------------------------------------------------------------------------
static UInt32 DsrSourceRouteGetSegLeft(UInt16 dsrSr)
{
    UInt16 segLeft = (UInt16)dsrSr;

    //clears all the bits except 11,16
    segLeft = segLeft & maskShort(11, 16);

    return (UInt32)segLeft;
}


#define DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH 4

#define DSR_SIZE_OF_SRC_ROUTE_WITHOUT_PATH_TYPE_LEN 2

#define DSR_SEGS_LEFT_MASK 0x3f

#define DSR_SALVAGE_MASK 0x3c0

#define DSR_NUM_BITS_IN_SEG_LEFT 6

#define DSR_SIZE_OF_TYPE_LEN 2

// Format of pad1 option
typedef struct
{
    unsigned char optionType;
} DsrPad1;

// Format of padN option
typedef struct
{
    unsigned char optionType;
    unsigned char optDataLen;
    // After this it contains unsigned char byte
} DsrPadN;

#define DSR_SIZE_OF_PADN_WITHOUT_PAD 2

// Dsr route cache entry structures. This entry should be maintained
// for all known destinations
typedef struct str_dsr_route_cache_entry
{
    NodeAddress destAddr;
    int hopCount;           // Hop length to the destination

    NodeAddress* path;

    clocktype   routeEntryTime;
    struct str_dsr_route_cache_entry* prev;
    struct str_dsr_route_cache_entry* next;
    struct str_dsr_route_cache_entry* deletePrev;
    struct str_dsr_route_cache_entry* deleteNext;
} DsrRouteEntry;

// Dsr route cache
typedef struct
{
    DsrRouteEntry* hashTable[DSR_ROUTE_HASH_TABLE_SIZE];
    DsrRouteEntry* deleteListHead;
    DsrRouteEntry* deleteListTail;
    int count;               // Count of current entries
} DsrRouteCache;


typedef struct str_dsr_mem_poll
{
    DsrRouteEntry routeEntry ;
    struct str_dsr_mem_poll* next;
} DsrMemPollEntry;


// Dsr Gratuitous reply table. This table is used to limit number of
// of gratuitous reply sent for a destination.
typedef struct str_dsr_grat_reply_entry
{
    NodeAddress srcOfPacket;
    unsigned int prevHopOfPacket;
    clocktype remainingLifeTime;
} DsrGratReplyEntry;

// Dsr route cache
typedef struct
{
    DsrGratReplyEntry* head;
    DsrGratReplyEntry* last;
    int numEntries;          // Count of current entries
} DsrGratReplyTable;

// Dsr Request table
typedef enum
{
    ROUTING_DSR_ALL_ENTRY,
    ROUTING_DSR_SENT_ENTRY,
    ROUTING_DSR_SEEN_ENTRY
} DsrReqEntryType;

// Request list entry
typedef struct str_dsr_req_list_entry
{
    clocktype                       timeStamp;
    NodeAddress                     addr;
    DsrReqEntryType                 type;
    void*                           data;
    struct str_dsr_req_list_entry*  next;
    struct str_dsr_req_list_entry*  prev;
    struct str_dsr_req_list_entry*  LRUNext;
    struct str_dsr_req_list_entry*  LRUPrev;
} DsrRequestEntry;

typedef struct
{
    DsrRequestEntry* seenHashTable[DSR_REPLY_HASH_TABLE_SIZE];
    DsrRequestEntry* sentHashTable[DSR_REPLY_HASH_TABLE_SIZE];
    DsrRequestEntry* LRUListHead;
    DsrRequestEntry* LRUListTail;
    int count;
} DsrRequestTable;

// Dsr request sent entry structure. This should contain necessary
// informations
// about destinations for which a route discovery has been intiated.
typedef struct str_dsr_rreq_sent_entry
{
    clocktype backoffInterval;         // No additional Req for this time
    int ttl;                           // Last used ttl to send rreq
    int count;                         // Number of times rreq has been sent
} DsrSentEntry;

// Dsr request seen table. This should contain information about source and
// sequence number pair for each route request which came in the node for
// first time.
typedef struct str_dsr_rreq_seen_entry
{
    unsigned short seqNumber[DSR_REQUEST_TABLE_IDS];
    NodeAddress targetAddr[DSR_REQUEST_TABLE_IDS];
    int front;
    int rear;
    int size;
} DsrSeenEntry;


// Dsr message buffer entry. Buffer to temporarily store messages when there
// is no route for a destination.
typedef struct str_dsr_msg_buffer_entry
{
    NodeAddress destAddr;
    clocktype timeStamp;
    BOOL isErrorPacket;
    Message* msg;
    struct str_dsr_msg_buffer_entry* next;
} DsrBufferEntry;

// Dsr message buffer
typedef struct
{
    DsrBufferEntry* head;
    int sizeInPacket;
    int sizeInByte;
} DsrBuffer;

// Dsr message retransmit buffer entry. Buffer to temporarily store messages
// After sending the message out to destination, until an acknowledgement
// comes for the message.

typedef struct str_dsr_rexmt_buffer_entry
{
    NodeAddress destAddr; // destination for which the message has been sent
    NodeAddress srcAddr;
    NodeAddress nextHop;  // next hop to which the message has been sent
    unsigned int count;   // number of times retransmitted
    unsigned short msgId;
    clocktype timeStamp;  // when the message has been inserted
    Message* msg;         // sent message
    struct str_dsr_rexmt_buffer_entry* next;
} DsrRexmtBufferEntry;

// Dsr message buffer
typedef struct
{
    DsrRexmtBufferEntry* head; // pointer to the first entry
    int sizeInPacket;          // number of packets in the retransmit buffer
} DsrRexmtBuffer;

// Dsr statistical variables
typedef struct
{
    unsigned int numRequestInitiated;
    unsigned int numRequestResent;
    unsigned int numRequestRelayed;

    unsigned int numRequestRecved;
    unsigned int numRequestDuplicate;
    unsigned int numRequestTtlExpired;
    unsigned int numRequestRecvedAsDest;
    unsigned int numRequestInLoop;

    unsigned int numReplyInitiatedAsDest;
    unsigned int numReplyInitiatedAsIntermediate;

    unsigned int numReplyRecved;
    unsigned int numReplyRecvedAsSource;

    unsigned int numRerrInitiated;

    unsigned int numRerrRecvedAsSource;
    unsigned int numRerrRecved;

    unsigned int numDataInitiated;
    unsigned int numDataForwarded;

    unsigned int numDataRecved;

    unsigned int numDataDroppedForNoRoute;
    unsigned int numDataDroppedForOverlimit;
    unsigned int numDataDroppedRexmtTO;

    unsigned int numRoutes;
    unsigned int numHops;

    unsigned int numSalvagedPackets;
    unsigned int numLinkBreaks;

    unsigned int numPacketsGreaterMTU;
} DsrStats;


// Dsr main data structure
typedef struct
{
    DsrBuffer msgBuffer;           // Dsr messge buffer
    DsrRexmtBuffer rexmtBuffer;

    DsrRequestTable reqTable;

    DsrRouteCache routeCache;      // Dsr Route Cache

    DsrMemPollEntry* freeList;

    BOOL statsCollected;           // Whether or not to collect statistics
    BOOL statsPrinted;
    DsrStats stats;                // Dsr statistical variables

    unsigned short identificationNumber;      // Dsr Identification number

    int bufferMaxSizeInByte;       // Maximum size of message buffer
    int bufferMaxSizeInPacket;     // Maximum size of message buffer in
                                   // number of packets
    NodeAddress localAddress;      // 0th interface address

    BOOL isTimerSet;
    // For packet trace
    #define     ROUTING_DSR_TRACE_NO    0
    #define     ROUTING_DSR_TRACE_YES   1

    int trace;
    // End packet trace

    RandomSeed seed;
} DsrData;

// Function to initialize dsr routing protocol
void DsrInit(
         Node* node,
         DsrData** dsrPtr,
         const NodeInput* nodeInput,
         int interfaceIndex);

// Function to handle dsr protocol specific packets.
void DsrHandleProtocolPacket(
         Node* node,
         Message* msg,
         NodeAddress srcAddr,
         NodeAddress destAddr,
         int ttl,
         NodeAddress prevHop);

// Function to handle dsr internal messages such as timers
void DsrHandleProtocolEvent(Node* node, Message* msg);

// Function to handle data packets. Sometimes for incoming packets
// to update internal variables. Sometimes outgoing packets from nodes
// upperlayer or protocols
void DsrRouterFunction(
         Node* node,
         Message* msg,
         NodeAddress destAddr,
         NodeAddress previousHopAddress,
         BOOL* packetWasRouted);

// Function to print statistical variables at the end of simulation.
void DsrFinalize(Node* node);

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingDsrHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node* : Pointer to Node structure
// + interfaceIndex          : Int32 : interface index
// + old_addr              : Address* old address
// + NetworkType networkType : type of network protocol
// RETURN :: void : NULL
//--------------------------------------------------------------------------
void RoutingDsrHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* old_addr,
    NodeAddress subnetMask,
    NetworkType networkType);
#endif

