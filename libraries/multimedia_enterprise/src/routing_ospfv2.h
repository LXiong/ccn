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

#ifndef OSPFv2_H
#define OSPFv2_H

#include "route_map.h"
#include "buffer.h"
#include <vector>

#define OSPFv2_CURRENT_VERSION                  0x2

// Initial size of routing table maintained by OSPF.
#define OSPFv2_INITIAL_TABLE_SIZE               (10)

#define OSPFv2_LS_INFINITY                      0xFFFFFF

// Jitter transmissions to avoid  sychronization.
#define OSPFv2_BROADCAST_JITTER                 (40 * MILLI_SECOND)
//#define OSPFv2_BROADCAST_JITTER                 (100 * MILLI_SECOND)

// How often do we increment LS age.
// Has to be be greater than or equal to 1 second.

#define OSPFv2_LSA_INCREMENT_AGE_INTERVAL       (1 * SECOND)

// Maximum age limit for LSAs.
#define OSPFv2_LSA_MAX_AGE                      (60 * MINUTE)

// Should be based on transmission delay.  However, since
// OSPF's unit is in seconds, one second should cover most, if
// not all, transmission delays out there.
#define OSPFv2_INF_TRANS_DELAY       (1 * SECOND)

// Interval of sending hello packets (in seconds).
#define OSPFv2_HELLO_INTERVAL        (10 * SECOND)

// Interval of sending hello packets when the interface is down
#define OSPFv2_POLL_INTERVAL         (2 * MINUTE)

// Interval to know when can we consider a routing to be dead.
#define OSPFv2_ROUTER_DEAD_INTERVAL  (4 * OSPFv2_HELLO_INTERVAL)

// Retransmission interval of sending link state request (in seconds)
#define OSPFv2_RXMT_INTERVAL         (5 * SECOND)

// How often to originate LSA.
#define OSPFv2_LS_REFRESH_TIME       (30 * MINUTE)

// Minimum time between LSA arrival.
#define OSPFv2_MIN_LS_ARRIVAL        (1 * SECOND)

// Maximum age difference to be considered more recent LSA.
#define OSPFv2_MAX_AGE_DIFF          (15 * MINUTE)

#define OSPFv2_WAIT_TIMER        OSPFv2_ROUTER_DEAD_INTERVAL

#define OSPFv2_FLOOD_TIMER       (100 * MILLI_SECOND)
//#define OSPFv2_FLOOD_TIMER       (1 * SECOND)

#define OSPFv2_MIN_LS_INTERVAL   (5 * SECOND)

#define OSPFv2_EVENT_SCHEDULING_DELAY   (1 * NANO_SECOND)

// BGP-OSPF Patch Start
// Maximum time an ASBR wait to originate AS External LSA (Injected by user)
#define OSPFv2_AS_EXTERNAL_LSA_ORIGINATION_DELAY   (2 * MINUTE)

#define OSPFv2_MAX_AS_NUMBER     65535    // maximum AS number
#define OSPFv2_MIN_AS_NUMBER     1        // minimum AS number
// BGP-OSPF Patch End

#define  OSPFv2_OPTION_EXTERNAL  0x2
#define  OSPFv2_OPTION_NSSA_EXTERNAL  0x8

#define OSPFv2_SINGLE_AREA_ID    0xFFFFFFFF
#define OSPFv2_BACKBONE_AREA     0x0
#define OSPFv2_INVALID_AREA_ID   0xFFFFFFFE

#define OSPFv2_INITIAL_SEQUENCE_NUMBER  0x80000001
#define OSPFv2_MAX_SEQUENCE_NUMBER      0x7FFFFFFF

#define OSPFv2_ALL_SPF_ROUTERS    0xE0000005
#define OSPFv2_ALL_D_ROUTERS      0xE0000006

#define OSPFv2_DEFAULT_DESTINATION  0
#define OSPFv2_DEFAULT_MASK         0

#define OSPFv2_LSA_CHECKSUM_MODX     4102
#define OSPFv2_LSA_CHECKSUM_OFFSET   15

// Cisco specifies to maximize stability, one router should not be in more
// than three areas. However, users are free to change this value.
// Ref - http://www.cisco.com/univercd/cc/td/doc/cisintwk/idg4/nd2003.htm
// [Section - OSPF Network Topology]
#define OSPFv2_MAX_ATTACHED_AREA    3

#define OSPFv2_MAXAGE_LSA_REMOVAL_TIME  (2 * SECOND)

#ifdef ADDON_BOEINGFCS
#define OSPFv2_ROSPF_MAX_NEIGHBORS 10000
#endif

#define OSPFv2_DO_NOT_AGE  0x8000

// Structure for each item of a list.
typedef struct struct_Ospfv2_ListItem
{
    void* data;
    clocktype timeStamp;

    clocktype unreachableTimeStamp;
#ifdef ADDON_BOEINGFCS
    // referenceCount is used for garbage collection for some LSAs.
    // Currently only linkStateRetxList is implemented.  When adding
    // to the list using garbage collection call the
    // Ospfv2InsertToListReferenceCount function.  Ospfv2RemoveFromList
    // and Ospfv2FreeList can be called as usual.
    int* referenceCount;
    int numReferences;
#endif
    struct struct_Ospfv2_ListItem* prev;
    struct struct_Ospfv2_ListItem* next;
} Ospfv2ListItem;

// A list that stores different types of structures.
typedef struct struct_Ospfv2_List
{
    int size;
    struct struct_Ospfv2_ListItem* first;       // First item in list.
    struct struct_Ospfv2_ListItem* last;        // Last item in list.
} Ospfv2List;

typedef struct struct_Ospfv2_NonBroadcastNeighborListItem
{
    NodeAddress neighborAddr;
    struct struct_Ospfv2_NonBroadcastNeighborListItem* prev;
    struct struct_Ospfv2_NonBroadcastNeighborListItem* next;
} Ospfv2NonBroadcastNeighborListItem;

typedef struct struct_Ospfv2_NonBroadcastNeighborList
{
    int size;
    struct struct_Ospfv2_NonBroadcastNeighborListItem* first;
    struct struct_Ospfv2_NonBroadcastNeighborListItem* last;
} Ospfv2NonBroadcastNeighborList;

// Type of nodes.
typedef enum
{
    OSPFv2_ROUTER_TYPE,
    OSPFv2_HOST_TYPE
} Ospfv2RouterType;

// Used to calculate shortest path.
typedef enum
{
    OSPFv2_VERTEX_ROUTER,
    OSPFv2_VERTEX_NETWORK
} Ospfv2VertexType;

typedef struct
{
    NodeAddress outIntf;//interfaceId
    NodeAddress nextHop;
} Ospfv2NextHopListItem;

typedef struct
{
    NodeAddress             vertexId;
    Ospfv2VertexType vertexType;
    char*                   LSA;
    Ospfv2List*             nextHopList;
    unsigned int            distance;
} Ospfv2Vertex;

typedef struct
{
    char linkStateType;
    NodeAddress linkStateID;
    NodeAddress advertisingRouter;
} Ospfv2SendLSAInfo;


//-------------------------------------------------------------------------//
//                                 LSA FORMAT                              //
//-------------------------------------------------------------------------//
// Type of LSA.
typedef enum
{
    OSPFv2_ROUTER               = 1,
    OSPFv2_NETWORK              = 2,
    OSPFv2_NETWORK_SUMMARY      = 3,
    OSPFv2_ROUTER_SUMMARY       = 4,
    OSPFv2_ROUTER_AS_EXTERNAL   = 5,
    OSPFv2_GROUP_MEMBERSHIP     = 6,
    OSPFv2_ROUTER_NSSA_EXTERNAL = 7,
    OSPFv2_LINK_LOCAL_SCOPE_OPAQUE = 9,
    OSPFv2_AS_SCOPE_OPAQUE      = 11
} Ospfv2LinkStateType;

#define OSPFv2_MAX_LSA_TYPE      11 // used by ROSPF

// Type of link.
typedef enum
{
    OSPFv2_POINT_TO_POINT       = 1,
    OSPFv2_TRANSIT              = 2,
    OSPFv2_STUB                 = 3,
    OSPFv2_VIRTUAL              = 4
} Ospfv2LinkType;

/***** Start: OPAQUE-LSA *****/
// Type of Opaque LSA option
typedef enum
{
    ROSPF_CONFIGURATION_ADV = 5,
    HAIPE_ADDRESS_ADV = 6
} Ospfv2OpaqueType;
/***** End: OPAQUE-LSA *****/

typedef struct
{
    short unsigned int linkStateAge;
    //+------------------------------------+
    //| * | * | DC | EA | N/P | MC | E | T |
    //+------------------------------------+
    unsigned char options;
    //options:2, DC:1, EA:1, N/P:1, multicast:1, external:1, TOS:1;
    char linkStateType;
    NodeAddress linkStateID;
    NodeAddress advertisingRouter;
    int linkStateSequenceNumber;
    short unsigned int linkStateCheckSum;
    short int length;
#ifdef ADDON_MA
    NodeAddress mask;
#endif
} Ospfv2LinkStateHeader;

