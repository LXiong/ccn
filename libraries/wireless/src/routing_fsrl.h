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


// /**
// PROTOCOL     :: LANMAR
//
// SUMMARY      :: This is the implementation of Landmark Ad Hoc
//                 Routing (LANMAR)
//
// LAYER        :: Network
//
// STATICSTICS  ::
// + intraScopeUpdate : Number of local scope routing updates
// + LMUpdate : Number of landmark updates
// + controlOH : Total routing control overhead in number of bytes
// + controlPkts : Total number of routing packets
// + dropInScope : Number of packets dropped due to no local route
// + dropNoLM : Number of packets dropped due to no landmark info
// + dropNoDF : Number of packets dropped due to no drifter info
// + dropNoRoute : Number of packets dropped due to no route
//                 All drops other above three reasons
//
// CONFIG_PARAM ::
// + ROUTING-STATISTICS : BOOL : Determins if LANMAR will collect stats
// + LANMAR-MIN-MEMBER-THRESHOLD : int : Minimum number of members in
//                                       scope for being a landmark
// + LANMAR-FISHEYE-SCOPE : int : The scope limit for local routing
// + LANMAR-FISHEYE-UPDATE-INTERVAL : clocktype : Frequency for local
//                                                Fisheye routing
// + LANMAR-LANDMARK-UPDATE-INTERVAL : clocktype : Frequency ofr landmark
//                                                 updates
// + LANMAR-NEIGHBOR-TIMEOUT-INTERVAL : clocktype : Timeout duration of
//                                                  neighbor list
// + LANMAR-FISHEYE-MAX-AGE : clocktype : Lifetime of an entry in local
//                                        topology table
// + LANMAR-LANDMARK-MAX-AGE : clocktype : Lifetime of an entry in
//                                         landmark table
// + LANMAR-DRIFTER-MAX-AGE : clocktype : Lifetime of an entry in
//                                        drifter table
//
// VALIDATION   ::
//
// IMPLEMENTED FEATURES ::
// + Local Scope Routing : Fisheye state routing
// + Landmark Updates : Distance vector routing
//
// OMITTED FEATURES ::
//
// ASSUMPTIONS  ::
// + Nodes are dived into landmark groups as different subnets
// + Nodes in the same landmark group move as group mobility
// + Group mobility model is configured
//
// STANDARD     :: IETF draft <draft-ietf-manet-lanmar-04.txt>
// **/

#ifndef FSRL_H
#define FSRL_H

#include "list.h"

// /**
// CONSTANT    :: FISHEYE_INFINITY : 100
// DESCRIPTION :: Infinite distance in terms of number of hops
// **/
#define FISHEYE_INFINITY                        100

// /**
// CONSTANT    :: FISHEYE_DEFAULT_ALPHA : 1.3
// DESCRIPTION :: Default value of the alpha parameter of LANMAR election
// **/
#define FISHEYE_DEFAULT_ALPHA                   1.3

// /**
// CONSTANT    :: FISHEYE_DEFAULT_SCOPE : 2
// DESCRIPTION :: Default value of the scope for local routing
// **/
#define FISHEYE_DEFAULT_SCOPE                   2

// /**
// CONSTANT    :: FISHEYE_DEFAULT_MIN_MEMBER_THRESHOLD : 18
// DESCRIPTION :: Default value of the minimal member threshold of
//                LANMAR election algorithm
// **/
#define LANMAR_DEFAULT_MIN_MEMBER_THRESHOLD     8

// /**
// CONSTANT    :: FISHEYE_RANDOM_JITTER : 100ms
// DESCRIPTION :: Used for avoiding synchronization effect
// **/
#define FISHEYE_RANDOM_JITTER                   (100 * MILLI_SECOND)

// /**
// CONSTANT    :: FISHEYE_SEQUENCE_NO_INCREMENT : 2
// DESCRIPTION :: Used for increment the sequence number of a node's
//                own entries in the routing table.
// **/
#define FISHEYE_SEQUENCE_NO_INCREMENT           2

// /**
// CONSTANT    :: FISHEYE_MAX_TOPOLOGY_TABLE
// DESCRIPTION :: The maximum size of the control packet
// **/
#define FISHEYE_MAX_TOPOLOGY_TABLE  (MAX_NW_PKT_SIZE - sizeof(IpHeaderType))

