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


#ifndef BGP_H
#define BGP_H

#include "buffer.h"

#include "types.h"
#include "partition.h"

#define BGPVERSION                         0x04
#define BGP_RANDOM_TIMER                   2 * MICRO_SECOND

// All these are suggested value for the timers according to rfc 1772
// section 6.4
#define BGP_DEFAULT_CONNECT_RETRY_TIMER        (120 * SECOND)
#define BGP_DEFAULT_KEEP_ALIVE_TIMER           (30 * SECOND)
#define BGP_DEFAULT_HOLD_TIME                  (90 * SECOND)
#define BGP_DEFAULT_START_TIME                 (0 * SECOND)

#define BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_EBGP (30 * SECOND)//30
#define BGP_DEFAULT_MIN_RT_ADVERTISEMENT_TIMER_IBGP (5 * SECOND)//5
#define BGP_DEFAULT_MIN_AS_ORIGINATION_TIMER   (15 * SECOND)//15

#define BGP_DEFAULT_LARGE_HOLD_TIME            (4 * MINUTE)
#define BGP_DEFAULT_ROUTE_WAITING_TIME         (15 * SECOND)
#define BGP_DEFAULT_START_EVENT_TIME           (60 * SECOND)
#define BGP_DEFAULT_IGP_PROBE_TIME             (10 * MINUTE)

#define BGP_DEFAULT_LOCAL_PREF_VALUE        100
#define BGP_DEFAULT_MULTI_EXIT_DISC_VALUE   0

#define BGP_HOLD_INTERVAL_MIN_RANGE (3 * SECOND)
#define BGP_TIME_INTERVAL_MAX_RANGE (65535 * SECOND)

// Constants for BGP_PacketHeader.Type rfc 1771
#define BGP_OPEN               0x01
#define BGP_UPDATE             0x02
#define BGP_NOTIFICATION       0x03
#define BGP_KEEP_ALIVE         0x04


// Constants for Error codes in BGP Notification message rfc 1771
#define BGP_ERR_CODE_MSG_HDR_ERR                 0x01
#define BGP_ERR_CODE_OPEN_MSG_ERR                0x02
#define BGP_ERR_CODE_UPDATE_MSG_ERR              0x03
#define BGP_ERR_CODE_HOLD_TIMER_EXPIRED_ERR      0x04
#define BGP_ERR_CODE_FINITE_STATE_MACHINE_ERR    0x05
#define BGP_ERR_CODE_CEASE                       0x06

// Constants for Error subcodes in MessageHeaderError reference
// rfc 1771
#define BGP_ERR_SUB_CODE_DEFAULT                                0x00
#define BGP_ERR_SUB_CODE_MSG_HDR_CONNECTION_NOT_SYNCHRONIZED    0x01
#define BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_LEN                    0x02
#define BGP_ERR_SUB_CODE_MSG_HDR_BAD_MSG_TYPE                   0x03

// Constants for Error subcodes in Open Message Error rfc 1771
#define BGP_ERR_SUB_CODE_OPEN_MSG_UNSUPPORTED_VERSION           0x01
#define BGP_ERR_SUB_CODE_OPEN_MSG_BAD_PEER                      0x02
#define BGP_ERR_SUB_CODE_OPEN_MSG_BAD_BGP_IDENTIFIER            0x03
#define BGP_ERR_SUB_CODE_OPEN_MSG_UNSUPPORTED_OPT_PARAM         0x04
#define BGP_ERR_SUB_CODE_OPEN_MSG_AUTHENTICATION_FAILURE        0x05
#define BGP_ERR_SUB_CODE_OPEN_MSG_UNACCEPTABLE_HOLD_TIME        0x06

// Constants for Error subcodes in Update Message Error: rfc 1771
#define BGP_ERR_SUB_CODE_UPDATE_MSG_MALFORMED_ATTR_LIST          0x01
#define BGP_ERR_SUB_CODE_UPDATE_MSG_UNRECOGNIZED_WELL_KNOWN_ATTR 0x02
#define BGP_ERR_SUB_CODE_UPDATE_MSG_MISSING_WELL_KNOWN_ATTR      0x03
#define BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_FALG_ERR                0x04
#define BGP_ERR_SUB_CODE_UPDATE_MSG_ATTR_LEN_ERR                0x05
#define BGP_ERR_SUB_CODE_UPDATE_MSG_INVALID_ORIGIN_ATTR          0x06
#define BGP_ERR_SUB_CODE_UPDATE_MSG_AS_ROUTING_LOOP              0x07
#define BGP_ERR_SUB_CODE_UPDATE_MSG_INVALID_NEXT_HOP_ATTR        0x08
#define BGP_ERR_SUB_CODE_UPDATE_MSG_OPTIONAL_ATTR_ERR            0x09
#define BGP_ERR_SUB_CODE_UPDATE_MSG_INVLID_NETWORK_FIELD         0x0A
#define BGP_ERR_SUB_CODE_UPDATE_MSG_MALFORMED_AS_PATH            0x0B

