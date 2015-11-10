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
// File     : policy_routing.cpp
// Objective: Source file for policy_routing (CISCO implementation
//              of policy based routing)
// Reference: CISCO document Set
//--------------------------------------------------------------------------

#ifndef POLICY_ROUTING_H
#define POLICY_ROUTING_H

#include "network_ip.h"


#define PBR_ADDRESS_STRING 20

// used for trace
#define PBR_NO_INTERFACE -1


typedef struct
struct_policy_route_set_values
{
    int precedence;

    // next hop address as set by PBR
    NodeAddress nextHop;

    // outgoing interface index
    int intfIndex;

    // next hop address as set by PBR, default value
    NodeAddress defNextHop;

    // outgoing interface index
    int defIntfIndex;

} PbrSetValues;



typedef struct
struct_policy_route_stat
{
    // number of packets policy routed
    UInt32 packetsPolicyRouted;

    // number of packets policy routed by local PBR
    UInt32 packetsPolicyRoutedLocal;

    // number of packets not policy routed
    UInt32 packetsNotPolicyRouted;

    // number of packets set by precedence
    UInt32 packetPrecSet;

} PbrStat;


//--------------------------------------------------------------------------
// FUNCTION  PbrInit
// PURPOSE   Plug the route maps that are included in the PBR and do the
//  required checks.
// ARGUMENTS node, node concerned
//           nodeInput, Main configuration file
// RETURN    None
// COMMENTS  Statements which are not used will be not parsed. No feedback
//              is given to the user.
//--------------------------------------------------------------------------
void PbrInit(Node* node, const NodeInput* nodeInput);



//--------------------------------------------------------------------------
// FUNCTION  PbrFillValues
// PURPOSE   Squeeze the values from the packet and fill up the value list
//              for route map.
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           msg, the incoming message
// RETURN    None
//--------------------------------------------------------------------------
void PbrFillValues(Node* node,
                   RouteMapValueSet* valueSet,
                   const Message* msg);



//--------------------------------------------------------------------------
// FUNCTION  PbrRouteMapTest
// PURPOSE   Match with the route map
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           entry, the route map entry
// RETURN    BOOL, returns the match and route map type result
//--------------------------------------------------------------------------
BOOL PbrRouteMapTest(Node* node,
                     RouteMapValueSet* valueSet,
                     RouteMapEntry* entry);


//--------------------------------------------------------------------------
// FUNCTION  PbrSetEntry
// PURPOSE   Use the values of route map for PBR
// ARGUMENTS node, node concerned
//           valueSet, Structure for passing values to the route map
//           msg, The message that has reached
//           incomingInterface, the interface number where PBR is implemented
// RETURN    None
//--------------------------------------------------------------------------
void PbrSetEntry(Node* node,
                 RouteMapValueSet* valueSet,
                 Message* msg,
                 int incomingInterface);

//--------------------------------------------------------------------------
// FUNCTION  PbrInvoke
// PURPOSE   Call PBR
// ARGUMENTS node, node concerned
//           msg, The message that has reached
//           interfaceIndex, the incoming interface
//           rMap, the route map associated
//           isDefault, is it a default call
//           isLocal, is it a local pbr
// RETURN    BOOL, true if action taken
//--------------------------------------------------------------------------
BOOL PbrInvoke(Node* node,
               Message* msg,
               NodeAddress prevHop,
               int interfaceIndex,
               RouteMap* rMap,
               BOOL isDefault,
               BOOL isLocal);


//--------------------------------------------------------------------------
// FUNCTION  PbrUpdateStat
// PURPOSE   Update the policy routing stats
// ARGUMENTS node, current node
//           flag, the result of PBR action
//           incomingInterface, the interface index
// RETURN    None
//--------------------------------------------------------------------------
void PbrUpdateStat(Node* node,
                   BOOL flag,
                   int incomingInterface,
                   BOOL isLocal);


//--------------------------------------------------------------------------
// FUNCTION  PbrFinalize
// PURPOSE   Policy routing functionalities handled during termination.
//              Here we print the various PBR statistics.
// ARGUMENTS node, current node
// RETURN    None
//--------------------------------------------------------------------------
void PbrFinalize(Node* node);



//--------------------------------------------------------------------------
// FUNCTION  PbrDebug
// PURPOSE   A trace file generator
// ARGUMENTS
// RETURN    None
//--------------------------------------------------------------------------
void PbrDebug(Node* node,
              IpHeaderType* ipHeader,
              int incomingInterface,
              NodeAddress nextHop,
              int outgoingInterface);


#endif // POLICY_ROUTING_H