// /**
// CONSTANT    :: LANMAR_DEFAULT_NEIGHBOR_TIMEOUT_INTERVAL
// DESCRIPTION :: The timeout duration of neighbor list
// **/
#define LANMAR_DEFAULT_NEIGHBOR_TIMEOUT_INTERVAL    (6 * SECOND)

// /**
// CONSTANT    :: LANMAR_DEFAULT_FISHEYE_UPDATE_INTERVAL
// DESCRIPTION :: Routing table update frequency within the fisheye scope
// **/
#define LANMAR_DEFAULT_FISHEYE_UPDATE_INTERVAL      (2 * SECOND)

// /**
// CONSTANT    :: LANMAR_DEFAULT_LANDMARK_UPDATE_INTERVAL
// DESCRIPTION :: Frequency of landmark updates
// **/
#define LANMAR_DEFAULT_LANDMARK_UPDATE_INTERVAL     (4 * SECOND)

// /**
// CONSTANT    :: LANMAR_DEFAULT_FISHEYE_MAX_AGE
// DESCRIPTION :: Maximum age of fisheye entries,
//                i.e, lifetime of an entry in local topology table
// **/
#define LANMAR_DEFAULT_FISHEYE_MAX_AGE              (6 * SECOND)

// /**
// CONSTANT    :: LANMAR_DEFAULT_LANDMARK_MAX_AGE
// DESCRIPTION :: Maximum age of landmark entries,
//                i.e, lifetime of an entry in landmark table
// **/
#define LANMAR_DEFAULT_LANDMARK_MAX_AGE             (12 * SECOND)

// /**
// CONSTANT    :: LANMAR_DEFAULT_DRIFTER_MAX_AGE
// DESCRIPTION :: Maximum age for drifter entries,
//                i.e, lifetime of an entry in drifter table
// **/
#define LANMAR_DEFAULT_DRIFTER_MAX_AGE              (12 * SECOND)

// /**
// ENUM        :: FsrlPacketType
// DESCRIPTION :: Type of control packet
// **/
typedef enum
{
    FSRL_UPDATE_DV,
    FSRL_UPDATE_TOPOLOGY_TABLE
} FsrlPacketType;


// /**
// STRUCT      :: FsrlFisheyeHeaderField
// DESCRIPTION :: Header of the Fisheye routing packet
// **/
typedef struct
{
    // packetType needs to be declared first to determine packet type.
    unsigned char packetType;
    unsigned char reservedBits;
    unsigned short packetSize;
} FsrlFisheyeHeaderField;

// /**
// STRUCT      :: FsrlFisheyeDataField
// DESCRIPTION :: Data unit format of Fisheye routing packet
// **/
typedef struct
{
    NodeAddress destAddr;
    UInt32 dataField;//sequenceNumber:24,
                            //numNeighbor:8;
} FsrlFisheyeDataField;

// /**
// FUNCTION    :: FsrlFisheyeDataFieldSetSeqNum()
// LAYER       :: NETWORK
// PURPOSE     :: Set the value of sequenceNumber for FsrlFisheyeDataField
// PARAMETERS  ::
// + dataField : UInt32* : The variable containing the value of
//                         sequenceNumber and numNeighbor
// + sequenceNumber : UInt32: Input value for set operation
// RETURN     :: void : NULL
// **/
static void FsrlFisheyeDataFieldSetSeqNum(UInt32* dataField,
                                          UInt32 sequenceNumber)
{
    //masks sequenceNumber within boundry range
    sequenceNumber = sequenceNumber & maskInt(9, 32);

    //clears the first 24 bits
    *dataField = *dataField & maskInt(25, 32);

    //setting the value of sequence number in data field
    *dataField = *dataField | LshiftInt(sequenceNumber, 24);
}