// /**
// FUNCTION     :: Ospfv2LinkStateHeaderSetQOS()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of qos for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char* : The variable containing the value of qos,
//                                 external,multicast and options1
// + qos        : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2LinkStateHeaderSetQOS(unsigned char *ospfLsh, BOOL qos)
{
    unsigned char x = (unsigned char)qos;

    //masks qos within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *ospfLsh = *ospfLsh & (~(maskChar(1, 1)));

    //setting the value of x in ospfLsh
    *ospfLsh = *ospfLsh | LshiftChar(x, 1);
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderSetExt()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of external for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char* : The variable containing the value of qos,
//                                 external,multicast and options1
// + external   : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2LinkStateHeaderSetExt(unsigned char *ospfLsh,
                                        BOOL external)
{
    unsigned char x = (unsigned char)external;

    //masks external within boundry range
    x = x & maskChar(8, 8);

    //clears the second bit
    *ospfLsh = *ospfLsh & (~(maskChar(2, 2)));

    //setting the value of x in ospfLsh
    *ospfLsh = *ospfLsh | LshiftChar(x, 2);
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderSetMulticast()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of multicast for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char* : The variable containing the value of qos,
//                                 external,multicast and options1
// + multicast  : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2LinkStateHeaderSetMulticast(unsigned char *ospfLsh,
                                              BOOL multicast)
{
    unsigned char x = (unsigned char)multicast;

    //masks multicast within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *ospfLsh = *ospfLsh & (~(maskChar(3, 3)));

    //setting the value of x in ospfLsh
    *ospfLsh = *ospfLsh | LshiftChar(x, 3);
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderSetOptions1()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of options1 for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char* : The variable containing the value of qos,
//                                 external,multicast and options1
// + options1    : unsigned char :Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2LinkStateHeaderSetOptions1(unsigned char *ospfLsh,
                                             unsigned char options1)
{
    //masks options1 within boundry range
    options1 = options1 & maskChar(4, 8);

    //clears the last five bits
    *ospfLsh = *ospfLsh & maskChar(1, 3);

    //setting the value of reserved in ospfLsh
    *ospfLsh = *ospfLsh | options1;
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderGetQOS()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of qos for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char : The variable containing the value of qos,
//                                 external,multicast and options1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2LinkStateHeaderGetQOS(unsigned char ospfLsh)
{
    unsigned char qos = ospfLsh;

    //clears all the bits except first bit
    qos = qos & maskChar(1, 1);

    //Right shifts so that last bit represents qos
    qos = RshiftChar(qos, 1);

    return (BOOL)qos;
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderGetExt()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of external for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char : The variable containing the value of qos,
//                                 external,multicast and options1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2LinkStateHeaderGetExt(unsigned char ospfLsh)
{
    unsigned char external = ospfLsh;

    //clears all the bits except second bit
    external = external & maskChar(2, 2);

    //Right shifts so that last bit represents external
    external = RshiftChar(external, 2);

    return (BOOL)external;
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderGetMulticast()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of multicast for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char : The variable containing the value of qos,
//                                 external,multicast and options1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2LinkStateHeaderGetMulticast(unsigned char ospfLsh)
{
    unsigned char multicast = ospfLsh;

    //clears all the bits except third bit
    multicast = multicast & maskChar(3, 3);

    //Right shifts so that last bit represents multicast
    multicast = RshiftChar(multicast, 3);

    return (BOOL)multicast;
}


// /**
// FUNCTION     :: Ospfv2LinkStateHeaderGetOptions1()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of options1 for Ospfv2LinkStateHeader
// PARAMETERS   ::
// + ospfLsh    : unsigned char : The variable containing the value of qos,
//                                 external,multicast and options1
// RETURN       :: BOOL
// **/
static unsigned char Ospfv2LinkStateHeaderGetOptions1(unsigned char ospfLsh)
{
    unsigned char options1 = ospfLsh;

    //clears the first three bits
    options1 = options1 & maskChar(4, 8);

    return options1;
}


// Each link of router LSA.
typedef struct
{
    NodeAddress linkID;
    NodeAddress linkData;
    unsigned char type;
    unsigned char numTOS;
    short int metric;
} Ospfv2LinkInfo;

// Structure of router LSA.
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;
    unsigned char ospfRouterLsa;//reserved:3, NSSATranslation:1,
            //wildCardMulticastReceiver:1, virtualLinkEndpoint:1,
            //ASBoundaryRouter:1, areaBorderRouter:1;
    unsigned char reserved2;
    short int numLinks;
    // Dynamically allocating next part.
} Ospfv2RouterLSA;

// /**
// FUNCTION     :: Ospfv2RouterLSASetABRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of areaBorderRouter for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// + abr           : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2RouterLSASetABRouter(unsigned char *ospfRouterLsa,
                                       BOOL abr)
{
    unsigned char x = (unsigned char)abr;

    //masks abr within boundry range
    x = x & maskChar(8, 8);

    //clears the eight bit
    *ospfRouterLsa = *ospfRouterLsa & (~(maskChar(8, 8)));

    //setting the value of x in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | LshiftChar(x, 8);
}


// /**
// FUNCTION     :: Ospfv2RouterLSASetASBRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of ASBoundaryRouter for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// + asbr          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2RouterLSASetASBRouter(unsigned char *ospfRouterLsa,
                                        BOOL asbr)
{
    unsigned char x = (unsigned char)asbr;

    //masks asbr within boundry range
    x = x & maskChar(8, 8);

    //clears the seventh bit
    *ospfRouterLsa = *ospfRouterLsa & (~(maskChar(7, 7)));

    //setting the value of x in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | LshiftChar(x, 7);
}


// /**
// FUNCTION     :: Ospfv2RouterLSASetVirtLnkEndPt()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of virtualLinkEndpoint for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// + vlep          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2RouterLSASetVirtLnkEndPt(unsigned char *ospfRouterLsa,
                                           BOOL vlep)
{
    unsigned char x = (unsigned char)vlep;

    //masks vlep within boundry range
    x = x & maskChar(8, 8);

    //clears the sixth bit
    *ospfRouterLsa = *ospfRouterLsa & (~(maskChar(6, 6)));

    //setting the value of x in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | LshiftChar(x, 6);
}


// /**
// FUNCTION     :: Ospfv2RouterLSASetWCMCReceiver()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of wildCardMulticastReceiver for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// + wcmr          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2RouterLSASetWCMCReceiver(unsigned char *ospfRouterLsa,
                                           BOOL wcmr)
{
    unsigned char x = (unsigned char)wcmr;

    //masks wcmr within boundry range
    x = x & maskChar(8, 8);

    //clears the fifth bit
    *ospfRouterLsa = *ospfRouterLsa & (~(maskChar(5, 5)));

    //setting the value of x in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | LshiftChar(x, 5);
}


// /**
// FUNCTION     :: Ospfv2RouterLSASetReserved1()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of reserved1 for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// + reserved    : unsigned char : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2RouterLSASetReserved1(unsigned char *ospfRouterLsa,
                                        unsigned char reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskChar(1, 4);

    //clears the first four bits
    *ospfRouterLsa = *ospfRouterLsa & maskChar(5, 8);

    //setting the value of reserved in ospfRouterLsa
    *ospfRouterLsa = *ospfRouterLsa | reserved;
}


// /**
// FUNCTION     :: Ospfv2RouterLSAGetABRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of abr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2RouterLSAGetABRouter(unsigned char ospfRouterLsa)
{
    unsigned char abr = ospfRouterLsa;

    //clears all the bits except eighth bit
    abr = abr & maskChar(8, 8);

    //Right shifts so that last bit represents areaBorderRouter
    abr = RshiftChar(abr, 8);

    return (BOOL)abr;
}


// /**
// FUNCTION     :: Ospfv2RouterLSAGetASBRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of asbr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2RouterLSAGetASBRouter(unsigned char ospfRouterLsa)
{
    unsigned char asbr = ospfRouterLsa;

    //clears all the bits except seventh bit
    asbr = asbr & maskChar(7, 7);

    //Right shifts so that last bit represents ASBoundaryRouter
    asbr = RshiftChar(asbr, 7);

    return (BOOL)asbr;
}


// /**
// FUNCTION     :: Ospfv2RouterLSAGetVirtLnkEndPt()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of vlep for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2RouterLSAGetVirtLnkEndPt(unsigned char ospfRouterLsa)
{
    unsigned char vlep = ospfRouterLsa;

    //clears all the bits except sixth bit
    vlep = vlep & maskChar(6, 6);

    //Right shifts so that last bit represents virtualLinkEndpoint
    vlep = RshiftChar(vlep, 6);

    return (BOOL)vlep;
}


// /**
// FUNCTION     :: Ospfv2RouterLSAGetWCMCReceiver()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of wcmr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2RouterLSAGetWCMCReceiver(unsigned char ospfRouterLsa)
{
    unsigned char wcmr = ospfRouterLsa;

    //clears all the bits except fifth bit
    wcmr = wcmr & maskChar(5, 5);

    //Right shifts so that last bit represents wildCardMulticastReceiver
    wcmr = RshiftChar(wcmr, 5);

    return (BOOL)wcmr;
}


// /**
// FUNCTION     :: Ospfv2RouterLSAGetReserved1()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of reserved1 for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of ,
//                                    areaBorderRouter,ASBoundaryRouter,
//                                    virtualLinkEndpoint,
//                                    wildCardMulticastReceiver and reserved1
// RETURN       :: unsigned char
// **/
static unsigned char Ospfv2RouterLSAGetReserved1(unsigned char ospfRouterLsa)
{
    unsigned char reserved = ospfRouterLsa;

    //clears the last four bits
    reserved = reserved & maskChar(1, 4);

    return reserved;
}


// Structure of network LSA.
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;
    // Dynamically allocating next part.
} Ospfv2NetworkLSA;

// Structure of summary LSA
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;
    // Dynamically allocate this part
} Ospfv2SummaryLSA;

// BGP-OSPF Patch Start
// External link info for AS external LSA.
typedef struct
{
    NodeAddress networkMask;
    UInt32 ospfMetric;      //eBit:1, reserved:7, metric:24;
    NodeAddress forwardingAddress;
    NodeAddress externalRouterTag;
} Ospfv2ExternalLinkInfo;

// /**
// FUNCTION     :: Ospfv2ExternalLinkSetMetric()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of metric for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32* : The variable containing the value of metric,
//                          reserved and eBit
// + metric     :UInt32 : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2ExternalLinkSetMetric(UInt32 *ospfMetric, UInt32 metric)
{
    //masks metric within boundry range
    metric = metric & maskInt(9, 32);

    //clears first 24 bits
    *ospfMetric = *ospfMetric & maskInt(25, 32);

    //setting the value of metric in ospfMetric
    *ospfMetric = *ospfMetric | LshiftInt(metric, 24);
}


// /**
// FUNCTION     :: Ospfv2ExternalLinkSetReserved()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of reserved for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32* : The variable containing the value of metric,
//                          reserved and eBit
// + reserved   : UInt32 : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2ExternalLinkSetReserved(UInt32 *ospfMetric,
                                          UInt32 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskInt(26, 32);

    //clears 25-31 bits
    *ospfMetric = *ospfMetric & (~(maskInt(25, 31)));

    //setting the value of reserved in ospfMetric
    *ospfMetric = *ospfMetric | LshiftInt(reserved, 31);
}


// /**
// FUNCTION     :: Ospfv2ExternalLinkSetEBit()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of eBit for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32* : The variable containing the value of metric,
//                          reserved and eBit
// + eBit       : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
static void Ospfv2ExternalLinkSetEBit(UInt32 *ospfMetric, BOOL eBit)
{
    UInt32 x = eBit;

    //masks eBit within boundry range
    x = x & maskInt(32, 32);

    //clears the 32nd bit
    *ospfMetric = *ospfMetric & (~(maskInt(32, 32)));

    //setting the value of x in ospfMetric
    *ospfMetric = *ospfMetric | x;
}


// /**
// FUNCTION     :: Ospfv2ExternalLinkGetMetric()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of metric for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32: The variable containing the value of metric,
//                        reserved and eBit
// RETURN       :: UInt32
// **/
static UInt32 Ospfv2ExternalLinkGetMetric(UInt32 ospfMetric)
{
    UInt32 metric = ospfMetric;

    //clears the last 8 bits
    metric = metric & maskInt(1, 24);

    //right shift 8 bits so that last 24 bits represent metric
    metric = RshiftInt(metric, 24);

    return metric;
}


// /**
// FUNCTION     :: Ospfv2ExternalLinkGetReserved()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of reserved for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32: The variable containing the value of metric,
//                        reserved and eBit
// RETURN       :: UInt32
// **/
static UInt32 Ospfv2ExternalLinkGetReserved(UInt32 ospfMetric)
{
    UInt32 reserved = ospfMetric;

    //clears all the bits except 25-31
    reserved = reserved & maskInt(25, 31);

    //right shift 1 bits so that last 7 bits represent reserved
    reserved = RshiftInt(reserved, 31);

    return reserved;
}


// /**
// FUNCTION     :: Ospfv2ExternalLinkGetEBit()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of eBit for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32: The variable containing the value of metric,
//                        reserved and eBit
// RETURN       :: BOOL
// **/
static BOOL Ospfv2ExternalLinkGetEBit(UInt32 ospfMetric)
{
    UInt32 eBit = ospfMetric;

    //clears all bits except 32nd
    eBit = eBit & maskInt(32, 32);

    return (BOOL) eBit;
}


// Structure of AS external LSA
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;
    // Dynamically allocate this part
} Ospfv2ASexternalLSA;
// BGP-OSPF Patch End

