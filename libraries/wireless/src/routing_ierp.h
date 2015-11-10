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

#ifndef _IERP_NETWORK_H_
#define _IERP_NETWORK_H_


typedef struct struct_ierp_str IerpData;

//UserCodeStart
#include "buffer.h"
#include "routing_zrp.h"

//    Define IERP packet type used.
typedef enum {
    IERP_QUERY_PACKET = 0,
    IERP_REPLY_PACKET = 1
} IerpPacketType;



//    Define IERP routing table entry structure
typedef struct {
    NodeAddress destinationAddress;
    NodeAddress subnetMask;
    clocktype lastUsed;
    unsigned int hopCount;
    NodeAddress* pathList;
} IerpRoutingTableEntry;


//    Define IERP routing table structure.
typedef struct {
    IerpRoutingTableEntry* ierpRoutingTableEntry;
    unsigned int numEntries;
    unsigned int maxEntries;
} IerpRoutingTable;


//    Define a sequence table for keeping track of already
//    seen query sequence numbers
typedef struct ierp_seq_num_table_struct {
    NodeAddress linkSource;
    unsigned short seqNum;
    struct ierp_seq_num_table_struct* leftChild;
    struct ierp_seq_num_table_struct* rightChild;
} IerpSeqTable;


typedef struct ierp_general_header_struct {
    unsigned char type;
    unsigned char length;
    unsigned char nodePtr;
    unsigned char reserved1;
    unsigned short queryId;
    unsigned short reserved2;
} IerpGeneralHeader;


typedef struct {
    void* anyPtr;
    unsigned int anyAddress;
} MessageExtraInfo;


//  Structure to store packets temporarily until one route to the destination of the packet is found or the packets are timed out to find a route
typedef struct str_ierp_fifo_buffer {
    NodeAddress destAddr; // Destination address of the packet
    Message* msg; // The packet to be sent
    struct str_ierp_fifo_buffer* next; // Pointer to the next message
} IerpBufferNode;


typedef struct {
    IerpBufferNode* head;
    int size; // Number of packets
} IerpMessageBuffer;



//UserCodeEnd

typedef struct {
    unsigned numQueryPacketSend;
    int numQueryPacketSendId;
    unsigned numQueryPacketRecv;
    int numQueryPacketRecvId;
    unsigned numQueryPacketRelayed;
    int numQueryPacketRelayedId;
    unsigned numReplyPacketSend;
    int numReplyPacketSendId;
    unsigned numReplyPacketRecv;
    int numReplyPacketRecvId;
    unsigned numReplyPacketRelayed;
    int numReplyPacketRelayedId;
    unsigned numPacketRouted;
    int numPacketRoutedId;
    unsigned numDataDroppedForBufferOverFlow;
    int numDataDroppedForBufferOverFlowId;
} IerpStatsType;


enum {
    IERP_PROCESS_IERP_QUERY_TIMER_EXPIRED,
    IERP_SEND_QUERY_PACKET,
    IERP_ROUTE_PACKET,
    IERP_IDLE_STATE,
    IERP_MAC_EVENT,
    IERP_PROCESS_REPLY_PACKET,
    IERP_IERP_ROUTER_FUNCTION,
    IERP_PROCESS_QUERY_PACKET,
    IERP_INITIAL_STATE,
    IERP_RECEIVE_PACKET_FROM_NETWORK_LAYER,
    IERP_PROCESS_FLUSH_TIMER_EXPIRED,
    IERP_FINAL_STATE,
    IERP_DEFAULT_STATE
};



struct struct_ierp_str
{
    int state;

    IerpRoutingTable ierpRoutingTable;
    unsigned short queryId;
    unsigned int zoneRadius;
    IerpSeqTable* seqTable;
    void* brpData;
    clocktype ierpRouteRequestTimer;
    IerpMessageBuffer msgBuffer;
    int maxBufferSize;
    BOOL enableBrp;
    IerpStatsType stats;
    RandomSeed seed;
    int initStats;
    int printStats;
    BOOL statsPrinted;
};


void
IerpInit(Node* node, IerpData** dataPtr, const NodeInput* nodeInput, int interfaceIndex);


void
IerpFinalize(Node* node);


void
IerpProcessEvent(Node* node, Message* msg);


void IerpRunTimeStat(Node* node, IerpData* dataPtr);


void IerpMacLayerStatusHandler(Node* node, const Message* msg,
                               const NodeAddress nextHopAddress, const int interfaceIndex);
void IerpHandleProtocolPacket(Node* node,Message* msg, NodeAddress sourceAddr,
                              NodeAddress destAddr, int interfaceIndex, unsigned ttl);
void IerpRouterFunction(Node* node, Message* msg, NodeAddress destAddr,
                        NodeAddress previousHopAddr, BOOL* packetWasRouted);



//Interface Utility Functions


// FUNCTION IerpReturnRouteTableUpdateFunction
// PURPOSE  Returning routing table update function pointer
// Parameters:
//  node:   node which will return the function pointer
IerpUpdateFunc IerpReturnRouteTableUpdateFunction(Node* node);


// FUNCTION IerpRegisterZrp
// PURPOSE  Build a co-ordination system with zrp.
// Parameters:
//  node:   node which is initializing.
//  dataPtr:    pointer to the ierp data structure.
//  nodeInput:  pointer to node input
//  interfaceIndex: interface index which is initializing
//  ierpRouterFunc: pointer to ierp router function
//  macStatusHandlar:   pointer to mac layer function handler
void IerpRegisterZrp(Node* node,IerpData* * dataPtr,const NodeInput* nodeInput,int interfaceIndex,RouterFunctionType* ierpRouterFunc,MacLayerStatusEventHandlerFunctionType* macStatusHandlar);


// FUNCTION IerpGetRoutingProtocol
// PURPOSE  Get routing protocol structure associated with routing protocol
// Parameters:
//  node:   Pointer to node
//  routingProtocolType:    Routing protocol to retrieve
void* IerpGetRoutingProtocol(Node* node,NetworkRoutingProtocolType routingProtocolType);



#endif
