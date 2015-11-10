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
// PACKAGE     :: CELLULAR_MAC
// DESCRIPTION :: Defines the data structures used in the Cellular MAC
//                Most of the time the data structures and functions start
//                with MacCellular**
// **/

#ifndef _CELLULAR_MAC_H_
#define _CELLULAR_MAC_H_

#ifdef CELLULAR_LIB
#include "mac_cellular_abstract.h"
#endif

#ifdef UMTS_LIB
#include "layer2_umts.h"
#endif

#ifdef MUOS_LIB
#include "layer2_muos.h"
#endif

// /**
// ENUM        :: CellularMACProtocolType
// DESCRIPTION :: Enlisted different Cellular MAC Protocols
// **/
typedef enum
{
    Cellular_ABSTRACT_Layer2 = 0,  // abstract cellular MAC
    Cellular_GSM_Layer2,           // GSM cellular MAC
    Cellular_GPRS_Layer2,          // GPRS cellular MAC
    Cellular_UMTS_Layer2,          // UMTS cellular MAC
    Cellular_CDMA2000_Layer2,      // CDMA2000 cellular MAC
    Cellular_MUOS_Layer2,          // MUOS cellular MAC
} CellularMACProtocolType;

// /**
// STRUCT      :: MacCellularData
// DESCRIPTION :: Definition of the data structure for the cellular MAC
// **/
typedef struct struct_mac_cellular_str
{
    MacData *myMacData;
    RandomSeed randSeed;

    // Must be set at initialization for every node
    CellularMACProtocolType cellularMACProtocolType;

#ifdef CELLULAR_LIB
    // Must be set at initialization for every node
    CellularNodeType nodeType;
    // Each interface has only one of the following based on its type,
    MacCellularAbstractData *macCellularAbstractData;
#endif

#ifdef UMTS_LIB
    CellularUmtsLayer2Data *cellularUmtsL2Data;
#endif

#ifdef MUOS_LIB
    CellularMuosLayer2Data *cellularMuosL2Data;
#endif
    //The following not defined yet
    //MacCellularGSMData *macCellularGSMData;
    //MacCellularGPRSData *macCellularGPRSData;

    BOOL collectStatistics;    // whether collect satistics
} MacCellularData;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacCellularInit
// LAYER      :: MAC
// PURPOSE    :: Initialize Cellular MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacCellularInit(Node *node,
                     int interfaceIndex,
                     const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacCellularFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacCellularFinalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacCellularLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacCellularLayer(Node *node, int interfaceIndex, Message *msg);

// /**
// FUNCTION   :: MacCellularNetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
//               This function is not used in the cellular model
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + cellularMac      : MacCellularData*  : Pointer to cellular MAC structure
// RETURN     :: void : NULL
// **/
void MacCellularNetworkLayerHasPacketToSend(
         Node *node,
         int inerfaceIndex,
         MacCellularData *cellularMac);

// /**
// FUNCTION   :: MacCellularReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularReceivePacketFromPhy(
         Node *node,
         int interfaceIndex,
         Message *msg);
#endif /* _CELLULAR_MAC_H_ */