/***** Start: OPAQUE-LSA *****/
// Structure of Type-11 (AS-scoped) Opaque LSA
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;

    // Dynamically allocating next part.
} Ospfv2ASOpaqueLSA;

// /**
// FUNCTION     :: Ospfv2OpaqueSetOpaqueId()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of opaque ID for Opaque LSA linkstatID
// PARAMETERS   ::
// + linkStateID : NodeAddress *: The variable containing the value of
//                                 linkStateID
// + opaqueId    : UInt32 : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2OpaqueSetOpaqueId(NodeAddress *linkStateID,
                                    UInt32 opaqueId)
{
    UInt32 opaqueData = (UInt32)(*linkStateID);

    //masks opaqueId within boundry range
    opaqueId = opaqueId & maskInt(9, 32);

    //clears first 24 bits
    opaqueData = opaqueData & maskInt(25, 32);

    //setting the value of opaqueId in opaqueData
    opaqueData = opaqueData | LshiftInt(opaqueId, 24);

    *linkStateID = (NodeAddress)opaqueData;
}

// /**
// FUNCTION     :: Ospfv2OpaqueSetOpaqueType()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of opaque Type for Opaque LSA linkstatID
// PARAMETERS   ::
// + linkStateID : NodeAddress *: The variable containing the value of
//                                 linkStateID
// + opaqueType  : UInt32: Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2OpaqueSetOpaqueType(NodeAddress *linkStateID,
                                      UInt32 opaqueType)
{
    UInt32 opaqueData = (UInt32)(*linkStateID);

    //masks opaqueType within boundry range
    opaqueType = opaqueType & maskInt(25, 32);

    //clears last 8 bits
    opaqueData = opaqueData & maskInt(1, 24);

    //setting the value of opaqueType in opaqueData
    opaqueData = opaqueData | opaqueType;

    *linkStateID = (NodeAddress)opaqueData;
}

// /**
// FUNCTION     :: Ospfv2OpaqueGetOpaqueId()
// LAYER        :: NETWORK
// PURPOSE      :: Get the value of opaque ID from Opaque LSA linkstatID
// PARAMETERS   ::
// + linkStateID : NodeAddress : The variable containing the value of
//                               linkStateID
// RETURN       ::  UInt32: Opaque ID value from linkStateID
// **/
static UInt32 Ospfv2OpaqueGetOpaqueId(NodeAddress linkStateID)
{
    UInt32 opaqueData = (UInt32)linkStateID;

    //clears the last 8 bits
    opaqueData = opaqueData & maskInt(1, 24);

    //right shift 8 bits so that last 24 bits represent metric
    opaqueData = RshiftInt(opaqueData, 24);

    return opaqueData;
}

// /**
// FUNCTION     :: Ospfv2OpaqueGetOpaqueType()
// LAYER        :: NETWORK
// PURPOSE      :: Get the value of opaque Type from Opaque LSA linkstatID
// PARAMETERS   ::
// + linkStateID : NodeAddress : The variable containing the value of
//                               linkStateID
// RETURN       ::  unsigned char: Opaque Type value from linkStateID
// **/
static unsigned char Ospfv2OpaqueGetOpaqueType(NodeAddress linkStateID)
{
    UInt32 opaqueData = (UInt32)linkStateID;

    //clears the first 24 bits
    opaqueData = opaqueData & maskInt(25, 32);

    return (unsigned char)opaqueData;
}

/***** End: OPAQUE-LSA *****/

//-------------------------------------------------------------------------//
//                         OSPFv2 PACKET FORMAT                            //
//-------------------------------------------------------------------------//
// Packet Type Consants //
typedef enum
{
    OSPFv2_HELLO                    = 1,
    OSPFv2_DATABASE_DESCRIPTION     = 2,
    OSPFv2_LINK_STATE_REQUEST       = 3,
    OSPFv2_LINK_STATE_UPDATE        = 4,
    OSPFv2_LINK_STATE_ACK           = 5,
    // ROSPF
    OSPFv2_ROSPF_DBDG               = 7,
    OSPFv2_ROSPF_REDIRECT           = 8,
    OSPFv2_ROSPF_CONFIG             = 0xA,
    OSPFv2_ROSPF_CONFIG_ACK         = 0xB
} Ospfv2PacketType;

// 24 byte common header. //
typedef struct
{
    unsigned char version;
    unsigned char packetType;
    short int packetLength;
    NodeAddress routerID;
    unsigned int areaID;
    short int checkSum;
    short int authenticationType;  // Not used in simulation.
    char authenticationData[8];    // Not used in simulation.
} Ospfv2CommonHeader;

#ifdef ADDON_BOEINGFCS
// ** New for ROSPF **
typedef struct
{
    unsigned char opaqueType;
    short int opaqueId;
    NodeAddress routerID;
    int seqNum;
    short int checkSum;           // Not used in simulation.
    short int length;
} Ospfv2OpaqueHeader;
// ** end of New for ROSPF **
#endif

// Hello packet structure.
typedef struct
{
    Ospfv2CommonHeader commonHeader;
    NodeAddress networkMask;
    short int helloInterval;        // In seconds.
    unsigned char options;          // Options field
    unsigned char rtrPri;
    int routerDeadInterval;         // In seconds.
    NodeAddress designatedRouter;
    NodeAddress backupDesignatedRouter;
    // Neighbors will be dynamically allocated.
} Ospfv2HelloPacket;

// Update packet structure.
typedef struct
{
    Ospfv2CommonHeader commonHeader;
    int numLSA;
    // List of LSA (allocated dynamically).
} Ospfv2LinkStateUpdatePacket;

// Database Description packet structure
typedef struct
{
    Ospfv2CommonHeader commonHeader;
    unsigned short interfaceMtu;
    unsigned char options;
    unsigned char ospfDbDp;//reserved:5, init:1, more:1, masterSlave:1;
    unsigned int ddSequenceNumber;
    // List of LSA Header (allocated dynamically)
} Ospfv2DatabaseDescriptionPacket;

// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetMS()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of ms for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved2
// + ms         : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetMS(unsigned char *ospfDbDp,
                                                 BOOL ms)
{
    unsigned char x = (unsigned char)ms;

    //masks ms within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *ospfDbDp = *ospfDbDp & (~(maskChar(1, 1)));

    //setting the value of x in ospfDbDp
    *ospfDbDp = *ospfDbDp | LshiftChar(x, 1);
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetMore()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of more for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved2
// + more       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetMore(unsigned char *ospfDbDp,
                                                   BOOL more)
{
    unsigned char x = (unsigned char)more;

    //masks more within boundry range
    x = x & maskChar(8, 8);

    //clears the second bit
    *ospfDbDp = *ospfDbDp & (~(maskChar(2, 2)));

    //setting the value of x in ospfDbDp
    *ospfDbDp = *ospfDbDp | LshiftChar(x, 2);
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetInit()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of init for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved2
// + init       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetInit(unsigned char *ospfDbDp,
                                                   BOOL init)
{
    unsigned char x = (unsigned char)init;

    //masks init within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *ospfDbDp = *ospfDbDp & (~(maskChar(3, 3)));

    //setting the value of x in ospfDbDp
    *ospfDbDp = *ospfDbDp | LshiftChar(x, 3);
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetRsrvd2()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of reserved2 for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved2
// + reserved2  : BOOL : Input value for set operation
// RETURN      :: void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetRsrvd2(unsigned char *ospfDbDp,
                                                     unsigned char reserved2)
{
    //masks reserved within boundry range
    reserved2 = reserved2 & maskChar(4, 8);

    //clears the last five bits
    *ospfDbDp = *ospfDbDp & maskChar(1, 3);

    //setting the value of reserved2 in ospfDbDp
    *ospfDbDp = *ospfDbDp | reserved2;
}

/***** Start: OPAQUE-LSA *****/
// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetO()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of O-bit for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + reserved1  : unsigned char* : The variable containing the value of
//                                 reserved1
// + Obit       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetO(unsigned char *reserved1,
                                                 BOOL Obit)
{
    unsigned char x = (unsigned char)Obit;

    //masks O-bit within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *reserved1 = *reserved1 & (~(maskChar(7, 7)));

    //setting the value of x in reserved1
    *reserved1 = *reserved1 | LshiftChar(x, 7);
}

// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetE()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of E-bit for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + reserved1  : unsigned char* : The variable containing the value of
//                                 reserved1
// + Obit       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
static void Ospfv2DatabaseDescriptionPacketSetE(unsigned char *reserved1,
                                                BOOL Ebit)
{
    unsigned char x = (unsigned char)Ebit;

    //masks E-bit within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *reserved1 = *reserved1 & (~(maskChar(2, 2)));

    //setting the value of x in reserved1
    *reserved1 = *reserved1 | LshiftChar(x, 2);
}
/***** End: OPAQUE-LSA *****/

// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetMS()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of ms for
//                 Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved2
// RETURN       :: BOOL
// **/
static BOOL Ospfv2DatabaseDescriptionPacketGetMS(unsigned char ospfDbDp)
{
    unsigned char ms = ospfDbDp;

    //clears all the bits except first bit
    ms = ms & maskChar(1, 1);

    //Right shifts so that last bit represents masterSlave
    ms = RshiftChar(ms, 1);

    return (BOOL)ms;
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetMore()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of more for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved2
// RETURN       :: BOOL
// **/
static BOOL Ospfv2DatabaseDescriptionPacketGetMore(unsigned char ospfDbDp)
{
    unsigned char more = ospfDbDp;

    //clears all the bits except second bit
    more = more & maskChar(2, 2);

    //Right shifts so that last bit represents more
    more = RshiftChar(more, 2);

    return (BOOL)more;
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetInit()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of init for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved2
// RETURN VALUE :: BOOL
// **/
static BOOL Ospfv2DatabaseDescriptionPacketGetInit(unsigned char ospfDbDp)
{
    unsigned char init = ospfDbDp;

    //clears all the bits except third bit
    init = init & maskChar(3, 3);

    //Right shifts so that last bit represents init
    init = RshiftChar(init, 3);

    return (BOOL)init;
}


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetRsrvd2()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of reserved2 for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved2
// RETURN VALUE :: BOOL
// **/
static unsigned char Ospfv2DatabaseDescriptionPacketGetRsrvd2
(unsigned char ospfDbDp)
{
    unsigned char reserved2 = ospfDbDp;

    //clears the first three bits
    reserved2 = reserved2 & maskChar(4, 8);

    return reserved2;
}

/***** Start: OPAQUE-LSA *****/
// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetO()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of O-bit for
//                 Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + reserved1  : unsigned char : The variable containing the value of
//                                reserved1
// RETURN       :: BOOL
// **/
static BOOL Ospfv2DatabaseDescriptionPacketGetO(unsigned char reserved1)
{
    unsigned char Obit = reserved1;

    //clears all the bits except seventh bit
    Obit = Obit & maskChar(7, 7);

    //Right shifts so that last bit represents Opaque capability
    Obit = RshiftChar(Obit, 7);

    return (BOOL)Obit;
}
/***** End: OPAQUE-LSA *****/

// Item for Link State Request packet
typedef struct
{
    Ospfv2LinkStateType linkStateType;
    NodeAddress linkStateID;
    NodeAddress advertisingRouter;
#ifdef ADDON_MA
    NodeAddress mask;
#endif
} Ospfv2LSRequestInfo;

// Link State Request packet structure
typedef struct
{
    Ospfv2CommonHeader commonHeader;
    // This part will be allocated dynamically
} Ospfv2LinkStateRequestPacket;

// Ack packet structure.
typedef struct
{
    Ospfv2CommonHeader commonHeader;
    // List of LSA Header (allocated dynamically)
} Ospfv2LinkStateAckPacket;

//-------------------------------------------------------------------------//
//                   STATISTICS COLLECTED BY PROTOCOL                      //
//-------------------------------------------------------------------------//

// Statistics structure
typedef struct
{
    int numHelloSent;
    int numHelloRecv;

    int numLSUpdateSent;
    unsigned int numLSUpdateBytesSent;
    int numLSUpdateRecv;
    unsigned int numLSUpdateBytesRecv;

    int numAckSent;
    int numAckBytesSent;
    int numAckRecv;
    int numAckBytesRecv;

    int numLSUpdateRxmt;
    int numExpiredLSAge;

    int numDDPktSent;
    unsigned int numDDPktBytesSent;
    int numDDPktRecv;
    int numDDPktRxmt;
    unsigned int numDDPktBytesRxmt;
    unsigned int numDDPktBytesRecv;

    int numLSReqSent;
    unsigned int numLSReqBytesSent;
    int numLSReqRecv;
    int numLSReqRxmt;
    unsigned int numLSReqBytesRecv;

    int numRtrLSAOriginate;
    int numNetLSAOriginate;
    int numSumLSAOriginate;
    int numASExtLSAOriginate;
    /***** Start: OPAQUE-LSA *****/
    int numASOpaqueOriginate;

    /***** End: OPAQUE-LSA *****/

    int numLSARefreshed;

    BOOL alreadyPrinted;

    int numDoNotAgeLSASent;
    int numDoNotAgeLSARecv;
}
Ospfv2Stats;


//-------------------------------------------------------------------------//
//                       INTERFACE DATA STRUCTURE                          //
//-------------------------------------------------------------------------//

// Possible interface types.
typedef enum
{
    OSPFv2_NON_OSPF_INTERFACE,
    OSPFv2_POINT_TO_POINT_INTERFACE,
    OSPFv2_BROADCAST_INTERFACE,
    OSPFv2_NBMA_INTERFACE,
    OSPFv2_POINT_TO_MULTIPOINT_INTERFACE,
#ifdef ADDON_BOEINGFCS
    OSPFv2_ROSPF_INTERFACE,
#endif
    OSPFv2_VIRTUAL_LINK_INTERFACE
} Ospfv2InterfaceType;

// Different interface state.
typedef enum
{
    S_Down,
    S_Loopback,
    S_Waiting,
    S_PointToPoint,
    S_DROther,
    S_Backup,
    S_DR
} Ospfv2InterfaceState;

// Event that cause interface state change.
typedef enum
{
    E_InterfaceUp,
    E_WaitTimer,
    E_BackupSeen,
    E_NeighborChange,
    E_LoopInd,
    E_UnloopInd,
    E_InterfaceDown
} Ospfv2InterfaceEvent;


// To keep available bandwidth and average delay for a queue.
// This information is required for Q-OSPF.
typedef struct
{
    unsigned char queueNo;
    int           linkBandwidth;
    int           utilizedBandwidth;
    int           availableBandwidth;
    int           propDelay;
    int           qDelay;
} Ospfv2StatusOfQueue;

// Various values kept for each interface.
typedef struct
{
    Ospfv2InterfaceType type;
    Ospfv2InterfaceState state;
#ifdef ADDON_BOEINGFCS
    Ospfv2InterfaceState newDRState;
    Ospfv2InterfaceState oldDRState;
    int drCost;
#endif

    // Interface based Stats
    Ospfv2Stats interfaceStats;

    int interfaceIndex;
    NodeAddress address;
    NodeAddress subnetMask;

    //virtual Link support
    BOOL isVirtualInterface;

    unsigned int areaId;

    clocktype helloInterval;
    clocktype routerDeadInterval;
    clocktype infTransDelay;

    int routerPriority;

    BOOL floodTimerSet;
    Ospfv2List* updateLSAList;

    BOOL delayedAckTimer;
    Ospfv2List* delayedAckList;

    clocktype networkLSAOriginateTime;  // Used only if DR on interface.
    BOOL networkLSTimer;                // Used only if DR on interface.
    int networkLSTimerSeqNumber;        // Used only if DR on interface.

#ifdef ADDON_BOEINGFCS
    BOOL networkLSFlushTimer;
#endif

    BOOL ISMTimer;

    Message* waitTimerMsg;
    NodeAddress designatedRouter;
    NodeAddress designatedRouterIPAddress;

    NodeAddress backupDesignatedRouter;
    NodeAddress backupDesignatedRouterIPAddress;

    int outputCost;

    clocktype rxmtInterval;

    // Not used in simulation.
    int AuType;
    char* authenticationKey;

    // This field is required for Q-OSPF, as the available
    // bandwidth and average delay for each queue of a interface is
    // required for the advertisement.Each item of the list contains
    // Ospfv2StatusOfQueue structure.
    LinkedList* presentStatusOfQueue;
    int  lastAdvtUtilizedBandwidth;

    Ospfv2List* neighborList;

    BOOL useDemandCircuit;
    BOOL ospfIfDemand;
    BOOL helloSuppressionSuccess;
    BOOL includeSubnetRts;

    Ospfv2NonBroadcastNeighborList* NonBroadcastNeighborList;

} Ospfv2Interface;


//-------------------------------------------------------------------------//
//                        NEIGHBOR DATA STRUCTURE                          //
//-------------------------------------------------------------------------//

typedef enum
{
    S_NeighborDown,
    S_Attempt,
    S_Init,
    S_TwoWay,
    S_ExStart,
    S_Exchange,
    S_Loading,
    S_Full
} Ospfv2NeighborState;

typedef enum
{
    E_HelloReceived,
    E_Start,
    E_TwoWayReceived,
    E_NegotiationDone,
    E_ExchangeDone,
    E_BadLSReq,
    E_LoadingDone,
    E_AdjOk,
    E_SeqNumberMismatch,
    E_OneWay,
    E_KillNbr,
    E_InactivityTimer,
    E_LLDown
} Ospfv2NeighborEvent;

typedef enum
{
    T_Master,
    T_Slave
} Ospfv2MasterSlaveType;

typedef struct
{
    Ospfv2LinkStateHeader* LSHeader;
    unsigned int seqNumber;
#ifdef ADDON_MA
    NodeAddress mask;
#endif
} Ospfv2LSReqItem;

// Information needed for each neighbor.
typedef struct
{
    Ospfv2NeighborState state;

    // Indicates that no Hello packet has been
    // seen from this neighbor recently.
    unsigned int inactivityTimerSequenceNumber;

    // Indicates that no ACK packet is received
    // for LSA sent out.
    unsigned int rxmtTimerSequenceNumber;

    Ospfv2MasterSlaveType masterSlave;
    unsigned int DDSequenceNumber;
    Message* lastReceivedDDPacket;
    NodeAddress neighborID;
    int neighborPriority;
    NodeAddress neighborIPAddress;
    unsigned char neighborOptions;
    NodeAddress neighborDesignatedRouter;
    NodeAddress neighborBackupDesignatedRouter;

    // Lists of LSA Update packets.
    Ospfv2List* linkStateRetxList;
    Ospfv2List* linkStateSendList;
    BOOL LSRetxTimer;

    Ospfv2List* DBSummaryList;
    Ospfv2List* linkStateRequestList;

    // Retransmission purpose
    clocktype lastDDPktSent;
    Message* lastSentDDPkt;
#ifdef ADDON_BOEINGFCS
    BOOL isMobileLeafChild;
    BOOL newMobileLeafChild;
    NodeId neighborNodeId;
    int ddReTxCount;
    class RoutingCesRospfDbdgNeighbor* dbdgNbr;
#endif
    unsigned int LSReqTimerSequenceNumber;

    /***** Start: OPAQUE-LSA *****/
     //This will indicate the opaque capability of the nbr
    unsigned char dbDescriptionNbrOptions;
    /***** End: OPAQUE-LSA *****/
    //This will keep the options bits of last generated DD packet
    UInt8 optionsBits;
} Ospfv2Neighbor;

typedef struct
{
    NodeAddress     routerID;
    int             routerPriority;
    NodeAddress     routerIPAddress;
    int             routerOptions;
    NodeAddress     DesignatedRouter;
    NodeAddress     BackupDesignatedRouter;
} Ospfv2DREligibleRouter;


//-------------------------------------------------------------------------//
//                      ROUTING TABLE STRUCTURE                            //
//-------------------------------------------------------------------------//

typedef enum
{
    OSPFv2_DESTINATION_ABR,
    OSPFv2_DESTINATION_ASBR,
    OSPFv2_DESTINATION_ABR_ASBR,
    OSPFv2_DESTINATION_NETWORK,
    OSPFv2_DESTINATION_ROUTER
} Ospfv2DestType;

typedef enum
{
    OSPFv2_INTRA_AREA,
    OSPFv2_INTER_AREA,
    OSPFv2_TYPE1_EXTERNAL,
    OSPFv2_TYPE2_EXTERNAL
} Ospfv2PathType;

typedef enum
{
    OSPFv2_ROUTE_INVALID,
    OSPFv2_ROUTE_CHANGED,
    OSPFv2_ROUTE_NO_CHANGE,
    OSPFv2_ROUTE_NEW
} Ospfv2Flag;

// A row struct for routing table
typedef struct
{
    // Type of destination, either network or router
    Ospfv2DestType           destType;

    // Router ID for router or IP Address for network
    NodeAddress                     destAddr;

    // Only defined for network
    NodeAddress                     addrMask;

    // Optional capabilities
    char                            option;

    // Area whose LS info has led to this entry
    NodeAddress                     areaId;

    // Type of path to destination
    Ospfv2PathType           pathType;

    // Cost to destination
    Int32                           metric;

    // Only for Type 2 cost
    Int32                           type2Metric;

    // Valid only for Intra-Area paths
    void*                            LSOrigin;

    // Next hop router address
    // TBD: Enable equal cost multipath later
    NodeAddress                     nextHop;
    NodeAddress                     outIntf;

    // Valid only for intra-area and AS-External path
    NodeAddress                     advertisingRouter;

    Ospfv2Flag               flag;
} Ospfv2RoutingTableRow;

// Routing table kept by ospf.
typedef struct
{
    int         numRows;
    DataBuffer  buffer;
} Ospfv2RoutingTable;

//-------------------------------------------------------------------------//
//                            AREA DATA STRUCTURE                          //
//-------------------------------------------------------------------------//

typedef struct
{
    unsigned int    address;
    unsigned int    mask;
    BOOL            advertise;
    BOOL            advrtToArea[OSPFv2_MAX_ATTACHED_AREA];

    void*           area;
} Ospfv2AreaAddressRange;

typedef struct
{
   NodeAddress address;
   NodeAddress mask;
   char* LSA;               //used for nssaAdvertise case only in translation
   Ospfv2PathType pathType; //used for nssaAdvertise case only in translation
   BOOL wasExactMatched;    //used for nssaAdvertise case only in translation
} NSSASummaryAddress;


typedef struct
{
   NodeAddress address;
   NodeAddress mask;
   unsigned int areaId;
} NetworkSummaryLSA;


typedef struct
{
    NodeId nodeId;
    NodeAddress hostIpAddress;
    unsigned int cost;
    unsigned int areaId;
} Ospfv2HostRoutes;


typedef struct
{
    unsigned int        areaID;
    Ospfv2List*         areaAddrRange;

    Ospfv2List*         connectedInterface;

    // LSDB for the area
    Ospfv2List*         routerLSAList;
    Ospfv2List*         networkLSAList;
    Ospfv2List*         routerSummaryLSAList;
    Ospfv2List*         networkSummaryLSAList;

    // Timer associated with self originated LSAs
    BOOL                routerLSTimer;
    clocktype           routerLSAOriginateTime;

    // for group membership LSA
    Ospfv2List*         groupMembershipLSAList;

    // Timer associated with self originated LSAs
    BOOL                groupMembershipLSTimer;
    clocktype           groupMembershipLSAOriginateTime;

    BOOL                transitCapability;
    BOOL                externalRoutingCapability;
    BOOL                isNSSAEnabled;
    BOOL                isNSSANoSummary;
    BOOL                isNSSANoRedistribution;

    // Shortest path tree of this area
    Ospfv2List*         shortestPathList;

    Int32               stubDefaultCost;

    Ospfv2List*         maxAgeLSAList;
} Ospfv2Area;


#ifdef ADDON_BOEINGFCS

// STRUCT      :: HEPp2plinkMappingType
// DESCRIPTION :: It is used to hold the AMD for CES_HAIPE
typedef struct hep_p2p_link_mapping_type
{
    NodeAddress destRedInterface;
    NodeAddress srcRedInterface;
    clocktype redLinkTime;
    hep_p2p_link_mapping_type* next;
    hep_p2p_link_mapping_type* prev;
}HEPP2PLinkMappingType;

// STRUCT      :: HEPP2PLinkData
// DESCRIPTION :: It is used to hold the CES_HAIPE P2P link Data
typedef struct
{
    HEPP2PLinkMappingType* redP2PTable;
    unsigned char       numOfP2PLinksEntries;
}HEPP2PLinkData;

#endif

#ifdef ADDON_MA
typedef struct
{
    NodeAddress     externalAddr;
    NodeAddress     mask;
    int             cost;

}Ospfv2ExternalCostPolicy;
#endif

//-------------------------------------------------------------------------//
//                   PROTOCOL DATA STRUCTURE                               //
//-------------------------------------------------------------------------//

typedef struct
{
    BOOL defaultInfoOriginate;
    BOOL always;
    unsigned int metricValue;
    unsigned int metricType;
    RouteMap* routeMap;
} DefaultInfo;

// OSPFv2 Layer structure for node
typedef struct
{
    unsigned int asID;
    NodeAddress routerID;
    Ospfv2List* area;
    BOOL partitionedIntoArea;
    BOOL areaBorderRouter;
    BOOL nssaAreaBorderRouter;
    BOOL nssaCandidateNode;

    // Backbone kept seperately from other areas
    Ospfv2Area* backboneArea;
    clocktype SPFTimer;

    // Virtual link not considered yet

// BGP-OSPF Patch Start
    // Only in ASBR
    Ospfv2List* asExternalLSAList;
    BOOL asBoundaryRouter;
// BGP-OSPF Patch End
    Ospfv2List* nssaExternalLSAList;

    BOOL isAdvertSelfIntf;
    BOOL collectStat;
    Ospfv2Stats stats;

    Ospfv2RoutingTable routingTable;

    Ospfv2Interface* iface;

    int neighborCount;

    // parameter used to check multicast capability of a node
    BOOL isMospfEnable;

    // pointer used to indicate the type of multicast routing protocol used
    void * multicastRoutingProtocol;

    // whether qos is enabled in this node
    BOOL isQosEnabled;

    // This pointer will hold layer structure of Q-OSPF if QOS is required
    void* qosRoutingProtocol;

    // number of queue advertisement
    int numQueueAdvertisedForQos;

    // holding delayed start time of ospf process.
    clocktype staggerStartTime;

#ifdef ADDON_BOEINGFCS
    BOOL displayRospfIcons;

    //ces haipe
    HEPP2PLinkData* hepP2PLinkData;

    int maxNumRetx;

#endif

    clocktype incrementTime;
    clocktype spfCalcDelay;
    clocktype floodTimer;

    // random seed for use with broadcast jitter, flood timers, etc.
    RandomSeed seed;

    DefaultInfo defaultInfo;
    BOOL nssaDefInfoOrg;
    std::vector <NSSASummaryAddress> nssaAdvertiseList;
    std::vector <NSSASummaryAddress> nssaNotAdvertiseList;
    std::vector <NetworkSummaryLSA> networkSummaryLSAAdvertiseList;
    std::vector <NetworkSummaryLSA> networkSummaryLSANotAdvertiseList;

    std::vector <Ospfv2HostRoutes> hostRoutes;

#ifdef ADDON_MA
    Ospfv2List* costPolicyList;
    // This parameter just indicates the scenarion is running with MA
    // The current node may not be MA/ RMA node
    BOOL        maMode;
    BOOL        sendLSAtoMA;
#endif

    /***** Start: OPAQUE-LSA *****/
    // Opaque Capability of router
    BOOL  opaqueCapable;
    // Type-11 Opaque LSDB
    Ospfv2List* ASOpaqueLSAList;
    // Indication of whether self origination of type-11 opaque LSA
    // is under progress
    BOOL ASOpaqueLSTimer;
    clocktype ASOpaqueLSAOriginateTime;
    /***** End: OPAQUE-LSA *****/

    BOOL supportDC;   //For enabling Demand circuit

    BOOL maxAgeLSARemovalTimerSet;
} Ospfv2Data;


//-------------------------------------------------------------------------//
//                         VARIOUS TIMER INFORMATION                       //
//-------------------------------------------------------------------------//
// Information about various timers being triggered.
typedef struct
{
    unsigned int sequenceNumber;
    NodeAddress neighborAddr;
    int interfaceIndex;
    // For retransmission timer.
    NodeAddress advertisingRouter;

    Ospfv2PacketType type;
} Ospfv2TimerInfo;

typedef struct
{
    int interfaceId;
    NodeAddress nbrAddr;
    Ospfv2NeighborEvent nbrEvent;
} Ospfv2NSMTimerInfo;

typedef struct
{
    int interfaceId;
    Ospfv2InterfaceEvent intfEvent;
} Ospfv2ISMTimerInfo;


//////////////////////// End of Structure declarataion //////////////////////

// /**
// FUNCTION     :: Ospfv2CheckLSAForDoNotAge()
// LAYER        :: NETWORK
// PURPOSE      :: Checks router LSA for DoNotAge bit
// PARAMETERS   ::
// + ospf          :  Ospfv2Data pointer:contains ospf data for a router
// + routerLSAAge  :  short int : contains the router LSA Age
// RETURN       :: BOOL
// **/
static BOOL Ospfv2CheckLSAForDoNotAge(Ospfv2Data* ospf, short int routerLSAAge)
{
    //checking router that whether it supports DoNotage or not
    if (ospf->supportDC)
    {
        if (routerLSAAge & OSPFv2_DO_NOT_AGE)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        //if router does not support DoNotAge so DoNotAge bit does not
        //matter for it because in anyway router will treat it as an age
        return FALSE;
    }
}

// /**
// FUNCTION     :: Ospfv2MaskDoNotAge()
// LAYER        :: NETWORK
// PURPOSE      :: Masks DoNotAge bit in LSA
// PARAMETERS   ::
// + ospf          :  Ospfv2Data pointer:contains ospf data for a router
// + routerLSAAge  :  short int : contains the age of router LSA
// RETURN       :: short int
// **/
static unsigned short int Ospfv2MaskDoNotAge(Ospfv2Data* ospf,
                                             short int routerLSAAge)
{
    if (Ospfv2CheckLSAForDoNotAge(ospf , routerLSAAge))
    {
        //Mask the DoNotAge bit
        return routerLSAAge & ~OSPFv2_DO_NOT_AGE;
    }
    else
    {
        return routerLSAAge;
    }
}


// /**
// FUNCTION     :: Ospfv2DifferenceOfTwoLSAAge()
// LAYER        :: NETWORK
// PURPOSE      :: subtract two LSA Age
// PARAMETERS   ::
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// + age1  :  short int : contains the age1 as operand1
// + age2  :  short int : contains the age2 as operand2
// RETURN       :: short int
// **/
static short int Ospfv2DifferenceOfTwoLSAAge(Ospfv2Data* ospf,
                                             short int age1,
                                             short int age2)
{
    age1 = Ospfv2MaskDoNotAge(ospf, age1);
    age2 = Ospfv2MaskDoNotAge(ospf, age2);

    return (age1 - age2);
}

// /**
// FUNCTION     :: Ospfv2AssignNewAgeToLSAAge()
// LAYER        :: NETWORK
// PURPOSE      :: assign NEW AGE to LSA age
// PARAMETERS   ::
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// + routerLSAAge  :  short int : contains the LSA Age
// + newLSAAge  :  short int : contains the LSA Age to be assigned
// RETURN       :: None
// **/
static void Ospfv2AssignNewAgeToLSAAge(Ospfv2Data* ospf,
                                       short unsigned int &routerLSAAge,
                                       const short unsigned int newLSAAge)
{
    if (Ospfv2CheckLSAForDoNotAge(ospf, routerLSAAge))
    {
       routerLSAAge = OSPFv2_DO_NOT_AGE | newLSAAge;
    }
    else
    {
       routerLSAAge = newLSAAge;
    }
}

// /**
// FUNCTION     :: Ospfv2LinkStateHeaderSetDoNotAge()
// LAYER        :: NETWORK
// PURPOSE      :: Set DoNotAge bit in LSA Header
// PARAMETERS   ::
// + routerLSAAge  :  short int : contains the router LSA Age
// RETURN       :: None
// **/
static void Ospfv2LinkStateHeaderSetDoNotAge(
    short unsigned int &routerLSAAge)
{
    routerLSAAge = routerLSAAge | OSPFv2_DO_NOT_AGE;
}


void Ospfv2Init(Node* node,
                Ospfv2Data** ospfLayerPtr,
                const NodeInput* nodeInput,
                BOOL rospf,
#ifdef ADDON_NGCNMS
                BOOL ifaceReset,
#endif
                int interfaceIndex);

void Ospfv2Finalize(Node* node);

void Ospfv2HandleRoutingProtocolEvent(Node* node, Message* msg);

void Ospfv2HandleRoutingProtocolPacket(Node* node,
                                       Message* msg,
                                       NodeAddress sourceAddress,
                                       NodeAddress destinationAddress,
                                       NodeId sourceId,
                                       int interfaceIndex);

void Ospfv2RouterFunction(Node* node,
                          Message* msg,
                          NodeAddress destAddr,
                          NodeAddress prevHopAddr,
                          BOOL* packetRouted);

void Ospfv2InterfaceStatusHandler(Node* node,
                                  int interfaceIndex,
                                  MacInterfaceState state);
// /**
// FUNCTION   :: Ospfv2PrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA header trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + msg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void Ospfv2PrintTraceXML(Node* node, Message* msg);

// Used by MOSPF other than OSPFv2
BOOL Ospfv2FloodLSA(Node* node,
                    int interfaceIndex,
                    char* LSA,
                    NodeAddress sourceAddr,
                    unsigned int areaId);

// Used by MOSPF other than OSPFv2
Ospfv2RoutingTableRow* Ospfv2GetValidRoute(Node* node,
                                           NodeAddress destAddr,
                                           Ospfv2DestType destType,
                                           int interfaceIndex = ANY_INTERFACE);

// Used by MOSPF for now. Find most specific match for destAddr.
Ospfv2RoutingTableRow* Ospfv2GetRouteToSrcNet(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType);

Ospfv2RoutingTableRow* Ospfv2GetPreferredPath(
    Node* node,
    NodeAddress destAddr,
    Ospfv2DestType destType);

// Used by MOSPF other than OSPFv2
Ospfv2LinkStateHeader* Ospfv2LookupLSAList(Ospfv2List* list,
                                           NodeAddress advertisingRouter,
                                           NodeAddress linkStateID);
#ifdef ADDON_MA
Ospfv2LinkStateHeader* Ospfv2LookupLSAList_Extended(
                                            Node* node,
                                            Ospfv2List* list,
                                            NodeAddress advertisingRouter,
                                            NodeAddress linkStateID,
                                            NodeAddress mask);
int Ospfv2FindCostFromPolicy(Node* node, NodeAddress destAddr);
#endif

// Used by MOSPF other than OSPFv2
void Ospfv2PrintRoutingTable(Node* node);

// Used by MOSPF other than OSPFv2
void Ospfv2PrintNetworkLSAList(Node* node);

BOOL Ospfv2RoutesMatchSame(
          Ospfv2RoutingTableRow* newRoute,
          Ospfv2RoutingTableRow* oldRoute);

// Used by MOSPF other than OSPFv2
BOOL Ospfv2CreateLSHeader(Node* node,
                          unsigned int areaId,
                          Ospfv2LinkStateType LSType,
                          Ospfv2LinkStateHeader* LSHeader,
                          Ospfv2LinkStateHeader* oldLSHeader);

// Used by MOSPF other than OSPFv2
BOOL Ospfv2InstallLSAInLSDB(Node* node,
                            Ospfv2List* list,
                            char* LSA,
                            unsigned int areaId);

// Used by MOSPF other than OSPFv2
void Ospfv2RemoveLSAFromLSDB(
    Node* node,
    char* LSA,
    unsigned int areaId);

// Used by MOSPF other than OSPFv2
Ospfv2AreaAddressRange* Ospfv2GetAddrRangeInfo(Node* node,
                                               unsigned int areaId,
                                               NodeAddress destAddr);

// Used by MOSPF other than OSPFv2
void Ospfv2PrintLSHeader(Ospfv2LinkStateHeader* LSHeader);

// Used by MOSPF and QOSPF other than OSPFv2
Ospfv2Area* Ospfv2GetArea(Node* node, unsigned int areaID);

// Used by MOSPF and QOSPF other than OSPFv2
void Ospfv2PrintLSA(char* LSA);

// Used by QOSPF other than OSPFv2
void Ospfv2OriginateRouterLSAForThisArea(
    Node* node,
    unsigned int areaId,
    BOOL flush,
    BOOL flood = TRUE);

// BGP-OSPF Patch Start
// Used by BGP OR other EGP protocol
BOOL Ospfv2IsEnabled(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush);
// BGP-OSPF Patch End

// Hooking function for Route Redistribution
void Ospfv2HookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* cost);

// Used by ROSPF for mobile leaf and SRW domain
// AS-external-LSA distribution
void Ospfv2OriginateASExternalLSA(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush);

// NSSA-external-LSA distribution
void Ospfv2OriginateNSSAExternalLSA(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush);

void Ospfv2ScheduleSPFCalculation(Node* node);

void Ospfv2OriginateNetworkLSA(
    Node* node,
    int interfaceIndex,
    BOOL flush);


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLCommonHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet common header information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + header  : Ospfv2CommonHeader* : Pointer to header
// RETURN ::  void : NULL
// **/
void Ospfv2PrintTraceXMLCommonHeader(Node* node, Ospfv2CommonHeader* header);


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLPacketOptions
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet common header information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + options  : unsigned char : Packet options
// RETURN ::  void : NULL
// **/
void Ospfv2PrintTraceXMLPacketOptions(Node* node, unsigned char options);


// /**
// FUNCTION   :: Ospfv2PrintTraceXMLLsaHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA header trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv2LinkStateHeader* : Pointer to Ospfv2LinkStateHeader
// RETURN ::  void : NULL
// **/
void Ospfv2PrintTraceXMLLsaHeader(Node* node,
                                  Ospfv2LinkStateHeader* lsaHdr);
// *********************** Functions needed for ROSPF ***********************

void Ospfv2CreateCommonHeader(Node* node,
                              Ospfv2CommonHeader* commonHdr,
                              Ospfv2PacketType pktType);

void Ospfv2PrintHelloPacket(Ospfv2HelloPacket* helloPkt);

void Ospfv2HandleHelloPacket(Node* node,
                             Ospfv2HelloPacket* helloPkt,
                             NodeAddress sourceAddr,
                             NodeId sourceId,
                             int interfaceIndex);

void Ospfv2InsertToList(Ospfv2List* list,
                        clocktype timeStamp,
                        void* data);

void Ospfv2InsertToNonBroadcastNeighborList(
                                      Ospfv2NonBroadcastNeighborList* list,
                                      NodeAddress addr);

Ospfv2Neighbor* Ospfv2GetNeighborInfoByIPAddress(
    Node* node,
    int interfaceIndex,
    NodeAddress neighborAddr);

void Ospfv2InitList(Ospfv2List** list);

void Ospfv2InitNonBroadcastNeighborList(
                                    Ospfv2NonBroadcastNeighborList** list);

void Ospfv2FreeList(Node* node, Ospfv2List* list, BOOL type);

void Ospfv2ScheduleNeighborEvent(
    Node* node,
    int interfaceId,
    NodeAddress nbrAddr,
    Ospfv2NeighborEvent nbrEvent);

void Ospfv2SetInterfaceState(
    Node* node,
    int interfaceIndex,
    Ospfv2InterfaceState newState);

int Ospfv2GetInterfaceForThisNeighbor(Node* node, NodeAddress neighbor);

void Ospfv2ScheduleSPFCalculation(Node* node);

void Ospfv2QueueLSAToFlood(
    Node* node,
    Ospfv2Interface* thisInterface,
    char* LSA,
    BOOL setDoNotAge
    );

void Ospfv2AddToLSRetxList(
    Node* node,
    int interfaceIndex,
    Ospfv2Neighbor* nbrInfo,
    char* LSA,
    int* referenceCount);

void Ospfv2SendLSUpdate(Node* node, NodeAddress nbrAddr, int interfaceId);


Ospfv2ListItem* Ospfv2GetLSAListItem(
    Ospfv2List* list,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID);

void Ospfv2ScheduleSummaryLSA(
    Node* node,
    NodeAddress destAddr,
    NodeAddress addrMask,
    Ospfv2DestType destType,
    unsigned int areaId,
    BOOL flush);

void Ospfv2OriginateSummaryLSA(
    Node* node,
    Ospfv2RoutingTableRow* advRt,
    Ospfv2Area* thisArea,
    BOOL flush);

Ospfv2LinkStateHeader* Ospfv2LookupLSAListByID(
    Ospfv2List* list,
    NodeAddress linkStateID);

void Ospfv2GetNeighborStateString(
    Ospfv2NeighborState state,
    char* str);

clocktype Ospfv2GetHelloInterval(Node* node, int interfaceIndex);
clocktype Ospfv2GetDeadInterval(Node* node, int interfaceIndex);
void Ospfv2PrintNeighborState(Node* node);
BOOL Ospfv2RospfDisplayIcons(Node* node);

void Ospfv2SetTimer(
    Node* node,
    int interfaceIndex,
    int eventType,
    NodeAddress neighborAddr,
    clocktype timerDelay,
    unsigned int sequenceNumber,
    NodeAddress advertisingRouter,
    Ospfv2PacketType type);

void Ospfv2HandleNeighborEvent(
    Node* node,
    int interfaceIndex,
    NodeAddress nbrAddr,
    Ospfv2NeighborEvent eventType);

Ospfv2LinkStateHeader* Ospfv2LookupLSDB(
    Node* node,
    char linkStateType,
    NodeAddress advertisingRouter,
    NodeAddress linkStateID,
    int interfaceIndex);

BOOL Ospfv2NoNbrInStateExhangeOrLoading(
    Node* node,
    int interfaceIndex);

int Ospfv2CompareLSA(
    Node* node,
    Ospfv2LinkStateHeader* LSHeader,
    Ospfv2LinkStateHeader* oldLSHeader);

BOOL Ospfv2UpdateLSDB(
    Node* node,
    int interfaceIndex,
    char* LSA,
    NodeAddress srcAddr,
    unsigned int areaId);

Ospfv2LinkStateHeader* Ospfv2SearchRequestList(
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader);

void Ospfv2RemoveFromList(
    Node* node,
    Ospfv2List* list,
    Ospfv2ListItem* listItem,
    BOOL type);

BOOL Ospfv2ListIsEmpty(Ospfv2List* list);

void Ospfv2SendDelayedAck(Node* node,
                          int interfaceIndex,
                          char* LSA,
                          NodeAddress nbrAddr);

void Ospfv2SendUpdatePacket(
    Node* node,
    NodeAddress dstAddr,
    int interfaceId,
    char* payload,
    int payloadSize,
    int LSACount);

void Ospfv2SendDirectAck(
        Node* node,
        int interfaceIndex,
        Ospfv2List* ackList,
        NodeAddress nbrAddr);

void Ospfv2ScheduleNetworkLSA(Node* node, int interfaceId, BOOL flush);

BOOL Ospfv2RequestedLSAReceived(Ospfv2Neighbor* nbrInfo);

void Ospfv2SendLSRequestPacket(
    Node* node,
    NodeAddress nbrAddr,
    int interfaceIndex,
    BOOL retx);

BOOL Ospfv2DRFullyAdjacentWithAnyRouter(Node* node, int interfaceId);

UInt16 ComputeLsaChecksum(
    UInt8* lsa,
    int length);

void Ospfv2AddType2Link(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo);

void Ospfv2AddType3Link(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo);

void Ospfv2AddType1Link(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* tempNbInfo,
    Ospfv2LinkInfo** linkInfo
#ifdef ADDON_BOEINGFCS
    ,NodeAddress nbrAddr = 0,
    int cost = -1
#endif
);

BOOL Ospfv2IsPresentInNextHopList(
    Ospfv2List* nextHopList,
    Ospfv2NextHopListItem* nextHopListItem);

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SendLSUpdateToNeighbor()
// PURPOSE      :Send Updated LSA to neighbor
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

void Ospfv2SendLSUpdateToNeighbor(Node* node,
                                  NodeAddress nbrAddr,
                                  int interfaceId,
                                  BOOL deleteLSAsFromUpdateList = TRUE);

void Ospfv2DeleteList(Node* node, Ospfv2List* list, BOOL type);

int Ospfv2GetInterfaceIndexFromlinkData(Node* node, NodeAddress linkData);

void Ospfv2AddSelfRoute(
    Node* node,
    int interfaceId,
    Ospfv2LinkInfo** linkInfo);

//
// NAME         : Ospfv2FlushAllDoNotAgeFromArea
// PURPOSE      : Flush All DoNotAge From Area
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf
//              router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2FlushAllDoNotAgeFromArea(Node* node,
                                    Ospfv2Data* ospf,
                                    Ospfv2Area* thisArea);

