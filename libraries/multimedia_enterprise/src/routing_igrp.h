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


#ifndef IGRP_H
#define IGRP_H

#include "main.h"
#include "buffer.h"

#define DESTINATION_INACCESSABLE  0xFFFFFF

// define important default timer values of IGRP
// Ref  http://www.cisco.com/warp/public/103/5.html
// section "Details of Update Process"
#define IGRP_DEFAULT_BROADCAST_TIMER (90 * SECOND)
#define IGRP_DEFAULT_INVALID_TIMER   (3 * IGRP_DEFAULT_BROADCAST_TIMER)
#define IGRP_DEFAULT_HOLD_TIMER      (3 * IGRP_DEFAULT_BROADCAST_TIMER) + \
                                     (10 * SECOND)

#define IGRP_DEFAULT_FLUSH_TIMER     (7 * IGRP_DEFAULT_BROADCAST_TIMER)
#define IGRP_DEFAULT_PERIODIC_TIMER  (1 * SECOND)
#define IGRP_TRIGGER_DELAY           (5 * SECOND)
#define IGRP_MIN_SLEEP_TIME          (1 * SECOND)

#define IGRP_START_UP_MIN_DELAY (10 * MILLI_SECOND)

#define IGRP_TRIGGER_JITTER                    0.8
#define IGRP_DEFAULT_TOS                       1
#define IGRP_MAX_NUM_OF_TOS                    4
#define IGRP_ASSUMED_MAX_NUM_OF_ROUTE_ENTRIES  10
#define IGRP_INVALID_REMOTE_METRIC            -1
#define IGRP_DEFAULT_VARIANCE_VALUE            1
#define IGRP_DEFAULT_MAXIMUM_HOP_COUNT         100
#define IGRP_MINIMUM_NUM_HOPS                  5
#define IGRP_MAX_ROUTE_ADVERTISED              104
#define IGRP_DEFAULT_AS_ID                     1
#define IGRP_COST_INFINITY                    -1
typedef enum
{
    ADDRESS_TYPE_CLASS_A = 0,
    ADDRESS_TYPE_CLASS_B = 2,
    ADDRESS_TYPE_CLASS_C = 6,
    ADDRESS_TYPE_CLASS_D = 14
} IpAddressType;

// IGRP also advertises three types of routes:
// interior, system, and exterior,
// Ref http://www.cisco.com/univercd/cc/td/doc/product/software/
typedef enum
{
    IGRP_INTERIOR_ROUTE, // Interior routes are routes between
                         // subnets in the network attached
                         // to a router interface

    IGRP_SYSTEM_ROUTE,   // System routes are routes to
                         // networks within an autonomous system

    IGRP_EXTERIOR_ROUTE  // Exterior routes are routes to
                         // networks outside the autonomous
                         // system that are considered when
                         // identifying a gateway of last resort
} IgrpRouteType;

typedef enum
{
    IGRP_REQUEST = 1,
    IGRP_UPDATE
} IgrpOpcodeType;

// define IGRP Routing Information.
// Ref  http://www.cisco.com/warp/public/103/5.html
// section "Details of IP implementation"
typedef struct
{
    unsigned char number[3];       // 3 significant octets of IP address
    unsigned char delay[3];        // delay, in tens of microseconds
    unsigned char bandwidth[3];    // bandwidth, in units of 1 Kbit/sec
    short    mtu;                  // MTU, in octets
    unsigned char reliability;     // percent packets successfully tx/rx
    unsigned char load;            // percent of channel occupied
    unsigned char hopCount;        // hop count
} IgrpRoutingInfo;

typedef IgrpRoutingInfo IgrpMetricStruct;

// define IGRP datagram header
// Ref  http://www.cisco.com/warp/public/103/5.html
// section "Details of IP implementation"
typedef struct
{
    unsigned char  IgrpHdr;//version: 4,    // protocol version number
                   //opcode : 4;    // opcode
    unsigned char  edition;       // edition number
    unsigned short asystem;       // autonomous system number
    unsigned short ninterior;     // number of subnets in local net
    unsigned short nsystem;       // number of networks in AS
    unsigned short nexterior;     // number of networks outside AS
    unsigned short checksum;      // checksum of IGRP header and data
} IgrpHeader;

