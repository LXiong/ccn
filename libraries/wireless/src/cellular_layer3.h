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
// PACKAGE     :: CELLULAR_LAYER3
// DESCRIPTION :: Defines the data structures used in the Cellular Framework
//                Most of the time the data structures and functions start
//                with CellularLayer3**
// **/

#ifndef _CELLULAR_LAYER3_H_
#define _CELLULAR_LAYER3_H_

#include "cellular.h"

#ifdef CELLULAR_LIB
//Abstrat
#include "cellular_abstract.h"
#include "cellular_abstract_layer3.h"
#endif

#ifdef UMTS_LIB
//UMTS
#include "cellular_umts.h"
#include "layer3_umts.h"
#endif

#ifdef MUOS_LIB
//MUOS
#include "cellular_muos.h"
#include "layer3_muos.h"
#endif

//define the maximum cellular interfece a cellular node could have.
//Currently only one cellular interface is supported besides the IP interface
//6 would match the definition of protocol type
//#define CELLULAR_MAXIMUM_INTERFACE_PER_NODE 1
//#define CELLULAR_INTERFACE_ABSTRACT   0
//#define CELLULAR_INTERFACE_GSM        1
//#define CELLULAR_INTERFACE_GPRS       2
//#define CELLULAR_INTERFACE_UMTS       3
//#define CELLULAR_INTERFACE_CDMA2000   4

// /**
// ENUM        :: CellularLayer3ProtocolType
// DESCRIPTION :: Enlisted different Cellular Protocols
// **/
typedef enum
{
    Cellular_ABSTRACT_Layer3 = 0,
    Cellular_GSM_Layer3,
    Cellular_GPRS_Layer3,
    Cellular_UMTS_Layer3,
    Cellular_CDMA2000_Layer3,
    Cellular_MUOS_Layer3,
}CellularLayer3ProtocolType;

// /**
// STRUCT      :: CellularLayer3Data
// DESCRIPTION :: Definition of the data structure for the cellular Layer3
// **/
typedef struct struct_cellular_layer3_str
{
    // Must be set at initialization for every Cellular node
    CellularNodeType nodeType;

    RandomSeed randSeed;
    int interfaceIndex;
    CellularLayer3ProtocolType cellularLayer3ProtocolType;

    // Each interface has only one of the following based on its type,
    // one node cannot have two identical types of interface,
    // so the total interface is liitted by the number of protocol type
#ifdef CELLULAR_LIB
    // Abstract Cellular Model
    CellularAbstractLayer3Data *cellularAbstractLayer3Data;

    //The following not defined yet
    //CellularGSMLayer3Data *cellularGSMLayer3Data;
    //CellularGPRSLayer3Data *cellularGPRSLayer3Data;
#endif

#ifdef UMTS_LIB
    // UMTS model
    UmtsLayer3Data *umtsL3;
#endif

#ifdef MUOS_LIB
    // MUOS model
    MuosLayer3Data *muosL3;
#endif

    BOOL collectStatistics;
}CellularLayer3Data;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: CellularLayer3PreInit
// LAYER      :: Layer3
// PURPOSE    :: Preinitialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularLayer3PreInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularLayer3Init
// LAYER      :: Layer3
// PURPOSE    :: Initialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularLayer3Init(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularLayer3Finalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularLayer3Finalize(Node *node);

// /**
// FUNCTION   :: CellularLayer3Layer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void CellularLayer3Layer(Node *node, Message *msg);
// /**
// FUNCTION   :: CellularLayer3ReceivePacketOverIp
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret
// + interfaceIndex   : int      : Interface from which packet was received
// + sourceAddress    : Address  : Node address of the source node
// RETURN     :: void : NULL
// **/
void CellularLayer3ReceivePacketOverIp(Node *node,
                                       Message *msg,
                                       int interfaceIndex,
                                       Address sourceAddress);
// /**
// FUNCTION   :: CellularLayer3ReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// + interfaceIndex   : int               : Interface from which the packet is received
// RETURN     :: void : NULL
// **/
void CellularLayer3ReceivePacketFromMacLayer(Node *node,
                                            Message *msg,
                                            NodeAddress lastHopAddress,
                                            int interfaceIndex);

// /**
// FUNCTION   :: CellularLayer3IsUserDevices
// LAYER      :: Layer3
// PURPOSE    :: TO see if it is a user devices ranther than fixed nodes
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface from which the packet is received
// RETURN     :: void : NULL
// **/
BOOL CellularLayer3IsUserDevices(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: CellularLayer3IsPacketGateway
// LAYER      :: Layer3
// PURPOSE    :: To check if the node is a packet gateway which connects
//               cellular network with packet data network (PDN).
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// RETURN     :: void  : NULL
// **/
BOOL CellularLayer3IsPacketGateway(Node *node);

// /**
// FUNCTION   :: CellularLayer3IsPacketForMyPlmn
// LAYER      :: Layer3
// PURPOSE    :: Check if destination of the packet is in my PLMN
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Packet to be checked
// + inIfIndex : int      : Interface from which the packet is received
// + network   : int      : network type. IPv4 or IPv6
// RETURN     :: BOOL     : TRUE if pkt for my PLMN, FALSE otherwise
// **/
BOOL CellularLayer3IsPacketForMyPlmn(
        Node* node,
        Message* msg,
        int inIfIndex,
        int networkType);

// /**
// FUNCTION   :: CellularLayer3HandlePacketFromUpperOrOutside
// LAYER      :: Layer3
// PURPOSE    :: Handle packets from upper layer app or outside PDN
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + msg            : Message* : Message to be sent onver the air interface
// + interfaceIndex : int      : Interface from which the packet is received
// + networkType    : int      : use IPv4 or IPv6
// RETURN     :: void          : NULL
// **/
void CellularLayer3HandlePacketFromUpperOrOutside(
           Node* node,
           Message* msg,
           int interfaceIndex,
           int networkType);

#endif /* _CELLULAR_LAYER3_H_ */