// /**
// FUNCTION     :: FsrlFisheyeDataFieldSetNumNeighbour()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of numNeighbour for FsrlFisheyeDataField
// PARAMETERS   ::
// + dataField : UInt32* : The variable containing the value of
//                         sequenceNumber and numNeighbor
// + numNeighbor : UInt32: Input value for set operation
// RETURN       :: void : NULL
// **/
static void FsrlFisheyeDataFieldSetNumNeighbour(UInt32* dataField,
                                                UInt32 numNeighbor)
{
    //masks numNeighbor within boundry range
    numNeighbor = numNeighbor & maskInt(25, 32);

    //clears the last 8 bits
    *dataField = *dataField & maskInt(1, 24);

    //setting the value of numneighbor in data field
    *dataField = *dataField | numNeighbor;
}


// /**
// FUNCTION      :: FsrlFisheyeDataFieldGetSeqNum()
// LAYER         :: NETWORK
// PURPOSE       :: Returns the value of sequenceNumber for
//                  FsrlFisheyeDataField
// PARAMETERS    ::
// + dataField : UInt32 : The variable containing the value of
//                        sequenceNumber and numNeighbor
// RETURN        :: UInt32
// **/
static UInt32 FsrlFisheyeDataFieldGetSeqNum(UInt32 dataField)
{
    UInt32 sequenceNumber = dataField;

    //clears the last 8 bits
    sequenceNumber = sequenceNumber & maskInt(1, 24);

    //right shifts 8 bits
    sequenceNumber = RshiftInt(sequenceNumber, 24);

    return sequenceNumber;
}


// /**
// FUNCTION      :: FsrlFisheyeDataFieldGetNumNeighbour()
// LAYER         :: NETWORK
// PURPOSE       :: Returns the value of numNeighbour for
//                  FsrlFisheyeDataField
// PARAMETERS    ::
// + dataField : UInt32 : The variable containing the value of
//                        sequenceNumber and numNeighbor
// RETURN        :: UInt32
// **/
static unsigned char FsrlFisheyeDataFieldGetNumNeighbour(UInt32 dataField)
{
    UInt32 numNeighbour = dataField;

    //clears the first 24 bits
    numNeighbour  = numNeighbour & maskInt(25, 32);

    return (unsigned char) numNeighbour;
}


// /**
// STRUCT      :: FsrlLanmarUpdateHeaderField
// DESCRIPTION :: Header of the landmark update packet
// **/
typedef struct
{
    // packetType needs to be declared first to determine packet type.
    // This takes the place of 'reserve' field for the time being.
    unsigned char packetType;

    // Not needed.  LANMAR author will remove this field in the next draft.
    unsigned char landmarkFlag;

    unsigned char numLandmark;
    unsigned char numDrifter;
} FsrlLanmarUpdateHeaderField;

// /**
// STRUCT      :: FsrlLanmarUpdateLandmarkField
// DESCRIPTION :: Data unit format of landmark update packet
// **/
typedef struct
{
    NodeAddress landmarkAddr;
    NodeAddress nextHopAddr;
    unsigned char distance;
    unsigned char numMember;
    unsigned short sequenceNumber;
} FsrlLanmarUpdateLandmarkField;

// /**
// STRUCT      :: FsrlLanmarUpdateDrifterField
// DESCRIPTION :: Data unit format of drifter
// **/
typedef struct
{
    NodeAddress drifterAddr;
    NodeAddress nextHopAddr;
    unsigned short sequenceNumber;
    unsigned char distance;
    unsigned char reservedBits;
} FsrlLanmarUpdateDrifterField;

// /**
// STRUCT      :: FsrlListToArrayItem
// DESCRIPTION :: Data structure used for shorest-path calculation
// **/
typedef struct fsrl_list_to_array_item_str
{
    NodeAddress nodeAddr;
    int index;
    struct fsrl_list_to_array_item_str* prev;
    struct fsrl_list_to_array_item_str* next;
} FsrlListToArrayItem;


// /**
// STRUCT      :: FsrlListToArray
// DESCRIPTION :: Data structure used for shorest-path calculation
// **/
typedef struct
{
    int size;
    FsrlListToArrayItem* first;
} FsrlListToArray;


// /**
// STRUCT      :: FsrlNeighborListItem
// DESCRIPTION :: information of one neighbor node
// **/
typedef struct fsrl_neighbor_list_str
{
    // Used during shortest path calculation.
    int* index;

    NodeAddress nodeAddr;

    clocktype timeLastHeard;

    int incomingInterface;

    struct fsrl_neighbor_list_str* next;
} FsrlNeighborListItem;

