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
// PROTOCOL :: OSPFV3.
// SUMMARY :: The principal characteristic of OSPF is that it is based on SPF
// algorithm. The algorithm sometimes referred to as Dijkstra Algorithm.
// Based on topology information this algorithm calculates a shortest path
// tree with calculating node as root. This shortest path tree gives route
// to each destination in the Autonomous System (AS). The topology
// information is termed as Link State Database (LSDB), which consists of
// Link State Advertisement (LSA) that every router in the domain originates
// in a periodical and on-demand basis.
// What we mean by Link State is a description of a router's usable interface
// and its relation to its attached neighbors and networks. Each router in
// the domain advertises its local state (e.g. router's usable interfaces and
// attached neighbors) throughout the AS by flooding.
// The flooding procedure needs to be adequately efficient so that each
// router in the domain have the same LSDB.
// One of the major advantages of OSPF over RIP is that it can operate within
// a hierarchy. The largest entity within the hierarchy is
// Autonomous System (AS), which is a collection of networks under a single
// administrative control. Though OSPF is an Interior Gateway Protocol it can
// receive and update routing information from other AS. An AS can further be
// divided into a number of areas, which are groups of contiguous networks
// and attached hosts. The topology of an area is hidden from the rest of the
// AS.
// Doing so it is no longer required that all router in the AS have exactly
// same set of LSAs, thus one of the significant effect of this feature is
// reduction in routing traffic. Routing within an area is solely depends on
// areas own topology. For inter area routing some mechanism is required for
// finding out routes to networks belong to other area. This is acheive with
// help Area Border Router who inject inter area routes into the area.
// An OSPF router built its routing table from the Shortest Path Tree it
// constructs using its attached areas database. All router runs the exactly
// same algorithm, in parallel. Whenever the topology changes occur, the
// routing table is built again from the scratch. IP packets are routed
// "as is" & they are not encapsulated in any further protocol headers as
// they transit the Autonomous System. As a dynamic routing protocol OSPF
// quickly detects topology changes in the AS and calculates new loop free
// routes after a period of convergence.

// LAYER :: Network Layer Protocol runs directly over IPV6.
// STATISTICS :: Ospfv3Stats : numHelloSent;
//                             numHelloRecv;
//                             numLSUpdateSent;
//                             numLSUpdateRecv;
//                             numAckSent;
//                             numAckRecv;
//                             numLSUpdateRxmt;
//                             numExpiredLSAge;
//                             numDDPktSent;
//                             numDDPktRecv;
//                             numDDPktRxmt;
//                             numLSReqSent;
//                             numLSReqRecv;
//                             numLSReqRxmt;
//                             numRtrLSAOriginate;
//                             numNetLSAOriginate;
//                             numInterAreaPrefixLSAOriginate;
//                             numInterAreaRtrLSAOriginate;
//                             numLinkLSAOriginate;
//                             numIntraAreaPrefixLSAOriginate;
//                             numASExternalLSAOriginate
//                             numLSARefreshed;
//
// CONFIG_PARAM ::Parameter Name format
//
//           ROUTING-PROTOCOL               OSPFv3
//           OSPFv3-STAGGER-START           YES | NO
//
//           OSPFv3-INJECT-EXTERNAL-ROUTE   YES
//           OSPFv3-INJECT-ROUTE-FILE       <filename>
//
//           <node_id>  AS-NUMBER           <as_id>
//           <node_id>  AS-BOUNDARY-ROUTER  YES
//
//           OSPFv3-DEFINE-AREA             YES | NO
//           OSPFv3-CONFIG-FILE             <filename>

// OSPFv3 Configuration File Parameters
//           [<Network number>]  AREA-Id  <Area Id in IP address format>
//           AREA <Area Id> RANGE { <Network1>,  ... , <NetworkN> } [<as_id>]
//           AREA <Area Id> STUB <Stub Default Cost>    [<as_id>]
//
//           [Interface Qualifier] INTERFACE-COST       <cost>
//           [Interface Qualifier] RXMT-INTERVAL        <interval>
//           [Interface Qualifier] INF-TRANS-DELAY      <delay>
//           [Interface Qualifier] ROUTER-PRIORITY      <priority>
//           [Interface Qualifier] HELLO-INTERVAL       <interval>
//           [Interface Qualifier] ROUTER-DEAD-INTERVAL <interval>
//           [Interface Qualifier] INTERFACE-TYPE       <type>
//
//
// IMPLEMENTED_FEATURES :: Flooding scope
//                         Intra-area-prefix-LSA
//                         IPV6 prefix represention of group of addresses.
//                         Single Area and Splitting AS into areas
//                         Physically contiguous Backbone Area
//                         Inter area routing
//                         Hello protocol
//                         Broadcast Network
//                         Point-to-point networks
//                         Point-to-Multipoint networks
//                         Aging of LSA
//                         AS-External LSA

//
// Features to be considered for Future Work
// Phase-2 implementations:
//    1. Link local addresses
//    2. Link-LSA
//

// OMITTED_FEATURES :: (Title) : (Description) (list)
//                         1. Virtual link.
//                         2. Multiple OSPF instance on a single link.
//                         3. NBMA (Non broadcast Multiple Access).
//                         4. Equal cost Multi-path.
//                         5. Incremental LS update.
//                         6. Type-7 LSA.
//                         7. Unknown LSA types handling.
//                         8. Backbone that are physically discontinuous.

// ASSUMPTIONS :: (Description of Assumption) (list)
// 1.   While running over non broadcast network OSPFv3 will consider
//      underlying network as Point-to-Point unless user specifies/configures
//      the same.
// 2.   ASBR will not calculate AS-External routes. BGP will be responsible
//      to inject AS-External routes in IP Forwarding table.
// 3.   Assumptions regarding virtual links are same as for OSPFV2.
// 4.   Link local addressing is not used because of IPv6 Addressing
//      restriction. Although for future use, Link local LSA will be
//      generated and flooded across the defined scopes.
// 5.   Each router configured with multiple interfaces will originate single
//      aggregated router lsa and the intra
//      area prefix lsa for the OSPF node.
// 6.   Permissible Area-ID configuration is within the range of 0.0.0.0 to
//      255.255.255.253.
// 7.   Users need to insert appropriate Outgoing Interface Cost inside
//      the .ospf file in order to choose a High Bandwidth Route path.
// 8.   In a switch scenario, dr-bdr nodes are elected correctly around 60s.
// 9.  As Link-Local Addressing is not used, the Intra-Area-Prefix LSA with
//      reference to Network LSA is not generated.
// 10.  All Area Border Routers must have an interface connected to the
//      Backbone area.
// 11.  Multiple common interfaces between nodes are not supported.
// 12.  Interface type broadcast needs to be specified for Wireless Subnet.


// STANDARD :: (Standard used for implementation)
// Reference:
// 1. RFC 2740, "OSPF for IPv6" R. Coltun, D. Ferguson and J. Moy Dec 1999.
// 2. RFC 2328, "OSPF version 2" J. Moy April 1998.

// Other references:
// 3. OSPF: Frequently Asked Questions, Document ID: 9237,Author:
//    Syed Faraz Shamim, available at
//    http://www.cisco.com/warp/public/104/9.html
// 4. Configuring OSPFv3, available at
// http://www.foundrynet.com/services/documentation/srIPv6/OSPF_Version_3.html

// RELATED :: (Name of related protocol) (list) {optional}
// **/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "api.h"
#include "network_ip.h"
#include "ipv6.h"
#include "routing_ospfv3.h"
#include "mac_link.h"
#include "mac_802_3.h"
#include "partition.h"
#include "network_dualip.h"

#include "ipv6_auto_config.h"

// /**
// ------------- Please uncomment the Flags to print DEBUG outputs ----------
// **/

//#define OSPFv3_DEBUG_TABLE
//#define OSPFv3_DEBUG_TABLEErr
//
//#define OSPFv3_DEBUG
//#define OSPFv3_DEBUG_LSDB
//#define OSPFv3_DEBUG_ISM
//#define OSPFv3_DEBUG_SYNC
//#define OSPFv3_DEBUG_FLOOD
//#define OSPFv3_DEBUG_HELLO
//
//#define OSPFv3_DEBUG_ERRORS
//#define OSPFv3_DEBUG_SPTErr
//#define OSPFv3_DEBUG_LSDBErr
//#define OSPFv3_DEBUG_SYNCErr
//#define OSPFv3_DEBUG_FLOODErr
//#define OSPFv3_DEBUG_HELLOErr

// /**
// FUNCTION   :: Ospfv3PrintLSHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print Link State Header.
// PARAMETERS ::
// + LSHeader   :  Ospfv3LinkStateHeader*  : Pointer to node.
// RETURN     :: void : NULL
// **/
void Ospfv3PrintLSHeader(Ospfv3LinkStateHeader* LSHeader)
{
    printf("\n\n");

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("    |        Age = %-18d|    Type = %d    |\n",
        LSHeader->linkStateAge,
        LSHeader->linkStateType);

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("    |                    Link State ID = %-30d|\n",
        LSHeader->linkStateId);

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("    |                       AdvrRtr = %-33d|\n",
        LSHeader->advertisingRouter);

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("    |                     Seq Number = %-32x|\n",
        LSHeader->linkStateSequenceNumber);

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("    |        Checksum = %-13d|          length = %-14d|\n",
        LSHeader->linkStateCheckSum,
        LSHeader->length);

    printf("    -----------------------------------------------"
            "---------------------\n");

    printf("\n\n");
}

// /**
// FUNCTION   :: Ospfv3PrintDBSummaryList
// LAYER      :: NETWORK
// PURPOSE    :: Print Database summary list of the Router.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + DBSummaryList : LinkedList* : Pointer to List.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintDBSummaryList(Node* node, LinkedList* DBSummaryList)
{
    ListItem* item = DBSummaryList->first;
    Ospfv3LinkStateHeader* LSHeader = NULL;

    printf("    Database Summary list of node %u:\n", node->nodeId);

    while (item)
    {
        LSHeader = (Ospfv3LinkStateHeader* ) item->data;

        printf("        advertisingRouter               = %d\n",
            LSHeader->advertisingRouter);

        printf("        linkStateID                 = 0x%x\n",
            LSHeader->linkStateType);

        printf("        linkStateID                 = %d\n",
            LSHeader->linkStateId);

        printf("        linkStateSequenceNumber     = 0x%x\n\n",
            LSHeader->linkStateSequenceNumber);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintDatabaseDescriptionPacket
// LAYER      :: NETWORK
// PURPOSE    :: Print Database Description Packet.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to Message.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintDatabaseDescriptionPacket(Node* node, Message* msg)
{
    Ospfv3DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    int size = 0;

    dbDscrpPkt = (Ospfv3DatabaseDescriptionPacket* )
                    MESSAGE_ReturnPacket(msg);

    LSHeader = (Ospfv3LinkStateHeader* ) (dbDscrpPkt + 1);
    size = dbDscrpPkt->commonHeader.packetLength;

    printf("    Content of DD packet of Node %d:\n", node->nodeId);

    // For each LSA in the packet
    for (size -= sizeof(Ospfv3DatabaseDescriptionPacket); size > 0;
        size -= sizeof(Ospfv3LinkStateHeader))
    {
        printf("   \tlinkStateType              = 0x%x\n",
            LSHeader->linkStateType);

        printf("   \tlinkStateID                = 0x%x\n",
            LSHeader->linkStateId);

        printf("   \tadvertisingRouter          = %d\n",
            LSHeader->advertisingRouter);

        printf("   \tlinkStateSequenceNumber    = 0x%x\n\n",
            LSHeader->linkStateSequenceNumber);

        LSHeader += 1;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintRouterLSAListForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Print Router LSA List of an Area.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + thisArea : Ospfv3Area* : Pointer to Area Structure.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintRouterLSAListForThisArea(Node* node, Ospfv3Area* thisArea)
{
    ListItem* item = NULL;
    char areaStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaId, areaStr);

    printf("   Router LSA for node %u, area %s\n",
        node->nodeId,
        areaStr);

    printf("   Size of Router LSA list of Node %d is = %d\n",
        node->nodeId,
        thisArea->routerLSAList->size);

    item = thisArea->routerLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintNetworkLSAListForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Print Network LSA List of an Area.
// PARAMETERS ::
// + node     :  Node*                      : Pointer to node.
// + thisArea :  Ospfv3Area*                : Pointer to Area Structure.
// RETURN     :: void : NULL
// **/
static
void Ospfv3PrintNetworkLSAListForThisArea(Node* node, Ospfv3Area* thisArea)
{
    ListItem* item = NULL;
    char areaStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaId, areaStr);

    printf("   Network LSAs for node %u, area %s\n",
        node->nodeId,
        areaStr);

    printf("   Size of Network LSA list of Node %d is = %d\n",
        node->nodeId,
        thisArea->networkLSAList->size);

    item = thisArea->networkLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintInterAreaPrefixLSAListForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Print Inter Area Prefix LSA List of an Area.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + thisArea : Ospfv3Area* : Pointer to Area Structure.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintInterAreaPrefixLSAListForThisArea(
    Node* node,
    Ospfv3Area* thisArea)
{
    ListItem* item = NULL;
    char areaStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaId, areaStr);

    printf("   InterAreaPrefix LSAs for node %u, area %s\n",
        node->nodeId,
        areaStr);

    printf("   Size of Inter-Area-Prefix LSA list of Node %d is %d\n",
        node->nodeId,
        thisArea->interAreaPrefixLSAList->size);

    item = thisArea->interAreaPrefixLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintInterAreaRouterLSAListForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Print Inter Area Router LSA List of an Area.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + thisArea : Ospfv3Area* : Pointer to Area Structure.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintInterAreaRouterLSAListForThisArea(Node* node,
                                                  Ospfv3Area* thisArea)
{
    ListItem* item = NULL;
    char areaStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaId, areaStr);

    printf("   InterAreaPrefix LSAs for node %u, area %s\n",
        node->nodeId,
        areaStr);

    printf("   Size of Inter-Area-Prefix LSA list of Node %d is = %d\n",
        node->nodeId,
        thisArea->interAreaRouterLSAList->size);

    item = thisArea->interAreaRouterLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintIntraAreaPrefixLSAListForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Print Intra Area Prefix LSA List of an Area.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + thisArea : Ospfv3Area* : Pointer to Area Structure.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintIntraAreaPrefixLSAListForThisArea(Node* node,
                                                  Ospfv3Area* thisArea)
{
    ListItem* item = NULL;
    char areaStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(thisArea->areaId, areaStr);

    printf("   IntraAreaRouter LSAs for node %u, area %s\n",
        node->nodeId,
        areaStr);

    printf("   Size of Intra-Area-Prefix LSA list of Node %d is = %d\n",
        node->nodeId,
        thisArea->intraAreaPrefixLSAList->size);

    item = thisArea->intraAreaPrefixLSAList->first;

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintRouterLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Print Router LSA List of all the Area to which node belongs.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintRouterLSAList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item = NULL;

    printf("Router LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) item->data;

        Ospfv3PrintRouterLSAListForThisArea(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintNetworkLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Print Network LSA List of all the Area to which node belongs.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
void Ospfv3PrintNetworkLSAList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item = NULL;

    printf("\nNetwork LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) item->data;

        Ospfv3PrintNetworkLSAListForThisArea(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintInterAreaPrefixLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Print Inter Area Prefix LSA List of all the Area to which
//               node belongs.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintInterAreaPrefixLSAList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item = NULL;

    printf("InterAreaPrefix LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) item->data;

        Ospfv3PrintInterAreaPrefixLSAListForThisArea(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintInterAreaRouterLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Print Inter Area Router LSA List of all the Area to which
//               node belongs.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintInterAreaRouterLSAList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item = NULL;

    printf("InterAreaRouter LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) item->data;

        Ospfv3PrintInterAreaRouterLSAListForThisArea(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintASExternalLSA
// LAYER      :: NETWORK
// PURPOSE    :: Print AS-External LSA List of the Router.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintASExternalLSA(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item;

    if (!ospf->asExternalLSAList)
    {
        return;
    }

    item = ospf->asExternalLSAList->first;

    printf("AS External LSA list for node %u\n", node->nodeId);

    printf("   Size of AS-External LSA list of Node %d is = %d\n",
        node->nodeId,
        ospf->asExternalLSAList->size);

    // Print each link state information that we have in our
    // link state database.
    while (item)
    {
        Ospfv3PrintLSA((char* ) item->data);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintLinkLSA
// LAYER      :: NETWORK
// PURPOSE    :: Print Link LSA List of the Router.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintLinkLSA(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item;
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        item = ospf->pInterface[i].linkLSAList->first;

        printf("Link Local LSA list for node %u\n", node->nodeId);

        printf("Size is = %d\n", ospf->pInterface[i].linkLSAList->size);

        // Print each link state information that we have in our
        // link state database.
        while (item)
        {
            Ospfv3PrintLSA((char* ) item->data);

            item = item->next;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3PrintIntraAreaPrefixLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Print Intra Area Prefix LSA List of the Router.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintIntraAreaPrefixLSAList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* item = NULL;

    printf("IntraAreaRouter LSAs for node %u\n", node->nodeId);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) item->data;

        Ospfv3PrintIntraAreaPrefixLSAListForThisArea(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintLSDB
// LAYER      :: NETWORK
// PURPOSE    :: Print Link State Database of the Router.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintLSDB(Node* node)
{
    char timeStringInSecond[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), timeStringInSecond);

    printf("---------------------------------------------------\n");

    printf("LSDB OF NODE %u at time %15s\n",
        node->nodeId,
        timeStringInSecond);

    Ospfv3PrintRouterLSAList(node);

    Ospfv3PrintNetworkLSAList(node);

    Ospfv3PrintInterAreaPrefixLSAList(node);

    Ospfv3PrintInterAreaRouterLSAList(node);

    Ospfv3PrintASExternalLSA(node);

    // Link-Local LSA is not enabled as Link-Local address is not used.
    //Ospfv3PrintLinkLSA(node);

    Ospfv3PrintIntraAreaPrefixLSAList(node);

    printf("---------------------------------------------------\n");
}

// /**
// FUNCTION   :: Ospfv3GetNeighborStateString
// LAYER      :: NETWORK
// PURPOSE    :: Change Neighbor State into String Printable format.
// PARAMETERS ::
// + state : Ospfv2NeighborState : Neighbor State
// +str   : char* : Pointer to string.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3GetNeighborStateString(Ospfv2NeighborState state, char* str)
{
    if (state == S_NeighborDown)
    {
        strcpy(str, "S_NeighborDown");
    }
    else if (state == S_Attempt)
    {
        strcpy(str, "S_Attempt");
    }
    else if (state == S_Init)
    {
        strcpy(str, "S_Init");
    }
    else if (state == S_TwoWay)
    {
        strcpy(str, "S_TwoWay");
    }
    else if (state == S_ExStart)
    {
        strcpy(str, "S_ExStart");
    }
    else if (state == S_Exchange)
    {
        strcpy(str, "S_Exchange");
    }
    else if (state == S_Loading)
    {
        strcpy(str, "S_Loading");
    }
    else if (state == S_Full)
    {
        strcpy(str, "S_Full");
    }
    else
    {
        strcpy(str, "Unknown");
    }
}

// /**
// FUNCTION   :: Ospfv3PrintNeighborState
// LAYER      :: NETWORK
// PURPOSE    :: Print Neighbor State.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintNeighborState(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i = 0;

    printf("\n\nNeighbor state for Node %u:\n", node->nodeId);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        ListItem* listItem = NULL;
        Ospfv3Neighbor* nbrInfo = NULL;

        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        listItem = ospf->pInterface[i].neighborList->first;

        while (listItem)
        {
            char stateStr[MAX_STRING_LENGTH];

            nbrInfo = (Ospfv3Neighbor* ) listItem->data;

            Ospfv3GetNeighborStateString(nbrInfo->state, stateStr);

            printf("State of neighbor %d : %15s\n",
                nbrInfo->neighborId,
                stateStr);

            listItem = listItem->next;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3PrintVertexList
// LAYER      :: NETWORK
// PURPOSE    :: Print Vertex List formed during SPF calculation of the node.
// PARAMETERS ::
// + list : LinkedList* : Pointer to list.
// RETURN     ::  void : NULL
// **/
void Ospfv3PrintVertexList(LinkedList* list)
{
    ListItem* listItem = list->first;

    while (listItem)
    {
        int i = 1;
        Ospfv3Vertex* entry;
        ListItem* nextHopItem;
        char vertexTypeStr[MAX_STRING_LENGTH];

        entry = (Ospfv3Vertex* ) listItem->data;

        if (entry->vertexType == OSPFv3_VERTEX_ROUTER)
        {
            printf("\n    Vertex ID                 = %d\n",
                entry->vertexId.routerVertexId);

            strcpy (vertexTypeStr, "OSPFv3_VERTEX_ROUTER");
        }
        else if (entry->vertexType == OSPFv3_VERTEX_NETWORK)
        {
            printf("\n    Vertex ID (DR, Interface) = (%d, %d)\n",
                entry->vertexId.networkVertexId.routerIdOfDR,
                entry->vertexId.networkVertexId.interfaceIdOfDR);

            strcpy(vertexTypeStr, "OSPFv3_VERTEX_NETWORK");
        }
        else
        {
            strcpy(vertexTypeStr, "Unknown Vertex Type");

            ERROR_Assert(FALSE, "Unknown Vertex Type\n");
        }


        printf("    vertex Type               = %s\n", vertexTypeStr);

        printf("    metric                    = %d\n", entry->distance);

        nextHopItem = entry->nextHopList->first;

        while (nextHopItem)
        {
            Ospfv3NextHopListItem* nextHopInfo = NULL;
            char nextHopStr[MAX_ADDRESS_STRING_LENGTH];

            nextHopInfo = (Ospfv3NextHopListItem* ) nextHopItem->data;

            IO_ConvertIpAddressToString(&nextHopInfo->nextHop, nextHopStr);

            printf("    NextHop[%d]                = %s\n", i, nextHopStr);

            nextHopItem = nextHopItem->next;
            i += 1;
        }

        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintShortestPathList
// LAYER      :: NETWORK
// PURPOSE    :: Print shortest path list of the node.
// PARAMETERS ::
// +Node  : Node*   : Pointer to node.
// + shortestPathList : LinkedList* : Pointer to list.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintShortestPathList(Node* node, LinkedList* shortestPathList)
{
    printf("\n----------------------------------------------\n");

    printf("Shortest path list for node %u\n", node->nodeId);

    printf("Size of the shortest path list is   = %d\n",
        shortestPathList->size);

    Ospfv3PrintVertexList(shortestPathList);

    printf("\n----------------------------------------------\n");
}

// /**
// FUNCTION   :: Ospfv3PrintCandidateList
// LAYER      :: NETWORK
// PURPOSE    :: Print candidate list of the node.
// PARAMETERS ::
// +Node  : Node*   : Pointer to node.
// + candidateList : LinkedList* : Pointer to list.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintCandidateList(Node* node, LinkedList* candidateList)
{
    printf("Node %d's candidate list size = %d.\n",
        node->nodeId,
        candidateList->size);

    Ospfv3PrintVertexList(candidateList);
}

// /**
// FUNCTION   :: Ospfv3PrintLSA
// LAYER      :: NETWORK
// PURPOSE    :: Print the LSA.
// PARAMETERS ::
// + LSA      :  char* : Pointer to bytes of LSA.
// RETURN     :: void : NULL
// **/
void Ospfv3PrintLSA(char* LSA)
{
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    int i;

    // LSA is a router LSA.
    if (LSHeader->linkStateType == OSPFv3_ROUTER)
    {
        Ospfv3RouterLSA* routerLSA = (Ospfv3RouterLSA* ) LSA;
        Ospfv3LinkInfo* linkList = (Ospfv3LinkInfo* ) (routerLSA + 1);

        int numLinks = (LSHeader->length - sizeof(Ospfv3RouterLSA) )
                        / sizeof(Ospfv3LinkInfo);

        printf("       LSA is a router LSA\n");

        printf("       LSA from node %d contains following link's Info:\n",
            LSHeader->advertisingRouter);

        for (i = 0; i < numLinks; i++)
        {
            printf("       Link %d:\n"
                    "       \t interfaceId          = %d\n"
                    "       \t neighbor             = %d\n"
                    "       \t metric               = %d\n"
                    "       \t type                 = %d\n",
                i + 1,
                linkList->interfaceId,
                linkList->neighborRouterId,
                linkList->metric,
                linkList->type);

           linkList = linkList + 1;
        }
    }
    else if (LSHeader->linkStateType == OSPFv3_NETWORK)
    {
        Ospfv3NetworkLSA* pNetworkLSA = (Ospfv3NetworkLSA* ) LSA;
        unsigned int* pRouterId = (unsigned int* ) (pNetworkLSA + 1);
        int iNumRouterId =
            (pNetworkLSA->LSHeader.length - sizeof(Ospfv3NetworkLSA))
            / (sizeof(unsigned int));

        printf("   LSA is a Network LSA\n");

        printf("   LSA from node %d contains:\n",
            LSHeader->advertisingRouter);

        printf("   \tOptions             = 0x%x\n"
                "   \tNumber Of Neighbors = %d\n",
            pNetworkLSA->bitsAndOptions,
            iNumRouterId);

        for (i = 0; i < iNumRouterId; i++)
        {
            printf("   \t\tRouterId        = %d\n",
                pRouterId[i]);
        }
    }
    else if (LSHeader->linkStateType == OSPFv3_INTRA_AREA_PREFIX)
    {
        Ospfv3IntraAreaPrefixLSA* intraAreaPrefixLSA =
                                        (Ospfv3IntraAreaPrefixLSA* ) LSA;
        Ospfv3PrefixInfo* prefixInfo =
                            (Ospfv3PrefixInfo* ) (intraAreaPrefixLSA + 1);

        printf("   LSA is a Intra Area Prefix LSA\n");

        printf("   LSA from node %d contains"
                " following Network Prefix Info:\n",
            LSHeader->advertisingRouter);

        for (i = 0; i < intraAreaPrefixLSA->numPrefixes; i++)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&prefixInfo->prefixAddr, addrString);

            printf("   Prefix %d:\n   \tprefix length = %d\n"
                    "   \tmetric = %d\n   \tdestination prefix = %s\n",
                i + 1,
                prefixInfo->prefixLength,
                prefixInfo->reservedOrMetric,
                addrString);

            prefixInfo = prefixInfo + 1;
        }
    }
    else if (LSHeader->linkStateType == OSPFv3_INTER_AREA_PREFIX)
    {
        Ospfv3InterAreaPrefixLSA* interAreaPrefixLSA =
                                        (Ospfv3InterAreaPrefixLSA* ) LSA;
        char addrString[MAX_STRING_LENGTH];

        printf("   LSA is a Inter Area Prefix LSA\n");

        printf("   LSA from node %d contains the following prefix info :\n",
            LSHeader->advertisingRouter);

        IO_ConvertIpAddressToString(
            &interAreaPrefixLSA->addrPrefix,
            addrString);

        printf("   \tprefix Length      = %d\n"
                "   \tmetric             = %d\n"
                "   \tDestination prefix = %s\n",
            interAreaPrefixLSA->prefixLength,
            interAreaPrefixLSA->bitsAndMetric,
            addrString);
    }
    else if (LSHeader->linkStateType == OSPFv3_INTER_AREA_ROUTER)
    {
        Ospfv3InterAreaRouterLSA* pInterAreaRouterLSA =
                                    (Ospfv3InterAreaRouterLSA* ) LSA;

        printf("   LSA is a Inter-Area-Router LSA\n");

        printf("   LSA from node %d contains following info :\n",
            LSHeader->advertisingRouter);

        printf("   \tOptions = 0x%x\n"
                "   \tPath Metric = %d\n"
                "   \tDestination Router Id = %d\n",
            pInterAreaRouterLSA->bitsAndOptions,
            pInterAreaRouterLSA->bitsAndPathMetric,
            pInterAreaRouterLSA->destinationRouterId);
    }

    // BGP-OSPF Patch Start
    else if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
    {
        char netStr[MAX_STRING_LENGTH];
        Ospfv3ASexternalLSA* asExternalLSA = (Ospfv3ASexternalLSA* )LSHeader;

        IO_ConvertIpAddressToString(&asExternalLSA->addrPrefix, netStr);

        printf("   LSA is AS External LSA\n");

        printf("   Advertising Router = %d\n",
            LSHeader->advertisingRouter);

        printf("   \tdestination prefix = %s\n", netStr);

        printf("   \tprefix Length      = %d\n",
            asExternalLSA->prefixLength);

        printf("   \tmetric             = %d\n",
            (asExternalLSA->OptionsAndMetric) & 0x00ffffff);
    }
    // BGP-OSPF Patch End

    else
    {
        ERROR_Assert(FALSE, "Cannot print unknown LSA\n");
    }
}

// /**
// FUNCTION   :: Ospfv3PrintHelloPacket
// LAYER      :: NETWORK
// PURPOSE    :: Print the LSA.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + helloPkt : Ospfv3HelloPacket* : Pointer to hello packet.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintHelloPacket(Node* node, Ospfv3HelloPacket* helloPkt)
{
    int i;
    Int32 numNeighbor;
    UInt32* tempNeighborId = NULL;

    numNeighbor =
        (helloPkt->commonHeader.packetLength - sizeof(Ospfv3HelloPacket))
        / sizeof(UInt32);

    tempNeighborId = (unsigned int* ) (helloPkt + 1);

    ERROR_Assert(tempNeighborId,
        "Neighbor not found in the Neighbor list\n");

    printf("Node %u Hello packet content:\n", node->nodeId);

    printf("   Number of neighbor          = %d\n", numNeighbor);

    for (i = 0; i < numNeighbor; i++)
    {
        printf("   NeighborId[%d]               = %d\n",
            i + 1,
            tempNeighborId[i]);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintNeighborList
// LAYER      :: NETWORK
// PURPOSE    :: Print Neighbor List of the node.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// RETURN     ::  void : NULL
// **/
static
void Ospfv3PrintNeighborList(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    Ospfv3Neighbor* tempNeighborInfo = NULL;
    ListItem* tempListItem = NULL;

    printf("Neighbor list for node %u\n", node->nodeId);

    printf("    number of interfaces = %d\n", node->numberInterfaces);

    printf("    size is %d\n", ospf->neighborCount);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        int j = 1;
        char addrStr[MAX_ADDRESS_STRING_LENGTH];

        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        IO_ConvertIpAddressToString(&ospf->pInterface[i].address, addrStr);

        printf("    neighbors on interface %15s\n", addrStr);

        tempListItem = ospf->pInterface[i].neighborList->first;

        // Print out each of our neighbors in our neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

            ERROR_Assert(tempNeighborInfo, "Neighbor data not found\n");

            printf("        neighbor[%d] = %15d\n",
                j,
                tempNeighborInfo->neighborId);

            tempListItem = tempListItem->next;
            j += 1;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3PrintRoutingTable
// LAYER      :: NETWORK
// PURPOSE    :: Print Routing Table of the node.
// PARAMETERS ::
// + node     :  Node* : Pointer to Node.
// RETURN     :: void : NULL
// **/
void Ospfv3PrintRoutingTable(Node* node)
{

    int i;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&ospf->routingTable.buffer);

    char timeStringInSecond[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), timeStringInSecond);

    printf("\nOspfv3 Routing table for node %u at time %15s\n",
        node->nodeId,
        timeStringInSecond);

    printf("   ------------------------------------------------------"
            "---------------------------------------"
            "----------------------\n");

     printf("  %15s  %15s  %15s  %15s  %15s  %15s  %5s  %5s\n",
        "destination",
        "prefixLength",
        "nextHop",
        "metric",
        "DestType",
        "pathType",
        "flag",
        "areaId");

    printf("   ------------------------------------------------------"
            "-------------------------------------------"
            "------------------\n");

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
        char destStr[MAX_ADDRESS_STRING_LENGTH];
        char nextHopStr[MAX_ADDRESS_STRING_LENGTH];
        char destTypeStr[MAX_STRING_LENGTH];
        char pathTypeStr[MAX_STRING_LENGTH];

        if (rowPtr[i].flag == OSPFv3_ROUTE_INVALID)
        {
            continue;
        }

        IO_ConvertIpAddressToString(&rowPtr[i].destAddr, destStr);

        IO_ConvertIpAddressToString(&rowPtr[i].nextHop, nextHopStr);


        if (rowPtr[i].pathType == OSPFv3_INTRA_AREA)
        {
            strcpy(pathTypeStr, "INTRA-AREA");
        }
        else if (rowPtr[i].pathType == OSPFv3_INTER_AREA)
        {
            strcpy(pathTypeStr, "INTER-AREA");
        }
        else if (rowPtr[i].pathType == OSPFv3_TYPE1_EXTERNAL)
        {
            strcpy(pathTypeStr, "TYPE1-EXTERNAL");
        }
        else if (rowPtr[i].pathType == OSPFv3_TYPE2_EXTERNAL)
        {
            strcpy(pathTypeStr, "TYPE2-EXTERNAL");
        }
        else
        {
            ERROR_Assert(FALSE, "Unknown path type in routing table\n");
        }

        if (rowPtr[i].destType == OSPFv3_DESTINATION_ABR)
        {
            strcpy(destTypeStr, "ABR");
        }
        else if (rowPtr[i].destType == OSPFv3_DESTINATION_ASBR)
        {
            strcpy(destTypeStr, "ASBR");
        }
        else if (rowPtr[i].destType == OSPFv3_DESTINATION_ABR_ASBR)
        {
            strcpy(destTypeStr, "ABR_ASBR");
        }
        else if (rowPtr[i].destType == OSPFv3_DESTINATION_NETWORK)
        {
            strcpy(destTypeStr, "NETWORK");
        }
        else
        {
            ERROR_Assert(FALSE, "Unknown path type in routing table\n");
        }

        printf("  %15s  %10d  %20s  %15d  %15s  %15s  %4d",
            destStr,
            rowPtr[i].prefixLength,
            nextHopStr,
            rowPtr[i].metric,
            destTypeStr,
            pathTypeStr,
            rowPtr[i].flag);

        printf("%8d\n", rowPtr[i].areaId);
    }

    printf("\n");

}

// /**
// ---------------------------- Initalization -------------------------------
// **/

// /**
// FUNCTION   :: Ospfv3ReadRouterInterfaceParameters.
// LAYER      :: NETWORK.
// PURPOSE    :: Read user specified interface related parameters from file.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interface: Ospfv3Interface* : Pointer to interface.
// + ospfConfigFile : NodeInput* : Pointer to chached input file.
// RETURN     :: void : NULL
// **/
static
void Ospfv3ReadRouterInterfaceParameters(
    Node* node,
    Ospfv3Interface* pInterface,
    const NodeInput* ospfConfigFile)
{
    char* errStr = new char[MAX_STRING_LENGTH + ospfConfigFile->maxLineLen];
    char* valStr = new char[ospfConfigFile->maxLineLen];
    char* qualifier = new char[ospfConfigFile->maxLineLen];
    char* parameterName = new char[ospfConfigFile->maxLineLen];
    int qualifierLen;
    int i;

    // Process each line of input file
    for (i = 0; i < ospfConfigFile->numLines; i++)
    {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;
        int matchType;
        in6_addr address;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            if (strncmp(currentLine, "HELLO-INTERVAL", 14) == 0)
            {
                sscanf(currentLine, "%*s %s", valStr);

                pInterface->helloInterval = TIME_ConvertToClock(valStr);

                if (pInterface->helloInterval < 1 * SECOND
                    || pInterface->helloInterval > 0xffff * SECOND)
                {
                    sprintf(errStr, "Permissible values for OSPF"
                            " Interface Hello Interval are in the range"
                            " of 1-65535s.");

                    ERROR_ReportWarning(errStr);

                    sprintf(errStr, "Invalid Interface Hello Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" 
                              TYPES_64BITFMT "ds.",
                              OSPFv3_HELLO_INTERVAL / SECOND);

                    ERROR_ReportWarning(errStr);

                    pInterface->helloInterval = OSPFv3_HELLO_INTERVAL;
                }
            }
            else if (strncmp(currentLine, "ROUTER-DEAD-INTERVAL", 20) == 0)
            {
                sscanf(currentLine, "%*s %s", valStr);

                pInterface->routerDeadInterval = TIME_ConvertToClock(valStr);

                if (pInterface->routerDeadInterval < 1 * SECOND
                    || pInterface->routerDeadInterval > 0xffff * SECOND)
                {
                    sprintf(errStr, "Permissible values for OSPF"
                            " Interface Router Dead Interval are in the"
                            " range of 1-65535s.");

                    ERROR_ReportWarning(errStr);

                    sprintf(errStr, "Invalid Interface Router Dead "
                           "Interval Parameter specified, hence continuing "
                           "with the default value: %" TYPES_64BITFMT "ds.",
                        OSPFv3_ROUTER_DEAD_INTERVAL / SECOND);

                    ERROR_ReportWarning(errStr);

                    pInterface->routerDeadInterval
                        = OSPFv3_ROUTER_DEAD_INTERVAL;
                }
            }

            continue;
        }
        
        aStr = strchr(currentLine, ']');
        if (aStr == NULL)
        {
            sprintf(errStr, "Unterminated qualifier:\n %s\n", currentLine);

            ERROR_ReportError(errStr);
        }

        qualifierLen = (int)(aStr - currentLine - 1);

        strncpy(qualifier, &currentLine[1], qualifierLen);

        qualifier[qualifierLen] = '\0';

        aStr++;

        if (!isspace(*aStr))
        {
            sprintf(errStr,
                    "White space required after qualifier:\n %s\n",
                currentLine);

            ERROR_ReportError(errStr);
        }

        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

        Ipv6GetGlobalAggrAddress(node, pInterface->interfaceId, &address);

        if (!QualifierMatches(node->nodeId,
                &address,
                qualifier,
                &matchType))
        {
            continue;
        }

        sscanf(aStr, "%s %s", parameterName, valStr);

        if (!strcmp(parameterName, "INTERFACE-COST"))
        {
            pInterface->outputCost = atoi(valStr);

            if (pInterface->outputCost <= 0
                || pInterface->outputCost > 0xffff)
            {
                sprintf(errStr,
                        "Permissible values for OSPF Outgoing "
                        "Interface cost are in the range of 1-65535.");

                ERROR_ReportWarning(errStr);

                sprintf(errStr,
                        "Invalid Interface Cost Parameter specified,"
                        " hence continuing with the default value: %d.",
                        OSPFv3_DEFAULT_COST);

                ERROR_ReportWarning(errStr);

                pInterface->outputCost = OSPFv3_DEFAULT_COST;
            }
        }
        else if (!strcmp(parameterName, "RXMT-INTERVAL"))
        {
            pInterface->rxmtInterval = TIME_ConvertToClock(valStr);

            if (pInterface->rxmtInterval <= 0)
            {
                sprintf(errStr, "OSPF Interface Retransmission Interval "
                        "must be greater than 0. ");

                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Retransmission Interval"
                        " Parameter specified, hence continuing"
                        " with the default value: %" TYPES_64BITFMT "dS.",
                    OSPFv3_RXMT_INTERVAL / SECOND);

                ERROR_ReportWarning(errStr);

                pInterface->rxmtInterval = OSPFv3_RXMT_INTERVAL;
            }
        }
        else if (!strcmp(parameterName, "INF-TRANS-DELAY"))
        {
            pInterface->infTransDelay = TIME_ConvertToClock(valStr);

            if (pInterface->infTransDelay <= 0)
            {
                sprintf(errStr, "OSPF Interface Transmission Delay "
                        "must be greater than 0.");

                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Transmission Delay"
                        " Parameter specified, hence continuing"
                        " with the default value: %" TYPES_64BITFMT "dS.",
                    OSPFv3_INF_TRANS_DELAY / SECOND);

                ERROR_ReportWarning(errStr);

                pInterface->infTransDelay = OSPFv3_INF_TRANS_DELAY;
            }
        }
        else if (!strcmp(parameterName, "ROUTER-PRIORITY"))
        {
            int val = atoi(valStr);

            pInterface->routerPriority = (unsigned char) val;
            if (val < 0 || val > 0xff)
            {
                sprintf(errStr, "Permissible values for OSPF Router "
                        "Priority are in the range of 1-255.");

                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Router Priority Parameter"
                        " specified, hence continuing with the default"
                        " value: %d.",
                    OSPFv3_DEFAULT_PRIORITY);

                ERROR_ReportWarning(errStr);

                pInterface->routerPriority = OSPFv3_DEFAULT_PRIORITY;
            }
        }
        else if (!strcmp(parameterName, "HELLO-INTERVAL"))
        {
            pInterface->helloInterval = TIME_ConvertToClock(valStr);

            if (pInterface->helloInterval < 1 * SECOND
                || pInterface->helloInterval > 0xffff * SECOND)
            {
                sprintf(errStr, "Permissible values for OSPF"
                        " Interface Hello Interval are in the range"
                        " of 1-65535s.");

                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Hello Interval"
                        " Parameter specified, hence continuing with the "
                        "default value: %" TYPES_64BITFMT "ds.",
                    OSPFv3_HELLO_INTERVAL / SECOND);

                ERROR_ReportWarning(errStr);

                pInterface->helloInterval = OSPFv3_HELLO_INTERVAL;
            }
        }
        else if (!strcmp(parameterName, "ROUTER-DEAD-INTERVAL"))
        {
            pInterface->routerDeadInterval = TIME_ConvertToClock(valStr);

            if (pInterface->routerDeadInterval < 1 * SECOND
                || pInterface->routerDeadInterval > 0xffff * SECOND)
            {
                sprintf(errStr, "Permissible values for OSPF"
                        " Interface Router Dead Interval are in the range"
                        " of 1-65535s.");

                ERROR_ReportWarning(errStr);

                sprintf(errStr, "Invalid Interface Router Dead "
                        "Interval Parameter specified, hence continuing "
                        "with the default value: %" TYPES_64BITFMT "ds.",
                    OSPFv3_ROUTER_DEAD_INTERVAL / SECOND);

                ERROR_ReportWarning(errStr);

                pInterface->routerDeadInterval = OSPFv3_ROUTER_DEAD_INTERVAL;
            }
        }
        else if (!strcmp(parameterName, "INTERFACE-TYPE"))
        {
            if (!strcmp(valStr, "BROADCAST"))
            {
                pInterface->type = OSPFv3_BROADCAST_INTERFACE;
            }
            else if (!strcmp(valStr, "POINT-TO-POINT"))
            {
                pInterface->type = OSPFv3_POINT_TO_POINT_INTERFACE;
            }
            else if (!strcmp(valStr, "POINT-TO-MULTIPOINT"))
            {
                pInterface->type = OSPFv3_POINT_TO_MULTIPOINT_INTERFACE;
            }
            else
            {
                sprintf(errStr,
                        "Unknown OSPF Interface type %s specified.",
                    parameterName);

                ERROR_Assert(FALSE, errStr);
            }
        }
        else
        {
            continue;
        }
    }

    if (pInterface->routerDeadInterval < pInterface->helloInterval)
    {
        char helloInterval[MAX_STRING_LENGTH];
        char rtrDeadInterval[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(pInterface->helloInterval, helloInterval);

        TIME_PrintClockInSecond(
            pInterface->routerDeadInterval,
            rtrDeadInterval);

        sprintf(errStr,
                "OSPF Interface Router Dead Interval ( %ss ) "
                "should be some multiple of Hello Interval ( %ss )",
            rtrDeadInterval,
            helloInterval);

        ERROR_ReportWarning(errStr);
    }
    delete[] errStr;
    delete[] valStr;
    delete[] qualifier;
    delete[] parameterName;
}

// /**
// FUNCTION     :: Ospfv3GetAreaId.
// LAYER        :: NETWORK.
// PURPOSE      :: Get Area Id from chached .ospf file.
// PARAMETERS   ::
// + node       : Node* : Pointer to Node.
// + interface  : int : Interface Identifier.
// + ospfConfigFile : NodeInput* : Pointer to chached input file.
// + areaId     : unsigned int* : Pointer to Area Id Variable.
// RETURN       :: void : NULL.
// **/
static
void Ospfv3GetAreaId(
    Node* node,
    int interfaceId,
    const NodeInput* ospfConfigFile,
    unsigned int* areaId)
{
    if (!ospfConfigFile)
    {
        // Setting default area id and return
        *areaId = OSPFv3_SINGLE_AREA_ID;
        return;
    }

    char* areaIdString = new char[ospfConfigFile->maxLineLen];
    char* qualifier = new char[ospfConfigFile->maxLineLen];
    char* parameterName = new char[ospfConfigFile->maxLineLen];
    int qualifierLen;
    int i;

    for (i = 0; i < ospfConfigFile->numLines; i++)
    {
        char* currentLine = ospfConfigFile->inputStrings[i];
        char* aStr = NULL;
        BOOL isNodeId;
        int matchType;
        in6_addr address;

        // Skip lines that are not begining with "["
        if (strncmp(currentLine, "[", 1) != 0)
        {
            continue;
        }

        aStr = strchr(currentLine, ']');
        if (aStr == NULL)
        {
            char* errStr = new char[MAX_STRING_LENGTH + strlen(currentLine)];

            sprintf(errStr, "Unterminated qualifier:\n %s\n", currentLine);

            ERROR_ReportError(errStr);
        }

        qualifierLen = (int)(aStr - currentLine - 1);

        strncpy(qualifier, &currentLine[1], qualifierLen);

        qualifier[qualifierLen] = '\0';
        aStr++;

        if (!isspace(*aStr))
        {
            char* errStr = new char[MAX_STRING_LENGTH + strlen(currentLine)];

            sprintf(errStr,
                    "White space required after qualifier:\n %s\n",
                currentLine);

            ERROR_ReportError(errStr);
        }

        // skip white space
        while (isspace(*aStr))
        {
            aStr++;
        }

        Ipv6GetGlobalAggrAddress(node, interfaceId, &address);

        if (!QualifierMatches(node->nodeId,
                &address,
                qualifier,
                &matchType))
        {
            continue;
        }

        sscanf(aStr, "%s %s", parameterName, areaIdString);

        if (strcmp(parameterName, "AREA-ID"))
        {
            continue;
        }

        IO_ParseNodeIdOrHostAddress(areaIdString, areaId, &isNodeId);

        if ((*areaId) >= OSPFv3_INVALID_AREA_ID)
        {
            Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                                node,
                                                ROUTING_PROTOCOL_OSPFv3,
                                                NETWORK_IPV6);

            if (ospf->partitionedIntoArea == TRUE)
            {
                ERROR_Assert(FALSE, "Area Specified is Invalid");
            }
        }

        if (isNodeId)
        {
            ERROR_ReportError("The format is AREA-ID  "
                              "<Area ID in IP address format>\n");
        }

        break;
    }
    if (i == ospfConfigFile->numLines)
    {
        char* errStr = new char[MAX_STRING_LENGTH + 
                                ospfConfigFile->maxLineLen];
        char addrStr[MAX_ADDRESS_STRING_LENGTH];
        in6_addr addr;

        Ipv6GetGlobalAggrAddress(node, interfaceId, &addr);

        IO_ConvertIpAddressToString(&addr, addrStr);

        sprintf(errStr,
                "Node %u: AreaId not specified on interface %15s\n",
            node->nodeId,
            addrStr);

        ERROR_ReportError(errStr);
    }
    delete[] areaIdString;
    delete[] qualifier;
    delete[] parameterName;
}// Function End

// /**
// FUNCTION :: Ospfv3InitArea.
// LAYER    :: NETWORK.
// PURPOSE  :: Initialize Area configuration from chached .ospf file.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + ospfConfigFile : NodeInput* : Pointer to chached input file.
// + interface : int : Interface Identifier.
// + areaId : unsigned int : Area Id.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3InitArea(
    Node* node,
    const NodeInput* ospfConfigFile,
    int interfaceId,
    unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* newArea = (Ospfv3Area* ) MEM_malloc(sizeof(Ospfv3Area));

    memset(newArea, 0, sizeof(Ospfv3Area));

    int i;

    if (ospf->area->size == OSPFv3_MAX_ATTACHED_AREA)
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u attached to more than %d area.\n"
                "Please increase the value of"
                "\"OSPFv3_MAX_ATTACHED_AREA\" in ospfv3.h\n",
            node->nodeId,
            OSPFv3_MAX_ATTACHED_AREA);

        ERROR_ReportError(errStr);
    }

    newArea->areaId = areaId;

    ListInit(node, &newArea->areaAddrRange);

    ListInit(node, &newArea->connectedInterface);

    ListInit(node, &newArea->routerLSAList);

    ListInit(node, &newArea->networkLSAList);

    ListInit(node, &newArea->interAreaPrefixLSAList);

    ListInit(node, &newArea->interAreaRouterLSAList);

    ListInit(node, &newArea->intraAreaPrefixLSAList);

    ListInit(node, &newArea->unknownLSAUbitOne);

    ListInit(node, &newArea->groupMembershipLSAList);

    newArea->routerLSTimer = FALSE;
    newArea->routerLSAOriginateTime = (clocktype) 0;
    newArea->groupMembershipLSTimer = FALSE;
    newArea->groupMembershipLSAOriginateTime = (clocktype) 0;

    // If entire domain defined as single area
    if (areaId == OSPFv3_SINGLE_AREA_ID)
    {
        ospf->areaBorderRouter = FALSE;
        newArea->transitCapability = TRUE;
        newArea->externalRoutingCapability = TRUE;
        newArea->stubDefaultCost = 0;

        // Make a link to connected interface structure
        ListInsert(
            node,
            newArea->connectedInterface,
            0,
            &ospf->pInterface[interfaceId]);

        // Insert area structure to list
        ListInsert(node, ospf->area, 0, newArea);

        return;
    }

    // By default the are has transit capability
    newArea->transitCapability = TRUE;
    newArea->externalRoutingCapability = TRUE;
    newArea->stubDefaultCost = 0;

    
    if (ospfConfigFile)
    {
        // Process each line of input file
        for (i = 0; i < ospfConfigFile->numLines; i++)
        {
            char* currentLine = ospfConfigFile->inputStrings[i];
            char* areaIdString = new char[ospfConfigFile->maxLineLen];
            BOOL isNodeId;
            unsigned int currentAreaId;
            char* parameterName = new char[ospfConfigFile->maxLineLen];

            // Skip lines that are not begining with "AREA" keyword
            if (strncmp(currentLine, "AREA ", 5) != 0)
            {
                delete[] areaIdString;
                delete[] parameterName;
                continue;
            }

            sscanf(currentLine, "%*s %s %s", areaIdString, parameterName);

            IO_ParseNodeIdOrHostAddress(
                areaIdString,
                &currentAreaId,
                &isNodeId);

            // Skip lines that are not for this area
            if (currentAreaId != areaId)
            {
                delete[] areaIdString;
                delete[] parameterName;
                continue;
            }

            // Get address range
            if (strcmp(parameterName, "RANGE") == 0)
            {
                // For IO_GetDelimitedToken
                char* next;
                char* ptr = NULL;
                const char* delims = "{,} \t\n";
                char* token = NULL;
                char iotoken[OSPFv3_MAX_STRING_LENGTH];
                char areaRangeString[OSPFv3_MAX_STRING_LENGTH];
                char* asIdStr;
                BOOL alwaysTrue = TRUE;

                if ((ptr = strchr(currentLine,'{')) == NULL)
                {
                    sprintf(areaRangeString,
                            "Could not find '{' character:\n%s\n",
                        currentLine);

                    ERROR_ReportError(areaRangeString);
                }

                if ((asIdStr = strchr(currentLine, '}')) == NULL)
                {
                    sprintf(areaRangeString,
                            "Could not find '}' character:\n%s\n",
                        currentLine);

                    ERROR_ReportError(areaRangeString);
                }
                else
                {
                    unsigned int asId = 0;
                    int retVal = 0;

                    asIdStr += 1;

                    retVal = sscanf(asIdStr, "%u", &asId);

                    if (retVal && asId != ospf->asId)
                    {
                        delete[] areaIdString;
                        delete[] parameterName;
                        continue;
                    }
                }

                strncpy(areaRangeString, ptr, asIdStr - ptr);

                areaRangeString[asIdStr - ptr] = '\0';

                token = IO_GetDelimitedToken(
                            iotoken,
                            areaRangeString,
                            delims,
                            &next);

                if (!token)
                {
                    sprintf(areaRangeString,
                            "Cann't find network prefix, "
                            " :\n%s\n", currentLine);

                    ERROR_ReportError(areaRangeString);
                }

                while (alwaysTrue)
                {
                    Ospfv3AreaAddressRange* newData = NULL;
                    in6_addr addressPrefix;
                    unsigned int prefixLength = 0;
                    unsigned int tla = 0;
                    unsigned int nla = 0;
                    unsigned int sla = 0;
                    int k;

                    if (IO_FindCaseInsensitiveStringPos(token, "SLA") != -1)
                    {

                        IO_ParseNetworkAddress(token, &tla, &nla, &sla);

                        MAPPING_CreateIpv6SubnetAddr(
                            tla,
                            nla,
                            sla,
                            &prefixLength,
                            &addressPrefix);

                    }
                    else
                    {
                        IO_ParseNetworkAddress(
                            token,
                            &addressPrefix,
                            &prefixLength);
                    }

                    newData = (Ospfv3AreaAddressRange* )
                        MEM_malloc(sizeof(Ospfv3AreaAddressRange));

                    newData->addressPrefix = addressPrefix;
                    newData->prefixLength = (unsigned char)prefixLength;

                    newData->advertise = TRUE;

                    // Link back to main area structure
                    newData->area = (void* ) newArea;

                    for (k = 0; k < OSPFv3_MAX_ATTACHED_AREA; k++)
                    {
                        newData->advrtToArea[k] = FALSE;
                    }

                    ListInsert(node, newArea->areaAddrRange,0, newData);

                    // Retrieve next token.
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token)
                    {
                        break;
                    }
                } //while end
            }
            // Check whether this area has been configured as stub area
            else if (strcmp(parameterName, "STUB") == 0)
            {
                Int32 stubDefaultCost;
                unsigned int asId;
                int numParameters= 0;

                // Get stub default cost
                numParameters = sscanf(currentLine,
                                        "%*s %*s %*s %d %u",
                                    &stubDefaultCost,
                                    &asId);

                if (numParameters == 1 || asId == ospf->asId)
                {
                    newArea->transitCapability = FALSE;
                    newArea->externalRoutingCapability = FALSE;
                    newArea->stubDefaultCost = stubDefaultCost;
                }
            }
            delete[] areaIdString;
            delete[] parameterName;
        } //for end
    }

    // Make a link to connected interface structure
    ListInsert(node,
        newArea->connectedInterface,
        0,
        &ospf->pInterface[interfaceId]);

    // Insert area structure to list
    ListInsert(node, ospf->area, 0, newArea);

    if (ospf->area->size > 1)
    {
        ospf->areaBorderRouter = TRUE;
    }
    else
    {
        ospf->areaBorderRouter = FALSE;
    }

    // If this is backbone area
    if (areaId == 0)
    {
        ospf->backboneArea = newArea;
    }
}

// /**
// FUNCTION :: Ospfv3AddInterfaceToArea.
// LAYER    :: NETWORK.
// PURPOSE  :: Add interface to the List of connected interface in the Area.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + areaId : unsigned int : Area Id.
// + interface : int : Interface Identifier.
// RETURN   :: void : NULL.
// ** /
static
void Ospfv3AddInterfaceToArea(
    Node* node,
    unsigned int areaId,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv3Area* areaInfo = (Ospfv3Area* ) listItem->data;

        if (areaInfo->areaId == areaId)
        {
            // Make a link to connected interface structure
            ListInsert(
                node,
                areaInfo->connectedInterface,
                0,
                &ospf->pInterface[interfaceId]);

            break;
        }
    }

    ERROR_Assert(listItem, "Unable to add interface. Area doesn't exist.\n");

}//Function End

// /**
// FUNCTION :: Ospfv3InitRoutingTable.
// LAYER    :: NETWORK.
// PURPOSE  :: Allocate memory for initial routing table.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// RETURN   :: void : NULL.
// ** /
static
void Ospfv3InitRoutingTable(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int size = sizeof(Ospfv3RoutingTableRow) * OSPFv3_INITIAL_TABLE_SIZE;

    BUFFER_InitializeDataBuffer(&ospf->routingTable.buffer, size);

    ospf->routingTable.numRows = 0;
}

// /*
// FUNCTION :: Ospfv3ScheduleRouterLSA.
// LAYER    :: NETWORK.
// PURPOSE  :: Schedule router LSA origination. If Flush is true, then LSA is
//             removed from routing domain.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + areaId : unsigned int : Area Id.
// + flush : BOOL : Flushing of LSA.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ScheduleRouterLSA(
    Node* node,
    unsigned int areaId,
    BOOL flush)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    ERROR_Assert(thisArea, "Area doesn't exist\n");

    if (thisArea->routerLSTimer == TRUE)
    {
        return;
    }
    else if (thisArea->routerLSAOriginateTime == 0)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);

        delay += ospf->staggerStartTime;
    }
    else if ((node->getNodeTime() - thisArea->routerLSAOriginateTime)
            >= OSPFv3_MIN_LS_INTERVAL)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv3_MIN_LS_INTERVAL - (node->getNodeTime()
                            - thisArea->routerLSAOriginateTime));
    }
    thisArea->routerLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType)
        + sizeof(unsigned int)
        + sizeof(BOOL));

    lsType = OSPFv3_ROUTER;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &areaId, sizeof(unsigned int));

    msgInfo += sizeof(unsigned int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) delay);

}// Function End

// /*
// FUNCTION :: Ospfv3ScheduleLinkLSA.
// LAYER    :: NETWORK.
// PURPOSE  :: Schedule Link LSA origination. If Flush is true, then LSA is
//             removed from routing domain.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interfaceId : unsigned int : Interface Id.
// + flush : BOOL : Flushing of LSA.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ScheduleLinkLSA(
    Node* node,
    unsigned int interfaceId,
    BOOL flush)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    clocktype LSAOriginateTime =
        ospf->pInterface[interfaceId].linkLSAOriginateTime;

    if (ospf->pInterface[interfaceId].linkLSTimer == TRUE)
    {
        return;
    }
    else if (LSAOriginateTime == 0)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);

        delay += ospf->staggerStartTime;
    }
    else if ((node->getNodeTime() - LSAOriginateTime) >= OSPFv3_MIN_LS_INTERVAL)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv3_MIN_LS_INTERVAL
                            - (node->getNodeTime()- LSAOriginateTime));
    }
    ospf->pInterface[interfaceId].linkLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType)
        + sizeof(unsigned int)
        + sizeof(BOOL));

    lsType = OSPFv3_LINK;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &interfaceId, sizeof(unsigned int));

    msgInfo += sizeof(unsigned int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}

// /*
// FUNCTION :: Ospfv3ScheduleIntraAreaPrefixLSA.
// LAYER    :: NETWORK.
// PURPOSE  :: Schedule Intra Area Prefix LSA origination. If Flush is true,
//             then LSA is removed from routing domain.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + areaId : unsigned int : Area Id.
// + flush : BOOL : Flushing of LSA.
// + refLSType : Ospfv3LinkStateType : reference LS type.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ScheduleIntraAreaPrefixLSA(
    Node* node,
    unsigned int areaId,
    BOOL flush,
    Ospfv3LinkStateType refLSType)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = NULL;

    unsigned int interfaceId = 0;

    if (refLSType == OSPFv3_NETWORK)
    {
        //areaId is infact interfaceId of atransit network;
        interfaceId = areaId;

        thisArea = Ospfv3GetArea(node, ospf->pInterface[interfaceId].areaId);

    }
    else
    {
        thisArea = Ospfv3GetArea(node, areaId);
    }

    ERROR_Assert(thisArea, "Area doesn't exist\n");

    if (thisArea->interAreaPrefixLSTimer == TRUE)
    {
        return;
    }
    else if (thisArea->interAreaPrefixLSTimer == 0)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);

        delay += ospf->staggerStartTime;
    }
    else if ((node->getNodeTime() - thisArea->interAreaPrefixLSOriginateTime)
            >= OSPFv3_MIN_LS_INTERVAL)
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv3_MIN_LS_INTERVAL- (node->getNodeTime()
                            - thisArea->interAreaPrefixLSOriginateTime));
    }
    thisArea->routerLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType)
        + sizeof(unsigned int)
        + sizeof(BOOL)
        + sizeof(unsigned int));

    lsType = OSPFv3_INTRA_AREA_PREFIX;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &areaId, sizeof(unsigned int));

    msgInfo += sizeof(unsigned int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    msgInfo += sizeof(BOOL);

    memcpy(msgInfo, &refLSType, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}

// /*
// FUNCTION :: Ospfv3DRFullyAdjacentWithAnyRouter.
// LAYER    :: NETWORK.
// PURPOSE  :: Verify if router is fully adjacent with any of it's
//             neighbor.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interfaceId : int : interface Id.
// RETURN   :: TRUE if DR fully ajacent with any Router, FALSE otherwise.
// **/
static
BOOL Ospfv3DRFullyAdjacentWithAnyRouter(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* neighborInfo = NULL;
    ListItem* listItem = ospf->pInterface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv3Neighbor* ) listItem->data;

        if (neighborInfo->state == S_Full)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

// /*
// FUNCTION :: Ospfv3ScheduleNetworkLSA.
// LAYER    :: NETWORK.
// PURPOSE  :: Schedule Network LSA origination.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interfaceId : int : interface Id.
// + flush : BOOL : Flushing of LSA.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ScheduleNetworkLSA(
    Node* node,
    int interfaceId,
    BOOL flush)
{
    Message* newMsg = NULL;
    clocktype delay;
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;
    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    BOOL haveAdjNbr = FALSE;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Interface* thisInterface = &ospf->pInterface[interfaceId];

    Ospfv3Area* thisArea = Ospfv3GetArea(node, thisInterface->areaId);

    ERROR_Assert(thisArea, "Area not fount\n");

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->networkLSAList,
                        ospf->routerId,
                        interfaceId);

    haveAdjNbr = Ospfv3DRFullyAdjacentWithAnyRouter(node, interfaceId);

    if (haveAdjNbr && !flush)
    {
        // Originate LSA
    }
    else if (oldLSHeader && (!haveAdjNbr || flush))
    {
        // Flush LSA
        flush = TRUE;
    }
    else
    {
        // Do nothing
        return;
    }

    if (thisInterface->networkLSTimer
        && !flush)
    {
        return;
    }

    // We need to cancel previous timer if any
    thisInterface->networkLSTimerSeqNumber++;

    if ((thisInterface->networkLSAOriginateTime == 0)
        || ((node->getNodeTime() - thisInterface->networkLSAOriginateTime)
        >= OSPFv3_MIN_LS_INTERVAL))
    {
        delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv3_MIN_LS_INTERVAL - (node->getNodeTime()
                            - thisInterface->networkLSAOriginateTime));
    }

    thisInterface->networkLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleLSDB);
    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType) + sizeof(int)
        + sizeof(BOOL) + sizeof(int));

    lsType = OSPFv3_NETWORK;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &interfaceId, sizeof(int));

    msgInfo += sizeof(int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    msgInfo += sizeof(BOOL);

    memcpy(msgInfo, &thisInterface->networkLSTimerSeqNumber, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) delay);

}

// /*
// FUNCTION :: Ospfv3IsRoutingProtocolEnabled.
// LAYER    :: NETWORK.
// PURPOSE  :: To check if OSPFv3 is enabled at particular interface or not.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interfaceId : int : interface Id.
// + nodeInput : const NodeInput* : Pointer to chached config file.
// RETURN   :: TRUE if Ospfv3 is enabled on the Interface, FALSE otherwise.
// **/
static
BOOL Ospfv3IsRoutingProtocolEnabled(
    Node* node,
    unsigned int interfaceId,
    const NodeInput* nodeInput)
{
    in6_addr address;
    BOOL funcRetVal = FALSE;

    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    if (NetworkIpGetInterfaceType(node, interfaceId) == NETWORK_IPV4)
    {
        return funcRetVal;
    }

    Ipv6GetGlobalAggrAddress(node, interfaceId, &address);

    ////IO_ReadString(
    ////    node->nodeId,
    ////    &address,
    ////    nodeInput,
    ////    "ROUTING-PROTOCOL",
    ////    &retVal,
    ////    routingProtocolString);

    ////if (retVal && strcmp(routingProtocolString, "OSPFv3") == 0)
    if (ip->interfaceInfo[interfaceId]->ipv6InterfaceInfo->
                    routingProtocolType == ROUTING_PROTOCOL_OSPFv3)
    {
        funcRetVal = TRUE;
    }

    return funcRetVal;
}

// /*
// FUNCTION :: Ospfv3ScheduleWaitTimer.
// LAYER    :: NETWORK.
// PURPOSE  :: Schedule wait timer for broadcast interface.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + interfaceId : int : interface Id.
// RETURN   :: void : NULL.
// **/
void Ospfv3ScheduleWaitTimer(Node* node, unsigned int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Message* waitTimerMsg;
    clocktype delay;

    ospf->pInterface[interfaceId].state = S_Waiting;

    // Send wait timer to self
    waitTimerMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        MSG_ROUTING_OspfWaitTimer);

    MESSAGE_InfoAlloc(node, waitTimerMsg, sizeof(int));

    memcpy(MESSAGE_ReturnInfo(waitTimerMsg), &interfaceId, sizeof(int));

    // Use jitter to avoid synchrinisation
    delay = (clocktype) (RANDOM_nrand (ospf->seed) % OSPFv3_BROADCAST_JITTER
                        + OSPFv3_WAIT_TIMER);

    MESSAGE_Send(
        node,
        waitTimerMsg,
        delay + ospf->staggerStartTime);
}

BOOL IsConnectedToSwitch(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL isSwitch = FALSE;
    LinkData* linkData;

    if (node->macData[interfaceIndex]->macProtocol != MAC_PROTOCOL_LINK &&
        node->macData[interfaceIndex]->macProtocol != MAC_PROTOCOL_802_3)
    {
        return FALSE;
    }

    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK)
    {
        linkData = (LinkData* )(node->macData[interfaceIndex]->macVar);
    }
    else
    {
        linkData = (LinkData* )((MacData802_3 *)
                   (node->macData[interfaceIndex]->macVar))->link;
    }

    IO_ReadString(
        linkData->destNodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH",
        &retVal,
        buf);

    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            isSwitch = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            isSwitch = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "Switch_CheckAndInit: "
                "Error in SWITCH specification.\n"
                "Expecting YES or NO\n");
        }
    }
    return isSwitch;
}

// /*
// FUNCTION :: Ospfv3InitInterface.
// LAYER    :: NETWORK.
// PURPOSE  :: Initialize the available network interface.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + ospfConfigFile : const NodeInput* : Pointer to chached .ospf file.
// + nodeInput : const NodeInput* : Pointer to chached config file.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3InitInterface(
    Node* node,
    const NodeInput* ospfConfigFile,
    const NodeInput* nodeInput)
{
#ifdef DEBUG
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
#endif

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;

    ospf->pInterface = (Ospfv3Interface* ) MEM_malloc(
                                            sizeof(Ospfv3Interface)
                                            * node->numberInterfaces);

    memset(
        ospf->pInterface,
        0,
        sizeof(Ospfv3Interface) * node->numberInterfaces);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        //LinkData* linkData = (LinkData* ) (node->macData[i]->macVar);
        BOOL isWired = TRUE;

        ospf->pInterface[i].helloInterval = OSPFv3_HELLO_INTERVAL;

        if (Ospfv3IsRoutingProtocolEnabled(node, i, nodeInput) == FALSE)
        {
            ospf->pInterface[i].type = OSPFv3_NON_OSPF_INTERFACE;
            continue;
        }

        ERROR_Assert(NetworkIpGetInterfaceType(node, i) == NETWORK_DUAL
                    ||NetworkIpGetInterfaceType(node, i) == NETWORK_IPV6,
                    "Currenly, OSPFv3 can only run on IPv6 node or"
                    " dual IP node\n");


        // Determine network type.
        // The default operation mode of OSPF is BROADCAST
        // or POINT_TO_POINT depending on underlying media type.
        if (TunnelIsVirtualInterface(node, i))
        {
            ospf->pInterface[i].type = OSPFv3_POINT_TO_POINT_INTERFACE;
        }
        else if (MAC_IsOneHopBroadcastNetwork(node, i) ||
            IsConnectedToSwitch(node, nodeInput, i))
        {
            ospf->pInterface[i].type = OSPFv3_BROADCAST_INTERFACE;
        }
        else if (MAC_IsPointToMultiPointNetwork(node, i))
        {
            ospf->pInterface[i].type = OSPFv3_POINT_TO_MULTIPOINT_INTERFACE;
        }
        else
        {
            ospf->pInterface[i].type = OSPFv3_POINT_TO_POINT_INTERFACE;
        }

        // Interface index will be used as interfaceId
        ospf->pInterface[i].interfaceId = i;

        // instanceId won't be used for current implementation
        ospf->pInterface[i].instanceId = 0;

        //virtual links are not considered in current development
        Ipv6GetGlobalAggrAddress(node, i, &ospf->pInterface[i].address);

        ospf->pInterface[i].prefixLen =
            (unsigned char)Ipv6GetPrefixLength(node, i);

        ospf->pInterface[i].routerDeadInterval = OSPFv3_ROUTER_DEAD_INTERVAL;
        ospf->pInterface[i].infTransDelay = OSPFv3_INF_TRANS_DELAY;

        // Set default router priority.
        ospf->pInterface[i].routerPriority = OSPFv3_DEFAULT_PRIORITY;
        ospf->pInterface[i].outputCost = OSPFv3_DEFAULT_COST;
        ospf->pInterface[i].rxmtInterval = OSPFv3_RXMT_INTERVAL;

        if (TunnelIsVirtualInterface(node, i) == FALSE
            && (isWired = MAC_IsWiredNetwork(node, i)) == FALSE)
        {
            ospf->pInterface[i].type = OSPFv3_POINT_TO_MULTIPOINT_INTERFACE;
            ospf->pWirelessFlag = TRUE;
        }

        //Check for interface configurable parameter
        if (ospfConfigFile)
        {
            Ospfv3ReadRouterInterfaceParameters(
                node,
                &ospf->pInterface[i],
                ospfConfigFile);
        }

        // Initializes area data structure, if area is enabled
        if (ospf->partitionedIntoArea == TRUE)
        {
            unsigned int areaId;

            // Get Area Id associated with this interface
            Ospfv3GetAreaId(node, i, ospfConfigFile, &areaId);

            ospf->pInterface[i].areaId = areaId;

            if (!Ospfv3GetArea(node, areaId))
            {
                Ospfv3InitArea(node, ospfConfigFile, i, areaId);
            }
            else
            {
                Ospfv3AddInterfaceToArea(node, areaId, i);
            }
        }
        else
        {
            // Consider the routing domain as single area
            ospf->pInterface[i].areaId = OSPFv3_SINGLE_AREA_ID;

            if (!Ospfv3GetArea(node, OSPFv3_SINGLE_AREA_ID))
            {
                Ospfv3InitArea(
                    node,
                    ospfConfigFile,
                    i,
                    OSPFv3_SINGLE_AREA_ID);
            }
            else
            {
                Ospfv3AddInterfaceToArea(node, OSPFv3_SINGLE_AREA_ID, i);
            }
        }

        // Set initial interface state
        if (NetworkIpInterfaceIsEnabled(node, i))
        {
            if (ospf->pInterface[i].type == OSPFv3_BROADCAST_INTERFACE)
            {
                Ospfv3ScheduleWaitTimer(node, i);
            }
            else
            {
                ospf->pInterface[i].state = S_PointToPoint;
            }

            // Interface State change so schedule Router, Link
            // and Intra-Area-Prefix LSA
            Ospfv3ScheduleRouterLSA(
                node,
                ospf->pInterface[i].areaId,
                FALSE);

            // Schedule Link LSA for link connected to interace.
            //Ospfv3ScheduleLinkLSA(node, i, FALSE);

            Ospfv3ScheduleIntraAreaPrefixLSA(
                node,
                ospf->pInterface[i].areaId,
                FALSE,
                OSPFv3_ROUTER);
        }
        else
        {
            ospf->pInterface[i].state = S_Down;
        }

        ListInit(node, &ospf->pInterface[i].updateLSAList);

        ListInit(node, &ospf->pInterface[i].delayedAckList);

        ListInit(node, &ospf->pInterface[i].neighborList);

        // Unknown LSA not handled.
        ListInit(node, &ospf->pInterface[i].linkPrefixList);

        ListInit(node, &ospf->pInterface[i].linkLSAList);
    }
    if (ospf->partitionedIntoArea && ospf->areaBorderRouter
        && !ospf->backboneArea)
    {
        ERROR_ReportError("Node connected to multiple area must have "
                            "an interface to backbone\n");
    }
}// Function End

// /*
// FUNCTION :: Ospfv3CompDestType.
// LAYER    :: NETWORK.
// PURPOSE  :: Compare two Dest type.
// PARAMETERS ::
// + destType1 : Ospfv3DestType : network destination type.
// + destType2 : Ospfv3DestType : network destination type.
// RETURN   :: Return TRUE if the dest types are same otherwise FALSE.
// **/
static
BOOL Ospfv3CompDestType(Ospfv3DestType destType1, Ospfv3DestType destType2)
{
    if (destType1 == destType2)
    {
        return TRUE;
    }
    else if (destType1 == OSPFv3_DESTINATION_ABR_ASBR
            && ((destType2 == OSPFv3_DESTINATION_ABR)
            || (destType2 == OSPFv3_DESTINATION_ASBR)))
    {
        return TRUE;
    }
    else if (destType2 == OSPFv3_DESTINATION_ABR_ASBR
            && ((destType1 == OSPFv3_DESTINATION_ABR)
            || (destType1 == OSPFv3_DESTINATION_ASBR)))
    {
        return TRUE;
    }

    return FALSE;
}// Function End

// /*
// FUNCTION :: Ospfv3GetIntraAreaRoute.
// LAYER    :: NETWORK.
// PURPOSE  :: Get desired intra area route from routing table.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + destAddr : in6_addr : destination address.
// + destType : Ospfv3DestType : network destination type.
// + areaId : unsigned int : Area Id.
// RETURN   :: Route entry. NULL if route not found.
// **/
static
Ospfv3RoutingTableRow* Ospfv3GetIntraAreaRoute(
    Node* node,
    in6_addr destAddr,
    Ospfv3DestType destType,
    unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        in6_addr destPrefix;
        in6_addr tableDestPrefix;

        Ipv6GetPrefix(
            &rowPtr[i].destAddr,
            &tableDestPrefix,
            rowPtr[i].prefixLength);

        Ipv6GetPrefix(
            &destAddr,
            &destPrefix,
            rowPtr[i].prefixLength);

        if (CMP_ADDR6(tableDestPrefix, destPrefix) == 0
            && Ospfv3CompDestType(rowPtr[i].destType, destType)
            && (rowPtr[i].pathType == OSPFv3_INTRA_AREA)
            && (rowPtr[i].areaId == areaId))
        {
            return &rowPtr[i];
        }
    }
    return NULL;
}//Function End

// /*
// FUNCTION :: Ospfv3GetRoute.
// LAYER    :: NETWORK.
// PURPOSE  :: Get desired route from routing table.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + destAddr : in6_addr : destination address.
// + destType : Ospfv3DestType : network destination type.
// RETURN   :: Route entry. NULL if route not found.
// **/
static
Ospfv3RoutingTableRow* Ospfv3GetRoute(
    Node* node,
    in6_addr destAddr,
    Ospfv3DestType destType)
{
    Ospfv3Data* ospf = (Ospfv3Data* )
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv3, NETWORK_IPV6);

    int i = 0;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (CMP_ADDR6(rowPtr[i].destAddr, destAddr) == 0
            && Ospfv3CompDestType(rowPtr[i].destType, destType))
        {
            return &rowPtr[i];
        }
    }
    return NULL;
}//Function End

// /*
// FUNCTION :: Ospfv3RoutesMatchSame.
// LAYER    :: NETWORK.
// PURPOSE  :: Check if two routes are identical.
// PARAMETERS ::
// + newRoute : Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// + oldRoute : Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// RETURN   :: TRUE if routes are same, FALSE otherwise.
// **/
static
BOOL Ospfv3RoutesMatchSame(
    Ospfv3RoutingTableRow* newRoute,
    Ospfv3RoutingTableRow* oldRoute)
{
    if ((newRoute->metric == oldRoute->metric)
        && (CMP_ADDR6(newRoute->nextHop, oldRoute->nextHop)) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

// /*
// FUNCTION :: Ospfv3AddRoute.
// LAYER    :: NETWORK.
// PURPOSE  :: Add route to routing table.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + newRoute : Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3AddRoute(
    Node* node,
    Ospfv3RoutingTableRow* newRoute)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr;

    // Get old route if any ..
    if (newRoute->destType != OSPFv3_DESTINATION_NETWORK)
    {
        rowPtr = Ospfv3GetIntraAreaRoute(
                    node,
                    newRoute->destAddr,
                    newRoute->destType,
                    newRoute->areaId);
    }
    else
    {
        rowPtr = Ospfv3GetRoute(
                    node,
                    newRoute->destAddr,
                    newRoute->destType);
    }

    if (rowPtr)
    {
        if (Ospfv3RoutesMatchSame(newRoute, rowPtr))
        {
            newRoute->flag = OSPFv3_ROUTE_NO_CHANGE;
        }
        else
        {
            newRoute->flag = OSPFv3_ROUTE_CHANGED;
        }

        memcpy(rowPtr, newRoute, sizeof(Ospfv3RoutingTableRow));
    }
    else
    {
        newRoute->flag = OSPFv3_ROUTE_NEW;
        rtTable->numRows++;

        BUFFER_AddDataToDataBuffer(
            &rtTable->buffer,
            (char* ) newRoute,
            sizeof(Ospfv3RoutingTableRow));
    }
}//Function End


// /**
// FUNCTION :: Ospfv3ParseNetworkAddress.
// LAYER    :: NETWORK.
// PURPOSE  :: Parse Nexthop Network Address.
// PARAMETERS ::
// + addrString : const char* : Pointer to string.
// + ipAddress : in6_addr : Ip Address.
// + isIpAddr : BOOL* : Is Ip address or something else.
// + isNodeId : NodeId* : Pointer Node Id.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ParseNetworkAddress(
    const char addrString[],
    in6_addr* ipAddress,
    BOOL* isIpAddr,
    NodeId* isNodeId,
    unsigned int* prefixLength)
{
    if (!strchr(addrString, ':'))
    {
        IO_ParseNodeIdHostOrNetworkAddress(
            addrString,
            ipAddress,
            isIpAddr,
            isNodeId);

        if ((*isIpAddr) && !(*isNodeId))
        {
            if (ipAddress->s6_addr32[2] != 0 &&
                ipAddress->s6_addr32[3] != 0)
            {
                char err[MAX_STRING_LENGTH];

                sprintf(err,
                        "Static Routing: destination must be a "
                        "network\n  %s\n",
                    addrString);

                ERROR_ReportError(err);
            }
            *prefixLength = OSPFv3_MAX_PREFIX_LENGTH;
        }
        else
        {
            if (!(*isNodeId))
            {
                unsigned int tla = ipAddress->s6_addr32[0];
                unsigned int nla = ipAddress->s6_addr32[1];
                unsigned int sla = ipAddress->s6_addr32[2];

                MAPPING_CreateIpv6SubnetAddr(
                    tla,
                    nla,
                    sla,
                    prefixLength,
                    ipAddress);

            }
            else
            {
                char err[MAX_STRING_LENGTH];

                // Error Message
                sprintf(err,
                        "Static Routing: destination must be a "
                        "network or host address\n  %s\n",
                    addrString);

                ERROR_ReportError(err);
            }
        }
    }
    else
    {
        IO_ParseNodeIdHostOrNetworkAddress(
            addrString,
            ipAddress,
            isIpAddr,
            isNodeId,
            prefixLength);
    }
}

// BGP-OSPF Patch Start
// /**
// FUNCTION :: Ospfv3ExternalRouteInit.
// LAYER    :: NETWORK.
// PURPOSE  :: Inject "External Route" into Routing Table
//             for AS Boundary Router.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + ospfExternalRouteInput : const NodeInput* : Pointer to chached External
//                                               Static Route File.
// RETURN   :: void : NULL.
// **/
static
void Ospfv3ExternalRouteInit(
    Node* node,
    const NodeInput* ospfExternalRouteInput)
{
    int i;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    for (i = 0; i < ospfExternalRouteInput->numLines; i++)
    {
        char destAddressString[MAX_STRING_LENGTH];
        char nextHopString[MAX_STRING_LENGTH];

        // In Static route file it is assumed that Source is
        // represented by node Id
        unsigned int sourceNodeId;
        int numParameters;
        int cost;

        numParameters = sscanf(ospfExternalRouteInput->inputStrings[i],
                            "%u %s %s %d",
                            &sourceNodeId,
                            destAddressString,
                            nextHopString,
                            &cost);

        if ((numParameters < 3) || (numParameters > 4))
        {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "OspfExternalRouting: Wrong number "
                    "of addresses in external route entry.\n  %s\n",
                ospfExternalRouteInput->inputStrings[i]);

            ERROR_ReportError(errorMessage);
        }
        if (sourceNodeId == node->nodeId)
        {
            in6_addr destAddress;
            BOOL destIsIpAdr = 0;
            NodeId destNodeId = 0;
            in6_addr nextHopAddress;
            BOOL nextHopIsIpAdr = 0;
            NodeId nextHopNodeId = 0;
            Ospfv3RoutingTableRow newRow;
            clocktype delay;
            Message* newMsg = NULL;
            char* msgInfo = NULL;
            BOOL flush = FALSE;
            Ospfv3LinkStateType lsType;
            char currentLine[MAX_STRING_LENGTH];
            unsigned int destPrefixLength = 0;
            unsigned int nextHopPrefixLength = 0;

            Ospfv3ParseNetworkAddress(
                destAddressString,
                &destAddress,
                &destIsIpAdr,
                &destNodeId,
                &destPrefixLength);

            Ospfv3ParseNetworkAddress(
                nextHopString,
                &nextHopAddress,
                &nextHopIsIpAdr,
                &nextHopNodeId,
                &nextHopPrefixLength);

            if (nextHopIsIpAdr != TRUE)
            {
                char errorMessage[MAX_STRING_LENGTH];

                sprintf(errorMessage,
                        "OspfExternalRouting: Next hop "
                        "address can't be a network\n  %s\n",
                    ospfExternalRouteInput->inputStrings[i]);

                ERROR_ReportError(errorMessage);
            }
            if (numParameters == 3)
            {
                cost = 1;
            }

            // Add new route
            newRow.destType = OSPFv3_DESTINATION_NETWORK;

            COPY_ADDR6(destAddress, newRow.destAddr);

            newRow.prefixLength = (unsigned char)destPrefixLength;
            newRow.option = 0;
            newRow.areaId = 0;
            newRow.pathType = OSPFv3_TYPE2_EXTERNAL;
            newRow.metric = 1;
            newRow.type2Metric = cost;
            newRow.LSOrigin = NULL;
            newRow.advertisingRouter = ospf->routerId;

            COPY_ADDR6(nextHopAddress, newRow.nextHop);

            newRow.outIntfId =
                (unsigned int)Ipv6IpGetInterfaceIndexForNextHop(
                                    node,
                                    nextHopAddress);

            if (newRow.outIntfId == ANY_DEST)
            {
                ERROR_Assert(FALSE,
                            "Next Hop for Static route unreachable\n");
            }

            strcpy(currentLine, ospfExternalRouteInput->inputStrings[i]);

            //Ipv6RoutingStaticEntry(node, currentLine);

            Ospfv3AddRoute(node, &newRow);

            newMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        MSG_ROUTING_OspfScheduleLSDB);

            MESSAGE_InfoAlloc(
                node,
                newMsg,
                sizeof(Ospfv3LinkStateType)
                + (sizeof(in6_addr) * 2)
                + sizeof(unsigned char)
                + sizeof(int)
                + sizeof(BOOL));

            lsType = OSPFv3_AS_EXTERNAL;
            flush = FALSE;

            msgInfo = MESSAGE_ReturnInfo(newMsg);

            memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

            msgInfo += sizeof(Ospfv3LinkStateType);

            memcpy(msgInfo, &destAddress, sizeof(in6_addr));

            msgInfo += sizeof(in6_addr);

            memcpy(msgInfo, &newRow.prefixLength, sizeof(unsigned char));

            msgInfo += sizeof(unsigned char);

            memcpy(msgInfo, &nextHopAddress, sizeof(in6_addr));

            msgInfo += sizeof(in6_addr);

            memcpy(msgInfo, &cost, sizeof(int));

            msgInfo += sizeof(int);

            memcpy(msgInfo, &flush, sizeof(BOOL));

            // Schedule for originate AS External LSA
            delay = (clocktype) (OSPFv3_AS_EXTERNAL_LSA_ORIGINATION_DELAY
                                + (RANDOM_nrand(ospf->seed)
                                % OSPFv3_BROADCAST_JITTER));

            MESSAGE_Send(node, newMsg, delay + ospf->staggerStartTime);
        }
    }//for end
}// Function End
// BGP-OSPF Patch End

// /**
// FUNCTION :: Ospfv3GetLSIdFromPrefix.
// LAYER    :: NETWORK.
// PURPOSE  :: Return Link State Id.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + prefix : in6_addr : Ip Address.
// RETURN   :: Link State Id.
// **/
static
unsigned int Ospfv3GetLSIdFromPrefix(Node* node, in6_addr prefix)
{
    Ospfv3Data* ospf = (Ospfv3Data* )
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv3, NETWORK_IPV6);

    ListItem* listItem = ospf->linkStateIdList->first;

    while (listItem)
    {
        Ospfv3LinkStateIdItem* LSIdItem =
            (Ospfv3LinkStateIdItem* )listItem->data;

        if (CMP_ADDR6(prefix, LSIdItem->destPrefix) == 0)
        {
            return LSIdItem->LSId;
        }
        listItem = listItem->next;
    }
    return ANY_DEST;
}

// /**
// FUNCTION :: Ospfv3CreateLSId.
// LAYER    :: NETWORK.
// PURPOSE  :: Create unique Link State.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + prefix : in6_addr : Ip Address Prefix.
// RETURN   :: Link State Id.
// **/
static
unsigned int Ospfv3CreateLSId(Node* node, in6_addr prefix)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int LSId = Ospfv3GetLSIdFromPrefix(node, prefix);

    if (LSId == ANY_DEST)
    {
        Ospfv3LinkStateIdItem* LSIdItem = (Ospfv3LinkStateIdItem* )
            MEM_malloc(sizeof(Ospfv3LinkStateIdItem)) ;

        COPY_ADDR6(prefix, LSIdItem->destPrefix);

        LSIdItem->LSId = ospf->linkStateIdList->size + 1;

        LSIdItem->LSId = LSIdItem->LSId + node->nodeId;

        ListInsert(node, ospf->linkStateIdList, 0, LSIdItem);

        return LSIdItem->LSId;
    }
    else
    {
        return LSId;
    }
}

// /**
// FUNCTION   :: Ospfv3PrintTraceXMLLsaHeader
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA header trace information in XML format
// PARAMETERS ::
// + node     :  Node* : Pointer to node, doing the packet trace
// + lsaHdr   :  Ospfv3LinkStateHeader* : Pointer to Ospfv3LinkStateHeader
// RETURN     :: void : NULL
// **/
void Ospfv3PrintTraceXMLLsaHeader(Node* node,
                                  Ospfv3LinkStateHeader* lsaHdr)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, " <lsaHdr>%hd",
        lsaHdr->linkStateAge);

    TRACE_WriteToBufferXML(node, buf);

    sprintf(buf, "%hu %d %d %d %hd %hd",
        lsaHdr->linkStateType,
        lsaHdr->linkStateId,
        lsaHdr->advertisingRouter,
        lsaHdr->linkStateSequenceNumber,
        lsaHdr->linkStateCheckSum,
        lsaHdr->length);

    TRACE_WriteToBufferXML(node, buf);

    sprintf(buf, "</lsaHdr>");

    TRACE_WriteToBufferXML(node, buf);
}

// /**
// FUNCTION   :: Ospfv3PrintTraceXMLLsa
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA body trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv3LinkStateHeader* : Pointer to Ospfv3LinkStateHeader
// RETURN ::  void : NULL
// **/
static
void Ospfv3PrintTraceXMLLsa(Node* node, Ospfv3LinkStateHeader* lsaHdr)
{
    unsigned int i = 0;
    char buf[MAX_STRING_LENGTH];
    char addr1[MAX_STRING_LENGTH];

    Ospfv3PrintTraceXMLLsaHeader(node, lsaHdr);

    if (lsaHdr->linkStateType == OSPFv3_ROUTER)
    {
        Ospfv3RouterLSA* routerLSA = (Ospfv3RouterLSA* ) lsaHdr;
        Ospfv3LinkInfo* linkList = (Ospfv3LinkInfo* ) (routerLSA + 1);

         unsigned int numLinks = (lsaHdr->length - sizeof(Ospfv3RouterLSA))
                                    / sizeof(Ospfv3LinkInfo);

        sprintf(buf, " <routerLsaBody>");

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " %hu %hu %hu %hu %hu ",
            (routerLSA->bitsAndOptions & OSPFv3_W_BIT) ? 1 : 0,
            (routerLSA->bitsAndOptions & OSPFv3_V_BIT) ? 1 : 0,
            (routerLSA->bitsAndOptions & OSPFv3_E_BIT) ? 1 : 0,
            (routerLSA->bitsAndOptions & OSPFv3_B_BIT) ? 1 : 0,
            routerLSA->bitsAndOptions & 0x00ffffff);

        TRACE_WriteToBufferXML(node, buf);

        if (numLinks) {
        sprintf(buf, " <linkInfoList>");

        TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < numLinks; i++)
        {
            sprintf(buf, " <linkInfo>%hu %hu %hu %hu %hd %hu</linkInfo>",
                linkList->type,
                linkList->reserved,
                linkList->metric,
                linkList->interfaceId,
                linkList->neighborInterfaceId,
                linkList->neighborRouterId);

            TRACE_WriteToBufferXML(node, buf);

            linkList = linkList + 1;
        }

            sprintf(buf, "</linkInfoList>");
            TRACE_WriteToBufferXML(node, buf);
        }
        sprintf(buf, "</routerLsaBody>");
        TRACE_WriteToBufferXML(node, buf);

    }
    else if (lsaHdr->linkStateType == OSPFv3_NETWORK)
    {
        Ospfv3NetworkLSA* networkLSA = (Ospfv3NetworkLSA* ) lsaHdr;
        unsigned int* routerId = ((unsigned int* ) (networkLSA + 1)) + 1;

        unsigned int numLink = (networkLSA->LSHeader.length
                                - sizeof(Ospfv3NetworkLSA)
                                - sizeof(unsigned int))
                                / (sizeof(unsigned int));

        sprintf(buf, " <networkLsaBody>%hu %hu",
            0,
            networkLSA->bitsAndOptions);

        TRACE_WriteToBufferXML(node, buf);

        if (numLink) {
        for (i = 0; i < numLink; i++)
        {
            IO_ConvertIpAddressToString(routerId[i], addr1);
                sprintf(buf, "<routerList>%hu</routerList>", routerId[i]);
            TRACE_WriteToBufferXML(node, buf);
        }
        }
        sprintf(buf, "</networkLsaBody>");
        TRACE_WriteToBufferXML(node, buf);
    }
    else if (lsaHdr->linkStateType == OSPFv3_INTER_AREA_PREFIX)
    {

        Ospfv3InterAreaPrefixLSA* LSA = (Ospfv3InterAreaPrefixLSA* ) lsaHdr;

        IO_ConvertIpAddressToString(&LSA->addrPrefix, addr1);

        sprintf(buf,
                " <InterAreaPrefixLsaBody>%d %d %d %d %d %s"
                "</InterAreaPrefixLsaBody>",
            0,
            LSA->bitsAndMetric,
            LSA->prefixLength,
            LSA->prefixOptions,
            LSA->reserved,
            addr1);

        TRACE_WriteToBufferXML(node, buf);

    }
    else if (lsaHdr->linkStateType == OSPFv3_INTER_AREA_ROUTER)
    {

        Ospfv3InterAreaRouterLSA* LSA = (Ospfv3InterAreaRouterLSA* ) lsaHdr;

        sprintf(buf,
                " <InterAreaPrefixLsaBody>%d %d %d %d %d "
                "</InterAreaPrefixLsaBody>",
            0,
            LSA->bitsAndOptions,
            0,
            LSA->bitsAndPathMetric,
            LSA->destinationRouterId);

        TRACE_WriteToBufferXML(node, buf);

    }
    else if (lsaHdr->linkStateType == OSPFv3_AS_EXTERNAL)
    {
        Ospfv3ASexternalLSA* LSA = (Ospfv3ASexternalLSA* ) lsaHdr ;

        IO_ConvertIpAddressToString(&LSA->addrPrefix, addr1);

        sprintf(buf,
                " <asexternalLsaBody>%hu %hu %hu %hu %hu %hu %hu %s"
                "</asexternalLsaBody>",
            (LSA->OptionsAndMetric & OSPFv3_EXTERNAL_LSA_E_BIT) ? 1 : 0,
            (LSA->OptionsAndMetric & OSPFv3_F_BIT) ? 1 : 0,
            (LSA->OptionsAndMetric & OSPFv3_T_BIT) ? 1 : 0,
            LSA->OptionsAndMetric & 0x00ffffff,
            LSA->prefixLength,
            LSA->prefixOptions,
            LSA->referencedLSType,
            addr1);

        TRACE_WriteToBufferXML(node, buf);
    }
    else if (lsaHdr->linkStateType == OSPFv3_LINK)
    {
        Ospfv3LinkLSA* LSA = (Ospfv3LinkLSA* ) lsaHdr ;
        Ospfv3PrefixInfo* prefixInfo = (Ospfv3PrefixInfo* ) (LSA + 1);

        IO_ConvertIpAddressToString(&LSA->linkLocalAddress, addr1);

        sprintf(buf, "<externalLsaBody>%hu %hu %s %hu",
            (LSA->priorityAndOptions & 0xff000000) >> 24,
            LSA->priorityAndOptions & 0x00ffffff,
            addr1,
            LSA->numPrefixes);

        TRACE_WriteToBufferXML(node, buf);
        if (LSA->numPrefixes) {

            sprintf(buf, "<prefixInfoList>");
            TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < LSA->numPrefixes; i++)
        {
            IO_ConvertIpAddressToString(&prefixInfo->prefixAddr, addr1);

                sprintf(buf, "<prefixInfo>%hu %hu %hu %s</prefixInfo>",
                prefixInfo->prefixLength,
                prefixInfo->prefixOptions,
                prefixInfo->reservedOrMetric,
                addr1);

            TRACE_WriteToBufferXML(node, buf);

            prefixInfo += 1;
        }
            sprintf(buf, "</prefixInfoList></externalLsaBody>");
            TRACE_WriteToBufferXML(node, buf);
        }
    }
    else if (lsaHdr->linkStateType == OSPFv3_INTRA_AREA_PREFIX)
    {
        Ospfv3IntraAreaPrefixLSA* LSA =
                        (Ospfv3IntraAreaPrefixLSA* ) lsaHdr ;
        Ospfv3PrefixInfo* prefixInfo = (Ospfv3PrefixInfo* ) (LSA + 1);

        sprintf(buf, " <ExternalLsaBody>%u %u %u %u ",
            LSA->numPrefixes,
            LSA->referencedLSType,
            LSA->referencedLSId,
            LSA->referencedAdvertisingRouter);

        TRACE_WriteToBufferXML(node, buf);

        if (LSA->numPrefixes) {
        sprintf(buf, " <prefixInfoList>");

        TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < LSA->numPrefixes; i++)
        {
            IO_ConvertIpAddressToString(&prefixInfo->prefixAddr, addr1);

            sprintf(buf, "<prefixInfo> %hu %hu %hu %s </prefixInfo>",

                prefixInfo->prefixLength,
                prefixInfo->prefixOptions,
                prefixInfo->reservedOrMetric,
                addr1);

            TRACE_WriteToBufferXML(node, buf);

            prefixInfo += 1;
        }
            }
        sprintf(buf, "</prefixInfoList></ExternalLsaBody>");

        TRACE_WriteToBufferXML(node, buf);
    }
}

// /**
// FUNCTION   :: Ospfv3PrintTrace
// LAYER      :: NETWORK
// PURPOSE    :: Print out packet trace information in XML format.
// PARAMETERS ::
// + node     :  Node*                       : Pointer to node, doing the
//                                             packet trace
// + msg      :  Message*                    : Message representing a packet
//                                             event for the protocol.
// RETURN ::  void : NULL
// **/
void Ospfv3PrintTraceXML(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char addr1[MAX_STRING_LENGTH];

    Ospfv3CommonHeader* header =
        (Ospfv3CommonHeader* ) MESSAGE_ReturnPacket(msg);

    sprintf(buf, "<ospfv3>");

    TRACE_WriteToBufferXML(node, buf);

    IO_ConvertIpAddressToString(header->areaId, addr1);

    sprintf(buf, "<commonHdr>%hu %hu %hd %d %s %hd %hd %d</commonHdr>",
        header->version,
        header->packetType,
        header->packetLength,
        header->routerId,
        addr1,
        header->checkSum,
        header->instanceId,
        0);

    TRACE_WriteToBufferXML(node, buf);

    if (header->packetType == OSPFv3_HELLO)
    {
        int i = 0;
        Ospfv3HelloPacket* helloPkt =
            (Ospfv3HelloPacket* ) MESSAGE_ReturnPacket(msg);

        unsigned int* tempNeighbor = (NodeAddress* ) (helloPkt + 1);
        int numNeighbor = (helloPkt->commonHeader.packetLength
                            - sizeof(Ospfv3HelloPacket))
                            / sizeof(unsigned int);

        sprintf(buf, " <helloPkt>");

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, "%hd %d",
            helloPkt->interfaceId,
            OSPFV3_GetHelloPacketPriorityFeild(
                helloPkt->priorityAndOptions));

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " <helloOptions>%hu %hu %hu %hu %hu %hu</helloOptions>",
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_DC_BIT) ? 1 : 0,
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_R_BIT) ? 1 : 0,
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_N_BIT) ? 1 : 0,
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_MC_BIT) ? 1 : 0,
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_E_BIT) ? 1 : 0,
            (helloPkt->priorityAndOptions & OSPFv3_OPTIONS_V6_BIT) ? 1 : 0);

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " %hu %d %d %d",
            helloPkt->helloInterval,
            helloPkt->routerDeadInterval,
            helloPkt->designatedRouter,
            helloPkt->backupDesignatedRouter);

        TRACE_WriteToBufferXML(node, buf);

        if (numNeighbor)
        {
            sprintf(buf, " <neighborList>");

            TRACE_WriteToBufferXML(node, buf);

            for (i = 0; i < numNeighbor; i++)
            {
                sprintf(buf, "%d ", tempNeighbor[i]);

                TRACE_WriteToBufferXML(node, buf);
            }

            sprintf(buf, "</neighborList>");

            TRACE_WriteToBufferXML(node, buf);
        }

        sprintf(buf, "</helloPkt>");

        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv3_DATABASE_DESCRIPTION)
    {

        Ospfv3DatabaseDescriptionPacket* dbDscrpPkt =
            (Ospfv3DatabaseDescriptionPacket* ) MESSAGE_ReturnPacket(msg);

        Ospfv3LinkStateHeader* lsaHdr =
            (Ospfv3LinkStateHeader* ) (dbDscrpPkt + 1) ;

        int size = dbDscrpPkt->commonHeader.packetLength;

        sprintf(buf, " <PktOptions>%u %u %u %u %u %u</PktOptions>",
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_DC_BIT) ? 1 : 0,
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_R_BIT) ? 1 : 0,
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_N_BIT) ? 1 : 0,
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_MC_BIT) ? 1 : 0,
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_E_BIT) ? 1 : 0,
            (dbDscrpPkt->bitsAndOptions & OSPFv3_OPTIONS_V6_BIT) ? 1 : 0);

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, "<ddPkt>%hu",
            (dbDscrpPkt->mtuAndBits & 0xffff0000) >> 16);

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " <ddOptions>%hu %hu %hu</ddOptions>",
            dbDscrpPkt->mtuAndBits & OSPFv3_MS_BIT,
            dbDscrpPkt->mtuAndBits & OSPFv3_M_BIT,
            dbDscrpPkt->mtuAndBits & OSPFv3_I_BIT);

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " %hu", dbDscrpPkt->ddSequenceNumber);

        TRACE_WriteToBufferXML(node, buf);

        if (size -= sizeof(Ospfv3DatabaseDescriptionPacket))
        {
            sprintf(buf, " <lsaHdrList>");

            TRACE_WriteToBufferXML(node, buf);

            for (; size > 0; size -= sizeof(Ospfv3LinkStateHeader))
            {
                Ospfv3PrintTraceXMLLsaHeader(node, lsaHdr);

                lsaHdr += 1;
            }

            sprintf(buf, "</lsaHdrList>");

            TRACE_WriteToBufferXML(node, buf);
        }

        sprintf(buf, "</ddPkt>");

        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv3_LINK_STATE_REQUEST)
    {
        Ospfv3LinkStateRequestPacket* reqPkt =
            (Ospfv3LinkStateRequestPacket* ) MESSAGE_ReturnPacket(msg);

        int numLSReqObject = (reqPkt->commonHeader.packetLength
                             - sizeof(Ospfv3LinkStateRequestPacket))
                             / sizeof(Ospfv3LSRequestInfo);

        Ospfv3LSRequestInfo* lsaReqObject =
            (Ospfv3LSRequestInfo* ) (reqPkt + 1);

        int i = 0;

        sprintf(buf, "<requestPkt>");

        TRACE_WriteToBufferXML(node, buf);

        for (i = 0; i < numLSReqObject; i++)
        {
            sprintf(buf, " <reqInfo>%u %u %d %d</reqInfo>",
                lsaReqObject->reserved,
                lsaReqObject->linkStateType,
                lsaReqObject->linkStateId,
                lsaReqObject->advertisingRouter);

            TRACE_WriteToBufferXML(node, buf);
            lsaReqObject++;
        }

        sprintf(buf, "</requestPkt>");

        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv3_LINK_STATE_UPDATE)
    {
        int count = 0;
        Ospfv3LinkStateUpdatePacket* updatePkt =
            (Ospfv3LinkStateUpdatePacket* ) MESSAGE_ReturnPacket(msg);

        Ospfv3LinkStateHeader* LSHeader =
            (Ospfv3LinkStateHeader* ) (updatePkt + 1);

        sprintf(buf, "<updatePkt>%d", updatePkt->numLSA);

        TRACE_WriteToBufferXML(node, buf);

        sprintf(buf, " <lsaList>");

        TRACE_WriteToBufferXML(node, buf);

        for (count = 0; count < updatePkt->numLSA; count++,
            LSHeader = (Ospfv3LinkStateHeader* )
                (((char* ) LSHeader) + LSHeader->length))
        {
            Ospfv3PrintTraceXMLLsa(node, LSHeader);
        }

        sprintf(buf, "</lsaList></updatePkt>");

        TRACE_WriteToBufferXML(node, buf);
    }
    else if (header->packetType == OSPFv3_LINK_STATE_ACK)
    {
        int count = 0;

        Ospfv3LinkStateAckPacket* ackPkt =
            (Ospfv3LinkStateAckPacket* ) MESSAGE_ReturnPacket(msg);

        Ospfv3LinkStateHeader* LSHeader =
            (Ospfv3LinkStateHeader* ) (ackPkt + 1);

        int numAck = (ackPkt->commonHeader.packetLength
                        - sizeof(Ospfv3LinkStateAckPacket))
                        / sizeof(Ospfv3LinkStateHeader);

        sprintf(buf, "<ackPkt>");

        TRACE_WriteToBufferXML(node, buf);

        for (count = 0; count < numAck; count++)
        {
            Ospfv3PrintTraceXMLLsaHeader(node, LSHeader);

            LSHeader = LSHeader + 1;
        }

        sprintf(buf, "</ackPkt>");

        TRACE_WriteToBufferXML(node, buf);
    }

    sprintf(buf, "</ospfv3>");

    TRACE_WriteToBufferXML(node, buf);
}

// /**
// FUNCTION   :: Ospfv3InitTrace
// LAYER      :: NETWORK
// PURPOSE    :: Initialize OSPFv3 protocol Trace.
// PARAMETERS ::
// +node:  Node* : Pointer to node, doing the packet trace
// +nodeInput :  const NodeInput* : Pointer to chached config file.
// RETURN ::  void : NULL
// **/
static
void Ospfv3InitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    static BOOL writeMap = TRUE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-OSPFv3",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-OSPFv3 should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll || node->traceData->layer[TRACE_NETWORK_LAYER])
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
            TRACE_EnableTraceXML(
                node,
                TRACE_OSPFv3,
                "OSPFv3",
                Ospfv3PrintTraceXML, writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(
                node,
                TRACE_OSPFv3,
                "OSPFv3",
                writeMap);
    }
    writeMap = FALSE;
}

// /**
// FUNCTION   :: Ospfv3Init
// LAYER      :: NETWORK
// PURPOSE    :: Initialize OSPFv3 routing protocol. This function is called
//               from IPv6Init of ipv6.cpp.
// PARAMETERS ::
// + node            :  Node*                  : Pointer to node, used for
//                                               debugging.
// + ospfLayerPtr    :  Ospfv3Data**           : Pointer to Pointer of OSPFv3
//                                               Protocol Data Structure.
// + nodeInput       :  const NodeInput*       : Pointer to the configuration
//                                               file
// + interfaceIndex  :  interfaceIndex         : Node's interface Index.
// RETURN     :: void : NULL
// **/
void Ospfv3Init(
    Node* node,
    Ospfv3Data**ospfLayerPtr,
    const NodeInput* nodeInput,
    int interfaceId)
{
    Message* newMsg;
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    NodeInput ospfConfigFile;
    NodeInput* inputFile;
    clocktype delay;

    Ospfv3Data* ospf = (Ospfv3Data* ) MEM_malloc (sizeof (Ospfv3Data));

    // BGP-OSPF Patch Start
    int asVal = 0;
    // BGP-OSPF Patch End

    memset(ospf, 0, sizeof (Ospfv3Data));

    RANDOM_SetSeed(ospf->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_OSPFv3,
                   interfaceId);

    (*ospfLayerPtr) = ospf;

    Ospfv3InitTrace(node, nodeInput);

    // BGP-OSPF Patch Start
    // Read the Node's Autonomous System id.
    in6_addr interfaceAddr;
    Ipv6GetGlobalAggrAddress(
         node,
         interfaceId,
         &interfaceAddr);
    IO_ReadInt(
        node->nodeId,
        &interfaceAddr,
        nodeInput,
        "AS-NUMBER",
        &retVal,
        &asVal);

    if (retVal)
    {
        if (asVal > OSPFv3_MAX_AS_NUMBER || asVal < OSPFv3_MIN_AS_NUMBER)
        {
            ERROR_ReportError("Autonomous System id must be less than "
                            "equal to 65535 and greater than 0");
        }
        ospf->asId = asVal;
    }

    // Determine whether the node is AS BOUNDARY ROUTER or not.
    // Format is: [node] AS-BOUNDARY-ROUTER <YES/NO>
    // If not specified, node is not a AS BOUNDARY ROUTER.
    IO_ReadString(node->nodeId,
            &interfaceAddr,
            nodeInput,
            "AS-BOUNDARY-ROUTER",
            &retVal,
            buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->asBoundaryRouter = FALSE;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->asBoundaryRouter = TRUE;
    }
    else
    {
        ERROR_ReportError(
            "AS-BOUNDARY-ROUTER: Unknown value in configuration file.\n");
    }

    ListInit(node, &ospf->asExternalLSAList);
    // BGP-OSPF Patch End

    // Determine whether the domain is partitioned into several areas.
    IO_ReadString(
        node->nodeId,
        &interfaceAddr,
        nodeInput,
        "OSPFv3-DEFINE-AREA",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->partitionedIntoArea = FALSE;

        ListInit(node, &ospf->area);

        ospf->backboneArea = NULL;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->partitionedIntoArea = TRUE;

        ListInit(node, &ospf->area);

        ospf->backboneArea = NULL;
    }
    else
    {
        ERROR_ReportError(
            "OSPFv3-DEFINE-AREA: Unknown value in configuration file.\n");
    }

     Address ipv6Addr;
     ipv6Addr.networkType = NETWORK_IPV6;
     COPY_ADDR6(
         interfaceAddr,
          ipv6Addr.interfaceAddr.ipv6);

    // Read config file
    IO_ReadCachedFile(
        node->nodeId,
        &ipv6Addr,
        nodeInput,
        "OSPFv3-CONFIG-FILE",
        &retVal,
        &ospfConfigFile);

    if (!retVal)
    {
        if (ospf->partitionedIntoArea)
        {
            ERROR_ReportError("The OSPFv3-CONFIG-FILE file is required "
                            "to configure area\n");
        }

        inputFile = NULL;
    }
    else
    {
        inputFile = &ospfConfigFile;
    }

    // If we need to desynchronize routers start up time.
    IO_ReadString(
        node->nodeId,
        &interfaceAddr,
        nodeInput,
        "OSPFv3-STAGGER-START",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->staggerStartTime = (clocktype) 0;
    }
    else if (!strcmp (buf, "YES"))
    {
        Node* pNode = NULL;
        clocktype maxDelay = 0;
        unsigned int numNode = 0;

        for (pNode = node; pNode; pNode = pNode->nextNodeData)
        {
            numNode++;
        }

        for (pNode = node->prevNodeData; pNode; pNode = pNode->prevNodeData)
        {
            numNode++;
        }

        maxDelay = numNode * SECOND;

        ospf->staggerStartTime =
            (clocktype) (RANDOM_erand(ospf->seed) * maxDelay);
    }
    else
    {
        ERROR_ReportError(
            "OSPFv3-STAGGER-START: Unknown value in configuration file.\n");
    }

    Ospfv3InitRoutingTable(node);

    ospf->neighborCount = 0;
    ospf->SPFTimer = (clocktype) 0;

    // Router Id is chosen to be node Id
    ospf->routerId = node->nodeId;

    Ospfv3InitInterface(node, inputFile, nodeInput);

    // Schedule for sending Hello packets
    delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleHello);
    // fix start for bug 1189
    MESSAGE_SetInstanceId(newMsg, interfaceId);
    // fix end for bug 1189

    MESSAGE_Send(node, newMsg, delay + ospf->staggerStartTime);

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfIncrementLSAge);

    MESSAGE_Send(
        node,
        newMsg,
        (clocktype) OSPFv3_LSA_INCREMENT_AGE_INTERVAL
        + ospf->staggerStartTime);

    Ipv6SetRouterFunction(
        node,
        &Ospfv3RouterFunction,
        interfaceId);

    // generate LS id for inter area prefix LSA for network mapping.
    ListInit(node, &ospf->linkStateIdList);

    // BGP-OSPF Patch Start
    if (ospf->asBoundaryRouter)
    {
        NodeInput ospfExternalRouteInput;

        // Determine whether the user inject External Route or not.
        // Format is:[node] OSPF-INJECT-ROUTE
        IO_ReadString(
            node->nodeId,
            &interfaceAddr,
            nodeInput,
            "OSPFv3-INJECT-EXTERNAL-ROUTE",
            &retVal,
            buf);

        if (retVal)
        {
            if (strcmp(buf, "YES") == 0)
            {
                IO_ReadCachedFile(
                    node->nodeId,
                    &ipv6Addr,
                    nodeInput,
                    "OSPFv3-INJECT-ROUTE-FILE",
                    &retVal,
                    &ospfExternalRouteInput);

                Ospfv3ExternalRouteInit(node, &ospfExternalRouteInput);

                if (!retVal)
                {
                    ERROR_ReportError("The OSPFv3-INJECT-ROUTE-FILE "
                                      "file is not specified\n");
                }
            }
        }
    }
    // BGP-OSPF Patch End

    // Determine whether or not to print the stats at the end of simulation.
    IO_ReadString(
        node->nodeId,
        &interfaceAddr,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if (!retVal || !strcmp (buf, "NO"))
    {
        ospf->collectStat = FALSE;
    }
    else if (!strcmp (buf, "YES"))
    {
        ospf->collectStat = TRUE;
    }
    else
    {
        ERROR_ReportError(
            "ROUTING-STATISTICS: Unknown value in configuration file.\n");
    }

    // Used to print statistics only once during finalization.  In
    // the future, stats should be tied to each interface.
    ospf->stats.alreadyPrinted = FALSE;

     // registering RoutingOSPFv3HandleAddressChangeEvent function
    Ipv6AddPrefixChangedHandlerFunction(node,
                            &RoutingOSPFv3HandleAddressChangeEvent);
}// Function End


// ********************** Event Processing*********************************//
// /**
// FUNCTION   :: Ospfv3GetArea
// LAYER      :: NETWORK
// PURPOSE    :: Get area structure pointer by area Id.
// PARAMETERS ::
//  +node:  Node* : Pointer to node, used for debugging.
//  +areaId: unsigned int : Area Id.
// RETURN     :: Pointer to area if found, else NULL.
// **/
Ospfv3Area* Ospfv3GetArea(Node* node, unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv3Area* areaInfo = (Ospfv3Area* ) listItem->data;

        if (areaInfo->areaId == areaId)
        {
            return areaInfo;
        }
    }

    return NULL;
}

// /**
// FUNCTION   :: Ospfv3FillNeighborField
// LAYER      :: NETWORK
// PURPOSE    :: Fill the neighbors to be sent in hello packet.
// PARAMETERS ::
//  +node:  Node* : Pointer to node, used for debugging.
//  +nbrList: unsigned int** : Pointer to array Neighbor Id.
//  +areaId: unsigned int : Area Id.
// RETURN     :: Count of Neighbor added in the list.
// **/
static
int Ospfv3FillNeighborField(
    Node* node,
    unsigned int** nbrList,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* tmpListItem = NULL;
    Ospfv3Neighbor* tmpNbInfo = NULL;
    Ospfv3Interface* thisIntf = NULL;
    int count = 0;
    int neighborCount = 0;
    thisIntf = &ospf->pInterface[interfaceId];
    neighborCount = thisIntf->neighborList->size;

    if (neighborCount == 0)
    {
        return count;
    }
    *nbrList =
        (unsigned int* ) MEM_malloc(sizeof(unsigned int) * neighborCount);

    tmpListItem = thisIntf->neighborList->first;

    // Get a list of all my neighbors from this interface.
    while (tmpListItem)
    {
        tmpNbInfo = (Ospfv3Neighbor* ) tmpListItem->data;

        // Consider only neighbors from which Hello packet
        // have been seen recently
        if (tmpNbInfo->state >= S_Init)
        {
            // Add neighbor to the list.
            (*nbrList)[count] = tmpNbInfo->neighborId;

#ifdef OSPFv3_DEBUG_HELLOErr
            {
                printf("   Adding Nbr to Hello Packet");
            }
#endif

            count++;
        }
        tmpListItem = tmpListItem->next;
    }
    return count;
}

// /**
// FUNCTION   :: Ospfv3FillCommonHeader
// LAYER      :: NETWORK
// PURPOSE    :: Fill the common packet header.
// PARAMETERS ::
//  +node:  Node* : Pointer to node, used for debugging.
//  +commonHdr: Ospfv3CommonHeader* : Pointer to Packet Header.
//  +areaId: unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FillCommonHeader(
    Node* node,
    Ospfv3CommonHeader* commonHdr,
    Ospfv3PacketType pktType)
{
    Ospfv3Data* ospf =
        (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_OSPFv3,
                            NETWORK_IPV6);

    commonHdr->version = OSPFv3_CURRENT_VERSION;
    commonHdr->packetType = (unsigned char) pktType;
    commonHdr->routerId = ospf->routerId;
    commonHdr->areaId = 0x0;

    // Checksum not used in simulation.
    commonHdr->checkSum = 0x0;

    // Multiple OSPFv3 instance not implemented.
    commonHdr->instanceId = 0x0;
    commonHdr->reserved = 0x0;
}

// /**
// FUNCTION   :: Ospfv3GetAllSPFRoutersMulticastAddress
// LAYER      :: NETWORK
// PURPOSE    :: Get the AllSPFRouters multicast group.
// PARAMETERS ::
//  +multicastAddress:  in6_addr* : Pointer to Ip Address.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3GetAllSPFRoutersMulticastAddress(in6_addr* multicastAddress)
{
    //This Multicast group address is FF02::5.
    multicastAddress->s6_addr8[0] = 0xff;
    multicastAddress->s6_addr8[1] = 0x02;
    multicastAddress->s6_addr8[2] = 0x00;
    multicastAddress->s6_addr8[3] = 0x00;
    multicastAddress->s6_addr32[1] = 0;
    multicastAddress->s6_addr32[2] = 0;
    multicastAddress->s6_addr8[12] = 0x00;
    multicastAddress->s6_addr8[13] = 0x00;
    multicastAddress->s6_addr8[14] = 0x00;
    multicastAddress->s6_addr8[15] = 0x05;
}

// /**
// FUNCTION   :: Ospfv3GetAllDRoutersMulticastAddress
// LAYER      :: NETWORK
// PURPOSE    :: Get the AllDRouters multicast group.
// PARAMETERS ::
//  +multicastAddress:  in6_addr* : Pointer to Ip Address.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3GetAllDRoutersMulticastAddress(in6_addr* multicastAddress)
{
    //This Multicast group address is FF02::6.
    multicastAddress->s6_addr8[0] = 0xff;
    multicastAddress->s6_addr8[1] = 0x02;
    multicastAddress->s6_addr8[2] = 0x00;
    multicastAddress->s6_addr8[3] = 0x00;
    multicastAddress->s6_addr32[1] = 0;
    multicastAddress->s6_addr32[2] = 0;
    multicastAddress->s6_addr8[12] = 0x00;
    multicastAddress->s6_addr8[13] = 0x00;
    multicastAddress->s6_addr8[14] = 0x00;
    multicastAddress->s6_addr8[15] = 0x06;
}

// /**
// FUNCTION   :: Ospfv3GetNeighborInfoByRtrId
// LAYER      :: NETWORK
// PURPOSE    :: Get the neighbor structure from the neighbor list using
//               the neighbor's IP address.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +neighborId:  unsigned int : Neighbor Id.
// RETURN     :: Neighbor Info if found, NULL otherwise.
// **/
static
Ospfv3Neighbor* Ospfv3GetNeighborInfoByRtrId(
    Node* node,
    unsigned int neighborId)
{
    int i = 0;
    ListItem* tempListItem = NULL;
    Ospfv3Neighbor* tempNeighborInfo = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }
        tempListItem = ospf->pInterface[i].neighborList->first;

        // Search for the neighbor in my neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

            ERROR_Assert(tempNeighborInfo,
                        "Neighbor not found in the Neighbor list");

            if (tempNeighborInfo->neighborId == neighborId)
            {
                // Found the neighbor we are looking for.
                return tempNeighborInfo;
            }

            tempListItem = tempListItem->next;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: Ospfv3GetNeighborInfoByRtrId
// LAYER      :: NETWORK
// PURPOSE    :: This function is used to schedule Inactivity Timer and
//               Retransmission Timer.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +eventType:  int : Timer Event Type.
//  +neighborId:  unsigned int : Neighbor Id.
//  +timerDelay:  clocktype : Delay for timer scheduling.
//  +sequenceNumber:  unsigned int : Sequence Number of Timer.
//  +advertisingRouter:  unsigned int : Advertiser Router Id.
//  +type:  Ospfv3PacketType : Packet type.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SetTimer(
    Node* node,
    int eventType,
    unsigned int neighborId,
    clocktype timerDelay,
    unsigned int sequenceNumber,
    unsigned int advertisingRouter,
    Ospfv3PacketType type)
{
    Ospfv3TimerInfo* info = NULL;

    Message* newMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        eventType);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(Ospfv3TimerInfo));

    info = (Ospfv3TimerInfo* ) MESSAGE_ReturnInfo(newMsg);

    info->neighborId = neighborId;

#ifdef OSPFv3_DEBUG_ERRORS
    {
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        printf("    Node %d setting timer at time %s\n",
            node->nodeId,
            clockStr);

        printf("        type is ");
    }
#endif

    switch (eventType)
    {
        case MSG_ROUTING_OspfInactivityTimer:
        {
            Ospfv3Neighbor* neighborInfo =
                Ospfv3GetNeighborInfoByRtrId(node, neighborId);

            ERROR_Assert(neighborInfo, "Neighbor Info not found");

            info->sequenceNumber =
                ++neighborInfo->inactivityTimerSequenceNumber;

#ifdef OSPFv3_DEBUG_ERRORS
            {
                printf("InactivityTimer with seqno %u\n",
                    info->sequenceNumber);
            }
#endif

            break;
        }
        case MSG_ROUTING_OspfRxmtTimer:
        {
            info->sequenceNumber = sequenceNumber;
            info->advertisingRouter = advertisingRouter;
            info->type = type;

#ifdef OSPFv3_DEBUG_ERRORS
            {
                printf("Node %d Setting RxmtTimer with seqno %u"
                        " for neighbor %d\n",
                    node->nodeId,
                    info->sequenceNumber,
                    neighborId);
            }
#endif

            break;
        }
        default:
        {
            //Shouldn't get here.
            ERROR_Assert(FALSE, "Invalid Timer EventType\n");
        }
    }

#ifdef OSPFv3_DEBUG_ERRORS
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime() + timerDelay, clockStr);

        printf("        sequence number is %d\n", info->sequenceNumber);

        printf("        to expired at %s\n", clockStr);
    }
#endif

    MESSAGE_Send(node, newMsg, timerDelay);
}

// /**
// FUNCTION   :: Ospfv3SendHelloPacket
// LAYER      :: NETWORK
// PURPOSE    :: Broadcast Hello Packet on all OSPFv3 interfaces.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SendHelloPacket(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* )
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv3, NETWORK_IPV6);

    int numNeighbor;
    int i;
    unsigned int* neighborList = NULL;
    Message* helloMsg = NULL;
    Ospfv3HelloPacket* helloPacket = NULL;
    in6_addr srcAddr;
    in6_addr destAddr;

    Ospfv3GetAllSPFRoutersMulticastAddress(&destAddr);

    // Send Hello packet out to each interface.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, i, NETWORK_IPV6)
            == ROUTING_PROTOCOL_OSPFv3)
        {
            Ospfv3Area* thisArea = Ospfv3GetArea(
                                        node,
                                        ospf->pInterface[i].areaId);

            ERROR_Assert(thisArea, "Area not identified\n");

            // Get all my neighbors that I've learned.
            numNeighbor = Ospfv3FillNeighborField(
                            node,
                            &neighborList,
                            i);

            // Now, create the Hello packet.
            helloMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            ROUTING_PROTOCOL_OSPFv3,
                            MSG_ROUTING_OspfPacket);

            MESSAGE_PacketAlloc(
                node,
                helloMsg,
                sizeof(Ospfv3HelloPacket)
                + (sizeof(NodeAddress) * numNeighbor),
                TRACE_OSPFv3);

            helloPacket =
                (Ospfv3HelloPacket* ) MESSAGE_ReturnPacket(helloMsg);

            memset(helloPacket, 0, sizeof(Ospfv3HelloPacket));

            // Fill in the header fields for the Hello packet.
            Ospfv3FillCommonHeader(
                node,
                &helloPacket->commonHeader,
                OSPFv3_HELLO);

            // Keep track of the Hello packet length so my neighbors can
            // determine how many neighbors are in the Hello packet.
            helloPacket->commonHeader.packetLength =
                (short) (sizeof(Ospfv3HelloPacket)
                        + (sizeof(unsigned int) *  numNeighbor));

            // Add our neighbor list at the end of the Hello packet.
            if (numNeighbor !=0)
            {
                memcpy(
                    helloPacket + 1,
                    neighborList,
                    sizeof(unsigned int) * numNeighbor);
            }


#ifdef OSPFv3_DEBUG_HELLOErr
            {
                printf("\nNode %d Printing Hello Packet Before Sending\n",
                    node->nodeId);

                printf("   Hello packet length         = %d\n",
                    helloPacket->commonHeader.packetLength);

                Ospfv3PrintHelloPacket(node, helloPacket);
            }
#endif

            // Fill area Id field of header
            helloPacket->commonHeader.areaId = ospf->pInterface[i].areaId;

            //Initilize bits and options to 0.
            helloPacket->priorityAndOptions = 0;

            // Set E-bit if not a stub area
            if (thisArea->transitCapability == TRUE)
            {
                helloPacket->priorityAndOptions |= OSPFv3_OPTIONS_E_BIT;
            }

            helloPacket->interfaceId = (ospf->pInterface[i].interfaceId);

            helloPacket->helloInterval =
                (unsigned short) (ospf->pInterface[i].helloInterval
                                    / SECOND);

            OSPFv3_SetPriorityFeild(
                &helloPacket->priorityAndOptions,
                ospf->pInterface[i].routerPriority);

            helloPacket->routerDeadInterval =
                (unsigned short) (ospf->pInterface[i].routerDeadInterval
                                    / SECOND);

            // Designated router and backup Designated router is identified
            // by it's IP interface address on the network
            helloPacket->designatedRouter =
                ospf->pInterface[i].designatedRouter;

            helloPacket->backupDesignatedRouter =
                ospf->pInterface[i].backupDesignatedRouter;

            //Trace sending pkt
            ActionData acnData;
            acnData.actionType = SEND;
            acnData.actionComment = NO_COMMENT;

            TRACE_PrintTrace(
                node,
                helloMsg,
                TRACE_NETWORK_LAYER,
                PACKET_OUT,
                &acnData);

            Ipv6GetGlobalAggrAddress(node, i, &srcAddr);

            Ipv6SendRawMessage(
                node,
                helloMsg,
                srcAddr,
                destAddr,
                i,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_OSPF,
                1);

            if (numNeighbor !=0)
            {
                MEM_free(neighborList);
            }

            ospf->stats.numHelloSent++;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3CreateCommonHeader
// LAYER      :: NETWORK
// PURPOSE    :: Create Commom Packet header.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +commonHdr:  Ospfv3CommonHeader* : Pointer to common packet header.
//  +pktType:  Ospfv3PacketType : Packet Type.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3CreateCommonHeader(
    Node* node,
    Ospfv3CommonHeader* commonHdr,
    Ospfv3PacketType pktType)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    commonHdr->version = OSPFv3_CURRENT_VERSION;
    commonHdr->packetType = (unsigned char) pktType;
    commonHdr->routerId = ospf->routerId;
    commonHdr->areaId = 0x0;
    commonHdr->checkSum = 0;
    commonHdr->instanceId = 0;
    commonHdr->reserved = 0;
}

// /**
// FUNCTION   :: Ospfv3GetTopLSAFromList
// LAYER      :: NETWORK
// PURPOSE    :: Create Commom Packet header.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +list:  LinkedList* : Pointer to list.
//  +LSHeader:  Ospfv3LinkStateHeader* : Pointer to  LS header.
// RETURN     :: TRUE if top of LSA found, FALSE otherwise.
// **/
static
BOOL Ospfv3GetTopLSAFromList(
    Node* node,
    LinkedList* list,
    Ospfv3LinkStateHeader* LSHeader)
{
    ListItem* listItem = NULL;

    if (list->first == NULL)
    {
        return FALSE;
    }

    listItem = list->first;

    memcpy(LSHeader, listItem->data, sizeof(Ospfv3LinkStateHeader));

    // Remove item from list
    ListGet(node, list, listItem, TRUE, FALSE);

    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3SendDatabaseDescriptionPacket
// LAYER      :: NETWORK
// PURPOSE    :: Send DD Packet on specified interface.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +neighborId:  unsigned int : Neighbor Id.
//  +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL
// **/
static
void Ospfv3SendDatabaseDescriptionPacket(
    Node* node,
    unsigned int neighborId,
    int interfaceId)
{
    Ospfv3Data* ospf =
        (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                            node,
                            ROUTING_PROTOCOL_OSPFv3,
                            NETWORK_IPV6);

    Ospfv3Area* thisArea = Ospfv3GetArea(
                                node,
                                ospf->pInterface[interfaceId].areaId);

    Message* dbDscrpMsg = NULL;
    Ospfv3DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;
    clocktype delay;
    in6_addr destAddr;
    in6_addr srcAddr;

    // Find neighbor structure
    nbrInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");


    // Create Database Description packet.
    dbDscrpMsg = MESSAGE_Alloc(
                    node,
                    NETWORK_LAYER,
                    ROUTING_PROTOCOL_OSPFv3,
                    MSG_ROUTING_OspfPacket);

#ifdef OSPFv3_DEBUG
    {
        if (nbrInfo->masterSlave == T_Master)
        {
            printf("    Node %d is Master for Neighbor %d\n",
                node->nodeId,
                nbrInfo->neighborId);
        }
        else
        {
            printf("    Node %d is Slave for Neighbor %d\n",
                node->nodeId,
                nbrInfo->neighborId);
        }
    }
#endif

    if (nbrInfo->state == S_ExStart)
    {

        // Create empty packet
        MESSAGE_PacketAlloc(
            node,
            dbDscrpMsg,
            sizeof(Ospfv3DatabaseDescriptionPacket),
            TRACE_OSPFv3);

        dbDscrpPkt = (Ospfv3DatabaseDescriptionPacket* )
            MESSAGE_ReturnPacket(dbDscrpMsg);

        memset(dbDscrpPkt, 0, sizeof(Ospfv3DatabaseDescriptionPacket));

        // Set Init, More and master-slave bit
        dbDscrpPkt->mtuAndBits |= OSPFv3_MS_BIT;
        dbDscrpPkt->mtuAndBits |= OSPFv3_M_BIT;
        dbDscrpPkt->mtuAndBits |= OSPFv3_I_BIT;

        dbDscrpPkt->ddSequenceNumber = nbrInfo->DDSequenceNumber;
        dbDscrpPkt->commonHeader.packetLength =
            sizeof(Ospfv3DatabaseDescriptionPacket);

#ifdef OSPFv3_DEBUG_SYNC
        {
            char clockStr[MAX_STRING_LENGTH];

            ctoa(node->getNodeTime(), clockStr);

            fprintf(stdout,
                    "    Node %d attempt adjacency with neighbor "
                    "0x%x at time %s\n"
                    "    neighbor state is S_ExStart\n",
                node->nodeId,
                neighborId,
                clockStr);

            fprintf(stdout,
                    "    Node %d sending empty DD packet with seqno = %u "
                    "to neigbhor %d\n",
                node->nodeId,
                nbrInfo->DDSequenceNumber,
                nbrInfo->neighborId);
        }
#endif

    }
    else if (nbrInfo->state == S_Exchange)
    {
        Ospfv3LinkStateHeader* payloadBuff;

        int maxLSAHeader =
            (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
            - sizeof(Ospfv3DatabaseDescriptionPacket))
            / sizeof(Ospfv3LinkStateHeader);

        int numLSAHeader = 0;

        payloadBuff = (Ospfv3LinkStateHeader* )
            MEM_malloc(maxLSAHeader * sizeof(Ospfv3LinkStateHeader));

#ifdef OSPFv3_DEBUG_SYNCErr
        {
            fprintf(stdout,
                    "    Node %d preparing DD Packet for Neighbor %d\n",
                node->nodeId, nbrInfo->neighborId);
        }
#endif

        // Get LSA Header list
        while ((numLSAHeader < maxLSAHeader)
                && Ospfv3GetTopLSAFromList(
                        node,
                        nbrInfo->DBSummaryList,
                        &payloadBuff[numLSAHeader]))
        {
            numLSAHeader++;
        }

        // Create packet
        MESSAGE_PacketAlloc(
            node,
            dbDscrpMsg,
            sizeof(Ospfv3DatabaseDescriptionPacket)
            + numLSAHeader
            * sizeof(Ospfv3LinkStateHeader),
            TRACE_OSPFv3);

        dbDscrpPkt = (Ospfv3DatabaseDescriptionPacket* )
            MESSAGE_ReturnPacket(dbDscrpMsg);

        memset(dbDscrpPkt, 0, sizeof(Ospfv3DatabaseDescriptionPacket));


        dbDscrpPkt->mtuAndBits &= (~OSPFv3_I_BIT);

        if (nbrInfo->masterSlave == T_Master)
        {
            if (ListIsEmpty(node, nbrInfo->DBSummaryList))
            {
                dbDscrpPkt->mtuAndBits &= (~OSPFv3_M_BIT);
            }
            else
            {
                dbDscrpPkt->mtuAndBits |= OSPFv3_M_BIT;
            }
            dbDscrpPkt->mtuAndBits |= OSPFv3_MS_BIT;
        }
        else
        {
            dbDscrpPkt->mtuAndBits |= (nbrInfo->optionsBits & OSPFv3_M_BIT);
            dbDscrpPkt->mtuAndBits &= (~OSPFv3_MS_BIT);
        }

        dbDscrpPkt->ddSequenceNumber = nbrInfo->DDSequenceNumber;

        dbDscrpPkt->commonHeader.packetLength =
            (short) (sizeof(Ospfv3DatabaseDescriptionPacket)
                    + numLSAHeader
                    * sizeof(Ospfv3LinkStateHeader));

        memcpy(
            dbDscrpPkt + 1,
            payloadBuff,
            numLSAHeader
            * sizeof(Ospfv3LinkStateHeader));

        MEM_free(payloadBuff);

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "    Node %d send DD packet to neighbor 0x%x\n"
                    "        seqno = %u, numLSA = %d\n",
                node->nodeId,
                neighborId,
                nbrInfo->DDSequenceNumber,
                numLSAHeader);
        }
#endif

    }

    // Fill in the header fields for the Database Description packet
    Ospfv3CreateCommonHeader(
        node,
        &dbDscrpPkt->commonHeader,
        OSPFv3_DATABASE_DESCRIPTION);

    dbDscrpPkt->commonHeader.areaId = ospf->pInterface[interfaceId].areaId;

    // Set MTU
    OSPFv3_SetDDPacketMTUFeild(
        &dbDscrpPkt->mtuAndBits,
        (unsigned ) GetNetworkIPFragUnit(node, interfaceId));

    // Set E-bit if not a stub area
    if (thisArea->transitCapability == TRUE)
    {
        dbDscrpPkt->bitsAndOptions |= OSPFv3_OPTIONS_E_BIT;
    }
    else
    {
        dbDscrpPkt->bitsAndOptions &= (~OSPFv3_OPTIONS_E_BIT);
    }

    // Add to retransmission list
    if (nbrInfo->lastSentDDPkt != NULL)
    {
        MESSAGE_Free(node, nbrInfo->lastSentDDPkt);
        nbrInfo->lastSentDDPkt = NULL;
    }

    nbrInfo->lastSentDDPkt = MESSAGE_Duplicate(node, dbDscrpMsg);

    // Determine destination address
    if (ospf->pInterface[interfaceId].type
        == OSPFv3_POINT_TO_POINT_INTERFACE)
    {
        Ospfv3GetAllSPFRoutersMulticastAddress(&destAddr);
    }
    else
    {
        COPY_ADDR6(nbrInfo->neighborIPAddress, destAddr);
    }

    // Now, send packet
    delay = 0;

#ifdef OSPFv3_DEBUG
    {
        printf("Node %d Sending DD Packet to Neighbor %d\n",
            node->nodeId,
            nbrInfo->neighborId);

        Ospfv3PrintDatabaseDescriptionPacket(node, dbDscrpMsg);
    }
#endif

    nbrInfo->lastDDPktSent = node->getNodeTime();

    ospf->stats.numDDPktSent++;

    // Set retransmission timer if this is Master
    if (nbrInfo->masterSlave == T_Master)
    {




        delay =
            (clocktype) (ospf->pInterface[interfaceId].rxmtInterval
                        + (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER));

        Ospfv3SetTimer(
            node,
            MSG_ROUTING_OspfRxmtTimer,
            nbrInfo->neighborId,
            delay,
            dbDscrpPkt->ddSequenceNumber,
            0,
            OSPFv3_DATABASE_DESCRIPTION);
    }

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;

    TRACE_PrintTrace(
        node,
        dbDscrpMsg,
        TRACE_NETWORK_LAYER,
        PACKET_OUT, &acnData);

    Ipv6GetGlobalAggrAddress(node, interfaceId, &srcAddr);

    Ipv6SendRawMessage(
        node,
        dbDscrpMsg,
        srcAddr,
        destAddr,
        interfaceId,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_OSPF,
        1);

}

// /**
// FUNCTION   :: Ospfv3ScheduleInterfaceEvent
// LAYER      :: NETWORK
// PURPOSE    :: Schedule an interface event to be executed later.
// PARAMETERS ::
//  +node:  Node* : Pointer to node.
//  +interfaceId:  int : Interface Id.
//  +intfEvent:  Ospfv2InterfaceEvent : Interface Event.
// RETURN     :: void : NULL
// **/
static
void Ospfv3ScheduleInterfaceEvent(
    Node* node,
    int interfaceId,
    Ospfv2InterfaceEvent intfEvent)
{
    Ospfv3Data* ospf = (Ospfv3Data * ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Message* newMsg = NULL;
    clocktype delay;
    Ospfv3ISMTimerInfo* ISMTimerInfo = NULL;

    // Timer is already scheduled
    if (ospf->pInterface[interfaceId].ISMTimer)
    {
        return;
    }

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfInterfaceEvent);

    MESSAGE_InfoAlloc(node, newMsg,  sizeof(Ospfv3ISMTimerInfo));

    ISMTimerInfo = (Ospfv3ISMTimerInfo* ) MESSAGE_ReturnInfo(newMsg);

    ISMTimerInfo->interfaceId = interfaceId;
    ISMTimerInfo->intfEvent = intfEvent;

    ospf->pInterface[interfaceId].ISMTimer = TRUE;
    delay = OSPFv3_EVENT_SCHEDULING_DELAY;

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}

// /**
// FUNCTION   :: Ospfv3GetInterfaceForThisNeighbor
// LAYER      :: NETWORK
// PURPOSE    :: Return Interface Id to which the neighbor is connected.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborId:  unsigned int : neighbor Id.
// RETURN     :: Interface Id for the Neighbor.
// **/
static
unsigned int Ospfv3GetInterfaceForThisNeighbor(
    Node* node,
    unsigned int neighborId)
{
    unsigned int i = 0;
    ListItem* tempListItem = NULL;
    Ospfv3Neighbor* tempNeighborInfo = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    for (i = 0; i < (unsigned int) (node->numberInterfaces); i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }
        tempListItem = ospf->pInterface[i].neighborList->first;

        // Search for the neighbor in my neighbor list.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

            ERROR_Assert(tempNeighborInfo,
                        "Neighbor not found in the Neighbor list");

            if (tempNeighborInfo->neighborId == neighborId)
            {
                // Found the neighbor we are looking for.
                return i;
            }

            tempListItem = tempListItem->next;
        }
    }

    return ANY_DEST;
}

// /**
// FUNCTION   :: Ospfv3AttemptAdjacency
// LAYER      :: NETWORK
// PURPOSE    :: Attempt Adjacency with the eligible neighbor.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AttemptAdjacency(
    Node* node,
    Ospfv3Neighbor* nbrInfo,
    int interfaceId)
{

    // If this is the first time that the adjacency has been attempted,
    // the DD Sequence number should be assigned to some unique value.
    if (nbrInfo->DDSequenceNumber == 0)
    {
        nbrInfo->DDSequenceNumber =
            (unsigned int) (node->getNodeTime() / SECOND);
    }
    else
    {
        nbrInfo->DDSequenceNumber++;
    }

    nbrInfo->masterSlave = T_Master;

#ifdef OSPFv3_DEBUG_SYNC
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);

        fprintf(stdout,
                "    Node %d attempt adjacency with neighbor 0x%x at %s\n",
            node->nodeId,
            nbrInfo->neighborId,
            clockStr);

        fprintf(stdout,
                "        Sending blank DD with initial DD seqno = %u\n",
            nbrInfo->DDSequenceNumber);
    }
#endif

    // Send Empty DD Packet
    Ospfv3SendDatabaseDescriptionPacket(
        node,
        nbrInfo->neighborId,
        interfaceId);
}

// /**
// FUNCTION   :: Ospfv3CreateLSReqObject
// LAYER      :: NETWORK
// PURPOSE    :: Create Link State Request Object.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// +LSReqObject:  Ospfv3LSRequestInfo* : Pointer to Link State Request Item.
// +retx:  BOOL : Whether to retransmit or not.
// RETURN     :: TRUE if Object is created, FALSE otherwise.
// **/
static
BOOL Ospfv3CreateLSReqObject(
    Node* node,
    Ospfv3Neighbor* nbrInfo,
    Ospfv3LSRequestInfo* LSReqObject,
    BOOL retx)
{
    ListItem* listItem = nbrInfo->linkStateRequestList->first;
    Ospfv3LSReqItem* itemInfo = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;

    if (retx)
    {
        while (listItem)
        {
            itemInfo = (Ospfv3LSReqItem* ) listItem->data;

            if (itemInfo->seqNumber == nbrInfo->LSReqTimerSequenceNumber)
            {
                LSHeader = itemInfo->LSHeader;

                itemInfo->seqNumber = nbrInfo->LSReqTimerSequenceNumber + 1;
                break;
            }
            listItem = listItem->next;
        }
    }
    else
    {
        while (listItem)
        {
            itemInfo = (Ospfv3LSReqItem* ) listItem->data;

            if (itemInfo->seqNumber == 0)
            {
                LSHeader = itemInfo->LSHeader;

                itemInfo->seqNumber = nbrInfo->LSReqTimerSequenceNumber + 1;
                break;
            }
            listItem = listItem->next;
        }
    }

    if (LSHeader == NULL)
    {
        return FALSE;
    }

#ifdef OSPFv3_DEBUG
    {
        char clkStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clkStr);

        printf("Node %u creating ReqObj for neighbor %d at %s\n",
            node->nodeId,
            nbrInfo->neighborId,
            clkStr);

        printf("   Requesting:\n");

        printf("       linkStateType           = %d\n",
            LSHeader->linkStateType);

        printf("       linkStateID             = %d\n",
            LSHeader->linkStateId);

        printf("       advertisingRouter       = %d\n",
            LSHeader->advertisingRouter);

        printf("       linkStateSequenceNumber = 0x%x\n",
            LSHeader->linkStateSequenceNumber);
    }
#endif

    LSReqObject->reserved = 0;
    LSReqObject->linkStateType = LSHeader->linkStateType;
    LSReqObject->linkStateId = LSHeader->linkStateId;
    LSReqObject->advertisingRouter = LSHeader->advertisingRouter;

    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3SendLSRequestPacket
// LAYER      :: NETWORK
// PURPOSE    :: Send Link State Request. Parameter retx indicates whether
//               we are retransmissing the requests or seding fresh list.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborId:  unsigned int : Interface Id.
// +interfaceId:  int : Interface Id.
// +retx:  BOOL : Whether to retransmit or not.
// RETURN     :: void : NULL
// **/
static
void Ospfv3SendLSRequestPacket(
    Node* node,
    unsigned int neighborId,
    int interfaceId,
    BOOL retx)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Message* reqMsg = NULL;
    Ospfv3LinkStateRequestPacket* reqPkt = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;
    in6_addr dstAddr;
    in6_addr srcAddr;
    clocktype delay;
    Ospfv3LSRequestInfo* payloadBuff = NULL;

    int maxLSReqObject =
        (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
        - sizeof(Ospfv3LinkStateRequestPacket))
        / sizeof(Ospfv3LSRequestInfo);

    int numLSReqObject;

    // Find neighbor structure
    nbrInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");

    payloadBuff = (Ospfv3LSRequestInfo* )
        MEM_malloc(maxLSReqObject * sizeof(Ospfv3LSRequestInfo));

    numLSReqObject = 0;

    // Prepare LS Request packet payload
    while ((numLSReqObject < maxLSReqObject)
            && Ospfv3CreateLSReqObject(
                    node,
                    nbrInfo,
                    &payloadBuff[numLSReqObject],
                    retx))
    {
        numLSReqObject++;
    }

    // Create LS Request packet
    reqMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(
        node,
        reqMsg,
        sizeof(Ospfv3LinkStateRequestPacket)
        + numLSReqObject
        * sizeof(Ospfv3LSRequestInfo),
        TRACE_OSPFv3);

    reqPkt = (Ospfv3LinkStateRequestPacket* ) MESSAGE_ReturnPacket(reqMsg);

    memset(
        reqPkt,
        0,
        sizeof(Ospfv3LinkStateRequestPacket)
        + numLSReqObject
        * sizeof(Ospfv3LSRequestInfo));

    // Fill in the header fields for the LS Request packet
    Ospfv3CreateCommonHeader(
        node,
        &reqPkt->commonHeader,
        OSPFv3_LINK_STATE_REQUEST);

    reqPkt->commonHeader.packetLength =
        (short) (sizeof(Ospfv3LinkStateRequestPacket)
                + numLSReqObject * sizeof(Ospfv3LSRequestInfo));

    reqPkt->commonHeader.areaId = ospf->pInterface[interfaceId].areaId;

    // Add payload
    memcpy(
        reqPkt + 1,
        payloadBuff,
        numLSReqObject * sizeof(Ospfv3LSRequestInfo));

    // Free payload Buffer
    MEM_free(payloadBuff);

    // Determine destination address
    if (ospf->pInterface[interfaceId].type == OSPFv3_POINT_TO_POINT_INTERFACE)
    {
        Ospfv3GetAllSPFRoutersMulticastAddress(&dstAddr);
    }
    else
    {
        COPY_ADDR6(nbrInfo->neighborIPAddress, dstAddr);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout,
                "    Node %d send REQUEST to neighbor 0x%x\n",
            node->nodeId,
            neighborId);
    }
#endif

    // Set retransmission timer
    Ospfv3SetTimer(
        node,
        MSG_ROUTING_OspfRxmtTimer,
        nbrInfo->neighborId,
        ospf->pInterface[interfaceId].rxmtInterval,
        ++nbrInfo->LSReqTimerSequenceNumber,
        0,
        OSPFv3_LINK_STATE_REQUEST);

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;

    TRACE_PrintTrace(
        node,
        reqMsg,
        TRACE_NETWORK_LAYER,
        PACKET_OUT,
        &acnData);


    // Now, send packet
    delay = 0;

    Ipv6GetGlobalAggrAddress(node, interfaceId, &srcAddr);

    Ipv6SendRawMessage(
        node,
        reqMsg,
        srcAddr,
        dstAddr,
        interfaceId,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_OSPF,
        1);

    ospf->stats.numLSReqSent++;
}

// /**
// FUNCTION   :: Ospfv3IsMulticastAddress
// LAYER      :: NETWORK
// PURPOSE    :: Check if Multicast Address.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +multicastAddress:  in6_addr : Ip address.
// RETURN     :: TRUE if Multicast address, FALSE otherwise.
// **/
static
BOOL Ospfv3IsMulticastAddress(Node* node, in6_addr multicastAddress)
{
    if (multicastAddress.s6_addr8[0] == 0xff
        && multicastAddress.s6_addr8[1] == 0x02)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Ospfv3SendUpdatePacket
// LAYER      :: NETWORK
// PURPOSE    :: Send Update Packet on specified interface.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +dstAddr:  in6_addr : Destination Ip address.
// +interfaceId:  int : Interface Id.
// +payload:  char* : Pointer to update packet payload.
// +payloadSize:  int : Size of update packet payload.
// +LSACount:  int : Count of LSA to be send in update packet.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SendUpdatePacket(
    Node* node,
    in6_addr dstAddr,
    int interfaceId,
    char* payload,
    int payloadSize,
    int LSACount,
    bool incrementStat = TRUE)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateUpdatePacket* updatePkt = NULL;
    in6_addr srcAddr;

    Message* updateMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            ROUTING_PROTOCOL_OSPFv3,
                            MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(
        node,
        updateMsg,
        sizeof(Ospfv3LinkStateUpdatePacket)
        + payloadSize,
        TRACE_OSPFv3);

    updatePkt =
        (Ospfv3LinkStateUpdatePacket* ) MESSAGE_ReturnPacket(updateMsg);

    // Fill in the header fields for the Update packet
    Ospfv3CreateCommonHeader(
        node,
        &updatePkt->commonHeader,
        OSPFv3_LINK_STATE_UPDATE);

    updatePkt->commonHeader.packetLength =
            (short) ((sizeof(Ospfv3LinkStateUpdatePacket) + payloadSize));

    updatePkt->commonHeader.areaId = ospf->pInterface[interfaceId].areaId;

    updatePkt->numLSA = LSACount;

    // Add payload
    memcpy(updatePkt + 1, payload, payloadSize);

#ifdef OSPFv3_DEBUG_FLOOD
    {
        char addrStr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&dstAddr, addrStr);

        fprintf(stdout,
                "Node %u sending UPDATE to dest %s\n",
            node->nodeId,
            addrStr);
    }
#endif

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;

    TRACE_PrintTrace(
        node,
        updateMsg,
        TRACE_NETWORK_LAYER,
        PACKET_OUT,
        &acnData);

    Ipv6GetGlobalAggrAddress(node, interfaceId, &srcAddr);

    Ipv6SendRawMessage(
        node,
        updateMsg,
        srcAddr,
        dstAddr,
        interfaceId,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_OSPF,
        1);

    if (incrementStat)
    {
        ospf->stats.numLSUpdateSent++;
    }
}

// /**
// FUNCTION   :: Ospfv3SendAckPacket
// LAYER      :: NETWORK
// PURPOSE    :: Send Acknowledgment Packet on specified interface.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +payload:  char* : Pointer to ACK packet payload.
// +count:  int : Count of LSA to be send in ACK packet.
// +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SendAckPacket(
    Node* node,
    char* payload,
    int count,
    unsigned int neighborId,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Message* ackMsg = NULL;
    Ospfv3LinkStateAckPacket* ackPkt = NULL;
    Ospfv3Neighbor* neighborInfo = NULL;
    in6_addr srcAddr;
    int size;

    neighborInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    Ipv6GetGlobalAggrAddress(node, interfaceId, &srcAddr);

    if (count == 0)
    {
        return;
    }

    size = sizeof(Ospfv3LinkStateAckPacket) + sizeof(Ospfv3LinkStateHeader)
            * count;

    ackMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfPacket);

    MESSAGE_PacketAlloc(node, ackMsg, size, TRACE_OSPFv3);

    ackPkt = (Ospfv3LinkStateAckPacket* ) MESSAGE_ReturnPacket(ackMsg);

    // Copy LSA header to acknowledge packet.
    // This is how we acknowledged the LSA.
    memcpy(
        ackPkt + 1,
        payload,
        sizeof(Ospfv3LinkStateHeader) * count);

    Ospfv3CreateCommonHeader(
        node,
        &(ackPkt->commonHeader),
        OSPFv3_LINK_STATE_ACK);

    ackPkt->commonHeader.packetLength = (short) size;
    ackPkt->commonHeader.areaId = ospf->pInterface[interfaceId].areaId;

#ifdef OSPFv3_DEBUG_FLOOD
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);

        printf("Node %u sending ACK to neighbor %x (neighbor ffffffff mean"
                " any neighbor e.i. flooding of ACK) at time %s\n",
            node->nodeId,
            neighborId,
            clockStr);
    }
#endif

    ospf->stats.numAckSent++;

    //Trace sending pkt
    ActionData acnData;
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;

    TRACE_PrintTrace(
        node,
        ackMsg,
        TRACE_NETWORK_LAYER,
        PACKET_OUT, &acnData);

    if (neighborId == ANY_DEST)
    {
        in6_addr dstAddr;

        // Determine Destination Address
        if ((ospf->pInterface[interfaceId].state == S_DR)
            || (ospf->pInterface[interfaceId].state == S_Backup))
        {
            Ospfv3GetAllSPFRoutersMulticastAddress(&dstAddr);
        }
        else if (ospf->pInterface[interfaceId].type
                == OSPFv3_BROADCAST_INTERFACE)
        {
            Ospfv3GetAllDRoutersMulticastAddress(&dstAddr);
        }
        else
        {
            Ospfv3GetAllSPFRoutersMulticastAddress(&dstAddr);
        }

        Ipv6SendRawMessage(
            node,
            ackMsg,
            srcAddr,
            dstAddr,
            interfaceId,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_OSPF,
            1);
    }
    else
    {
        Ipv6SendRawMessage(
            node,
            ackMsg,
            srcAddr,
            neighborInfo->neighborIPAddress,
            interfaceId,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_OSPF,
            1);
    }
}

// /**
// FUNCTION   :: Ospfv3SendLSUpdate
// LAYER      :: NETWORK
// PURPOSE    :: Send Link State Update Packet on the specified interface.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SendLSUpdate(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Interface* thisInterface = &ospf->pInterface[interfaceId];
    ListItem* listItem = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    in6_addr dstAddr;
    char* payloadBuff = NULL;
    int maxPayloadSize;
    int payloadSize;
    int count;

    if (ListIsEmpty(node, thisInterface->updateLSAList))
    {
        return;
    }

    // Determine Destination Address
    if ((thisInterface->state == S_DR) || (thisInterface->state == S_Backup))
    {
        Ospfv3GetAllSPFRoutersMulticastAddress(&dstAddr);
    }
    else if (thisInterface->type == OSPFv3_BROADCAST_INTERFACE)
    {
        Ospfv3GetAllDRoutersMulticastAddress(&dstAddr);
    }
    else
    {
        Ospfv3GetAllSPFRoutersMulticastAddress(&dstAddr);
    }

    // Determine maximum payload size of update packet
    maxPayloadSize = (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
                        - sizeof(Ospfv3LinkStateUpdatePacket));

    payloadBuff = (char* ) MEM_malloc(maxPayloadSize);

    payloadSize = 0;
    count = 0;

    // Prepare LS Update packet payload
    while (!ListIsEmpty(node, thisInterface->updateLSAList))
    {
        listItem = thisInterface->updateLSAList->first;
        LSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

        if (payloadSize == 0 && LSHeader->length > maxPayloadSize)
        {
            MEM_free(payloadBuff);

            payloadBuff = (char* ) MEM_malloc(LSHeader->length);
        }
        else if (payloadSize + LSHeader->length > maxPayloadSize)
        {

            Ospfv3SendUpdatePacket(
                node,
                dstAddr,
                interfaceId,
                payloadBuff,
                payloadSize,
                count);

            // Reset variables
            payloadSize = 0;
            count = 0;
            continue;
        }

        // Incrememnt LS age transmission delay (in seconds).
        LSHeader->linkStateAge =
            (short) (LSHeader->linkStateAge
                    + ((thisInterface->infTransDelay) / SECOND));

        // LS age has a maximum age limit.
        if (LSHeader->linkStateAge > (OSPFv3_LSA_MAX_AGE / SECOND))
        {
            LSHeader->linkStateAge = OSPFv3_LSA_MAX_AGE / SECOND;
        }

        memcpy(&payloadBuff[payloadSize], listItem->data, LSHeader->length);

        payloadSize += LSHeader->length;
        count++;

        // Remove item from list
        ListGet(
            node,
            thisInterface->updateLSAList,
            listItem,
            TRUE,
            FALSE);
    }

    Ospfv3SendUpdatePacket(
        node,
        dstAddr,
        interfaceId,
        payloadBuff,
        payloadSize,
        count);

    MEM_free(payloadBuff);
}

// /**
// FUNCTION   :: Ospfv3SetNeighborState
// LAYER      :: NETWORK
// PURPOSE    :: Set Neighbor State. Also call scheduler
//               for origination of LSAs.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// +state:  Ospfv2NeighborState : Neighbor State.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SetNeighborState(
    Node* node,
    Ospfv3Neighbor* neighborInfo,
    Ospfv2NeighborState state)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv2NeighborState oldState;

    unsigned int interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                                    node,
                                    neighborInfo->neighborId);

    ERROR_Assert(interfaceId != ANY_DEST, "Neighbor on unknown interface\n");

    oldState = neighborInfo->state;
    neighborInfo->state = state;

#ifdef OSPFv3_DEBUG_SYNCErr
    {
        char clockStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);

        printf("Node %u: neighbor %u goes to %d state at"
                " time %s\n",
            node->nodeId,
            neighborInfo->neighborId,
            neighborInfo->state,
            clockStr);

    }
#endif

    // Attempt to form adjacency if new state is S_ExStart
    if ((oldState != state) && (state == S_ExStart))
    {
        Ospfv3AttemptAdjacency(node, neighborInfo, interfaceId);
    }

    // Need to originate LSA when transitioning from/to S_Full state.
    // This is to inform other neighbors of new link.
    if ((oldState == S_Full && state != S_Full)
        || (oldState != S_Full && state == S_Full))
    {

#ifdef OSPFv3_DEBUG_SYNCErr
        {
            char clockStr[MAX_STRING_LENGTH];

            ctoa(node->getNodeTime(), clockStr);

            if (state == S_Full)
            {
                printf("Node %u: neighbor %u goes to FULL state at"
                        " time %s\n",
                    node->nodeId,
                    neighborInfo->neighborId,
                    clockStr);
            }
        }
#endif
        Ospfv3ScheduleRouterLSA(
            node,
            ospf->pInterface[interfaceId].areaId,
            FALSE);

        Ospfv3ScheduleIntraAreaPrefixLSA(
            node,
            ospf->pInterface[interfaceId].areaId,
            FALSE,
            OSPFv3_ROUTER);

        if (ospf->pInterface[interfaceId].state == S_DR)
        {
            Ospfv3ScheduleNetworkLSA(node, interfaceId, FALSE);

        }
    }

    // It may be necessary to return to DR election algorithm
    if (((oldState < S_TwoWay) && (state >= S_TwoWay))
        || ((oldState >= S_TwoWay) && (state < S_TwoWay)))
    {
        Ospfv3ScheduleInterfaceEvent(node, interfaceId, E_NeighborChange);
    }
}

// /**
// FUNCTION   :: Ospfv3FreeRequestList
// LAYER      :: NETWORK
// PURPOSE    :: Free Link State Request List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FreeRequestList(Node* node, Ospfv3Neighbor* nbrInfo)
{
    ListItem* listItem = nbrInfo->linkStateRequestList->first;

    while (listItem)
    {
        Ospfv3LSReqItem* itemInfo = (Ospfv3LSReqItem* ) listItem->data;

        // Remove item from list
        MEM_free(itemInfo->LSHeader);

        listItem = listItem->next;
    }

    ListFree(node, nbrInfo->linkStateRequestList, FALSE);
}

// /**
// FUNCTION   :: Ospfv3DestroyAdjacency
// LAYER      :: NETWORK
// PURPOSE    :: Destroy Adjacency with the neighbor.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3DestroyAdjacency(Node* node, Ospfv3Neighbor* nbrInfo)
{
    // Clear lists.
    ListFree(node, nbrInfo->linkStateRetxList, FALSE);

    ListFree(node, nbrInfo->DBSummaryList, FALSE);

    Ospfv3FreeRequestList(node, nbrInfo);

    ListInit(node, &nbrInfo->linkStateRetxList);

    ListInit(node, &nbrInfo->DBSummaryList);

    ListInit(node, &nbrInfo->linkStateRequestList);

    nbrInfo->LSReqTimerSequenceNumber += 1;

    if (nbrInfo->lastSentDDPkt != NULL)
    {
        MESSAGE_Free(node, nbrInfo->lastSentDDPkt);

        nbrInfo->lastSentDDPkt = NULL;
    }
}

// /**
// FUNCTION   :: Ospfv3AdjacencyRequire
// LAYER      :: NETWORK
// PURPOSE    :: Check if Adjacency is required with the neighbor.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +neighborInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: TRUE if Adjacency is required, False otherwise.
// **/
static
BOOL Ospfv3AdjacencyRequire(
    Node* node,
    int interfaceId,
    Ospfv3Neighbor* nbrInfo)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    if ((ospf->pInterface[interfaceId].type
        == OSPFv3_POINT_TO_POINT_INTERFACE)
        || (ospf->pInterface[interfaceId].type
        == OSPFv3_POINT_TO_MULTIPOINT_INTERFACE)
        || (ospf->pInterface[interfaceId].designatedRouter == ospf->routerId)
        || (ospf->pInterface[interfaceId].backupDesignatedRouter
        == ospf->routerId)
        || (ospf->pInterface[interfaceId].designatedRouter
        == nbrInfo->neighborId)
        || (ospf->pInterface[interfaceId].backupDesignatedRouter
        == nbrInfo->neighborId))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Ospfv3RemoveNeighborFromNeighborList
// LAYER      :: NETWORK
// PURPOSE    :: Remove a neighbor from neighbor List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborId:  unsigned int : neighborId Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3RemoveNeighborFromNeighborList(Node* node,
                                          unsigned int neighborId)
{
    unsigned int interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                                    node,
                                    neighborId);

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* tempListItem
        = ospf->pInterface[interfaceId].neighborList->first;

    Ospfv3Neighbor* tempNeighborInfo = NULL;

    // Search for the neighbor in my neighbor list.
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

        ERROR_Assert(tempNeighborInfo,
                    "Neighbor not found in the Neighbor list");

        // Found and delete.
        if (tempNeighborInfo->neighborId == neighborId)
        {

#ifdef OSPFv3_DEBUG_ERRORS
            {
                printf("Node %u removing %u from neighbor list\n",
                    node->nodeId,
                    neighborId);
            }
#endif

            // Clear lists.
            ListFree(
                node,
                tempNeighborInfo->linkStateRetxList,
                FALSE);

            ListFree(
                node,
                tempNeighborInfo->DBSummaryList,
                FALSE);

            Ospfv3FreeRequestList(node, tempNeighborInfo);

            if (tempNeighborInfo->lastSentDDPkt != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastSentDDPkt);

                tempNeighborInfo->lastSentDDPkt = NULL;
            }

            if (tempNeighborInfo->lastReceivedDDPacket != NULL)
            {
                MESSAGE_Free(node, tempNeighborInfo->lastReceivedDDPacket);

                tempNeighborInfo->lastReceivedDDPacket = NULL;
            }

            // Then remove neighbor from neighbor list.
            ListGet(
                node,
                ospf->pInterface[interfaceId].neighborList,
                tempListItem,
                TRUE,
                FALSE);

            ospf->neighborCount--;
            return;
        }

        tempListItem = tempListItem->next;
    }

    // Neighbor info should have been found.
    ERROR_Assert(FALSE, "Neighbor not found in the Neighbor list");
}

// BGP Patch
// /**
// FUNCTION   :: Ospfv3NeighborIsInStubArea
// LAYER      :: NETWORK
// PURPOSE    :: Check if Neighbor is in stub Area.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: TRUE if neighbor in stub Area, FALSE otherwise.
// **/
static
BOOL Ospfv3NeighborIsInStubArea(
    Node* node,
    Ospfv3Neighbor* nbrInfo)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int interfaceId;
    Ospfv3Area* thisArea;

    interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                    node,
                    nbrInfo->neighborId);

    thisArea = Ospfv3GetArea(node, ospf->pInterface[interfaceId].areaId);

    if (thisArea->transitCapability == FALSE)
    {
        return TRUE;
    }

    return  FALSE;
}
// BGP END

// /**
// FUNCTION   :: Ospfv3CopyLSA
// LAYER      :: NETWORK
// PURPOSE    :: Create copy of an LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSA:  char* : Pointer to LSA byte.
// RETURN     :: Pointer to copy of LSA.
// **/
static
char* Ospfv3CopyLSA(Node* node, char* LSA)
{
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    char* newLSA = (char* ) MEM_malloc(LSHeader->length);

    memcpy(newLSA, LSA, LSHeader->length);

    return newLSA;
}

// /**
// FUNCTION   :: Ospfv3AddToLSRetxList
// LAYER      :: NETWORK
// PURPOSE    :: Add an LSA in the retransmission List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// +LSA:  char* : Pointer to LSA byte.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AddToLSRetxList(
    Node* node,
    Ospfv3Neighbor* nbrInfo,
    char* LSA)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int interfaceId;
    char* newLSA = Ospfv3CopyLSA(node, LSA);
    Ospfv3LinkStateHeader* newLSHeader = (Ospfv3LinkStateHeader* ) newLSA;

    // Add to link state retransmission list.
    ListInsert(node, nbrInfo->linkStateRetxList, node->getNodeTime(), newLSA);

    interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                    node,
                    nbrInfo->neighborId);

#ifdef OSPFv3_DEBUG_FLOODErr
    {
        printf("    Node %u add LSA with seqno 0x%x to rext"
                " list of neighbor %u\n",
            node->nodeId,
            newLSHeader->linkStateSequenceNumber,
            nbrInfo->neighborId);

        printf("   \tlinkStateType         = %x\n"
                "   \tlinkStateId           = %u\n"
                "   \tadvertisingRouterId   = %u\n",
            newLSHeader->linkStateType,
            newLSHeader->linkStateId,
            newLSHeader->advertisingRouter);
    }
#endif

    // Set timer for possible retransmission.
    if (nbrInfo->LSRetxTimer == FALSE)
    {
        nbrInfo->rxmtTimerSequenceNumber++;

        Ospfv3SetTimer(
            node,
            MSG_ROUTING_OspfRxmtTimer,
            nbrInfo->neighborId,
            ospf->pInterface[interfaceId].rxmtInterval,
            nbrInfo->rxmtTimerSequenceNumber,
            newLSHeader->advertisingRouter,
            OSPFv3_LINK_STATE_UPDATE);

        nbrInfo->LSRetxTimer = TRUE;
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateDBSummaryList
// LAYER      :: NETWORK
// PURPOSE    :: Update Database Summary List List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list:  LinkedList* : Pointer to list Info.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateDBSummaryList(
    Node* node,
    LinkedList* list,
    Ospfv3Neighbor* nbrInfo)
{

    ListItem* item = list->first;

    while (item)
    {
        Ospfv3LinkStateHeader* LSHeader = NULL;

        // Get LSA...
        Ospfv3LinkStateHeader* listLSHeader =
            (Ospfv3LinkStateHeader* ) item->data;

        // Add to retx list if LSAge is MaxAge
        if (listLSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
        {
            Ospfv3AddToLSRetxList(node, nbrInfo, (char* ) listLSHeader);
        }

        // Create new item to insert to list
        LSHeader =
            (Ospfv3LinkStateHeader* ) MEM_malloc(
                                        sizeof(Ospfv3LinkStateHeader));

        memcpy(LSHeader, listLSHeader, sizeof(Ospfv3LinkStateHeader));

        ListInsert(node, nbrInfo->DBSummaryList, 0, LSHeader);

        item = item->next;
    }
}

// /**
// FUNCTION   :: Ospfv3CreateDBSummaryList
// LAYER      :: NETWORK
// PURPOSE    :: Create Database Summary List List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor Info.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3CreateDBSummaryList(
    Node* node,
    Ospfv3Neighbor* nbrInfo)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                                    node,
                                    nbrInfo->neighborId);

    Ospfv3Area* thisArea = Ospfv3GetArea(
                                node,
                                ospf->pInterface[interfaceId].areaId);

    ERROR_Assert(interfaceId != ANY_DEST, "Unknown Neighbor\n");

    ERROR_Assert(thisArea, "Area doesn't exist\n");

#ifdef OSPFv3_DEBUG_SYNCErr
    {
        printf("    Node %d DB summary list before update\n", node->nodeId);

        Ospfv3PrintDBSummaryList(node, nbrInfo->DBSummaryList);
    }
#endif

    // Add Link LSA.
    Ospfv3UpdateDBSummaryList(
        node,
        ospf->pInterface[interfaceId].linkLSAList,
        nbrInfo);

    // Add router LSA Sumary
    Ospfv3UpdateDBSummaryList(node, thisArea->routerLSAList, nbrInfo);

    // Add network LSA Summary
    Ospfv3UpdateDBSummaryList(node, thisArea->networkLSAList, nbrInfo);

    Ospfv3UpdateDBSummaryList(
        node,
        thisArea->interAreaPrefixLSAList,
        nbrInfo);

    Ospfv3UpdateDBSummaryList(
        node,
        thisArea->interAreaRouterLSAList,
        nbrInfo);

    // For virtual neighbor, AS-Extarnal LSA should not be included.
    // BGP-OSPF Patch Start
    if (!Ospfv3NeighborIsInStubArea(node, nbrInfo))
    {
        Ospfv3UpdateDBSummaryList(node, ospf->asExternalLSAList, nbrInfo);
    }
    // BGP-OSPF Patch End

    Ospfv3UpdateDBSummaryList(
        node,
        thisArea->intraAreaPrefixLSAList,
        nbrInfo);

#ifdef OSPFv3_DEBUG_SYNCErr
    {
        printf("    Node %d DB summary list after update\n", node->nodeId);

        Ospfv3PrintDBSummaryList(node, nbrInfo->DBSummaryList);
    }
#endif

}

// /**
// FUNCTION   :: Ospfv3CreateDBSummaryList
// LAYER      :: NETWORK
// PURPOSE    :: Handle neighbor event and change neighbor
//               state accordingly.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +neighborId:  unsigned int : Neighbor Id.
// +eventType:  Ospfv2NeighborEvent : Neighbor Event Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleNeighborEvent(
    Node* node,
    int interfaceId,
    unsigned int neighborId,
    Ospfv2NeighborEvent eventType)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    // Find neighbor structure
    Ospfv3Neighbor* tempNeighborInfo = Ospfv3GetNeighborInfoByRtrId(
                                            node,
                                            neighborId);

    ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list");

    switch (eventType)
    {
        case E_HelloReceived:
        {
            if (tempNeighborInfo->state < S_Init)
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event E_HelloReceived\n"
                            "    Neighbor state (0x%x) move up to S_Init\n",
                        node->nodeId,
                        neighborId);
                }
#endif

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_Init);
            }

            // Reschedule inactivity timer.
            Ospfv3SetTimer(
                node,
                MSG_ROUTING_OspfInactivityTimer,
                neighborId,
                ospf->pInterface[interfaceId].routerDeadInterval,
                0,
                0,
                OSPFv3_HELLO);

            break;
        }
        case E_Start:
        {
            // This event will be generated for NBMA networks only.
            // Avoid this case for now
            break;
        }
        case E_TwoWayReceived:
        {
            if (tempNeighborInfo->state == S_Init)
            {
                // Determine whether adjacency is required
                if (Ospfv3AdjacencyRequire(
                        node,
                        interfaceId,
                        tempNeighborInfo))
                {

#ifdef OSPFv3_DEBUG_SYNC
                    {
                        fprintf(stdout,
                                "    Node %d NSM receive event "
                                "S_TwoWayReceived.\n"
                                "    Neighbor state (0x%x) "
                                "move up to S_ExStart\n",
                            node->nodeId,
                            neighborId);
                    }
#endif

                    Ospfv3SetNeighborState(
                        node,
                        tempNeighborInfo,
                        S_ExStart);
                }
                else
                {

#ifdef OSPFv3_DEBUG_SYNC
                    {
                        fprintf(stdout, "   Node %d NSM receive event "
                                "S_TwoWayReceived\n"
                                "    Neighbor state (0x%x) "
                                "move up to S_TwoWay\n",
                            node->nodeId,
                            neighborId);
                    }
#endif

#ifdef OSPFv3_DEBUG_HELLO
                    {
                        printf("Node: %d Setting two way neighbor state\n",
                                node->nodeId);
                    }
#endif

                    Ospfv3SetNeighborState(
                        node,
                        tempNeighborInfo,
                        S_TwoWay);
                }
            }

            // No state change for neighbor state higher than S-Init
            break;
        }
        case E_NegotiationDone:
        {
            if (tempNeighborInfo->state != S_ExStart)
            {
                break;
            }

            Ospfv3SetNeighborState(node, tempNeighborInfo, S_Exchange);

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                        "    Node %d NSM receive event E_NegotiationDone\n"
                        "    Neighbor state (0x%x) move up to "
                        "E_Exchange\n",
                    node->nodeId,
                    neighborId);
            }
#endif

            // List the contents of its entire area link state database
            // in the neighbor Database summary list.
            Ospfv3CreateDBSummaryList(node, tempNeighborInfo);
            break;
        }

        case E_ExchangeDone:
        {
            if (tempNeighborInfo->state != S_Exchange)
            {
                break;
            }

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                        "    Node %d NSM receive event E_ExchangeDone\n",
                    node->nodeId);
            }
#endif

            if (ListIsEmpty(node, tempNeighborInfo->linkStateRequestList))
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    request list is empty. Neighbor"
                            " state (0x%x) move up to S_Full\n",
                            neighborId);
                }
#endif

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_Full);

#ifdef OSPFv3_DEBUG
                {
                    Ipv6PrintForwardingTable(node);
                }
#endif

            }
            else
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    Neighbor state (0x%x) move up "
                            "to S_Loading\n",
                        neighborId);
                }
#endif

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_Loading);

                // Start (or continue) sending Link State Request packet
                Ospfv3SendLSRequestPacket(
                    node,
                    neighborId,
                    interfaceId,
                    FALSE);
            }
            break;
        }
        case E_LoadingDone:
        {
            if (tempNeighborInfo->state != S_Loading)
            {
                break;
            }

#ifdef OSPFv3_DEBUG_SYNC
            {
                if (tempNeighborInfo->state != S_Full)
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event E_LoadingDone\n"
                            "    Neighbor state (0x%x) move up "
                            "to S_Full\n",
                        node->nodeId,
                        neighborId);
                }
            }
#endif

            Ospfv3SetNeighborState(node, tempNeighborInfo, S_Full);

#ifdef OSPFv3_DEBUG
            {
                Ipv6PrintForwardingTable(node);
            }
#endif

            break;
        }
        case E_AdjOk:
        {
            if ((Ospfv3AdjacencyRequire(
                    node,
                    interfaceId,
                    tempNeighborInfo))
                && (tempNeighborInfo->state == S_TwoWay))
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event E_AdjOk\n"
                            "    Neighbor state (0x%x) "
                            "move up to S_ExStart\n",
                        node->nodeId,
                        neighborId);
                }
#endif

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_ExStart);
            }
            else if ((!Ospfv3AdjacencyRequire(
                            node,
                            interfaceId,
                            tempNeighborInfo))
                    && (tempNeighborInfo->state > S_TwoWay))
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event E_AdjOk\n"
                            "    Neighbor state (0x%x) "
                            "move down to S_TwoWay\n",
                        node->nodeId,
                        neighborId);
                }
#endif

                // Break possibly fromed adjacency
                Ospfv3DestroyAdjacency(node, tempNeighborInfo);

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_TwoWay);
            }

            break;
        }

        case E_BadLSReq:
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                        "    Node %d NSM receive event E_BadLSReq\n"
                        "    Neighbor state (0x%x) move down "
                        "to S_ExStart\n",
                    node->nodeId,
                    neighborId);
            }
#endif

        }
        case E_SeqNumberMismatch:
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                if (eventType == E_SeqNumberMismatch)
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event "
                            "E_SeqNumberMismatch\n"
                            "    Neighbor state (0x%x) "
                            "move down to S_ExStart\n",
                        node->nodeId,
                        neighborId);
                }
            }
#endif

            if (tempNeighborInfo->state < S_Exchange)
            {
                break;
            }

            // Torn down possibly formed adjacency
            Ospfv3DestroyAdjacency(node, tempNeighborInfo);

            Ospfv3SetNeighborState(node, tempNeighborInfo, S_ExStart);

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout, "Attempt to reestablished adjacency\n");
            }
#endif

            break;
        }
        case E_OneWay:
        {
            if (tempNeighborInfo->state >= S_TwoWay)
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "    Node %d NSM receive event E_OneWay\n"
                            "    Neighbor state (0x%x) move down "
                            "to S_Init\n",
                        node->nodeId,
                        neighborId);
                }
#endif

                Ospfv3DestroyAdjacency(node, tempNeighborInfo);

                Ospfv3SetNeighborState(node, tempNeighborInfo, S_Init);
            }
            break;
        }
        case E_InactivityTimer:
        case E_KillNbr:
        case E_LLDown:
        {
#ifdef OSPFv3_DEBUG_SYNC
            {
                printf("    Node %d NSM receive event %d\n"
                        "    Neighbor state (0x%x) move down to "
                        "S_NeighborDown\n",
                    node->nodeId,
                    eventType,
                    neighborId);
            }
#endif

            Ospfv3SetNeighborState(node, tempNeighborInfo, S_NeighborDown);

            Ospfv3RemoveNeighborFromNeighborList(node, neighborId);

            break;
        }

        default:
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "Node %u : Unknown neighbor event %d for "
                    "neighbor (0x%x).\n",
                node->nodeId,
                eventType,
                neighborId);

            ERROR_Assert(FALSE, errorStr);
        }
    }
}

// /**
// FUNCTION   :: Ospfv3ListDREligibleRouters
// LAYER      :: NETWORK
// PURPOSE    :: List DR eligible Routers.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrList:  LinkedList* : Neighbor List Id.
// +eligibleRoutersList:  LinkedList* : DR eligible Routers List Id.
// +interfaceId:  int : interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ListDREligibleRouters(
    Node* node,
    LinkedList* nbrList,
    LinkedList* eligibleRoutersList,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;
    Ospfv3DREligibleRouter* newRouter = NULL;

    // Calculating router itself is considered to be on the list
    if (ospf->pInterface[interfaceId].routerPriority > 0)
    {
        newRouter = (Ospfv3DREligibleRouter* )
            MEM_malloc(sizeof(Ospfv3DREligibleRouter));

        newRouter->routerId = ospf->routerId;
        newRouter->routerPriority =
            ospf->pInterface[interfaceId].routerPriority;

        COPY_ADDR6(
            ospf->pInterface[interfaceId].address,
            newRouter->routerIPAddress );

        newRouter->routerOptions = 0;

        newRouter->DesignatedRouter =
                ospf->pInterface[interfaceId].designatedRouter;

        newRouter->BackupDesignatedRouter =
            ospf->pInterface[interfaceId].backupDesignatedRouter;

        ListInsert(
            node,
            eligibleRoutersList,
            0,
            (void* ) newRouter);
    }

    listItem = nbrList->first;

    while (listItem)
    {
        nbrInfo = (Ospfv3Neighbor* ) listItem->data;

        // Consider only neighbors having established bidirectional
        // communication with this router and have rtrPriority > 0
        if (nbrInfo->state >= S_TwoWay && nbrInfo->neighborPriority > 0)
        {
            newRouter = (Ospfv3DREligibleRouter* )
                MEM_malloc(sizeof(Ospfv3DREligibleRouter));

            newRouter->routerId = nbrInfo->neighborId;
            newRouter->routerPriority = nbrInfo->neighborPriority;

            COPY_ADDR6(
                nbrInfo->neighborIPAddress,
                newRouter->routerIPAddress);

            newRouter->routerOptions = nbrInfo->neighborOptions;
            newRouter->DesignatedRouter = nbrInfo->neighborDesignatedRouter;

            newRouter->BackupDesignatedRouter =
                nbrInfo->neighborBackupDesignatedRouter;

            ListInsert(
                node,
                eligibleRoutersList,
                0,
                (void* ) newRouter);
        }
        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3ElectBDR
// LAYER      :: NETWORK
// PURPOSE    :: Calculate Backup Designated Router.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +eligibleRoutersList:  LinkedList* : DR eligible Routers List Id.
// +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ElectBDR(
    Node* node,
    LinkedList* eligibleRoutersList,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3DREligibleRouter* routerInfo = NULL;
    Ospfv3DREligibleRouter* tempBDR = NULL;
    BOOL flag = FALSE;

    for (listItem = eligibleRoutersList->first;
        listItem; listItem = listItem->next)
    {
        routerInfo = (Ospfv3DREligibleRouter* ) listItem->data;

        // If the router declared itself to be DR
        // it is not eligible to become BDR
        if (routerInfo->routerId == routerInfo->DesignatedRouter)
        {
            continue;
        }

        // If neighbor declared itself to be BDR
        if (routerInfo->routerId == routerInfo->BackupDesignatedRouter)
        {
            if ((flag) && (tempBDR)
                && ((tempBDR->routerPriority > routerInfo->routerPriority)
                || ((tempBDR->routerPriority == routerInfo->routerPriority)
                && (tempBDR->routerId > routerInfo->routerId))))
            {
                // do nothing
            }
            else
            {
                tempBDR = routerInfo;
                flag = TRUE;
            }
        }
        else if (!flag)
        {
            if ((tempBDR)
                && ((tempBDR->routerPriority > routerInfo->routerPriority)
                || ((tempBDR->routerPriority == routerInfo->routerPriority)
                && (tempBDR->routerId > routerInfo->routerId))))
            {
                // do nothing
            }
            else
            {
                tempBDR = routerInfo;
            }
        }
    }

    // Set BDR to this interface
    if (tempBDR)
    {
        ospf->pInterface[interfaceId].backupDesignatedRouter =
            tempBDR->routerId;

        COPY_ADDR6(
            tempBDR->routerIPAddress,
            ospf->pInterface[interfaceId].backupDesignatedRouterIPAddress);

    }
    else
    {
        ospf->pInterface[interfaceId].backupDesignatedRouter = 0;

        memset(
            &ospf->pInterface[interfaceId].backupDesignatedRouterIPAddress,
            0,
            sizeof(in6_addr));
    }
}

// /**
// FUNCTION   :: Ospfv3ElectDR
// LAYER      :: NETWORK
// PURPOSE    :: Called from Ospfv3DRElection. Calculate Designated Router.
//               The Interface State is changed in this process.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +eligibleRoutersList:  LinkedList* : DR eligible Routers List Id.
// +interfaceId:  int : Interface Id.
// RETURN     :: New Interface State.
// **/
static
Ospfv2InterfaceState Ospfv3ElectDR(
                    Node* node,
                    LinkedList* eligibleRoutersList,
                    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3DREligibleRouter* routerInfo = NULL;
    Ospfv3DREligibleRouter* tempDR = NULL;

    for (listItem = eligibleRoutersList->first;
        listItem; listItem = listItem->next)
    {
        routerInfo = (Ospfv3DREligibleRouter* ) listItem->data;

        // If router declared itself to be DR
        if (routerInfo->routerId == routerInfo->DesignatedRouter)
        {
            if ((tempDR)
                && ((tempDR->routerPriority > routerInfo->routerPriority)
                || ((tempDR->routerPriority == routerInfo->routerPriority)
                && (tempDR->routerId > routerInfo->routerId))))
            {
                // do nothing
            }
            else
            {
                tempDR = routerInfo;
            }
        }
    }

    if (tempDR)
    {
        ospf->pInterface[interfaceId].designatedRouter = tempDR->routerId;

        COPY_ADDR6(
            tempDR->routerIPAddress,
            ospf->pInterface[interfaceId].designatedRouterIPAddress);
    }
    else
    {
        ospf->pInterface[interfaceId].designatedRouter =
            ospf->pInterface[interfaceId].backupDesignatedRouter;

        COPY_ADDR6(
            ospf->pInterface[interfaceId].backupDesignatedRouterIPAddress,
            ospf->pInterface[interfaceId].designatedRouterIPAddress);
    }

    // Return new interface state
    if (ospf->pInterface[interfaceId].designatedRouter == ospf->routerId)
    {
        return S_DR;
    }
    else if (ospf->pInterface[interfaceId].backupDesignatedRouter
            == ospf->routerId)
    {
        return S_Backup;
    }
    else if (ospf->pInterface[interfaceId].designatedRouter != 0)
    {
        return S_DROther;
    }
    else
    {
        return S_Waiting;
    }
}

// /**
// FUNCTION   :: Ospfv3ScheduleNeighborEvent
// LAYER      :: NETWORK
// PURPOSE    :: Schedule Neighbor Event.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +nbrId:  unsigned int : Neighbor Id.
// +nbrEvent:  Ospfv2NeighborEvent : Neighbor Event.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ScheduleNeighborEvent(
    Node* node,
    int interfaceId,
    unsigned int nbrId,
    Ospfv2NeighborEvent nbrEvent)
{
    clocktype delay;
    Ospfv3NSMTimerInfo* NSMTimerInfo = NULL;

    Message* newMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        MSG_ROUTING_OspfNeighborEvent);

    MESSAGE_InfoAlloc(node, newMsg,  sizeof(Ospfv3NSMTimerInfo));

    NSMTimerInfo = (Ospfv3NSMTimerInfo* ) MESSAGE_ReturnInfo(newMsg);

    NSMTimerInfo->interfaceId = interfaceId;
    NSMTimerInfo->neighborId = nbrId;
    NSMTimerInfo->nbrEvent = nbrEvent;
    delay = OSPFv3_EVENT_SCHEDULING_DELAY;

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}

// /**
// FUNCTION   :: Ospfv3DRElection
// LAYER      :: NETWORK
// PURPOSE    :: Elect Designated Router.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// RETURN     :: New Interface State.
// **/
static
Ospfv2InterfaceState Ospfv3DRElection(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    LinkedList* eligibleRoutersList = NULL;
    unsigned int oldDR;
    unsigned int oldBDR;
    Ospfv2InterfaceState oldState;
    Ospfv2InterfaceState newState;

    //Make a list of eligible routers that will participate in DR election
    ListInit(node, &eligibleRoutersList);

    Ospfv3ListDREligibleRouters(
        node,
        ospf->pInterface[interfaceId].neighborList,
        eligibleRoutersList,
        interfaceId);

    // Note current values for the network's DR and BDR
    oldDR = ospf->pInterface[interfaceId].designatedRouter;
    oldBDR = ospf->pInterface[interfaceId].backupDesignatedRouter;

    // Note interface state
    oldState = ospf->pInterface[interfaceId].state;

    // RFC-2328, Section: 9.4.2 & 9.4.3
    // First election of DR and BDR
    Ospfv3ElectBDR(node, eligibleRoutersList, interfaceId);

    newState = Ospfv3ElectDR(
                    node,
                    eligibleRoutersList,
                    interfaceId);

    ListFree(node, eligibleRoutersList, FALSE);

    // RFC-2328, Section: 9.4.4
    // If Router X is now newly the Designated Router or newly the Backup
    // Designated Router, or is now no longer the Designated Router or no
    // longer the Backup Designated Router, repeat steps 2 and 3, and then
    // proceed to step 5.
    if ((newState != oldState)
        && ((newState != S_DROther) || (oldState > S_DROther)))
    {
        ListInit(node, &eligibleRoutersList);

        Ospfv3ListDREligibleRouters(
            node,
            ospf->pInterface[interfaceId].neighborList,
            eligibleRoutersList,
            interfaceId);

        Ospfv3ElectBDR(node, eligibleRoutersList, interfaceId);

        newState = Ospfv3ElectDR(
                        node,
                        eligibleRoutersList,
                        interfaceId);

        ListFree(node, eligibleRoutersList, FALSE);
    }

#ifdef OSPFv3_DEBUG_ISM
    {
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        printf("    Node %d declare DR = %d and BDR = %d\n"
                "    on interface %d at time %s\n",
            node->nodeId,
            ospf->pInterface[interfaceId].designatedRouter,
            ospf->pInterface[interfaceId].backupDesignatedRouter,
            interfaceId,
            clockStr);
    }
#endif

    // RFC-2328, Section: 9.4.7
    // If the above calculations have caused the identity of either the
    // Designated Router or Backup Designated Router to change, the set
    // of adjacencies associated with this interface will need to be
    // modified.
    if ((oldDR != ospf->pInterface[interfaceId].designatedRouter)
        || (oldBDR != ospf->pInterface[interfaceId].backupDesignatedRouter))
    {
        ListItem* listItem = NULL;
        Ospfv3Neighbor* nbrInfo = NULL;

        for (listItem = ospf->pInterface[interfaceId].neighborList->first;
            listItem; listItem = listItem->next)
        {
            nbrInfo = (Ospfv3Neighbor* ) listItem->data;

            if (nbrInfo->state >= S_TwoWay)
            {
                Ospfv3ScheduleNeighborEvent(
                    node,
                    interfaceId,
                    nbrInfo->neighborId,
                    E_AdjOk);
            }
        }
    }
    return newState;

}

// /**
// FUNCTION   :: Ospfv3SetInterfaceState
// LAYER      :: NETWORK
// PURPOSE    :: Set Interface State.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +newState:  Ospfv2InterfaceState : New Interface State.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SetInterfaceState(
    Node* node,
    int interfaceId,
    Ospfv2InterfaceState newState)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv2InterfaceState oldState = ospf->pInterface[interfaceId].state;

    ospf->pInterface[interfaceId].state = newState;

    // We may need to produce a new instance of router LSA
    Ospfv3ScheduleRouterLSA(
        node,
        ospf->pInterface[interfaceId].areaId,
        FALSE);

    //Ospfv3ScheduleLinkLSA(node, interfaceId, FALSE);
    Ospfv3ScheduleIntraAreaPrefixLSA(
        node,
        ospf->pInterface[interfaceId].areaId,
        FALSE,
        OSPFv3_ROUTER);

    // If I'm new DR the produce network LSA for this network.
    if (oldState != S_DR && newState == S_DR)
    {
        Ospfv3ScheduleNetworkLSA(node, interfaceId, FALSE);
    }

    // Else if I'm no longer the DR, flush previously
    // originated network LSA
    else if (oldState == S_DR && newState != S_DR)
    {
        // Flush previously originated network LSA from routing domain
        Ospfv3ScheduleNetworkLSA(node, interfaceId, TRUE);
    }
}

// /**
// FUNCTION   :: Ospfv3HandleInterfaceEvent
// LAYER      :: NETWORK
// PURPOSE    :: Handle Interface Events.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +eventType:  Ospfv2InterfaceState : Interface State.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleInterfaceEvent(
    Node* node,
    int interfaceId,
    Ospfv2InterfaceEvent eventType)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv2InterfaceState newState = S_Down;

    // Copy old state information
    Ospfv2InterfaceState oldState = ospf->pInterface[interfaceId].state;

    switch (eventType)
    {
        case E_InterfaceUp:
        {
            if (ospf->pInterface[interfaceId].state != S_Down)
            {
                newState = oldState;
                break;
            }

            // Start Hello timer & enabling periodic sending of Hello packet
            // Virtual Link not Implemented.
            if ((ospf->pInterface[interfaceId].type
                == OSPFv3_POINT_TO_POINT_INTERFACE)
               || (ospf->pInterface[interfaceId].type
               == OSPFv3_POINT_TO_MULTIPOINT_INTERFACE)
               || (ospf->pInterface[interfaceId].type
               == OSPFv3_VIRTUAL_LINK_INTERFACE))
            {
                newState = S_PointToPoint;
            }
            else if (ospf->pInterface[interfaceId].routerPriority == 0)
            {
                newState = S_DROther;
            }
            else
            {
                newState = S_Waiting;

                Ospfv3ScheduleWaitTimer(node, interfaceId);
            }

            break;
        }

        case E_WaitTimer:
        case E_BackupSeen:
        case E_NeighborChange:
        {
            if (((eventType == E_BackupSeen || eventType == E_WaitTimer)
                && (ospf->pInterface[interfaceId].state != S_Waiting))
                || ((eventType == E_NeighborChange)
                && (ospf->pInterface[interfaceId].state != S_DROther
                && ospf->pInterface[interfaceId].state != S_Backup
                && ospf->pInterface[interfaceId].state != S_DR)))
            {
                newState = oldState;
                break;
            }

            newState = Ospfv3DRElection(node, interfaceId);

            break;
        }
        case E_LoopInd:
        case E_UnloopInd:
        {
            // These 2 events are not used in simulation
            break;
        }

        case E_InterfaceDown:
        {
            // 1) Reset all interface variables
            // 2) Disable timers associated with this interface
            // 3) Generate S_KillNbr event to destry all neighbor
            //      associated with this interface
            ListItem* listItem =
                ospf->pInterface[interfaceId].neighborList->first;

            in6_addr address;

            Ipv6GetGlobalAggrAddress(node, interfaceId, &address);

            while (listItem)
            {
                Ospfv3Neighbor* nbrInfo = (Ospfv3Neighbor* ) listItem->data;

                listItem = listItem->next;

                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    nbrInfo->neighborId,
                    E_KillNbr);
            }

            ospf->pInterface[interfaceId].designatedRouter = 0;

            memset(
                &ospf->pInterface[interfaceId].designatedRouterIPAddress,
                0,
                sizeof(in6_addr));

            ospf->pInterface[interfaceId].backupDesignatedRouter = 0;

            memset(
                &ospf->pInterface[interfaceId].backupDesignatedRouterIPAddress,
                0,
                sizeof(in6_addr));

            ospf->pInterface[interfaceId].floodTimerSet = FALSE;
            ospf->pInterface[interfaceId].delayedAckTimer = FALSE;

            ListFree(
                node,
                ospf->pInterface[interfaceId].updateLSAList,
                FALSE);

            ListFree(
                node,
                ospf->pInterface[interfaceId].delayedAckList,
                FALSE);

            ListInit(
                node,
                &ospf->pInterface[interfaceId].updateLSAList);

            ListInit(
                node,
                &ospf->pInterface[interfaceId].delayedAckList);

            newState = S_Down;
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown interface event!\n");
        }
    }
    if (oldState != newState)
    {
        Ospfv3SetInterfaceState(node, interfaceId, newState);
    }
}

// /**
// FUNCTION   :: Ospfv3IsMyAddress
// LAYER      :: NETWORK
// PURPOSE    :: Determine if an IP address is my own.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +addr:  in6_addr : Ip Address.
// +eventType:  Ospfv2InterfaceState : Interface State.
// RETURN     :: TRUE if it's my IP address, FALSE otherwise.
// **/
static
BOOL Ospfv3IsMyAddress(Node* node, in6_addr addr)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    in6_addr tempAddr;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        Ipv6GetGlobalAggrAddress(node, i, &tempAddr);

        if (CMP_ADDR6(addr, tempAddr) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3RetransmitLSA
// LAYER      :: NETWORK
// PURPOSE    :: Retransmit LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrInfo:  Ospfv3Neighbor* : Neighbor Info.
// +interfaceId:  int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3RetransmitLSA(
    Node* node,
    Ospfv3Neighbor* nbrInfo,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    char* payloadBuff = NULL;
    int maxPayloadSize;
    int payloadSize;
    int count;

    if (ListIsEmpty(node, nbrInfo->linkStateRetxList))
    {
        nbrInfo->LSRetxTimer = FALSE;
        nbrInfo->rxmtTimerSequenceNumber++;
        return;
    }

    // Determine maximum payload size of update packet
    maxPayloadSize = (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
                        - sizeof(Ospfv3LinkStateUpdatePacket));

    payloadBuff = (char* ) MEM_malloc(maxPayloadSize);

    payloadSize = 0;
    count = 0;
    listItem = nbrInfo->linkStateRetxList->first;

    // Prepare LS Update packet payload
    while (listItem)
    {
        LSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

        if (payloadSize + LSHeader->length > maxPayloadSize)
        {
            break;
        }

        // Incrememnt LS age transmission delay (in seconds).
        LSHeader->linkStateAge =
            (short) (LSHeader->linkStateAge
            + ((ospf->pInterface[interfaceId].infTransDelay / SECOND)));

        // LS age has a maximum age limit.
        if (LSHeader->linkStateAge > (OSPFv3_LSA_MAX_AGE / SECOND))
        {
            LSHeader->linkStateAge = OSPFv3_LSA_MAX_AGE / SECOND;
        }

#ifdef OSPFv3_DEBUG_FLOOD
        {
            char clockStr[MAX_STRING_LENGTH];

            ctoa(node->getNodeTime(), clockStr);

            printf("   Node %u Retransmitting LSA to node %u at %s\n"
                    "   \ttype              = %d\n"
                    "   \tlinkStateId       = %d\n"
                    "   \tadvertisingRouter = %d\n"
                    "   \tSeqNo             = 0x%x\n",
                node->nodeId,
                nbrInfo->neighborId,
                clockStr,
                LSHeader->linkStateType,
                LSHeader->linkStateId,
                LSHeader->advertisingRouter,
                LSHeader->linkStateSequenceNumber);
        }
#endif

        memcpy(&payloadBuff[payloadSize], listItem->data, LSHeader->length);

        payloadSize += LSHeader->length;
        count++;
        listItem = listItem->next;
    }

    Ospfv3SendUpdatePacket(
        node,
        nbrInfo->neighborIPAddress,
        interfaceId,
        payloadBuff,
        payloadSize,
        count,
        FALSE);

    ospf->stats.numLSUpdateRxmt++;
    nbrInfo->rxmtTimerSequenceNumber++;

    // If LSA stil exist in retx list, start timer again
    if (!ListIsEmpty(node, nbrInfo->linkStateRetxList))
    {
        Ospfv3SetTimer(
            node,
            MSG_ROUTING_OspfRxmtTimer,
            nbrInfo->neighborId,
            ospf->pInterface[interfaceId].rxmtInterval,
            nbrInfo->rxmtTimerSequenceNumber,
            0,
            OSPFv3_LINK_STATE_UPDATE);

        nbrInfo->LSRetxTimer = TRUE;
    }
    else
    {
        nbrInfo->LSRetxTimer = FALSE;
    }

    MEM_free(payloadBuff);
}

// /**
// FUNCTION   :: Ospfv3HandleRetransmitTimer
// LAYER      :: NETWORK
// PURPOSE    :: Handle Retransmit Timer.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +sequenceNumber:  unsigned int : Timer's sequence number.
// +neighborId:  unsigned int : Neighbor Id.
// +type:  Ospfv3PacketType : Packet Type.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleRetransmitTimer(Node* node,
                                 unsigned int sequenceNumber,
                                 unsigned int neighborId,
                                 Ospfv3PacketType type)
{

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* nbrInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    Message* msg = NULL;

    unsigned int interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                                    node,
                                    neighborId);

    BOOL foundPacket = FALSE;

    ERROR_Assert(nbrInfo, "Neighbor not found in the Neighbor list");

    ERROR_Assert(interfaceId != ANY_DEST,
                "Not found any interface on which this neighbor"
                " is attached");

    switch (type)
    {
        case OSPFv3_LINK_STATE_UPDATE:
        {
            // Timer expired.
            if (sequenceNumber != nbrInfo->rxmtTimerSequenceNumber)
            {

#ifdef OSPFv3_DEBUG_FLOODErr
                {
                    printf("    Old timer\n");
                }
#endif

                // Timer is an old one, so just ignore.
                break;
            }

            Ospfv3RetransmitLSA(node, nbrInfo, interfaceId);

            break;
        }

        case OSPFv3_DATABASE_DESCRIPTION:
        {
            Ospfv3DatabaseDescriptionPacket* dbDscrpPkt = NULL;

            msg = nbrInfo->lastSentDDPkt;

            // Old Timer
            if (nbrInfo->masterSlave != T_Master)
            {
                break;
            }
            if (msg == NULL)
            {
                // No packet to retransmit
                break;
            }

            dbDscrpPkt = (Ospfv3DatabaseDescriptionPacket* )
                MESSAGE_ReturnPacket(msg);

            // Send only database description packets that the
            // rmxtTimer indicates.
            if ((dbDscrpPkt->ddSequenceNumber == nbrInfo->DDSequenceNumber)
                && (dbDscrpPkt->ddSequenceNumber == sequenceNumber))
            {

#ifdef OSPFv3_DEBUG
                {
                    char clockStr[MAX_STRING_LENGTH];

                    ctoa(node->getNodeTime(), clockStr);

                    fprintf(stdout,
                            "Node %u retransmitting DD packet "
                            "(seqno %u) to node %d at time %s\n",
                        node->nodeId,
                        sequenceNumber,
                        nbrInfo->neighborId,
                        clockStr);
                }
#endif

                foundPacket = TRUE;

                // Packet is retransmitted at the end of function.
                ospf->stats.numDDPktRxmt++;

                // Set retransmission timer again if Master.
                if (nbrInfo->masterSlave == T_Master)
                {
                    clocktype delay;

                    delay = (clocktype)
                            (ospf->pInterface[interfaceId].rxmtInterval
                            + (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER));

                    Ospfv3SetTimer(
                        node,
                        MSG_ROUTING_OspfRxmtTimer,
                        nbrInfo->neighborId,
                        delay,
                        dbDscrpPkt->ddSequenceNumber,
                        0,
                        OSPFv3_DATABASE_DESCRIPTION);
                }
            }
            break;
        }
        case OSPFv3_LINK_STATE_REQUEST:
        {
            if (nbrInfo->LSReqTimerSequenceNumber != sequenceNumber)
            {
                // No packet to retransmit
                break;
            }

#ifdef OSPFv3_DEBUG_SYNC
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                fprintf(stdout,
                        "Node %u retransmitting LS Request packet "
                        "to node 0x%x at time %s\n",
                    node->nodeId,
                    nbrInfo->neighborId,
                    clockStr);
            }
#endif

            Ospfv3SendLSRequestPacket(
                node,
                neighborId,
                interfaceId,
                TRUE);

            ospf->stats.numLSReqRxmt++;

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown packet type");
        }
    }
    if (foundPacket)
    {
        in6_addr sourceAddr;

        //Trace sending pkt
        ActionData acnData;
        acnData.actionType = SEND;
        acnData.actionComment = NO_COMMENT;

        TRACE_PrintTrace(
            node,
            msg,
            TRACE_NETWORK_LAYER,
            PACKET_OUT, &acnData);

        // Found packet to retransmit, so done...
        // delay = 0;

        Ipv6GetGlobalAggrAddress(
            node,
            interfaceId,
            &sourceAddr);

        Ipv6SendRawMessage(
            node,
            MESSAGE_Duplicate(node, nbrInfo->lastSentDDPkt),
            sourceAddr,
            nbrInfo->neighborIPAddress,
            interfaceId,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_OSPF,
            1);
    }
}

// /**
// FUNCTION   :: Ospfv3AddPrefixInfoForRouter
// LAYER      :: NETWORK
// PURPOSE    :: Called from Ospfv3OriginateIntraAreaPrefixLSAForRouter.
//               Add prefix Info in LSA body.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +prefixInfo:  Ospfv3PrefixInfo** : Pointer to pointer to Prefix Info.
// +interfaceId:  unsigned int : interfaceId Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AddPrefixInfoForRouter(
    Node* node,
    Ospfv3PrefixInfo** prefixInfo,
    unsigned int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* )NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    (*prefixInfo)->prefixOptions |= OSPFv3_PREFIX_LA_BIT;

    (*prefixInfo)->reservedOrMetric =
        (short)ospf->pInterface[interfaceId].outputCost;
}

// /**
// FUNCTION   :: Ospfv3InterfaceBelongToThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Verify if the interface belong to this area.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +areaId:  unsigned int : Area Id.
// +interfaceId:  int : interfaceId Id.
// RETURN     :: True if interface belongs to this Area.
// **/
static
BOOL Ospfv3InterfaceBelongToThisArea(
    Node* node,
    unsigned int areaId,
    int interfaceId)
{
    ListItem* listItem = NULL;
    Ospfv3Interface* thisInterface = NULL;

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    in6_addr tempAddress;

    memset(&tempAddress, 0, sizeof(in6_addr));

    ERROR_Assert(thisArea, "Area doesn't exist\n");

    listItem = thisArea->connectedInterface->first;

    Ipv6GetGlobalAggrAddress(node, interfaceId, &tempAddress);

    while (listItem)
    {
        thisInterface = (Ospfv3Interface* ) listItem->data;

        if (CMP_ADDR6(thisInterface->address, tempAddress) == 0)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3ScheduleSPFCalculation
// LAYER      :: NETWORK
// PURPOSE    :: Schedule SPF calculation.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ScheduleSPFCalculation(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* )NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Message* newMsg = NULL;
    clocktype delay;

    if (ospf->SPFTimer > node->getNodeTime())
    {
        return;
    }

    delay = (clocktype) (RANDOM_nrand(ospf->seed) % OSPFv3_BROADCAST_JITTER);

    ospf->SPFTimer = node->getNodeTime() + delay;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleSPF);

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}

// /**
// FUNCTION   :: Ospfv3OriginateRtrIntraAreaPrefixLSA
// LAYER      :: NETWORK
// PURPOSE    :: Originate Intra Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +areaId:  unsigned int : Area Id.
// +flush:  BOOL : flush or not flush.
// RETURN     :: void : NULL.
// **/
void Ospfv3OriginateIntraAreaPrefixLSAForRouter(
    Node* node,
    unsigned int areaId,
    BOOL flush)
{
    int i;
    int count = 0;
    int listSize = 0;
    Ospfv3IntraAreaPrefixLSA* LSA = NULL;
    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    Ospfv3PrefixInfo* prefixInfo = NULL;
    Ospfv3PrefixInfo* tempWorkingPointer = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* )NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    listSize = 2 * (ospf->neighborCount + node->numberInterfaces);

    prefixInfo = (Ospfv3PrefixInfo* ) MEM_malloc(
                                        sizeof(Ospfv3PrefixInfo) * listSize);

    memset(
        prefixInfo,
        0,
        sizeof(Ospfv3PrefixInfo) * listSize);

    tempWorkingPointer = prefixInfo;

    for (i = 0; i < node->numberInterfaces; i++)
    {

        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // If the attached network does not belong to this area, no links are
        // added to the LSA, and the next interface should be examined.
        if ((areaId != OSPFv3_SINGLE_AREA_ID)
            && (!Ospfv3InterfaceBelongToThisArea(node, areaId, i)))
        {
            continue;
        }

        // If interface state is down, no link should be added
        if (ospf->pInterface[i].state == S_Down)
        {
            continue;
        }

        // If interface is Point-to-Multipoint
        if (ospf->pInterface[i].type == OSPFv3_POINT_TO_MULTIPOINT_INTERFACE)
        {
            ListItem* nbrListItem =  ospf->pInterface[i].neighborList->first;
            Ospfv3Neighbor* neighborInfo = NULL;

            Ospfv3AddPrefixInfoForRouter(node, &tempWorkingPointer, i);

            COPY_ADDR6(
                ospf->pInterface[i].address,
                tempWorkingPointer->prefixAddr);

            tempWorkingPointer->prefixLength = ospf->pInterface[i].prefixLen;
            count += 1;
            tempWorkingPointer++;

            while (nbrListItem)
            {
                neighborInfo = (Ospfv3Neighbor* )nbrListItem->data;

                ERROR_Assert(neighborInfo,
                            "\n Nighbor not found in Neighbor List");

                Ospfv3AddPrefixInfoForRouter(node, &tempWorkingPointer, i);

                COPY_ADDR6(
                    neighborInfo->neighborIPAddress,
                    tempWorkingPointer->prefixAddr);

                tempWorkingPointer->prefixLength = OSPFv3_MAX_PREFIX_LENGTH;

                tempWorkingPointer++;
                count += 1;
                nbrListItem = nbrListItem->next;
            }
        }
        else if (ospf->pInterface[i].type == OSPFv3_POINT_TO_POINT_INTERFACE
                || ospf->pInterface[i].type == OSPFv3_BROADCAST_INTERFACE)
        {

            // Cuurent configuration is TLA NLA SLA format
            Ospfv3AddPrefixInfoForRouter(node, &tempWorkingPointer, i);

            COPY_ADDR6(
                ospf->pInterface[i].address,
                tempWorkingPointer->prefixAddr);

            tempWorkingPointer->prefixLength = ospf->pInterface[i].prefixLen;

            count += 1;
            tempWorkingPointer++;
        }
    }

#ifdef OSPFv3_DEBUG_LSDBErr
    {
        printf("    Total entries are %d\n", count);
    }
#endif

    ERROR_Assert(count <= listSize, "Calculation of listSize is wrong\n");

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    thisArea->intraAreaPrefixLSAList,
                    ospf->routerId,
                    ospf->routerId);


    LSA = (Ospfv3IntraAreaPrefixLSA* ) MEM_malloc(
                                            sizeof(Ospfv3IntraAreaPrefixLSA)
                                            + (sizeof(Ospfv3PrefixInfo)
                                            * count));

    memset(LSA,
        0,
        sizeof(Ospfv3IntraAreaPrefixLSA)
        + (sizeof(Ospfv3PrefixInfo) * count));

    if (!Ospfv3CreateLSHeader(
            node,
            areaId,
            OSPFv3_INTRA_AREA_PREFIX,
            &LSA->LSHeader,
            oldLSHeader))
    {
        MEM_free(prefixInfo);

        MEM_free(LSA);

        Ospfv3ScheduleIntraAreaPrefixLSA(
            node,
            areaId,
            FALSE,
            OSPFv3_ROUTER);

        return;
    }
    if (flush)
    {
        LSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }

    LSA->LSHeader.linkStateId = ospf->routerId;

    LSA->LSHeader.length =
        (short) (sizeof(Ospfv3IntraAreaPrefixLSA)
                + (sizeof(Ospfv3PrefixInfo) * count));

    LSA->numPrefixes = (unsigned short)count;
    LSA->referencedAdvertisingRouter = ospf->routerId;
    LSA->referencedLSId = 0;
    LSA->referencedLSType = OSPFv3_ROUTER;

    // Copy my link information into the LSA.
    memcpy(
        LSA + 1,
        prefixInfo,
        ((sizeof(Ospfv3PrefixInfo) * count)));

    // Note LSA Origination time
    thisArea->interAreaPrefixLSOriginateTime = node->getNodeTime();
    thisArea->interAreaPrefixLSTimer = FALSE;

#ifdef OSPFv3_DEBUG_LSDB
    {
        printf("    Node %d adding originated LSA to its own LSDB\n",
            node->nodeId);
    }
#endif

    if (Ospfv3InstallLSAInLSDB(
            node,
            thisArea->intraAreaPrefixLSAList,
            (char* ) LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout,
                "\nNode %u Flood self originated Intra-Area-Prefix LSA\n",
            node->nodeId);
    }
#endif

    // Flood LSA
    Ospfv3FloodLSAThroughArea(node, (char* )LSA, ANY_DEST, areaId);

    ospf->stats.numIntraAreaPrefixLSAOriginate++;

    MEM_free(prefixInfo);

    MEM_free(LSA);
}

// /**
// FUNCTION   :: Ospfv3DRFullyAdjacentWithRouter
// LAYER      :: NETWORK
// PURPOSE    :: Check if the Router is fully adjacent with Designated Rtr.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// +advertisingRouter:  unsigned int : Advertising Router Id.
// RETURN     :: TRUE if DR fully adjacent, FALSE otherwise.
// **/
static
BOOL Ospfv3DRFullyAdjacentWithRouter(
    Node* node,
    int interfaceId,
    unsigned int advertisingRouter)
{
    Ospfv3Data* ospf = (Ospfv3Data* )NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* neighborInfo = NULL;
    ListItem* listItem = ospf->pInterface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv3Neighbor* ) listItem->data;

        if (advertisingRouter == neighborInfo->neighborId
            && neighborInfo->state == S_Full)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3FlushLSA
// LAYER      :: NETWORK
// PURPOSE    :: Flush LSA from Area routing domain.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSA:  char* : Pointer to LSA.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL.
// NOTE         :: To flush AS-External LSA call this function with areaId
//                 argument as OSPFv3_INVALID_AREA_ID. For Link LSA this
//                 areaId is interfaceId.
// **/
static
void Ospfv3FlushLSA(Node* node, char* LSA, unsigned int areaId)
{
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    LSHeader->linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);

#ifdef OSPFv3_DEBUG_FLOOD
    {
        char timeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);

        printf("Node %u flush LSA in area %u at time %s\n",
            node->nodeId,
            areaId,
            timeStr);

        printf("    Type = %d, ID = 0x%x\n",
            LSHeader->linkStateType,
            LSHeader->linkStateId);
    }
#endif

    if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
    {
        Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

        ListItem* listItem = ospf->area->first;

        ERROR_Assert(areaId == OSPFv3_INVALID_AREA_ID,
                "Wrong function call\n");

        while (listItem)
        {
            Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

            ERROR_Assert(thisArea, "Area structure not initialized");

            Ospfv3FloodLSAThroughArea(node, LSA, ANY_DEST, thisArea->areaId);

            listItem = listItem->next;
        }
    }
    else if (LSHeader->linkStateType == OSPFv3_LINK)
    {
        unsigned int interfaceId = areaId;

        Ospfv3FloodLSAOnInterface(node, LSA, ANY_DEST, interfaceId);
    }
    else
    {
        // Flood LSA
        Ospfv3FloodLSAThroughArea(node, LSA, ANY_DEST, areaId);
    }
}

// /**
// FUNCTION   :: Ospfv3FloodThroughAS
// LAYER      :: NETWORK
// PURPOSE    :: Flush LSA from routing domain.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSA:  char* : Pointer to LSA.
// +neighborId:  unsigned int : Neighbor Id.
// RETURN     :: TRUE if one of areas was not stub, FALSE otherwise.
// **/
static
BOOL Ospfv3FloodThroughAS(Node* node, char* LSA, unsigned int neighborId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = NULL;
    ListItem* listItem = NULL;
    BOOL retVal = FALSE;
    listItem = ospf->area->first;

    while (listItem)
    {
        thisArea = (Ospfv3Area* ) listItem->data;

        // If this is not a Stub area, flood lsa through this area
        if (thisArea->transitCapability)
        {
            if (Ospfv3FloodLSAThroughArea(
                    node,
                    LSA,
                    neighborId,
                    thisArea->areaId))
            {
                retVal = TRUE;
            }
        }

        listItem = listItem->next;
    }

    return retVal;
}

// /**
// FUNCTION   :: Ospfv3ScheduleASExternalLSA
// LAYER      :: NETWORK
// PURPOSE    :: Schedule AS-External LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +destAddress:  in6_addr : Dest Ip Address.
// +prefixLength:  unsigned char : Dest Prefix Length.
// +nextHopAddress:  in6_addr : Next Hop Ip Address.
// +external2Cost:  int : external type-2 Cost.
// +flush:  BOOL : flush or not flush.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ScheduleASExternalLSA(
    Node* node,
    in6_addr destAddress,
    unsigned char prefixLength,
    in6_addr nextHopAddress,
    int external2Cost,
    BOOL flush)

{
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;

    Message* newMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType) + (sizeof(in6_addr) * 2)
        + sizeof(unsigned char)
        + sizeof(int) + sizeof(BOOL));

    lsType = OSPFv3_AS_EXTERNAL;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &destAddress, sizeof(in6_addr));

    msgInfo += sizeof(in6_addr);

    memcpy(msgInfo, &prefixLength, sizeof(unsigned char ));

    msgInfo += sizeof(unsigned char);

    memcpy(msgInfo, &nextHopAddress, sizeof(in6_addr));

    msgInfo += sizeof(in6_addr);

    memcpy(msgInfo, &external2Cost, sizeof(int));

    msgInfo += sizeof(int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    MESSAGE_Send(node, newMsg, (clocktype) OSPFv3_MIN_LS_INTERVAL);
}

// /**
// FUNCTION   :: Ospfv3ScheduleLSA
// LAYER      :: NETWORK
// PURPOSE    :: Schedule Router or Network LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSHeader:  Ospfv3LinkStateHeader* : Pointer to LS header.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ScheduleLSA(
    Node* node,
    Ospfv3LinkStateHeader* LSHeader,
    unsigned int areaId)
{
    switch (LSHeader->linkStateType)
    {
        case OSPFv3_ROUTER:
        {
            Ospfv3ScheduleRouterLSA(node, areaId, FALSE);

            break;
        }
        case OSPFv3_NETWORK:
        {
            Ospfv3ScheduleNetworkLSA(node, areaId, FALSE);

            break;
        }
        default :
        {
            ERROR_Assert(FALSE, "Unknown LS type not supported\n");
        }

    }
}

// /**
// FUNCTION   :: Ospfv3RefreshLSA
// LAYER      :: NETWORK
// PURPOSE    :: Refresh LSA after refreshing interval.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSAItem:  ListItem* : LSA list Item.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3RefreshLSA(Node* node, ListItem* LSAItem, unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSAItem->data;

    if (LSHeader->linkStateSequenceNumber
        == (signed int) OSPFv3_MAX_SEQUENCE_NUMBER)
    {
        // Sequence number reaches the maximum value. We need to
        // flush this instance first before originating any instance.
        Ospfv3FlushLSA(node, (char* ) LSHeader, areaId);

        Ospfv3ScheduleLSA(node, LSHeader, areaId);

        Ospfv3RemoveLSAFromLSDB(node, (char* ) LSHeader, areaId);
    }
    else
    {
        LSHeader->linkStateSequenceNumber += 1;
        LSHeader->linkStateAge = 0;
        LSAItem->timeStamp = node->getNodeTime();

#ifdef OSPFv3_DEBUG_LSDBErr
        {
            printf("    Node %u: Refreshing LSA. LSType = %d\n",
                node->nodeId,
                LSHeader->linkStateType);
        }
#endif

        // Flood LSA
        if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
        {
            Ospfv3FloodThroughAS(node, (char* ) LSHeader, ANY_DEST);
        }
        else if (LSHeader->linkStateType == OSPFv3_LINK)
        {
            unsigned int interfaceId = areaId;

            Ospfv3FloodLSAOnInterface(
                node,
                (char* )LSHeader,
                ANY_DEST,
                interfaceId);
        }
        else
        {
            Ospfv3FloodLSAThroughArea(
                node,
                (char* ) LSHeader,
                ANY_DEST,
                areaId);
        }
        ospf->stats.numLSARefreshed++;
    }
}

// /**
// FUNCTION   :: Ospfv3IncrementLSAgeInLSAList
// LAYER      :: NETWORK
// PURPOSE    :: Increment the link state age field of LSAs stored in the
//               LSDB and handle appropriately if this age field passes
//               a maximum age limit.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list:  LinkedList* : LSA list.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3IncrementLSAgeInLSAList(
    Node* node,
    LinkedList* list,
    unsigned int areaId)
{
    ListItem* item = list->first;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    while (item)
    {
        short tempAge;
        Ospfv3LinkStateHeader* LSHeader =
            (Ospfv3LinkStateHeader* ) item->data;

#ifdef OSPFv3_DEBUG_LSDBErr
        {
            printf("    LSA for node %u age was %d seconds type %d\n",
                LSHeader->advertisingRouter,
                LSHeader->linkStateAge,
                LSHeader->linkStateType);
        }
#endif

        tempAge =
            (short) (LSHeader->linkStateAge
                        + (OSPFv3_LSA_INCREMENT_AGE_INTERVAL / SECOND));

        if (tempAge > (OSPFv3_LSA_MAX_AGE / SECOND))
        {
            tempAge = OSPFv3_LSA_MAX_AGE / SECOND;
        }

        // Increment LS age.
        LSHeader->linkStateAge = tempAge;

#ifdef OSPFv3_DEBUG_LSDBErr
        {
            printf("    LSA for node %u now has age %d seconds type 0x%x\n",
                LSHeader->advertisingRouter,
                LSHeader->linkStateAge,
                LSHeader->linkStateType);
        }
#endif

        // LS Age field of Self originated LSA reaches LSRefreshTime
        if ((LSHeader->advertisingRouter == ospf->routerId)
            && (tempAge == (OSPFv3_LS_REFRESH_TIME / SECOND)))
        {
            ListItem* tempItem;

            tempItem = item;
            item = item->next;

            switch (LSHeader->linkStateType)
            {
                case OSPFv3_ROUTER:
                case OSPFv3_NETWORK:
                case OSPFv3_INTER_AREA_PREFIX:
                case OSPFv3_INTER_AREA_ROUTER:
                case OSPFv3_AS_EXTERNAL:
                case OSPFv3_LINK:
                case OSPFv3_INTRA_AREA_PREFIX:
                {
                    Ospfv3RefreshLSA(node, tempItem, areaId);

                    break;
                }
                default :
                {
                    ERROR_Assert(FALSE, "\n Unknown LSA\n");
                }
            }
        }

        // Expired, so remove from LSDB and flood.
        else if (tempAge == (OSPFv3_LSA_MAX_AGE / SECOND))
        {
            ListItem* deleteItem = item;
            item = item->next;

#ifdef OSPFv3_DEBUG_LSDBErr
            {
                printf("    LSA for node %u deleted from LSDB\n",
                    LSHeader->advertisingRouter);
            }
#endif

            ospf->stats.numExpiredLSAge++;

            if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
            {
                Ospfv3FloodThroughAS(node, (char* ) LSHeader, ANY_DEST);
            }
            else if (LSHeader->linkStateType == OSPFv3_LINK)
            {

                unsigned int interfaceId = areaId;

                Ospfv3FloodLSAOnInterface(
                    node,
                    (char* ) LSHeader,
                    ANY_DEST,
                    interfaceId);
            }
            else
            {

                Ospfv3FloodLSAThroughArea(
                    node,
                    (char* ) LSHeader,
                    ANY_DEST,
                    areaId);
            }

            ListGet(node, list, deleteItem, TRUE, FALSE);

            // Need to recalculate shortest path since topology changed.
            Ospfv3ScheduleSPFCalculation(node);
        }
        else
        {
            item = item->next;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3IncrementLSAge
// LAYER      :: NETWORK
// PURPOSE    :: Increment the link state age of all the LSA in Database.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3IncrementLSAge(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    ListItem* listItem;
    int i;

    // Increment Age of link local LSA
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }
        Ospfv3IncrementLSAgeInLSAList(
            node,
            ospf->pInterface[i].linkLSAList,
            i);
    }

    // Increment Age of LSA having Area flooding scope
    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

        Ospfv3IncrementLSAgeInLSAList(
            node,
            thisArea->networkLSAList,
            thisArea->areaId);

        Ospfv3IncrementLSAgeInLSAList(
            node,
            thisArea->routerLSAList,
            thisArea->areaId);

        Ospfv3IncrementLSAgeInLSAList(
            node,
            thisArea->interAreaRouterLSAList,
            thisArea->areaId);

        Ospfv3IncrementLSAgeInLSAList(
            node,
            thisArea->interAreaPrefixLSAList,
            thisArea->areaId);

        Ospfv3IncrementLSAgeInLSAList(
            node,
            thisArea->intraAreaPrefixLSAList,
            thisArea->areaId);

    }

    // BGP-OSPF Patch Start
    Ospfv3IncrementLSAgeInLSAList(
        node,
        ospf->asExternalLSAList,
        OSPFv3_INVALID_AREA_ID);
    // BGP-OSPF Patch End
}

// /**
// FUNCTION   :: Ospfv3InvalidateRoutingTable
// LAYER      :: NETWORK
// PURPOSE    :: Invalidate old Routing Table.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3InvalidateRoutingTable(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    int i;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&ospf->routingTable.buffer);

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {

        // BGP-OSPF Patch Start
        if (ospf->asBoundaryRouter
            && (rowPtr[i].pathType == OSPFv3_TYPE2_EXTERNAL))
        {
            continue;
        }
        // BGP-OSPF Patch End

        rowPtr[i].flag = OSPFv3_ROUTE_INVALID;
    }
}

// /**
// FUNCTION   :: Ospfv3LSAHasMaxAge
// LAYER      :: NETWORK
// PURPOSE    :: Check if LSA has Maximum age.
// PARAMETERS ::
// +LSA:  char* : Pointer to LSA.
// RETURN     :: TRUE if Maximum age, FALSE otherwise.
// **/
static
BOOL Ospfv3LSAHasMaxAge(char* LSA)
{
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    if (LSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3LSAHasLink
// LAYER      :: NETWORK
// PURPOSE    :: Check if the SPF tree calculating vertex has back link with
//               other vertex.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +wLSA:  char* : Pointer to LSA.
// +vLSA:  char* : Pointer to LSA.
// RETURN     :: void : NULL.
// **/
static
BOOL Ospfv3LSAHasLink(Node* node, char* wLSA, char* vLSA)
{
    Ospfv3LinkStateHeader* wLSHeader = (Ospfv3LinkStateHeader* ) wLSA;
    Ospfv3LinkStateHeader* vLSHeader = (Ospfv3LinkStateHeader* ) vLSA;

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    Checking for common link\n");

        printf("    Child LSA:\n");

        Ospfv3PrintLSA(wLSA);

        printf("    Parent LSA:\n");

        Ospfv3PrintLSA(vLSA);
    }
#endif

    if (wLSHeader->linkStateType == OSPFv3_ROUTER)
    {
        short numLinks =
            (wLSHeader->length - sizeof(Ospfv3RouterLSA))
            / sizeof(Ospfv3LinkInfo);

        short i;

        Ospfv3LinkInfo* link =
            (Ospfv3LinkInfo* ) (wLSA + sizeof(Ospfv3RouterLSA));

        for (i = 0; i < numLinks; i++)
        {
            if (link->type == OSPFv3_POINT_TO_POINT)
            {
                if ((vLSHeader->linkStateType == OSPFv3_ROUTER)
                    && (link->neighborRouterId
                    == vLSHeader->advertisingRouter))
                {
                    return TRUE;
                }
            }
            else if (link->type == OSPFv3_TRANSIT)
            {
                if ((vLSHeader->linkStateType == OSPFv3_NETWORK)
                    && (link->neighborRouterId
                    == vLSHeader->advertisingRouter))
                {
                    return TRUE;
                }
            }
            link = (Ospfv3LinkInfo* ) (link + 1);
        }
    }
    else if (wLSHeader->linkStateType == OSPFv3_NETWORK)
    {
        short numLink =
            (short) ((wLSHeader->length - sizeof(Ospfv3NetworkLSA))
            / sizeof(unsigned int));

        short i;
        unsigned int* attachedRouter =
            (unsigned int* ) (wLSA + sizeof(Ospfv3NetworkLSA));

        if (vLSHeader->linkStateType == OSPFv3_NETWORK)
        {
            return FALSE;
        }

        for (i = 0; i < numLink; i++)
        {
            if (attachedRouter[i] == vLSHeader->advertisingRouter)
            {
                return TRUE;
            }
        }
    }

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    LSAs do not have common link\n");
    }
#endif

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3InShortestPathList
// LAYER      :: NETWORK
// PURPOSE    :: Check if the vertex is in Shortest Path List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +shortestPathList:  LinkedList* : Pointer to Shortest Path List.
// +vertexType:  Ospfv3VertexType : vertex type.
// +vertexId:  Ospfv3VertexId : Vertex Id.
// RETURN     :: void : NULL.
// **/
static
BOOL Ospfv3InShortestPathList(
    Node* node,
    LinkedList* shortestPathList,
    Ospfv3VertexType vertexType,
    Ospfv3VertexId vertexId)
{
    ListItem* listItem  = shortestPathList->first;
    Ospfv3Vertex* listVertex = NULL;

    // Search through the shortest path list for the node.
    while (listItem)
    {
        listVertex = (Ospfv3Vertex* ) listItem->data;

        // Found it.
        if ((listVertex->vertexType == vertexType)
            && (listVertex->vertexType == OSPFv3_VERTEX_ROUTER))
        {
            if (listVertex->vertexId.routerVertexId ==
                                     vertexId.routerVertexId)
            {

#ifdef OSPFv3_DEBUG_ERRORS
                {
                    printf("   Node %d: Vertex %d already"
                            "in shortest path list\n"
                            "   \tVertex Type        = %d\n",
                        node->nodeId,
                        vertexId.routerVertexId,
                        vertexType);
                }
#endif

                return TRUE;
            }
        }

        if ((listVertex->vertexType == vertexType)
            && (listVertex->vertexType == OSPFv3_VERTEX_NETWORK))
        {
            if ((listVertex->vertexId.networkVertexId.interfaceIdOfDR
                == vertexId.networkVertexId.interfaceIdOfDR)
                && (listVertex->vertexId.networkVertexId.routerIdOfDR
                == vertexId.networkVertexId.routerIdOfDR))
            {

#ifdef OSPFv3_DEBUG_ERRORS
                {
                    printf("   Node %d: Vertex %d already"
                            "in shortest path list\n"
                            "   \tVertex Type        = %d\n",
                        node->nodeId,
                        vertexId.routerVertexId,
                        vertexType);
                }
#endif

                return TRUE;
            }
        }
        listItem = listItem->next;
    }

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    Vertex = 0x%x of type %d not in shortest path list\n",
            node->nodeId,
            vertexType);
    }
#endif

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3GetInterfaceForThisVertex
// LAYER      :: NETWORK
// PURPOSE    :: Get interface for connected vertex.
// PARAMETERS ::
// +vertex:  Ospfv3Vertex* : child vertex.
// +parent:  Ospfv3Vertex* : Parent vertex.
// RETURN     :: Interface Index.
// **/
static
unsigned int Ospfv3GetInterfaceForThisVertex(
            Ospfv3Vertex* vertex,
            Ospfv3Vertex* parent)
{
    int i = 0;
    char assertStr[MAX_STRING_LENGTH];
    Ospfv3RouterLSA* rtrLSA = (Ospfv3RouterLSA* ) parent->LSA;
    Ospfv3LinkInfo* linkList = (Ospfv3LinkInfo* ) (rtrLSA + 1);

    int numLinks =
        rtrLSA->LSHeader.length - sizeof(Ospfv3RouterLSA)
        / sizeof(Ospfv3LinkInfo);

    for (i = 0; i < numLinks; i++)
    {
        if (linkList->neighborRouterId == vertex->vertexId.routerVertexId)
        {
            return linkList->interfaceId;
        }
        linkList = linkList + 1;
    }

    // Shouldn't get here
    sprintf(assertStr,
            "Not get link data from the associated link "
            "for this vertex 0x%x\n", vertex->vertexId.routerVertexId);

    ERROR_Assert(FALSE, assertStr);

    return ANY_DEST;
}

// /**
// FUNCTION   :: Ospfv3CheckRouterLSASetThisNetworkTransit
// LAYER      :: NETWORK
// PURPOSE    :: Check Router LSA and Check the Network is set Transit.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +routerLSA:  Ospfv3RouterLSA* : Pointer to Router LSA.
// +vertexId:  Ospfv3VertexId : Vertex Id.
// RETURN     :: True the Network is set Transit, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckRouterLSASetThisNetworkTransit(
    Node* node,
    Ospfv3RouterLSA* routerLSA,
    Ospfv3VertexId vertexId)
{
    int j = 0;
    int numLinks = (routerLSA->LSHeader.length - sizeof(Ospfv3RouterLSA))
                    / sizeof(Ospfv3LinkInfo);

    Ospfv3LinkInfo* linkList = (Ospfv3LinkInfo* ) (routerLSA + 1);

    for (j = 0; j < numLinks; j++)
    {
        if ((linkList->neighborInterfaceId
            == vertexId.networkVertexId.interfaceIdOfDR)
            && (linkList->neighborRouterId
            == vertexId.networkVertexId.routerIdOfDR)
            && linkList->type == OSPFv3_TRANSIT)
        {
            return TRUE;
        }

        linkList = linkList + 1;
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3HaveLinkWithNetworkVertex
// LAYER      :: NETWORK
// PURPOSE    :: Check Link with Network Vertex.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +vertexId:  Ospfv3VertexId : Vertex Id.
// RETURN     :: TRUE if has link with vertex, FALSE otherwise.
// **/
static
BOOL Ospfv3HaveLinkWithNetworkVertex(
    Node* node,
    Ospfv3VertexId vertexId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        Ospfv3Area* thisArea = Ospfv3GetArea(
                                node,
                                ospf->pInterface[i].areaId);

        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE
            || thisArea == NULL)
        {
            continue;
        }

        Ospfv3RouterLSA* routerLSA =
            (Ospfv3RouterLSA* )Ospfv3LookupLSAList(
                                    node,
                                    thisArea->routerLSAList,
                                    ospf->routerId,
                                    ospf->routerId);



        if (Ospfv3CheckRouterLSASetThisNetworkTransit(
                node,
                routerLSA,
                vertexId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3GetNbrInfoOnInterface
// LAYER      :: NETWORK
// PURPOSE    :: Return Neighbor Info on particular interface.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +nbrId:  unsigned int : Neighbor Id.
// +interfaceId:  int : Interface Id.
// RETURN     :: Neighbor Info.
// **/
static
Ospfv3Neighbor* Ospfv3GetNbrInfoOnInterface(
    Node* node,
    unsigned int nbrId,
    int interfaceId)
{
    ListItem* listItem = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);


    listItem = ospf->pInterface[interfaceId].neighborList->first;

    // Search for the neighbor in my neighbor list.
    while (listItem)
    {
        nbrInfo = (Ospfv3Neighbor* ) listItem->data;

        ERROR_Assert(nbrInfo, "Neighbor not found in the Neighbor list");

        if (nbrInfo->neighborId == nbrId)
        {
            // Found the neighbor we are looking for.
            return nbrInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}

// /**
// FUNCTION   :: Ospfv3SetNextHopForThisVertex
// LAYER      :: NETWORK
// PURPOSE    :: Set Next Hop For The Vertex.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +vertex:  Ospfv3Vertex* : Pointer to Child Vertex.
// +parent:  Ospfv3Vertex* : Pointer to Parent Vertex.
// RETURN     :: TRUE on success, FALSE otherwise
// **/
static
BOOL Ospfv3SetNextHopForThisVertex(
    Node* node,
    Ospfv3Vertex* vertex,
    Ospfv3Vertex* parent)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    Ospfv3NextHopListItem* nextHopItem = NULL;

    // If parent is root, vertex is either directly connected router
    // or network
    if ((parent->vertexType == OSPFv3_VERTEX_ROUTER)
        && (parent->vertexId.routerVertexId == ospf->routerId))
    {
        unsigned int interfaceId = ANY_DEST;

        interfaceId = Ospfv3GetInterfaceForThisVertex(vertex, parent);

        if (ospf->pInterface[interfaceId].state == S_Down)
        {
            return FALSE;
        }

        nextHopItem = (Ospfv3NextHopListItem* )
                       MEM_malloc(sizeof(Ospfv3NextHopListItem));

        nextHopItem->outIntfId = interfaceId;



        // Ideally this field is not required for an directly connected
        // network or router which is connected via a point-to-point link.
        // We doesn't considering the case of router connecting through a
        // poin-to-multipoint network. Still adding this field for wireless
        // network.
        if (vertex->vertexType == OSPFv3_VERTEX_ROUTER)
        {
            Ospfv3Neighbor* nbrInfo = Ospfv3GetNbrInfoOnInterface(node,
                                            vertex->vertexId.routerVertexId,
                                            interfaceId);

            if (nbrInfo == NULL)
            {
                // Neighbor probably removed
                MEM_free(nextHopItem);
                return FALSE;
            }

            COPY_ADDR6(nbrInfo->neighborIPAddress, nextHopItem->nextHop);
        }
        else
        {
            // Vertex is directly connected network
            memset(&nextHopItem->nextHop, 0, sizeof(in6_addr));
        }

        ListInsert(node,
            vertex->nextHopList,
            0,
            (void* ) nextHopItem);
    }
    // Else if parent is network that directly connects with root
    else if ((parent->vertexType == OSPFv3_VERTEX_NETWORK) &&
        (Ospfv3HaveLinkWithNetworkVertex(node, parent->vertexId)))
    {
        unsigned int interfaceId = ANY_DEST;
        Ospfv3Neighbor* neighborInfo = NULL;

        nextHopItem = (Ospfv3NextHopListItem* )
                       MEM_malloc(sizeof(Ospfv3NextHopListItem));

        interfaceId = Ospfv3GetInterfaceForThisNeighbor(node,
                                vertex->vertexId.routerVertexId);

        neighborInfo = Ospfv3GetNeighborInfoByRtrId(node,
                                vertex->vertexId.routerVertexId);

        if (neighborInfo == NULL)
        {
            return FALSE;
        }

        if (ospf->pInterface[interfaceId].state == S_Down)
        {
            MEM_free(nextHopItem);
            return FALSE;
        }

        COPY_ADDR6(neighborInfo->neighborIPAddress, nextHopItem->nextHop);

        nextHopItem->outIntfId = interfaceId;

        ListInsert(node,
            vertex->nextHopList,
            0,
            (void* ) nextHopItem);
    }
    else
    {
        // There should be an intervening router. So inherits the set
        // of next hops from parent
        ListItem* listItem = parent->nextHopList->first;

        while (listItem)
        {
            nextHopItem = (Ospfv3NextHopListItem* )
                           MEM_malloc(sizeof(Ospfv3NextHopListItem));

            memcpy(nextHopItem,
                listItem->data,
                sizeof(Ospfv3NextHopListItem));
            ListInsert(node,
                vertex->nextHopList,
                0,
                (void* ) nextHopItem);

            listItem = listItem->next;
        }
    }
    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3FindCandidate
// LAYER      :: NETWORK
// PURPOSE    :: Find Candidate in candidate list.
// PARAMETERS ::
// +candidateList:  LinkedList* : Pointer to candidate List.
// +vertexType:  Ospfv3VertexType : Ospfv3 Vertex Type.
// +vertexId:  Ospfv3VertexId : Ospfv3 Vertex Id.
// RETURN     :: Pointer to Vertex.
// **/
static
Ospfv3Vertex* Ospfv3FindCandidate(
    LinkedList* candidateList,
    Ospfv3VertexType vertexType,
    Ospfv3VertexId vertexId)
{
    ListItem* tempItem = candidateList->first;

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("        Vertex ID = 0x%x\n", vertexId);
        printf("        Vertex Type = %d\n", vertexType);
    }
#endif

    // Search entire candidate list
    while (tempItem)
    {
        Ospfv3Vertex* tempEntry = (Ospfv3Vertex* ) tempItem->data;

#ifdef OSPFv3_DEBUG_SPTErr
        {
            printf("        tempEntry->vertexId = 0x%x\n",
                    tempEntry->vertexId);
            printf("        tempEntry->vertexType = %d\n",
                    tempEntry->vertexType);
            printf("        tempEntry->distance = %d\n",
                    tempEntry->distance);
        }
#endif

        // Candidate found.
        if ((tempEntry->vertexType == vertexType)
            && (tempEntry->vertexType == OSPFv3_VERTEX_ROUTER))
        {
            if ((tempEntry->vertexId.routerVertexId ==
                                        vertexId.routerVertexId))
            return tempEntry;
        }
        if ((tempEntry->vertexType == vertexType)
            && (tempEntry->vertexType == OSPFv3_VERTEX_NETWORK))
        {
            if ((tempEntry->vertexId.networkVertexId.interfaceIdOfDR ==
                        vertexId.networkVertexId.interfaceIdOfDR)
                && (tempEntry->vertexId.networkVertexId.routerIdOfDR ==
                        vertexId.networkVertexId.routerIdOfDR))
            return tempEntry;
        }
        tempItem = tempItem->next;
    }
    return NULL;
}


// /**
// FUNCTION   :: Ospfv3UpdateCandidateListUsingRouterLSA
// LAYER      :: NETWORK
// PURPOSE    :: Update Candidate List Using Router LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +candidateList:  LinkedList* : Pointer to candidate List.
// +v:  Ospfv3Vertex* : Pointer to candidate List.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateCandidateListUsingRouterLSA(
    Node* node,
    Ospfv3Area* thisArea,
    LinkedList* candidateList,
    Ospfv3Vertex* v)
{

    Ospfv3RouterLSA* vLSA = (Ospfv3RouterLSA* ) v->LSA;
    char* wLSA = NULL;
    Ospfv3LinkInfo* linkList = NULL;
    Ospfv3LinkInfo* nextLink = NULL;
    Int32 numLinks;
    Int32 i;

    nextLink = (Ospfv3LinkInfo* ) (vLSA + 1);
    numLinks = (vLSA->LSHeader.length - sizeof(Ospfv3RouterLSA))
                    / sizeof(Ospfv3LinkInfo);


    // Examine vertex v's links.
    for (i = 0; i < numLinks; i++)
    {
        Ospfv3Vertex* candidateListItem = NULL;
        Ospfv3VertexId newVertexId;
        Ospfv3VertexType newVertexType = OSPFv3_VERTEX_ROUTER;
        UInt32 newVertexDistance;
        Ospfv3LinkStateHeader* wLSHeader = NULL;

        linkList = nextLink;

        nextLink = (Ospfv3LinkInfo* ) (nextLink + 1);
        memset(&newVertexId, 0, sizeof(Ospfv3VertexId));

#ifdef OSPFv3_DEBUG_SPTErr
        {
            printf("\n    Examining link %u, node %d  linkType = %d\n",
                    linkList->interfaceId,
                    node->nodeId,
                    linkList->type);
        }
#endif

        if (linkList->type == OSPFv3_POINT_TO_POINT)
        {
            wLSA = (char* ) Ospfv3LookupLSAList(
                                node,
                                thisArea->routerLSAList,
                                linkList->neighborRouterId,
                                linkList->neighborRouterId);
            if (wLSA)
            {
                wLSHeader = (Ospfv3LinkStateHeader* ) wLSA;
                newVertexType = OSPFv3_VERTEX_ROUTER;
                newVertexId.routerVertexId = wLSHeader->advertisingRouter;
            }

        }
        else if (linkList->type == OSPFv3_TRANSIT)
        {
            wLSA = (char* ) Ospfv3LookupLSAList(
                                node,
                                thisArea->networkLSAList,
                                linkList->neighborRouterId,
                                linkList->neighborInterfaceId);

            newVertexId.networkVertexId.routerIdOfDR =
                linkList->neighborRouterId;

            newVertexId.networkVertexId.interfaceIdOfDR =
                linkList->neighborInterfaceId;

            newVertexType = OSPFv3_VERTEX_NETWORK;
        }
        else
        {
            // Virtual link is not considered yet
            continue;
        }

        // RFC2328, Sec-16.1 (2.b & 2.c)
        if ((wLSA == NULL) || (Ospfv3LSAHasMaxAge(wLSA))
            || (!Ospfv3LSAHasLink(node, wLSA, v->LSA))
            || (Ospfv3InShortestPathList(
                    node,
                    thisArea->shortestPathList,
                    newVertexType,
                    newVertexId)))
        {
            continue;
        }


        wLSHeader = (Ospfv3LinkStateHeader* ) wLSA;


        newVertexDistance = v->distance + linkList->metric;

        candidateListItem = Ospfv3FindCandidate(
                                candidateList,
                                newVertexType,
                                newVertexId);

        if (candidateListItem == NULL)
        {
            // Insert new candidate
            candidateListItem =
                (Ospfv3Vertex* ) MEM_malloc(sizeof(Ospfv3Vertex));

            memcpy(
                &candidateListItem->vertexId,
                &newVertexId,
                sizeof(Ospfv3VertexId));

            candidateListItem->vertexType = newVertexType;
            candidateListItem->LSA = wLSA;
            candidateListItem->distance = newVertexDistance;

            ListInit(node, &candidateListItem->nextHopList);

            if (Ospfv3SetNextHopForThisVertex(node, candidateListItem, v))
            {

#ifdef OSPFv3_DEBUG_SPTErr
                {
                    printf("    Inserting new vertex 0x%x\n", newVertexId);
                }
#endif

                ListInsert(
                    node,
                    candidateList,
                    0,
                    (void* ) candidateListItem);
            }
            else
            {
                ListFree(node, candidateListItem->nextHopList, FALSE);

                MEM_free(candidateListItem);
            }
        }
        else if (candidateListItem->distance > newVertexDistance)
        {
            // update
            candidateListItem->distance = newVertexDistance;

#ifdef OSPFv3_DEBUG_SPTErr
            {
                printf("    Update distance and next hop for vertex 0x%x\n",
                        newVertexId);
            }
#endif

            // Free old next hop items
            ListFree(node, candidateListItem->nextHopList, FALSE);

            ListInit(node, &candidateListItem->nextHopList);

            Ospfv3SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else if (candidateListItem->distance == newVertexDistance)
        {

#ifdef OSPFv3_DEBUG_SPTErr
            {
                printf("    Add new set of next hop for vertex 0x%x\n",
                        newVertexId);
            }
#endif

            // Add new set of next hop values
            Ospfv3SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else
        {
            // Examine next link
            continue;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3IsTransitWithNetworkVertex
// LAYER      :: NETWORK
// PURPOSE    :: Check if network vertex is transit.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +candidateList:  LinkedList* : Pointer to candidate List.
// +vertexId:  Ospfv3VertexId : Vertex Id.
// RETURN     :: TRUE if node have a Transit link. FALSE otherwise.
// **/
static
BOOL Ospfv3IsTransitWithNetworkVertex(
    Node* node,
    Ospfv3Area* thisArea,
    Ospfv3VertexId vertexId)
{
    int i = 0;
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    Ospfv3RouterLSA* ownRouterLSA = NULL;

    ownRouterLSA = (Ospfv3RouterLSA* ) Ospfv3LookupLSAList(
                                            node,
                                            thisArea->routerLSAList,
                                            ospf->routerId,
                                            ospf->routerId);

    if (ownRouterLSA == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }
        if ((ospf->pInterface[i].state != S_Down)
            && Ospfv3CheckRouterLSASetThisNetworkTransit(
                    node,
                    ownRouterLSA,
                    vertexId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3UpdateCandidateListUsingNetworkLSA.
// LAYER      :: NETWORK
// PURPOSE    :: Update Candidate List Using Network LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +candidateList:  LinkedList* : Pointer to candidate List.
// +v:  Ospfv3Vertex* : Pointer to Vertex.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateCandidateListUsingNetworkLSA(
    Node* node,
    Ospfv3Area* thisArea,
    LinkedList* candidateList,
    Ospfv3Vertex* v)
{
    Ospfv3NetworkLSA* vLSA = NULL;
    unsigned int* attachedRouter = NULL;
    int numNeighbor = 0;
    char* wLSA = NULL;
    int i = 0;

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    Looking for vertex from network LSA of vertex 0x%x\n",
                v->vertexId);
    }
#endif

    vLSA = (Ospfv3NetworkLSA* ) v->LSA;
    attachedRouter = (unsigned int* ) (vLSA + 1);

    numNeighbor = (vLSA->LSHeader.length - sizeof(Ospfv3NetworkLSA))
                  / (sizeof(unsigned int));

    for (i = 0; i < numNeighbor; i++)
    {
        Ospfv3Vertex* candidateListItem = NULL;
        Ospfv3VertexId newVertexId;
        Ospfv3VertexType newVertexType;
        unsigned int newVertexDistance;
        Ospfv3LinkStateHeader* wLSHeader = NULL;

#ifdef OSPFv3_DEBUG_SPTErr
        {
            printf("        examining router 0x%x\n", attachedRouter[i]);
        }
#endif

        wLSA = (char* ) Ospfv3LookupLSAList(
                            node,
                            thisArea->routerLSAList,
                            attachedRouter[i],
                            attachedRouter[i]);

        if (wLSA == NULL)
        {
            continue;
        }

        wLSHeader = (Ospfv3LinkStateHeader* )wLSA;

        newVertexId.routerVertexId = wLSHeader->advertisingRouter;

        // RFC2328, Sec-16.1 (2.b & 2.c)
        if ((wLSA == NULL) || (Ospfv3LSAHasMaxAge(wLSA))
            || (!Ospfv3LSAHasLink(node, wLSA, v->LSA))
            || (Ospfv3InShortestPathList(
                    node,
                    thisArea->shortestPathList,
                    OSPFv3_VERTEX_ROUTER,
                    newVertexId)))
        {
            continue;
        }

        // If it's directly connected network.
        if (Ospfv3IsTransitWithNetworkVertex(node, thisArea, v->vertexId))
        {
            // Consider only routers that are directly reachable
            int interfaceId = ANY_DEST;
            Ospfv3Neighbor* nbrInfo = NULL;
            Ospfv3Vertex* root =
                (Ospfv3Vertex* ) thisArea->shortestPathList->first->data;

            interfaceId = Ospfv3GetInterfaceForThisVertex(v, root);

            nbrInfo = Ospfv3GetNbrInfoOnInterface(
                        node,
                        attachedRouter[i],
                        interfaceId);

            if (!nbrInfo)
            {
                continue;
            }
        }

        wLSHeader = (Ospfv3LinkStateHeader* ) wLSA;

        newVertexType = OSPFv3_VERTEX_ROUTER;
        newVertexDistance = v->distance;

        candidateListItem = Ospfv3FindCandidate(
                                candidateList,
                                newVertexType,
                                newVertexId);

        if (candidateListItem == NULL)
        {
            // Insert new candidate
            candidateListItem =
                (Ospfv3Vertex* ) MEM_malloc(sizeof(Ospfv3Vertex));

            candidateListItem->vertexId = newVertexId;
            candidateListItem->vertexType = newVertexType;
            candidateListItem->LSA = wLSA;
            candidateListItem->distance = newVertexDistance;

            ListInit(node, &candidateListItem->nextHopList);

            if (Ospfv3SetNextHopForThisVertex(node, candidateListItem, v))
            {

#ifdef OSPFv3_DEBUG_SPTErr
                {
                    printf("    Inserting new vertex 0x%x\n", newVertexId);
                }
#endif

                ListInsert(
                    node,
                    candidateList,
                    0,
                    (void* ) candidateListItem);
            }
            else
            {
                ListFree(
                    node,
                    candidateListItem->nextHopList,
                    FALSE);

                MEM_free(candidateListItem);
            }
        }
        else if (candidateListItem->distance > newVertexDistance)
        {
            candidateListItem->distance = newVertexDistance;

#ifdef OSPFv3_DEBUG_SPTErr
            {
                printf("    Update distance and next hop for vertex 0x%x\n",
                        newVertexId);
            }
#endif

            ListFree(node,
                candidateListItem->nextHopList,
                FALSE);

            Ospfv3SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else if (candidateListItem->distance == newVertexDistance)
        {

#ifdef OSPFv3_DEBUG_SPTErr
            {
                printf("    Add new set of next hop for vertex 0x%x\n",
                        newVertexId);
            }
#endif

            // Add new set of next hop values
            Ospfv3SetNextHopForThisVertex(node, candidateListItem, v);
        }
        else
        {
            // Examine next link
            continue;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateCandidateList.
// LAYER      :: NETWORK
// PURPOSE    :: Update Candidate List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +candidateList:  LinkedList* : Pointer to candidate List.
// +v:  Ospfv3Vertex* : Pointer to Vertex.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateCandidateList(
    Node* node,
    Ospfv3Area* thisArea,
    LinkedList* candidateList,
    Ospfv3Vertex* v)
{
    if (v->vertexType == OSPFv3_VERTEX_NETWORK)
    {
        Ospfv3UpdateCandidateListUsingNetworkLSA(
            node,
            thisArea,
            candidateList,
            v);
    }
    else
    {
        Ospfv3UpdateCandidateListUsingRouterLSA(
            node,
            thisArea,
            candidateList,
            v);
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateShortestPathList.
// LAYER      :: NETWORK
// PURPOSE    :: Update Shortest Path List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +candidateList:  LinkedList* : Pointer to candidate List.
// RETURN     :: Pointer to Vertex.
// **/
static
Ospfv3Vertex* Ospfv3UpdateShortestPathList(
    Node* node,
    Ospfv3Area* thisArea,
    LinkedList* candidateList)
{
    ListItem* candidateListItem = NULL;
    ListItem* listItem = NULL;
    Ospfv3Vertex* closestCandidate = NULL;
    Ospfv3Vertex* listEntry = NULL;

    Ospfv3Vertex* shortestPathListItem =
        (Ospfv3Vertex* ) MEM_malloc(sizeof(Ospfv3Vertex));

    unsigned int lowestMetric = OSPFv3_LS_INFINITY;

    listItem = candidateList->first;

    // Get the vertex with the smallest metric from the candidate list...
    while (listItem)
    {
        listEntry = (Ospfv3Vertex* ) listItem->data;

        if (listEntry->distance < lowestMetric)
        {
            lowestMetric = listEntry->distance;

            // Keep track of closest vertex
            candidateListItem = listItem;
            closestCandidate = listEntry;
        }
        // Network vertex get preference over router vertex
        else if ((listEntry->distance == lowestMetric)
                && (candidateListItem)
                && (closestCandidate->vertexType == OSPFv3_VERTEX_ROUTER)
                && (listEntry->vertexType == OSPFv3_VERTEX_NETWORK))
        {
            // Keep track of closest vertex
            candidateListItem = listItem;
            closestCandidate = listEntry;
        }

        listItem = listItem->next;
    }

    ERROR_Assert(candidateListItem,
                "Candidate not found in the candidate list.\n");

    memcpy(shortestPathListItem, closestCandidate, sizeof(Ospfv3Vertex));

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("Node %d added following Vertex to shortest path list\n",
                node->nodeId);

        printf("   \tvertexId              = %d\n"
                "   \tvertexType            = %u\n",
            shortestPathListItem->vertexId.routerVertexId,
            shortestPathListItem->vertexType);

        printf("    \tmetric                = %d\n",
            shortestPathListItem->distance);
    }
#endif

    // Now insert it into the shortest path list...
    ListInsert(
        node,
        thisArea->shortestPathList,
        0,
        (void* ) shortestPathListItem);

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    removing Vertex 0x%x from candidate list\n",
            closestCandidate->vertexId);

        printf("    size was %d\n", candidateList->size);
    }
#endif

    // And remove it from the candidate list since no longer available.
    ListGet(node, candidateList, candidateListItem, TRUE, FALSE);

#ifdef OSPFv3_DEBUG_SPTErr
    {
        printf("    Candidate List size now %d\n", candidateList->size);

        Ospfv3PrintShortestPathList(node, thisArea->shortestPathList);
    }
#endif

    return shortestPathListItem;
}

// /**
// FUNCTION   :: Ospfv3FreeVertexList.
// LAYER      :: NETWORK
// PURPOSE    :: To Free Vertex List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +vertexList:  LinkedList* : Pointer to vertex List.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FreeVertexList(Node* node, LinkedList* vertexList)
{
    ListItem* listItem = vertexList->first;

    while (listItem)
    {
        Ospfv3Vertex* vertex = (Ospfv3Vertex* ) listItem->data;

        ListFree(node, vertex->nextHopList, FALSE);

        listItem = listItem->next;
    }

    ListFree(node, vertexList, FALSE);
}

// /**
// FUNCTION   :: Ospfv3GetValidRoute
// LAYER      :: NETWORK
// PURPOSE    :: Get desired route from the OSPFv3 routing table.
// PARAMETERS ::
// + node       :  Node*                        : Pointer to node, doing the
//                                                packet trace
// + LSA        :  char*                        : Poiter to an LSA.
// + neighborId :  unsigned int                 : Neighbor ID
// + areaId     :  unsigned int                 : Area Id to which LSA need
//                                                to be flooded.
// RETURN     :: Route entry. NULL if route not found.
// **/
Ospfv3RoutingTableRow* Ospfv3GetValidRoute(
    Node* node,
    in6_addr destAddr,
    Ospfv3DestType destType)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;
    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        in6_addr tablePrefix;

        Ipv6GetPrefix(
            &rowPtr[i].destAddr,
            &tablePrefix,
            rowPtr[i].prefixLength);

        if ((CMP_ADDR6(tablePrefix, destAddr) == 0)
            && Ospfv3CompDestType(rowPtr[i].destType, destType)
            && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
        {
            return &rowPtr[i];
        }

    }
    return NULL;
}

// /**
// FUNCTION   :: Ospfv3GetNodeInterfaceForNeighbor.
// LAYER      :: NETWORK
// PURPOSE    :: To Get Node Interface For Neighbor.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +neighborIpAddress:  in6_addr : Neighbor Ip Address.
// RETURN     :: Interface Index.
// **/
static
unsigned int Ospfv3GetNodeInterfaceForNeighbor(
    Node* node,
    in6_addr neighborIpAddress)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int i = 0;
    for (; i < (unsigned int) (node->numberInterfaces); i++)
    {
        Ospfv3Neighbor* neighborInfo = NULL;

        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        ListItem* neighborListItem = ospf->pInterface[i].neighborList->first;
        while (neighborListItem)
        {
            neighborInfo = (Ospfv3Neighbor* ) neighborListItem->data;

            if (CMP_ADDR6(
                    neighborInfo->neighborIPAddress,
                    neighborIpAddress) == 0)
            {
                return i;
            }
            neighborListItem = neighborListItem->next;
        }
    }
    return ANY_DEST;
}

// /**
// FUNCTION   :: Ospfv3AddRouteBorderRouter
// LAYER      :: NETWORK
// PURPOSE    :: To Add Route for Border Router
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSA:  Ospfv3IntraAreaPrefixLSA* : Pointer to LSA.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +vertex:  Ospfv3Vertex* : Pointer to Vertex.
// +destType:  Ospfv3DestType : Dest type.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AddRouteBorderRouter(
    Node* node,
    Ospfv3IntraAreaPrefixLSA* LSA,
    Ospfv3Area* thisArea,
    Ospfv3Vertex* vertex,
    Ospfv3DestType destType)
{
    Ospfv3PrefixInfo* prefixInfo = (Ospfv3PrefixInfo* ) (LSA + 1);
    int i = 0;

    for (i = 0; i < LSA->numPrefixes; prefixInfo++, i++)
    {
        Ospfv3RoutingTableRow* rowPtr = NULL;
        Ospfv3RoutingTableRow newRow;
        in6_addr prefix;
        Ospfv3NextHopListItem* nextHopItem = NULL;

        rowPtr = Ospfv3GetRoute(
                    node,
                    prefixInfo->prefixAddr,
                    destType);

        if (rowPtr)
        {
            rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;
            return ;
        }

        Ipv6GetPrefix(
            &prefixInfo->prefixAddr,
            &prefix,
            prefixInfo->prefixLength);

        rowPtr = Ospfv3GetRoute(
                    node,
                    prefix,
                    OSPFv3_DESTINATION_NETWORK);

        if (rowPtr)
        {

            if ((unsigned int) (rowPtr->metric) != vertex->distance)
            {
                continue;
            }

            newRow.destType = destType;
            COPY_ADDR6(prefixInfo->prefixAddr, newRow.destAddr);
            newRow.prefixLength = OSPFv3_MAX_PREFIX_LENGTH;
            newRow.option = 0;
            newRow.areaId = thisArea->areaId;
            newRow.pathType = OSPFv3_INTRA_AREA;
            newRow.metric = vertex->distance;
            newRow.type2Metric = 0;
            newRow.LSOrigin = (void* ) vertex->LSA;
            newRow.advertisingRouter = vertex->vertexId.routerVertexId;
            nextHopItem =
                (Ospfv3NextHopListItem* ) vertex->nextHopList->first->data;

            memcpy(
                &newRow.nextHop,
                &nextHopItem->nextHop,
                sizeof(in6_addr));

            newRow.outIntfId = nextHopItem->outIntfId;

            Ospfv3AddRoute(node, &newRow);

            break;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateIntraAreaPrefixDestination
// LAYER      :: NETWORK
// PURPOSE    :: To Update Intra Area Prefix Destination
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateIntraAreaPrefixDestination(Node* node,
                                            Ospfv3Area* thisArea)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* shortestPathListItem = thisArea->shortestPathList->first;
    Ospfv3IntraAreaPrefixLSA* intraAreaPrefixLSA = NULL;
    Ospfv3PrefixInfo* prefixInfo = NULL;

    while (shortestPathListItem )
    {
        Ospfv3Vertex* vertex = (Ospfv3Vertex* ) shortestPathListItem->data;

        if (vertex->vertexType == OSPFv3_VERTEX_NETWORK)
        {
            shortestPathListItem = shortestPathListItem->next;
            continue;
        }

        intraAreaPrefixLSA =
            (Ospfv3IntraAreaPrefixLSA* ) Ospfv3LookupLSAList(
                                            node,
                                            thisArea->intraAreaPrefixLSAList,
                                            vertex->vertexId.routerVertexId,
                                            vertex->vertexId.routerVertexId);

        Ospfv3RouterLSA* routerLSA = (Ospfv3RouterLSA* ) vertex->LSA;

        if (intraAreaPrefixLSA != NULL)
        {
            int i = 0;
            unsigned int eBit = 0;
            unsigned int bBit = 0;

            prefixInfo = (Ospfv3PrefixInfo* ) (intraAreaPrefixLSA + 1);

            if (intraAreaPrefixLSA->numPrefixes == 0
                || prefixInfo->prefixOptions & OSPFv3_PREFIX_NU_BIT)
            {
                shortestPathListItem = shortestPathListItem->next;
                continue;
            }

            eBit = routerLSA->bitsAndOptions & OSPFv3_E_BIT;
            bBit = routerLSA->bitsAndOptions & OSPFv3_B_BIT;

            if (bBit
                && ospf->routerId != routerLSA->LSHeader.advertisingRouter)
            {
                Ospfv3AddRouteBorderRouter(
                    node,
                    intraAreaPrefixLSA,
                    thisArea,
                    vertex,
                    OSPFv3_DESTINATION_ABR);
            }
            if (eBit
                && ospf->routerId != routerLSA->LSHeader.advertisingRouter)
            {
                Ospfv3AddRouteBorderRouter(
                    node,
                    intraAreaPrefixLSA,
                    thisArea,
                    vertex,
                    OSPFv3_DESTINATION_ASBR);
            }

            for (i = 0 ;i < intraAreaPrefixLSA->numPrefixes;
                 prefixInfo++, i++)
            {
                in6_addr prefix;
                Ospfv3RoutingTableRow* rowPtr = NULL;
                short newDistance = 0;
                Ospfv3RoutingTableRow newRow;
                Ospfv3NextHopListItem* nextHopItem = NULL;

                Ipv6GetPrefix(
                    &prefixInfo->prefixAddr,
                    &prefix,
                    prefixInfo->prefixLength);

                rowPtr = Ospfv3GetRoute(
                            node,
                            prefix,
                            OSPFv3_DESTINATION_NETWORK);

                if (ospf->routerId != vertex->vertexId.routerVertexId)
                {
                    nextHopItem = (Ospfv3NextHopListItem* )
                        vertex->nextHopList->first->data;

                    memcpy(
                        &newRow.nextHop,
                        &nextHopItem->nextHop,
                        sizeof(in6_addr));

                    newRow.outIntfId = nextHopItem->outIntfId;
                }
                else
                {
                    memset(&newRow.nextHop, 0, sizeof(in6_addr));

                    if (prefixInfo->prefixLength == OSPFv3_MAX_PREFIX_LENGTH)
                    {
                        newRow.outIntfId = Ospfv3GetNodeInterfaceForNeighbor(
                                                node,
                                                prefixInfo->prefixAddr);

                        if (newRow.outIntfId == ANY_DEST)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        newRow.outIntfId = Ipv6GetInterfaceIndexFromAddress(
                                                node,
                                                &prefixInfo->prefixAddr);
                    }
                }

                COPY_ADDR6(prefix, newRow.destAddr);

                newRow.prefixLength = prefixInfo->prefixLength;
                newRow.option = 0;
                newRow.areaId = thisArea->areaId;
                newRow.pathType = OSPFv3_INTRA_AREA;
                newRow.type2Metric = 0;
                newRow.LSOrigin = (void* ) vertex->LSA;
                newRow.advertisingRouter = vertex->vertexId.routerVertexId;

                if (rowPtr == NULL)
                {
                    newRow.destType = OSPFv3_DESTINATION_NETWORK;

                    if (prefixInfo->prefixLength == OSPFv3_MAX_PREFIX_LENGTH
                        && Ospfv3IsMyAddress(node, prefixInfo->prefixAddr))
                    {
                        continue;
                    }

                    newRow.metric =
                        vertex->distance + prefixInfo->reservedOrMetric;

                    Ospfv3AddRoute(node, &newRow);

                }
                else
                {
                    newDistance = (short) (vertex->distance)
                                            + prefixInfo->reservedOrMetric;

                    if (newDistance < rowPtr->metric)
                    {
                        newRow.destType = OSPFv3_DESTINATION_NETWORK;
                        newRow.metric = newDistance;

                        if (rowPtr->flag == OSPFv3_ROUTE_NEW)
                        {
                            newRow.flag = rowPtr->flag;
                        }
                        else
                        {
                            newRow.flag = OSPFv3_ROUTE_CHANGED;
                        }

                        memcpy(
                            rowPtr,
                            &newRow,
                            sizeof(Ospfv3RoutingTableRow));
                    }
                    else
                    {
                        if (rowPtr->flag == OSPFv3_ROUTE_INVALID)
                        {
                            rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;

                        }
                    }
                }
            }
        }
        shortestPathListItem = shortestPathListItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3FindShortestPathForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: To Find Shortest Path For This Area.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FindShortestPathForThisArea(Node* node, unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    LinkedList* candidateList = NULL;
    Ospfv3Vertex* tempV = NULL;

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    ERROR_Assert(thisArea, "Specified Area is not found");

    ListInit(node, &thisArea->shortestPathList);

    ListInit(node, &candidateList);

    tempV = (Ospfv3Vertex* ) MEM_malloc(sizeof(Ospfv3Vertex));

#ifdef OSPFv3_DEBUG_SPTErr
    {
        char areaStr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(areaId, areaStr);

        printf("\nNode %u Calculating shortest path for area %s\n",
            node->nodeId,
            areaStr);
    }
#endif

    // The shortest path starts with myself as the root.
    tempV->vertexId.routerVertexId = ospf->routerId;
    tempV->vertexType = OSPFv3_VERTEX_ROUTER;

    tempV->LSA = (char* ) Ospfv3LookupLSAList(
                            node,
                            thisArea->routerLSAList,
                            ospf->routerId,
                            ospf->routerId);

    if (tempV->LSA == NULL)
    {
        MEM_free(tempV);

        ListFree(node, thisArea->shortestPathList, FALSE);

        ListFree(node, candidateList, FALSE);

        return;
    }

    ListInit(node, &tempV->nextHopList);

    tempV->distance = 0;

    // Insert myself (root) to the shortest path list.
    ListInsert(node, thisArea->shortestPathList, 0, (void* ) tempV);

    // Find candidates to be considered for the shortest path list.
    Ospfv3UpdateCandidateList(node, thisArea, candidateList, tempV);

#ifdef OSPFv3_DEBUG_SPTErr
    {
        Ospfv3PrintCandidateList(node, candidateList);
    }
#endif

    // Keep calculating shortest path until the candidate list is empty.
    while (candidateList->size > 0)
    {
        // Select the next best node in the candidate list into
        // the shortest path list.  That node is tempV.
        tempV = Ospfv3UpdateShortestPathList(node, thisArea, candidateList);

#ifdef OSPFv3_DEBUG_SPTErr
        {
           Ospfv3PrintShortestPathList(node, thisArea->shortestPathList);
        }
#endif

        // Find more candidates to be considered for the shortest path list.
        Ospfv3UpdateCandidateList(node, thisArea, candidateList, tempV);

#ifdef OSPFv3_DEBUG_SPTErr
        {
            Ospfv3PrintCandidateList(node, candidateList);
        }
#endif

    }

    Ospfv3UpdateIntraAreaPrefixDestination(node, thisArea);

    Ospfv3FreeVertexList(node, thisArea->shortestPathList);

    Ospfv3FreeVertexList(node, candidateList);
}

// /**
// FUNCTION   :: Ospfv3GetAddrRangeInfoForAllArea
// LAYER      :: NETWORK
// PURPOSE    :: To Get Address Range For All Area.
// PARAMETERS ::
// +node      :  Node* : Pointer to node.
// +destAddr  :  in6_addr : Dest Address.
// RETURN     :: Area Address Ragne Info.
// **/
static
Ospfv3AreaAddressRange* Ospfv3GetAddrRangeInfoForAllArea(
    Node* node,
    in6_addr destAddr)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* area = NULL;
    Ospfv3AreaAddressRange* addrRangeInfo = NULL;
    ListItem* listItem = ospf->area->first;

    while (listItem)
    {
        area = (Ospfv3Area* ) listItem->data;

        addrRangeInfo = Ospfv3GetAddrRangeInfo(
                            node,
                            area->areaId,
                            destAddr);

        if (addrRangeInfo != NULL)
        {
            return addrRangeInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}

// /**
// FUNCTION   :: Ospfv3AddInterAreaRoute
// LAYER      :: NETWORK
// PURPOSE    :: To Add Inter Area Route.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +newRoute:  Ospfv3RoutingTableRow* : Pointer Routing Table Row.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AddInterAreaRoute(Node* node, Ospfv3RoutingTableRow* newRoute)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    // Get old route if any ..
    Ospfv3RoutingTableRow* rowPtr =
        Ospfv3GetRoute(node, newRoute->destAddr, newRoute->destType);

    if (rowPtr)
    {
        if (Ospfv3RoutesMatchSame(newRoute, rowPtr))
        {
            newRoute->flag = OSPFv3_ROUTE_NO_CHANGE;
        }
        else
        {
            newRoute->flag = OSPFv3_ROUTE_CHANGED;
        }

        memcpy(rowPtr, newRoute, sizeof(Ospfv3RoutingTableRow));
    }
    else
    {
        newRoute->flag = OSPFv3_ROUTE_NEW;
        rtTable->numRows++;

        BUFFER_AddDataToDataBuffer(
            &rtTable->buffer,
            (char* ) newRoute,
            sizeof(Ospfv3RoutingTableRow));
    }
}

// /**
// FUNCTION   :: Ospfv3AddInterAreaRoute
// LAYER      :: NETWORK
// PURPOSE    :: To Examine Inter Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ExamineInterAreaPrefixLSA(Node* node, Ospfv3Area* thisArea)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTableRow* rowPtr = NULL;
    Ospfv3RoutingTableRow* oldRoute = NULL;
    ListItem* listItem = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    in6_addr defaultAddress;

    memset(&defaultAddress, 0, sizeof(in6_addr));

    listItem = thisArea->interAreaPrefixLSAList->first;

    while (listItem)
    {
        Ospfv3RoutingTableRow newRow;
        in6_addr destAddr;
        unsigned char prefixLength;
        unsigned int borderRt;
        Int32 metric;
        Ospfv3IntraAreaPrefixLSA* intraAreaPrefixLSA = NULL;
        Ospfv3PrefixInfo* prefixInfo = NULL;
        int i = 0;

        LSHeader = (Ospfv3LinkStateHeader* ) listItem->data;
        prefixLength =
            ((Ospfv3InterAreaPrefixLSA* )LSHeader) ->prefixLength;

        Ipv6GetPrefix(
            &((Ospfv3InterAreaPrefixLSA* )LSHeader)->addrPrefix,
            &destAddr,
            prefixLength);

        metric = ((Ospfv3InterAreaPrefixLSA* )LSHeader)->bitsAndMetric;
        borderRt = LSHeader->advertisingRouter;

        // If cost equal to LSInfinity or age equal to MxAge, examine next
        if ((metric == OSPFv3_LS_INFINITY)
            || (LSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND)))
        {
            listItem = listItem->next;
            continue;
        }

        // Don't process self originated LSA
        if (LSHeader->advertisingRouter == ospf->routerId)
        {
            listItem = listItem->next;
            continue;
        }

        Ospfv3AreaAddressRange* addrInfo =
            Ospfv3GetAddrRangeInfoForAllArea(node, destAddr);

        if (addrInfo != NULL)
        {
            rowPtr = Ospfv3GetValidRoute(
                        node,
                        destAddr,
                        OSPFv3_DESTINATION_NETWORK);

            if (rowPtr != NULL)
            {
                listItem = listItem->next;
                continue;
            }

        }

        intraAreaPrefixLSA =
            (Ospfv3IntraAreaPrefixLSA* ) Ospfv3LookupLSAList(
                                            node,
                                            thisArea->intraAreaPrefixLSAList,
                                            borderRt,
                                            borderRt);

        if (intraAreaPrefixLSA != NULL)
        {
            prefixInfo = (Ospfv3PrefixInfo* ) (intraAreaPrefixLSA + 1);


            for (i = 0; i < intraAreaPrefixLSA->numPrefixes; i++)
            {
                // Lookup the routing table entry for border router having
                // this area as associated area
                rowPtr = Ospfv3GetIntraAreaRoute(
                            node,
                            prefixInfo->prefixAddr,
                            OSPFv3_DESTINATION_ABR,
                            thisArea->areaId);

                if (rowPtr)
                {
                    break;
                }
                prefixInfo = prefixInfo + 1;
            }

        }

        if ((rowPtr == NULL)
            || (rowPtr->flag == OSPFv3_ROUTE_INVALID))
        {
            listItem = listItem->next;
            continue;
        }

        COPY_ADDR6(destAddr, newRow.destAddr);

        newRow.option = 0;
        newRow.areaId = thisArea->areaId;
        newRow.pathType = OSPFv3_INTER_AREA;
        newRow.metric = rowPtr->metric + metric;

        // BGP-OSPF Patch Start
        if (CMP_ADDR6(defaultAddress, destAddr) == 0)
        {
            newRow.metric = metric;
        }
        // BGP-OSPF Patch End

        newRow.type2Metric = 0;
        newRow.LSOrigin = (void* ) listItem->data;
        newRow.advertisingRouter = borderRt;
        newRow.nextHop = rowPtr->nextHop;
        newRow.outIntfId = rowPtr->outIntfId;
        newRow.prefixLength = prefixLength;

        if (newRow.prefixLength == OSPFv3_MAX_PREFIX_LENGTH)
        {
            Ospfv3InterAreaRouterLSA* ASDestLSA =
                (Ospfv3InterAreaRouterLSA* )
                Ospfv3LookupLSAList(
                    node,
                    thisArea->interAreaRouterLSAList,
                    LSHeader->advertisingRouter,
                    LSHeader->linkStateId);

            newRow.destType = OSPFv3_DESTINATION_ASBR;

            if (ASDestLSA)
            {
                newRow.advertisingRouter = ASDestLSA->destinationRouterId;
            }
        }
        else
        {
            newRow.destType = OSPFv3_DESTINATION_NETWORK;
        }
        newRow.prefixLength = prefixLength;

        oldRoute = Ospfv3GetValidRoute(
                        node,
                        destAddr,
                        OSPFv3_DESTINATION_NETWORK);

        if ((oldRoute == NULL)
            || (oldRoute->pathType == OSPFv3_TYPE1_EXTERNAL)
            || (oldRoute->pathType == OSPFv3_TYPE2_EXTERNAL))
        {
            Ospfv3AddInterAreaRoute(node, &newRow);
        }
        listItem = listItem->next;

    }
}

// /**
// FUNCTION   :: Ospfv3FindInterAreaPath
// LAYER      :: NETWORK
// PURPOSE    :: To Find Inter Area Path.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FindInterAreaPath(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int examinTransitArea = EXAMINE_TRANSIT_AREA;

    // Examine only Backbone Summary LSA if the router
    // has active attachment to multiple areas.
    if (ospf->areaBorderRouter == TRUE)
    {
        //assert(ospf->backboneArea);
        ERROR_Assert(ospf->backboneArea, "Node is an ABR but not any "
            "interface belong into the backbone area\n");

        Ospfv3ExamineInterAreaPrefixLSA(node, ospf->backboneArea);

        if (examinTransitArea)
        {
            ListItem* listItem = ospf->area->first;

            while (listItem)
            {
                Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

                if (thisArea->areaId != OSPFv3_BACKBONE_AREA)
                {
                    Ospfv3ExamineInterAreaPrefixLSA(node, thisArea);
                }

                listItem = listItem->next;
            }
        }
    }
    else
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) ospf->area->first->data;

        ERROR_Assert(ospf->area->size == 1, "Node is an interior router, "
                    "so it should belong into only one area.\n");

        Ospfv3ExamineInterAreaPrefixLSA(node, thisArea);
    }
}

// /**
// FUNCTION   :: Ospfv3GetRouteForASBR
// LAYER      :: NETWORK
// PURPOSE    :: To Get Route For AS Boundary Router.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +asRouterId:  unsigned int : AS Router Id.
// RETURN     :: void : NULL.
// **/
static
Ospfv3RoutingTableRow* Ospfv3GetRouteForASBR(
    Node* node,
    unsigned int asRouterId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    int i = 0;

    for (; i < rtTable->numRows; i++)
    {
        if (rowPtr[i].advertisingRouter == asRouterId)
        {
            return (rowPtr + i);
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: Ospfv3FindASExternalPath
// LAYER      :: NETWORK
// PURPOSE    :: To Find AS-External Path.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3FindASExternalPath(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    LinkedList* list = ospf->asExternalLSAList;
    ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv3RoutingTableRow* rowPtr = NULL;
        Ospfv3RoutingTableRow newRow;
        Int32 cost = 0;
        Ospfv3LinkStateHeader* LSHeader =
            (Ospfv3LinkStateHeader* ) listItem->data;

        Ospfv3ASexternalLSA* externalLSA = (Ospfv3ASexternalLSA* )LSHeader;

        //unsigned int asExternalRt = LSHeader->advertisingRouter;
        cost = externalLSA->OptionsAndMetric & 0x00ffffff;

        // If cost equal to LSInfinity or age equal to MxAge, examine next
        if ((cost == OSPFv3_LS_INFINITY)
            || (LSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND)))
        {
            listItem = listItem->next;
            continue;
        }

        // Don't process self originated LSA
        if (LSHeader->advertisingRouter == ospf->routerId)
        {
            listItem = listItem->next;
            continue;
        }
        rowPtr = Ospfv3GetRouteForASBR(
                    node,
                    externalLSA->LSHeader.advertisingRouter);

        // If no entry exist for advertising Router (ASBR), do nothing.
        if (!rowPtr)
        {
            listItem = listItem->next;
            continue;
        }

        COPY_ADDR6(externalLSA->addrPrefix, newRow.destAddr);

        newRow.option = 0;
        newRow.areaId = 0;
        newRow.pathType = OSPFv3_TYPE2_EXTERNAL;
        newRow.type2Metric = cost;
        newRow.LSOrigin = (void* ) listItem->data;
        newRow.advertisingRouter = externalLSA->LSHeader.advertisingRouter;
        newRow.destType = OSPFv3_DESTINATION_NETWORK;
        newRow.prefixLength = externalLSA->prefixLength;
        newRow.metric = rowPtr->metric;

        COPY_ADDR6(rowPtr->nextHop, newRow.nextHop);

        newRow.outIntfId = rowPtr->outIntfId;
        newRow.areaId = rowPtr->areaId;

        rowPtr = Ospfv3GetRoute(
                    node,
                    newRow.destAddr,
                    newRow.destType);

        //Route exist into the Routing Table.
        if (rowPtr)
        {
            //Intra-area and inter-area paths are always preferred
            //over AS external paths.
            if ((rowPtr->pathType == OSPFv3_INTRA_AREA)
                || (rowPtr->pathType == OSPFv3_INTER_AREA))
            {
                rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;
            }
            else
            {
                if (newRow.type2Metric > rowPtr->type2Metric)
                {
                    rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;
                }
                else if (newRow.type2Metric == rowPtr->type2Metric)
                {
                    if (rowPtr->metric <= newRow.metric)
                    {
                        rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;
                    }
                    else
                    {
                        newRow.flag = OSPFv3_ROUTE_CHANGED;

                        memcpy(
                            rowPtr,
                            &newRow,
                            sizeof(Ospfv3RoutingTableRow));
                    }
                }
                else
                {
                    newRow.flag = OSPFv3_ROUTE_CHANGED;

                    memcpy(rowPtr, &newRow, sizeof(Ospfv3RoutingTableRow));
                }
            }
        }
        else
        {
            Ospfv3RoutingTable* rtTable = &ospf->routingTable;

            newRow.flag = OSPFv3_ROUTE_NEW;
            rtTable->numRows++;

            BUFFER_AddDataToDataBuffer(
                &rtTable->buffer,
                (char* ) (&newRow),
                sizeof(Ospfv3RoutingTableRow));
        }

        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateIpForwardingTable
// LAYER      :: NETWORK
// PURPOSE    :: To Update Ip Forwarding Table.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3UpdateIpForwardingTable(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    in6_addr destPrefix;
    int i;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&ospf->routingTable.buffer);

    in6_addr tempAddr;

    memset(&tempAddr, 0, sizeof(in6_addr));

#ifdef OSPFv3_DEBUG_ERRORS
    {
        Ospfv3PrintRoutingTable(node);
    }
#endif

    if (ospf->pWirelessFlag == FALSE)
    {
        for (i = 0; i < ospf->routingTable.numRows; i++)
        {
            // Call update function with metric value Network unreachable to
            // to invalidate old routes before updating again.
            // Avoid updating IP forwarding table for route to self.
            if ((!Ospfv3IsMyAddress(node, rowPtr[i].destAddr))
                && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
            {
                Ipv6GetPrefix(
                    &rowPtr[i].destAddr,
                    &destPrefix,
                    rowPtr[i].prefixLength);

                Ipv6UpdateForwardingTable(
                    node,
                    destPrefix,
                    rowPtr[i].prefixLength,
                    rowPtr[i].nextHop,
                    ANY_INTERFACE,
                    NETWORK_UNREACHABLE,
                    ROUTING_PROTOCOL_OSPFv3);
            }
        }

        for (i = 0; i < ospf->routingTable.numRows; i++)
        {
            Ipv6GetPrefix(
                &rowPtr[i].destAddr,
                &destPrefix,
                rowPtr[i].prefixLength);

            // Avoid updating IP forwarding table for route to self.
            if ((!Ospfv3IsMyAddress(node, rowPtr[i].destAddr))
                && (rowPtr[i].destType == OSPFv3_DESTINATION_NETWORK)
                && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
            {
                Ipv6UpdateForwardingTable(
                    node,
                    destPrefix,
                    rowPtr[i].prefixLength,
                    rowPtr[i].nextHop,
                    rowPtr[i].outIntfId,
                    rowPtr[i].metric,
                    ROUTING_PROTOCOL_OSPFv3);
            }
        }
    }

    // update Ip forwarding table in case of wireless
    else
    {
        for (i = 0; i < ospf->routingTable.numRows; i++)
        {
            // Call update function with metric value Network unreachable to
            // to invalidate old routes before updating again.
            // Avoid updating IP forwarding table for route to self.
            if ((!Ospfv3IsMyAddress(node, rowPtr[i].destAddr))
                && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
            {
                if (CMP_ADDR6(tempAddr, rowPtr[i].nextHop) == 0)
                {
                    if (rowPtr[i].prefixLength != OSPFv3_MAX_PREFIX_LENGTH)
                    {
                        continue;
                    }

                    Ipv6UpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        OSPFv3_MAX_PREFIX_LENGTH,
                        rowPtr[i].destAddr,
                        ANY_INTERFACE,
                        NETWORK_UNREACHABLE,
                        ROUTING_PROTOCOL_OSPFv3);
                }
                else
                {
                    Ipv6UpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        rowPtr[i].prefixLength,
                        rowPtr[i].nextHop,
                        ANY_INTERFACE,
                        NETWORK_UNREACHABLE,
                        ROUTING_PROTOCOL_OSPFv3);
                }
            }
        }

        for (i = 0; i < ospf->routingTable.numRows; i++)
        {

            // Avoid updating IP forwarding table for route to self.
            if ((!Ospfv3IsMyAddress(node, rowPtr[i].destAddr))
                && (rowPtr[i].destType == OSPFv3_DESTINATION_NETWORK)
                && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
            {
                if (CMP_ADDR6(tempAddr, rowPtr[i].nextHop) == 0)
                {
                    if (rowPtr[i].prefixLength != OSPFv3_MAX_PREFIX_LENGTH)
                    {
                        continue;
                    }

                    Ipv6UpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        OSPFv3_MAX_PREFIX_LENGTH,
                        rowPtr[i].destAddr,
                        rowPtr[i].outIntfId,
                        rowPtr[i].metric,
                        ROUTING_PROTOCOL_OSPFv3);
                }
                else
                {
                    Ipv6UpdateForwardingTable(
                        node,
                        rowPtr[i].destAddr,
                        rowPtr[i].prefixLength,
                        rowPtr[i].nextHop,
                        rowPtr[i].outIntfId,
                        rowPtr[i].metric,
                        ROUTING_PROTOCOL_OSPFv3);
                }
            }
        }
    }
}

// /**
// FUNCTION   :: Ospfv3ResetAdvrtToAreaFlag
// LAYER      :: NETWORK
// PURPOSE    :: To Reset advertise to Area flag within Area Address Range
//               struct for each area..
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ResetAdvrtToAreaFlag(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

        ListItem* addrListItem = thisArea->areaAddrRange->first;

        while (addrListItem)
        {
            int i;
            Ospfv3AreaAddressRange* addrInfo
                = (Ospfv3AreaAddressRange* ) addrListItem->data;

            for (i = 0; i < OSPFv3_MAX_ATTACHED_AREA; i++)
            {
                addrInfo->advrtToArea[i] = FALSE;
            }

            addrListItem = addrListItem->next;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3NextHopBelongsToThisArea
// LAYER      :: NETWORK
// PURPOSE    :: To check if Next Hop Belongs To This Area.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisRoute:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +area:  Ospfv3Area* : Pointer to Area.
// RETURN     :: TRUE if next hop belong to the area, FALSE otherwise.
// **/
static
BOOL Ospfv3NextHopBelongsToThisArea(Node* node,
                                    Ospfv3RoutingTableRow* thisRoute,
                                    Ospfv3Area* area)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ERROR_Assert(thisRoute->outIntfId != ANY_DEST,
                "Neighbor on unknown interface\n");

    if (ospf->pInterface[thisRoute->outIntfId].areaId == area->areaId)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3GetAddrRangeInfo
// LAYER      :: NETWORK
// PURPOSE    :: To Get Addr Range Info.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +areaId:  unsigned int : Area Id.
// +destAddr:  in6_addr : Pointer to Dest Address.
// RETURN     :: Pointer to Address Range.
// **/
Ospfv3AreaAddressRange* Ospfv3GetAddrRangeInfo(
    Node* node,
    unsigned int areaId,
    in6_addr destAddr)
{
    Ospfv3AreaAddressRange* addrRangeInfo = NULL;
    Ospfv3Area* area = Ospfv3GetArea(node, areaId);
    ListItem* listItem = area->areaAddrRange->first;

    while (listItem)
    {
        in6_addr prefix1;
        in6_addr prefix2;

        addrRangeInfo = (Ospfv3AreaAddressRange* ) listItem->data;

        Ipv6GetPrefix(
            &destAddr,
            &prefix1,
            addrRangeInfo->prefixLength);

        Ipv6GetPrefix(
            &addrRangeInfo->addressPrefix,
            &prefix2,
            addrRangeInfo->prefixLength);

        if (CMP_ADDR6(prefix1, prefix2) == 0)
        {
            return addrRangeInfo;
        }

        listItem = listItem->next;
    }

    return NULL;
}

// /**
// FUNCTION   :: Ospfv3ScheduleInterAreaPrefixLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Schedule Inter Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +row:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +areaId:  unsigned int : Area Id.
// +flush:  BOOL : Flush or Not Flush.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ScheduleInterAreaPrefixLSA(
    Node* node,
    Ospfv3RoutingTableRow* row,
    unsigned int areaId,
    BOOL flush)
{
    Message* newMsg = NULL;
    char* msgInfo = NULL;
    Ospfv3LinkStateType lsType;

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    ERROR_Assert(thisArea, "Area doesn't exist\n");

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv3,
                MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(Ospfv3LinkStateType) + sizeof(unsigned int)
        + sizeof(BOOL) + sizeof(in6_addr) + sizeof(unsigned char)
        + sizeof(Ospfv3DestType) + sizeof(Int32) + sizeof(Ospfv3Flag)
        + sizeof(unsigned int));

    lsType = OSPFv3_INTER_AREA_PREFIX;

    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv3LinkStateType));

    msgInfo += sizeof(Ospfv3LinkStateType);

    memcpy(msgInfo, &areaId, sizeof(unsigned int));

    msgInfo += sizeof(unsigned int);

    memcpy(msgInfo, &flush, sizeof(BOOL));

    msgInfo += sizeof(BOOL);

    memcpy(msgInfo, &(row->destAddr), sizeof(in6_addr));

    msgInfo += sizeof(in6_addr);

    memcpy(msgInfo, &(row->prefixLength), sizeof(unsigned char));

    msgInfo += sizeof(unsigned char);

    memcpy(msgInfo, &(row->destType), sizeof(Ospfv3DestType));

    msgInfo += sizeof(Ospfv3DestType);

    memcpy(msgInfo, &(row->metric), sizeof(Int32));

    msgInfo += sizeof(Int32);

    memcpy(msgInfo, &(row->flag), sizeof(Ospfv3Flag));

    msgInfo += sizeof(Ospfv3Flag);

    memcpy(msgInfo, &(row->advertisingRouter), sizeof(unsigned int));

    MESSAGE_Send(node, newMsg, (clocktype) OSPFv3_MIN_LS_INTERVAL);
}

// /**
// FUNCTION   :: Ospfv3OriginateInterAreaPrefixLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Originate Inter Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +advRt:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +flush:  BOOL : Flush or Not Flush.
// RETURN     :: UInt64 combination of 32 bits LSId and Advertising Rtr Id.
// **/
static
UInt64 Ospfv3OriginateInterAreaPrefixLSA(
    Node* node,
    Ospfv3RoutingTableRow* advRt,
    Ospfv3Area* thisArea,
    BOOL flush)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    ListItem* LSAItem = NULL;
    UInt64 retValLSId = (UInt64)-1;
    BOOL retVal = FALSE;
    unsigned int linkStateId = 0;

    Ospfv3InterAreaPrefixLSA* prefixLSA = (Ospfv3InterAreaPrefixLSA* )
            MEM_malloc(sizeof(Ospfv3InterAreaPrefixLSA));

    memset(prefixLSA, 0, sizeof(Ospfv3InterAreaPrefixLSA));

    if (advRt->destType == OSPFv3_DESTINATION_NETWORK
        || advRt->destType == OSPFv3_DESTINATION_ASBR
        || advRt->destType == OSPFv3_DESTINATION_ABR_ASBR)
    {
        linkStateId = Ospfv3GetLSIdFromPrefix(node, advRt->destAddr);

        if (linkStateId == ANY_DEST)
        {
            linkStateId = Ospfv3CreateLSId(node, advRt->destAddr);
        }

        // Get old instance, if any..
        oldLSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->interAreaPrefixLSAList,
                        ospf->routerId,
                        linkStateId);

        retVal = Ospfv3CreateLSHeader(
                    node,
                    thisArea->areaId,
                    OSPFv3_INTER_AREA_PREFIX,
                    &prefixLSA->LSHeader,
                    oldLSHeader);

        prefixLSA->prefixLength = advRt->prefixLength;
        prefixLSA->prefixOptions = 0;

        COPY_ADDR6(advRt->destAddr, prefixLSA->addrPrefix);

        retValLSId &= (unsigned int)linkStateId;
        retValLSId = retValLSId << 32;
        retValLSId |= (unsigned int)advRt->advertisingRouter;
    }
    else
    {
        // Shouldn't be here
        ERROR_Assert(FALSE, "Destination type is not a Network,"
            " so it is not possible to create Inter Area Prefix LSA\n");

    }

    if ((LSAItem
        && (node->getNodeTime() - LSAItem->timeStamp < OSPFv3_MIN_LS_INTERVAL))
        || (!retVal))
    {
        MEM_free(prefixLSA);

        Ospfv3ScheduleInterAreaPrefixLSA(
            node,
            advRt,
            thisArea->areaId,
            FALSE);

        return (UInt64)-1;
    }

    prefixLSA->bitsAndMetric = advRt->metric;
    prefixLSA->LSHeader.length = sizeof(Ospfv3InterAreaPrefixLSA);
    prefixLSA->LSHeader.linkStateId = linkStateId;

    if (advRt->flag == OSPFv3_ROUTE_INVALID)
    {
        prefixLSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }
    if (flush)
    {
        prefixLSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }

    if (Ospfv3InstallLSAInLSDB(
            node,
            thisArea->interAreaPrefixLSAList,
            (char* )prefixLSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout,
            "   \nFlood self originated Inter-Area-Prefix LSA\n");
    }
#endif

    // Flood LSA
    Ospfv3FloodLSAThroughArea(
        node,
        (char* )prefixLSA,
        ANY_DEST,
        thisArea->areaId);

    ospf->stats.numInterAreaPrefixLSAOriginate++;

    MEM_free(prefixLSA);

    return retValLSId;
}

// /**
// FUNCTION   :: Ospfv3OriginateInterAreaRouterLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Originate Inter Area Router LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +advRt:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// +val:  UInt64 : 64 bit value represent LS Id and Advertising Id
//                 32 bit each
// +flush:  BOOL : Flush or Not Flush.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3OriginateInterAreaRouterLSA(
    Node* node,
    Ospfv3RoutingTableRow* advRt,
    Ospfv3Area* thisArea,
    UInt64 val,
    BOOL flush)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    ListItem* LSAItem = NULL;
    unsigned int linkStateId = 0;
    BOOL retVal = FALSE;

    Ospfv3InterAreaRouterLSA * LSA = (Ospfv3InterAreaRouterLSA* )
        MEM_malloc(sizeof(Ospfv3InterAreaRouterLSA));

    memset(LSA, 0, sizeof(Ospfv3InterAreaRouterLSA));

    LSA->destinationRouterId = (unsigned int) val;
    val = val >> 32;
    linkStateId = (unsigned int) val;
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    thisArea->interAreaRouterLSAList,
                    ospf->routerId,
                    linkStateId);

    retVal = Ospfv3CreateLSHeader(
                    node,
                    thisArea->areaId,
                    OSPFv3_INTER_AREA_ROUTER,
                    LSHeader,
                    oldLSHeader);

    if ((LSAItem
        && (node->getNodeTime() - LSAItem->timeStamp < OSPFv3_MIN_LS_INTERVAL))
        || (!retVal))
    {
        MEM_free(LSA);

        Ospfv3ScheduleInterAreaPrefixLSA(
            node,
            advRt,
            thisArea->areaId,
            FALSE);

        return;
    }

    LSA->bitsAndPathMetric = advRt->metric;
    LSA->LSHeader.length = sizeof(Ospfv3InterAreaRouterLSA);
    LSA->LSHeader.linkStateId = linkStateId;
    LSA->bitsAndOptions = 0;

    if (advRt->flag == OSPFv3_ROUTE_INVALID)
    {
        LSHeader->linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }
    if (flush)
    {
        LSHeader->linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }

    if (Ospfv3InstallLSAInLSDB(
            node,
            thisArea->interAreaRouterLSAList,
            (char* )LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        //Ospfv3FindShortestPath(node);
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout, "\nFlood self originated Inter-Area-Router LSA\n");
    }
#endif

    // Flood LSA
    Ospfv3FloodLSAThroughArea(node, (char* )LSA, ANY_DEST, thisArea->areaId);

    ospf->stats.numInterAreaRtrLSAOriginate++;

    MEM_free(LSA);
}

// /**
// FUNCTION   :: Ospfv3GetMaxMetricForThisRange
// LAYER      :: NETWORK
// PURPOSE    :: To Get Maximum Metric For The Range.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +rtTable:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +addrInfo:  Ospfv3AreaAddressRange* : Pointer to Area Address Range.
// RETURN     :: Maximum Metric.
// **/
static
Int32 Ospfv3GetMaxMetricForThisRange(
    Node* node,
    Ospfv3RoutingTable* rtTable,
    Ospfv3AreaAddressRange* addrInfo)
{
    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    int i = 0;
    Int32 maxMetric = 0;
    Ospfv3Area* thisArea = (Ospfv3Area* ) addrInfo->area;

    for (i = 0; i < rtTable->numRows; i++)
    {
        in6_addr prefix1;
        in6_addr prefix2;

        Ipv6GetPrefix(
            &rowPtr[i].destAddr,
            &prefix1,
            addrInfo->prefixLength);

        Ipv6GetPrefix(
            &addrInfo->addressPrefix,
            &prefix2,
            addrInfo->prefixLength);

        if ((CMP_ADDR6(prefix1, prefix2) == 0)
            && (rowPtr[i].areaId == thisArea->areaId)
            && (rowPtr[i].pathType == OSPFv3_INTRA_AREA)
            && (rowPtr[i].metric > maxMetric)
            && (rowPtr[i].flag != OSPFv3_ROUTE_INVALID))
        {
            maxMetric = (short) (rowPtr[i].metric);
        }
    }

    return maxMetric;
}

// /**
// FUNCTION   :: Ospfv3AdvertiseRoute
// LAYER      :: NETWORK
// PURPOSE    :: To Advertise Route.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisRoute:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AdvertiseRoute(Node* node, Ospfv3RoutingTableRow* thisRoute)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3Area* area = NULL;
    int count = 0;

    for (listItem = ospf->area->first; listItem;
        listItem = listItem->next, count++)
    {
        area = (Ospfv3Area* ) listItem->data;

        // Inter-Area routes are never advertised to backbone
        if ((thisRoute->pathType == OSPFv3_INTER_AREA)
            && (area->areaId == OSPFv3_BACKBONE_AREA))
        {
            continue;
        }

        // Do not generate Summary LSA if the area associated
        // with this set of path is the area itself
        if (thisRoute->areaId == area->areaId)
        {
            continue;
        }

        // If next hop belongs to this area ...
        if (Ospfv3NextHopBelongsToThisArea(node, thisRoute, area))
        {
            continue;
        }

        if (thisRoute->destType == OSPFv3_DESTINATION_ASBR
            || thisRoute->destType == OSPFv3_DESTINATION_ABR_ASBR)
        {
            UInt64 retVal = Ospfv3OriginateInterAreaPrefixLSA(
                                node,
                                thisRoute,
                                area,
                                FALSE);

            Ospfv3OriginateInterAreaRouterLSA(
                node,
                thisRoute,
                area,
                retVal,
                FALSE);
        }
        else if (thisRoute->pathType == OSPFv3_INTER_AREA)
        {
            Ospfv3OriginateInterAreaPrefixLSA(node, thisRoute, area, FALSE);
        }
        else
        {
            // This is an Intra area route to a network which is contained
            // in one of the router's directly attached network
            Ospfv3AreaAddressRange* addrInfo = Ospfv3GetAddrRangeInfo(
                                                node,
                                                thisRoute->areaId,
                                                thisRoute->destAddr);

            // 1) By default, if a network is not contained in any explicitly
            //    configured address range, a Type 3 summary-LSA is generated
            //
            // 2) The backbone's configured ranges should be ignored when
            //    originating summary-LSAs into transit areas.
            //
            // 3) When range's status indicates DoNotAdvertise, the Type 3
            //    summary-LSA is suppressed and the component networks remain
            //    hidden from other areas.
            //
            // 4) At most a single type3 Summary-LSA is
            //    generated for each range
            if (!addrInfo
                || ((thisRoute->areaId == OSPFv3_BACKBONE_AREA)
                && (area->transitCapability == TRUE)))
            {
                Ospfv3OriginateInterAreaPrefixLSA(
                    node,
                    thisRoute,
                    area,
                    FALSE);
            }
            else if (addrInfo->advertise == TRUE
                    && addrInfo->advrtToArea[count] == FALSE)
            {
                in6_addr prefix;
                Ospfv3RoutingTableRow advRt;

                memset(&advRt, 0, sizeof(Ospfv3RoutingTableRow));

                Ipv6GetPrefix(
                    &addrInfo->addressPrefix,
                    &prefix,
                    addrInfo->prefixLength);

                advRt.destType = OSPFv3_DESTINATION_NETWORK;

                COPY_ADDR6(prefix, advRt.destAddr);

                advRt.prefixLength = addrInfo->prefixLength;
                advRt.flag = OSPFv3_ROUTE_NO_CHANGE;

                advRt.metric = Ospfv3GetMaxMetricForThisRange(
                                    node,
                                    &ospf->routingTable,
                                    addrInfo);

                Ospfv3OriginateInterAreaPrefixLSA(node, &advRt, area, FALSE);

                addrInfo->advrtToArea[count] = TRUE;
            }
        }
    }
}

// /**
// FUNCTION   :: Ospfv3CheckForInterAreaPrefixLSAValidity
// LAYER      :: NETWORK
// PURPOSE    :: To Check For Inter Area Prefix LSA Validity.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +advRt:  Ospfv3RoutingTableRow* : Pointer to Routing Table Row.
// +areaId:  unsigned int : Area Id.
// +matchPtr: Ospfv3RoutingTableRow** : Pointer to matched Routing Table Row.
// RETURN     :: 1 if have perfect match.
//               -1 if not a perfect match.
//               0 no match.
// **/
static
int Ospfv3CheckForInterAreaPrefixLSAValidity(
    Node* node,
    Ospfv3RoutingTableRow* advRt,
    unsigned int areaId,
    Ospfv3RoutingTableRow** matchPtr)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i = 0;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    Ospfv3AreaAddressRange* addrInfo = NULL;
    *matchPtr = NULL;
    in6_addr defaultDestAddr;

    memset(&defaultDestAddr, 0, sizeof(in6_addr));

    if ((CMP_ADDR6(advRt->destAddr, defaultDestAddr) == 0)
        && advRt->prefixLength == OSPFv3_DEFAULT_PREFIX_LENGTH)
    {
        return 1;
    }

    for (i = 0; i < rtTable->numRows; i++)
    {
        in6_addr prefix1;
        in6_addr prefix2;

        Ipv6GetPrefix(&rowPtr[i].destAddr, &prefix1, rowPtr[i].prefixLength);

        Ipv6GetPrefix(&advRt->destAddr, &prefix2, advRt->prefixLength);

        // Do we have a perfect match?
        if ((CMP_ADDR6(prefix1, prefix2) == 0)
            && advRt->prefixLength == rowPtr[i].prefixLength
            && Ospfv3CompDestType(rowPtr[i].destType, advRt->destType)
            && rowPtr[i].areaId != areaId
            && rowPtr[i].flag != OSPFv3_ROUTE_INVALID)
        {
            *matchPtr = &rowPtr[i];
            return 1;
        }
    }

    // We didn't get a perfect match. So it might be an aggregated
    // advertisement which must be one of our configured area address
    // range. So search for an intra area entry contained within this range.
    for (i = 0; i < rtTable->numRows; i++)
    {
        in6_addr prefix1;
        in6_addr prefix2;

        Ipv6GetPrefix(&rowPtr[i].destAddr, &prefix1, rowPtr[i].prefixLength);

        Ipv6GetPrefix(&advRt->destAddr, &prefix2, advRt->prefixLength);

        if ((CMP_ADDR6(prefix1, prefix2) == 0)
            && advRt->prefixLength < rowPtr[i].prefixLength
            && rowPtr[i].pathType == OSPFv3_INTRA_AREA
            && Ospfv3CompDestType(rowPtr[i].destType, advRt->destType)
            && rowPtr[i].areaId != areaId
            && rowPtr[i].flag != OSPFv3_ROUTE_INVALID)
        {
            addrInfo = Ospfv3GetAddrRangeInfoForAllArea(
                            node,
                            advRt->destAddr);

            ERROR_Assert(addrInfo != NULL
                    && (CMP_ADDR6(addrInfo->addressPrefix, advRt->destAddr)
                    == 0)
                    && (addrInfo->prefixLength == advRt->prefixLength),
                    "Something wrong with route suppression\n");

            *matchPtr = &rowPtr[i];
            return -1;
        }
    }
    return 0;
}

// /**
// FUNCTION   :: Ospfv3CheckOldInterAreaPrefixLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Check old Inter Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisArea:  Ospfv3Area* : Pointer to Area.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3CheckOldInterAreaPrefixLSA(Node* node, Ospfv3Area* thisArea)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTableRow* rowPtr = NULL;

    LinkedList* list = NULL;
    ListItem* listItem = NULL;
    list = thisArea->interAreaPrefixLSAList;
    listItem = list->first;

    while (listItem)
    {
        Ospfv3InterAreaPrefixLSA* prefixLSA = NULL;
        Ospfv3RoutingTableRow newRow;
        int retVal = 0;

        memset(&newRow, 0, sizeof(Ospfv3RoutingTableRow));

        prefixLSA = (Ospfv3InterAreaPrefixLSA* ) listItem->data;

        // Process only self originated LSA
        if (prefixLSA->LSHeader.advertisingRouter != ospf->routerId)
        {
            listItem = listItem->next;
            continue;
        }

        COPY_ADDR6(prefixLSA->addrPrefix, newRow.destAddr);

        newRow.prefixLength = prefixLSA->prefixLength;

        if (prefixLSA->prefixLength == OSPFv3_MAX_PREFIX_LENGTH)
        {
            newRow.destType = OSPFv3_DESTINATION_ASBR;
        }
        else
        {
            newRow.destType = OSPFv3_DESTINATION_NETWORK;
        }

        // Lookup the routing table entry for an entry to this route
        retVal = Ospfv3CheckForInterAreaPrefixLSAValidity(
                    node,
                    &newRow,
                    thisArea->areaId,
                    &rowPtr);

        if (!retVal)
        {
            Ospfv3FlushLSA(node, (char* ) prefixLSA, thisArea->areaId);

            listItem = listItem->next;

            Ospfv3RemoveLSAFromLSDB(
                node,
                (char* ) prefixLSA,
                thisArea->areaId);

            continue;
        }

        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3HandleRemovedOrChangedRoutes
// LAYER      :: NETWORK
// PURPOSE    :: To Handle Removed Or Changed Routes.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
static
void Ospfv3HandleRemovedOrChangedRoutes(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3Area* area = NULL;

    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        area = (Ospfv3Area* ) listItem->data;

        // Examine all inter area LSAs originated by me for this area.
        Ospfv3CheckOldInterAreaPrefixLSA(node, area);
    }
}

// /**
// FUNCTION   :: Ospfv3OriginateDefaultInterAreaPrefixLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Originate Default Inter Area Prefix LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
static
void Ospfv3OriginateDefaultInterAreaPrefixLSA(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTableRow defaultRt;
    ListItem* listItem;

    memset(&defaultRt, 0, sizeof(Ospfv3RoutingTableRow));

    defaultRt.destType = OSPFv3_DESTINATION_NETWORK;

    memset(&defaultRt.destAddr, 0, sizeof(in6_addr));

    defaultRt.prefixLength = 0;
    defaultRt.flag = OSPFv3_ROUTE_NO_CHANGE;
    listItem = ospf->area->first;

    while (listItem)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

        if (thisArea->transitCapability == FALSE)
        {
            defaultRt.metric = thisArea->stubDefaultCost;

            Ospfv3OriginateInterAreaPrefixLSA(
                node,
                &defaultRt,
                thisArea,
                FALSE);
        }
        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3HandleABRTask
// LAYER      :: NETWORK
// PURPOSE    :: To Handle ABR Task.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
static
void Ospfv3HandleABRTask(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i = 0;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&ospf->routingTable.buffer);

    BOOL originateDefaultForAttachedStubArea = FALSE;

    // Check entire routing table for added, deleted or changed
    // Intra-Area or Inter-Area routes that need to be advertised
    Ospfv3ResetAdvrtToAreaFlag(node);

    for (i = 0; i < ospf->routingTable.numRows; i++)
    {
        // Examine next route if this have not been changed
        if (rowPtr[i].flag == OSPFv3_ROUTE_NO_CHANGE)
        {
            continue;
        }

        if (rowPtr[i].flag == OSPFv3_ROUTE_INVALID)
        {
            continue;
        }

        // Only destination of type newtwork and ASBR needs to be advertised
        if (rowPtr[i].destType == OSPFv3_DESTINATION_ABR)
        {
            continue;
        }

        // External routes area never advertised in Summary LSA
        if ((rowPtr[i].pathType == OSPFv3_TYPE1_EXTERNAL)
            || (rowPtr[i].pathType == OSPFv3_TYPE2_EXTERNAL))
        {
            originateDefaultForAttachedStubArea = TRUE;
            continue;
        }

        // Summary LSA shouldn't be generated for a route whose
        // cost equal equal or exceed the value LSInfinity
        if (rowPtr[i].metric >= OSPFv3_LS_INFINITY)
        {
            continue;
        }

        // Advertise this route
        Ospfv3AdvertiseRoute(node, &rowPtr[i]);
    }

    // If the destination is still reachable, yet can no longer be
    // advertised according to the above procedure (e.g., it is now
    // an inter-area route, when it used to be an intra-area route
    // associated with some non-backbone area; it would thus no longer
    // be advertisable to the backbone), the LSA should also be flushed
    // from the routing domain.
    Ospfv3HandleRemovedOrChangedRoutes(node);

    if (originateDefaultForAttachedStubArea)
    {
        Ospfv3OriginateDefaultInterAreaPrefixLSA(node);
    }
}

// /**
// FUNCTION   :: Ospfv3FreeRoute
// LAYER      :: NETWORK
// PURPOSE    :: To Free a Route.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +rowPtr:  Ospfv3RoutingTableRow* : Pointer to Routing table row.
// RETURN     :: void : NULL.
static
void Ospfv3FreeRoute(Node* node, Ospfv3RoutingTableRow* rowPtr)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    ERROR_Assert(rowPtr, "this variable rowPtr is NULL\n");

    BUFFER_ClearDataFromDataBuffer(
        &rtTable->buffer,
        (char* ) rowPtr,
        sizeof(Ospfv3RoutingTableRow),
        FALSE);

    rtTable->numRows--;
}

// /**
// FUNCTION   :: Ospfv3FreeInvalidRoute
// LAYER      :: NETWORK
// PURPOSE    :: To Free Invalid Route.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
static
void Ospfv3FreeInvalidRoute(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (rowPtr[i].flag == OSPFv3_ROUTE_INVALID)
        {
            Ospfv3FreeRoute(node, &rowPtr[i]);

            i--;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3SaveOldRoutingTable
// LAYER      :: NETWORK
// PURPOSE    :: To Save Old Routing Table.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +oldRtTablePtr:  Ospfv3RoutingTableRow* : Pointer to old Routing table.
// RETURN     :: void : NULL.
static
void Ospfv3SaveOldRoutingTable(
    Node* node,
    Ospfv3RoutingTable* oldRtTablePtr)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    memcpy(oldRtTablePtr, rtTable, sizeof(Ospfv3RoutingTable));

    oldRtTablePtr->buffer.data =
        (char* ) MEM_malloc(BUFFER_GetMaxSize(&rtTable->buffer));

    memcpy(
        oldRtTablePtr->buffer.data,
        rowPtr,
        BUFFER_GetCurrentSize(&rtTable->buffer));

    BUFFER_DestroyDataBuffer(&rtTable->buffer);

    Ospfv3InitRoutingTable(node);
}

// /**
// FUNCTION   :: Ospfv3AdjustIntraAreaPath
// LAYER      :: NETWORK
// PURPOSE    :: To Adjust Intra Area Path.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +oldRtTable:  Ospfv3RoutingTableRow* : Pointer to old Routing table.
// RETURN     :: void : NULL.
static
void Ospfv3AdjustIntraAreaPath(Node* node, Ospfv3RoutingTable* oldRtTable)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable tempRtTable;
    Ospfv3RoutingTableRow* tempRowPtr = NULL;
    Ospfv3RoutingTableRow* rowPtr = NULL;
    int i = 0;

    memcpy(&tempRtTable, &ospf->routingTable, sizeof(Ospfv3RoutingTable));

    memcpy(&ospf->routingTable, oldRtTable, sizeof(Ospfv3RoutingTable));

    tempRowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&tempRtTable.buffer);

    for (; i < tempRtTable.numRows; i++)
    {
        rowPtr = Ospfv3GetIntraAreaRoute(
                    node,
                    tempRowPtr[i].destAddr,
                    tempRowPtr[i].destType,
                    tempRowPtr[i].areaId);

        if (rowPtr)
        {
            if (Ospfv3RoutesMatchSame(tempRowPtr + i, rowPtr))
            {
                memcpy(
                    rowPtr,
                    tempRowPtr + i,
                    sizeof(Ospfv3RoutingTableRow));

                rowPtr->flag = OSPFv3_ROUTE_NO_CHANGE;
            }
            else
            {
                memcpy(
                    rowPtr,
                    tempRowPtr + i,
                    sizeof(Ospfv3RoutingTableRow));

                rowPtr->flag = OSPFv3_ROUTE_CHANGED;
            }
        }
        else
        {
            tempRowPtr[i].flag = OSPFv3_ROUTE_NEW;
            ospf->routingTable.numRows++;

            BUFFER_AddDataToDataBuffer(
                &ospf->routingTable.buffer,
                (char* ) (tempRowPtr + i),
                sizeof(Ospfv3RoutingTableRow));
        }
    }
    BUFFER_DestroyDataBuffer(&tempRtTable.buffer);
}

// /**
// FUNCTION   :: Ospfv3FindShortestPath
// LAYER      :: NETWORK
// PURPOSE    :: To Find Shortest Path.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// RETURN     :: void : NULL.
static
void Ospfv3FindShortestPath(Node* node)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* listItem = NULL;
    Ospfv3RoutingTable oldRtTable;

    // Invalidate present routing table and save it so that
    // changes in routing table entries can be identified.
    Ospfv3InvalidateRoutingTable(node);

    Ospfv3SaveOldRoutingTable(node, &oldRtTable);

#ifdef OSPFv3_DEBUG_TABLEErr
    {
        Ospfv3PrintLSDB(node);
    }
#endif

    // Find Intra Area route for each attached area
    for (listItem = ospf->area->first; listItem; listItem = listItem->next)
    {
        Ospfv3Area* thisArea = (Ospfv3Area* ) listItem->data;

        Ospfv3FindShortestPathForThisArea(node, thisArea->areaId);
    }

    Ospfv3AdjustIntraAreaPath(node, &oldRtTable);

    // Calculate Inter Area routes
    if (ospf->partitionedIntoArea == TRUE)
    {
        Ospfv3FindInterAreaPath(node);
    }

    // BGP-OSPF Patch Start
    if (!ospf->asBoundaryRouter )
    {
        Ospfv3FindASExternalPath(node);
    }
    // BGP-OSPF Patch End

    // Update IP forwarding table using new shortest path.
    Ospfv3UpdateIpForwardingTable(node);

#ifdef OSPFv3_DEBUG_TABLE
    {
        Ospfv3PrintRoutingTable(node);

        Ospfv3PrintNeighborState(node);
    }
#endif

    if ((ospf->partitionedIntoArea == TRUE)
        && (ospf->areaBorderRouter == TRUE))
    {
        Ospfv3HandleABRTask(node);
    }
    Ospfv3FreeInvalidRoute(node);
}

// /**
// FUNCTION   :: Ospfv3OriginateNetworkLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Find Shortest Path.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  unsigned int : Interface Id.
// +flush:  BOOL : Flush or Not Flush.
// RETURN     :: void : NULL.
static
void Ospfv3OriginateNetworkLSA(
    Node* node,
    unsigned int interfaceId,
    BOOL flush)
{
    int count = 0;
    ListItem* tempListItem = NULL;
    Ospfv3Neighbor* tempNeighborInfo = NULL;
    Ospfv3NetworkLSA* LSA = NULL;
    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    unsigned int* attachedRouters = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea =
        Ospfv3GetArea(node, ospf->pInterface[interfaceId].areaId);

    int numNbr = ospf->pInterface[interfaceId].neighborList->size;

    attachedRouters =
        (unsigned int* ) MEM_malloc(sizeof(unsigned int) * (numNbr + 1));

#ifdef OSPFv3_DEBUG_LSDB
    {
        char clockStr[MAX_STRING_LENGTH];
        char addrString[MAX_STRING_LENGTH];
        in6_addr netAddr;
        in6_addr prefix;

        ctoa(node->getNodeTime(), clockStr);

        Ipv6GetGlobalAggrAddress(node, interfaceId, &netAddr);

        Ipv6GetPrefix(
            &netAddr,
            &prefix,
            Ipv6GetPrefixLength(node, interfaceId));

        IO_ConvertIpAddressToString(&prefix, addrString);

        printf("    Node %u originating Network LSA for network %s "
                "at time %s\n",
            node->nodeId,
            addrString,
            clockStr);
    }
#endif

    // Include the designated router also.
    attachedRouters[count++] = ospf->routerId;
    tempListItem = ospf->pInterface[interfaceId].neighborList->first;

    // Get all my neighbors information from my neighbor list.
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

        ERROR_Assert(tempNeighborInfo,
            "Neighbor not present into the Neighbor list\n");

        // List those router that area fully adjacent to DR
        if (tempNeighborInfo->state == S_Full)
        {
            attachedRouters[count++] = tempNeighborInfo->neighborId;
        }

        tempListItem = tempListItem->next;
    }

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    thisArea->networkLSAList,
                    ospf->routerId,
                    interfaceId);

    // Start constructing the LSA
    LSA = (Ospfv3NetworkLSA* ) MEM_malloc(
                                sizeof(Ospfv3NetworkLSA)
                                + (sizeof(unsigned int) *  count));

    memset(LSA, 0, sizeof(Ospfv3NetworkLSA) + (sizeof(unsigned int)* count));

    if (!Ospfv3CreateLSHeader(
            node,
            ospf->pInterface[interfaceId].areaId,
            OSPFv3_NETWORK,
            &LSA->LSHeader,
            oldLSHeader))
    {
        MEM_free(attachedRouters);

        MEM_free(LSA);

        Ospfv3ScheduleNetworkLSA(node, interfaceId, FALSE);

        return;
    }

    if (flush)
    {
        LSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }

    LSA->LSHeader.length =
        (short) (sizeof(Ospfv3NetworkLSA) + (sizeof(unsigned int) * count));

    // LSA->LSHeader.linkStateID =
    LSA->LSHeader.linkStateId = interfaceId;

    // Copy my link information to the LSA.
    memcpy(
        LSA + 1,
        attachedRouters,
        sizeof(unsigned int) * count);

    // Note LSA Origination time
    ospf->pInterface[interfaceId].networkLSAOriginateTime =
        node->getNodeTime();

    ospf->pInterface[interfaceId].networkLSTimer = FALSE;

#ifdef OSPFv3_DEBUG_LSDBErr
    {
        Ospfv3PrintLSA((char* ) LSA);

        printf("    now adding originated LSA to own LSDB\n");
    }
#endif

    if (Ospfv3InstallLSAInLSDB(node, thisArea->networkLSAList, (char* ) LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout, "\n    Flood self originated Network LSA\n");
    }
#endif

    // Flood LSA
    Ospfv3FloodLSAThroughArea(
        node,
        (char* ) LSA,
        ANY_DEST,
        thisArea->areaId);

#ifdef OSPFv3_DEBUG_LSDBErr
    {
        Ospfv3PrintNetworkLSAList(node);
    }
#endif

    ospf->stats.numNetLSAOriginate++;

    MEM_free(attachedRouters);

    MEM_free(LSA);
}

// /**
// FUNCTION   :: Ospfv3OriginateASExternalLSA
// LAYER      :: NETWORK
// PURPOSE    :: To Originate AS External LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +destAddress:  in6_addr : Dest Address.
// +prefixLength:  unsigned char : Dest Prefic Length.
// +nextHopAddress:  in6_addr : Next Hop Address.
// +external2Cost:  int : External Type-2 cost.
// +flush:  BOOL : Flush or Not Flush.
// RETURN     :: void : NULL.
static
void Ospfv3OriginateASExternalLSA(
    Node* node,
    in6_addr destAddress,
    unsigned char prefixLength,
    in6_addr nextHopAddress,
    int external2Cost,
    BOOL flush)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    BOOL retVal = FALSE;

    char* LSA = (char* ) MEM_malloc(sizeof(Ospfv3ASexternalLSA));

    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    Ospfv3ASexternalLSA* externalLSA = (Ospfv3ASexternalLSA* )LSHeader;
    unsigned int linkStateId = 0;

    memset(LSA, 0, sizeof(Ospfv3ASexternalLSA));

    ERROR_Assert(ospf->asBoundaryRouter, "Router is not an ASBR.\n");

    linkStateId = Ospfv3GetLSIdFromPrefix(node, destAddress);

    if (linkStateId == ANY_DEST)
    {
        linkStateId = Ospfv3CreateLSId(node, destAddress);
    }

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    ospf->asExternalLSAList,
                    ospf->routerId,
                    linkStateId);

    if (external2Cost == NETWORK_UNREACHABLE)
    {
        ERROR_Assert(oldLSHeader, "This situation should not occur.\n");

        Ospfv3FlushLSA(node, (char* ) oldLSHeader, OSPFv3_INVALID_AREA_ID);

        Ospfv3RemoveLSAFromLSDB(
            node,
            (char* ) oldLSHeader,
            OSPFv3_INVALID_AREA_ID);

        MEM_free(LSA);

        return;
    }
    retVal = Ospfv3CreateLSHeader(
                node,
                OSPFv3_INVALID_AREA_ID,
                OSPFv3_AS_EXTERNAL,
                LSHeader,
                oldLSHeader);

    if (!retVal)
    {
        MEM_free(LSA);

        Ospfv3ScheduleASExternalLSA(
            node,
            destAddress,
            prefixLength,
            nextHopAddress,
            external2Cost,
            FALSE);

        return;
    }
    externalLSA->prefixLength = prefixLength;
    externalLSA->OptionsAndMetric = external2Cost;

    COPY_ADDR6(destAddress, externalLSA->addrPrefix);

    externalLSA->OptionsAndMetric |= OSPFv3_EXTERNAL_LSA_E_BIT;
    LSHeader->length = sizeof(Ospfv3ASexternalLSA);
    LSHeader->linkStateId = linkStateId;

    if (flush)
    {
        LSHeader->linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }
    if (Ospfv3InstallLSAInLSDB(node, ospf->asExternalLSAList, LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout, "\nFlood self originated AS-External LSA\n");
    }
#endif

    // Flood LSA
    Ospfv3FloodThroughAS(node, LSA, ANY_DEST);

    ospf->stats.numASExternalLSAOriginate++;

    MEM_free(LSA);
}

// /**
// FUNCTION   :: Ospfv3IsEnabled
// LAYER      :: NETWORK
// PURPOSE    :: Check Interior Gateway protocol(IGP) is OSPF or NOT.
//               If IGP is OSPF then External routing information is injected.
//               If flush is TRUE then External routing Information is
//               flushed from routing domain.
// PARAMETERS ::
//  +node     :  Node*             : Pointer to node, used for
//                                            debugging.
//  +destAddress :  in6_addr*      : External Destination Address
//  +prefixLength :  unsigned char : Destination Subnet Prefix Length
//  +nextHopAddress :  in6_addr    : Next Hop for external Destination
//  +external2Cost :  int          : Type 2 cost
//  +flush :  BOOL                 : Whether to flush External Info or NOT
// RETURN     ::  Return TRUE if runnning OSPF Otherwise FALSE
// **/
BOOL Ospfv3IsEnabled(
    Node* node,
    in6_addr destAddress,
    unsigned char prefixLength,
    in6_addr nextHopAddress,
    int external2Cost,
    BOOL flush)
{
    if (NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv3, NETWORK_IPV6))
    {
        Ospfv3OriginateASExternalLSA(node,
                                 destAddress,
                                 prefixLength,
                                 nextHopAddress,
                                 external2Cost,
                                 flush);
        return TRUE;
    }
    return FALSE;
}
// BGP-OSPF Patch End

// /**
// FUNCTION   :: Ospfv3HandleRoutingProtocolEvent
// LAYER      :: NETWORK
// PURPOSE    :: Self-sent messages are handled in this function. Generally,
//               this function handles the timers required for the
//               implementation of OSPF. This function is called from the
//               funtion Ipv6Layer()in ipv6.cpp.
// PARAMETERS ::
//  +node            :  Node*                : Pointer to node, used for
//                                            debugging.
//  +msg             :  Message*             : Message representing a timer
//                                            event for the protocol.
// RETURN     ::  void : NULL
// **/
void Ospfv3HandleRoutingProtocolEvent(Node* node, Message* msg)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    //Message* newMsg = NULL;
    if (!ospf)
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u got invalid (ospf) timer\n", node->nodeId);

        ERROR_ReportError(errStr);
    }


    switch (msg->eventType)
    {
        // Time to increment the age of each LSA.
        case MSG_ROUTING_OspfIncrementLSAge:
        {

#ifdef OSPFv3_DEBUG_LSDBErr
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u got IncrementLSAge expired at time %s\n",
                    node->nodeId,
                    clockStr);
            }
#endif

            // Increment age of each LSA.
            Ospfv3IncrementLSAge(node);

            // Reschedule LSA increment interval.
            MESSAGE_Send(
                node,
                msg,
                OSPFv3_LSA_INCREMENT_AGE_INTERVAL);

            break;
        }

        // Check to make sure that my neighbor is still there.
        case MSG_ROUTING_OspfInactivityTimer:
        {
            Ospfv3TimerInfo* info =
                (Ospfv3TimerInfo* ) MESSAGE_ReturnInfo(msg);

            Ospfv3Neighbor* neighborInfo = NULL;
            unsigned int interfaceId = ANY_DEST;

#ifdef OSPFv3_DEBUG_HELLOErr
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u got InactivityTimer expired for "
                        "neighbor %d at time %s\n",
                    node->nodeId,
                    info->neighborId,
                    clockStr);

                printf("    sequence number is %u\n", info->sequenceNumber);
            }
#endif

            // Get the neighbor's information from my neighbor list.
            neighborInfo = Ospfv3GetNeighborInfoByRtrId(
                                node,
                                info->neighborId);

            interfaceId = Ospfv3GetInterfaceForThisNeighbor(
                                node,
                                info->neighborId);

            // If neighbor doesn't exist (possibly removed from
            // neighbor list).
            if (neighborInfo == NULL)
            {

#ifdef OSPFv3_DEBUG_HELLOErr
                {
                    printf("    neighbor %d NO LONGER EXISTS\n",
                        info->neighborId);
                }
#endif

                MESSAGE_Free(node, msg);

                break;

            }

#ifdef OSPFv3_DEBUG_HELLOErr
            {
                printf("    Current sequence number is %u\n",
                    neighborInfo->inactivityTimerSequenceNumber);
            }
#endif
            // Timer expired.
            if (info->sequenceNumber
                != neighborInfo->inactivityTimerSequenceNumber)
            {

#ifdef OSPFv3_DEBUG_HELLOErr
                {
                    printf("    Old timer\n");
                }
#endif

                // Timer is an old one, so just ignore.
                MESSAGE_Free(node, msg);

                break;
            }

            // If this timer has not expired already.
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                neighborInfo->neighborId,
                E_InactivityTimer);

#ifdef OSPFv3_DEBUG_HELLOErr
            {
                Ospfv3PrintNeighborList(node);
            }
#endif

            MESSAGE_Free(node, msg);

            break;
        }

        // Check to see if I need to retransmit any Packet.
        case MSG_ROUTING_OspfRxmtTimer:
        {
           Ospfv3TimerInfo* info =
                (Ospfv3TimerInfo* ) MESSAGE_ReturnInfo(msg);

            Ospfv3Neighbor* neighborInfo = NULL;

#ifdef OSPFv3_DEBUG_ERRORS
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u got RxmtTimer expired at time %s\n",
                    node->nodeId,
                    clockStr);

                printf("    sequence number is %u\n", info->sequenceNumber);
            }
#endif

            neighborInfo = Ospfv3GetNeighborInfoByRtrId(
                                node,
                                info->neighborId);

            // If neighbor doesn't exist (possibly removed from
            // neighbor list).
            if (neighborInfo == NULL)
            {

#ifdef OSPFv3_DEBUG_ERRORS
                {
                    printf("    neighbor NO LONGER EXISTS\n");
                }
#endif

                MESSAGE_Free(node, msg);

                break;
            }

            // Retransmit packet to specified neighbor.
            Ospfv3HandleRetransmitTimer(
                node,
                info->sequenceNumber,
                info->neighborId,
                info->type);

            MESSAGE_Free(node, msg);

            break;
        }

        // Time to send out Hello packets.
        case MSG_ROUTING_OspfScheduleHello:
        {
            clocktype delay;

#ifdef OSPFv3_DEBUG_HELLO
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("\nNode %u sending HELLO at time %s\n",
                    node->nodeId,
                    clockStr);
            }
#endif

            // Broadcast Hello packets to neighbors. on each interface
            Ospfv3SendHelloPacket(node);

            // Avoid synchronization
            /*delay =
                (clocktype) (OSPFv3_HELLO_INTERVAL + (RANDOM_erand(ospf->seed)
                             * OSPFv3_BROADCAST_JITTER));*/
            // bug 1189 fix start
            int ifIndex = (int)MESSAGE_GetInstanceId(msg);
            // bug 1189 fix end
            delay =
                (clocktype) (ospf->pInterface[ifIndex].helloInterval
                            + (RANDOM_nrand(ospf->seed)
                            % OSPFv3_BROADCAST_JITTER));

            // Reschedule Hello packet broadcast.
            MESSAGE_Send(node, msg, delay);

            break;
        }

        // Time to originate LSAs.
        case MSG_ROUTING_OspfScheduleLSDB:
        {
            Ospfv3LinkStateType lsType;
            int interfaceId;
            unsigned int areaId;
            BOOL flush = FALSE;
            char* info = NULL;

            if (MESSAGE_ReturnInfoSize(msg) == 0)
            {
                ERROR_Assert(FALSE, "Wrong LSA Scheduled.");

                MESSAGE_Free(node, msg);

                break;
            }

            info = MESSAGE_ReturnInfo(msg);

            memcpy(&lsType, info, sizeof(Ospfv3LinkStateType));

            info += sizeof(Ospfv3LinkStateType);

            if (lsType == OSPFv3_ROUTER)
            {
                memcpy(&areaId, info, sizeof(unsigned int));

                info += sizeof(unsigned int);

                memcpy(&flush, info, sizeof(BOOL));

                Ospfv3OriginateRouterLSAForThisArea(node, areaId, flush);
            }
            else if (lsType == OSPFv3_NETWORK)
            {
                int seqNum;

                memcpy(&interfaceId, info, sizeof(int));

                info += sizeof(int);

                memcpy(&flush, info, sizeof(BOOL));

                info += sizeof(BOOL);

                memcpy(&seqNum, info, sizeof(int));

                // If this timer has not been cancelled yet
                if (ospf->pInterface[interfaceId].networkLSTimerSeqNumber
                    == seqNum)
                {
                    Ospfv3OriginateNetworkLSA(node, interfaceId, flush);
                }
            }
            else if (lsType == OSPFv3_INTER_AREA_PREFIX)
            {
                Ospfv3RoutingTableRow advRt;
                Ospfv3RoutingTableRow* rowPtr = NULL;
                Ospfv3Area* thisArea = NULL;
                int retVal = 0;

                memset(&advRt, 0, sizeof(Ospfv3RoutingTableRow));

                memcpy(&areaId, info, sizeof(unsigned int));

                info += sizeof(unsigned int);

                memcpy(&flush, info, sizeof(BOOL));

                info += sizeof(BOOL);

                memcpy(&(advRt.destAddr), info, sizeof(in6_addr));

                info += sizeof(in6_addr);

                memcpy(&(advRt.prefixLength), info, sizeof(unsigned char));

                info += sizeof(unsigned char);

                memcpy(&(advRt.destType), info, sizeof(Ospfv3DestType));

                info += sizeof(Ospfv3DestType);

                memcpy(&(advRt.metric), info, sizeof(Int32));

                info += sizeof(Int32);

                memcpy(&(advRt.flag), info, sizeof(Ospfv3Flag));

                info += sizeof(Ospfv3Flag);

                memcpy(
                    &(advRt.advertisingRouter),
                    info,
                    sizeof(unsigned int));

                thisArea = Ospfv3GetArea(node, areaId);

                retVal = Ospfv3CheckForInterAreaPrefixLSAValidity(
                            node,
                            &advRt,
                            thisArea->areaId,
                            &rowPtr);

                if (retVal == 1)
                {
                    in6_addr defaultDestAddr;

                    memset(&defaultDestAddr, 0, sizeof(in6_addr));

                    if (CMP_ADDR6(defaultDestAddr, advRt.destAddr) == 0)
                    {
                        ERROR_Assert(!thisArea->transitCapability,
                                    "Default route should be advertised in "
                                    "stub area\n");

                        advRt.metric = thisArea->stubDefaultCost;
                        advRt.flag = OSPFv3_ROUTE_NO_CHANGE;
                    }
                    else
                    {
                        advRt.metric = rowPtr->metric;
                        advRt.flag = rowPtr->flag;
                    }
                }
                else if (retVal == -1)
                {
                    Ospfv3AreaAddressRange* addrInfo;

                    addrInfo = Ospfv3GetAddrRangeInfoForAllArea(
                                    node,
                                    advRt.destAddr);

                    ERROR_Assert(addrInfo
                                && (CMP_ADDR6(
                                        addrInfo->addressPrefix,
                                        advRt.destAddr) == 0),
                                "Scheduled for wrong summary information\n");

                    advRt.metric = Ospfv3GetMaxMetricForThisRange(
                                        node,
                                        &ospf->routingTable,
                                        addrInfo);

                    advRt.flag = OSPFv3_ROUTE_NO_CHANGE;
                }
                else
                {
                    MESSAGE_Free(node, msg);

                    break;
                }

                Ospfv3OriginateInterAreaPrefixLSA(
                    node,
                    &advRt,
                    thisArea,
                    flush);
            }
            else if (lsType == OSPFv3_AS_EXTERNAL)
            {
                in6_addr destAddress;
                unsigned char prefixLength = 0;
                in6_addr nextHopAddress;
                int external2Cost = 0;

                memcpy(&destAddress, info, sizeof(in6_addr));

                info += sizeof(in6_addr);

                memcpy(&prefixLength, info, sizeof(unsigned char));

                info += sizeof(unsigned char);

                memcpy(&nextHopAddress, info, sizeof(in6_addr));

                info += sizeof(in6_addr);

                memcpy(&external2Cost, info, sizeof(int));

                info += sizeof(int);

                memcpy(&flush, info, sizeof(BOOL));

                Ospfv3OriginateASExternalLSA(
                    node,
                    destAddress,
                    prefixLength,
                    nextHopAddress,
                    external2Cost,
                    flush);
            }
            else if (lsType == OSPFv3_LINK)
            {
                unsigned int interfaceId;

                memcpy(&interfaceId, info, sizeof(unsigned int));

                info += sizeof(unsigned int);

                memcpy(&flush, info, sizeof(BOOL));

                Ospfv3OriginateLinkLSAForThisInterface(
                    node,
                    interfaceId,
                    flush);
            }
            else if (lsType == OSPFv3_INTRA_AREA_PREFIX)
            {
                unsigned int refLSType = 0;

                memcpy(&areaId, info, sizeof(unsigned int));

                info += sizeof(unsigned int);

                memcpy(&flush, info, sizeof(BOOL));

                info += sizeof(BOOL);

                memcpy(&refLSType, info, sizeof(BOOL));

                if (refLSType == OSPFv3_ROUTER)
                {
                    Ospfv3OriginateIntraAreaPrefixLSAForRouter(
                        node,
                        areaId,
                        flush);
                }
                else if (refLSType == OSPFv3_NETWORK)
                {
                    // Link Local Address not used.
                }
                else
                {
                    ERROR_Assert(FALSE, "\nWrong Reference LS Type");
                }
            }

            MESSAGE_Free(node, msg);

            break;
        }

        // Single Shot Wait Timer
        case MSG_ROUTING_OspfWaitTimer:
        {
           int interfaceId = *((int* ) MESSAGE_ReturnInfo(msg));

#ifdef OSPFv3_DEBUG_ISM
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                fprintf(stdout,
                        "Node %u, Interface %d: Wait Timer Expire "
                        "at time %s\n",
                    node->nodeId,
                    interfaceId,
                    clockStr);
            }
#endif

            Ospfv3HandleInterfaceEvent(node, interfaceId, E_WaitTimer);

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_ROUTING_OspfFloodTimer:
        {
            int interfaceId;

            memcpy(&interfaceId, MESSAGE_ReturnInfo(msg), sizeof(int));

            if (ospf->pInterface[interfaceId].floodTimerSet == FALSE)
            {
                MESSAGE_Free(node, msg);

                break;
            }

            Ospfv3SendLSUpdate(node, interfaceId);

            ospf->pInterface[interfaceId].floodTimerSet = FALSE;

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_ROUTING_OspfDelayedAckTimer:
        {
            int interfaceId;
            ListItem* listItem = NULL;
            int count;
            Ospfv3LinkStateHeader* payload = NULL;
            int maxCount;

            memcpy(&interfaceId, MESSAGE_ReturnInfo(msg), sizeof(int));

            if (ospf->pInterface[interfaceId].delayedAckTimer == FALSE)
            {
                MESSAGE_Free(node, msg);

                break;
            }

            maxCount = (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
                        - sizeof(Ospfv3LinkStateAckPacket))
                        / sizeof(Ospfv3LinkStateHeader);

            payload = (Ospfv3LinkStateHeader* )
                MEM_malloc(maxCount * sizeof(Ospfv3LinkStateHeader));

            count = 0;

            while (!ListIsEmpty(
                        node,
                        ospf->pInterface[interfaceId].delayedAckList))
            {
                listItem =
                    ospf->pInterface[interfaceId].delayedAckList->first;

                if (count == maxCount)
                {
                    Ospfv3SendAckPacket(
                        node,
                        (char* ) payload,
                        count,
                        ANY_DEST,
                        interfaceId);

                    // Reset variables
                    count = 0;
                    continue;
                }

                memcpy(
                    &payload[count],
                    listItem->data,
                    sizeof(Ospfv3LinkStateHeader));

                count++;

                // Remove item from list
                ListGet(
                    node,
                    ospf->pInterface[interfaceId].delayedAckList,
                    listItem,
                    TRUE,
                    FALSE);
            }

            Ospfv3SendAckPacket(
                node,
                (char* )payload,
                count,
                ANY_DEST,
                interfaceId);

            ospf->pInterface[interfaceId].delayedAckTimer = FALSE;

            MEM_free(payload);

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_ROUTING_OspfScheduleSPF:
        {
            ospf->SPFTimer = node->getNodeTime();

            Ospfv3FindShortestPath(node);

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_ROUTING_OspfNeighborEvent:
        {
            Ospfv3NSMTimerInfo* NSMTimerInfo =
                (Ospfv3NSMTimerInfo* ) MESSAGE_ReturnInfo(msg);

            Ospfv3HandleNeighborEvent(
                node,
                NSMTimerInfo->interfaceId,
                NSMTimerInfo->neighborId,
                NSMTimerInfo->nbrEvent);

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_ROUTING_OspfInterfaceEvent:
        {
            Ospfv3ISMTimerInfo* ISMTimerInfo =
                (Ospfv3ISMTimerInfo* ) MESSAGE_ReturnInfo(msg);

            Ospfv3HandleInterfaceEvent(
                node,
                ISMTimerInfo->interfaceId,
                ISMTimerInfo->intfEvent);

            ospf->pInterface[ISMTimerInfo->interfaceId].ISMTimer = FALSE;

            MESSAGE_Free(node, msg);

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];

            // Shouldn't get here.
            sprintf(errStr,
                    "Node %u: Got invalid ospf timer\n",
                node->nodeId);

            ERROR_Assert(FALSE, errStr);
        }
    }// end switch
}// Function End

// /**
// FUNCTION   :: Ospfv3RouterFullyAdjacentWithDR
// LAYER      :: NETWORK
// PURPOSE    :: To check if the Router is Fully Adjacent With the DR.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// RETURN     :: TRUE if fully adjacent with DR, FALSE otherwise.
// **/
static
BOOL Ospfv3RouterFullyAdjacentWithDR(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* neighborInfo = NULL;
    ListItem* listItem = ospf->pInterface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv3Neighbor* ) listItem->data;

        if (neighborInfo->neighborId
            == ospf->pInterface[interfaceId].designatedRouter
            && neighborInfo->state == S_Full)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3GetDRInterfaceId
// LAYER      :: NETWORK
// PURPOSE    :: Find Interface Id advertised by DR..
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  int : Interface Id.
// RETURN     :: Return Interface Id if found,
//                 otherwise return ANY_DEST.
// **/
static
unsigned int Ospfv3GetDRInterfaceId(Node* node, unsigned int interfaceId)
{

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    ListItem* tempListItem =
        ospf->pInterface[interfaceId].neighborList->first;
    Ospfv3Neighbor* tempNeighborInfo = NULL;

    if (ospf->routerId == ospf->pInterface[interfaceId].designatedRouter)
    {
        return interfaceId;
    }
    else
    {
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

            ERROR_Assert(tempNeighborInfo,
                        "Neighbor not found in the Neighbor list\n");

            if (tempNeighborInfo->neighborId
                == ospf->pInterface[interfaceId].designatedRouter)
            {
                return tempNeighborInfo->neighborInterfaceId;
            }
            tempListItem = tempListItem->next;
        }
    }
    return ANY_DEST;
}// Function End

// /**
// FUNCTION   :: Ospfv3AddType2Link
// LAYER      :: NETWORK
// PURPOSE    :: To Add Type-2 Link
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  unsigned int : Interface Id.
// +neighborInterfaceId: unsigned int : Neighbor Id.
// +linkInfo:  Ospfv3LinkInfo** : Pointer to Pointer to Link Info.
// RETURN     :: void : NULL
// **/
static
void Ospfv3AddType2Link(
    Node* node,
    unsigned int interfaceId,
    unsigned int neighborInterfaceId,
    Ospfv3LinkInfo** linkInfo)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    (*linkInfo)->type = (unsigned char) OSPFv3_TRANSIT;
    (*linkInfo)->reserved = 0;
    (*linkInfo)->metric =
        (unsigned short) ospf->pInterface[interfaceId].outputCost;
    (*linkInfo)->interfaceId =
        ospf->pInterface[interfaceId].interfaceId;
    (*linkInfo)->neighborInterfaceId = neighborInterfaceId;
    (*linkInfo)->neighborRouterId =
        ospf->pInterface[interfaceId].designatedRouter;
    (*linkInfo)++;
}

// /**
// FUNCTION   :: Ospfv3AddType1Link
// LAYER      :: NETWORK
// PURPOSE    :: To Add type 1 link for originating LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +interfaceId:  unsigned int : Interface Id.
// +neighborInfo: Ospfv3Neighbor* : Pointer to Neighbor.
// +linkInfo:  Ospfv3LinkInfo** : Pointer to Pointer to Link Info.
// RETURN     :: void : NULL
// **/
static
void Ospfv3AddType1Link(
    Node* node,
    unsigned int interfaceId,
    Ospfv3Neighbor* neighborInfo,
    Ospfv3LinkInfo**linkInfo)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    (*linkInfo)->type = (unsigned char) OSPFv3_POINT_TO_POINT;
    (*linkInfo)->reserved = 0;
    (*linkInfo)->metric =
        (unsigned short) ospf->pInterface[interfaceId].outputCost;
    (*linkInfo)->interfaceId = ospf->pInterface[interfaceId].interfaceId;
    (*linkInfo)->neighborInterfaceId = neighborInfo->neighborInterfaceId;
    (*linkInfo)->neighborRouterId= neighborInfo->neighborId;
    (*linkInfo)++;
}

// /**
// FUNCTION   :: Ospfv3OriginateRouterLSAForThisArea
// LAYER      :: NETWORK
// PURPOSE    :: Originate router LSA for the specified area. If areaId
//               passed as OSPFv3_SINGLE_AREA_ID, then consider total
//               domain as single area, and include all functional
//               interfaces.
// PARAMETERS ::
// +node      :  Node* : Pointer to node.
// +areaId    :  unsigned int : Area Id.
// +flush     :  BOOL : Flush or not Flush.
// RETURN     :: void : NULL
// **/
void Ospfv3OriginateRouterLSAForThisArea(
    Node* node,
    unsigned int areaId,
    BOOL flush)
{
    int i = 0;
    int count = 0;
    int listSize = 0;
    ListItem* tempListItem = NULL;
    Ospfv3Neighbor* tempNeighborInfo = NULL;
    Ospfv3RouterLSA* LSA = NULL;
    Ospfv3LinkStateHeader* oldLSHeader = NULL;
    Ospfv3LinkInfo* linkList = NULL;
    Ospfv3LinkInfo* tempWorkingPointer = NULL;

    Ospfv3Data* ospf = (Ospfv3Data* )
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv3, NETWORK_IPV6);

    Ospfv3Area* thisArea = Ospfv3GetArea(node, areaId);

    listSize = 2 * (ospf->neighborCount + node->numberInterfaces);

    linkList =
        (Ospfv3LinkInfo* ) MEM_malloc(sizeof(Ospfv3LinkInfo) * listSize);

    memset(linkList, 0, sizeof(Ospfv3LinkInfo) * listSize);

    tempWorkingPointer = linkList;

#ifdef OSPFv3_DEBUG_LSDB
    {
        char clockStr[MAX_STRING_LENGTH];
        char areaStr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(areaId, areaStr);
        printf("Node %u originating Router LSA for area %s at"
                " time %s\n",
            node->nodeId,
            areaStr,
            clockStr);
    }
#endif

    // Look at each of my interface...
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // If the attached network does not belong to this area, no links are
        // added to the LSA, and the next interface should be examined.
        if ((areaId != OSPFv3_SINGLE_AREA_ID)
            && (!Ospfv3InterfaceBelongToThisArea(node, areaId, i)))
        {
            continue;
        }

        // If interface state is down, no link should be added
        if (ospf->pInterface[i].state == S_Down)
        {
            continue;
        }

        // If interface is a broadcast network.
        if (ospf->pInterface[i].type == OSPFv3_BROADCAST_INTERFACE)
        {

#ifdef OSPFv3_DEBUG_LSDBErr
            {
                printf("    interface %d is a broadcast network\n", i);
            }
#endif

            if (((ospf->pInterface[i].state != S_DR)
                && (Ospfv3RouterFullyAdjacentWithDR(node, i)))
                || ((ospf->pInterface[i].state == S_DR)
                && (Ospfv3DRFullyAdjacentWithAnyRouter(node, i))))
            {
                unsigned int interfaceIdDR = Ospfv3GetDRInterfaceId(node, i);

                ERROR_Assert(interfaceIdDR != ANY_DEST,
                            "Designated Router interface Id not found\n");

                // Add type 2 link
                Ospfv3AddType2Link(
                    node,
                    i,
                    interfaceIdDR,
                    &tempWorkingPointer);
            }
            count++;
        }

        // If interface is Point-to-Multipoint
        else if (ospf->pInterface[i].type
                == OSPFv3_POINT_TO_MULTIPOINT_INTERFACE)
        {
            // For each fully adjacent neighbor add an type1 link.
            tempListItem = ospf->pInterface[i].neighborList->first;

            while (tempListItem)
            {
                tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

                ERROR_Assert(tempNeighborInfo,
                            "Neighbor not found in the Neighbor list\n");

                // If neighbor is fully adjacent, add a type1 link
                if (tempNeighborInfo->state == S_Full)
                {
                    // Add Type 1 link
                    Ospfv3AddType1Link(
                        node,
                        i,
                        tempNeighborInfo,
                        &tempWorkingPointer);

                    count++;
                }
                tempListItem = tempListItem->next;
            }
        }

        // If interface is a point-to-point network.
        else
        {

#ifdef OSPFv3_DEBUG_LSDBErr
            {
                printf("    interface %d is a point-to-point network\n", i);
            }
#endif

            tempListItem = ospf->pInterface[i].neighborList->first;

            // Get all my neighbors information from my neighbor list.
            while (tempListItem)
            {
                tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

                //assert (tempNeighborInfo != NULL);
                ERROR_Assert(tempNeighborInfo,
                            "Neighbor not found in the Neighbor list\n");

#ifdef OSPFv3_DEBUG_LSDBErr
                {
                    char addrString[MAX_STRING_LENGTH];
                    in6_addr address;

                    COPY_ADDR6(tempNeighborInfo->neighborIPAddress, address);

                    IO_ConvertIpAddressToString(&address, addrString);

                    printf("    Neighbor %s, State = %d\n",
                        addrString,
                        tempNeighborInfo->state);
                }
#endif

                // If neighbor is fully adjacent, add a type1 link
                if (tempNeighborInfo->state == S_Full)
                {
                    Ospfv3AddType1Link(
                        node,
                        i,
                        tempNeighborInfo,
                        &tempWorkingPointer);

                    count++;
                }
                tempListItem = tempListItem->next;
            }
        }
    }

#ifdef OSPFv3_DEBUG_LSDBErr
    {
        printf("    total link entries are %d\n", count);
    }
#endif

    ERROR_Assert(count <= listSize, "Calculation of listSize is wrong\n");

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    thisArea->routerLSAList,
                    ospf->routerId,
                    ospf->routerId);

    // Start constructing the Router LSA.
    LSA = (Ospfv3RouterLSA* ) MEM_malloc(sizeof(Ospfv3RouterLSA)
                                        + (sizeof(Ospfv3LinkInfo) * count));
    memset(LSA,
        0,
        sizeof(Ospfv3RouterLSA) + (sizeof(Ospfv3LinkInfo) * count));

    if (!Ospfv3CreateLSHeader(
            node,
            areaId,
            OSPFv3_ROUTER,
            &LSA->LSHeader,
            oldLSHeader) )
    {
        MEM_free(linkList);

        MEM_free(LSA);

        Ospfv3ScheduleRouterLSA(node, areaId, FALSE);

        return;
    }
    if (flush)
    {
        LSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }
    LSA->LSHeader.linkStateId = ospf->routerId;
    LSA->LSHeader.length =
        (short) (sizeof(Ospfv3RouterLSA) + (sizeof(Ospfv3LinkInfo) * count));

    OSPFv3_ClearRouterLSAReservedBits(LSA);

    // Clear W-bit.
    LSA->bitsAndOptions &= (~OSPFv3_W_BIT);
    LSA->bitsAndOptions &= (~OSPFv3_V_BIT);

    // BGP-OSPF Patch Start
    if (ospf->asBoundaryRouter == TRUE)
    {
        LSA->bitsAndOptions |= OSPFv3_E_BIT;
    }
    else
    {
        LSA->bitsAndOptions &= (~OSPFv3_E_BIT);
    }
    // BGP-OSPF Patch End

    if (ospf->areaBorderRouter == TRUE)
    {
        LSA->bitsAndOptions |= OSPFv3_B_BIT;
    }
    else
    {
        LSA->bitsAndOptions &= (~OSPFv3_B_BIT);
    }

    // Assign optional capabilities.
    // Set v6 optional bit.
    LSA->bitsAndOptions |= OSPFv3_OPTIONS_V6_BIT;

    // If Area is Stub Area Clear E-bit
    if (thisArea->externalRoutingCapability == TRUE)
    {
        LSA->bitsAndOptions |= OSPFv3_OPTIONS_E_BIT;
    }
    else
    {
        LSA->bitsAndOptions &= (~OSPFv3_OPTIONS_E_BIT);
    }

    // Currently MOSPF not supported, so clear MC bit.
    LSA->bitsAndOptions &= (~OSPFv3_OPTIONS_MC_BIT);

    // NSSA area not supported, so clear N-bit.
    LSA->bitsAndOptions &= (~OSPFv3_OPTIONS_N_BIT);

    // R-bit is set for router LSA
    LSA->bitsAndOptions |= OSPFv3_OPTIONS_R_BIT;

    // Clear DC-bit
    LSA->bitsAndOptions &= (~OSPFv3_OPTIONS_DC_BIT);

    // Copy my link information into the LSA.
    memcpy(
        LSA + 1,
        linkList,
        sizeof(Ospfv3LinkInfo) * count);

    // Note LSA Origination time
    thisArea->routerLSAOriginateTime = node->getNodeTime();

    thisArea->routerLSTimer = FALSE;

#ifdef OSPFv3_DEBUG_LSDB
    {
        printf("    now adding originated LSA to own LSDB\n");
    }
#endif

    if (Ospfv3InstallLSAInLSDB(node, thisArea->routerLSAList, (char* ) LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout, "Node %u Flood self originated Router LSA\n",
            node->nodeId);
    }
#endif

    // Flood LSA
    Ospfv3FloodLSAThroughArea(node, (char* )LSA, ANY_DEST, areaId);

    ospf->stats.numRtrLSAOriginate++;

    MEM_free(linkList);

    MEM_free(LSA);

}//Function End

// /**
// FUNCTION   :: Ospfv3RemoveLSAFromRetxList
// LAYER      :: NETWORK
// PURPOSE    :: Remove LSA from Retransmission List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +linkStateType:  short : LS type.
// +linkStateId:  unsigned int : LS Id.
// +advertisingRouter:  unsigned int : Advertising Router Id.
// +areaId:  unsigned int : Area Id.
// RETURN     :: void : NULL
// **/
static
void Ospfv3RemoveLSAFromRetxList(
    Node* node,
    short linkStateType,
    unsigned int linkStateId,
    unsigned int advertisingRouter,
    unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i;
    ListItem* tempListItem = NULL;
    ListItem* retxItem = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;

    // Look at each of my attached interface.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
        {
            continue;
        }

        // Skip the interface if it doesn't belong to specified area
        if (ospf->pInterface[i].areaId != areaId)
        {
            continue;
        }
        tempListItem = ospf->pInterface[i].neighborList->first;

        // Get the neighbors for each interface.
        while (tempListItem)
        {
            nbrInfo = (Ospfv3Neighbor* ) tempListItem->data;

            ERROR_Assert(nbrInfo,
                "Neighbor not found in the Neighbor list\n");

            retxItem = nbrInfo->linkStateRetxList->first;

            // Look for LSA in retransmission list of each neighbor.
            while (retxItem)
            {
                LSHeader = (Ospfv3LinkStateHeader* ) retxItem->data;

                // If LSA exists in retransmission list, remove it.
                if ((LSHeader->linkStateType == linkStateType)
                    && (LSHeader->linkStateId == linkStateId)
                    && (LSHeader->advertisingRouter == advertisingRouter))
                {
                    ListItem* deleteItem = retxItem;
                    retxItem = retxItem->next;

#ifdef OSPFv3_DEBUG_FLOODErr
                    {
                        printf("    Node %u: Removing LSA with seqno 0x%x"
                                " from rext list of nbr 0x%x\n",
                            node->nodeId,
                            LSHeader->linkStateSequenceNumber,
                            nbrInfo->neighborId);

                        printf("    Type = %d, ID = 0x%x, advRtr = 0x%x\n",
                            LSHeader->linkStateType,
                            LSHeader->linkStateId,
                            LSHeader->advertisingRouter);
                    }
#endif

                    // Remove acknowledged LSA.
                    ListGet(
                        node,
                        nbrInfo->linkStateRetxList,
                        deleteItem,
                        TRUE,
                        FALSE);
                }
                else
                {
                    retxItem = retxItem->next;
                }
            }

            if ((ListIsEmpty(node, nbrInfo->linkStateRetxList))
                && (nbrInfo->LSRetxTimer == TRUE))
            {
                nbrInfo->LSRetxTimer = FALSE;
                nbrInfo->rxmtTimerSequenceNumber++;
            }
            tempListItem = tempListItem->next;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3SearchRequestList
// LAYER      :: NETWORK
// PURPOSE    :: To Search Request List for LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list:  LinkedList* : Pointer to list.
// +LSHeader:  Ospfv3LinkStateHeader* : Pointer to LS header.
// RETURN     :: Return LSA if found, otherwise return NULL
// **/
static
Ospfv3LinkStateHeader* Ospfv3SearchRequestList(
    Node* node,
    LinkedList* list,
    Ospfv3LinkStateHeader* LSHeader)
{
    ListItem* listItem = list->first;

    while (listItem)
    {
        Ospfv3LSReqItem* itemInfo = (Ospfv3LSReqItem* ) listItem->data;
        Ospfv3LinkStateHeader* listLSHeader = itemInfo->LSHeader;

        if ((LSHeader->linkStateType == listLSHeader->linkStateType)
            && (LSHeader->linkStateId == listLSHeader->linkStateId)
            && (LSHeader->advertisingRouter
            == listLSHeader->advertisingRouter))
        {
            return listLSHeader;
        }
        listItem = listItem->next;
    }
    return NULL;
}

// /**
// FUNCTION   :: Ospfv3CompareLSA
// LAYER      :: NETWORK
// PURPOSE    :: Check whether the received LSA is more recent.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +LSHeader:  Ospfv3LinkStateHeader* : Pointer to LS header.
// +oldLSHeader:  Ospfv3LinkStateHeader* : Pointer to old LS header.
// RETURN     :: 1 if the first LSA is more recent,
//            :: -1 if the second LSA is more recent,
//            :: 0 if the two LSAs are sam
// **/
static
int Ospfv3CompareLSA(
    Node* node,
    Ospfv3LinkStateHeader* LSHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{
    // If LSA have newer sequence number
    if (LSHeader->linkStateSequenceNumber
        > oldLSHeader->linkStateSequenceNumber)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "At Node %d Sequence number of new LSA is newer.\n",
                node->nodeId);

        }
#endif

        return 1;
    }
    else if (oldLSHeader->linkStateSequenceNumber
            > LSHeader->linkStateSequenceNumber)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "At Node %d Sequence number of new LSA is older.\n",
                node->nodeId);
        }
#endif

        return -1;
    }

    // If only the new LSA have age equal to LSMaxAge
    else if ((LSHeader->linkStateAge >= (OSPFv3_LSA_MAX_AGE / SECOND))
            && (oldLSHeader->linkStateAge < (OSPFv3_LSA_MAX_AGE / SECOND)))
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "At Node %d Sequence number of old LSA and new LSA is"
                    " same and new LSA's age is equal to LSAMaxAge\n",
                node->nodeId);
        }
#endif

        return 1;
    }
    else if ((oldLSHeader->linkStateAge >= (OSPFv3_LSA_MAX_AGE / SECOND))
            && (LSHeader->linkStateAge < (OSPFv3_LSA_MAX_AGE / SECOND)))
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "Sequence number is the same and old LSA's"
                    " age is equal to LSAMaxAge\n");
        }
#endif

        return -1;
    }

    // If the LS age fields of two instances differ by more than MaxAgeDiff,
    // the instance having the smaller LS age is considered to be more
    // recent.
    else if ((abs(LSHeader->linkStateAge - oldLSHeader->linkStateAge)
            > (OSPFv3_MAX_AGE_DIFF / SECOND))
            && (LSHeader->linkStateAge < oldLSHeader->linkStateAge))
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout, "Sequence number is equal, but new LSA has"
                " smaller LS age\n");
        }
#endif

        return 1;
    }
    else if ((abs(LSHeader->linkStateAge - oldLSHeader->linkStateAge)
            > (OSPFv3_MAX_AGE_DIFF / SECOND))
            && (LSHeader->linkStateAge > oldLSHeader->linkStateAge))
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout, "Sequence number is equal, but old LSA"
                " has smaller LS age\n");
        }
#endif

        return -1;
    }

    return 0;
}

// /**
// FUNCTION   :: Ospfv3RemoveFromRequestList
// LAYER      :: NETWORK
// PURPOSE    :: To Remove LSA From Request List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list:  LinkedList* : Pointer to list.
// +LSHeader:  Ospfv3LinkStateHeader* : Pointer to LS header.
// RETURN     :: void : NULL
// **/
static
void Ospfv3RemoveFromRequestList(
    Node* node,
    LinkedList* list,
    Ospfv3LinkStateHeader* LSHeader)
{
    ListItem* listItem = list->first;

    ERROR_Assert(list->size != 0, "Request list is empty\n");

    while (listItem)
    {
        Ospfv3LSReqItem* itemInfo = (Ospfv3LSReqItem* ) listItem->data;
        Ospfv3LinkStateHeader* listLSHeader = itemInfo->LSHeader;

        if ((LSHeader->linkStateType == listLSHeader->linkStateType)
            && (LSHeader->linkStateId == listLSHeader->linkStateId)
            && (LSHeader->advertisingRouter
            == listLSHeader->advertisingRouter))
        {
            // Remove item from list
            MEM_free(itemInfo->LSHeader);

            ListGet(node, list, listItem, TRUE, FALSE);

            break;
        }
        listItem = listItem->next;
    }
}

// /**
// FUNCTION     :: Ospfv3OriginateLinkLSAForThisInterface
// LAYER        :: NETWORK
// PURPOSE      :: To Originate Link LSA For This Interface.
// PARAMETERS   ::
// +node        :  Node*            : Pointer to node.
// +interfaceId :  unsigned int     : Interface Id.
// +flush       :  BOOL             : Flush or Not to Flush.
// RETURN       :: void : NULL
// **/
void Ospfv3OriginateLinkLSAForThisInterface(
    Node* node,
    unsigned int interfaceId,
    BOOL flush)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkLSA* LSA = NULL;
    Ospfv3PrefixInfo info;
    Ospfv3LinkStateHeader* oldLSHeader;

    Ospfv3Area* thisArea = Ospfv3GetArea(
                            node,
                            ospf->pInterface[interfaceId].areaId);

    if (ospf->pInterface[interfaceId].type == OSPFv3_NON_OSPF_INTERFACE)
    {
        return;
    }

    // If interface state is down, no link should be added
    if (ospf->pInterface[interfaceId].state == S_Down)
    {
        return;
    }

    // Get old instance, if any..
    oldLSHeader = Ospfv3LookupLSAList(
                    node,
                    ospf->pInterface[interfaceId].linkLSAList,
                    ospf->routerId,
                    ospf->routerId);

    LSA = (Ospfv3LinkLSA* ) MEM_malloc(sizeof(Ospfv3LinkLSA)
                                       + sizeof(Ospfv3PrefixInfo) * 2);

    memset(LSA, 0, sizeof(Ospfv3LinkLSA) + sizeof(Ospfv3PrefixInfo) * 2);

    if (!Ospfv3CreateLSHeader(
            node,
            ospf->pInterface[interfaceId].areaId,
            OSPFv3_LINK,
            &LSA->LSHeader,
            oldLSHeader) )
    {
        MEM_free(LSA);

        Ospfv3ScheduleLinkLSA(node, interfaceId, FALSE);

        return;
    }

    if (flush)
    {
        LSA->LSHeader.linkStateAge = (OSPFv3_LSA_MAX_AGE / SECOND);
    }

    LSA->LSHeader.linkStateId = ospf->routerId;
    LSA->LSHeader.length =
        (sizeof(Ospfv3LinkLSA) + sizeof(Ospfv3PrefixInfo) * 2);
    LSA->priorityAndOptions = 0;

    // Assign optional capabilities.
    // Set v6 optional bit.
    LSA->priorityAndOptions |= OSPFv3_OPTIONS_V6_BIT;

    // If Area is Stub Area Clear E-bit
    if (thisArea->externalRoutingCapability == TRUE)
    {
        LSA->priorityAndOptions |= OSPFv3_OPTIONS_E_BIT;
    }
    else
    {
        LSA->priorityAndOptions &= (~OSPFv3_OPTIONS_E_BIT);
    }

    // Currently MOSPF not supported, so clear MC bit.
    LSA->priorityAndOptions &= (~OSPFv3_OPTIONS_MC_BIT);

    // NSSA area not supported, so clear N-bit.
    LSA->priorityAndOptions &= (~OSPFv3_OPTIONS_N_BIT);

    // R-bit is set for router LSA
    LSA->priorityAndOptions |= OSPFv3_OPTIONS_R_BIT;

    // Clear DC-bit
    LSA->priorityAndOptions &= (~OSPFv3_OPTIONS_DC_BIT);

    // Set priority
    OSPFv3_SetPriorityFeild(
        &LSA->priorityAndOptions,
        ospf->pInterface[interfaceId].routerPriority);

    LSA->numPrefixes = 2;

    Ipv6GetLinkLocalAddress(node, interfaceId, &LSA->linkLocalAddress);

    info.prefixLength = OSPFv3_CONST_PREFIX_LENGTH;
    info.prefixOptions |= 0x02;
    info.reservedOrMetric = 0;

    Ipv6AutoConfigGetPreferedAddress(node, interfaceId, &info.prefixAddr);
    if (!IS_LINKLADDR6(info.prefixAddr))
    {
        // Copy my link information into the LSA.
        memcpy(LSA + 1, &info, sizeof(Ospfv3PrefixInfo));
    }

    Ipv6GetDeprecatedAddress(node, interfaceId, &info.prefixAddr);
    if (!IS_LINKLADDR6(info.prefixAddr) && IS_ANYADDR6(info.prefixAddr))
    {
        // Copy my link information into the LSA.
        memcpy(LSA + 1, &info, sizeof(Ospfv3PrefixInfo));
    }

    Ipv6GetSiteLocalAddress(node, interfaceId, &info.prefixAddr);

    memcpy(
        (Ospfv3PrefixInfo* ) (LSA + 1) + 1,
        &info,
        sizeof(Ospfv3PrefixInfo));

    // Note LSA Origination time
    ospf->pInterface[interfaceId].linkLSAOriginateTime = FALSE;

    if (Ospfv3InstallLSAInLSDB(
            node,
            ospf->pInterface[interfaceId].linkLSAList,
            (char* ) LSA))
    {
        // I need to recalculate shortest path since my LSDB changed
        Ospfv3ScheduleSPFCalculation(node);
    }

    // Flood LSA
    Ospfv3FloodLSAOnInterface(
        node,
        (char* )LSA,
        ANY_DEST,
        interfaceId);

    ospf->stats.numLinkLSAOriginate++;

    MEM_free(LSA);
}

// /**
// FUNCTION   :: Ospfv3CreateLSHeader
// LAYER      :: NETWORK
// PURPOSE    :: Create LS Header for newly created LSA.
// PARAMETERS ::
// +node:  Node*                              : Pointer to node.
// +areaId:  unsigned int                     : Area Id.
// +linkStateType: Ospfv3LinkStateType        : Type of Link State.
// +linkStateHeader: Ospfv3LinkStateHeader*   : Pointer to Link State Header.
// +oldLSHeader:  Ospfv3LinkStateHeader*      : Pointer to Old Link State
//                                              Header
// RETURN     :: FALSE if old LSA has MAX sequence number, TRUE otherwise.
// **/
BOOL Ospfv3CreateLSHeader(
    Node* node,
    unsigned int areaId,
    Ospfv3LinkStateType linkStateType,
    Ospfv3LinkStateHeader* linkStateHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = NULL;

    // BGP-OSPF Patch Start
    if (linkStateType != OSPFv3_AS_EXTERNAL)
    {
        thisArea = Ospfv3GetArea(node, areaId);

        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }
    // BGP-OSPF Patch End

    memset(linkStateHeader, 0, sizeof(Ospfv3LinkStateHeader));

    linkStateHeader->linkStateAge = 0;
    linkStateHeader->linkStateType = (short) linkStateType;

    // Advertising router and Link State Id is same for router LSA
    linkStateHeader->advertisingRouter = ospf->routerId;
    if (oldLSHeader)
    {
        if (oldLSHeader->linkStateSequenceNumber
            == (signed int) OSPFv3_MAX_SEQUENCE_NUMBER)
        {
            // Sequence number reaches the maximum value. We need to
            // flush this instance first before originating any instance.
            Ospfv3FlushLSA(node, (char* ) oldLSHeader, areaId);

            Ospfv3RemoveLSAFromLSDB(node, (char* ) oldLSHeader, areaId);

            return FALSE;
        }
        linkStateHeader->linkStateSequenceNumber =
            oldLSHeader->linkStateSequenceNumber + 1;
    }
    else
    {
        linkStateHeader->linkStateSequenceNumber =
            (signed int) OSPFv3_INITIAL_SEQUENCE_NUMBER;
    }
    linkStateHeader->linkStateCheckSum = 0x0;
    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3RemoveLSAFromList
// LAYER      :: NETWORK
// PURPOSE    :: To remove AS-External LSA from LSDB call this function
//                 with areaId argument as OSPFv3_INVALID_AREA_ID.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list:  LinkedList* : Pointer to list.
// +LSA:  char* : Pointer to LSA
// RETURN     :: void : NULL
// **/
static
void Ospfv3RemoveLSAFromList(Node* node, LinkedList* list, char* LSA)
{
    ListItem* listItem = list->first;
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    Ospfv3LinkStateHeader* listLSHeader = NULL;

    while (listItem)
    {
        listLSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

        if (listLSHeader->advertisingRouter == LSHeader->advertisingRouter
            && listLSHeader->linkStateId == LSHeader->linkStateId)
        {
            // Remove item
            ListGet(node, list, listItem, TRUE, FALSE);

            break;
        }
        listItem = listItem->next;
    }
}

// /**
// FUNCTION   :: Ospfv3QueueLSAToFlood.
// LAYER      :: NETWORK
// PURPOSE    :: Queue LSA To Flood.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +thisInterface:  Ospfv3Interface* : Pointer to Interface.
// +LSA:  char* : Pointer to LSA
// RETURN     :: void : NULL
// **/
static
void Ospfv3QueueLSAToFlood(
    Node* node,
    Ospfv3Interface* thisInterface,
    char* LSA)
{
    char* newLSA = NULL;
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    ListItem* listItem = thisInterface->updateLSAList->first;

    while (listItem)
    {
        Ospfv3LinkStateHeader* listLSHeader =
            (Ospfv3LinkStateHeader* ) listItem->data;

        // If this LSA present in list
        if (LSHeader->linkStateType == listLSHeader->linkStateType
            && LSHeader->linkStateId == listLSHeader->linkStateId
            && LSHeader->advertisingRouter
            == listLSHeader->advertisingRouter)
        {
            // If this is newer instance add this instance replacing old one
            if (Ospfv3CompareLSA(node, LSHeader, listLSHeader) > 0)
            {
                MEM_free(listItem->data);

                listItem->data = (void* ) Ospfv3CopyLSA(node, LSA);

                listItem->timeStamp = node->getNodeTime();
            }

            return;
        }

        listItem = listItem->next;
    }

    newLSA = Ospfv3CopyLSA(node, LSA);

    // Add to update LSA list.
    ListInsert(
        node,
        thisInterface->updateLSAList,
        node->getNodeTime(),
        (void* ) newLSA);
}

// /**
// FUNCTION     :: Ospfv3FloodLSAOnInterface.
// LAYER        :: NETWORK
// PURPOSE      :: To Flood LSA On Interface.
// PARAMETERS   ::
// +node        :  Node*        : Pointer to node.
// +LSA         :  char*        : Pointer to LSA
// +neighborId  :  unsigned int : Neighbor Id.
// +interfaceId :  unsigned int : Interface Id.
// RETURN       :: void : NULL
// **/
void Ospfv3FloodLSAOnInterface(
    Node* node,
    char* LSA,
    unsigned int neighborId,
    unsigned int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* tempNeighborInfo = NULL;
    ListItem* tempListItem = NULL;
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    // Need to first remove LSA from all neigbhor's
    // retx list before flooding it.
    // This is because we want to use the most
    // up-to-date LSA for retransmission.
    Ospfv3RemoveLSAFromRetxList(
        node,
        LSHeader->linkStateType,
        LSHeader->linkStateId,
        LSHeader->advertisingRouter,
        ospf->pInterface[interfaceId].areaId);

    tempListItem = ospf->pInterface[interfaceId].neighborList->first;

    // Check each neighbor on this interface.
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

        ERROR_Assert(tempNeighborInfo,
                    "Neighbor not found in the Neighbor list\n");

        // RFC2328, Sec-13.3.1.a
        // If neighbor is in a lesser state than exchange,
        // it does not participate in flooding
        if (tempNeighborInfo->state < S_Exchange)
        {
            tempListItem = tempListItem->next;
            continue;
        }

        // RFC2328, Sec-13.3.1.b
        // If neighbor state is Exchange or Loading
        else if (tempNeighborInfo->state != S_Full)
        {
            Ospfv3LinkStateHeader* listLSHeader =
                Ospfv3SearchRequestList(
                    node,
                    tempNeighborInfo->linkStateRequestList,
                    LSHeader);

            if (listLSHeader)
            {
                int moreRecentIndicator = Ospfv3CompareLSA(
                                            node,
                                            LSHeader,
                                            listLSHeader);

                // If the new LSA is less recent
                if (moreRecentIndicator < 0)
                {
                    tempListItem = tempListItem->next;
                    continue;
                }
                // If the two LSAs are same instances
                else if (moreRecentIndicator == 0)
                {
                    Ospfv3RemoveFromRequestList(
                        node,
                        tempNeighborInfo->linkStateRequestList,
                        listLSHeader);

                        if (ListIsEmpty(node,
                            tempNeighborInfo->linkStateRequestList)
                            && (tempNeighborInfo->state == S_Loading))
                        {
                            tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            Ospfv3HandleNeighborEvent(
                                node,
                                interfaceId,
                                tempNeighborInfo->neighborId,
                                E_LoadingDone);
                        }

                    tempListItem = tempListItem->next;
                    continue;
                }
                // New LSA is more recent
                else
                {
                    Ospfv3RemoveFromRequestList(
                        node,
                        tempNeighborInfo->linkStateRequestList,
                        listLSHeader);

                        if (ListIsEmpty(node,
                            tempNeighborInfo->linkStateRequestList)
                            && (tempNeighborInfo->state == S_Loading))
                        {
                            tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            Ospfv3HandleNeighborEvent(
                                node,
                                interfaceId,
                                tempNeighborInfo->neighborId,
                                E_LoadingDone);
                        }
                }
            }
        }

        // RFC2328, Sec-13.3.1.c
        // If LSA received from this neighbor, examine next neighbor
        if (tempNeighborInfo->neighborId == neighborId)
        {
            tempListItem = tempListItem->next;
            continue;
        }

        // RFC2328, Sec-13.3.1.d
        // Add to Retransmission list
        Ospfv3AddToLSRetxList(node, tempNeighborInfo, LSA);

        tempListItem = tempListItem->next;
    }

    Ospfv3QueueLSAToFlood(node, &ospf->pInterface[interfaceId], LSA);

    if (ospf->pInterface[interfaceId].floodTimerSet == FALSE)
    {
        Message* floodMsg = NULL;
        clocktype delay;

        ospf->pInterface[interfaceId].floodTimerSet = TRUE;

        // Set Timer
        floodMsg = MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        ROUTING_PROTOCOL_OSPFv3,
                        MSG_ROUTING_OspfFloodTimer);

        MESSAGE_InfoAlloc(node, floodMsg, sizeof(int));

        memcpy(MESSAGE_ReturnInfo(floodMsg), &interfaceId, sizeof(int));

        delay = (clocktype) (RANDOM_erand(ospf->seed) * OSPFv3_FLOOD_TIMER);

        MESSAGE_Send(node, floodMsg, delay);
    }
}

// /**
// FUNCTION   :: Ospfv3FloodLSAThroughArea
// LAYER      :: NETWORK
// PURPOSE    :: Flood LSA throughout the area.
// PARAMETERS ::
// + node       :  Node*                        : Pointer to node, doing the
//                                                packet trace
// + LSA        :  char*                        : Poiter to an LSA.
// + neighborId :  unsigned int                 : Neighbor ID
// + areaId     :  unsigned int                 : Area Id to which LSA need
//                                                to be flooded.
// RETURN     :: TRUE if LSA flooded back to receiving interface,
//               FALSE otherwise.
// **/
BOOL Ospfv3FloodLSAThroughArea(
    Node* node,
    char* LSA,
    unsigned int neighborId,
    unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int i = 0;
    Ospfv3Neighbor* tempNeighborInfo = NULL;
    ListItem* tempListItem = NULL;

    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    unsigned int inIntf =
        Ospfv3GetInterfaceForThisNeighbor(node, neighborId);

    BOOL floodedBackToRecvIntf = FALSE;

    // Need to first remove LSA from all neigbhor's
    // retx list before flooding it.
    // This is because we want to use the most
    // up-to-date LSA for retransmission.
    Ospfv3RemoveLSAFromRetxList(
        node,
        LSHeader->linkStateType,
        LSHeader->linkStateId,
        LSHeader->advertisingRouter,
        areaId);

    for (i = 0; i < (unsigned int) (node->numberInterfaces); i++)
    {
        BOOL addIntoRetxList = FALSE;

        // If this is not eligible interface, examine next interface
        if ((NetworkIpGetUnicastRoutingProtocolType(node, i, NETWORK_IPV6)
            != ROUTING_PROTOCOL_OSPFv3)
            || (!Ospfv3InterfaceBelongToThisArea(node, areaId, i)))
        {
            continue;
        }

        tempListItem = ospf->pInterface[i].neighborList->first;

        // Check each neighbor on this interface.
        while (tempListItem)
        {
            tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;
            ERROR_Assert(tempNeighborInfo,
                "Neighbor not found in the Neighbor list\n");

            // RFC2328, Sec-13.3.1.a
            // If neighbor is in a lesser state than exchange,
            // it does not participate in flooding
            if (tempNeighborInfo->state < S_Exchange)
            {
                tempListItem = tempListItem->next;
                continue;
            }

            // RFC2328, Sec-13.3.1.b
            // If neighbor state is Exchange or Loading
            else if (tempNeighborInfo->state != S_Full)
            {
                Ospfv3LinkStateHeader* listLSHeader =
                    Ospfv3SearchRequestList(
                        node,
                        tempNeighborInfo->linkStateRequestList,
                        LSHeader);

                if (listLSHeader)
                {
                    int moreRecentIndicator =
                        Ospfv3CompareLSA(node, LSHeader, listLSHeader);

                    // If the new LSA is less recent
                    if (moreRecentIndicator < 0)
                    {
                        tempListItem = tempListItem->next;
                        continue;
                    }
                    // If the two LSAs are same instances
                    else if (moreRecentIndicator == 0)
                    {
                        Ospfv3RemoveFromRequestList(
                            node,
                            tempNeighborInfo->linkStateRequestList,
                            listLSHeader);

                        if (ListIsEmpty(node,
                            tempNeighborInfo->linkStateRequestList)
                            && (tempNeighborInfo->state == S_Loading))
                        {
                           tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            Ospfv3HandleNeighborEvent(
                                node,
                                i,
                                tempNeighborInfo->neighborId,
                                E_LoadingDone);
                        }

                        tempListItem = tempListItem->next;
                        continue;
                    }
                    // New LSA is more recent
                    else
                    {
                        Ospfv3RemoveFromRequestList(
                            node,
                            tempNeighborInfo->linkStateRequestList,
                            listLSHeader);

                        if (ListIsEmpty(node,
                            tempNeighborInfo->linkStateRequestList)
                            && (tempNeighborInfo->state == S_Loading))
                        {
                            tempNeighborInfo->LSReqTimerSequenceNumber += 1;

                            Ospfv3HandleNeighborEvent(
                                node,
                                i,
                                tempNeighborInfo->neighborId,
                                E_LoadingDone);
                        }
                    }
                }
            }

            // RFC2328, Sec-13.3.1.c
            // If LSA received from this neighbor, examine next neighbor
            if (tempNeighborInfo->neighborId == neighborId)
            {
                tempListItem = tempListItem->next;
                continue;
            }

            // RFC2328, Sec-13.3.1.d
            // Add to Retransmission list
            Ospfv3AddToLSRetxList(node, tempNeighborInfo, LSA);

            addIntoRetxList = TRUE;
            tempListItem = tempListItem->next;
        }

        // RFC2328, Sec-13.3.2
        if (!addIntoRetxList)
        {
            continue;
        }

        if ((ospf->pInterface[i].type == OSPFv3_BROADCAST_INTERFACE)
            && (neighborId != ANY_DEST)
            && (i == inIntf))
        {
            // RFC2328, Sec-13.3.3
            // If the new LSA was received on this interface, and it was
            // received from either the Designated Router or the Backup
            // Designated Router, chances are that all the neighbors have
            // received the LSA already. Therefore, examine the next
            // interface.
            if ((ospf->pInterface[i].designatedRouter == neighborId)
                || (ospf->pInterface[i].backupDesignatedRouter
                == neighborId))
            {
                continue;
            }

            // RFC2328, Sec-13.3.4
            // If the new LSA was received on this interface, and the
            // interface state is Backup, examine the next interface. The
            // Designated Router will do the flooding on this interface.
            if (ospf->pInterface[i].state == S_Backup)
            {
                continue;
            }
        }

#ifdef OSPFv3_DEBUG_FLOOD
        {
            char clockStr[MAX_STRING_LENGTH];

            ctoa((node->getNodeTime()) / SECOND, clockStr);

            printf("    Node %u: flood UPDATE PACKET"
                    " on interface %d at time %s\n",
                node->nodeId,
                i,
                clockStr);
        }
#endif

        // Note if we flood the LSA back to receiving interface
        // then this will be used later for sending Ack.
        if ((neighborId != ANY_DEST) && (i == inIntf))
        {
            floodedBackToRecvIntf = TRUE;
        }

        Ospfv3QueueLSAToFlood(node, &ospf->pInterface[i], LSA);

        if (ospf->pInterface[i].floodTimerSet == FALSE)
        {
            Message* floodMsg = NULL;
            clocktype delay;

            ospf->pInterface[i].floodTimerSet = TRUE;

            // Set Timer
            floodMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            ROUTING_PROTOCOL_OSPFv3,
                            MSG_ROUTING_OspfFloodTimer);

            MESSAGE_InfoAlloc(node, floodMsg, sizeof(int));

            memcpy(MESSAGE_ReturnInfo(floodMsg), &i, sizeof(int));

            delay = (clocktype) (RANDOM_erand(ospf->seed) * OSPFv3_FLOOD_TIMER);

            MESSAGE_Send(node, floodMsg, delay);
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3CheckRouterLSABody.
// LAYER      :: NETWORK
// PURPOSE    :: Compare body of two Router LSA for change.
// PARAMETERS ::
// +newRouterLSA:  Ospfv3RouterLSA* : Pointer to New Router LSA.
// +oldRouterLSA:  Ospfv3RouterLSA* : Pointer to Old Router LSA.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckRouterLSABody(
    Ospfv3RouterLSA* newRouterLSA,
    Ospfv3RouterLSA* oldRouterLSA)
{
    int i = 0;
    int j = 0;
    int size = (oldRouterLSA->LSHeader.length - sizeof(Ospfv3LinkStateHeader)
                - sizeof(unsigned int) ) / sizeof(Ospfv3LinkInfo);

    if (size == 0)
    {
        return FALSE;
    }

    // Used to flag same link.
    BOOL* sameFlag = (BOOL* ) MEM_malloc(sizeof(BOOL) * size);

    memset(sameFlag, 0, sizeof(BOOL) * size);

    Ospfv3LinkInfo* newLinkList = (Ospfv3LinkInfo* ) (newRouterLSA + 1);
    Ospfv3LinkInfo* oldLinkList = (Ospfv3LinkInfo* ) (oldRouterLSA + 1);

    if (newRouterLSA->bitsAndOptions != oldRouterLSA->bitsAndOptions)
    {
        return TRUE;
    }
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            if (sameFlag[j] != TRUE)
            {
                if (memcmp(
                        newLinkList + i,
                        oldLinkList + j,
                        sizeof(Ospfv3LinkInfo) == 0))
                {
                    sameFlag[j] = TRUE;
                    break;
                }
            }
        }
        if (j == size)
        {
            MEM_free(sameFlag);

            return TRUE;
        }
    }
    MEM_free(sameFlag);

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3CheckNetworkLSABody.
// LAYER      :: NETWORK
// PURPOSE    :: Compare body of two Network LSA for change.
// PARAMETERS ::
// +newNetworkLSA:  Ospfv3NetworkLSA* : Pointer to New Network LSA.
// +oldNetworkLSA:  Ospfv3NetworkLSA* : Pointer to Old Network LSA.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckNetworkLSABody(
    Ospfv3NetworkLSA* newNetworkLSA,
    Ospfv3NetworkLSA* oldNetworkLSA)
{
    int i = 0;
    int j = 0;
    int size =
        (oldNetworkLSA->LSHeader.length - sizeof(Ospfv3LinkStateHeader)
        - sizeof(unsigned int) ) / sizeof(unsigned int);

    // Used to flag same Attached router.
    BOOL* sameFlag = (BOOL* ) MEM_malloc(sizeof(BOOL) * size);

    memset(sameFlag, 0, sizeof(BOOL) * size);

    unsigned int* newAttachedRouter = (unsigned int* ) (newNetworkLSA + 1);
    unsigned int* oldAttachedRouter = (unsigned int* ) (oldNetworkLSA + 1);

    if (newNetworkLSA->bitsAndOptions != oldNetworkLSA->bitsAndOptions)
    {
        return TRUE;
    }
    for (i = 0; i < size; i++)
    {

        for (j = 0; j < size; j++)
        {
            if (sameFlag[j] != TRUE)
            {
                if (newAttachedRouter[i] == oldAttachedRouter[j])
                {
                    sameFlag[j] = TRUE;
                    break;
                }
            }
        }
        if (j == size)
        {
            MEM_free(sameFlag);

            return TRUE;
        }
    }

    MEM_free(sameFlag);

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3CheckInterAreaPrefixLSABody.
// LAYER      :: NETWORK
// PURPOSE    :: Compare body of two InterAreaPrefix LSA for change.
// PARAMETERS ::
// +newLSA:  void* : Pointer to New LSA.
// +oldLSA:  void* : Pointer to Old LSA.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckInterAreaPrefixLSABody(void* newLSA, void* oldLSA)
{
    if (memcmp(newLSA, oldLSA, sizeof(Ospfv3InterAreaPrefixLSA)) == 0)
    {
        return FALSE;
    }

    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3CheckInterAreaRouterLSABody.
// LAYER      :: NETWORK
// PURPOSE    :: Compare the body of two Inter Area Router LSA.
// PARAMETERS ::
// +newLSHeader:  Ospfv3LinkStateHeader* : Pointer to New Link State Header.
// +oldLSHeader:  Ospfv3LinkStateHeader* : Pointer to Old Link State Header.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckInterAreaRouterLSABody(
    Ospfv3LinkStateHeader* newLSHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{
    if (memcmp(
            newLSHeader + 1,
            oldLSHeader + 1,
            sizeof(unsigned int) * 3) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Ospfv3CheckASExternalLSABody.
// LAYER      :: NETWORK
// PURPOSE    :: Compare the body of two External LSA.
// PARAMETERS ::
// +newLSHeader:  Ospfv3LinkStateHeader* : Pointer to New Link State Header.
// +oldLSHeader:  Ospfv3LinkStateHeader* : Pointer to Old Link State Header.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckASExternalLSABody(
    Ospfv3LinkStateHeader* newLSHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{
    if (memcmp(
            newLSHeader + 1,
            oldLSHeader + 1,
            sizeof(unsigned int) * 2 + sizeof(in6_addr)) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Ospfv3CheckLinkLSA.
// LAYER      :: NETWORK
// PURPOSE    :: Compare the body of Two link LSA.
// PARAMETERS ::
// +newLinkLSA:  Ospfv3LinkLSA* : Pointer to New Link LSA.
// +oldLinkLSA:  Ospfv3LinkLSA* : Pointer to Old Link LSA.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckLinkLSA(Ospfv3LinkLSA* newLinkLSA, Ospfv3LinkLSA* oldLinkLSA)
{
    int i = 0;
    int j = 0;
    int size = (oldLinkLSA->LSHeader.length - sizeof(in6_addr)
                - sizeof(unsigned int) * 2 );

    // Used to flag same Address Prefix.
    BOOL* sameFlag = (BOOL* ) MEM_malloc(sizeof(BOOL) * size);

    memset(sameFlag, 0, sizeof(BOOL) * size);

    Ospfv3PrefixInfo* newPrefixInfo = (Ospfv3PrefixInfo* ) (newLinkLSA + 1);
    Ospfv3PrefixInfo* oldPrefixInfo = (Ospfv3PrefixInfo* ) (oldLinkLSA + 1);

    if (memcmp(
            (Ospfv3LinkStateHeader* ) newLinkLSA + 1,
            (Ospfv3LinkStateHeader* )oldLinkLSA + 1,
            sizeof(in6_addr) + sizeof(unsigned int) * 2) != 0)
    {
        return TRUE;
    }
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            if (sameFlag[j] != TRUE)
            {
                if (memcmp(
                        newPrefixInfo + i,
                        oldPrefixInfo + j,
                        sizeof(Ospfv3PrefixInfo) == 0))
                {
                    sameFlag[j] = TRUE;
                    break;
                }
            }
        }
        if (j == size)
        {
            MEM_free(sameFlag);

            return TRUE;
        }
    }

    MEM_free(sameFlag);

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3CheckIntraAreaPrefixLSA.
// LAYER      :: NETWORK
// PURPOSE    :: Compare the body of an LSA with the already existing LSA.
// PARAMETERS ::
// +newIntraAreaPrefixLSA: Ospfv3IntraAreaPrefixLSA*: Pointer to New
//                                                    Intra Area Prefix LSA.
// +oldIntraAreaPrefixLSA: Ospfv3IntraAreaPrefixLSA*: Pointer to Old
//                                                    Intra Area Prefix LSA.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckIntraAreaPrefixLSA(
    Ospfv3IntraAreaPrefixLSA* newIntraAreaPrefixLSA,
    Ospfv3IntraAreaPrefixLSA* oldIntraAreaPrefixLSA)
{
    int i = 0;
    int j = 0;

    int size = oldIntraAreaPrefixLSA->numPrefixes;

    // Used to flag same Address Prefix.
    BOOL* sameFlag = (BOOL* ) MEM_malloc(sizeof(BOOL) * size);

    memset(sameFlag, 0, sizeof(BOOL) * size);

    Ospfv3PrefixInfo* newPrefixInfo =
        (Ospfv3PrefixInfo* ) (newIntraAreaPrefixLSA + 1);
    Ospfv3PrefixInfo* oldPrefixInfo =
        (Ospfv3PrefixInfo* ) (oldIntraAreaPrefixLSA + 1);

    if (memcmp(
            (Ospfv3LinkStateHeader* ) newIntraAreaPrefixLSA + 1,
            (Ospfv3LinkStateHeader* )oldIntraAreaPrefixLSA + 1,
            sizeof(unsigned int) * 3) != 0)
    {
        return TRUE;
    }
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            if (sameFlag[j] != TRUE)
            {
                if (memcmp(
                        newPrefixInfo + i,
                        oldPrefixInfo + j,
                        sizeof(Ospfv3PrefixInfo) == 0))
                {
                    sameFlag[j] = TRUE;
                    break;
                }
            }
        }
        if (j == size)
        {
            MEM_free(sameFlag);

            return TRUE;
        }
    }

    MEM_free(sameFlag);

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3LSABodyChanged.
// LAYER      :: NETWORK
// PURPOSE    :: Compare the body of an LSA with the already existing LSA.
// PARAMETERS ::
// +newLSHeader: Ospfv3LinkStateHeader* : Pointer to New Link State Header.
// +oldLSHeader: Ospfv3LinkStateHeader* : Pointer to Old Link State Header.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3LSABodyChanged(
    Ospfv3LinkStateHeader* newLSHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{
    BOOL retVal = FALSE;

    switch (newLSHeader->linkStateType)
    {
        case OSPFv3_ROUTER:
        {
            retVal = Ospfv3CheckRouterLSABody(
                        (Ospfv3RouterLSA* )newLSHeader,
                        (Ospfv3RouterLSA* ) oldLSHeader );

            break;
        }
        case OSPFv3_NETWORK:
        {
            retVal = Ospfv3CheckNetworkLSABody(
                        (Ospfv3NetworkLSA* ) newLSHeader,
                        (Ospfv3NetworkLSA* ) oldLSHeader);

            break;
        }
        case OSPFv3_INTER_AREA_PREFIX:
        {
            retVal = Ospfv3CheckInterAreaPrefixLSABody(
                        newLSHeader,
                        oldLSHeader);

            break;
        }
        case OSPFv3_INTER_AREA_ROUTER:
        {
            retVal = Ospfv3CheckInterAreaRouterLSABody(
                        newLSHeader,
                        oldLSHeader);

            break;
        }

        // BGP-OSPF Patch Start
        case OSPFv3_AS_EXTERNAL:
        {
            retVal = Ospfv3CheckASExternalLSABody(newLSHeader, oldLSHeader);

            break;
        }
        // BGP-OSPF Patch End

        case OSPFv3_GROUP_MEMBERSHIP:
        {
            ERROR_Assert(FALSE,
                        "\nGroup-membership-LSA not supported in OSPFv3\n");

            break;
        }
        case OSPFv3_TYPE_7:
        {
            ERROR_Assert(FALSE,
                        "\nType-7 LSA not supported in OSPFv3\n");

            break;
        }
        case OSPFv3_LINK:
        {
            retVal = Ospfv3CheckLinkLSA(
                        (Ospfv3LinkLSA* ) newLSHeader,
                        (Ospfv3LinkLSA* ) oldLSHeader);

            break;
        }
        case OSPFv3_INTRA_AREA_PREFIX:
        {
            retVal = Ospfv3CheckIntraAreaPrefixLSA(
                        (Ospfv3IntraAreaPrefixLSA* ) newLSHeader,
                        (Ospfv3IntraAreaPrefixLSA* ) oldLSHeader);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Not a valid linkStateType\n");
        }
    }

    return retVal;
}

// /**
// FUNCTION   :: Ospfv3LSAsContentsChanged.
// LAYER      :: NETWORK
// PURPOSE    :: Check contents of two LSA for any mismatch. This is used
//               to determine whether we need to recalculate routing table.
// PARAMETERS ::
// +newLSHeader: Ospfv3LinkStateHeader* : Pointer to New Link State Header.
// +oldLSHeader: Ospfv3LinkStateHeader* : Pointer to Old Link State Header.
// RETURN     :: TRUE if body changed, FALSE otherwise.
// **/
static
BOOL Ospfv3LSAsContentsChanged(
    Node* node,
    Ospfv3LinkStateHeader* newLSHeader,
    Ospfv3LinkStateHeader* oldLSHeader)
{

    // If one instance has LSA age MaxAge and other does not
    if (((newLSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
        && (oldLSHeader->linkStateAge != (OSPFv3_LSA_MAX_AGE / SECOND)))
        || ((oldLSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
        && (newLSHeader->linkStateAge != (OSPFv3_LSA_MAX_AGE / SECOND))))
    {
        return TRUE;
    }
    // Else if length field has changed
    else if (newLSHeader->length != oldLSHeader->length)
    {
        return TRUE;
    }
    // The body of the LSA has changed
    else if (Ospfv3LSABodyChanged(newLSHeader, oldLSHeader))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: Ospfv3AssignNewLSAIntoLSOrigin.
// LAYER      :: NETWORK
// PURPOSE    :: Assign newLSA address into the LSOrigin which previously
//               holding the old LSA address.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +oldLSA: char* : Pointer to Old LSA.
// +LSA: char* : Pointer to LSA.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AssignNewLSAIntoLSOrigin(Node* node, char* oldLSA, char* LSA)
{
    int i;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    for (i = 0; i < rtTable->numRows; i++)
    {
        if (rowPtr[i].LSOrigin == oldLSA)
        {
            rowPtr[i].LSOrigin = LSA;
            break;
        }
    }
}

// /**
// FUNCTION   :: Ospfv3InstallLSAInLSDB.
// LAYER      :: NETWORK
// PURPOSE    :: Install LSAs in the database.
// PARAMETERS ::
// + node      :  Node*         : Pointer to node.
// + list      : LinkedList*    : Pointer to linked list.
// + LSA       : char*          : Pointer to LSA.
// RETURN     :: TRUE if LSDB changed, FALSE otherwise.
// **/
BOOL Ospfv3InstallLSAInLSDB(Node* node, LinkedList* list, char* LSA)
{
    Ospfv3LinkStateHeader* listLSHeader = NULL;
    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    ListItem* item = list->first;
    char* newLSA = NULL;
    BOOL retVal = FALSE;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv3LinkStateHeader* ) item->data;

        if ((listLSHeader->advertisingRouter
            == LSHeader->advertisingRouter)
            && (listLSHeader->linkStateId
            == LSHeader->linkStateId))
        {
            break;
        }
        item = item->next;
    }
#ifdef OSPFv3_DEBUG_LSDB
    {
        printf("    Node %d Installing LSA (type = 0x%x) in LSDB\n\n",
            node->nodeId,
            LSHeader->linkStateType);
    }
#endif

    if (LSHeader->linkStateAge >= (OSPFv3_LSA_MAX_AGE / SECOND))
    {
        if (item)
        {
            ListGet(node, list, item, TRUE, FALSE);

            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }
    else
    {
        // LSA found in list
        if (item)
        {
            // Check for difference between old and new instance of LSA
            retVal = Ospfv3LSAsContentsChanged(
                        node,
                        LSHeader,
                        listLSHeader);

            item->timeStamp = node->getNodeTime();

            if (retVal)
            {
                newLSA = Ospfv3CopyLSA(node, LSA);

                item->data = (void* ) newLSA;

                if ((LSHeader->linkStateType == OSPFv3_ROUTER)
                    || (LSHeader->linkStateType == OSPFv3_NETWORK))
                {
                    // Assign newLSA address into the LSOrigin.
                    Ospfv3AssignNewLSAIntoLSOrigin(
                        node,
                        (char* )listLSHeader,
                        newLSA);
                }

                // Free old memory
                MEM_free(listLSHeader);
            }
            else
            {
                memcpy(
                    listLSHeader,
                    LSHeader,
                    LSHeader->length);
            }
        }

        // LSA not found in list
        else
        {
            newLSA = Ospfv3CopyLSA(node, LSA);

            ListInsert(
                node,
                list,
                node->getNodeTime(),
                (void* ) newLSA);

            retVal = TRUE;
        }
    }
    return retVal;
}

// /**
// FUNCTION   :: Ospfv3RemoveLSAFromLSDB.
// LAYER      :: NETWORK
// PURPOSE    :: To remove AS-External LSA from LSDB call this function
//               with areaId argument as OSPFv3_INVALID_AREA_ID.
//               For Link LSA areaId will be interfaceId.
// PARAMETERS ::
// +node      :  Node* : Pointer to node.
// +LSA       : char* : Pointer to LSA.
// +areaId    : unsigned int : Area Id.
// RETURN     :: void : NULL.
// **/
void Ospfv3RemoveLSAFromLSDB(Node* node, char* LSA, unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    Ospfv3Area * thisArea = NULL;

    // BGP-OSPF Patch Start
    if (LSHeader->linkStateType != OSPFv3_AS_EXTERNAL)
    {
        thisArea = Ospfv3GetArea(node, areaId);

        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }
    // BGP-OSPF Patch End

    switch (LSHeader->linkStateType)
    {
        case OSPFv3_ROUTER:
        {
            Ospfv3RemoveLSAFromList(node, thisArea->routerLSAList, LSA);

            break;
        }
        case OSPFv3_NETWORK:
        {
            Ospfv3RemoveLSAFromList(node, thisArea->networkLSAList, LSA);

            break;
        }
        case OSPFv3_INTER_AREA_PREFIX:
        {
            Ospfv3RemoveLSAFromList(
                node,
                thisArea->interAreaPrefixLSAList,
                LSA);

            break;
        }
        case OSPFv3_INTER_AREA_ROUTER:
        {
            Ospfv3RemoveLSAFromList(
                node,
                thisArea->interAreaRouterLSAList,
                LSA);

            break;
        }

        // BGP Patch Start
        case OSPFv3_AS_EXTERNAL:
        {
            ERROR_Assert(areaId == OSPFv3_INVALID_AREA_ID,
                        "Wrong function call\n");

            Ospfv3RemoveLSAFromList(node, ospf->asExternalLSAList, LSA);

            break;
        }
        // BGP-OSPF Patch End

        case OSPFv3_GROUP_MEMBERSHIP:
        {
            ERROR_Assert(FALSE,
                        "OSPFv3 Group_Membership LSA not Supported"
                        " in cuurent implementation\n");

            break;
        }
        case OSPFv3_LINK:
        {
            unsigned int interfaceId = areaId;

            Ospfv3RemoveLSAFromList(
                node,
                ospf->pInterface[interfaceId].linkLSAList,
                LSA);

            break;
        }
        case OSPFv3_INTRA_AREA_PREFIX:
        {
            Ospfv3RemoveLSAFromList(
                node,
                thisArea->intraAreaPrefixLSAList,
                LSA);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE,
                        "Unknow linkStateType not supported"
                        " in cuurent implementation\n");
        }
    }
}

// ******************** Call Back Functions********************************//

// /**
// FUNCTION   :: Ospfv3RouterFunction
// LAYER      :: NETWORK
// PURPOSE    :: Address of this function is assigned in the IPv6 interface
//               structure during initialization. IP forwards packet by
//               getting the nextHop from this function. This fuction will
//               be called via a pointer, RouterFunction, in the IPv6
//               interface structure, from function
//               RoutePacketAndSendToMac() in ip.cpp
// + node            :  Node*                 : Pointer to node, used for
//                                              debugging.
// + msg             :  Message*              : Message representing a packet
//                                              event for the protocol.
// + destAddr        :  in6_addr              : Destination Network Address.
// + packetRouted    :  BOOL*                 : If set FALSE, then it allows
//                                              IPv6 to route the packet.
// RETURN     :: void : NULL
// **/
void Ospfv3RouterFunction(
    Node* node,
    Message* ,    // msg
    in6_addr ,    // srcAddr
    in6_addr ,    // destAddr
    BOOL* packetWasRouted)
{
    *packetWasRouted = FALSE;
    return;
}

// /**
// FUNCTION   :: Ospfv3InterfaceStatusHandler
// LAYER      :: NETWORK
// PURPOSE    :: Handle mac interface status alert.
// + node            :  Node*                    : Pointer to node, used for
//                                                debugging.
// + interfaceIndex  :  interfaceIndex           : Node's interface Index.
// + state           :  MacInterfaceState        : MAC status of interface.
// RETURN     ::  void : NULL
// **/
void Ospfv3InterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state)
{
    switch (state)
    {
        case MAC_INTERFACE_DISABLE:
        {
            Ospfv3HandleInterfaceEvent(
                node,
                interfaceIndex,
                E_InterfaceDown);

            break;
        }
        case MAC_INTERFACE_ENABLE:
        {
            Ospfv3HandleInterfaceEvent(
                node,
                interfaceIndex,
                E_InterfaceUp);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown MacInterfaceState\n");
        }
    }
}

// /**
// FUNCTION   :: Ospfv3HandleHelloPacket.
// LAYER      :: NETWORK
// PURPOSE    :: Process received hello packet
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +helloPkt: Ospfv3HelloPacket* : Pointer to HELLO Packet.
// +sourceAddr: in6_addr : Source Address.
// +interfaceId: int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleHelloPacket(
    Node* node,
    Ospfv3HelloPacket* helloPkt,
    in6_addr sourceAddr,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Interface* thisIntf = NULL;
    Ospfv3Area* thisArea = NULL;
    Ospfv3Neighbor* tempNeighborInfo = NULL;
    ListItem* tempListItem = NULL;
    unsigned int* tempNeighbor = NULL;
    BOOL neighborFound;
    int numNeighbor;
    int count;
    unsigned int nbrPrevDR = 0;
    unsigned int nbrPrevBDR = 0;
    int nbrPrevPriority = 0;
    thisIntf = &ospf->pInterface[interfaceId];
    thisArea = Ospfv3GetArea(node, thisIntf->areaId);

    ERROR_Assert(thisArea, "Didn't find specified area\n");

    // The setting of the E-bit found in the Hello Packet's Options
    // field must match this area's ExternalRoutingCapability.
    if (((OSPFV3_GetOptionBits (helloPkt->priorityAndOptions)
        & OSPFv3_OPTIONS_E_BIT)
        && (thisArea->transitCapability == FALSE))
        || (!(OSPFV3_GetOptionBits (helloPkt->priorityAndOptions)
        & OSPFv3_OPTIONS_E_BIT)
        && (thisArea->transitCapability == TRUE)))
    {

#ifdef OSPFv3_DEBUG_HELLO
        {
            printf("    Option field mismatch. Drop hello packet\n");
        }
#endif

        return;
    }


    numNeighbor = (helloPkt->commonHeader.packetLength
                  - sizeof(Ospfv3HelloPacket)) / sizeof(unsigned int);



    // Get the neighbor part of the Hello packet.
    tempNeighbor = (unsigned int* ) (helloPkt + 1);

    ERROR_Assert(tempNeighbor,
                "Neighbor part of the Hello packet is not present\n");

#ifdef OSPFv3_DEBUG_HELLOErr
    {
        Ospfv3PrintHelloPacket(node, helloPkt);
    }
#endif

    // Update my records of my neighbor routers.
    tempListItem = thisIntf->neighborList->first;
    neighborFound = FALSE;
    while (tempListItem)
    {
        tempNeighborInfo = (Ospfv3Neighbor* ) tempListItem->data;

        ERROR_Assert(tempNeighborInfo,
                    "Neighbor not found in the Neighbor list\n");

            if (tempNeighborInfo->neighborId
                == helloPkt->commonHeader.routerId)
            {
                // The neighbor exists in our neighbor list, so no need to
                // add this neighbor to our neighbor list.
                neighborFound = TRUE;
                COPY_ADDR6(sourceAddr, tempNeighborInfo->neighborIPAddress);
#ifdef OSPFv3_DEBUG_HELLO
                {
                    printf("Node %d already know about neighbor %d\n",
                        node->nodeId,
                        helloPkt->commonHeader.routerId);
                }
#endif

                break;
            }
        tempListItem = tempListItem->next;
    }

    if (neighborFound == FALSE)
    {
        // Neighbor does not exist in my neighbor list,
        // so add the neighbor into my neighbor list.
        tempNeighborInfo =
            (Ospfv3Neighbor* ) MEM_malloc(sizeof(Ospfv3Neighbor));

        memset(tempNeighborInfo, 0, sizeof(Ospfv3Neighbor));

        tempNeighborInfo->neighborPriority =
            OSPFV3_GetHelloPacketPriorityFeild(helloPkt->priorityAndOptions);

        tempNeighborInfo->neighborId = helloPkt->commonHeader.routerId;

        COPY_ADDR6(sourceAddr, tempNeighborInfo->neighborIPAddress);

        // Set neighbor's view of DR and BDR
        tempNeighborInfo->neighborDesignatedRouter =
            helloPkt->designatedRouter;

        tempNeighborInfo->neighborBackupDesignatedRouter =
            helloPkt->backupDesignatedRouter;

        tempNeighborInfo->neighborInterfaceId = helloPkt->interfaceId;

        // Initializes different lists
        ListInit(node, &tempNeighborInfo->linkStateRetxList);

        ListInit(node, &tempNeighborInfo->DBSummaryList);

        ListInit(node, &tempNeighborInfo->linkStateRequestList);

        ListInsert(
            node,
            thisIntf->neighborList,
            0,
            (void* ) tempNeighborInfo);

        ospf->neighborCount++;

#ifdef OSPFv3_DEBUG_HELLO
        {
            printf("Node %d adding neighbor %d to neighbor list\n",
                node->nodeId,
                tempNeighborInfo->neighborId);
        }
#endif
    }

    // NBMA mode is not cosidered yet
    if (thisIntf->type == OSPFv3_BROADCAST_INTERFACE
        || thisIntf->type == OSPFv3_POINT_TO_MULTIPOINT_INTERFACE)
    {
        // Note changes in field DR, BDR, router priority for
        // possible use in later step.
        nbrPrevDR = tempNeighborInfo->neighborDesignatedRouter;
        nbrPrevBDR = tempNeighborInfo->neighborBackupDesignatedRouter;
        nbrPrevPriority = tempNeighborInfo->neighborPriority;
        tempNeighborInfo->neighborDesignatedRouter =
            helloPkt->designatedRouter;
        tempNeighborInfo->neighborBackupDesignatedRouter =
            helloPkt->backupDesignatedRouter;

        tempNeighborInfo->neighborPriority =
            OSPFV3_GetHelloPacketPriorityFeild(helloPkt->priorityAndOptions);
    }

    // Handle neighbor event
    Ospfv3HandleNeighborEvent(
        node,
        interfaceId,
        helloPkt->commonHeader.routerId,
        E_HelloReceived);

    // Check whether this router itself appear in the
    // list of neighbor contained in Hello packet.
    count = 0;

    while (count < numNeighbor)
    {
        unsigned int routerId;

        memcpy(&routerId, (tempNeighbor + count), sizeof(unsigned int));

        if (routerId == ospf->routerId)
        {
            // Handle neighbor event

#ifdef OSPFv3_DEBUG_HELLO
            {
                printf("Node %d setting two way "
                        "communication with Neighbor %d\n",
                    node->nodeId,
                    helloPkt->commonHeader.routerId);
            }
#endif

            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                helloPkt->commonHeader.routerId,
                E_TwoWayReceived);

            break;
        }
        count++;
    }

    if (count == numNeighbor)
    {
        // Handle neighbor event : S_OneWay
#ifdef OSPFv3_DEBUG_HELLO
        {
            printf("Node %d setting one way"
                    " communication with Neighbot %d\n",
                node->nodeId,
                helloPkt->commonHeader.routerId);
        }
#endif

        Ospfv3HandleNeighborEvent(
            node,
            interfaceId,
            helloPkt->commonHeader.routerId,
            E_OneWay);

        // Stop processing packet further
        return;
    }

    // If a change in the neighbor's Router Priority field was noted,
    // the receiving interface's state machine is scheduled with
    // the event NeighborChange.
    if (nbrPrevPriority != tempNeighborInfo->neighborPriority)
    {

#ifdef OSPFv3_DEBUG_HELLO
        {
            printf("Node %d Scheduling Interface E_NeighborChange\n",
                node->nodeId);
        }
#endif

        Ospfv3ScheduleInterfaceEvent(
            node,
            interfaceId,
            E_NeighborChange);
    }

    // If the neighbor is both declaring itself to be Designated Router and
    // the Backup Designated Router field in the packet is equal to 0.0.0.0
    // and the receiving interface is in state Waiting, the receiving
    // interface's state machine is scheduled with the event BackupSeen.
    // Otherwise, if the neighbor is declaring itself to be Designated Router
    // and it had not previously, or the neighbor is not declaring itself
    // Designated Router where it had previously, the receiving interface's
    // state machine is scheduled with the event NeighborChange.
    if ((helloPkt->designatedRouter == helloPkt->commonHeader.routerId)
        && (helloPkt->backupDesignatedRouter == 0)
        && (thisIntf->state == S_Waiting))
    {

#ifdef OSPFv3_DEBUG_HELLO
        {
            printf("Node: %d Scheduling E_BackupSeen\n",
                node->nodeId);
        }
#endif

        Ospfv3ScheduleInterfaceEvent(
            node,
            interfaceId,
            E_BackupSeen);
    }
    else if (((helloPkt->designatedRouter == helloPkt->commonHeader.routerId)
            &&(nbrPrevDR != helloPkt->commonHeader.routerId))
            ||((helloPkt->designatedRouter
            != helloPkt->commonHeader.routerId)
            && (nbrPrevDR == helloPkt->commonHeader.routerId)))
    {
        Ospfv3ScheduleInterfaceEvent(node, interfaceId, E_NeighborChange);
    }

    // If the neighbor is declaring itself to be Backup Designated Router and
    // the receiving interface is in state Waiting, the receiving interface's
    // state machine is scheduled with the event BackupSeen. Otherwise, if
    // neighbor is declaring itself to be Backup Designated Router and it had
    // not previously, or the neighbor is not declaring itself Backup
    // Designated Router where it had previously, the receiving interface's
    // state machine is scheduled with the event NeighborChange.
    if ((helloPkt->backupDesignatedRouter == helloPkt->commonHeader.routerId)
        && (thisIntf->state == S_Waiting))
    {
        Ospfv3ScheduleInterfaceEvent(
            node,
            interfaceId,
            E_BackupSeen);
    }
    else if (((helloPkt->backupDesignatedRouter
            == helloPkt->commonHeader.routerId)
            && (nbrPrevBDR != helloPkt->commonHeader.routerId))
            || ((helloPkt->backupDesignatedRouter
            !=helloPkt->commonHeader.routerId)
            && (nbrPrevBDR == helloPkt->commonHeader.routerId)))
    {
        Ospfv3ScheduleInterfaceEvent(node, interfaceId, E_NeighborChange);
    }
}

// /**
// FUNCTION   :: Ospfv3LookupLSDB.
// LAYER      :: NETWORK
// PURPOSE    :: To lookup LSA in database.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +linkStateType: unsigned int : Type of Link State.
// +advertisingRouter: unsigned int : Advertising Router.
// +linkStateId: unsigned int : Link State Id.
// +interfaceId: int : Interface Id.
// RETURN     :: Pointer to Link State Header.
// **/
static
Ospfv3LinkStateHeader* Ospfv3LookupLSDB(
    Node* node,
    unsigned int linkStateType,
    unsigned int advertisingRouter,
    unsigned int linkStateId,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* LSHeader = NULL;
    Ospfv3Area* thisArea = NULL;

    // BGP-OSPF Patch Start
    if (linkStateType != OSPFv3_AS_EXTERNAL)
    {
        thisArea = Ospfv3GetArea(node, ospf->pInterface[interfaceId].areaId);

        ERROR_Assert(thisArea, "Area doesn't exist\n");
    }
    // BGP-OSPF Patch End

    if (linkStateType == OSPFv3_ROUTER)
    {
        LSHeader =  Ospfv3LookupLSAList(
                        node,
                        thisArea->routerLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    else if (linkStateType == OSPFv3_NETWORK)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->networkLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    else if (linkStateType == OSPFv3_INTER_AREA_PREFIX)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->interAreaPrefixLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    else if (linkStateType == OSPFv3_INTER_AREA_ROUTER)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->interAreaRouterLSAList,
                        advertisingRouter,
                        linkStateId);
    }

    // BGP-OSPF Patch Start
    else if (linkStateType == OSPFv3_AS_EXTERNAL)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        ospf->asExternalLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    // BGP-OSPF Patch End

    else if (linkStateType == OSPFv3_GROUP_MEMBERSHIP)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "GROUP MEMBERSHIP or TYPE 7 LSA not"
                    " supported. Packet discarded\n",
                LSHeader->linkStateType);
        }
#endif

    }

    else if (linkStateType == OSPFv3_LINK)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        ospf->pInterface[interfaceId].linkLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    else if (linkStateType == OSPFv3_INTRA_AREA_PREFIX)
    {
        LSHeader = Ospfv3LookupLSAList(
                        node,
                        thisArea->intraAreaPrefixLSAList,
                        advertisingRouter,
                        linkStateId);
    }
    else
    {
        ERROR_Assert(FALSE, "Unknown LS Type\n");
    }

    return LSHeader;
}

// /**
// FUNCTION   :: Ospfv3LookupLSAList.
// LAYER      :: NETWORK
// PURPOSE    :: Search for the LSA in list.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +list: LinkedList* : Pointer to Linked list.
// +advertisingRouter: unsigned int : Advertising Router.
// +linkStateId: unsigned int : Link State Id.
// RETURN     :: Return pointer to Link State header if found,
//               otherwise return NULL.
// **/
Ospfv3LinkStateHeader* Ospfv3LookupLSAList(
    Node* node,
    LinkedList* list,
    unsigned int advertisingRouter,
    unsigned int linkStateId)
{
    Ospfv3LinkStateHeader* listLSHeader = NULL;
    ListItem* item = list->first;

    while (item)
    {
        // Get LS Header
        listLSHeader = (Ospfv3LinkStateHeader* ) item->data;

        if (listLSHeader->advertisingRouter == advertisingRouter
            && listLSHeader->linkStateId == linkStateId)
        {
            return listLSHeader;
        }

        item = item->next;
    }
    return NULL;
}

// /**
// FUNCTION   :: Ospfv3AddToRequestList.
// LAYER      :: NETWORK
// PURPOSE    :: Added To Request List.
// PARAMETERS ::
// +node:  Node* : Pointer to node.
// +linkStateRequestList: LinkedList* : Pointer to Link State Request.
// +LSHeader: Ospfv3LinkStateHeader* : Pointer to Link State Header.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3AddToRequestList(
    Node* node,
    LinkedList* linkStateRequestList,
    Ospfv3LinkStateHeader* LSHeader)
{

    Ospfv3LSReqItem* newItem = NULL;

#ifdef OSPFv3_DEBUG_SYNC
    {
        fprintf(stdout,
                "    Adding new LSA to request list\n"
                "        LS Type = %d, "
                "Link State ID = 0x%x, Advertising Router = 0x%x\n",
            LSHeader->linkStateType,
            LSHeader->linkStateId,
            LSHeader->advertisingRouter);
    }
#endif

    // Create new item to insert to list
    newItem = (Ospfv3LSReqItem* ) MEM_malloc(sizeof(Ospfv3LSReqItem));

    newItem->LSHeader =
        (Ospfv3LinkStateHeader* ) MEM_malloc(sizeof(Ospfv3LinkStateHeader));

    memcpy(newItem->LSHeader, LSHeader, sizeof(Ospfv3LinkStateHeader));

    newItem->seqNumber = 0;

    ListInsert(node, linkStateRequestList, 0, newItem);
}


// /**
// FUNCTION   :: Ospfv3ProcessDatabaseDescriptionPacket.
// LAYER      :: NETWORK
// PURPOSE    :: Process the database description packet.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +msg: Message* : Pointer to Message.
// +interfaceId: int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3ProcessDatabaseDescriptionPacket(
    Node* node,
    Message* msg,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea = Ospfv3GetArea(
                                node,
                                ospf->pInterface[interfaceId].areaId);

    Ospfv3Neighbor* nbrInfo = NULL;
    Ospfv3DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    int size;

    dbDscrpPkt =
        (Ospfv3DatabaseDescriptionPacket* ) MESSAGE_ReturnPacket(msg);

    LSHeader = (Ospfv3LinkStateHeader* ) (dbDscrpPkt + 1);

    // Find neighbor structure
    nbrInfo = Ospfv3GetNeighborInfoByRtrId(
                node,
                dbDscrpPkt->commonHeader.routerId);

    ERROR_Assert(nbrInfo != NULL, "Neighbor not found in the Neighbor list");

    size = dbDscrpPkt->commonHeader.packetLength;

    // For each LSA in the packet
    for (size -= sizeof(Ospfv3DatabaseDescriptionPacket); size > 0;
        size -= sizeof(Ospfv3LinkStateHeader))
    {
        Ospfv3LinkStateHeader* oldLSHeader = NULL;

        // Check for the validity of LSA
         switch (LSHeader->linkStateType)
        {
            case OSPFv3_ROUTER:
            case OSPFv3_NETWORK:
            case OSPFv3_INTER_AREA_PREFIX:
            case OSPFv3_INTER_AREA_ROUTER:
            case OSPFv3_AS_EXTERNAL:
            case OSPFv3_LINK:
            case OSPFv3_INTRA_AREA_PREFIX:
            {
                break;
            }
            default :
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "Unknown LS type (%d). Packet discarded\n",
                        LSHeader->linkStateType);
                }
#endif

                // Handle neighbor event : Sequence Number Mismatch
                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    nbrInfo->neighborId,
                    E_SeqNumberMismatch);

                return;
            }
        }

        // Stop processing packet if the neighbor is associated with stub
        // area and the LSA is AS-EXTERNAL-LAS
        if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL
            && thisArea->transitCapability == FALSE)
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                        "Receive AS_EXTERNAL_LSA from stub area. "
                        "Packet discarded\n");
            }
#endif

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                nbrInfo->neighborId,
                E_SeqNumberMismatch);

            return;
        }

        // Looks up for the LSA in its own Database
        oldLSHeader = Ospfv3LookupLSDB(
                        node,
                        LSHeader->linkStateType,
                        LSHeader->advertisingRouter,
                        LSHeader->linkStateId,
                        interfaceId);

        // Add to request list if the LSA doesn't exist or if
        // this instance is more recent one
        if (!oldLSHeader
            || (Ospfv3CompareLSA(node, LSHeader, oldLSHeader) > 0))
        {
            Ospfv3AddToRequestList(
                node,
                nbrInfo->linkStateRequestList,
                LSHeader);
        }
        LSHeader += 1;
    }

    // Master
    if (nbrInfo->masterSlave == T_Master)
    {
        nbrInfo->DDSequenceNumber++;

        // If the router has already sent its entire sequence of DD packets,
        // and the just accepted packet has the more bit (M) set to 0, the
        // neighbor event ExchangeDone is generated.
        if (ListIsEmpty(node, nbrInfo->DBSummaryList)
            && (!(dbDscrpPkt->mtuAndBits & OSPFv3_M_BIT)))
        {
            Ospfv3ScheduleNeighborEvent(
                node,
                interfaceId,
                nbrInfo->neighborId,
                E_ExchangeDone);
        }
        // Else send new DD packet.
        else
        {
            Ospfv3SendDatabaseDescriptionPacket(
                node,
                nbrInfo->neighborId,
                interfaceId);
        }
    }

    // Slave
    else
    {
        nbrInfo->DDSequenceNumber = dbDscrpPkt->ddSequenceNumber;
        nbrInfo->optionsBits = dbDscrpPkt->mtuAndBits;
        // Send DD packet
        Ospfv3SendDatabaseDescriptionPacket(
            node,
            nbrInfo->neighborId,
            interfaceId);

        if (!(dbDscrpPkt->mtuAndBits & OSPFv3_M_BIT)
            && ListIsEmpty(node, nbrInfo->DBSummaryList))
        {
            Ospfv3ScheduleNeighborEvent(
                node,
                interfaceId,
                nbrInfo->neighborId,
                E_ExchangeDone);
        }
    }

    // Save received DD Packet
    if (!nbrInfo->lastReceivedDDPacket)
    {
        nbrInfo->lastReceivedDDPacket = MESSAGE_Duplicate(node, msg);
    }
    else
    {
        MESSAGE_Free(node, nbrInfo->lastReceivedDDPacket);

        nbrInfo->lastReceivedDDPacket = MESSAGE_Duplicate(node, msg);
    }
}

// /**
// FUNCTION   :: Ospfv3HandleLSRequestPacket.
// LAYER      :: NETWORK
// PURPOSE    :: Handle Link State Request packet.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +msg: Message* : Pointer to Message.
// +neighborId: unsigned int : Neighbor Id.
// +interfaceId: int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleLSRequestPacket(
    Node* node,
    Message* msg,
    unsigned int neighborId,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Interface* thisInterface = &ospf->pInterface[interfaceId];
    Ospfv3LinkStateRequestPacket* reqPkt = NULL;
    int numLSReqObject;
    Ospfv3LSRequestInfo* LSReqObject = NULL;
    int i = 0;
    unsigned int sendDelayedUpdate = OSPFv3_SEND_DELAYED_UPDATE;

    // Find neighbor structure
    Ospfv3Neighbor* nbrInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    if (!nbrInfo)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "Node %u receive LS Request from unknown"
                    " neighbor 0x%x. Discard LS Request packet\n",
                node->nodeId,
                neighborId);
        }
#endif

        return;
    }

    // Neighbor state should be exchange or later
    if (nbrInfo->state < S_Exchange)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            fprintf(stdout,
                    "Node %u : neighbor (0x%x) state is below"
                    " Exchange. Discard LS Request packet\n",
                node->nodeId,
                neighborId);
        }
#endif

        return;
    }

    reqPkt = (Ospfv3LinkStateRequestPacket* ) MESSAGE_ReturnPacket(msg);

    numLSReqObject =
        (reqPkt->commonHeader.packetLength
        - sizeof(Ospfv3LinkStateRequestPacket))
        / sizeof(Ospfv3LSRequestInfo);

    LSReqObject = (Ospfv3LSRequestInfo* ) (reqPkt + 1);

    for (i = 0; i < numLSReqObject; i++)
    {
        Ospfv3LinkStateHeader* LSHeader = NULL;

        // Stop processing packet if requested LSA type is not identified
        switch (LSReqObject->linkStateType)
        {
            case OSPFv3_ROUTER:
            case OSPFv3_NETWORK:
            case OSPFv3_INTER_AREA_PREFIX:
            case OSPFv3_INTER_AREA_ROUTER:
            case OSPFv3_AS_EXTERNAL:
            case OSPFv3_LINK:
            case OSPFv3_INTRA_AREA_PREFIX:
            {
                break;
            }
            default :
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    fprintf(stdout,
                            "Node %u : Receive bad LS Request from"
                            " neighbor (0x%x). Discard LS Request packet\n",
                        node->nodeId,
                        neighborId);
                }
#endif

                // Handle neighbor event : Bad LS Request
                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    neighborId,
                    E_BadLSReq);

                return;
            }
        }

        // Find LSA in my own LSDB
        LSHeader = Ospfv3LookupLSDB(
                        node,
                        (short) LSReqObject->linkStateType,
                        LSReqObject->advertisingRouter,
                        LSReqObject->linkStateId,
                        interfaceId);

        // Stop processing packet if LSA is not found in my database
        if (LSHeader == NULL)
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                        "Node %u : Receive bad LS Request from"
                        " neighbor (0x%x). Discard LS Request packet\n",
                    node->nodeId,
                    neighborId);
            }
#endif

            // Handle neighbor event : Bad LS Request
            Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    neighborId,
                    E_BadLSReq);

            return;
        }

        // Add LSA to send this neighbor
        Ospfv3QueueLSAToFlood(node, thisInterface, (char* ) LSHeader);
        LSReqObject += 1;

        if (sendDelayedUpdate)
        {

            if (!thisInterface->floodTimerSet)
            {
                Message* floodMsg = NULL;
                clocktype delay;

                thisInterface->floodTimerSet = TRUE;

                // Set Timer
                floodMsg = MESSAGE_Alloc(
                                node,
                                NETWORK_LAYER,
                                ROUTING_PROTOCOL_OSPFv3,
                                MSG_ROUTING_OspfFloodTimer);

                MESSAGE_InfoAlloc(node, floodMsg, sizeof(int));

                memcpy(MESSAGE_ReturnInfo(floodMsg), &interfaceId, sizeof(int));

                delay =
                    (clocktype) (RANDOM_erand(ospf->seed) * OSPFv3_FLOOD_TIMER);

                MESSAGE_Send(node, floodMsg, delay);
            }
        }
    }

    if (!sendDelayedUpdate)

    {
        Ospfv3SendLSUpdate(node, interfaceId);
    }
}

// /**
// FUNCTION   :: Ospfv3HandleDatabaseDescriptionPacket.
// LAYER      :: NETWORK
// PURPOSE    :: Handle database description packet.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +msg: Message* : Pointer to Message.
// +srcAddr: in6_addr : Source Address.
// +interfaceId: int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleDatabaseDescriptionPacket(
    Node* node,
    Message* msg,
    in6_addr srcAddr,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* nbrInfo = NULL;
    Ospfv3DatabaseDescriptionPacket* dbDscrpPkt = NULL;
    BOOL isDuplicate = FALSE;
    Ospfv3DatabaseDescriptionPacket* oldPkt = NULL;

    dbDscrpPkt =
        (Ospfv3DatabaseDescriptionPacket* ) MESSAGE_ReturnPacket(msg);

    // Find neighbor structure
    nbrInfo = Ospfv3GetNeighborInfoByRtrId(
                node,
                dbDscrpPkt->commonHeader.routerId);

    if (!nbrInfo)
    {

#ifdef OSPFv3_DEBUG_SYNC
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&srcAddr, addrString);

            fprintf(stdout,
                    "    receive REQUEST from unknown "
                    "neighbor %s, packet discarded\n",
                addrString);
        }
#endif
        return;
    }

    // Check if two consecutive DD packets are same
    if (nbrInfo->lastReceivedDDPacket != NULL)
    {
        unsigned int iBit = 0;
        unsigned int mBit = 0;
        unsigned int msBit = 0;

        oldPkt = (Ospfv3DatabaseDescriptionPacket* )
            MESSAGE_ReturnPacket(nbrInfo->lastReceivedDDPacket);

        iBit = oldPkt->mtuAndBits & OSPFv3_I_BIT;
        mBit = oldPkt->mtuAndBits & OSPFv3_M_BIT;
        msBit = oldPkt->mtuAndBits & OSPFv3_MS_BIT;

        if ((iBit == (dbDscrpPkt->mtuAndBits & OSPFv3_I_BIT))
            && (mBit == (dbDscrpPkt->mtuAndBits & OSPFv3_M_BIT))
            && (msBit == (dbDscrpPkt->mtuAndBits & OSPFv3_MS_BIT))
            && (oldPkt->ddSequenceNumber == dbDscrpPkt->ddSequenceNumber))
        {
            isDuplicate = TRUE;
        }
    }
    switch (nbrInfo->state)
    {
        // Reject packet if neighbor state is Down or Attempt or 2-Way
        case S_NeighborDown:
        case S_Attempt:
        case S_TwoWay:
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                fprintf(stdout,
                    "    Node %d: Neighbor state is below S_ExStart. "
                    "Packet discarded\n", node->nodeId);
            }
#endif

            break;
        }
        case S_Init:
        {

            // Handle neighbor event : 2-Way receive
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                dbDscrpPkt->commonHeader.routerId,
                E_TwoWayReceived);

            if (nbrInfo->state != S_ExStart)
            {
                break;
            }
        }
        case S_ExStart:
        {
            unsigned int iBit = 0;
            unsigned int mBit = 0;
            unsigned int msBit = 0;

            iBit = dbDscrpPkt->mtuAndBits & OSPFv3_I_BIT;
            mBit = dbDscrpPkt->mtuAndBits & OSPFv3_M_BIT;
            msBit = dbDscrpPkt->mtuAndBits & OSPFv3_MS_BIT;

            // Slave
            if (iBit && mBit && msBit
                && (dbDscrpPkt->commonHeader.packetLength
                == sizeof(Ospfv3DatabaseDescriptionPacket))
                && (dbDscrpPkt->commonHeader.routerId > ospf->routerId))
            {
                nbrInfo->masterSlave = T_Slave;
                nbrInfo->DDSequenceNumber = dbDscrpPkt->ddSequenceNumber;

#ifdef OSPFv3_DEBUG_SYNC
                {
                    printf("    Neighbor %d will be Slave for Node %d\n",
                        nbrInfo->neighborId,
                        node->nodeId);
                }
#endif
            }

            // Master
            else if ((! iBit) && (! msBit)
                    && (dbDscrpPkt->ddSequenceNumber
                    == nbrInfo->DDSequenceNumber)
                    && (dbDscrpPkt->commonHeader.routerId < ospf->routerId))
            {
                nbrInfo->masterSlave = T_Master;

#ifdef OSPFv3_DEBUG_SYNC
                {
                    printf("    Neighbor %d will be Master for Node %d\n",
                        nbrInfo->neighborId,
                        node->nodeId);
                }
#endif

            }

            // Cannot determine Master or Slave, so discard packet
            else
            {

#ifdef OSPFv3_DEBUG_SYNC
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&srcAddr, addrString);

                    fprintf(stdout,
                            "    Node %d cannot determine Master or Slave"
                            " for neighbor %d, so discard packet"
                            " with Seq %d\n",
                        node->nodeId,
                        nbrInfo->neighborId,
                        dbDscrpPkt->ddSequenceNumber);

                }
#endif

                break;
            }


            // Handle neighbor event : Negotiation Done
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                dbDscrpPkt->commonHeader.routerId,
                E_NegotiationDone);

            // Process rest of the DD packet
            Ospfv3ProcessDatabaseDescriptionPacket(
                node,
                msg,
                interfaceId);

            break;
        }
        case S_Exchange:
        {
            // Check if the packet is duplicate
            if (isDuplicate)
            {
                // Master
                if (nbrInfo->masterSlave == T_Master)
                {
                    // Discard packet
                }

                // Slave
                else
                {
                    in6_addr sourceAddr;

                    ERROR_Assert(nbrInfo->lastSentDDPkt,
                                "No packet to retx\n");

                    //Trace sending pkt
                    ActionData acnData;
                    acnData.actionType = SEND;
                    acnData.actionComment = NO_COMMENT;

                    TRACE_PrintTrace(
                        node,
                        nbrInfo->lastSentDDPkt,
                        TRACE_NETWORK_LAYER,
                        PACKET_OUT, &acnData);


                    // This packet needs to be retransmitted,
                    // so send it out again.
                    Ipv6GetGlobalAggrAddress(
                        node,
                        interfaceId,
                        &sourceAddr);

                    Ipv6SendRawMessage(
                        node,
                        MESSAGE_Duplicate(node, nbrInfo->lastSentDDPkt),
                        sourceAddr,
                        nbrInfo->neighborIPAddress,
                        interfaceId,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_OSPF,
                        1);
                }
                break;
            }

            // Check for Master-Slave mismatch
            if (((nbrInfo->masterSlave == T_Master)
                && ((dbDscrpPkt->mtuAndBits & OSPFv3_MS_BIT)))
                || ((nbrInfo->masterSlave == T_Slave)
                && (!(dbDscrpPkt->mtuAndBits & OSPFv3_MS_BIT))))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    dbDscrpPkt->commonHeader.routerId,
                    E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Check if initialization bit is set
            if ((dbDscrpPkt->mtuAndBits & OSPFv3_I_BIT))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    dbDscrpPkt->commonHeader.routerId,
                    E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Check DD Sequence number
            if (((nbrInfo->masterSlave == T_Master)
                && (dbDscrpPkt->ddSequenceNumber
                != nbrInfo->DDSequenceNumber))
                || ((nbrInfo->masterSlave == T_Slave)
                && (dbDscrpPkt->ddSequenceNumber
                != nbrInfo->DDSequenceNumber + 1)))
            {
                // Handle neighbor event : Sequence Number Mismatch
                Ospfv3HandleNeighborEvent(
                    node,
                    interfaceId,
                    dbDscrpPkt->commonHeader.routerId,
                    E_SeqNumberMismatch);

                // Stop processing packet
                break;
            }

            // Process rest of the packet
            Ospfv3ProcessDatabaseDescriptionPacket(
                node,
                msg,
                interfaceId);

            break;
        }
        case S_Loading:
        case S_Full:
        {
            if (isDuplicate)
            {
                // Master discards duplicate packet
                if (nbrInfo->masterSlave == T_Master)
                {

#ifdef OSPFv3_DEBUG_SYNC
                    {
                        fprintf(stdout,
                                "    duplicate DD packet. Neighbor "
                                "state is T_Master. Packet discarded\n");
                    }
#endif

                    // Discard packet
                    break;
                }
                else
                {
                    // In states Loading and Full the slave must resend its
                    // last Database Description packet in response to
                    // duplicate Database Description packets received from
                    // the master.For this reason the slave must wait
                    // RouterDeadInterval seconds before freeing the last
                    // Database Description packet. Reception of a Database
                    // Description packet from the master after this interval
                    // will generate a SeqNumberMismatch neighbor event.
                    if (node->getNodeTime() - nbrInfo->lastDDPktSent
                        < ospf->pInterface[interfaceId].routerDeadInterval)
                    {
                        in6_addr sourceAddr;

#ifdef OSPFv3_DEBUG_SYNC
                        {
                            fprintf(stdout,
                                    "    duplicate DD packet."
                                    " Neighbor state is T_Slave. Retransmit "
                                    "previous packet\n");
                        }
#endif

                        ERROR_Assert(nbrInfo->lastSentDDPkt,
                                    "No packet to retx\n");


                        //Trace sending pkt
                        ActionData acnData;
                        acnData.actionType = SEND;
                        acnData.actionComment = NO_COMMENT;

                        TRACE_PrintTrace(
                            node,
                            nbrInfo->lastSentDDPkt,
                            TRACE_NETWORK_LAYER,
                            PACKET_OUT,
                            &acnData);

                        // Retransmit packet
                        Ipv6GetGlobalAggrAddress(
                            node,
                            interfaceId,
                            &sourceAddr);

                        Ipv6SendRawMessage(
                            node,
                            MESSAGE_Duplicate(node, nbrInfo->lastSentDDPkt),
                            sourceAddr,
                            nbrInfo->neighborIPAddress,
                            interfaceId,
                            IPTOS_PREC_INTERNETCONTROL,
                            IPPROTO_OSPF,
                            1);

                        break;
                    }
                }
            }

            // Handle neighbor event : Sequence Number Mismatch
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                dbDscrpPkt->commonHeader.routerId,
                E_SeqNumberMismatch);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Unknown neighbor state\n");
        }
    }
}

// /**
// FUNCTION   :: Ospfv3NoNbrInStateExhangeOrLoading.
// LAYER      :: NETWORK
// PURPOSE    :: To verify for any neighbor to be in Exchange/Loading state.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +interfaceId: int : Interface Id.
// RETURN     :: TRUE if the neighbor is in the Exchange/Loading state,
//               FALSE otherwise.
// **/
static
BOOL Ospfv3NoNbrInStateExhangeOrLoading(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Neighbor* neighborInfo = NULL;
    ListItem* listItem =
        ospf->pInterface[interfaceId].neighborList->first;

    while (listItem)
    {
        neighborInfo = (Ospfv3Neighbor* ) listItem->data;

        if (neighborInfo->state == S_Exchange
            || neighborInfo->state == S_Loading)
        {
            return FALSE;
        }
        listItem = listItem->next;
    }
    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3NoNbrInStateExhangeOrLoading.
// LAYER      :: NETWORK
// PURPOSE    :: To Send Delayed Ack.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +LSA:  char* : Pointer to LSA.
// +neighborId: unsigned int : Neighbor Id.
// RETURN     :: void : NULL
// **/
static
void Ospfv3SendDelayedAck(Node* node, char* LSA, unsigned int neighborId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    unsigned int interfaceId =
        Ospfv3GetInterfaceForThisNeighbor(node, neighborId);

    Ospfv3LinkStateHeader* LSHeader =
        (Ospfv3LinkStateHeader* ) MEM_malloc(sizeof(Ospfv3LinkStateHeader));

    memcpy(LSHeader, LSA, sizeof(Ospfv3LinkStateHeader));

    ListInsert(
        node,
        ospf->pInterface[interfaceId].delayedAckList,
        node->getNodeTime(),
        (void* ) LSHeader);

    // Set Delayed Ack timer, if not set yet
    if (ospf->pInterface[interfaceId].delayedAckTimer == FALSE)
    {
        Message* delayedAckMsg = NULL;
        clocktype delay;

        ospf->pInterface[interfaceId].delayedAckTimer = TRUE;

        // Interval between delayed transmission must be less than
        // RxmtInterval
        delay =
            (clocktype) ((RANDOM_erand(ospf->seed) *  OSPFv3_RXMT_INTERVAL) / 2);

        // Set Timer
        delayedAckMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            ROUTING_PROTOCOL_OSPFv3,
                            MSG_ROUTING_OspfDelayedAckTimer);

        MESSAGE_InfoAlloc(node, delayedAckMsg, sizeof(int));

        memcpy(MESSAGE_ReturnInfo(delayedAckMsg), &interfaceId, sizeof(int));

        MESSAGE_Send(node, delayedAckMsg, delay);
    }
}

// /**
// FUNCTION   :: Ospfv3UpdateLSAList.
// LAYER      :: NETWORK
// PURPOSE    :: To Update LSA List.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +list:  LinkedList* : Pointer to Linked list.
// +LSA:  char* : Pointer to LSA.
// +neighborId: unsigned int : Neighbor Id.
// +areaId: unsigned int : Area Id.
// RETURN     :: TRUE if there was an update, FALSE otherwise.
// **/
static
BOOL Ospfv3UpdateLSAList(
    Node* node,
    LinkedList* list,
    char* LSA,
    unsigned int neighborId,
    unsigned int areaId)
{
   Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* listLSHeader = NULL;
    Ospfv3LinkStateHeader* LSHeader = NULL;
    ListItem* listItem = NULL;
    BOOL retVal = FALSE;
    BOOL floodedBackToRecvIntf = FALSE;
    Ospfv3Neighbor* neighborInfo;

    unsigned int interfaceId =
        Ospfv3GetInterfaceForThisNeighbor(node, neighborId);

    neighborInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    LSHeader = (Ospfv3LinkStateHeader* ) LSA;

    listItem = list->first;

#ifdef OSPFv3_DEBUG_FLOODErr
    {
        printf("    Node %u updating LSDB\n", node->nodeId);
    }
#endif

    while (listItem)
    {
        listLSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

        if ((LSHeader->advertisingRouter == listLSHeader->advertisingRouter)
            && (LSHeader->linkStateId == listLSHeader->linkStateId))
        {

#ifdef OSPFv3_DEBUG_FLOODErr
            {
                printf("    LSA found in LSDB\n");
            }
#endif

            // RFC2328, Sec-13 (5.a)
            // If there is already a database copy, and if the database copy
            // was received via flooding and installed less than MinLSArrival
            // seconds ago, discard new LSA (without acknowledging it) and
            // examine the next LSA (if any).
            if ((node->getNodeTime() - listItem->timeStamp)
                < (OSPFv3_MIN_LS_ARRIVAL))

            {

#ifdef OSPFv3_DEBUG_FLOOD
                {
                    printf("Node %u:  Received LSA is more recent, but"
                            " installed < MinLSArrival ago, so don't "
                            "update LSDB\n",
                        node->nodeId);
                }
#endif

                return FALSE;
            }

            break;
        }

        listItem = listItem->next;
    }

    // RFC2328, Sec-13 (Bullet - 5.b) & (Bullet - 5.e)
    // Immediately Flood LSA
    if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
    {
        floodedBackToRecvIntf = Ospfv3FloodThroughAS(
                                    node,
                                    LSA,
                                    neighborInfo->neighborId);
    }
    else
    {
        floodedBackToRecvIntf = Ospfv3FloodLSAThroughArea(
                                    node,
                                    LSA,
                                    neighborInfo->neighborId,
                                    areaId);
    }
    if (!floodedBackToRecvIntf)
    {
        if (ospf->pInterface[interfaceId].state == S_Backup)
        {
            if (CMP_ADDR6(
                    ospf->pInterface[interfaceId].designatedRouterIPAddress,
                    neighborInfo->neighborIPAddress) == 0)
            {
                // Send Delayed Ack
                Ospfv3SendDelayedAck(
                    node,
                    LSA,
                    neighborInfo->neighborId);
            }
        }
        else
        {
            // Send Delayed Ack
            Ospfv3SendDelayedAck(
                node,
                LSA,
                neighborInfo->neighborId);
        }
    }

    // RFC2328, Sec-13 (5.d)
    // Install LSA in LSDB. this may schedule Routing table calculation.
    if (LSHeader->linkStateAge >= (OSPFv3_LSA_MAX_AGE / SECOND))
    {

#ifdef OSPFv3_DEBUG
        {
            printf("LSA's age is equal to LSAMaxAge, so remove "
                    "LSA from LSDB.\n");
        }
#endif

        // If greater than or equal to max age, remove from LSDB.
        ListGet(node, list, listItem, TRUE, FALSE);

        retVal = TRUE;
    }
    else
    {
        retVal = Ospfv3InstallLSAInLSDB(node, list, LSA);
    }

    return retVal;
}

// /**
// FUNCTION   :: Ospfv3CheckForIntraAreaPrefixLSAValidity.
// LAYER      :: NETWORK
// PURPOSE    :: To Check for Intra Area Prefix LSA Validity.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +addrPrefix:  in6_addr : Address Prefix.
// +prefixLength:  unsigned char : Prefix length.
// +destType:  Ospfv3DestType : Type of Destination.
// +areaId: unsigned int : Area Id.
// +pMatch: Ospfv3RoutingTableRow** : Pointer to pointer to
//                                    Row of the OSPFv3 Routing Table.
// RETURN     :: TRUE if valid, FALSE otherwise.
// **/
static
BOOL Ospfv3CheckForIntraAreaPrefixLSAValidity(
    Node* node,
    in6_addr addrPrefix,
    unsigned char prefixLength,
    Ospfv3DestType destType,
    unsigned int areaId,
    Ospfv3RoutingTableRow** pMatch)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    int i = 0;
    Ospfv3RoutingTable* rtTable = &ospf->routingTable;

    Ospfv3RoutingTableRow* rowPtr =
        (Ospfv3RoutingTableRow* ) BUFFER_GetData(&rtTable->buffer);

    Ospfv3AreaAddressRange* addrInfo = NULL;
    in6_addr defaultAddr;

    memset(&defaultAddr, 0, sizeof(in6_addr));

    *pMatch = NULL;

    if ((CMP_ADDR6(defaultAddr, addrPrefix) == 0)
        && prefixLength == 0)
    {
        return TRUE;
    }

    for (i = 0; i < rtTable->numRows; i++)
    {
        // Do we have a perfect match?
        if ((CMP_ADDR6(rowPtr[i].destAddr, addrPrefix) == 0)
            && prefixLength == rowPtr[i].prefixLength
            && Ospfv3CompDestType(rowPtr[i].destType, destType)
            && rowPtr[i].areaId != areaId
            && rowPtr[i].flag != OSPFv3_ROUTE_INVALID)
        {
            *pMatch = &rowPtr[i];
            return TRUE;
        }
    }

    // We didn't get a perfect match. So it might be an aggregated
    // advertisement which must be one of our configured area address
    // range. So search for an intra area entry contained within this range.
    for (i = 0; i < rtTable->numRows; i++)
    {
        if ((CMP_ADDR6(rowPtr[i].destAddr, addrPrefix) == 0)
            && prefixLength < rowPtr[i].prefixLength
            && rowPtr[i].pathType == OSPFv3_INTRA_AREA
            && Ospfv3CompDestType(rowPtr[i].destType, destType)
            && rowPtr[i].areaId != areaId
            && rowPtr[i].flag != OSPFv3_ROUTE_INVALID)
        {
            addrInfo = Ospfv3GetAddrRangeInfoForAllArea(node, addrPrefix);

            ERROR_Assert(addrInfo != NULL
                       && (CMP_ADDR6(
                                addrInfo->addressPrefix,
                                addrPrefix) == 0)
                        && addrInfo->prefixLength == prefixLength,
                    "Something wrong with route suppression\n");

            *pMatch = &rowPtr[i];
            return TRUE;
        }
    }

    return FALSE;
}

// /**
// FUNCTION   :: Ospfv3UpdateLSDB.
// LAYER      :: NETWORK
// PURPOSE    :: To Update node's LSDB for received LSA.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +LSA:  char* : Pointer to LSA.
// +neighborId:  unsigned int : Neighbor Id.
// +areaId: unsigned int : Area Id.
// RETURN     :: TRUE if there was an update, FALSE otherwise.
// **/
static
BOOL Ospfv3UpdateLSDB(
    Node* node,
    char* LSA,
    unsigned int neighborId,
    unsigned int areaId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3LinkStateHeader* LSHeader = (Ospfv3LinkStateHeader* ) LSA;
    Ospfv3Area* thisArea = NULL;
    BOOL retVal = FALSE;
    Ospfv3Neighbor* neighborInfo = NULL;
    in6_addr srcAddr;
    unsigned int interfaceId;

    neighborInfo = Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    interfaceId = Ospfv3GetInterfaceForThisNeighbor(node, neighborId);

    COPY_ADDR6(neighborInfo->neighborIPAddress, srcAddr);

    // BGP-OSPF Patch Start
    if (LSHeader->linkStateType != OSPFv3_AS_EXTERNAL)
    {
        thisArea = Ospfv3GetArea(node, areaId);

        ERROR_Assert(thisArea, "Doesn't find the area\n");
    }
    // BGP-OSPF Patch End

#ifdef OSPFv3_DEBUG_FLOODErr
    {
        Ospfv3PrintLSA(LSA);
    }
#endif

    // RFC 2740 3.6.  Definition of self-originated LSAs
    // Should take special action if this node is originator.
    if (LSHeader->advertisingRouter == ospf->routerId)
    {
        BOOL flush = FALSE;

        switch (LSHeader->linkStateType)
        {
            case OSPFv3_NETWORK:
            {
                int interfaceId = LSHeader->linkStateId;

                if (ospf->pInterface[interfaceId].state == S_DR)
                {
                    retVal = Ospfv3UpdateLSAList(
                                node,
                                thisArea->networkLSAList,
                                LSA,
                                neighborId,
                                areaId);

                    Ospfv3ScheduleNetworkLSA(node, interfaceId, FALSE);
                }
                else
                {
                    flush = TRUE;
                }
                break;
            }
            case OSPFv3_INTER_AREA_PREFIX:
            {
                Ospfv3RoutingTableRow* rowPtr;
                Ospfv3InterAreaPrefixLSA* interAreaLSA =
                    (Ospfv3InterAreaPrefixLSA* )LSA;
                BOOL matchFound = 0;

                if (interAreaLSA->prefixLength == OSPFv3_MAX_PREFIX_LENGTH)
                {
                    matchFound =
                        Ospfv3CheckForIntraAreaPrefixLSAValidity(
                            node,
                            interAreaLSA->addrPrefix,
                            interAreaLSA->prefixLength,
                            OSPFv3_DESTINATION_NETWORK,
                            areaId,
                            &rowPtr);
                }
                else
                {
                    in6_addr prefix;

                    Ipv6GetPrefix(
                        &interAreaLSA->addrPrefix,
                        &prefix,
                        interAreaLSA->prefixLength);

                    matchFound =
                        Ospfv3CheckForIntraAreaPrefixLSAValidity(
                            node,
                            prefix,
                            interAreaLSA->prefixLength,
                            OSPFv3_DESTINATION_NETWORK,
                            areaId,
                            &rowPtr);
                }

                if (!matchFound)
                {
                    flush = TRUE;
                }
                break;
            }
            case OSPFv3_ROUTER:
            {
                retVal = Ospfv3UpdateLSAList(
                            node,
                            thisArea->routerLSAList,
                            LSA,
                            neighborId,
                            areaId);

                Ospfv3ScheduleRouterLSA(node, thisArea->areaId, FALSE);

                break;
            }
            default:
            {
                break;
            }
        }

        if (flush)
        {
            Ospfv3FlushLSA(node, LSA, areaId);

            Ospfv3RemoveLSAFromLSDB(node, LSA, areaId);
        }
    }
    else if (LSHeader->linkStateType == OSPFv3_ROUTER)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating Router LSDB\n", node->nodeId);
        }
#endif

        retVal = Ospfv3UpdateLSAList(
                    node,
                    thisArea->routerLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else if (LSHeader->linkStateType == OSPFv3_NETWORK)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating Network LSDB\n", node->nodeId);
        }
#endif

        retVal = Ospfv3UpdateLSAList(
                    node,
                    thisArea->networkLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else if (LSHeader->linkStateType == OSPFv3_INTER_AREA_PREFIX)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating Inter Area Prefix LSDB\n",
                node->nodeId);
        }
#endif

        retVal = Ospfv3UpdateLSAList(
                    node,
                    thisArea->interAreaPrefixLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else if (LSHeader->linkStateType == OSPFv3_INTER_AREA_ROUTER)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating Inter Area Router LSDB\n",
                    node->nodeId);
        }
#endif

        retVal = Ospfv3UpdateLSAList(
                    node,
                    thisArea->interAreaRouterLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }

    // BGP-OSPF Patch Start
    else if (LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating OSPFv3_AS_EXTERNAL LSDB\n",
                node->nodeId);
        }
#endif

        retVal =  Ospfv3UpdateLSAList(
                    node,
                    ospf->asExternalLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    // BGP-OSPF Patch End

    else if (LSHeader->linkStateType == OSPFv3_GROUP_MEMBERSHIP)
    {

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("Node %u updating GroupMembership LSDB\n", node->nodeId);
        }
#endif

        retVal =  Ospfv3UpdateLSAList(
                    node,
                    thisArea->groupMembershipLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else if (LSHeader->linkStateType == OSPFv3_LINK)
    {
        retVal =  Ospfv3UpdateLSAList(
                    node,
                    ospf->pInterface[interfaceId].linkLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else if (LSHeader->linkStateType == OSPFv3_INTRA_AREA_PREFIX)
    {
        retVal =  Ospfv3UpdateLSAList(
                    node,
                    thisArea->intraAreaPrefixLSAList,
                    LSA,
                    neighborId,
                    areaId);
    }
    else
    {
        ERROR_Assert(FALSE, "LS Type not known. Can't update LSDB\n");
    }

    return retVal;
}



// /**
// FUNCTION   :: Ospfv3SendDirectAck.
// LAYER      :: NETWORK
// PURPOSE    :: To Send Direct Ack to a neighbor.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +ackList:  LinkedList* : Pointer to Linked list.
// +neighborId:  unsigned int : Neighbor Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3SendDirectAck(
    Node* node,
    LinkedList* ackList,
    unsigned int neighborId)
{
    ListItem* listItem = NULL;
    int count = 0;
    Ospfv3LinkStateHeader* payload = NULL;
    int maxCount;

    unsigned int interfaceId =
        Ospfv3GetInterfaceForThisNeighbor(node, neighborId);

    maxCount = (GetNetworkIPFragUnit(node, interfaceId) - sizeof(ip6_hdr)
                - sizeof(Ospfv3LinkStateAckPacket))
                / sizeof(Ospfv3LinkStateHeader);

    payload = (Ospfv3LinkStateHeader* )
        MEM_malloc(maxCount * sizeof(Ospfv3LinkStateHeader));

    while (!ListIsEmpty(node, ackList))
    {
        listItem = ackList->first;

        if (count == maxCount)
        {
            Ospfv3SendAckPacket(
                node,
                (char* ) payload,
                count,
                neighborId,
                interfaceId);

            // Reset variables
            count = 0;
            continue;
        }

        memcpy(
            &payload[count],
            listItem->data,
            sizeof(Ospfv3LinkStateHeader));

        count++;

        // Remove item from list
        ListGet(node, ackList, listItem, TRUE, FALSE);
    }

    Ospfv3SendAckPacket(
        node,
        (char* ) payload,
        count,
        neighborId,
        interfaceId);

    MEM_free(payload);
}

// /**
// FUNCTION   :: Ospfv3RequestedLSAReceived.
// LAYER      :: NETWORK
// PURPOSE    :: To Check whether requested LSA(s) has been received.
// PARAMETERS ::
// +nbrInfo:  Ospfv3Neighbor* : Pointer to Neighbor.
// RETURN     :: TRUE if received, FALSE otherwise.
// **/
static
BOOL Ospfv3RequestedLSAReceived(Ospfv3Neighbor* nbrInfo)
{

    // Requests are sent from top of the list. So it's sufficient
    // to check only first element of the list
    Ospfv3LSReqItem* itemInfo =
        (Ospfv3LSReqItem* ) nbrInfo->linkStateRequestList->first->data;

    if (itemInfo->seqNumber == nbrInfo->LSReqTimerSequenceNumber)
    {
        return FALSE;
    }

    return TRUE;
}

// /**
// FUNCTION   :: Ospfv3HandleUpdatePacket.
// LAYER      :: NETWORK
// PURPOSE    :: To Handle received Update packet.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +msg: Message* : Pointer to Message.
// +sourceAddr: in6_addr : Source Address.
// +interfaceId: int : Interface Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleUpdatePacket(
    Node* node,
    Message* msg,
    in6_addr sourceAddr,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Area* thisArea =
        Ospfv3GetArea(node, ospf->pInterface[interfaceId].areaId);

    Ospfv3LinkStateHeader* LSHeader = NULL;
    Ospfv3LinkStateUpdatePacket* updatePkt = NULL;
    Ospfv3Neighbor* nbrInfo = NULL;
    int count;
    BOOL isLSDBChanged = FALSE;
    int moreRecentIndicator = 0;
    LinkedList* directAckList = NULL;
    updatePkt = (Ospfv3LinkStateUpdatePacket* ) msg->packet;
    LSHeader = (Ospfv3LinkStateHeader* ) (updatePkt + 1);

    nbrInfo =
        Ospfv3GetNeighborInfoByRtrId(node, updatePkt->commonHeader.routerId);

    if (nbrInfo == NULL)
    {

#ifdef OSPFv3_DEBUG_FLOOD
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&sourceAddr, addrString);

            fprintf(stdout,
                    "Node %u receive LS Update from unknown"
                    " neighbor %s. Discard LS Update packet\n",
                node->nodeId,
                addrString);
        }
#endif

        return;
    }

    if (nbrInfo->state < S_Exchange)
    {

#ifdef OSPFv3_DEBUG_FLOOD
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&sourceAddr, addrString);

            printf("Node %u : Neighbor (%s) state is below S_Exchange. "
                    "Discard LS Update packet\n",
                node->nodeId,
                addrString);
        }
#endif

        return;
    }

#ifdef OSPFv3_DEBUG_FLOODErr
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&sourceAddr, addrString);

        printf("Node %u Process UPDATE PACKET from neighbor %s\n",
            node->nodeId,
            addrString);
    }
#endif

    ListInit(node, &directAckList);

    // Execute following steps for each LSA
    for (count = 0; count < updatePkt->numLSA;
        count++, LSHeader =
        (Ospfv3LinkStateHeader* ) (((char* ) LSHeader) + LSHeader->length))
    {
        Ospfv3LinkStateHeader* oldLSHeader;

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            printf("    Process %dth LSA (Type = %x) in update pkt\n",
                count + 1,
                LSHeader->linkStateType);

            Ospfv3PrintLSHeader(LSHeader);
        }
#endif

        // Handling of LSA with unknown type not implemented
        switch (LSHeader->linkStateType)
        {
            case OSPFv3_ROUTER:
            {
                Ospfv3RouterLSA* routerLSA = (Ospfv3RouterLSA* )LSHeader;

                // Stub area must have single ABR.
                if (thisArea->externalRoutingCapability == FALSE
                    && ospf->areaBorderRouter == TRUE
                    && (routerLSA->bitsAndOptions & OSPFv3_E_BIT
                    || routerLSA->bitsAndOptions & OSPFv3_B_BIT))
                {
                    ERROR_Assert(FALSE,
                                "Stub Area must have only"
                                " one Area Border Router\n");
                }
            }
            case OSPFv3_NETWORK:
            case OSPFv3_INTER_AREA_PREFIX:
            case OSPFv3_INTER_AREA_ROUTER:
            case OSPFv3_AS_EXTERNAL:
            case OSPFv3_LINK:
            case OSPFv3_INTRA_AREA_PREFIX:
            {
                break;
            }
            default :
            {

#ifdef OSPFv3_DEBUG_FLOOD
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&sourceAddr, addrString);

                    fprintf(stdout,
                            "Node %u : Receive bad LSA (Type = %d) from"
                            " neighbor (%s). Discard LSA and examine"
                            " next one\n",
                        node->nodeId,
                        LSHeader->linkStateType,
                        addrString);
                }
#endif

                continue;
            }
        }

        //AS-External LSA shouldn't flooded throughout stub area
        if ((LSHeader->linkStateType == OSPFv3_AS_EXTERNAL)
            && (thisArea->transitCapability == FALSE))
        {

#ifdef OSPFv3_DEBUG_FLOOD
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(
                    &ospf->pInterface[interfaceId].address,
                    addrString);

                fprintf(stdout,
                        "    Received Interface 0x%x belongs to stub"
                        " area. Shouldn't process ASExternal LSA.\n",
                    addrString);
            }
#endif

            continue;
        }

        // Find instance of this LSA contained in local database
        oldLSHeader = Ospfv3LookupLSDB(
                        node,
                        LSHeader->linkStateType,
                        LSHeader->advertisingRouter,
                        LSHeader->linkStateId,
                        interfaceId);

        if ((LSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
            && (!oldLSHeader)
            && (Ospfv3NoNbrInStateExhangeOrLoading(node, interfaceId)))
        {
            void* ackLSHeader = NULL;

            ackLSHeader = MEM_malloc(sizeof(Ospfv3LinkStateHeader));

            memcpy(ackLSHeader, LSHeader, sizeof(Ospfv3LinkStateHeader));

            ListInsert(node, directAckList, 0, ackLSHeader);

#ifdef OSPFv3_DEBUG_FLOODErr
            {
                printf("    LSA's LSAge is equal to MaxAge and there is "
                        "current no instance of the LSA in the LSDB,\n"
                        "    and none of the routers neighbor are in state "
                        "Exchange or Loading. Send direct Ack\n");
            }
#endif

            continue;
        }
        if (oldLSHeader)
        {
            moreRecentIndicator = Ospfv3CompareLSA(
                                    node,
                                    LSHeader,
                                    oldLSHeader);
        }
        if ((!oldLSHeader)
            || (moreRecentIndicator > 0))
        {

#ifdef OSPFv3_DEBUG_FLOODErr
            {
                printf("    Node %d: Received LSA is more recent or no "
                        "current Database copy exist\n", node->nodeId);
            }
#endif

            // Update our link state database, if applicable.
            if (Ospfv3UpdateLSDB(
                    node,
                    (char* ) LSHeader,
                    updatePkt->commonHeader.routerId,
                    thisArea->areaId))
            {
                isLSDBChanged = TRUE;
            }
            continue;
        }
        if (Ospfv3SearchRequestList(
                node,
                nbrInfo->linkStateRequestList,
                LSHeader))
        {
            Ospfv3HandleNeighborEvent(
                node,
                interfaceId,
                nbrInfo->neighborId,
                E_BadLSReq);

            ListFree(node, directAckList, FALSE);

            return;
        }
        if (moreRecentIndicator == 0)
        {
            ListItem* listItem;

#ifdef OSPFv3_DEBUG_FLOODErr
            {
                printf("    LSA is duplicate\n");
            }
#endif

            listItem = nbrInfo->linkStateRetxList->first;

            while (listItem)
            {
                Ospfv3LinkStateHeader* retxLSHeader;
                retxLSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

                // If LSA exists in retransmission list
                if ((LSHeader->linkStateType == retxLSHeader->linkStateType)
                    && (LSHeader->advertisingRouter
                    == retxLSHeader->advertisingRouter)
                    && (LSHeader->linkStateId == retxLSHeader->linkStateId))
                {
                    break;
                }
                listItem = listItem->next;
            }
            if (listItem)
            {

                // Treat received LSA as implied Ack and
                // remove LSA From LSRetx List
                ListGet(
                    node,
                    nbrInfo->linkStateRetxList,
                    listItem,
                    TRUE,
                    FALSE);

#ifdef OSPFv3_DEBUG_FLOODErr
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(
                        &nbrInfo->neighborIPAddress,
                        addrString);

                    printf("    Received LSA treated as implied ack\n");

                    printf("    Node %u: Removing LSA with seqno 0x%x from "
                            "rext list of nbr %s\n",
                        node->nodeId,
                        LSHeader->linkStateSequenceNumber,
                        addrString);

                    printf("        Type = %d, ID = 0x%x, advRtr = 0x%x\n",
                        LSHeader->linkStateType,
                        LSHeader->linkStateId,
                        LSHeader->advertisingRouter);
                }
#endif

                if (ListIsEmpty(node, nbrInfo->linkStateRetxList))
                {
                    nbrInfo->LSRetxTimer = FALSE;
                    nbrInfo->rxmtTimerSequenceNumber++;
                }
                if ((ospf->pInterface[interfaceId].state == S_Backup)
                    && !CMP_ADDR6(
                            ospf->pInterface[interfaceId]
                            .designatedRouterIPAddress,
                            sourceAddr) )
                {

#ifdef OSPFv3_DEBUG_FLOODErr
                    {
                        printf("    I'm BDR. Send delayed Ack to DR\n");
                    }
#endif

                    // Send Delayed ack
                    Ospfv3SendDelayedAck(
                        node,
                        (char* ) LSHeader,
                        updatePkt->commonHeader.routerId);
                }
            }
            else
            {
                void* ackLSHeader = NULL;

                ackLSHeader = MEM_malloc(sizeof(Ospfv3LinkStateHeader));

                memcpy(
                    ackLSHeader,
                    LSHeader,
                    sizeof(Ospfv3LinkStateHeader));

                ListInsert(
                    node,
                    directAckList,
                    0,
                    ackLSHeader);

#ifdef OSPFv3_DEBUG_FLOODErr
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&sourceAddr, addrString);

                    printf("    Received LSA was not treated as "
                            "implied Ack. Send direct ack to nbr %s\n",
                        addrString);
                }
#endif

            }
            continue;
        }
        if (moreRecentIndicator < 0)
        {
            if ((oldLSHeader->linkStateAge == (OSPFv3_LSA_MAX_AGE / SECOND))
                && (oldLSHeader->linkStateSequenceNumber
                ==(signed int) OSPFv3_MAX_SEQUENCE_NUMBER))
            {
                // Discard LSA without acknowledging
            }
            else
            {
                ListItem* listItem = NULL;
                Ospfv3LinkStateHeader* retxLSHeader = NULL;

                // Send Database copy to neighbor as long as it has not
                // been sent within last MinLSArrival seconds.
                listItem = nbrInfo->linkStateRetxList->first;

                while (listItem)
                {
                    retxLSHeader = (Ospfv3LinkStateHeader* ) listItem->data;

                    // If LSA exists in retransmission list
                    if ((LSHeader->linkStateType
                        == retxLSHeader->linkStateType)
                        && (LSHeader->advertisingRouter
                        == retxLSHeader->advertisingRouter)
                        && (LSHeader->linkStateId
                        ==retxLSHeader->linkStateId))
                    {
                        break;
                    }

                    listItem = listItem->next;

                }
                if ((listItem) && (node->getNodeTime() - listItem->timeStamp
                    > OSPFv3_MIN_LS_ARRIVAL))
                {
                    Ospfv3SendUpdatePacket(
                        node,
                        sourceAddr,
                        interfaceId,
                        (char* ) oldLSHeader,
                        oldLSHeader->length,
                        1);
                }
            }
        }
    }

    if (!ListIsEmpty(node, directAckList))
    {
        Ospfv3SendDirectAck(
            node,
            directAckList,
            updatePkt->commonHeader.routerId);
    }

    ListFree(node, directAckList, FALSE);

    if (isLSDBChanged == TRUE)
    {
        // Calculate shortest path as contents of LSDB has changed.
        Ospfv3ScheduleSPFCalculation(node);

#ifdef OSPFv3_DEBUG_FLOODErr
        {
            Ipv6PrintForwardingTable(node);
        }
#endif

    }
    if (!ListIsEmpty(node, nbrInfo->linkStateRequestList))
    {
        // If we've got the desired LSA(s), send next request
        if (Ospfv3RequestedLSAReceived(nbrInfo))
        {

            // Send next LS request
            Ospfv3SendLSRequestPacket(
                node,
                nbrInfo->neighborId,
                interfaceId,
                FALSE);
        }
    }
    else
    {
        // Cancel retransmission timer
        nbrInfo->LSReqTimerSequenceNumber += 1;

        Ospfv3HandleNeighborEvent(
            node,
            interfaceId,
            nbrInfo->neighborId,
            E_LoadingDone);
    }
}

// /**
// FUNCTION   :: Ospfv3HandleAckPacket.
// LAYER      :: NETWORK
// PURPOSE    :: To Remove the LSA being acknowledged
//               from the Retransmission list.
// PARAMETERS ::
// +node:  Node* : Pointer to Node.
// +ackPkt: Ospfv3LinkStateAckPacket* : Pointer to Ack Packet.
// +neighborId: unsigned int : Neighbor Id.
// RETURN     :: void : NULL.
// **/
static
void Ospfv3HandleAckPacket(
    Node* node,
    Ospfv3LinkStateAckPacket* ackPkt,
    unsigned int neighborId)
{
    ListItem* listItem = NULL;

    Ospfv3Neighbor* neighborInfo =
        Ospfv3GetNeighborInfoByRtrId(node, neighborId);

    Ospfv3LinkStateHeader* LSHeader = NULL;
    int numAck;
    int count;

    // Neighbor no longer exists, so do nothing.
    if (neighborInfo == NULL)
    {
        return;
    }
    if (neighborInfo->state < S_Exchange)
    {
        return;
    }
    numAck = (ackPkt->commonHeader.packetLength
                - sizeof(Ospfv3LinkStateAckPacket))
                / sizeof(Ospfv3LinkStateHeader);

    LSHeader = (Ospfv3LinkStateHeader* ) (ackPkt + 1);

    for (count = 0; count < numAck; count++, LSHeader = LSHeader + 1)
    {
        listItem = neighborInfo->linkStateRetxList->first;

        while (listItem)
        {
            Ospfv3LinkStateHeader* retxLSHeader =
                (Ospfv3LinkStateHeader* ) listItem->data;

            // If LSA exists in retransmission list
            if ((LSHeader->linkStateType == retxLSHeader->linkStateType)
                && (LSHeader->advertisingRouter
                == retxLSHeader->advertisingRouter)
                && (LSHeader->linkStateId == retxLSHeader->linkStateId)
                && (!Ospfv3CompareLSA(node, LSHeader, retxLSHeader)))
            {
                // Remove from List
                ListGet(
                    node,
                    neighborInfo->linkStateRetxList,
                    listItem,
                    TRUE,
                    FALSE);

#ifdef OSPFv3_DEBUG_FLOODErr
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(
                                &neighborInfo->neighborIPAddress,
                                addrString);

                    printf("    Node %u: Removing LSA with seqno 0x%x from "
                            "rext list of nbr %s\n",
                        node->nodeId,
                        LSHeader->linkStateSequenceNumber,
                        addrString);

                    printf("        Type = %d, ID = 0x%x, advRtr = 0x%x\n",
                        LSHeader->linkStateType,
                        LSHeader->linkStateId,
                        LSHeader->advertisingRouter);
                }
#endif

                break;
            }

            listItem = listItem->next;
        }
    }

    if ((ListIsEmpty(node, neighborInfo->linkStateRetxList))
        && (neighborInfo->LSRetxTimer == TRUE))
    {
        neighborInfo->LSRetxTimer = FALSE;
        neighborInfo->rxmtTimerSequenceNumber++;
    }
}

// /**
// FUNCTION   :: Ospfv3HandleRoutingProtocolPacket
// LAYER      :: NETWORK
// PURPOSE    :: Routing Protocol packet processor. This Function is called
//               from ip6_deliver of ip6_input.cpp.
// PARAMETERS ::
//  node            :  Node*                : Pointer to node, used for
//                                            debugging.
//  msg             :  Message              : Message representing a packet
//                                            event for the protocol.
//  sourceAddress   :  in6_addr             : Source Address of Packet.
//  interfaceIndex  :  interfaceIndex       : Node's interface Index.
// RETURN     ::  void : NULL
// **/
void Ospfv3HandleRoutingProtocolPacket(
    Node* node,
    Message* msg,
    in6_addr sourceAddr,
    int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                        node,
                                        ROUTING_PROTOCOL_OSPFv3,
                                        NETWORK_IPV6);

    Ospfv3Interface* thisIntf = NULL;

    Ospfv3CommonHeader* ospfHeader =
        (Ospfv3CommonHeader* ) MESSAGE_ReturnPacket(msg);

    // Make sure that ospf is running on the incoming interface.
    // If ospf is not running on the incoming interface,
    // then drop this packet.
    if ((NetworkIpGetUnicastRoutingProtocolType(node, interfaceId, NETWORK_IPV6)
        != ROUTING_PROTOCOL_OSPFv3)
        || (node->getNodeTime() < ospf->staggerStartTime))
    {
        //Trace drop

        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_PROTOCOL_UNAVAILABLE_AT_INTERFACE;

        TRACE_PrintTrace(
            node,
            msg,
            TRACE_NETWORK_LAYER,
            PACKET_IN,
            &acnData);

        MESSAGE_Free(node, msg);

        return;
    }

    // Locally originated packets should not be passed on to OSPF.
    if (Ospfv3IsMyAddress(node, sourceAddr))
    {
        MESSAGE_Free(node, msg);

        return;
    }

    thisIntf = &ospf->pInterface[interfaceId];

    // Area Id specified in the header matches the
    // area Id of receiving interface
    if (ospfHeader->areaId == thisIntf->areaId)
    {
        in6_addr prefix;
        in6_addr tempAddress;
        in6_addr tempPrefix;

        Ipv6GetPrefix(
            &sourceAddr,
            &prefix,
            Ipv6GetPrefixLength(node, interfaceId));

        Ipv6GetGlobalAggrAddress(node, interfaceId, &tempAddress);

        Ipv6GetPrefix(
            &tempAddress,
            &tempPrefix,
            Ipv6GetPrefixLength(node, interfaceId));

        // Make sure the packet has been sent over a single hop
        //POINT_TO_POINT_INTERFACE checking added for virtual link support
        if (thisIntf->type != OSPFv3_POINT_TO_POINT_INTERFACE &&
            CMP_ADDR6(prefix, tempPrefix) != 0)
        {
            // NOTE:
            // Although this checking should not be performed over
            // Point-to-Point networks as the interface address of each
            // end of the link may assigned independently. However,
            // current QUALNET assign a network number on link, so we can
            // ignore this restriction unless QUALNET being capable of
            //doing so.
            MESSAGE_Free(node, msg);

            return;
        }
    }
    else
    {
        //Trace drop
        ActionData acnData;
        acnData.actionType = DROP;
        acnData.actionComment = DROP_PROTOCOL_UNAVAILABLE_AT_INTERFACE;

        TRACE_PrintTrace(
            node,
            msg,
            TRACE_NETWORK_LAYER,
            PACKET_IN,
            &acnData);

        // Should be discarded as it comes from other area
        MESSAGE_Free(node, msg);

        return;
    }

    // trace recd pkt
    ActionData acnData;
    acnData.actionType = RECV;
    acnData.actionComment = NO_COMMENT;

    TRACE_PrintTrace(node, msg, TRACE_NETWORK_LAYER, PACKET_IN, &acnData);

    switch (ospfHeader->packetType)
    {
        // It's a Hello packet.
        case OSPFv3_HELLO:
        {
            Ospfv3HelloPacket* helloPkt =
                (Ospfv3HelloPacket* ) MESSAGE_ReturnPacket(msg);

#ifdef OSPFv3_DEBUG_HELLO
            {
                char clockStr[MAX_STRING_LENGTH];
                char addrString[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                IO_ConvertIpAddressToString(&sourceAddr, addrString);

                printf("\n\nNode %u receive HELLO from node %u at time %s."
                        "\tPacket Length = %d\n",
                    node->nodeId,
                    helloPkt->commonHeader.routerId,
                    clockStr,
                    ospfHeader->packetLength);
            }
#endif

            ospf->stats.numHelloRecv++;

            // Process the received Hello packet.
            Ospfv3HandleHelloPacket(
                node,
                helloPkt,
                sourceAddr,
                interfaceId);

            break;
        }

        // It's a link state update packet.
        case OSPFv3_LINK_STATE_UPDATE:
        {
#ifdef OSPFv3_DEBUG_FLOOD
            {
                Ospfv3LinkStateUpdatePacket* updatePkt =
                (Ospfv3LinkStateUpdatePacket* ) MESSAGE_ReturnPacket(msg);
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u receive UPDATE Packet"
                        " from node %u at time %s\n",
                    node->nodeId,
                    updatePkt->commonHeader.routerId,
                    clockStr);
            }
#endif

            ospf->stats.numLSUpdateRecv++;

            // Process the Update packet.
            Ospfv3HandleUpdatePacket(
                node,
                msg,
                sourceAddr,
                interfaceId);

            break;
        }

        // It's a link state acknowledgement packet.
        case OSPFv3_LINK_STATE_ACK:
        {
            Ospfv3LinkStateAckPacket* ackPkt =
                (Ospfv3LinkStateAckPacket* ) MESSAGE_ReturnPacket(msg);

#ifdef OSPFv3_DEBUG_FLOOD
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u receive ACK from node %u at time %s\n",
                    node->nodeId,
                    ackPkt->commonHeader.routerId,
                    clockStr);
            }
#endif

            ospf->stats.numAckRecv++;

            // Process the ACK packet.
            Ospfv3HandleAckPacket(
                node,
                ackPkt,
                ackPkt->commonHeader.routerId);

            break;
        }
        // It's a database description packet.
        case OSPFv3_DATABASE_DESCRIPTION:
        {

#ifdef OSPFv3_DEBUG_SYNC
            {
                Ospfv3DatabaseDescriptionPacket* dbDscrpPkt =
                (Ospfv3DatabaseDescriptionPacket* ) MESSAGE_ReturnPacket(
                                                        msg);
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u receive DD packet with seqno %u"
                        " from node %d at time %s\n",
                    node->nodeId,
                    dbDscrpPkt->ddSequenceNumber,
                    dbDscrpPkt->commonHeader.routerId,
                    clockStr);

                Ospfv3PrintDatabaseDescriptionPacket(node, msg);
            }
#endif

            ospf->stats.numDDPktRecv++;

            // Process the Database Description packet.
            Ospfv3HandleDatabaseDescriptionPacket(
                node,
                msg,
                sourceAddr,
                interfaceId);

            break;
        }

        // It's a Link State Request packet.
        case OSPFv3_LINK_STATE_REQUEST:
        {
            Ospfv3LinkStateRequestPacket* reqPkt =
                (Ospfv3LinkStateRequestPacket* ) MESSAGE_ReturnPacket(msg);

#ifdef OSPFv3_DEBUG_SYNC
            {
                char clockStr[MAX_STRING_LENGTH];
                char addrString[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);

                IO_ConvertIpAddressToString(&sourceAddr, addrString);

                printf("Node %u receive REQUEST from node %d "
                        "at time %s\n",
                    node->nodeId,
                    reqPkt->commonHeader.routerId,
                    clockStr);
            }
#endif

            ospf->stats.numLSReqRecv++;

            // Process the Link State Request packet.
            Ospfv3HandleLSRequestPacket(
                node,
                msg,
                reqPkt->commonHeader.routerId,
                interfaceId);

            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Packet type not identified\n");
        }
    }

    MESSAGE_Free (node, msg);

}

// ************************* Finalization********************************//

// /**
// FUNCTION   :: Ospfv3Finalize
// LAYER      :: NETWORK
// PURPOSE    :: Prints out the required statistics for OSPFv3 protocol after
//               completion of the simulation.
// PARAMETERS ::
// + node            :  Node*                   : Pointer to node, used for
//                                                debugging.
// + interfaceIndex  :  interfaceIndex          : Node's interface Index.
// RETURN     ::  void : NULL
// **/
void Ospfv3Finalize(Node* node, int interfaceId)
{
    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);

    // Used to print statistics only once during finalization. In
    // the future, stats should be tied to each interface.
    if (ospf->stats.alreadyPrinted == TRUE)
    {
        return;
    }
    ospf->stats.alreadyPrinted = TRUE;

#ifdef OSPFv3_DEBUG_TABLE
    {
        printf("\n");

        Ospfv3PrintRoutingTable(node);
    }
#endif

#ifdef OSPFv3_DEBUG_SYNC
    {
        Ospfv3PrintNeighborState(node);
    }
#endif

#ifdef OSPFv3_DEBUG_ERRORS
    {
        int i;

        printf("Node %u address info\n", node->nodeId);

        for (i = 0; i < node->numberInterfaces; i++)
        {
            char addrString[MAX_STRING_LENGTH];
            in6_addr addr;

            printf("    interface = %d\n", i);

            Ipv6GetGlobalAggrAddress(node, interfaceId, &addr);

            IO_ConvertIpAddressToString(&addr, addrString);

            printf("    network address = %s\n", addrString);
        }

        Ipv6PrintForwardingTable(node);
    }
#endif

#ifdef OSPFv3_DEBUG_ISM
    {
        int i;
        for (i = 0; i < node->numberInterfaces; i++)
        {
            char stateStr[MAX_STRING_LENGTH];

            if (ospf->pInterface[i].type == OSPFv3_NON_OSPF_INTERFACE)
            {
                continue;
            }

            if (ospf->pInterface[i].state == S_Down)
            {
                strcpy(stateStr, "S_Down");
            }
            else if (ospf->pInterface[i].state == S_Loopback)
            {
                strcpy(stateStr, "S_Loopback");
            }
            else if (ospf->pInterface[i].state == S_Waiting)
            {
                strcpy(stateStr, "S_Waiting");
            }
            else if (ospf->pInterface[i].state == S_PointToPoint)
            {
                strcpy(stateStr, "S_PointToPoint");
            }
            else if (ospf->pInterface[i].state == S_DROther)
            {
                strcpy(stateStr, "S_DROther");
            }
            else if (ospf->pInterface[i].state == S_Backup)
            {
                strcpy(stateStr, "S_Backup");
            }
            else if (ospf->pInterface[i].state == S_DR)
            {
                strcpy(stateStr, "S_DR");
            }
            else
            {
                strcpy(stateStr, "Unknown");
            }

            printf("    [Node %u] Interface[%d] state %20s\n",
                node->nodeId,
                i,
                stateStr);
        }
    }
#endif

    // Print out stats if user specifies it.
    if (ospf->collectStat == TRUE)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "Hello Packets Sent = %d", ospf->stats.numHelloSent);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Hello Packets Received = %d",
            ospf->stats.numHelloRecv);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Sent = %d",
            ospf->stats.numLSUpdateSent);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Received = %d",
            ospf->stats.numLSUpdateRecv);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State ACK Packets Sent = %d",
            ospf->stats.numAckSent);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State ACK Packets Received = %d",
            ospf->stats.numAckRecv);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Link State Update Packets Retransmitted = %d",
            ospf->stats.numLSUpdateRxmt);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Advertisements Expired = %d",
            ospf->stats.numExpiredLSAge);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Sent = %d",
            ospf->stats.numDDPktSent);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Received = %d",
            ospf->stats.numDDPktRecv);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Database Description Packets Retransmitted = %d",
            ospf->stats.numDDPktRxmt);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Sent = %d",
            ospf->stats.numLSReqSent);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Received = %d",
            ospf->stats.numLSReqRecv);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link State Request Packets Retransmitted = %d",
            ospf->stats.numLSReqRxmt);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Router LSA Originated = %d",
            ospf->stats.numRtrLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Network LSA Originated = %d",
            ospf->stats.numNetLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Inter-Area-Prefix LSA Originated = %d",
            ospf->stats.numInterAreaPrefixLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Inter-Area-Router LSA Originated = %d",
            ospf->stats.numInterAreaRtrLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Link LSA Originated = %d",
            ospf->stats.numLinkLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Intra-Area-Prefix LSA Originated = %d",
            ospf->stats.numIntraAreaPrefixLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "AS External LSA Originated = %d",
            ospf->stats.numASExternalLSAOriginate);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);

        sprintf(buf, "Number of LSA Refreshed = %d",
                 ospf->stats.numLSARefreshed);

        IO_PrintStat(
            node,
            "Network",
            "OSPFv3",
            ANY_DEST,
            -1,// instance Id,
            buf);
    }
}

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingOSPFv3HandleAddressChangeEvent
// LAYER      :: APPLICATION
// PURPOSE    :: Handles any change in the interface address
//               due to autoconfiguration feature
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldGlobalAddress   : in6_addr* old global address
// RETURN :: void : NULL
//---------------------------------------------------------------------------
void RoutingOSPFv3HandleAddressChangeEvent(
    Node* node,
    Int32 interfaceId,
    in6_addr* oldGlobalAddress)
{
    NetworkDataIp* ip = (NetworkDataIp* ) node->networkData.networkVar;
    IPv6InterfaceInfo* iface =
        ip->interfaceInfo[interfaceId]->ipv6InterfaceInfo;

    Ospfv3Data* ospf = (Ospfv3Data* ) NetworkIpGetRoutingProtocol(
                                            node,
                                            ROUTING_PROTOCOL_OSPFv3,
                                            NETWORK_IPV6);


    COPY_ADDR6(iface->globalAddr, ospf->pInterface[interfaceId].address);
    ospf->pInterface[interfaceId].prefixLen = iface->prefixLen;

    ospf->pInterface[interfaceId].designatedRouter = 0;

    memset(
        &ospf->pInterface[interfaceId].designatedRouterIPAddress,
        0,
        sizeof(in6_addr));

    ospf->pInterface[interfaceId].backupDesignatedRouter = 0;

    memset(
        &ospf->pInterface[interfaceId].backupDesignatedRouterIPAddress,
        0,
        sizeof(in6_addr));

    Ospfv3OriginateLinkLSAForThisInterface(node, interfaceId, FALSE);
}