///////////////////////////////////////////////////////////////////
//              Some symbolic constants for Bgp

#define BGP_INVALID_ID                           -1
#define BGP_INVALID_ADDRESS                       0
#define BGP_MIN_RT_BLOCK_SIZE                    10
#define BGP_MIN_CONN_BLOCK_SIZE                   5
#define BGP_MIN_HDR_LEN                          19
#define BGP_MAX_HDR_LEN                          4096
#define BGP_SIZE_OF_OPEN_MSG_EXCL_OPT            BGP_MIN_HDR_LEN + 10
#define BGP_SIZE_OF_UPDATE_MSG_EXCL_OPT          BGP_MIN_HDR_LEN + 4
#define BGP_SIZE_OF_NOTIFICATION_MSG_EXCL_DATA   BGP_MIN_HDR_LEN + 2
#define BGP_MIN_OPEN_MESSAGE_SIZE                10

// will be used to store ip address string
#define ADDR_STRING_LEN 16

// Symbolic constants to be used in Bgp packets

// Size of marker field in header
#define BGP_SIZE_OF_MARKER       16
#define BGP_MESSAGE_LENGTH_SIZE  2
#define BGP_MESSAGE_TYPE_SIZE    1

// Constants for Path Origin to be used in Origin attribute
#define BGP_ORIGIN_IGP               0x00
#define BGP_ORIGIN_EGP               0x01
#define BGP_ORIGIN_INCOMPLETE        0x02

#define BGP_PATH_ATTR_BEST_TRUE          2
#define BGP_PATH_ATTR_BEST_FALSE         1
#define BGP_DEFAULT_EXTERNAL_WEIGHT      0
#define BGP_DEFAULT_INTERNAL_WEIGHT      32768
#define BGP_MIN_DELAY                   (10 * NANO_SECOND)

// These are not defined yet and will be used latter....
#define BGP_PATH_ATTR_ATOMIC_AGGRIGATE_INVALID   0
#define BGP_PATH_ATTR_AGGRIGATOR_AS_INVALID      0
#define BGP_PATH_AGGRIGATOR_ADDR_INVALID         0

#define BGP_IPV4_ROUTE_INFO_STRUCT_SIZE 5
#define BGP_IPV6_ROUTE_INFO_STRUCT_SIZE 17

#define BGP_INFEASIBLE_ROUTE_LENGTH_SIZE 2
#define BGP_PATH_ATTRIBUTE_LENGTH_SIZE   2

#define BGP_ATTRIBUTE_FLAG_SIZE     1
#define BGP_ATTRIBUTE_TYPE_SIZE     1
#define BGP_ATTRIBUTE_LENGTH_SIZE   2

#define BGP_ATTRIBUTE_FLAG_TYPE_LENGTH_SIZE (BGP_ATTRIBUTE_FLAG_SIZE + \
                                             BGP_ATTRIBUTE_TYPE_SIZE + \
                                             BGP_ATTRIBUTE_LENGTH_SIZE)

#define BGP_VERSION_FIELD_LENGTH   1
#define BGP_ASID_FIELD_LENGTH      2
#define BGP_HOLDTIMER_FIELD_LENGTH 2

#define BGP_PATH_SEGMENT_TYPE_SIZE   1
#define BGP_PATH_SEGMENT_LENGTH_SIZE 1


#define MBGP_AFI_SIZE   2
#define MBGP_SUB_AFI_SIZE 1
#define MBGP_NEXT_HOP_LEN_SIZE 1
#define MBGP_NUM_SNPA_SIZE 1
#define MBGP_REACH_ATTR_MAX_HEADER_SIZE  (5+16)//change this one later????

#define MBGP_IPV4_AFI   1
#define MBGP_IPV6_AFI   2

#define MBGP_ALLOWED_ATTRIBUTE_TYPE 7

// Following constants will be used for debugging purpose
#define MAX_IP_STR_LEN 16
#define MAX_CLOCK_TYPE_STR_LEN 21