// /**
// STRUCT      :: FisheyeNeighborInfo
// DESCRIPTION :: description of neighboring information
// **/
typedef struct
{
    /* sequence number of the neigboring list */
    int sequenceNumber;

    /* neighbor number of the destionation node */
    unsigned char numNeighbor;

    FsrlNeighborListItem* list;

} FisheyeNeighborInfo;

// /**
// STRUCT      :: FisheyeRoutingTableRow
// DESCRIPTION :: One entry of the local routing table
// **/
typedef struct
{
    NodeAddress destAddr;
    NodeAddress nextHop;
    NodeAddress prevHop;
    int distance;

    int outgoingInterface;
} FisheyeRoutingTableRow;


// /**
// STRUCT      :: FisheyeTopologyTableRow
// DESCRIPTION :: One entry of the local fisheye topology table
//                The topology table is a matrix of link connectivity.
// **/
typedef struct fisheye_tt_row_str
{
    NodeAddress linkStateNode;

    // Used during shortest path calculation.
    // Maps link state node to sorted array.
    int* index;

    int distance;

    // Direct neighbors of this link state node.
    FisheyeNeighborInfo* neighborInfo;

    // Needed to age out stale entries.
    clocktype timestamp;
    clocktype removalTimestamp;

    // the incoming interface index of this packet
    // this interface will be the outgoing interface
    // for routing packet later (assuming symmetric
    // wireless links.)
    int incomingInterface;

    // Used only for debugging purposes.
    NodeAddress fromAddr;

    struct fisheye_tt_row_str* next;
} FisheyeTopologyTableRow;


// /**
// STRUCT      :: FisheyeTopologyTable
// DESCRIPTION :: Structure of local Fisheye topology table
// **/
typedef struct
{
    // start pointer to the TopologyTable
    FisheyeTopologyTableRow* first;
} FisheyeTopologyTable;


// /**
// STRUCT      :: FisheyeRoutingTable
// DESCRIPTION :: Structure of local routing table
// **/
typedef struct
{
    int rtSize;
    BOOL changed;
    FisheyeRoutingTableRow* row;
} FisheyeRoutingTable;


// /**
// STRUCT      :: FsrlLandmarkTableRow
// DESCRIPTION :: One entry of the landmark table
// **/
typedef struct fisheye_lmdv_row_str
{
    NodeAddress destAddr;
    NodeAddress nextHop;
    int numLandmarkMember;
    int distance;
    int sequenceNumber;

    // Needed to age out stale entries.
    clocktype timestamp;

    // For debugging purposes only
    NodeAddress fromAddr;

    int outgoingInterface;

    struct fisheye_lmdv_row_str* prev;
    struct fisheye_lmdv_row_str* next;
} FsrlLandmarkTableRow;


// /**
// STRUCT      :: FsrlLandmarkTable
// DESCRIPTION :: Structure of the landmark table
// **/
typedef struct
{
    int size;
    int ownSequenceNumber;
    FsrlLandmarkTableRow* row;
} FsrlLandmarkTable;


// /**
// STRUCT      :: FsrlDrifterTableRow
// DESCRIPTION :: One entry of the drifter table
// **/
typedef struct fisheye_dfdv_row_str
{
    NodeAddress destAddr;

    // Next hop from drifter to landmark
    NodeAddress nextHop;

    int distance;
    int sequenceNumber;
    clocktype lastModified;

    int outgoingInterface;

    struct fisheye_dfdv_row_str* next;
} FsrlDrifterTableRow;


// /**
// STRUCT      :: FsrlDrifterTable
// DESCRIPTION :: Structure the drifter table
// **/
typedef struct
{
    int size;
    int ownSequenceNumber;
    FsrlDrifterTableRow* row;
} FsrlDrifterTable;


// /**
// STRUCT      :: FsrlStats
// DESCRIPTION :: Statistics of LANMAR routing
// **/
typedef struct
{

    // Total number of TopologyTable updates In the scope
    int intraScopeUpdate;

    // Total number of LM updates
    int LMUpdate;

    // Total Control OH in bytes
    int controlOH;

    // Total Control Pkts
    int controlPkts;

    // drop within the scope
    int dropInScope;

    // drop no LM matching
    int dropNoLM;

    // drop no drifter matching
    int dropNoDF;

    // no route drop
    int dropNoRoute;

} FsrlStats;

