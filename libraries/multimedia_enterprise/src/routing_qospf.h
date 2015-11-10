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


#ifndef QOSPF_H
#define QOSPF_H

// Base value, which will be required to decode the exponential
// Qos metric into linear representation.
#define QOSPF_QOS_METRIC_MANTISSA_MAX                 8191.0f
#define QOSPF_QOS_METRIC_BASE_VAL                     8

// Different default values of flooding related parameters
#define QOSPF_DEFAULT_INTERFACE_OBSERVATION_INTERVAL  (2 * SECOND)
#define QOSPF_DEFAULT_FLOODING_FACTOR                 (0.10)

#define QOSPF_MAX_BANDWIDTH          0x0fffffff
#define QOSPF_MIN_DELAY              0
#define QOSPF_NEXT_HOP_UNREACHABLE   -2

// Different vertices
typedef enum
{
    QOSPF_ROUTER,
    QOSPF_TRANSIT_NETWORK,
    QOSPF_STUB_NETWORK,
    QOSPF_STUB_ROUTER

}QospfVertexType;


#define QOSPF_NUM_TOS_METRIC                            2

// Type of Quality of Service.
typedef enum
{
    QOSPF_BANDWIDTH = 20,
    QOSPF_DELAY =     24

} QospfTypeOfQos;


// Type of Qos path calculation algorithm.
typedef enum
{
    EXTENDED_BREADTH_FIRST_SEARCH_SINGLE_PATH

} QospfPathCalculationAlgorithm;


// QoS field information. This structure will be added dynamically
// during the construction of router LSA.
typedef struct struct_qospf_per_link_qos_metric_info
{
    // Eight bits are given to encode the type of service
    // (i.e. delay, bandwidth etc) and actually five bits are used
    // to encode a Qos parameter. the remaining three bits are used
    // to identify the queue no.
    unsigned char  queueNumber:3,
                   qos:5;

    // This is actually reserved field in router LSA (according to RFC 2328)
    // But we are using this field to identify index of the interface from
    // where the link originates.
    unsigned char  interfaceIndex;

    // To encode Qos metric value, this 16 bits are given. As
    // bandwidth value is high exponential coding is chosen
    unsigned short qosMetricExponentPart:3,
                   qosMetricMantissaPart:13;
} QospfPerLinkQoSMetricInfo;


/**************************************************************************
 *  Topology and Node structures, that will be collected from Router LSA  *
 **************************************************************************/

// For link's available QoS metric information
typedef struct struct_qospf_qos_constraint
{
    int          queueNumber;         // number of queue, considered
    unsigned int availableBandwidth;  // available link bandwidth of queue
    unsigned int averageDelay;        // average queuing delay of the queue
    TosType      priority; // User priority.
} QospfQosConstraint;


// These following structures given below define the topological
// information of a area for all nodes to that area. The information
// contains all the links present in that area. These links are stored
// as a directed link, i.e. either link is from node 'n' to node
//'m' or from node 'm' to 'n'. Besides, available bandwidth of the
// link and the interface index of the link is collected.

// Collects the information of the end node of a link.
typedef struct struct_qospf_node_linked_to
{
    // Node Id for end node of a link.
    NodeAddress            toRouterId;
    int                    toVertexNumber;

    // These to field identifies different links between two nodes
    NodeAddress            networkAddr;
    NodeAddress            networkMask;

    // Interface index of the node from where the link originates.
    int                    interfaceIndex;
    NodeAddress            ownIpAddr;

    // Interface IP address of this node where the link connected to
    NodeAddress            neighborIpAddr;

    // type of the link
    Ospfv2LinkType         linkType;

    // Available QoS metric for this link.
    int                    numQueues;
    QospfQosConstraint*    queueSpecificCost;
} QospfLinkedTo;


// Collects the information of the start node of a link.
typedef struct struct_qospf_node_linked_from
{
    NodeAddress  fromRouterId;  // Node Id for starting node of a link.
    int          fromVertexNumber;

    // Each element of the list contains QospfLinkedTo
    LinkedList*  linkToInfoList;
} QospfLinkedFrom;


// Collects IP address of a interface of current node and neighbor's
// interfaces that are connected to this interface.
typedef struct struct_qospf_interface_info
{
    int         ownInterfaceIndex;
    NodeAddress ownInterfaceIpAddr;

    // Contains IP interface address of neighbor's connected to this
    // interface. Element of this list is NodeAddress.
    LinkedList* connectedInterfaceIpAddr;
} QospfInterfaceInfo;


// These structures collect set of vertices exists in the area. A
// vertex may be router or network. Each vertex is identified by a
// integer number during path calculation. vertexnNumber is that
// variable, to which a number is assigned according to sequence
// of arrival.
typedef struct struct_qospf_set_of_vertices
{
    int             vertexNumber;   // Pseudo vertex.
                                    // identification number.

    QospfVertexType vertexType;     // Type of this vertex.
    NodeAddress     vertexAddress;  // Address of the vertex.

    // Interface information of the node. The list contains
    // QospfInterfaceInfo structure.
    LinkedList*     vertexInterfaceInfo;
} QospfSetOfVertices;