//-------------------------------------------------------------------------
// FUNCTION     : IgrpHeaderSetVersion()
//
// PURPOSE      : Set the value of version number for IgrpHeader
//
// PARAMETERS   : IgrpHdr - The variable containing the value of
//                          version and opcode
//                version - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void IgrpHeaderSetVersion(unsigned char *IgrpHdr, UInt32 version)
{
    unsigned char version_char = (unsigned char)version;

    //masks version within boundry range
    version_char = version_char & maskChar(5, 8);

    //clears first four bits
    *IgrpHdr = *IgrpHdr & maskChar(5, 8);

    //setting the value of version number in IgrpHdr
    *IgrpHdr = *IgrpHdr | LshiftChar(version_char, 4);
}


//-------------------------------------------------------------------------
// FUNCTION     : IgrpHeaderSetOpCode()
//
// PURPOSE      : Set the value of opcode for IgrpHeader
//
// PARAMETERS   : IgrpHdr - The variable containing the value of
//                          version and opcode
//                opcode  - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void IgrpHeaderSetOpCode(unsigned char *IgrpHdr, UInt32 opcode)
{
    unsigned char opcode_char = (unsigned char)opcode;

    //masks opcode within boundry range
    opcode_char = opcode_char & maskChar(5, 8);

    //clears last four bits
    *IgrpHdr = *IgrpHdr & maskChar(1, 4);

    //setting the value of opcode in IgrpHdr
    *IgrpHdr = *IgrpHdr | opcode_char;
}


//-------------------------------------------------------------------------
// FUNCTION     : IgrpHeaderGetVersion()
//
// PURPOSE      : Returns the value of version number for IgrpHeader
//
// PARAMETERS   :IgrpHdr - The variable containing the value of
//                         version and opcode
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 IgrpHeaderGetVersion(unsigned char IgrpHdr)
{
    unsigned char version = IgrpHdr;

    //clears last 4 bits
    version = version & maskChar(1, 4);

    //Right shift 4 bits
    version = RshiftChar(version, 4);

    return (UInt32)version;
}


//-------------------------------------------------------------------------
// FUNCTION     : IgrpHeaderGetOpCode()
//
// PURPOSE      : Returns the value of opcode for IgrpHeader
//
// PARAMETERS   : IgrpHdr - The variable containing the value of
//                          version and opcode
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 IgrpHeaderGetOpCode(unsigned char IgrpHdr)
{
    unsigned char opcode = IgrpHdr;

    //clears first 4 bits
    opcode = opcode & maskChar(5, 8);

    return (UInt32)opcode;
}


// define IGRP Message type:
// There are two kinds of IGRP messages: 1) request and 2)Update
// Ref  http://www.cisco.com/warp/public/103/5.html
// section "Details of IP implementation"

// 1) Request Message:
//      No data structure defined.
//      Request message consists of header only.

// 2) Update Message:
//      No data structure defined.
//      Update message consists of
//      (header + variable length IgrpRoutingInfo).

// define IGRP routing Table Type.
typedef struct
{
    IgrpMetricStruct vectorMetric;
    float metric;

    // The danger is that with a large enough variance, paths
    // become allowed that aren't just slower, but are actually
    // "in the wrong direction". Thus there should be an
    // additional rule to prevent traffic from being sent
    // "upstream": No traffic is sent along paths whose remote
    // composite metric (the composite metric calculated at
    // the next hop) is greater than the composite metric
    // calculated at the gateway
    // Ref http://www.cisco.com/warp/public/103/5.html
    // Section "Overall Description"
    // A path whose remote composite metric is greater than its
    // composite metric is a path whose next hop is "farther away"
    // from the destination, as measured by the metric. This is
    // referred to as an "upstream path." Normally one would
    // expect that the use of metrics would prevent upstream
    // paths from being chosen
    // Ref http://www.cisco.com/warp/public/103/5.html
    // Section "Packet Routing"
    float remoteCompositeMetric;

    unsigned short asystem;            // AS system number

    // entry type can be IGRP_INTERIOR_ROUTE, IGRP_SYSTEM_ROUTE
    // or IGRP_EXTERIOR_ROUTE
    unsigned char routeType;

    NodeAddress nextHop;
    int outgoingInterface;
    clocktype lastUpdateTime;
    BOOL isInHoldDownState;
    unsigned numOfPacketSend;
    BOOL isPermanent;

} IgrpRoutingTableEntry;


typedef struct
{
    int tos;              // the type of service

    // k1,k2,k3 and k4 are tos parameters that
    // are read from the configuration file
    // for each type of service  given
    int k1;
    int k2;
    int k3;
    int k4;
    int k5;

    // Each entry in this DataBuffer is of"IgrpRoutingTableEntry" type
    DataBuffer igrpRoutingTableEntry;
} IgrpRoutingTable;