// /**
// STRUCT      :: FsrlParameter
// DESCRIPTION :: Prameters of LANMAR routing
// **/
typedef struct
{
    // Fisheye scope
    short scope;

    // neighbor time out interval
    clocktype neighborTOInterval;

    // update hop < Scope interval
    clocktype fisheyeUpdateInterval;

    // landmark update interval
    clocktype landmarkUpdateInterval;

    // threshold for landmark election
    int minMemberThreshold;

    // factor for landmark election
    double alpha;

    // Used to expire entries in various tables.
    clocktype fisheyeMaxAge;
    clocktype landmarkMaxAge;
    clocktype drifterMaxAge;
} FsrlParameter;


// /**
// STRUCT      :: FsrlData
// DESCRIPTION :: Data structure of LANMAR routing
// **/
typedef struct
{
    // keep track of topology table for every node
    FisheyeTopologyTable topologyTable;

    // keep track of routing decisions
    FisheyeRoutingTable routingTable;

    // keep track of landmark distance vector
    FsrlLandmarkTable landmarkTable;

    // keep track of drifters
    FsrlDrifterTable drifterTable;


    int numLandmarkMember[MAX_NUM_PHYS];
    BOOL isLandmark[MAX_NUM_PHYS];

    // keep track of different routing statistics
    FsrlStats stats;

    // Keep track of the parameters of LANMAR protocol
    FsrlParameter parameter;

    NodeAddress ipAddress;
    int numHostBits;

    BOOL statsPrinted;

    RandomSeed seed;

} FsrlData;


//--------------------------------------------------------------------------
// PROTOTYPES FOR FUNCTIONS WITH EXTERNAL LINKAGE
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: FsrlInit
// LAYER      :: NETWORK
// PURPOSE    :: Initialization Function for LANMAR Protocol
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + fsrl : FsrlData** : Address of data structure pointer to be filled in
// + nodeInput : const NodeInput* : Pointer to NodeInput
// + interfaceIndex : int : Interface index
// RETURN     :: void : NULL
// **/
void FsrlInit(Node* node,
              FsrlData** fsrl,
              const NodeInput* nodeInput,
              int interfaceIndex);

// /**
// FUNCTION   :: FsrlFinalize
// LAYER      :: NETWORK
// PURPOSE    :: Finalization function, print out statistics
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: void : NULL
// **/
void FsrlFinalize(Node* node);

// /**
// FUNCTION   :: FsrlHandleProtocolEvent
// LAYER      :: NETWORK
// PURPOSE    :: Handle self-scheduled messages such as timers
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the event
// RETURN     :: void : NULL
// **/
void FsrlHandleProtocolEvent(Node* node,Message* msg);

// /**
// FUNCTION   :: FsrlHandleProtocolPacket
// LAYER      :: NETWORK
// PURPOSE    :: Handle routing packets
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// + srcAddr : NodeAddress : Source node of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + incomingInterface : int : From which interface the packet is received
// + ttl : int : TTL value in the IP header
// RETURN     :: void : NULL
// **/
void FsrlHandleProtocolPacket(Node* node,
                              Message* msg,
                              NodeAddress srcAddr,
                              NodeAddress destAddr,
                              int incomingInterface,
                              int ttl);

// /**
// FUNCTION   :: FsrlRouterFunction
// LAYER      :: NETWORK
// PURPOSE    :: Is called when network layer needs to route a packet
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// + msg : Message* : Pointer to message structure of the packet
// + destAddr : NodeAddress : Destination node of the packet
// + previousHopAddress : NodeAddress : Previous hop node
// + PacketWasRouted : BOOL* : For indicating whehter the packet is routed
//                             in this function
// RETURN     :: void : NULL
// **/
void FsrlRouterFunction(Node* node,
                        Message* msg,
                        NodeAddress destAddr,
                        NodeAddress previousHopAddress,
                        BOOL* PacketWasRouted);

#endif  // FSRL_H
