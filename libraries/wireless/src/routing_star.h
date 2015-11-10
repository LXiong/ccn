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

#ifndef STAR_H
#define STAR_H


typedef struct
{
    NodeAddress ipAddr_u;    // "An LSU for a link (u,v)"
    NodeAddress ipAddr_v;
    int       link_cost_l; // "where l represents the cost of the link"
    int       timestamp_t; // "and t is the timestamp assigned to the LSU"
}
StarLSU;


typedef struct
{
    int numLSU_sent,
        numLSU_recvd,
        numPkts_sent,
        numPkts_recvd;
}
StarStats;


typedef struct star_topo_graph_edge_entry
{
    NodeAddress ipAddr_u;
    NodeAddress ipAddr_v;
    int       link_cost_l;
    int       timestamp_t;
    int       distance_d;
    BOOL      del;
    struct star_topo_graph_edge_entry **neighbors;
    int numNeighbors;
    int maxNeighbors;
}
StarTopologyGraphEdgeEntry;


typedef struct
{
    NodeAddress ipAddr_vertex;
    NodeAddress ipAddr_pred;
    NodeAddress ipAddr_suc;
    NodeAddress ipAddr_suc_prime;
    int       distance_d;
    int       distance_d_prime;
    int       distance_d_double_prime;
    NodeAddress flag_nbr;
    BOOL      valid;
    BOOL      extracted;
}
StarTopologyGraphVertexEntry;


typedef struct
{
    StarTopologyGraphEdgeEntry *edgeRoot;
    StarTopologyGraphVertexEntry **vertexEntries;

    int                          numVertexEntries;
    int                          maxVertexEntries;

    StarTopologyGraphEdgeEntry **listOfEdges;
    int                        numEdges;
    int                        maxEdges;
}
StarTopologyGraph;


typedef struct
{
    StarTopologyGraph topologyGraph_i;
    StarTopologyGraph topoGraphST;
    StarTopologyGraph topoGraphST_prime;

    NodeAddress ipAddr_i;
    int delta;
    int seq_num;

    BOOL LORAmode;
    BOOL Mi;
    BOOL NSi;

    BOOL statsCollected;
    StarStats stats;

    StarLSU* MSGi;
    int MSGi_numEntries;
    int MSGi_maxEntries;

    RandomSeed seed;

    StarTopologyGraph *topoGraph_k;
    int numTopoGraph_k;
    int maxTopoGraph_k;

    StarTopologyGraph *ShortestPathTree_k;
    int numST_k;
    int maxST_k;
}
StarData;



//
// FUNCTION     StarInit()
// PURPOSE      Initialize MY_ROUTE Routing Protocol Dataspace
// PARAMETERS   myRoute            - pointer for dataspace
//              nodeInput
//
void
StarInit(
   Node* node,
   StarData** myRoute,
   const NodeInput* nodeInput,
   int interfaceIndex);


// PURPOSE      Process a MY_ROUTE generated control packet
// PARAMETERS   msg             - The packet
//
void StarHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress);

//
// FUNCTION     StarHandleProtocolEvent()
// PURPOSE      Process timeouts sent by MY_ROUTE to itself
// PARAMETERS   msg             - the timer
//
void
StarHandleProtocolEvent(
    Node* node,
    Message* msg);


// FUNCTION     StarRouterFunction()
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              previousHopAddr - previous hop of packet
//              PacketWasRouted - variable that indicates to IP that it
//                                no longer needs to act on this data packet
//
/* The Following must match RouterFunctionType defined in ip.h */

void
StarRouterFunction(
    Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddr,
    BOOL *PacketWasRouted);


//
// FUNCTION     StarMacLayerStatusHandler()
// PURPOSE      Handle internal messages from MAC Layer
// PARAMETERS   msg             - the packet returned by MAC
//
void
StarMacLayerStatusHandler(
    Node *node,
    const Message* msg,
    const NodeAddress nextHopAddress,
    const int interfaceIndex);


//
// FUNCTION     StarFinalize()
// PURPOSE      Finalize statistics Collection
//

void StarFinalize(Node *node);



#endif