//
// NAME         : Ospfv2SearchRouterDBforDC
// PURPOSE      : Search Router Database for LSA for Dc bit 0 or 1
//                depending on the last parameter
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchRouterDBforDC(Ospfv2Data* ospf,
                               Ospfv2Area* area,
                               BOOL dc);

//
// NAME         : Ospfv2StoreUnreachableTimeStampForAllLSA
// PURPOSE      : Store the Unreachable timestamp for each LSA
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf
//            router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2StoreUnreachableTimeStampForAllLSA(Node* node, Ospfv2Data* ospf);

//
// NAME         : Ospfv2SearchAddressInForwardingTable
// PURPOSE      : Search Advertising router of the LSA in network IP
//                Forwarding Table.If found then return True else
//                False
// PARAMETERS   ::
// + node       :  Node pointer:contains the node pointer of an ospf
//                 router
// + advRouter  :  NodeAddress:contains the interface address of the
//                 advertising router of the LSA
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchAddressInForwardingTable(Node* node,
                                            NodeAddress advRouter);

//
// NAME         : Ospfv2RemoveStaleDoNotAgeLSA
// PURPOSE      : Remove stale Do Not Age
// PARAMETERS   ::
// + node  :  Node pointer:contains the node pointer of an ospf router
// + ospf  :  Ospfv2Data pointer:contains ospf data for a router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2RemoveStaleDoNotAgeLSA(Node* node, Ospfv2Data* ospf);

