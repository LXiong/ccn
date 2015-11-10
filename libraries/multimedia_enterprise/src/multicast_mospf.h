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

//
//  File: mospf.h


#ifndef MOSPF_H_
#define MOSPF_H_

#include "network_ip.h"
#include "application.h"
#include "routing_ospfv2.h"

#ifdef ADDON_DB
#include "db_multimedia.h"
#endif

// Invalid interface and Node.
#define MOSPF_INVALID_INTERFACE               (-1)
#define MOSPF_UNREACHABLE_NODE                (-1)
#define ANY_GROUP                             (-1)
#define MOSPF_INVALID_LINKTYPE                (0)

#define MOSPF_LS_INFINITY                     0xFF

//----------------------------------------------------------------------//
//          Various structure To Handle GroupMembership LSA
//----------------------------------------------------------------------//

// Group Membership LSA
typedef struct
{
    Ospfv2LinkStateHeader LSHeader;

    // Dynamically allocating this.
    // MospfGroupMembershipInfo groupInfo;
} MospfGroupMembershipLSA;

// Type of vertex.
typedef enum
{
    MOSPF_VERTEX_ROUTER,
    MOSPF_VERTEX_TRANSIT_NETWORK,
    MOSPF_VERTEX_INVALID
} MospfVertexType;

// Group information.
typedef struct
{
    MospfVertexType vertexType;
    NodeAddress vertexID;
} MospfGroupMembershipInfo;



//----------------------------------------------------------------------//
//      Various Structure To Handle The Shortest Path Tree for Mospf
//----------------------------------------------------------------------//

//
// shortest path is calculated using Dkjstra or SPF algorithm.
// The algorithm is stated using graph oriented language :vertices & links
// Hence a new data structure is introduced known as vertex data structure .


// Type of incoming link
typedef enum
{
    MOSPF_IL_Virtual,
    MOSPF_IL_Direct,
    MOSPF_IL_Normal,
    MOSPF_IL_Summary,
    MOSPF_IL_External,
    MOSPF_IL_None
} MospfIncomingLinkType;

typedef struct struct_Mospf_VertexItem
{
    MospfVertexType                         vType;
    NodeAddress                             vId;
    Ospfv2LinkStateHeader*                  LSA;
    struct struct_Mospf_VertexItem*         parent;
    NodeAddress                             associatedIntf;
    char                                    cost;
    char                                    ttl;
    MospfIncomingLinkType                   incomingLinkType;
} MospfVertexItem;

//----------------------------------------------------------------------//
//      Various Structure To Handle The Forwarding Function for Mospf
//----------------------------------------------------------------------//

// Type of Network Ip Forwarding
typedef enum
{
    DATA_LINK_DISABLED,
    DATA_LINK_MULTICAST,
    DATA_LINK_UNICAST
} MospfForwardingType;

// downStream structure
typedef struct routing_mospf_downStream_str
{
    int                    interfaceIndex;
    int                    ttl;
}MospfdownStream;

// Forwarding table row
typedef struct routing_mospf_forwarding_table_row_str
{
    NodeAddress             groupAddr;
    NodeAddress             sourceAddressMask;
    NodeAddress             pktSrcAddr;
    NodeAddress             upStream;
    int                     incomingIntf;
#ifdef ADDON_DB
    NodeAddress             srcNodeAddr;
    int                     pktIncomingIntf;
    NodeAddress             prevHopAddress;
#endif
    unsigned int            rootArea;
    char                    rootCost;
    LinkedList*             downStream;
} MospfForwardingTableRow;

// Forwarding table
typedef struct routing_mospf_forwarding_table_str
{
    unsigned int             numRows;
    unsigned int             allottedNumRow;
    MospfForwardingTableRow* row;
} MospfForwardingTable;

//----------------------------------------------------------------------//
//       Structure To Handle The Simulation Statistics for Mospf
//----------------------------------------------------------------------//

// Statistics structure
typedef struct
{
    int numGroupMembershipLSAGenerated;
    int numGroupMembershipLSAFlushed;
    int numGroupJoin;
    int numGroupLeave;

    int numOfMulticastPacketGenerated;
    int numOfMulticastPacketDiscard;
    int numOfMulticastPacketForwarded;
    int numOfMulticastPacketReceived;

    BOOL alreadyPrinted;
} MospfStats;

//----------------------------------------------------------------------//
//        Mospf Layer structure & interface structure for a node
//----------------------------------------------------------------------//

// Various values kept for each interface.
typedef struct
{

    // for group member Info
    Ospfv2LinkType           linkType;

    // Not used in simulation
    MospfForwardingType      forwardingType;

} MospfInterface;

// MOSPF Layer structure for node
typedef struct
{
    BOOL                    interAreaMulticastFwder;
    //unsigned int            rootAreaId;
    MospfForwardingTable    forwardingTable;
    BOOL                    showStat;
    MospfStats              stats;
    MospfInterface*         mInterface;
    RandomSeed              seed;
#ifdef ADDON_DB
    StatsDBMospfSummary  mospfSummaryStats;
    StatsDBMulticastNetworkSummaryContent  mospfMulticastNetSummaryStats;
#endif
} MospfData;


//----------------------------------------------------------------------//
//          Various Initialization Function for Mospf
//----------------------------------------------------------------------//
void MospfInit(Node* node, const NodeInput* nodeInput, int interfaceIndex);

//----------------------------------------------------------------------//
//          Various Finalization Function for Mospf
//----------------------------------------------------------------------//
void MospfFinalize(Node*  node);

//----------------------------------------------------------------------//
//    Various Function To handle Group Related Information for Mospf
//----------------------------------------------------------------------//

void MospfLocalMembersJoinOrLeave(
    Node* node,
    NodeAddress groupAddr,
    int interfaceId,
    LocalGroupMembershipEventType event);

void MospfScheduleGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    unsigned int interfaceId,
    NodeAddress groupId,
    BOOL flush);

void MospfCheckLocalGroupDatabaseToOriginateGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    unsigned int interfaceId,
    NodeAddress groupId,
    BOOL flush);

BOOL MospfCheckGroupMembershipLSABody(
    MospfGroupMembershipLSA* newLSA,
    MospfGroupMembershipLSA* oldLSA);

BOOL MospfInstallGroupMembershipLSAInLSDB(
    Node* node,
    unsigned int areaId, char* LSA);

//----------------------------------------------------------------------//
//   Various Function To handle Packet Forwarding Using Mospf
//----------------------------------------------------------------------//

void MospfForwardingFunction(
    Node* node,
    Message* msg,
    GROUP_ID destAddr,
    int inInterface,
    BOOL* packetWasRouted,
    NodeAddress prevHop);

void MospfModifyShortestPathAndForwardingTable(Node* node);

// /**
// FUNCTION   :: MospfPrintTraceXMLLsa
// LAYER      :: NETWORK
// PURPOSE    :: Print MOSPF Specific Trace
// PARAMETERS ::
// + node      : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv2LinkStateHeader* : Pointer to Ospfv2LinkStateHeader
// RETURN     ::  void : NULL
// **/

void MospfPrintTraceXMLLsa(Node* node, Ospfv2LinkStateHeader* lsaHdr);

#endif // _MOSPF_H_






