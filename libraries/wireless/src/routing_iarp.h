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

#ifndef _IARP_NETWORK_H_
#define _IARP_NETWORK_H_


typedef struct struct_iarp_str IarpData;

//UserCodeStart
#include "clock.h"
#include "routing_zrp.h"
#include "buffer.h"
#include "network_ipv4_ndp.h"

//   Define IARP packet type
typedef enum enum_iarp_update_packet_type {
    NDP_PERIODIC_UPDATE = 0,
    IARP_PERIODIC_UPDATE = 1
} IarpUpdatePacketType;


//   Currently only one type of metric is defined.
//   That one and only metric is "Hop count"
typedef enum enum_iarp_metric_type {
    IARP_METRIC_HOP_COUNT = 0
} IarpMetric;



//   This structure defines "IARP Routing Table Entry" Type.
//   Where "IARP Routing Table Entry" defines each
//   row in the IARP Routing Table
typedef struct iarp_routing_entry_struct {
    NodeAddress destinationAddress;
    NodeAddress nextHopAddress;
    NodeAddress subnetMask;
    unsigned int hopCount;
    int outGoingInterface;
    clocktype lastHard;
    NodeAddress* path; // path is list of ip address of the nodes starting from nexthop to destination
} IarpRoutingEntry;


typedef struct iarp_routing_table_struct {
    IarpRoutingEntry* routingTable;
    unsigned int numEntries;
    unsigned int maxEntries;
} IarpRoutingTable;


typedef struct iarp_link_state_info_struct {
    NodeAddress destinationAddress;
    unsigned int subnetMask;
    int iarpMetric;
    clocktype insertTime;
    struct iarp_link_state_info_struct* next;
    struct iarp_link_state_info_struct* prev;
} IarpLinkStateInfo;


typedef struct link_state_table_entry_struct {
    NodeAddress linkSource;
    unsigned int zoneRadius;
    NodeAddress linkStateId;
    clocktype insertTime;
    IarpLinkStateInfo* linkStateinfo;
    struct link_state_table_entry_struct* next;
    struct link_state_table_entry_struct* prev;
} IarpLinkStateTableEntry;


typedef struct iarp_link_state_table {
    IarpLinkStateTableEntry* first;
    IarpLinkStateTableEntry* last;
} IarpLinkStateTable;


typedef struct iarp_gen_header_struct {
    NodeAddress linkSourceAddress;
    unsigned short linkStateSeqNum;
    unsigned char zoneRadius;
    unsigned char ttl;
    unsigned char reserved[3];
    unsigned char linkDestCnt;
} IarpGeneralHeader;


typedef struct neighbor_info_struct {
    NodeAddress linkDestinationAddress;
    NodeAddress linkDestSubnetMask;
    unsigned char reserved;
    unsigned char metricType;
    short metricValue;
} IarpNeighborInfo;


typedef struct seq_num_table_struct {
    NodeAddress linkSource;
    unsigned short seqNum;
    struct seq_num_table_struct* leftChild;
    struct seq_num_table_struct* rightChild;
} SeqTable;


typedef struct shortesPathStruct {
    NodeAddress dest;
    NodeAddress nextHop;
    IarpRoutingEntry* routePtr;
} ShortesPath;



//UserCodeEnd

typedef struct {
    unsigned numTriggeredUpdate;
    int numTriggeredUpdateId;
    unsigned numRegularUpdate;
    int numRegularUpdateId;
    unsigned numUpdateRelayed;
    int numUpdateRelayedId;
    unsigned numUpdateReceived;
    int numUpdateReceivedId;
    unsigned numPacketRouted;
    int numPacketRoutedId;
} IarpStatsType;


enum {
    IARP_ROUTE_PACKET,
    IARP_FINAL_STATE,
    IARP_PROCESS_RECEIVED_PACKET,
    IARP_IDLE,
    IARP_PROCESS_AGED_OUT_ROUTES,
    IARP_PROCESS_BROADCAST_TIMER_EXPIRED,
    IARP_INITIAL_STATE,
    IARP_DEFAULT_STATE
};



struct struct_iarp_str
{
    int state;
    IarpRoutingTable routeTable; //IARP routing table structure.
    unsigned zoneRadius;
    clocktype iarpBroadcastTimer; //IARP broadcast timer
    clocktype iarpRefreashTimer;
    clocktype linkStateLifeTime;
    IarpLinkStateTable iarpLinkStateTable;
    unsigned int numOfMyNeighbor;
    NodeAddress myLinkSourceAddress;
    unsigned short linkStateSeqNum;
    SeqTable* seqTable;
    IerpUpdateFunc ierpUpdateFunc;
    IarpStatsType stats;
    RandomSeed seed;
    int initStats;
    int printStats;
    BOOL statsPrinted;
};