//
// NAME         : Ospfv2CheckLSAToFloodOverDC ()
// PURPOSE      : Check whether the LSA should be flooded over demand
//                circuit or notHandle Link State Request packet
// PARAMETERS   ::
// +node           :Node pointer
// +interfaceId    :interface id on which LSA needs to be flooded
// +LSHeader       :LS Header of the LSA to be flooded
// ASSUMPTION   :None.
// RETURN VALUE :BOOL
//
BOOL Ospfv2CheckLSAToFloodOverDC(Node* node,
                                 int interfaceId,
                                 Ospfv2LinkStateHeader* LSHeader);

//
// NAME         : Ospfv2SearchAreaForLSAExceptIndLSAWithDC
// PURPOSE      : Search Area For LSA Except Indication LSA with DC
//                bit as given by dc (parameter)
// PARAMETERS   ::
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
//
BOOL Ospfv2SearchAreaForLSAExceptIndLSAWithDC(Ospfv2Area* area,
                                              BOOL dc);

//
// NAME         : Ospfv2SearchAreaForIndLSAAndCmpRouterIds()
// PURPOSE      : Search Area For Indication LSA and compare the router Id of
//                its originator with the current router and if an indication LSA
//                is found with greater router id then return FALSE
// PARAMETERS   ::
// + ospf  :  Ospf Data pointer:contains the ospf data for an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2SearchAreaForIndLSAAndCmpRouterIds(Ospfv2Data* ospf,
                                              Ospfv2Area* area);

