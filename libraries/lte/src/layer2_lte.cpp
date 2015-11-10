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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "lte_common.h"
#include "lte_rrc_config.h"

#include "layer2_lte.h"
#include "layer3_lte_filtering.h"
#include "layer2_lte_mac.h"
#include "phy.h"
#include "phy_lte.h"
#include "layer2_lte_sch_roundrobin.h"
#include "layer2_lte_sch_pf.h"
#include "layer2_lte_sch_ue_default.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

////////////////////////////////////////////////////////////////////////////
// Define
////////////////////////////////////////////////////////////////////////////

// parameter strings
#define MAC_LTE_STRING_STATION_TYPE       "MAC-LTE-STATION-TYPE"
#define MAC_LTE_STRING_NUM_SF_PER_TTI     "MAC-LTE-NUM-SF-PER-TTI"


#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
LteLayer2ValidationData* LteLayer2GetValidataionData(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti)
{
    LteLayer2ValidationData* valData = NULL;
    double startTime = lte::LteLog::getValidationStatOffset();
    if ((*(node->currentTime)/(double)SECOND) < startTime)
    {
        return valData;
    }
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "Layer2DataLte is not found.");

    MapLteLayer2ValidationData::iterator itr =
        layer2Data->validationData->end();

    itr = layer2Data->validationData->find(oppositeRnti);
    if (itr == layer2Data->validationData->end())
    {
        valData = new LteLayer2ValidationData();
        layer2Data->validationData->insert(
            PairLteLayer2ValidationData(oppositeRnti, valData));
    }
    else
    {
        valData = itr->second;
    }
    return valData;
}
#endif
#endif

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
#define LTE_LIB_HO_VALIDATION_CHECKING_ENB_INTERVAL (100*MILLI_SECOND)

static void LteLayer2ProcessCheckingEnbTimer(
    Node* node, UInt32 interfaceIndex, Message* msg);

// index is Partition ID
EnbPosMap enbPosMap[LTE_LIB_HO_VALIDATION_NUM_MAX_PARTITION];

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
Layer2DataLte* LteLayer2GetLayer2DataLte(Node* node, int interfaceIndex)
{
    return (Layer2DataLte*)(node->macData[interfaceIndex]->macVar);
}

// /**
// FUNCTION::       LteLayer2GetLtePdcpData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get PDCP data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LtePdcpData
// **/
LtePdcpData* LteLayer2GetLtePdcpData(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->ltePdcpData;
}

// /**
// FUNCTION::       LteLayer2GetLteRlcData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get RLC data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteRlcData
// **/
LteRlcData* LteLayer2GetLteRlcData(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return (LteRlcData*)layer2Data->lteRlcVar;
}

// /**
// FUNCTION::       LteLayer2GetLteMacData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get MAC data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteMacData
// **/
LteMacData* LteLayer2GetLteMacData(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->lteMacData;
}

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
    Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->layer3Filtering;
}

// /**
// FUNCTION::       LteLayer2GetLayer3DataLte
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Layer3 data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         Layer3DataLte
// **/
Layer3DataLte* LteLayer2GetLayer3DataLte(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->lteLayer3Data;
}

// /**
// FUNCTION::       LteLayer2GetScheduler
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get Scheduler data
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteScheduler
// **/
LteScheduler* LteLayer2GetScheduler(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->lteScheduler;
}

// /**
// FUNCTION::       LteLayer2GetStationType
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get station type
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteStationType
// **/
LteStationType LteLayer2GetStationType(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->stationType;
}

// /**
// FUNCTION::       LteLayer2GetRnti
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get own RNTI
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
// RETURN::         LteRnti
// **/
const LteRnti& LteLayer2GetRnti(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->myRnti;
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION::       LteLayer2UpperLayerHasPacketToSend
// LAYER::          LTE Layer2
// PURPOSE::        Called by upper layers to request
//                  Layer2 to send SDU
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + lte:        Layer2 data
// RETURN::         NULL
// **/
void  LteLayer2UpperLayerHasPacketToSend(
    Node* node,
    int interfaceIndex,
    Layer2DataLte* lte
)
{
    PdcpLteUpperLayerHasPacketToSend(node, interfaceIndex, lte->ltePdcpData);
}

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
                                   Message* msg)
{
    PhyLtePhyToMacInfo* ctrlInfo
        = (PhyLtePhyToMacInfo*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LtePhyLtePhyToMacInfo);

    if (ctrlInfo == NULL)
    {
        ERROR_ReportError(
            "MAC should receive DCI with PhyLtePhyToMacInfo from PHY.");
    }

    BOOL isError = ctrlInfo->isError;
    LteRnti srcRnti = ctrlInfo->srcRnti;

    // msg is freed in this function
    MacLteReceiveTransportBlockFromPhy(
        node, interfaceIndex, srcRnti, msg, isError);
}