void
IarpInit(Node* node, IarpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex);


void
IarpFinalize(Node* node);


void
IarpProcessEvent(Node* node, Message* msg);


void IarpRunTimeStat(Node* node, IarpData* dataPtr);


void IarpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                              NodeAddress destAddr, int interfaceIndex, unsigned ttl);
void IarpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                        NodeAddress previousHopAddr, BOOL* packetWasRouted);



//Interface Utility Functions


// FUNCTION IarpEnterRouteInRoutingTable
// PURPOSE  EnteringRoute In the routing table
// Parameters:
//  node:   node which is adding data into the IARP routing table
//  iarpData:   pointer to Iarp data structure
//  iarpRoutingTable:
//  iarpRoutingEntry:   iarp routing table entry (a row in the routing table)
IarpRoutingEntry* IarpEnterRouteInRoutingTable(Node* node,IarpData* iarpData,IarpRoutingTable* iarpRoutingTable,IarpRoutingEntry* iarpRoutingEntry);


// FUNCTION IarpSearchRouteInRoutingTableEntry
// PURPOSE  searching a route in routing table
// Parameters:
//  iarpData:   pointer to IarpData
//  iarpRoutingTable:
//  destinationAddress: entry to be searched in the routing table
IarpRoutingEntry* IarpSearchRouteInRoutingTableEntry(IarpData* iarpData,IarpRoutingTable* iarpRoutingTable,NodeAddress destinationAddress);


// FUNCTION IarpSearchLinkSourceInLinkStateTable
// PURPOSE  Searching a link source address in the link state table.
//          And returning a pointer to the ilnk source entry if
//          exists or NULL otherwise
// Parameters:
//  iarpData:   pointer to iarp data
//  linkSourceAddress:  the ip address of the node
IarpLinkStateTableEntry* IarpSearchLinkSourceInLinkStateTable(IarpData* iarpData,NodeAddress linkSourceAddress);


// FUNCTION isSourcePresentInQueue
// PURPOSE  adding a node address into a BFS queue. This function is used calculating shortest path
// Parameters:
//  queue:  pointer to queue
//  source: source node address
//  last:
BOOL isSourcePresentInQueue(NodeAddress* queue,NodeAddress source,int last);


// FUNCTION insertElementInQueue
// PURPOSE  Inserting an element in the queue. An "Element" hehe is node's IP address.
// Parameters:
//  queue:  the queue of node addresses
//  source: the IP address of source node
//  last:   last position (last index) in the queue
void insertElementInQueue(NodeAddress* queue,NodeAddress source,int* last);


// FUNCTION IarpFindNextHop
// PURPOSE  Finding a next hop for given destination
// Parameters:
//  iarpData:   pointer to iarp data
//  destAddress:    destination address for which next hop to be found
NodeAddress IarpFindNextHop(IarpData* iarpData,NodeAddress destAddress);


// FUNCTION IarpRegisterZrp
// PURPOSE  Registering Zrp with iarp
// Parameters:
//  node:   node which is initializing
//  dataPtr:    pointer to the iarp data structure
//  nodeInput:  pointer to node input structure
//  interfaceIndex: interface index which is initializing
//  iarpRouterFunc: pointer to iarp router function
void IarpRegisterZrp(Node* node,IarpData* * dataPtr,const NodeInput* nodeInput,int interfaceIndex,RouterFunctionType* iarpRouterFunc);


// FUNCTION IarpSetIerpUpdeteFuncPtr
// PURPOSE  Accept pointer to "IERP routing table update function" and storing ththe pointer in it's
//          internal data structure. This will be used to update IERP routing table from IARP
// Parameters:
//  node:   node which is performing above operation.
//  updateFunc: pointer to IERP routing table update function
void IarpSetIerpUpdeteFuncPtr(Node* node,IerpUpdateFunc updateFunc);


// FUNCTION IarpGetRoutingProtocol
// PURPOSE  Get routing protocol structure associated with routing protocol
// Parameters:
//  node:   Pointer to node
//  routingProtocolType:    Routing protocol to retrieve
void* IarpGetRoutingProtocol(Node* node,NetworkRoutingProtocolType routingProtocolType);



#endif
