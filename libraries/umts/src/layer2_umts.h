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
// PACKAGE     :: CELLULAR_UMTS_LAYER2
// DESCRIPTION :: Defines the data structures used in the UMTS Model
//                Most of the time the data structures and functions start 
//                with CellularUmtsL2**         
// **/

#ifndef _CELLULAR_UMTS_LAYER2_H_
#define _CELLULAR_UMTS_LAYER2_H_

#include "layer2_umts_rlc.h"
#include "layer2_umts_mac.h"
#include "layer2_umts_pdcp.h"

// /**
// CONSTANT    :: UMTS_LAYER2_HANDOFFPKT_DELAY
// DESCRIPTION :: Delay will be incurred for Handleoff packet at CRNC 
// **/
const clocktype UMTS_LAYER2_HANDOFFPKT_DELAY = 1 * MILLI_SECOND;

// /**
// ENUM::   UmtsLayer2SublayerType
// DESCRIPTION::
//      Layer2 sublayer type
// **/ 
typedef enum 
{
    UMTS_LAYER2_SUBLAYER_MAC,  // MAC sublayer
    UMTS_LAYER2_SUBLAYER_RLC,  // RLC sublayer
    UMTS_LAYER2_SUBLAYER_PDCP, // PDCP sublayer 
    UMTS_LAYER2_SUBLAYER_BMC,  // BMC sublayer
    UMTS_LAYER2_SUBLAYER_NONE  // default, for developing purpose only
} UmtsLayer2SublayerType;

// /**
// STRUCT:: UmtsLayer2TimerType
// DESCRIPTION::
//      Layer2 timer Type
// **/ 
typedef enum
{
    // MAC
    UMTS_MAC_SLOT_TIMER,
    UMTS_MAC_RACH_T2, // T2
    UMTS_MAC_RACH_BO, // backoff  timer
     
    // RLC
    UMTS_RLC_TIMER_RST,
    UMTS_RLC_TIMER_POLL,

    // MISC
    UMTS_RLC_TIMER_UPDATE_GUI,
}UmtsLayer2TimerType;

// /**
// STRUCT:: UmtsLayer2TimerInfoHdr
// DESCRIPTION::
//      Layer2 timer information
// **/ 
typedef struct
{
    UmtsLayer2SublayerType  subLayerType;
    UmtsLayer2TimerType timeType;
    unsigned int infoLen;
} UmtsLayer2TimerInfoHdr;

// /**
// STRUCT      :: CellularUmtsLayer2Data
// DESCRIPTION :: Layer2 data of the cellular UMTS LAYER2
// **/
typedef struct cellular_umts_layer2_str
{
    UmtsRlcData* umtsRlcData;
    UmtsMacData* umtsMacData;
    UmtsPdcpData* umtsPdcpData;
    //UmtsBmcData* umtsBmcData;
}CellularUmtsLayer2Data;

//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------

// /**
// FUNCTION::   UmtsLayer2InitTimer
// LAYER::      UMTS LAYER2
// PURPOSE::    Initiate a timer message for RLC or MAC
// PARAMETERS:: 
// + node             :  Node*    : node pointer
// + interfaceIndex   :  UInt32   : the interface index
// + sublayerType     :  UmtsLayer2SublayerType : sublayer type MAC/RLC
// + timerType        :  UmtsLayer2TimerType    :   Timer type
// + infoLen          :  unsigned int : Length of additional info
// + info             :  char*    : Additional info data
// RETURN:: MESSAGE*  : Timer message
// **/ 
Message* UmtsLayer2InitTimer(Node* node,
                             UInt32 interfaceIndex,
                             UmtsLayer2SublayerType sublayerType,
                             UmtsLayer2TimerType timerType,
                             unsigned int infoLen,
                             char*   info);

// /**
// FUNCTION   :: UmtsLayer2PrintRunTimeInfo
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Print run time info UMTS layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + sublayerType     : UmtsLayer2SublayerType : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer2PrintRunTimeInfo(Node *node,
                                UInt32 interfaceIndex,
                                UmtsLayer2SublayerType sublayerType);

// /**
// FUNCTION   :: isNodeTypeUe
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a UE at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if UE, FALSE if not
// **/
BOOL isNodeTypeUe(Node* node, UInt32 iface);

// /**
// FUNCTION   :: isNodeTypeNodeB
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a NodeB at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if NodeB, FALSE if not
// **/
BOOL isNodeTypeNodeB(Node* node, UInt32 iface);

// /**
// FUNCTION   :: isNodeTypeRnc
// LAYER      :: UMTS LAYER2
// PURPOSE    :: determine if it is a RNC at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + iface            : UInt32            : Interface index
// RETURN     :: BOOL : TRUE if RNC, FALSE if not
// **/
BOOL isNodeTypeRnc(Node* node, UInt32 iface);

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION::       UmtsLayer2UpperLayerHasPacketToSend
// LAYER::          UMTS Layer2 RLC 
// PURPOSE::        Called by upper layers to request
//                  Layer2 to send SDU
// PARAMETERS::     
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + rbId:       radio bearer Id asscociated with this RLC entity
//    + msg:        The SDU message from upper layer              
//    + ueId:       UE identifier, used to look up RLC entity at UTRAN side
// RETURN::         NULL
// **/ 
void  UmtsLayer2UpperLayerHasPacketToSend(
        Node* node,
        int interfaceIndex,
        unsigned int rbId,
        Message* msg,
        NodeAddress ueId = 0);

// /**
// FUNCTION   :: UmtsLayer2Init
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Initialize UMTS layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsLayer2Init(Node *node,
                    UInt32 interfaceIndex,
                    const NodeInput* nodeInput);

// /**
// FUNCTION   :: UmtsLayer2Finalize
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void : NULL
// **/
void UmtsLayer2Finalize(Node *node, UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsLayer2Layer
// LAYER      :: UMTS LAYER2
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void UmtsLayer2Layer(Node *node, UInt32 interfaceIndex, Message *msg);

#endif /* _CELLULAR_UMTS_LAYER2_H_ */
