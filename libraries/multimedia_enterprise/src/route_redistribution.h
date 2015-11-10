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

#ifndef ROUTE_REDISTRIBUTE_H
#define ROUTE_REDISTRIBUTE_H


// Cost of external routes
#define OSPFV2_DEFAULT_EXTERNAL_COST 20

#define ROUTE_COST -1

// Route Redistribute Command Type
typedef enum
{
    ROUTE_REDISTRIBUTE,
    ROUTE_NO_REDISTRIBUTE
} RedistributeCommandType;


// Structure for Route Redistribution statistics.
// Stats are collected interface wise

typedef
struct redistribute_stat{

    int     numNewRoutesDistributed;
    int     numInvalidRoutesInformed;
    // No. of routes discarded due to RouteMap
    int     numRoutesDiscarded;
    int     totalRoutesFound;
    // No. of routes blocked due to NO REDISTRIBUTE command
    int     numRoutesBlocked;
    // No. of routes discarded due to Simtime of REDISTRIBUTE command
    int     numRoutesDiscardedForSimTime;
    // For printing the interface index in the stat file
    int     interfaceIndex;
    } RedistributeStats;


// Function pointer to update routing table for a routing protocol.
typedef
void (*RoutingTableUpdateFunction)(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    void* metricPtr);

// Structure for Route Redistribution input.
typedef
struct redistribute_input
{
    NetworkRoutingProtocolType distributorProtocolType;

    NetworkRoutingProtocolType receiverProtocolType;

    clocktype   startTime; // When to start 'REDISTRIBUTION'
    clocktype   endTime;   // When to stop 'REDISTRIBUTION'

    clocktype   removalStartTime; // When to start 'NO REDISTRIBUTION'
    clocktype   removalEndTime;   // When to stop 'NO REDISTRIBUTION'
                                  // In between this time interval no
                                  // routes would be redistributed.

    BOOL removalIsEnabled;  // 'NO REDISTRIBUTION' is enabled or disabled
    RouteMap* rMap;     // The route map associated if any
    int metric;
    void* genericMetricPtr; // A void pointer for metric. Metric type can
                            // differ for different protocols. Appropriate
                            // typecasting is required.

    int versionReceiver;   //Used to check the version of RIP
    int versionDistributor;//in route redistribution command

    // Statistics collection for redistribution
    RedistributeStats* rtRedistributeStats;
    struct redistribute_input* next;

} RouteRedistribute;

// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeInit
// PURPOSE: Initialize the Route redistribution
// ARGUMENTS:
//          node:: The current node
//          nodeInput:: Main configuration file
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeInit(
    Node* node,
    const NodeInput* nodeInput);


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeAddHook
// PURPOSE: Plugging to the route redistribution
// ARGUMENTS:
//          node:: The current node
//          destAddress:: IP address of destination network or host
//          destAddressMask:: Netmask
//          nextHopAddress:: Next hop IP address
//          interfaceIndex:: Interface to get hooked from
//          cost:: Cost metric associated with the route
//          routingProtocolType:: Type value of routing protocol
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeAddHook(
    Node* node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    int cost,
    NetworkRoutingProtocolType routingProtocolType);


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeSetRoutingTableUpdateFunction
// PURPOSE: Sets the routing table update function to be used by route
//          redistribution
// ARGUMENTS:
//          node:: The current node
//          routingTableUpdateFunction:: Function pointer to update the
//          routing table by a routing protocol
//          interfaceIndex:: Interface Index
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeSetRoutingTableUpdateFunction(
    Node* node,
    RoutingTableUpdateFunction routingTableUpdateFunction,
    int interfaceIndex);


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeGetRoutingTableUpdateFunction
// PURPOSE: Gets the routing table update function to be used by route
//          redistribution
// ARGUMENTS:
//          node:: The current node
//          routingTableUpdateFunction:: Function pointer to update the
//          routing table by a routing protocol
//          interfaceIndex:: Interface index
// RETURN: routingTableUpdateFunction:: A function pointer of the routing
//         table update function for redistribution
// ------------------------------------------------------------------------

RoutingTableUpdateFunction RouteRedistributeGetRoutingTableUpdateFunction(
    Node* node,
    NetworkRoutingProtocolType routingProtocol);


// ------------------------------------------------------------------------
// FUNCTION: RouteRedistributeFinalize
// PURPOSE: Prints out the statistics at the end of the silumation
// ARGUMENTS:
//          node:: The current node
// RETURN: None
// ------------------------------------------------------------------------

void RouteRedistributeFinalize(Node* node);

//--------------------------------------------------------------------------
//  NAME:        RouteRedistributionLayer.
//  PURPOSE:     Handle timers and layer messages.
//  PARAMETERS: 
//               Node* node:     Node handling the incoming messages
//               Message* msg: Message for node to interpret.
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void RouteRedistributionLayer(Node* node, Message* msg);

#endif