static
void LteLayer2PreInitConfigurableParameters(
         Node* node,
         int interfaceIndex,
         const NodeInput* nodeInput)
{
    char retChar[MAX_STRING_LENGTH] = {0};

    BOOL wasFound = false;
    Layer2DataLte* lte =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);

    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            node, node->nodeId, interfaceIndex);
    IO_ReadString(node->nodeId,
                interfaceAddress,
                nodeInput,
                MAC_LTE_STRING_STATION_TYPE,
                &wasFound,
                retChar);

    if (wasFound)
    {
        if (strcmp(retChar, LTE_STRING_STATION_TYPE_ENB) == 0)
        {
            lte->stationType = LTE_STATION_TYPE_ENB;
        }

        else if (strcmp(retChar, LTE_STRING_STATION_TYPE_UE) == 0)
        {
            lte->stationType = LTE_STATION_TYPE_UE;
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "Layer2-LTE: Station Type as %s is not supported."
                    "Only eNB,UE are available."
                    "Please check %s.",
                     retChar,
                     MAC_LTE_STRING_STATION_TYPE);
            ERROR_ReportError(errorStr);
        }
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf, "Layer2-LTE: Station Type should be defined. "
            "Please check %s.", MAC_LTE_STRING_STATION_TYPE);
        ERROR_ReportError(errBuf);
    }

    if (lte->stationType == LTE_STATION_TYPE_ENB)
    {
        // Scheduler settings
        // MAC-LTE-ENB-SCHEDULER-TYPE
        IO_ReadString(node->nodeId,
                      interfaceAddress,
                      nodeInput,
                      MAC_LTE_STRING_ENB_SCHEDULER_TYPE,
                      &wasFound,
                      retChar);
        if (wasFound)
        {
            if (strcmp(retChar, "ROUND-ROBIN") == 0)
            {
                lte->lteScheduler =
                    new LteSchedulerENBRoundRobin(
                        node, interfaceIndex, nodeInput);
            }
            else if (strcmp(retChar, "PROPORTIONAL-FAIRNESS") == 0)
            {
                lte->lteScheduler =
                    new LteSchedulerENBPf(
                        node, interfaceIndex, nodeInput);
            }
            else
            {
                char errBuf[MAX_STRING_LENGTH];
                sprintf(errBuf,
                    "Layer2-LTE: eNB Scheduler type should be set "
                    "\"ROUND-ROBIN\" or \"PROPORTIONAL-FAIRNESS\". "
                    "Please check %s.", MAC_LTE_STRING_ENB_SCHEDULER_TYPE);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            char warnBuf[MAX_STRING_LENGTH];
            sprintf(warnBuf,
                    "Layer2-LTE: eNB Scheduler type should be set. "
                    "Change eNB Scheduler type should be set to "
                    "\"ROUND-ROBIN\".");
            ERROR_ReportWarning(warnBuf);
            lte->lteScheduler =
                new LteSchedulerENBRoundRobin(
                    node, interfaceIndex, nodeInput);
        }
    }
    // UE settings
    else
    {
        // Scheduler settings
        // MAC-LTE-UE-SCHEDULER-TYPE
        IO_ReadString(node->nodeId,
                      interfaceAddress,
                      nodeInput,
                      MAC_LTE_STRING_UE_SCHEDULER_TYPE,
                      &wasFound,
                      retChar);
        if (wasFound)
        {
            if (strcmp(retChar, "SIMPLE-SCHEDULER") == 0)
            {
                lte->lteScheduler =
                    new LteSchedulerUEDefault(
                        node, interfaceIndex, nodeInput);
            }
            else
            {
                char errBuf[MAX_STRING_LENGTH];
                sprintf(errBuf,
                    "Layer2-LTE: UE Scheduler type should be set "
                    "\"SIMPLE-SCHEDULER\". "
                    "Please check %s.", MAC_LTE_STRING_UE_SCHEDULER_TYPE);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            char warnBuf[MAX_STRING_LENGTH];
            sprintf(warnBuf,
                    "Layer2-LTE: UE Scheduler type should be set. "
                    "Change eNB Scheduler type should be set to "
                    "\"SIMPLE-SCHEDULER\".");
            ERROR_ReportWarning(warnBuf);
            lte->lteScheduler =
                new LteSchedulerUEDefault(
                    node, interfaceIndex, nodeInput);
        }
    }
}

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
void LteLayer2AddStatsForAllOfConnectedNodes(
        Node* node, const int interfaceIndex,
        MapLteLayer2ValidationData& enabledStats)
{
    // Layer2
    Layer2DataLte* lte = (Layer2DataLte*)
                          node->macData[interfaceIndex]->macVar;

    std::vector < lte::ConnectionInfo > ciList;

    lte::LteLog::getConnectionInfoConnectingWithMe(
                node->nodeId,
                lte->stationType == LTE_STATION_TYPE_ENB,
                ciList);

    for (int i = 0; i < ciList.size(); ++i){
        LteRnti oppositeRnti = LteRnti(ciList[i].nodeId,0);

        MapLteLayer2ValidationData::iterator itr;

        itr = lte->validationData->find(oppositeRnti);
        if (itr == lte->validationData->end())
        {
            LteLayer2ValidationData* valData = new LteLayer2ValidationData();
            lte->validationData->insert(
                PairLteLayer2ValidationData(oppositeRnti, valData));
        }
    }
}
#endif
#endif


static
void LteLayer2InitConfigurableParameters(
         Node* node,
         int interfaceIndex,
         const NodeInput* nodeInput)
{
}

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
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
    // Layer2
    Layer2DataLte* layer2Data = NULL;
    layer2Data = (Layer2DataLte*)MEM_malloc(sizeof(Layer2DataLte));
    ERROR_Assert(layer2Data != NULL,
        "LTE: Unable to allocate memory for lte data struct.");
    memset(layer2Data, 0, sizeof(Layer2DataLte));
    node->macData[interfaceIndex]->macVar = layer2Data;

    // RRC Config
    layer2Data->rrcConfig = new LteRrcConfig();
    // Layer3 Filtering
    layer2Data->layer3Filtering = new LteLayer3Filtering(layer2Data);

    // instans
    layer2Data->lteMacData = new LteMacData();
    layer2Data->lteRlcVar = new LteRlcData();
    layer2Data->ltePdcpData = new LtePdcpData();
    layer2Data->lteLayer3Data = new Layer3DataLte();

    layer2Data->numSubframePerTti = LTE_LAYER2_DEFAULT_NUM_SF_PER_TTI;

    LteLayer2PreInitConfigurableParameters(node, interfaceIndex, nodeInput);

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    layer2Data->validationData = new MapLteLayer2ValidationData();

    LteLayer2AddStatsForAllOfConnectedNodes(
                        node, interfaceIndex, *layer2Data->validationData);
#endif
#endif

}

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
                    const NodeInput* nodeInput)
{
    Layer2DataLte* lte = LteLayer2GetLayer2DataLte(node, interfaceIndex);

    lte->myMacData = node->macData[interfaceIndex];
    lte->myMacData->macVar = (void*) lte;
    LteLayer2InitConfigurableParameters(node,
                                        interfaceIndex,
                                        nodeInput);
    lte->myRnti.nodeId = node->nodeId;
    lte->myRnti.interfaceIndex = interfaceIndex;

    // Init Scheduler
    LteLayer2GetScheduler(node, interfaceIndex)->init();

    // Init MAC Layer
    MacLteInit(node, interfaceIndex, nodeInput);

    // Init RLC Layer
    LteRlcInit(node, interfaceIndex, nodeInput);

    // Init PDCP Layer
    PdcpLteInit(node, interfaceIndex, nodeInput);

    Layer3LteInitialize(node, interfaceIndex, nodeInput);

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    if (stationType == LTE_STATION_TYPE_ENB)
    {
        int partitionId = node->partitionId;
        EnbPosMap& curEnbPosMap = enbPosMap[partitionId];

        const cartesian_coordinates& myCartesian =
            node->mobilityData->current->position.cartesian;
        EnbPos enbPos(myCartesian.x, myCartesian.y, myCartesian.z);
        curEnbPosMap.insert(
            EnbPosMap::value_type(node->nodeId, enbPos));
    }
    else if (stationType == LTE_STATION_TYPE_UE)
    {
        Message* checkingEnbTimer = MESSAGE_Alloc(
            node, MAC_LAYER, MAC_PROTOCOL_LTE, MSG_LTE_CheckingEnbTimer);
        MESSAGE_SetInstanceId(checkingEnbTimer, interfaceIndex);
        MESSAGE_Send(node, checkingEnbTimer,
            LTE_LIB_HO_VALIDATION_CHECKING_ENB_INTERVAL);
    }
#endif
#endif
}