// Statistics for Q-OSPF
typedef struct struct_qospf_statistics
{
    int  numActiveConnection;    // Node specific active connection
    int  numRejectedConnection;  // Node specific rejected connection
    int  numPeriodicUpdate;      // Node specific periodic update
    int  numTriggeredUpdate;     // Node specific trigerred update
    int* linkBandwidth;          // Interface specific Link Bandwidth
    int* bandwidthUtilization;   // Interface specific Link's utilized Bw
} QospfStatistics;

/*****************************************************************************
 * Structures for Breadth First Search Single Path calculation Algorithm     *
 *****************************************************************************/

typedef struct struct_qospf_qos_metric
{
    unsigned int bandwidth;  // available bandwidth
    unsigned int delay;      // average delay
} QospfQosMetric;

// Defines the link descriptor structure
typedef struct struct_qospf_queue_specific_link_descriptor
{
    int            queueNumber;  // queue considered
    QospfQosMetric metricValue;  // Qos metric of queue
} QospfQSpecificDescriptor;

// Defines link descriptor.
typedef struct struct_qospf_link_descriptor
{
    // Pseudo identification no. of this vertex
    int             toVertexId;

    // Interface index of that interface, from where the link originated.
    int             interfaceIndex;

    // Neighbor's interface IP address to which the link connected.
    NodeAddress     interfaceIpAddr;

    // List contains available Qos metric of different queues for a link.
    // Element of this list is QospfQSpecificDescriptor structure.
    LinkedList*     queueSpecificLinkDescriptors;
} QospfLinkDescriptor;

// Defines the Dp structure of a vertex
typedef struct struct_qospf_Dp_structure
{
    int        previousVertexId;            // previous vertex.
    int        previousVertexInterfaceId;   // interface index of previous
                                            // vertex from where node
                                            // descriptor originated.
    NodeAddress  nextHop;                   // Next hop address.
} QospfDpStructure;


// Defines special structure for path calculation using Extended
// Breadth First Search Single Path calculation algorithm
typedef struct struct_qospf_descriptor_structure
{
    int             fromVertexId;    // Vertex Id
    NodeAddress     vertexAddress;   // Vertex Router Address
    QospfQosMetric  nodeDescriptor;  // Node descriptor

    // List contains link descriptor, i.e. the available Qos metric
    // of each link originated from this node. Element of this list
    // QospfLinkDescriptor type structure.
    LinkedList*     linkDescriptorsFromThisNode;

    // List contains the neighbor vertices of the vertex. Each element of
    // this list contains integer value.
    LinkedList*     neighborVertex;
    QospfDpStructure     nodeDpValue;
} QospfDescriptorStructure;

// Defines the structure, which collects releavent information of a
// newly arrived connection. If a Qos path found for this new
// connection, it also stores the path for that connection.
typedef struct struct_qospf_different_session_information
{
    int         sessionId;
    short       sourcePort;
    short       destPort;
    NodeAddress sourceAddress;
    NodeAddress destAddress;

    BOOL          isOriginator;
    clocktype     sessionAdmittedAt;
    unsigned int numTimesRetried;

    // Requested Qos Constraint
    QospfQosConstraint requestedMetric;

    // Bandwidth is the requested bandwidth of the data
    int actualBandwidthRequirement;

    // Path that will be used to forward the data packets
    NodeAddress* routeAddresses;
    int numRouteAddresses;
} QospfDifferentSessionInformation;


/**************************************************************************
 * Layer Structure of Q-OSPF                                              *
 **************************************************************************/

// QOSPF Layer structure for node
typedef struct
{
    // Each element of the list contains QospfLinkedFrom
    // structure. This is required to store the topological
    // information of the area in a router.
    LinkedList* topologyLinksList;

    // Set of vertices. Each item of the list contains
    // QospfSetOfVertices structure.
    LinkedList* vertexSet;

    int   maxQueueInEachInterface;

    // Path calculation algorithm, which will be used to find out
    // the path that will be used to forward the data packet.
    QospfPathCalculationAlgorithm pathCalculationAlgorithm;

    // Flag, if TRUE queue delay will be considered for path QoS calculation
    BOOL      isQueueDelayConsidered;

    BOOL collectStat;
    QospfStatistics qospfStat;

    // Periodic flooding interval received from config file
    clocktype floodingInterval;

    // Interface status observation interval and a relative factor which
    // indicates to trigger flooding of LSA due to abrupt change in link
    // utilization.
    clocktype interfaceMonitorInterval;
    double    floodingFactor;

    // List contains the paths for each connection. Elements of this
    // list is QospfDifferentSessionInformation
    LinkedList* pathList;

    BOOL trace;

} QospfData;


void QospfInit(
    Node* node,
    QospfData** qospfLayerPtr,
    const NodeInput* nodeInput);

void QospfFinalize(Node* node, QospfData* qospf);

void QospfOriginateLSA(
    Node* node, QospfData* qospf, BOOL isPeriodicUpdate);

void QospfGetQosInformationForTheLink(
    Node* node,
    int interfaceIndex,
    Ospfv2LinkInfo** linkAddress);

void QospfCreateDatabaseAndFindQosPath(
    Node* node, Message* msgForConnectionRequest);

BOOL QospfIsPacketDemandedQos(Node* node, Message* msg);

void QospfForwardQosApplicationPacket(
    Node* node,
    Message* msg,
    BOOL* PacketWasRouted);

#endif // QOSPF_H