//
// NAME         : Ospfv2CheckForIndicationLSA()
// PURPOSE      : Check LSA for indication LSA
// PARAMETERS   ::
// + Ospfv2RouterLSA  :  Router LSA
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2CheckForIndicationLSA(Ospfv2RouterLSA* LSA);

void Ospfv2CreateIndicationLSHeader(Node* node,
                                    Ospfv2LinkStateHeader* LSHeader);

//
// NAME         : Ospfv2SearchAreaForLSAWithDCOrgByOthers()
// PURPOSE      : Search Area For LSA with DC bit as set by the "dc"
//                parameter
//                and if their are LSA originated by other routers then
//                return TRUE
//                to originate indication LSA
// PARAMETERS   ::
// + ospf  :  Ospf Data pointer:contains the ospf data for an ospf
//            router
// + area  :  Ospfv2Area pointer:contains ospf area for a router
// + dc    :  BOOL:contains 0 to search LSA with DC bit 0 and 1 to
//            search LSA with DC bit 1
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
//
BOOL Ospfv2SearchAreaForLSAWithDCOrgByOthers(Ospfv2Data* ospf,
                                             Ospfv2Area* area,
                                             BOOL dc);

//
// NAME         : Ospfv2OriginateIndicationLSA()
// PURPOSE      : Originate indication LSAs
// PARAMETERS   ::
// +node  :Node pointer
// +thisArea  :Area Pointer in which LSA are to be flooded
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2OriginateIndicationLSA(Node* node,
                                  Ospfv2Area* thisArea);

//
// NAME         : Ospfv2IndicatingAcrossAreasForDoNotAge
// PURPOSE      : indicate across area boundaries about DoNotAge
//                incapable routers
// PARAMETERS   ::
// + node       :  Node pointer:contains the node pointer of an ospf
//                 router
// ASSUMPTION   :None.
// RETURN VALUE :None.
//
void Ospfv2IndicatingAcrossAreasForDoNotAge(Node* node);

//
// NAME         : Ospfv2CheckABRForLSAwithDC
// PURPOSE      : Area border router checks the area for LSA with DC
//                bit clear
// PARAMETERS   ::
// + ospf       :  Ospf data pointer:contains the node pointer of an ospf
//                 router
// + currentArea: Area in which LSAs with DC bit clear needs to be searched
// ASSUMPTION   :None.
// RETURN VALUE :BOOL.
//
BOOL Ospfv2CheckABRForLSAwithDC(Ospfv2Data* ospf,
                                Ospfv2Area* currentArea);

//-------------------------------------------------------------------------//
// NAME: Ospfv2LSAHasMaxAge()
// PURPOSE: Check whether the given LSA has MaxAge
// RETURN: TRUE if age is MaxAge, FALSE otherwise
//-------------------------------------------------------------------------//

BOOL Ospfv2LSAHasMaxAge(Ospfv2Data* ospf, char* LSA);

BOOL Ospfv2LSAsContentsChangedForDC(Ospfv2Data*ospf,
                                    Ospfv2LinkStateHeader* newLSHeader,
                                    Ospfv2LinkStateHeader* oldLSHeader);


std::string Ospfv2GetLinkTypeString(
    unsigned char type);

void Ospfv2DestroyAdjacency(
    Node* node,
    Ospfv2Neighbor* nbrInfo,
    int interfaceIndex);

void Ospfv2SetNeighborState(
    Node* node,
    int interfaceIndex,
    Ospfv2Neighbor* neighborInfo,
    Ospfv2NeighborState state
#ifdef ADDON_BOEINGFCS
    , BOOL reFormAdj = FALSE
#endif
);



#ifdef ADDON_MA
void Ospfv2ScheduleASExternalLSA_ForMA(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int external2Cost,
    BOOL flush,
    clocktype delay);
#endif

void Ospfv2RemoveLSAFromList(
    Node* node,
    Ospfv2List* list,
    char* LSA);

//-------------------------------------------------------------------------//
// NAME         :Ospfv2SearchSendList
// PURPOSE      :Search for a LSA in Link State Send list
// ASSUMPTION   :None
// RETURN VALUE :Return LSA if found, otherwise return NULL
//-------------------------------------------------------------------------//

Ospfv2ListItem* Ospfv2SearchSendList(
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader);

void Ospfv2SendDatabaseDescriptionPacket(
    Node* node,
    NodeAddress nbrAddr,
    int interfaceIndex);

void Ospfv2ScheduleInterfaceEvent(
        Node* node,
        int interfaceId,
        Ospfv2InterfaceEvent intfEvent);

// needed by ROSPF database digest
BOOL Ospfv2NeighborIsInStubArea(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* nbrInfo);
BOOL Ospfv2NeighborIsInNSSAArea(
    Node* node,
    int interfaceId,
    Ospfv2Neighbor* nbrInfo);

#endif // OSPFv2_H


// /**
// API       :: OspfFindLinkStateChecksum
// PURPOSE   :: Finds link state checksum for a state header and its contents.
// PARAMETERS::
// + lsh : Ospfv2LinkStateHeader* : he start of the link state header
// RETURN :: short unsigned int : Checksum calculated
// **/

short unsigned int OspfFindLinkStateChecksum(
    Ospfv2LinkStateHeader* lsh);


//OSPF options field
//   * | O | DC | EA | N/P | MC | E | T

// Current Implimentation
//   * | O | DC | * | N/P | MC | E | T
// T-bit : router's QOS capability
// E-bit : describes router's external routing capabilities. If E-bit is set,
//         the area is said to be transit (external capability) area.
// MC-bit: describes the multicast capability of the various pieces of the
//         OSPF routing domain. When calculating the path of multicast
//         datagrams, only those link state advertisements having their
//         MC-bit set are used. (rfc-1584)
// N-bit : describes the router's NSSA capability.  The N-bit is used only
//         in Hello packets. When the N-bit is set, it means that the router
//         will send and receive Type-7 LSAs on that interface. (rfc-3101)
// P-bit : is used only in the Type-7 LSA header.  It flags the NSSA border
//         router to translate the Type-7 LSA into a Type-5 LSA. The default
//         setting for the P-bit is clear. (rfc-3101)
// EA-bit: describes router's willingness to receive and forward External-
//         Attributes-LSAs.
// DC-bit: describes the handling of demand circuits. (rfc-1793)
// O-bit : describes the router's willingness to receive and forward
//         Opaque-LSAs (rfc-2370)

// /**
// FUNCTION     :: Ospfv2OptionsSetQOS()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of qos for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + qos        : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetQOS(unsigned char *options, BOOL qos);


// /**
// FUNCTION     :: Ospfv2OptionsSetExt()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of external for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + external   : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetExt(unsigned char *options, BOOL external);


// /**
// FUNCTION     :: Ospfv2OptionsSetMulticast()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of multicast for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + multicast  : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetMulticast(unsigned char *options, BOOL multicast);


// /**
// FUNCTION     :: Ospfv2OptionsSetNSSACapability()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of NSSA capability for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + np         : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetNSSACapability(unsigned char *options,
                                           BOOL np);


// /**
// FUNCTION     :: Ospfv2OptionsSetDC()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of demandCircuit for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + dc         : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetDC(unsigned char *options, BOOL dc);


// /**
// FUNCTION     :: Ospfv2OptionsSetOpaque()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of opaque capability for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char*
// + opaque     : BOOL :Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2OptionsSetOpaque(unsigned char *options, BOOL opaque);


// /**
// FUNCTION     :: Ospfv2OptionsGetQOS()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of QOS for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetQOS(unsigned char options);


// /**
// FUNCTION     :: Ospfv2OptionsGetExt()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of external for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetExt(unsigned char options);


// /**
// FUNCTION     :: Ospfv2OptionsGetMulticast()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of multicast for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetMulticast(unsigned char options);


// /**
// FUNCTION     :: Ospfv2OptionsGetNSSACapability()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of NSSA capability for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetNSSACapability(unsigned char options);


// /**
// FUNCTION     :: Ospfv2OptionsGetDC()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of demandCircuit for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetDC(unsigned char options);


// /**
// FUNCTION     :: Ospfv2OptionsGetOpaque()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of opaque capability for Ospfv2Options
// PARAMETERS   ::
// + options    : unsigned char
// RETURN       :: BOOL
// **/
BOOL Ospfv2OptionsGetOpaque(unsigned char options);