// /**
// FUNCTION   :: LteLayer2Finalize
// LAYER      :: layer2
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void LteLayer2Finalize(Node* node, UInt32 interfaceIndex)
{
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    MapLteLayer2ValidationData::iterator itr;
    for (itr  = layer2Data->validationData->begin();
        itr != layer2Data->validationData->end();
        ++itr)
    {
        LteLayer2ValidationData* valData = itr->second;
        NodeAddress oppositeNodeId = itr->first.nodeId;
        double valTime =
            node->getNodeTime() / (double)SECOND
            - lte::LteLog::getValidationStatOffset();

        // PDCP throughput
        lte::LteLog::InfoFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "PdcpThroughput,"
            "%d,"
            "%e",
            oppositeNodeId,
            valData->rlcBitsToPdcp / valTime);

        // Number of sending PDU
        lte::LteLog::InfoFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "PdcpNumSendPdu,"
            "%d,"
            "%d",
            oppositeNodeId,
            valData->rlcNumSduToTxQueue);

        // Number of receiving PDU
        lte::LteLog::InfoFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "PdcpNumRecvPdu,"
            "%d,"
            "%d",
            oppositeNodeId,
            valData->rlcNumSduToPdcp);
    }
#endif
#endif

    // RRC Config
    delete GetLteRrcConfig(node, interfaceIndex);
    // Layer3 Filtering
    delete LteLayer2GetLayer3Filtering(node, interfaceIndex);

    // MAC
    MacLteFinalize(node, interfaceIndex);

    // RLC
    LteRlcFinalize(node, interfaceIndex);

    // PDCP
    PdcpLteFinalize(node, interfaceIndex);

    // RRC
    Layer3LteFinalize(node, interfaceIndex);
}



