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

#ifndef _LAYER2_LTE_H_
#define _LAYER2_LTE_H_

#include <list>

#include "lte_common.h"
#include "lte_rrc_config.h"
#include "layer2_lte_rlc.h"
#include "layer2_lte_pdcp.h"
#include "layer2_lte_sch.h"
#include "layer2_lte_mac.h"
#include "layer3_lte.h"

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG

#include "log_lte.h"

struct LteLayer2ValidationData {
    UInt32 rlcNumSduToTxQueue; // RLC : Number of SDU to Tx Queue
    UInt32 rlcNumSduToPdcp;    // RLC : Number of SDU to PDCP
    UInt64 rlcBitsToPdcp;      // RLC : Bits of SDU to PDCP

    LteLayer2ValidationData()
        : rlcBitsToPdcp(0)
        , rlcNumSduToTxQueue(0)
        , rlcNumSduToPdcp(0)
    {
    }
};
typedef std::pair < LteRnti, LteLayer2ValidationData* >
    PairLteLayer2ValidationData;
typedef std::map < LteRnti, LteLayer2ValidationData* >
    MapLteLayer2ValidationData;
#endif
#endif

///////////////////////////////////////////////////////////////
// prototype
///////////////////////////////////////////////////////////////
struct lte_rrc_config;
class LteLayer3Filtering;
struct struct_layer3_lte_str;

///////////////////////////////////////////////////////////////
// define
///////////////////////////////////////////////////////////////
#ifdef PARALLEL
  #define LTE_LAYER2_DEFAULT_USE_SPECIFIED_DELAY (TRUE)
  #define LTE_LAYER2_DEFAULT_DELAY_UNTIL_AIRBORN (100 * NANO_SECOND)
#else
  #define LTE_LAYER2_DEFAULT_USE_SPECIFIED_DELAY (FALSE)
  #define LTE_LAYER2_DEFAULT_DELAY_UNTIL_AIRBORN (0)
#endif // PARALLEL_LTE
#define LTE_LAYER2_DEFAULT_NUM_SF_PER_TTI (1)


///////////////////////////////////////////////////////////////
// typedef enum
///////////////////////////////////////////////////////////////
// /**
// CONSTANT    :: LTE_LAYER2_HANDOFFPKT_DELAY
// DESCRIPTION :: Delay will be incurred for Handleoff packet at CRNC
// **/
//const clocktype LTE_LAYER2_HANDOFFPKT_DELAY = 1 * MILLI_SECOND;

// /**
// ENUM:: LteLayer2SublayerType
// DESCRIPTION:: Layer2 sublayer type
// **/
typedef enum
{
    LTE_LAYER2_SUBLAYER_MAC,  // layer2 sublayer
    LTE_LAYER2_SUBLAYER_RLC,  // RLC sublayer
    LTE_LAYER2_SUBLAYER_PDCP, // PDCP sublayer
    LTE_LAYER2_SUBLAYER_SCH,  // SCH sublayer
    LTE_LAYER2_SUBLAYER_NONE  // default, for developing purpose only
} LteLayer2SublayerType;

///////////////////////////////////////////////////////////////
// typedef struct
///////////////////////////////////////////////////////////////

// STRUCT:: MacLteMsgDestinationInfo
// DESCRIPTION:: MacLteMsgDestinationInfo
// **/
typedef struct struct_mac_lte_msg_destination_info_str
{
    LteRnti dstRnti;
} MacLteMsgDestinationInfo;

// /**
// STRUCT      :: Layer2DataLte
// DESCRIPTION :: layer2 data of LTE
// **/
typedef struct struct_layer2_data_lte
{
    MacData* myMacData;     // Mac Layer
    RandomSeed randSeed;    // Random Object
    LteRnti myRnti;         // Identifier

    // RRC Config, Layer3Filtering Module
    LteRrcConfig* rrcConfig;
    LteLayer3Filtering* layer3Filtering;

    // Sublayer, Scheduler, Layer3
    LteMacData* lteMacData; // layer2 layer lte struct pointer
    void* lteRlcVar; // RLC sublayer struct pointer
    LtePdcpData* ltePdcpData; // PDCP sublayer struct pointer
    class LteScheduler* lteScheduler; // SCH sublayer struct pointer
    Layer3DataLte* lteLayer3Data;

    // configurable parameter
    LteStationType stationType;
    int numSubframePerTti; // 1,2,5,10    //MAC-LTE-NUM-SF-PER-TTI

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    MapLteLayer2ValidationData* validationData;
#endif
#endif

}Layer2DataLte;

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
LteLayer2ValidationData* LteLayer2GetValidataionData(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti);
#endif
#endif


