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


//  PURPOSE: Simulate the HSRP protocol developed by CISCO following
//           RFC 2281.


#ifndef HSRP_H
#define HSRP_H

#include "list.h"

#include "types.h"

#define HSRP_MIN_GROUP_NUMBER               0
#define HSRP_MAX_GROUP_NUMBER               255

#define HSRP_DEFAULT_GROUP_NUMBER           0
#define HSRP_DEFAULT_HELLO_TIME             (3 * SECOND)
#define HSRP_DEFAULT_HOLD_TIME              (10 * SECOND)
#define HSRP_DEFAULT_PRIORITY               0
#define HSRP_DEFAULT_PREEMPTION_CAPABILITY  FALSE;

#define HSRP_HELLO_JITTER                   (50 * MILLI_SECOND)


#define HSRP_ATHENTICATION_DATA1            0x63697363
#define HSRP_ATHENTICATION_DATA2            0x6F000000

// Multicast address 224.0.0.2 where all nodes running HSRP must listen to.
#define HSRP_ALL_ROUTERS                    0xE0000002

// Possible states for HSRP.
typedef enum
{
    HSRP_INITIAL = 1,
    HSRP_LEARN   = 2,
    HSRP_LISTEN  = 3,
    HSRP_SPEAK   = 4,
    HSRP_STANDBY = 5,
    HSRP_ACTIVE  = 6
} HsrpState;


// Possible events for HSRP.
typedef enum
{
    HSRP_INTERFACE_UP,                                          // a
    HSRP_INTERFACE_DOWN,                                        // b
    HSRP_ACTIVE_TIMER_EXPIRY,                                   // c
    HSRP_STANDBY_TIMER_EXPIRY,                                  // d
    HSRP_HELLO_TIMER_EXPIRY,                                    // e
    HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_IN_SPEAK_STATE,          // f
    HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_ACTIVE,             // g
    HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_ACTIVE,              // h
    HSRP_RECEIVE_RESIGN_FROM_ACTIVE,                            // i
    HSRP_RECEIVE_COUP_HIGHER_PRIORITY,                          // j
    HSRP_RECEIVE_HELLO_HIGHER_PRIORITY_FROM_STANDBY,            // g
    HSRP_RECEIVE_HELLO_LOWER_PRIORITY_FROM_STANDBY              // h
} HsrpEvent;


typedef enum
{
    HSRP_HELLO              = 0,
    HSRP_COUP               = 1,
    HSRP_RESIGN             = 2
} HsrpPacketType;


// HSRP packet format.
typedef struct
{
    char version;
    char opCode;    
    char state; 
    char helloTime; 
    char holdTime;
    char priority;
    char group;
    char reserved;
    unsigned int authenticationData1;
    unsigned int authenticationData2;
    NodeAddress virtualIpAddress;
} HsrpPacket;


typedef struct
{
    UInt32 seqno;
    int interfaceIndex;
} HsrpTimerInfo;


typedef struct
{
    UInt32 activeSeqno;
    UInt32 standbySeqno;

    UInt32 helloSeqno;
} HsrpTimer;


typedef struct
{
    UInt32 numHelloSent;
    UInt32 numCoupSent;
    UInt32 numResignSent;
    UInt32 numHelloReceived;
    UInt32 numCoupReceived;
    UInt32 numResignReceived;
} HsrpStat;


// HSRP parmeters that must be maintained.
typedef struct
{
    int interfaceIndex;

    // Not used.
    NodeAddress virtualMacAddress;

    int priority;

    // Not used.
    int authenticationData;

    // Unit is in seconds.
    clocktype helloTime;
    clocktype holdTime;

    NodeAddress virtualIpAddress;
    BOOL preemptionCapability;

    BOOL helloTimeConfigured;
    BOOL holdTimeConfigured;
    BOOL virtualIpAddressConfigured;

    RandomSeed  jitterSeed;

    NodeAddress ipAddress;
    int group;
    HsrpState state;
    HsrpTimer timer;

    HsrpStat stat;
} HsrpInterfaceInfo;


typedef struct
{
    LinkedList* interfaceInfo;
} HsrpData;


// NAME:        RoutingHsrpLayer.
// PURPOSE:     Handles all messages sent to HSRP.
// RETURN:      None.

void
RoutingHsrpLayer(
    Node *node,
    Message *msg);


// NAME:        RoutingHsrpInit.
// PURPOSE:     Handling all initializations needed by static routing.
// RETURN:      None.

void
RoutingHsrpInit(
    Node *node,
    const NodeInput *nodeInput,
    const int interfaceIndex);


// NAME:        RoutingHsrpFinalize.
// PURPOSE:     Handling all finalization needed by static routing.
// Return:      None.

void
RoutingHsrpFinalize(
    Node *node,
    int interfaceIndex);


BOOL HsrpEnabledAndActiveRouter(Node *node,
                                int interfaceIndex);

void HsrpInterfaceStatusHandler(Node *node,
                                         int interfaceIndex,
                                         MacInterfaceState state);

#endif