// Structure collecting IGRP statistics
typedef struct
{
   BOOL isAlreadyPrinted;
   unsigned int numRegularUpdates;
   unsigned int numTriggeredUpdates;
   unsigned int numRouteTimeouts;
   unsigned int numResponsesReceived;
   unsigned int *packetSentStat;
} IgrpStat;


typedef struct
{
    float errorRate;
    float channelOccupency;
} InterfceInfo;


// Structure IGRP
typedef struct
{
    // IGRP is intended to handle multiple "types of service,"
    // A routing table is kept for each type of service.
    // Ref http://www.cisco.com/warp/public/103/5.html
    // section "Overall Description"
    // Each entry in this DataBuffer is of"IgrpRoutingTable" type
    DataBuffer igrpRoutingTable;

    // Edition is a serial number which is incremented whenever
    // there is a change in the routing table. (This is done in
    // those conditions in which an update is triggered in a
    // routing update.) The edition number allows gateways to
    // avoid processing updates containing information that they
    // have already seen. (This is not currently implemented.
    // That is, the edition number is generated correctly, but
    // it is ignored on input. Because it is possible for
    // packets to be dropped, it is not clear that the edition
    // number is sufficient to avoid duplicate processing.
    // It would be necessary to make sure that all of the
    // packets associated with the edition had been processed.)
    // Ref http://www.cisco.com/warp/public/103/5.html
    unsigned char edition;

    float variance;                  // IGRP varience
    short asystem;                   // the as Id

    unsigned char maxHop;            // maximum number of hops

    InterfceInfo* interfaceInfo;     // interface information,
                                     // kept per interface
    clocktype  broadcastTime;
    clocktype  invalidTime;
    clocktype  holdTime;
    clocktype  flushTime;
    clocktype  sleepTime;
    clocktype  lastRegularUpdateSend;
    clocktype  periodicTimer;        // For periodic processing.
                                     // Default value is 1 SECOND

    BOOL       holdDownEnabled;      // is holdDown enabled or
                                     // disabled. Default is enabled

    BOOL       splitHorizonEnabled;  // is splitHorizon enabled or
                                     // disabled. Default is enabled
    BOOL       recentlyTriggeredUpdateScheduled;
    BOOL       collectStat;
    IgrpStat   igrpStat;

    RandomSeed seed;
} RoutingIgrp;


// The information to be passed with hold down timer.
typedef struct
{
   IgrpRoutingInfo routingInfo;
   int interfaceIndex;
   int tos;
} IgrpHoldDownInfoType;


//-------------------------------------------------------------------------
// FUNCTION   : IgrpInit()
//
// PURPOSE    : Initializing Igrp Protocol
//
// ASSUMPTION : This Function will be called
//              from NetworkInit
//-------------------------------------------------------------------------
void IgrpInit(
    Node* node,
    RoutingIgrp** igrp,
    const NodeInput* nodeInput,
    int interfaceIndex);


//-------------------------------------------------------------------------
// FUNCTION   : IgrpHandleProtocolPacket()
//
// PURPOSE    : Handling the protocol packets like request
//              or update etc.
// ASSUMPTION : This function is assumed to be called from
//              the ip layer function DeliverPacket()
//-------------------------------------------------------------------------
void IgrpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    int interfaceIndex);


//-------------------------------------------------------------------------
// FUNCTION   : RoutingIgrpHandleProtocolEvent()
//
// PURPOSE    : Handling IgrpProtocol Timer releted Events
//              like expiration of timers.. etc.
// ASSUMPTION : This function is assumed to be called from
//              the ip layer event handling function
//              named "NetworkIpLayer()"
//-------------------------------------------------------------------------
void IgrpHandleProtocolEvent(Node *node, Message *msg);


//-------------------------------------------------------------------------
// FUNCTION     RoutingIgrpRouterFunction()
//
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              PacketWasRouted - variable that indicates to IP that it
//                                no longer needs to act on this data packet
//
// ASSUMPTION :The Following must match RouterFunctionType defined in ip.h
//-------------------------------------------------------------------------
void IgrpRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* PacketWasRouted);


//-------------------------------------------------------------------------
// FUNCTION     RoutingIgrpFinalize()
//
// PURPOSE      Printing Finalized statistics.
//
// PARAMETERS   node - node printing the statics.
//
// ASSUMPTION :The Following must match RouterFunctionType defined in ip.h
//-------------------------------------------------------------------------
void IgrpFinalize(Node* node);

// Hooking function for Route Redistribution
void IgrpHookToRedistribute(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* igrpRoutingInfo);

#endif