// /**
// FUNCTION   :: LteLayer2ProcessEvent
// LAYER      :: layer2
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void LteLayer2ProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg)
{
    switch (msg->eventType)
    {
        // MAC Event
        case MSG_MAC_LTE_TtiTimerExpired:
        case MSG_MAC_LTE_RaBackoffWaitingTimerExpired:
        case MSG_MAC_LTE_RaGrantWaitingTimerExpired:
        {
            MacLteProcessEvent(node, interfaceIndex, msg);
            break;
        }
        // RLC Event
        case MSG_RLC_LTE_PollRetransmitTimerExpired:
        case MSG_RLC_LTE_ReorderingTimerExpired:
        case MSG_RLC_LTE_StatusProhibitTimerExpired:
        case MSG_RLC_LTE_ResetTimerExpired:
        {
            LteRlcProcessEvent(node, interfaceIndex, msg);
            break;
        }
        // PDCP Event
        case MSG_PDCP_LTE_DelayedPdcpSdu:
        case MSG_PDCP_LTE_DiscardTimerExpired:
        {
            PdcpLteProcessEvent(node, interfaceIndex, msg);
            break;
        }
        // RRC Event
        case MSG_RRC_LTE_WaitRrcConnectedTimerExpired:
        case MSG_RRC_LTE_WaitRrcConnectedReconfTimerExpired:
        case MSG_MAC_LTE_RELOCprep:
        case MSG_MAC_LTE_X2_RELOCoverall:
        case MSG_MAC_LTE_X2_WaitSnStatusTransfer:
        case MSG_MAC_LTE_WaitAttachUeByHo:
        case MSG_MAC_LTE_X2_WaitEndMarker:
        case MSG_MAC_LTE_S1_WaitPathSwitchReqAck:
#ifdef LTE_LIB_USE_POWER_TIMER
        case MSG_RRC_LTE_WaitPowerOnTimerExpired:
        case MSG_RRC_LTE_WaitPowerOffTimerExpired:
#endif
        {
            Layer3LteProcessEvent(node, interfaceIndex, msg);
            break;
        }
        case MSG_RRC_LTE_PeriodicalReportTimerExpired:
        {
            Layer3LteMeasProcessEvent(node, interfaceIndex, msg);
            break;
        }
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
        case MSG_LTE_CheckingEnbTimer:
        {
            LteLayer2ProcessCheckingEnbTimer(node, interfaceIndex, msg);
            MESSAGE_Free(node, msg);
            break;
        }
#endif
#endif

        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Event Type(%d) is not supported "
                "by LTE Layer2.", msg->eventType);
            ERROR_ReportError(errBuf);
            break;
        }
    }
}