//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------
// /**
// FUNCTION::       LteLayer2GetLayer2DataLte
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Layer2 data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         Layer2DataLte
// **/
Layer2DataLte* LteLayer2GetLayer2DataLte(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetLtePdcpData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get PDCP data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LtePdcpData
// **/
LtePdcpData* LteLayer2GetLtePdcpData(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetLteRlcData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get RLC data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteRlcData
// **/
LteRlcData* LteLayer2GetLteRlcData(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetLteMacData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get MAC data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteMacData
// **/
LteMacData* LteLayer2GetLteMacData(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetLayer3Filtering
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Layer3 filtering data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteLayer3Filtering
// **/
LteLayer3Filtering* LteLayer2GetLayer3Filtering(
    Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetLayer3DataLte
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Layer3 data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         Layer3DataLte
// **/
Layer3DataLte* LteLayer2GetLayer3DataLte(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetScheduler
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Scheduler data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteScheduler
// **/
LteScheduler* LteLayer2GetScheduler(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetStationType
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get station type
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteStationType
// **/
LteStationType LteLayer2GetStationType(Node* node, int interfaceIndex);

// /**
// FUNCTION::       LteLayer2GetRnti
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get own RNTI
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteRnti
// **/
const LteRnti& LteLayer2GetRnti(Node* node, int interfaceIndex);

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION::       LteLayer2UpperLayerHasPacketToSend
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Called by upper layers to request
//                  sending SDU
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + lte:        Layer2 data
// RETURN::         NULL
// **/
void  LteLayer2UpperLayerHasPacketToSend(
        Node* node,
        int interfaceIndex,
        Layer2DataLte* lte);

// /**
// FUNCTION::       LteLayer2ReceivePacketFromPhy
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Called by lower layers to request
//                  receiving PDU
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + lte:        Layer2 data
// RETURN::         NULL
// **/
void LteLayer2ReceivePacketFromPhy(Node* node,
                                   int interfaceIndex,
                                   Layer2DataLte* lte,
                                   Message* msg);

// /**
// FUNCTION   :: LteLayer2PreInit
// LAYER      :: LTE LAYER2
// PURPOSE    :: Pre-initialize LTE layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void LteLayer2PreInit(
    Node* node, int interfaceIndex, const NodeInput* nodeInput);

// /**
// FUNCTION   :: LteLayer2Init
// LAYER      :: LTE LAYER2
// PURPOSE    :: Initialize LTE layer2 at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void LteLayer2Init(Node* node,
                    UInt32 interfaceIndex,
                    const NodeInput* nodeInput);

// /**
// FUNCTION   :: LteLayer2Finalize
// LAYER      :: LTE LAYER2
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void : NULL
// **/
void LteLayer2Finalize(Node* node, UInt32 interfaceIndex);

// /**
// FUNCTION   :: LteLayer2ProcessEvent
// LAYER      :: LTE LAYER2
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void LteLayer2ProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg);


// /**
// FUNCTION::       LteLayer2GetPdcpSnFromPdcpHeader
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        get SN from PDCP header
// PARAMETERS::
// + msg      : Message* : PDCP PDU
// RETURN:: const UInt16 : SN of this msg
// **/
const UInt16 LteLayer2GetPdcpSnFromPdcpHeader(
        const Message* msg);



#ifdef ADDON_DB
//--------------------------------------------------------------------------
// FUNCTION   :: LteLayer2GetPacketProperty
// LAYER      :: MAC
// PURPOSE    :: Called by the Mac Events Stats DB
// PARAMETERS :: Node* node
//                   Pointer to the node
//               Message* msg
//                   Pointer to the input message
//               Int32 interfaceIndex
//                   Interface index on which packet received
//               MacDBEventType eventType
//                   Receives the eventType
//               StatsDBMacEventParam& macParam
//                   Input StatDb event parameter
//               BOOL& isMyFrame
//                   Set TRUE if msg is unicast
// RETURN     :: NONE
//--------------------------------------------------------------------------
void LteLayer2GetPacketProperty(Node* node,
                             Message* msg,
                             Int32 interfaceIndex,
                             MacDBEventType eventType,
                             StatsDBMacEventParam& macParam,
                             BOOL& isMyFrame);
#endif

#endif /* _LAYER2_LTE_H_ */