// Bgp attibute types
typedef enum {
    BGP_PATH_ATTR_TYPE_ORIGIN = 1,
    BGP_PATH_ATTR_TYPE_AS_PATH,
    BGP_PATH_ATTR_TYPE_NEXT_HOP,
    BGP_PATH_ATTR_TYPE_MULTI_EXIT_DISC,
    BGP_PATH_ATTR_TYPE_LOCAL_PREF,
    BGP_PATH_ATTR_TYPE_ATOMIC_AGGREGATION,
    BGP_PATH_ATTR_TYPE_AGGREGATOR,
    BGP_PATH_ATTR_TYPE_ORIGINATOR_ID = 9,
    BGP_PATH_ATTR_TYPE_CLUSTER_ID,
    BGP_MP_REACH_NLRI = 14,
    BGP_MP_UNREACH_NLRI = 15
} AttributeTypeCode;

typedef struct{
    unsigned char prefixLen;
    Address prefix;
} BgpRouteInfo;

// Enums for attribute flag field in Update message
typedef enum {
    BGP_WELL_KNOWN_ATTRIBUTE,
    BGP_OPTIONAL_ATTRIBUTE
} OptionalBit;

typedef enum {
    BGP_NON_TRANSITIVE_ATTRIBUTE,
    BGP_TRANSITIVE_ATTRIBUTE
} TransitiveBit;

typedef enum {
    BGP_PATH_ATTR_COMPLETE,
    BGP_PATH_ATTR_PARTIAL
} PartialBit;

typedef enum {
    ONE_OCTATE,
    TWO_OCTATE
} ExtendedLengthBit;

typedef enum {
    BGP_PATH_SEGMENT_AS_SET = 1,
    BGP_PATH_SEGMENT_AS_SEQUENCE
} PathSegType;

/////////////////////////////////////////////////////////
// BGP States:
//
//            0 - Idle
//            1 - Connect
//            2 - Active
//            3 - OpenSent
//            4 - OpenConfirm
//            5 - Established

typedef enum {
    BGP_IDLE,
    BGP_CONNECT,
    BGP_ACTIVE,
    BGP_OPEN_SENT,
    BGP_OPEN_CONFIRM,
    BGP_ESTABLISHED
} BgpStateType;

// BGP Events: rfc: 1771
//
//            0 - BGP Start
//            1 - BGP Stop (initiated by the system or operator)
//            2 - BGP Transport connection open
//            3 - BGP Transport connection closed
//            4 - BGP Transport connection open failed
//            5 - BGP Transport fatal error
//            6 - ConnectRetry timer expired
//            7 - Hold Timer expired
//            8 - KeepAlive timer expired
//            9 - Receive OPEN message
//           10 - Receive KEEPALIVE message
//           11 - Receive UPDATE messages
//           12 - Receive NOTIFICATION message

typedef enum {
    BGP_START,
    BGP_TRANSPORT_CONNECTION_OPEN,
    BGP_TRANSPORT_CONNECTION_OPEN_FAILED,
    BGP_TRANSPORT_CONNECTION_CLOSED,
    BGP_CONNECTRETRY_TIMER_EXPIRED,
    BGP_HOLD_TIMER_EXPIRED,
    BGP_KEEPALIVE_TIMER_EXPIRED,
    BGP_RECEIVE_OPEN_MESSAGE,
    BGP_RECEIVE_KEEPALIVE_MESSAGE,
    BGP_RECEIVE_UPDATE_MESSAGE,
    BGP_RECEIVE_NOTIFICATION_MESSAGE,
    SEND_UPDATE
} BgpEvent;

// Buffer used to reassemble raw transport packet
typedef struct struct_bgp_msg_buffer
{
    unsigned char* data;
    int   currentSize;
    int   expectedSize;
} BgpMessageBuffer;

// Structure to be used in the info part of the Bgp timers
// to know for which connection the timer is meant for and
// to know whether the timer is a valid timer
typedef struct
{
    int uniqueId;
    unsigned int sequence;
} ConnectionInfo;


// Bgp different message formats

// This structures not are used to send packet to other nodes.
// there the packet has been allocated in flat bytes. The
// structures has been used internal to a node for easy
// accessibility of the fields

// BGP packet common header : rfc 1771
typedef struct {
    unsigned char  marker[BGP_SIZE_OF_MARKER];
    unsigned short length;
    unsigned char  type;
} BgpPacketHeader;

typedef struct {
    unsigned char  bgpPathAttr;//optional:1,
                   //transitive:1,
                   //partial:1,
                   //extendedLength:1,
                   //reserved:4;
    unsigned char  attributeType;
    unsigned short attributeLength;
    char* attributeValue;
} BgpPathAttribute;

