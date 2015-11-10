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

//--------------------------------------------------------------------------
//
// FILE       : ndp.h
//
// PURPOSE    : Implementing a neighbor discovery protocol for IARP.
//
//
// ASSUMPTION : This is a component protocol of IARP. See the file iarp.c.
//              This version of NDP transmits HELLO beacons at regular
//              intervals. Upon receiving a beacon, the neighbor table is
//              updated. Neighbors, for which no beacon has been received
//              within a specified time, are removed from the table.
//              IARP relies on this NDP to provide current information
//              about a node's neighbors. At the least, this information
//              must include the IP addresses of all the neighbors.

#ifndef NDP_H
#define NDP_H

#include "main.h"
#include "clock.h"
#include "list.h"

#define NDP_PERIODIC_HELLO_TIMER      (10 * SECOND)
#define NDP_NUM_HELLO_LOSS_ALLOWED    2
#define NDP_INITIAL_CACHE_ENTRY       8
#define NDP_HELLO_PACKET_SIZE         8
#define NDP_CONSTANT_TTL_VALUE        1
#define NDP_MAX_CALL_BACK_ENTRY       8

// Apply ramdom JITTER not greater than 1000 MS
#define NDP_MAX_JITTER   (1000 * MILLI_SECOND)


// Define call-back function pointer type. This call-back function pointer
// will be passed to ndp thru the function "NdpSetUpdateFunction()" by
// the protocols that want to use the service of ndp.
typedef void (*NdpUpdateFunctionType)(
    Node* node,
    NodeAddress neighborAddress,
    NodeAddress neighborSubnetMaskAddress,
    clocktype lastHardTime,
    int interfaceIndex,
    BOOL isNeighborReachable);


// Define NDP hello packet format.
//
// ASSUMPTION : This packet format is tolally assumed by the
//              developer (lack of availability) of documents.
//
typedef struct ndp_hello_packet_struct
{
    NodeAddress subnetMask;      // Subnet mask of the interface address.
    unsigned int helloInterval;  // The hello interval time.
} NdpHelloPacket;


// Define NDP neighbor structure
typedef struct ndp_neighbor_struct
{
    NodeAddress source;                  // Source Address of the neighbor
    NodeAddress subnetMask;              // Subnet mask advertised
    unsigned int neighborHelloInterval;  // Hello interval advertised
                                         // by neighbor.

    int interfaceIndex;                  // Interface index thru which the
                                         // packet is been received.

    clocktype lastHard;                  // Record when last hello received
} NdpNeighborStruct;


// Define NDP neighbor table cache
typedef struct ndp_neighbor_table_struct
{
    NdpNeighborStruct* neighborTable;  // The neighbor table array.
    unsigned int maxEntry;             // Maximun number of entries allowed
    unsigned int numEntry;             // Current number of entries.
} NdpNeighborTable;

// Define NDP statics.
typedef struct ndp_stat_type_struct
{
    unsigned int numHelloSend;        // Count number of hello packet send.
    unsigned int numHelloReceived;    // Count number of hello received.

    BOOL isStatPrinted;               // This variable is to prevent
                                      // printing of statistics twice.
} NdpStatType;


typedef struct ndp_update_func_handler_struct
{
    NdpUpdateFunctionType updateFunction;
    struct ndp_update_func_handler_struct* next;
} NdpUpdateFuncHandlar;

typedef struct ndp_update_func_list_struct
{
    NdpUpdateFuncHandlar* first;
    NdpUpdateFuncHandlar* last;
    unsigned int numEntry;
} NdpUpdateFuncHandlarList;


// Define NDP protocol structure.
typedef struct ndp_data_struct
{
    NdpNeighborTable ndpNeighborTable; // NDP neighbor table.
    clocktype ndpPeriodicHelloTimer;   // Periodic hello timer.
    unsigned numHelloLossAllowed;      // Number of hello loss allowed.
    BOOL collectStats;                 // Collect stats when TRUE
    NdpStatType stats;                 // Structure for NDP statistics
    NdpUpdateFuncHandlarList updateFuncHandlerList;
    RandomSeed seed;                   // Used for jitter.
} NdpData;

//
// FOLLOWING IS THE INTERFACE FUNCTION TO NETWORK LAYER.
// -----------------------------------------------------
//

//-------------------------------------------------------------------------
// FUNCTION     : NdpInit()
//
// PURPOSE      : Initializing NDP protocol.
//
// PARAMETERS   : node - Node which is initializing NDP
//                ndpData - Pointer to NdpData pointer
//                nodeInput - Node input
//                interfaceIndex - Interface index
//
// ASSUMPTION   : This function will be called from iarp.c.
//-------------------------------------------------------------------------
void NdpInit(

    Node* node,
    void** ndpData,
    const NodeInput* nodeInput,
    int interfaceIndex);


//-------------------------------------------------------------------------
// FUNCTION     : NdpHandleProtocolEvent()
//
// PURPOSE      : Handling NDP timer related events.
//
// PARAMETER    : node - The node that is handling the event.
//                msg - the event that is being handled
//
// ASSUMPTION   : his function will be called from NetworkIpLayer()
//-------------------------------------------------------------------------
void NdpHandleProtocolEvent(Node* node, Message* msg);


//-------------------------------------------------------------------------
// FUNCTION     : NdpHandleProtocolPacket()
//
// PURPOSE      : Processing receipt of an NDP hello.
//
// PARAMETERS   : node - Node receiving the packet
//                msg - The packet that has been received.
//                sourceAddress - Source address of the packet
//                interfaceIndex - interface index of the packet.
//
// ASSUMPTION   : This function will be called from "DeliverPacket()"
//                in ip.c
//-------------------------------------------------------------------------
void NdpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    int interfaceIndex);

//
// FOLLOWING IS THE INTERFACE FUNCTION TO ANY OTHER PROTOCOL.
// ----------------------------------------------------------
//

//-------------------------------------------------------------------------
// FUNCTION     : NdpSetUpdateFunction()
//
// PURPOSE      : Setting protocol defined update function handler that
//                will be called from NDP.
//
// PARAMETERS   : node - Node which is setting update function
//                nodeInput - node input.
//                updateFunction - update function pointer
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
void NdpSetUpdateFunction(
    Node* node,
    const NodeInput* nodeInput,
    NdpUpdateFunctionType updateFunction);

//-------------------------------------------------------------------------
// FUNCTION     : NdpFinalize()
//
// PURPOSE      : Printing the statisdics collected during simulation.
//
// PARAMETERS   : node - Node which is printing the statistics.
//
// RETURN VALUE : None
//-------------------------------------------------------------------------
void NdpFinalize(Node* node);

#endif