// /**
// FUNCTION     :: Ospfv2RouterLSASetABRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of areaBorderRouter for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// + abr           : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2RouterLSASetABRouter(unsigned char *ospfRouterLsa,
                                       BOOL abr);


// /**
// FUNCTION     :: Ospfv2RouterLSASetASBRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of ASBoundaryRouter for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// + asbr          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2RouterLSASetASBRouter(unsigned char *ospfRouterLsa,
                                        BOOL asbr);


// /**
// FUNCTION     :: Ospfv2RouterLSASetVirtLnkEndPt()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of virtualLinkEndpoint for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// + vlep          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2RouterLSASetVirtLnkEndPt(unsigned char *ospfRouterLsa,
                                           BOOL vlep);


// /**
// FUNCTION     :: Ospfv2RouterLSASetWCMCReceiver()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of wildCardMulticastReceiver for
//                Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// + wcmr          : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2RouterLSASetWCMCReceiver(unsigned char *ospfRouterLsa,
                                           BOOL wcmr);


// /**
// FUNCTION     :: Ospfv2RouterLSASetNSSATranslation()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of NSSATranslation for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char* : The variable containing the value of
//                                    reserved1, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// + Nt            : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2RouterLSASetNSSATranslation(unsigned char *ospfRouterLsa,
                                       BOOL Nt);


// /**
// FUNCTION     :: Ospfv2RouterLSAGetABRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of abr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// RETURN       :: BOOL
// **/
BOOL Ospfv2RouterLSAGetABRouter(unsigned char ospfRouterLsa);


// /**
// FUNCTION     :: Ospfv2RouterLSAGetASBRouter()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of asbr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// RETURN       :: BOOL
// **/
BOOL Ospfv2RouterLSAGetASBRouter(unsigned char ospfRouterLsa);


// /**
// FUNCTION     :: Ospfv2RouterLSAGetVirtLnkEndPt()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of vlep for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// RETURN       :: BOOL
// **/
BOOL Ospfv2RouterLSAGetVirtLnkEndPt(unsigned char ospfRouterLsa);


// /**
// FUNCTION     :: Ospfv2RouterLSAGetWCMCReceiver()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of wcmr for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char : The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// RETURN       :: BOOL
// **/
BOOL Ospfv2RouterLSAGetWCMCReceiver(unsigned char ospfRouterLsa);


// /**
// FUNCTION     :: Ospfv2RouterLSAGetNSSATranslation()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of Nt for Ospfv2RouterLSA
// PARAMETERS   ::
// + ospfRouterLsa : unsigned char :  The variable containing the value of
//                                    reserved, NSSATranslation,
//                                    wildCardMulticastReceiver,
//                                    virtualLinkEndpoint,ASBorderRouter and
//                                    areaBoundaryRouter
// RETURN       :: BOOL
// **/
BOOL Ospfv2RouterLSAGetNSSATranslation(unsigned char ospfRouterLsa);


// /**
// FUNCTION     :: Ospfv2ExternalLinkSetMetric()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of metric for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32* : The variable containing the value of eBit,
//                          reserved and metric
// + metric     :UInt32 : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2ExternalLinkSetMetric(UInt32 *ospfMetric, UInt32 metric);


// /**
// FUNCTION     :: Ospfv2ExternalLinkSetEBit()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of eBit for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32* : The variable containing the value of eBit,
//                          reserved and metric
// + eBit       : BOOL : Input value for set operation
// RETURN       :: void : NULL
// **/
void Ospfv2ExternalLinkSetEBit(UInt32 *ospfMetric, BOOL eBit);


// /**
// FUNCTION     :: Ospfv2ExternalLinkGetMetric()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of metric for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32: The variable containing the value of eBit,
//                          reserved and metric
// RETURN       :: UInt32
// **/
UInt32 Ospfv2ExternalLinkGetMetric(UInt32 ospfMetric);


// /**
// FUNCTION     :: Ospfv2ExternalLinkGetEBit()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of eBit for Ospfv2ExternalLinkInfo
// PARAMETERS   ::
// + ospfMetric : UInt32: The variable containing the value of eBit,
//                          reserved and metric
// RETURN       :: BOOL
// **/
BOOL Ospfv2ExternalLinkGetEBit(UInt32 ospfMetric);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetMS()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of ms for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved
// + ms         : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
void Ospfv2DatabaseDescriptionPacketSetMS(unsigned char *ospfDbDp,
                                                 BOOL ms);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetMore()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of more for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved
// + more       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
void Ospfv2DatabaseDescriptionPacketSetMore(unsigned char *ospfDbDp,
                                                   BOOL more);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketSetInit()
// LAYER        :: NETWORK
// PURPOSE      :: Set the value of init for Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char* : The variable containing the value of
//                                 masterSlave,more,init and reserved
// + init       : BOOL : Input value for set operation
// RETURN       ::  void : NULL
// **/
void Ospfv2DatabaseDescriptionPacketSetInit(unsigned char *ospfDbDp,
                                                   BOOL init);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetMS()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of ms for
//                 Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved
// RETURN       :: BOOL
// **/
BOOL Ospfv2DatabaseDescriptionPacketGetMS(unsigned char ospfDbDp);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetMore()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of more for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved
// RETURN       :: BOOL
// **/
BOOL Ospfv2DatabaseDescriptionPacketGetMore(unsigned char ospfDbDp);


// /**
// FUNCTION     :: Ospfv2DatabaseDescriptionPacketGetInit()
// LAYER        :: NETWORK
// PURPOSE      :: Returns the value of init for
//                Ospfv2DatabaseDescriptionPacket
// PARAMETERS   ::
// + ospfDbDp   : unsigned char : The variable containing the value of
//                                masterSlave,more,init and reserved
// RETURN VALUE :: BOOL
// **/
BOOL Ospfv2DatabaseDescriptionPacketGetInit(unsigned char ospfDbDp);

void Ospfv2ScheduleRouterLSA(
    Node* node,
    unsigned int areaId,
    BOOL flush);

void Ospfv2ScheduleNetworkLSA(Node* node, int interfaceId, BOOL flush);

BOOL Ospfv2FloodThroughAS(
    Node* node,
    int interfaceIndex,
    char* LSA,
    NodeAddress srcAddr);

/***** Start: OPAQUE-LSA *****/

// Added here due to requirement of processing and handling
// Type-11 in ROSPF
void Ospfv2RemoveFromRequestList(
    Node* node,
    Ospfv2List* list,
    Ospfv2LinkStateHeader* LSHeader);

// Added here due to requirement of processing and handling
// Type-11 in ROSPF
void Ospfv2RefreshLSA(
    Node* node,
    Ospfv2ListItem* LSAItem,
    unsigned int areaId);

// Added here due to requirement of processing and handling
// Type-11 in ROSPF
void Ospfv2FlushLSA(Node* node, char* LSA, unsigned int areaId);


void
Ospfv2ScheduleOpaqueLSA(Node* node,
                        Ospfv2LinkStateType LSType);

void Ospfv2PrintASOpaqueLSAList(Node* node);
/***** End: OPAQUE-LSA *****/

/***** Start: ROSPF Redirect *****/
void Ospfv2AddRoute(
    Node* node,
    Ospfv2RoutingTableRow* newRoute);

BOOL Ospfv2IsMyAddress(Node* node, NodeAddress addr);
/***** End: ROSPF Redirect *****/


//-------------------------------------------------------------------------//
// NAME: Ospfv2InsertToNeighborSendList
// PURPOSE: Inserts an item to neighbor send list
// RETURN: None.
//-------------------------------------------------------------------------//
void Ospfv2InsertToNeighborSendList(Ospfv2List* list,
                                    clocktype timeStamp,
                                    Ospfv2LinkStateHeader* LSHeader);


//-------------------------------------------------------------------------//
// NAME         :Ospfv2CopyLSA()
// PURPOSE      :Copy LSA.
// ASSUMPTION   :None.
// RETURN VALUE :char*
//-------------------------------------------------------------------------//
char* Ospfv2CopyLSA(char* LSA);


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2IsPresentInMaxAgeLSAList()
// LAYER        :: NETWORK
// PURPOSE      :: Check whether maxAgeItem is present in maxAgeList
// ASSUMPTION   :: None.
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + maxAgeList : Ospfv2List* : list containing all the MaxAge LSAs
// + MaxAgeLSAItem : Ospfv2LinkStateHeader* : item to be search in maxAgeList
// RETURN VALUE :: TRUE if present, FALSE otherwise
//-------------------------------------------------------------------------//
BOOL Ospfv2IsPresentInMaxAgeLSAList(
    Node* node,
    Ospfv2List* maxAgeList,
    Ospfv2LinkStateHeader* maxAgeItem);


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2RemoveMaxAgeLSAListFromLSDB()
// LAYER        :: NETWORK
// PURPOSE      :: Remove MaxAge LSAs from LSDB
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// RETURN VALUE :void
//-------------------------------------------------------------------------//
void Ospfv2RemoveMaxAgeLSAListFromLSDB(Node* node);


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2LsaIsSelfOriginated()
// LAYER        :: NETWORK
// PURPOSE      :: Check whether LSA is originated by node
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + LSA        : char* : LSA to be checked if it is originated by node
// RETURN VALUE :: BOOL
//-------------------------------------------------------------------------//
BOOL Ospfv2LsaIsSelfOriginated(Node* node, char* LSA);


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2AddLsaToMaxAgeLsaList()
// LAYER        :: NETWORK
// PURPOSE      :: To add the LSA to maxageLSA list and
//                 call function to schedule MaxAge LSA removal timer
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// + LSA        : char* : LSA to be added in maxAgeLSAList
// + areaId     : unsigned int : area whose maxAge LSAs need to be removed
// RETURN VALUE :: void
//-------------------------------------------------------------------------//
void Ospfv2AddLsaToMaxAgeLsaList(
    Node* node,
    char* LSA,
    unsigned int areaId);


//-------------------------------------------------------------------------//
// FUNCTION     :: Ospfv2ScheduleMaxAgeLSARemoveTimer()
// LAYER        :: NETWORK
// PURPOSE      :: To initiate MaxAge LSA removal timer
// PARAMETERS   ::
// + node       : Node* : Node pointer to current node doing the processing
// RETURN VALUE :: void
//-------------------------------------------------------------------------//
void Ospfv2ScheduleMaxAgeLSARemoveTimer(Node* node);


//-------------------------------------------------------------------------//
// FUNCTION      :: Ospfv2CompareLSAToListItem()
// LAYER         :: NETWORK
// PURPOSE       :: To add the LSA to maxageLSA list and
//                  call function to schedule MaxAge LSA removal timer
// PARAMETERS    ::
// + node        : Node* : Node pointer to current node doing the processing
// + LSHeader    : Ospfv2LinkStateHeader* : LSA to be compared
// + listLSHeader: Ospfv2LinkStateHeader* : LSA in list
// RETURN VALUE  :: BOOL
//-------------------------------------------------------------------------//
BOOL Ospfv2CompareLSAToListItem(
    Node* node,
    Ospfv2LinkStateHeader* LSHeader,
    Ospfv2LinkStateHeader* listLSHeader);

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingOspfv2HandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node*    : Pointer to Node structure
// + interfaceIndex          : Int32    : interface index
// + oldAddress              : Address* : old address
// + NetworkType networkType : type of network protocol
// RETURN     :: void        : NULL
//--------------------------------------------------------------------------
void RoutingOspfv2HandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);