typedef struct
{
    unsigned char   pathSegmentType;
    unsigned char   pathSegmentLength;
    unsigned short* pathSegmentValue;
} BgpPathAttributeValue;

typedef unsigned char         OriginValue;
typedef UInt32                NextHopValue;
typedef BgpPathAttributeValue AsPathValue;
typedef UInt32                MedValue;
typedef UInt32                LocalPrefValue;
typedef UInt32            AfiValue;
typedef unsigned char         SubAfiValue;


typedef struct {
    unsigned short int  afIden;
    unsigned char   subAfIden;
    unsigned char   nextHopLen;
    char*       nextHop;
    unsigned char   numSnpa;
    char*       Snpa;
    BgpRouteInfo*   Nlri;
}MpReachNlri;

typedef struct {
    unsigned short int  afIden;
    unsigned char   subAfIden;
    BgpRouteInfo*   Nlri;
}MpUnReachNlri;

typedef struct {
    BgpPacketHeader     commonHeader;
    unsigned short      infeasibleRouteLength;
    BgpRouteInfo*       withdrawnRoutes;
    unsigned short      totalPathAttributeLength;
    BgpPathAttribute*   pathAttribute;
    BgpRouteInfo*       networkLayerReachabilityInfo;
} BgpUpdateMessage;

// Bgp information Base

// BGP Statistics Structure
typedef struct{
    unsigned int openMsgSent;
    unsigned int openMsgReceived;
    unsigned int keepAliveSent;
    unsigned int keepAliveReceived;
    unsigned int updateSent;
    unsigned int reflectedupdateSent;
    unsigned int updateReceived;
    unsigned int notificationSent;
    unsigned int notificationReceived;
} BgpStats;



// Contents of Routing Table
// ref rfc 1657
typedef struct{
    BgpRouteInfo    destAddress;
    Address         peerAddress;      // Type and IP address of the peer 
                                      // from whom the route has been learnt

    unsigned short  asIdOfPeer;       // asIdOfPeer
    BOOL               isClient;        // will be nessary for route reflection

    NodeAddress     bgpIdentifier;    // bgpIdentifier of peer.

    NodeAddress     originatorId;    // originator ID
    Address     nextHop;   // Next hop for NLRI

    unsigned char   originType; //origin Attribute received in Update message
    unsigned char   origin;    // for this implementation all the entries
                               // during initialization will have origin 0
                               // as they are taken from IGP and after that
                               // the routes have origin of 1

    BgpPathAttributeValue* asPathList; // assuming that as id can not be
                                       // greater than 2 ^ 16 -1

    unsigned int    multiExitDisc;      // Used to discriminate between
                                        // multiple exit
                               // points to neighboring AS .. a value of -1
                               // is set if not used

    unsigned short weight;     // This is a Cisco specified parameter to
                               // locally select a destination if muliple
                               // path exists to the destination and their
                               // weight is different. Higher weight peer
                               // is chosen to forward packets

    unsigned int localPref;    // Originating bgp's degree of pref.

    unsigned char pathAttrAtomicAggregate; // 1 or 2 Whether or not the
                                           // local system has selected a
                                           // less specific route without
                                           // selecting a more specific
                                           // route

    unsigned short pathAttrAggregatorAS;   // AS id of the bgp speaker who
                                           // had done last route aggegation
                                           // with this route... a value 0
                                           // means absense of this entry

    NodeAddress pathAttrAggregatorAddr;    // Address of the last bgp speaker
                                           // who has done the route
                                           // aggregation. 0.0.0.0 if not
                                           // used

    unsigned int calcLocalPref; // Calculated local preference by the
                                // receiver bgp speaker for advertising the
                                // routes.... -1 if unused

    unsigned char pathAttrBest; // false (1) if not chosen as best route
                                // true  (2) if chosen as best route

    unsigned char* pathAttrUnknown;  // NOT USED IN THE IMPLEMENTATION

    BOOL isValid;
} BgpRoutingInformationBase;

typedef struct{
    BgpRoutingInformationBase* ptrAdjRibIn;
    BOOL movedToAdjRibOut;
    BOOL movedToWithdrawnRt;
    BOOL isValid;
} BgpAdjRibLocStruct;