// /**
// FUNCTION::       LteLayer2GetPdcpSnFromPdcpHeader
// LAYER::          LTE Layer2
// PURPOSE::        get PDCP SN from PDCP header
// PARAMETERS::
// + msg      : Message* : PDCP PDU
// RETURN:: const UInt16 : SN of this msg
// **/
const UInt16 LteLayer2GetPdcpSnFromPdcpHeader(
        const Message* pdcpPdu)
{
    ERROR_Assert(pdcpPdu, "pdcpPdu is NULL");
    const LtePdcpHeader* pdcpHeader
        = (LtePdcpHeader*) MESSAGE_ReturnPacket(pdcpPdu);
    const UInt16 sn = (pdcpHeader->octet[0] << 8) | pdcpHeader->octet[1];
    ERROR_Assert(sn <= PDCP_LTE_SEQUENCE_LIMIT,
            "PDCP SN is out of the range");

    return sn;
}

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
static void LteLayer2ProcessCheckingEnbTimer(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    BOOL isExistConInfo =
        layer3Data->connectionInfoMap.size() != 0
        ? TRUE : FALSE;
    if (isExistConInfo != FALSE)
    {
        LteRnti servingEnbRnti =
            layer3Data->connectionInfoMap.begin()->first;
        LteConnectionInfo& connectionInfo
            = layer3Data->connectionInfoMap.begin()->second;
        if (connectionInfo.state ==
                LAYER3_LTE_CONNECTION_CONNECTED)
        {
            EnbPos myPos(
                node->mobilityData->current->position.cartesian.x,
                node->mobilityData->current->position.cartesian.y,
                node->mobilityData->current->position.cartesian.z);
            double minD(10*1e+6);
            double srvD(0.0);
            int minP(-1);
            int minN(-1);
            for (int p = 0;
                p < LTE_LIB_HO_VALIDATION_NUM_MAX_PARTITION;
                p++)
            {
                EnbPosMap::iterator itr;
                for (itr = enbPosMap[p].begin();
                    itr != enbPosMap[p].end();
                    ++itr)
                {
                    double curD =
                        LteCalcDistance(myPos, (itr->second));
                    if (minD > curD)
                    {
                        minD = curD;
                        minP = p;
                        minN = itr->first;
                    }
                    if (itr->first == servingEnbRnti.nodeId)
                    {
                        srvD =
                            LteCalcDistance(myPos, (itr->second));
                    }
                }
            }
            ERROR_Assert(minP >= 0 && minN >=0, "");

            // Min Node RSRP
            double minRsrp = 0.0;
            BOOL isGotMinRsrp = FALSE;
            isGotMinRsrp =
                LteLayer2GetLayer3Filtering(node, interfaceIndex)
                    ->getAverage(minN, RSRP_FOR_HO, &minRsrp);
            minRsrp = isGotMinRsrp == FALSE ? LTE_INVALID_RSRP_dBm : minRsrp;
            // Srv Node RSRP
            double srvRsrp = 0.0;
            BOOL isGotSrvRsrp = FALSE;
            isGotSrvRsrp =
                LteLayer2GetLayer3Filtering(node, interfaceIndex)
                    ->get(servingEnbRnti, RSRP_FOR_HO, &srvRsrp);
            srvRsrp = isGotSrvRsrp == FALSE ? LTE_INVALID_RSRP_dBm : srvRsrp;

            UInt64 recvByte = connectionInfo.tempRecvByte;
            connectionInfo.tempRecvByte = 0;

            lte::LteLog::InfoFormat(
                node, interfaceIndex, LTE_STRING_LAYER_TYPE_RRC,
                "CheckingEnb,"LTE_STRING_FORMAT_RNTI","
                "%lf,"
                "MinNode,%d,%lf,%lf,"
                "SevNode,%d,%lf,%lf,"
                "RecvByte,%lld",
                ANY_NODEID, ANY_INTERFACE,
                fabs(minD-srvD),
                minN, minD, minRsrp,
                servingEnbRnti.nodeId, srvD, srvRsrp,
                recvByte);

        }
    }

    Message* checkingEnbTimer = MESSAGE_Alloc(
        node, MAC_LAYER, MAC_PROTOCOL_LTE,
        MSG_LTE_CheckingEnbTimer);
    MESSAGE_SetInstanceId(checkingEnbTimer, interfaceIndex);
    MESSAGE_Send(node, checkingEnbTimer,
        LTE_LIB_HO_VALIDATION_CHECKING_ENB_INTERVAL);
}
#endif
#endif

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
                                BOOL& isMyFrame)
{
    if (!msg)
    {
        // do nothing
        return;
    }

    MacHWAddress srcHWAddr;
    MacHWAddress destHWAddr;

    char srcAdd[32];
    char destAdd[32];
    memset(srcAdd, 0, 32);
    memset(destAdd, 0, 32);

    PhyLtePhyToMacInfo* ctrlInfo = (PhyLtePhyToMacInfo*)
                         MESSAGE_ReturnInfo(msg,
                                (UInt8)INFO_TYPE_LtePhyLtePhyToMacInfo);

    if (ctrlInfo)
    {
        MacSetDefaultHWAddress(ctrlInfo->srcRnti.nodeId,
                               &srcHWAddr,
                               ctrlInfo->srcRnti.interfaceIndex);
    }
    else
    {
        // assuming I am the source node
        MacSetDefaultHWAddress(node->nodeId,
                               &srcHWAddr,
                               interfaceIndex);
    }

    MacLteMsgDestinationInfo* dstInfo = (MacLteMsgDestinationInfo*)
                                MESSAGE_ReturnInfo(msg,
                                     (UInt8)INFO_TYPE_LteMacDestinationInfo);

    if (dstInfo)
    {
        MacSetDefaultHWAddress(dstInfo->dstRnti.nodeId,
                               &destHWAddr,
                               dstInfo->dstRnti.interfaceIndex);

        isMyFrame =(dstInfo->dstRnti == LteLayer2GetRnti(node,
                                                         interfaceIndex));
    }
    else
    {
        MacSetDefaultHWAddress(ANY_NODEID,
                               &destHWAddr,
                               ANY_INTERFACE);
    }

    // check for DATA or Control
    LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)MESSAGE_ReturnInfo(
                                  msg,
                                 (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);
    ERROR_Assert(info != NULL,
            "INFO_TYPE_LteStatsDbSduPduInfo not found in GetPacketProperty");

    if (info->isCtrlPacket || !info->dataSize)
    {
        macParam.SetFrameType("Control");
        macParam.SetHdrSize(0);
        macParam.SetMsgSize(info->ctrlSize);
    }
    else
    {
        macParam.SetFrameType("DATA");
        macParam.SetHdrSize(info->ctrlSize);
    }

    sprintf(srcAdd, "[%02X-%02X-%02X-%02X-%02X-%02X]",
            srcHWAddr.byte[0],
            srcHWAddr.byte[1],
            srcHWAddr.byte[2],
            srcHWAddr.byte[3],
            srcHWAddr.byte[4],
            srcHWAddr.byte[5]);

    sprintf(destAdd, "[%02X-%02X-%02X-%02X-%02X-%02X]",
            destHWAddr.byte[0],
            destHWAddr.byte[1],
            destHWAddr.byte[2],
            destHWAddr.byte[3],
            destHWAddr.byte[4],
            destHWAddr.byte[5]);

    macParam.SetDstAddr(destAdd);
    macParam.SetSrcAddr(srcAdd);
}
#endif