// The neighbors information of Bgp.. Reference rfc 1657
typedef struct struct_connection_info {
    Address     remoteAddr;   // All the connection informations
    unsigned short  remotePort;
    Address     localAddr;
    unsigned short  localPort;

    NodeAddress        localBgpIdentifier;
    NodeAddress        remoteBgpIdentifier;

    unsigned short     asIdOfPeer;

    int                connectionId;
    int                uniqueId;

    // Bgp internal informations for each connection
    NodeAddress        remoteId;
    NodeAddress        localId;

    // Current state of this connection
    BgpStateType       state;

    // These sequence numbers will be useful to cancel an event
    unsigned int       connectRetrySeq;
    unsigned int       keepAliveTimerSeq;
    unsigned int       holdTimerSeq;

    // This start event time will store the previous time interval after
    // which start event was raised so that if there is any close connection
    // due to any error then the next timer will be raised after exponential
    // increase of the previous start interval
    clocktype          startEventTime;

    // To store the negotiated hold time value
    clocktype          holdTime;

    // Time when the last update was sent. This information will be necessary
    // to maintain a minimum interval between subsequent route advertisements
    clocktype          nextUpdate;

    DataBuffer         adjRibOut;     // The routes to be advertised to the
                                      // peer. will be represented as a pointer
                                      // to the local RIB

    DataBuffer         withdrawnRoutes; // List of WithDrawnRoutes to be
                                        // Advertised to the peer. NOTE:
                                       // those routes for which a replacement
                                       // route is going in the NLRI field will
                                       // not be included in this list
    BOOL               isInternalConnection;
    BOOL               isInitiatingNode;
    BOOL               isClient;        // will be nessary for route reflection
    BgpMessageBuffer   buffer;
    unsigned short     weight;           // a cisco specific parameter to give
                                         // preference to certain routes.
    BOOL               ismedPresent;
    unsigned int       multiExitDisc;      // MED

    BOOL               updateTimerAlreadySet;
} BgpConnectionInformationBase;

typedef struct{
    NodeAddress    destNetwork;
    unsigned char  numSubnetBits;
    NodeAddress    borderRouter;
    NodeAddress    nextHop;
} ForwardingInfoBase;

// BGP node Structure

typedef struct{
    unsigned short asId;            // Autonomous System No
    NodeAddress    routerId;        // BGP router ID
    BOOL           statsAreOn;      // Says whether to collect stat
    BgpStats       stats;           // Structure to collect statistics

    NodeAddress    borderRouter;    // The address of the border router

    DataBuffer     connInfoBase;    // Connections already established
    DataBuffer     adjRibIn;        // Unprocessed routing Information
                                    // listened from the peers
    DataBuffer     ribLocal;        // Routes chosen from the input routes
                                    // to use internally: will be represented
                                       // as a pointer to the adjRibIn
    DataBuffer     forwardingInfoBase; // Routing table for BGP

    BOOL           isRtReflector;      // If the bgp speaker is working as a
                                       // route reflector
    int            clusterId;          // If the bgp speaker is a route
                                       // reflector then it will form a
                                       // cluster with its clients with one
                                       // cluster id

    unsigned int   localPref;          // Local preference of the bgp speaker.
    unsigned int   multiExitDisc;      // MED

    BOOL           advertiseIgp;       // whether or not to advertise IGP
                                       // routes

    BOOL           redistribute2Ospf;
    // Whether or not to advertise OSPFv2
    // external routes to BGP on
    // a BGP/OSPF ASBR.
    BOOL           advertiseOspfExternalType1;
    BOOL           advertiseOspfExternalType2;
    BOOL           isSynchronized;     // Whether it needs synchronization
                                       // with Igp
    NetworkType    ip_version;          //added for MBGP extension,IPv6 or IPv4

    BOOL           bgpNextHopLegacySupport ;
    BOOL           bgpRedistributeStaticAndDefaultRts;
    RandomSeed     timerSeed;

    clocktype      startTime;
    clocktype      holdInterval;       // The initial hold timer value
    clocktype      largeHoldInterval;
    clocktype      minRouteAdvertisementInterval_EBGP;
    clocktype      minRouteAdvertisementInterval_IBGP;
    clocktype      minASOriginationInterval;
    clocktype      keepAliveInterval;
    clocktype      connectRetryInterval;
    clocktype      waitingTimeForRoute;
    clocktype      igpProbeInterval;
} BgpData;


// The interface functions which other functions will call
void BgpInit(Node* node, const NodeInput* nodeInput);
void BgpLayer(Node* node, Message* msg);
void BgpFinalize(Node* node);
extern void BgpCloseConnection(Node* node,BgpData* bgp,
    BgpConnectionInformationBase* connection);
extern int MAPPING_CompareAddress(
    const Address& address1,
    const Address& address2);
#endif
