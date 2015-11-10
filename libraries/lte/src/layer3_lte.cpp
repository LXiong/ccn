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
#include <float.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"

#include "phy_lte.h"
#include "phy_lte_establishment.h"
#include "layer2_lte_mac.h"
#include "layer2_lte_pdcp.h"
#include "layer2_lte_rlc.h"
#include "layer3_lte_filtering.h"
#include "layer3_lte.h"
#include "epc_lte.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"

static std::string GetLteLayer3StateName(Layer3LteState state)
{
    if (state == LAYER3_LTE_POWER_OFF)
        return std::string("RRC_POWER_OFF");
    else if (state == LAYER3_LTE_POWER_ON)
        return std::string("RRC_POWER_ON");
    else if (state == LAYER3_LTE_RRC_IDLE)
        return std::string("RRC_IDLE");
    else if (state == LAYER3_LTE_RRC_CONNECTED)
        return std::string("RRC_CONNECTED");
    else {
        ERROR_Assert(false, "");
        return "";
    }
}

#endif

#include "epc_lte_app.h"

#define DEBUG              0

//--------------------------------------------------------------------------
//  Static functions define
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: Layer3LteInitConfigurableParameters
// LAYER      :: Layer3
// PURPOSE    :: Initialize configurable parameters of Layer 3.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
static void Layer3LteInitConfigurableParameters(
    Node* node, int interfaceIndex, const NodeInput* nodeInput);

// /**
// FUNCTION   :: Layer3LtePrintStat
// LAYER      :: Layer3
// PURPOSE    :: Print Statictics.
// PARAMETERS ::
// + node             : Node*                    : Pointer to node.
// + interfaceIndex   : int                      : Interface index
// + stat             : const LteLayer3StatData& : Statistics Data.
// RETURN     :: void : NULL
// **/
static void Layer3LtePrintStat(
    Node* node, int interfaceIndex, const LteLayer3StatData& rrcStat);

// /**
// FUNCTION   :: Layer3LteCreateRadioBearer
// LAYER      :: Layer3
// PURPOSE    :: Create Radio Bearer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + rnti             : const LteRnti&    : RNTI
// + connectedInfo    : LteConnectedInfo* : Connected Info
// RETURN     :: void : NULL
// **/
static void Layer3LteCreateRadioBearer(
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LteConnectedInfo* connectedInfo,
    BOOL isTargetEnb);



// /**
// FUNCTION   :: Layer3LteDeleteRadioBearer
// LAYER      :: Layer3
// PURPOSE    :: Delete Radio Bearer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + rnti             : const LteRnti&    : RNTI
// + connectedInfo    : LteConnectedInfo* : Connected Info
// RETURN     :: void : NULL
// **/
static void Layer3LteDeleteRadioBearer(
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LteConnectedInfo* connectedInfo);

// /**
// FUNCTION   :: Layer3LteSetState
// LAYER      :: Layer3
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : IN : Node*          : Pointer to node.
// + interfaceIndex     : IN : int            : Interface index
// + state              : IN : Layer3LteState : State
// RETURN     :: void : NULL
// **/
static void Layer3LteSetState(
    Node* node, int interfaceIndex, Layer3LteState state);

//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: Layer3LteGetConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Get Connection Info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: LteConnectionInfo : Connection Info
// **/
LteConnectionInfo* Layer3LteGetConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    Layer3DataLte* layer3Data = LteLayer2GetLayer3DataLte(
        node, interfaceIndex);

    MapLteConnectionInfo::iterator it =
        layer3Data->connectionInfoMap.find(rnti);

    if (it != layer3Data->connectionInfoMap.end())
    {
        return &(it->second);
    }

    return NULL;
}

// /**
// FUNCTION   :: Layer3LteGetConnectionList
// LAYER      :: RRC
// PURPOSE    :: Get connection List
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + store          : ListLteRnti*   : result
// + state       : Layer3LteConnectionState : state
// RETURN     :: void : NULL
// **/
void Layer3LteGetConnectionList(
    Node* node, int interfaceIndex, ListLteRnti* store,
    Layer3LteConnectionState state)
{
    Layer3DataLte* layer3Data = LteLayer2GetLayer3DataLte(
        node, interfaceIndex);

    // clear
    store->clear();
    // collect
    for (MapLteConnectionInfo::iterator it =
            layer3Data->connectionInfoMap.begin();
        it != layer3Data->connectionInfoMap.end();
        ++it)
    {
        if (it->second.state == state)
        {
            store->push_back(it->first);
        }
    }
}

// /**
// FUNCTION   :: Layer3LteGetSchedulableListSortedByConnectedTime
// LAYER      :: RRC
// PURPOSE    :: Get connected List sorted by connected time
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + store          : ListLteRnti*   : result
// RETURN     :: void : NULL
// **/
void Layer3LteGetSchedulableListSortedByConnectedTime(
    Node* node, int interfaceIndex, ListLteRnti* store)
{
    Layer3DataLte* layer3Data = LteLayer2GetLayer3DataLte(
        node, interfaceIndex);


    // temp buffer to sort
    std::multimap<clocktype, LteRnti> tempBuffer;

    // collect
    for (MapLteConnectionInfo::iterator it = 
            layer3Data->connectionInfoMap.begin();
        it != layer3Data->connectionInfoMap.end();
        ++it)
    {
        if (it->second.state == LAYER3_LTE_CONNECTION_CONNECTED &&
            it->second.connectedInfo.isSchedulable)
        {
            tempBuffer.insert(std::multimap<clocktype, LteRnti>::value_type(
                it->second.connectedInfo.connectedTime, it->first));
        }
    }
    // clear
    store->clear();
    // sort
    for (std::multimap<clocktype, LteRnti>::iterator it =
        tempBuffer.begin(); it != tempBuffer.end(); ++it)
    {
        store->push_back(it->second);
    }

}

// /**
// FUNCTION   :: Layer3LteGetConnectionState
// LAYER      :: RRC
// PURPOSE    :: get connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: Layer3LteConnectionState : state
// **/
Layer3LteConnectionState Layer3LteGetConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);
    ERROR_Assert(connectionInfo, "tried to get connection state"
        " of unknown connectonInfo");

    return connectionInfo->state;
}

// /**
// FUNCTION   :: Layer3LteSetConnectionState
// LAYER      :: RRC
// PURPOSE    :: set connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteSetConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState state)
{
    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);
    ERROR_Assert(connectionInfo, "tried to set connection state"
        " of unknown connectonInfo");

    connectionInfo->state = state;
}

// /**
// FUNCTION   :: Layer3LteGetHandoverParticipator
// LAYER      :: RRC
// PURPOSE    :: get participators of H.O.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: HandoverParticipator : participators of H.O.
// **/
HandoverParticipator Layer3LteGetHandoverParticipator(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);
    ERROR_Assert(connectionInfo, "tried to get participators of H.O."
        " of unknown connectonInfo");

    return connectionInfo->hoParticipator;
}

// /**
// FUNCTION   :: Layer3LteSetHandoverParticipator
// LAYER      :: RRC
// PURPOSE    :: set participators of H.O.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteSetHandoverParticipator(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    const HandoverParticipator& hoParticipator)
{
    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);
    ERROR_Assert(connectionInfo, "tried to set participators of H.O."
        " of unknown connectonInfo");

    connectionInfo->hoParticipator = hoParticipator;
}

// /**
// FUNCTION   :: Layer3LteChangeConnectionState
// LAYER      :: RRC
// PURPOSE    :: change connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + newState       : Layer3LteConnectionState : new state
// RETURN     :: void : NULL
// **/
void Layer3LteChangeConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState newState,
    const HandoverParticipator& hoParticipator)
{
    Layer3LteConnectionState currentState =
        Layer3LteGetConnectionState(node, interfaceIndex, rnti);
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    
    ERROR_Assert(newState == LAYER3_LTE_CONNECTION_HANDOVER || 
        IS_INVALID_HANDOVER_PARTICIPATOR(hoParticipator),
        "don't specify hoParticipator");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_CONNECTION_INFO","
            LTE_STRING_FORMAT_RNTI","
            "[change state],state(before),%d,state(after),%d",
            rnti.nodeId, rnti.interfaceIndex,
            currentState, newState);
    }
#endif

    switch (currentState)
    {
    case LAYER3_LTE_CONNECTION_WAITING:
    {
        if (newState == LAYER3_LTE_CONNECTION_CONNECTED)
        {
            Layer3LteSetConnectionState(
                node, interfaceIndex, rnti, LAYER3_LTE_CONNECTION_CONNECTED);

            // create radio bearer
            LteConnectionInfo* connectionInfo =
                Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
            connectionInfo->connectedInfo.connectedTime = node->getNodeTime();
            connectionInfo->connectedInfo.bsrInfo.bufferSizeByte = 0;
            connectionInfo->connectedInfo.bsrInfo.bufferSizeLevel = 0;
            connectionInfo->connectedInfo.bsrInfo.ueRnti =
               stationType == LTE_STATION_TYPE_ENB ? rnti : LTE_INVALID_RNTI;
            connectionInfo->connectedInfo.isSchedulable = TRUE;
            Layer3LteCreateRadioBearer(node, interfaceIndex, rnti,
                &connectionInfo->connectedInfo, FALSE);

            // Notify layer2 scheduler the attach of node
            LteLayer2GetScheduler(node, interfaceIndex)->notifyAttach(rnti);

            // clear H.O participator
            Layer3LteSetHandoverParticipator(
                node, interfaceIndex, rnti, HandoverParticipator());

#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                "%s,%s,Connected=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_CONNECTION,
                LTE_STRING_COMMON_TYPE_ADD,
                rnti.nodeId, rnti.interfaceIndex);
#endif
        }
        else
        {
            ERROR_Assert(FALSE,
                "invalid connection state transition");
        }
        break;
    }
    case LAYER3_LTE_CONNECTION_CONNECTED:
    {
        if (newState == LAYER3_LTE_CONNECTION_HANDOVER)
        {
            Layer3LteSetConnectionState(
                node, interfaceIndex, rnti, LAYER3_LTE_CONNECTION_HANDOVER);
            // Notify layer2 scheduler the detach of node
            LteLayer2GetScheduler(node, interfaceIndex)->notifyDetach(rnti);
            // set H.O participator
            Layer3LteSetHandoverParticipator(
                node, interfaceIndex, rnti, hoParticipator);

            // unset isSchedulable flag
            {
                LteConnectionInfo* connectionInfo =
                    Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
                connectionInfo->connectedInfo.isSchedulable = FALSE;
            }

            if (stationType == LTE_STATION_TYPE_ENB)
            {
                ERROR_Assert(rnti == hoParticipator.ueRnti &&
                    LteRnti(node->nodeId, interfaceIndex) ==
                        hoParticipator.srcEnbRnti,
                    "unexpected error");
            }
            else if (stationType == LTE_STATION_TYPE_UE)
            {
                ERROR_Assert(rnti == hoParticipator.srcEnbRnti &&
                    LteRnti(node->nodeId, interfaceIndex) ==
                        hoParticipator.ueRnti,
                    "unexpected error");

                MapLteConnectionInfo* connInfoMap = 
                    &LteLayer2GetLayer3DataLte(node, interfaceIndex)
                        ->connectionInfoMap;
                LteConnectionInfo* connectionInfo =
                    Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
                std::pair<MapLteConnectionInfo::iterator, bool> insertResult
                    = connInfoMap->insert(MapLteConnectionInfo::value_type(
                    hoParticipator.tgtEnbRnti, *connectionInfo));
                // set RLC Entity address after connInfoMap is set to map.
                MapLteRadioBearer* radioBearers = &insertResult.first
                    ->second.connectedInfo.radioBearers;
                for (MapLteRadioBearer::iterator it = radioBearers->begin();
                    it != radioBearers->end(); ++it)
                {
                    it->second.rlcEntity.setRlcEntityToXmEntity();
                }

                connInfoMap->erase(rnti);
            }
            else
            {
                ERROR_Assert(FALSE, "unexpected error");
            }
        }
        else
        {
            ERROR_Assert(FALSE,
                "invalid connection state transition");
        }
        break;
    }
    case LAYER3_LTE_CONNECTION_HANDOVER:
    {
        if (newState == LAYER3_LTE_CONNECTION_CONNECTED)
        {
            Layer3LteSetConnectionState(
                node, interfaceIndex, rnti, LAYER3_LTE_CONNECTION_CONNECTED);

            // Notify layer2 scheduler the attach of node
            LteLayer2GetScheduler(node, interfaceIndex)->notifyAttach(rnti);

            // notify to PDCP
            LteConnectionInfo* connectionInfo =
                Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);

            // if eNB, isSchedulable is not set to TRUE until SN Status
            // received.
            if (LteLayer2GetStationType(node, interfaceIndex)
                == LTE_STATION_TYPE_UE)
            {
                connectionInfo->connectedInfo.isSchedulable = TRUE;
            }

            // set connected time
            connectionInfo->connectedInfo.connectedTime =
                node->getNodeTime();

            ERROR_Assert(connectionInfo, "unexpected error");
            for (MapLteRadioBearer::iterator it =
                connectionInfo->connectedInfo.radioBearers.begin();
                it != connectionInfo->connectedInfo.radioBearers.end();
                ++it)
            {
                PdcpLteNotifyRrcConnected(
                    node, interfaceIndex, rnti, it->first);
            }

            // clear H.O participator
            Layer3LteSetHandoverParticipator(
                node, interfaceIndex, rnti, HandoverParticipator());
        }
        else
        {
            ERROR_Assert(FALSE,
                "invalid connection state transition");
        }
        break;
    }
    default:
    {
        ERROR_Assert(FALSE, "unknown connection state");
    }
    }
}
// /**
// FUNCTION   :: Layer3LteGetConnectedInfo
// LAYER      :: RRC
// PURPOSE    :: Get Connected Information which is specified RNTI
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: LteConnectedInfo : Connected Information
// **/
LteConnectedInfo* Layer3LteGetConnectedInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);
    ERROR_Assert(connectionInfo, "tried to get ConnectedInfo"
        " of unknown connectonInfo");

    return &(connectionInfo->connectedInfo);
}

// /**
// FUNCTION   :: Layer3LteGetConnectedEnb
// LAYER      :: RRC
// PURPOSE    :: Get Connected eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : Connected eNB
// **/
const LteRnti Layer3LteGetConnectedEnb(
    Node* node, int interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    if (layer3Data->layer3LteState == LAYER3_LTE_RRC_CONNECTED)
    {
        ListLteRnti lteRntiList;
        Layer3LteGetConnectionList(node, interfaceIndex, &lteRntiList,
            LAYER3_LTE_CONNECTION_CONNECTED);
        int num = lteRntiList.size();
        if (num != 1)
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Do not have Connected List in RRC. Node=%d, I/F=%d",
                node->nodeId, interfaceIndex);
            ERROR_ReportError(errBuf);
        }
        return *(lteRntiList.begin());
    }
    else
    {
        ListLteRnti lteRntiList;
        Layer3LteGetConnectionList(node, interfaceIndex, &lteRntiList,
            LAYER3_LTE_CONNECTION_WAITING);
        int num = lteRntiList.size();
        if (num != 1)
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Do not have Waiting Connected List in RRC. Node=%d, I/F=%d",
                node->nodeId, interfaceIndex);
            ERROR_ReportError(errBuf);
        }
        return *(lteRntiList.begin());
    }
}

// /**
// FUNCTION   :: Layer3LteGetWaitingEnb
// LAYER      :: RRC
// PURPOSE    :: Get waiting eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : waiting eNB
// **/
const LteRnti Layer3LteGetWaitingEnb(Node* node, int interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);

    ListLteRnti waitingList;
    Layer3LteGetConnectionList(node, interfaceIndex, &waitingList,
        LAYER3_LTE_CONNECTION_WAITING);
    int num = waitingList.size();
    if (num != 1)
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf,
            "Do not have Waiting Connected List in RRC. Node=%d, I/F=%d",
            node->nodeId, interfaceIndex);
        ERROR_ReportError(errBuf);
    }
    return *(waitingList.begin());
}

// /**
// FUNCTION   :: Layer3LteGetRandomAccessingEnb
// LAYER      :: RRC
// PURPOSE    :: Get waiting eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : waiting eNB
// **/
const LteRnti Layer3LteGetRandomAccessingEnb(Node* node, int interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);

    ListLteRnti waitingList;
    Layer3LteGetConnectionList(node, interfaceIndex, &waitingList,
        LAYER3_LTE_CONNECTION_WAITING);
    ListLteRnti handoverList;
    Layer3LteGetConnectionList(node, interfaceIndex, &handoverList,
        LAYER3_LTE_CONNECTION_HANDOVER);

    if (waitingList.size() == 1)
    {
        return *(waitingList.begin());
    }
    else if (handoverList.size() == 1)
    {
        return *(handoverList.begin());
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf,
            "Do not have Waiting Connected List in RRC. Node=%d, I/F=%d",
            node->nodeId, interfaceIndex);
        ERROR_ReportError(errBuf);
    }

    return LTE_INVALID_RNTI;
}

// /**
// FUNCTION   :: Layer3LteGetTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: get timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: Message* : timer message
// **/
Message* Layer3LteGetTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    ERROR_Assert(connInfo, "tried to retrieve timer of unknown rnti");

    LteMapMessage::iterator itr =
        connInfo->mapLayer3Timer.find(eventType);
    if (itr != connInfo->mapLayer3Timer.end())
    {
        return itr->second;
    }

    return NULL;
}

// /**
// FUNCTION   :: Layer3LteAddTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: add timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// + delay           : clocktype : delay before timer expired
// + timerMsg        : Message*  : timer added
// RETURN     :: void : NULL
// **/
void Layer3LteAddTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType,
    Message* timerMsg)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    ERROR_Assert(connInfo, "tried to add timer to unknown rnti");
    ERROR_Assert(connInfo->mapLayer3Timer.find(eventType)
        == connInfo->mapLayer3Timer.end(),
        "tried to cancel timer which already exists");

    connInfo->mapLayer3Timer[eventType] = timerMsg;
}

// /**
// FUNCTION   :: Layer3LteDeleteTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: delete timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: void : NULL
// **/
void Layer3LteDeleteTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    ERROR_Assert(connInfo, "tried to delete timer of unknown rnti");
    ERROR_Assert(connInfo->mapLayer3Timer.find(eventType)
        != connInfo->mapLayer3Timer.end(),
        "tried to delete timer of unknown eventType");

    connInfo->mapLayer3Timer.erase(eventType);
}

// /**
// FUNCTION   :: Layer3LteSetTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Set timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// + delay           : clocktype : delay before timer expired
// RETURN     :: void : NULL
// **/
void Layer3LteSetTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType,
    clocktype delay)
{
    Message* timerMsg = MESSAGE_Alloc(
            node, MAC_LAYER, MAC_PROTOCOL_LTE, eventType);
    // set rnti to default info
    *(LteRnti*)MESSAGE_InfoAlloc(node, timerMsg, sizeof(LteRnti)) = rnti;
    MESSAGE_SetInstanceId(timerMsg, interfaceIndex);
    MESSAGE_Send(node, timerMsg, delay);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_RRC_TIMER","
            LTE_STRING_FORMAT_RNTI","
            "[set],eventType,%d,delay,%lu",
            rnti.nodeId, rnti.interfaceIndex,
            eventType, delay);
    }
#endif

    Layer3LteAddTimerForRrc(node, interfaceIndex, rnti, eventType, timerMsg);
}

// /**
// FUNCTION   :: Layer3LteCancelTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Cancel timer
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: void : NULL
// **/
void Layer3LteCancelTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    ERROR_Assert(connInfo, "tried to cancel timer of unknown rnti");
    LteMapMessage::iterator it_tiemr =
        connInfo->mapLayer3Timer.find(eventType);
    ERROR_Assert(it_tiemr != connInfo->mapLayer3Timer.end(),
        "tried to cancel timer of unknown eventType");

    MESSAGE_CancelSelfMsg(node, it_tiemr->second);
    it_tiemr->second = NULL;

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_RRC_TIMER","
            LTE_STRING_FORMAT_RNTI","
            "[cancel],eventType,%d",
            rnti.nodeId, rnti.interfaceIndex,
            eventType);
    }
#endif

    Layer3LteDeleteTimerForRrc(node, interfaceIndex, rnti, eventType);
}

// /**
// FUNCTION   :: Layer3LteCancelAllTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Cancel all the timers
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteCancelAllTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    ERROR_Assert(connInfo, "tried to cancel timer of unknown rnti");
    LteMapMessage* mapLayer3Timer = &(connInfo->mapLayer3Timer);
    LteMapMessage::iterator timerItr = mapLayer3Timer->begin();
    while (!mapLayer3Timer->empty())
    {
        Layer3LteCancelTimerForRrc(
            node, interfaceIndex, rnti, timerItr->first);
        timerItr = mapLayer3Timer->begin();
    }
}

// /**
// FUNCTION   :: Layer3LteCheckTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Check timer
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: TRUE:Running FALSE:Not Running
// **/
BOOL Layer3LteCheckTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType)
{
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
    if (connInfo == NULL)
    {
        return FALSE;
    }
    LteMapMessage::iterator it_tiemr =
        connInfo->mapLayer3Timer.find(eventType);
    if (it_tiemr == connInfo->mapLayer3Timer.end())
    {
        return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteProcessEvent
// LAYER      :: RRC
// PURPOSE    :: Process Event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg)
{
    // get rnti from default info
    LteRnti* tempRnti = (LteRnti*)MESSAGE_ReturnInfo(msg);
    ERROR_Assert(tempRnti, "timer msg should contain rnti in default info");
    LteRnti rnti = *tempRnti;   // copy before delete
    int eventType = msg->eventType;

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_RRC_TIMER","
            LTE_STRING_FORMAT_RNTI","
            "[expire],eventType,%d",
            rnti.nodeId, rnti.interfaceIndex,
            eventType);
    }
#endif
    Layer3LteDeleteTimerForRrc(node, interfaceIndex, rnti, eventType);

    switch (eventType)
    {
        // RRC Event
        case MSG_RRC_LTE_WaitRrcConnectedTimerExpired:
        {
            Layer3LteProcessRrcConnectedTimerExpired(
                node, interfaceIndex, msg);
            break;
        }
        case MSG_RRC_LTE_WaitRrcConnectedReconfTimerExpired:
        {
            Layer3LteProcessRrcConnectedReconfTimerExpired(
                node, interfaceIndex, msg);
            break;
        }
        case MSG_MAC_LTE_RELOCprep:
        {
            // this timer must not expire
            char errorStr[MAX_STRING_LENGTH];
            const LteRnti& myRnti = LteLayer2GetRnti(node, interfaceIndex);
            sprintf(errorStr,
                "%f,myRnti:"LTE_STRING_FORMAT_RNTI
                ",oppositeRnti:"LTE_STRING_FORMAT_RNTI"\n"
                "failed to Tx/Rx Handover Request/Handover Request Ack",
                (double)node->getNodeTime()/(double)SECOND,
                rnti.nodeId, rnti.interfaceIndex,
                myRnti.nodeId, myRnti.interfaceIndex);
            ERROR_Assert(FALSE, errorStr);
            break;
        }
        case MSG_MAC_LTE_X2_RELOCoverall:
        {
            // retrieve handover info
            LteConnectionInfo* handoverInfo = Layer3LteGetHandoverInfo(
                node, interfaceIndex, rnti);
            ERROR_Assert(handoverInfo, "unexpected error");
            // Release Ue Context by timer expire
            Layer3LteReleaseUeContext(node, interfaceIndex,
                handoverInfo->hoParticipator);
            break;
        }
        case MSG_MAC_LTE_X2_WaitSnStatusTransfer:
        {
            // this timer must not expire
            char errorStr[MAX_STRING_LENGTH];
            const LteRnti& myRnti = LteLayer2GetRnti(node, interfaceIndex);
            sprintf(errorStr,
                "%f,myRnti:"LTE_STRING_FORMAT_RNTI
                ",oppositeRnti:"LTE_STRING_FORMAT_RNTI"\n"
                "failed to Tx/Rx SnStatusTransfer",
                (double)node->getNodeTime()/(double)SECOND,
                rnti.nodeId, rnti.interfaceIndex,
                myRnti.nodeId, myRnti.interfaceIndex);
            ERROR_Assert(FALSE, errorStr);
            break;
        }
        case MSG_MAC_LTE_WaitAttachUeByHo:
        {
            // detach UE
            Layer3LteDetachUE(node, interfaceIndex, rnti);
            break;
        }
        case MSG_MAC_LTE_X2_WaitEndMarker:
        {
            // this timer must not expire
            char errorStr[MAX_STRING_LENGTH];
            const LteRnti& myRnti = LteLayer2GetRnti(node, interfaceIndex);
            sprintf(errorStr,
                "%f,myRnti:"LTE_STRING_FORMAT_RNTI
                ",oppositeRnti:"LTE_STRING_FORMAT_RNTI"\n"
                "failed to Tx/Rx EndMarker",
                (double)node->getNodeTime()/(double)SECOND,
                rnti.nodeId, rnti.interfaceIndex,
                myRnti.nodeId, myRnti.interfaceIndex);
            ERROR_Assert(FALSE, errorStr);
            break;
        }
        case MSG_MAC_LTE_S1_WaitPathSwitchReqAck:
        {
            // this timer must not expire
            char errorStr[MAX_STRING_LENGTH];
            const LteRnti& myRnti = LteLayer2GetRnti(node, interfaceIndex);
            sprintf(errorStr,
                "%f,myRnti:"LTE_STRING_FORMAT_RNTI
                ",oppositeRnti:"LTE_STRING_FORMAT_RNTI"\n"
                "failed to Tx/Rx PathSwitchReqAck",
                (double)node->getNodeTime()/(double)SECOND,
                rnti.nodeId, rnti.interfaceIndex,
                myRnti.nodeId, myRnti.interfaceIndex);
            ERROR_Assert(FALSE, errorStr);
            break;
        }
#ifdef LTE_LIB_USE_POWER_TIMER
        case MSG_RRC_LTE_WaitPowerOnTimerExpired:
        {
            Layer3LtePowerOn(node, interfaceIndex);
            break;
        }
        case MSG_RRC_LTE_WaitPowerOffTimerExpired:
        {
            Layer3LtePowerOff(node, interfaceIndex);
            break;
        }
#endif // LTE_LIB_USE_POWER_TIMER
        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Event Type(%d) is not supported "
                "by LTE RRC.", msg->eventType);
            ERROR_ReportError(errBuf);
            break;
        }
    }
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: Layer3LteAddConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Add Connected RNTI to List & Create Radio Bearer
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + state          : Layer3LteConnectionState : initial state
// RETURN     :: BOOL : TRUE:exist FALSE:not exist
// **/
void Layer3LteAddConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState state)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    MapLteConnectionInfo* connInfoMap = &layer3Data->connectionInfoMap;

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_CONNECTION_INFO","
            LTE_STRING_FORMAT_RNTI","
            "[add],state,%d",
            rnti.nodeId, rnti.interfaceIndex,
            state);
    }
#endif

    switch (state)
    {
    case LAYER3_LTE_CONNECTION_WAITING:
    {
        // new entry
        LteConnectionInfo newInfo(LAYER3_LTE_CONNECTION_WAITING);
        // register
        connInfoMap->insert(MapLteConnectionInfo::value_type(
            rnti, newInfo));
        break;
    }
    case LAYER3_LTE_CONNECTION_CONNECTED:
    {
        ERROR_Assert(FALSE, "cannot call Layer3LteAddConnectionInfo"
            "on LAYER3_LTE_CONNECTION_CONNECTED");
        break;
    }
    case LAYER3_LTE_CONNECTION_HANDOVER:
    {
        // station type guard
        LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
            LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

        // new entry
        LteConnectionInfo newInfo(LAYER3_LTE_CONNECTION_HANDOVER);
        // register
        std::pair<MapLteConnectionInfo::iterator, bool> insertResult =
            connInfoMap->insert(MapLteConnectionInfo::value_type(
                rnti, newInfo));

        // set connectedInfo
        LteConnectedInfo* connectedInfo =
            &(insertResult.first->second.connectedInfo);
        connectedInfo->connectedTime = 0;           // as invalid value
        connectedInfo->bsrInfo.bufferSizeByte = 0;
        connectedInfo->bsrInfo.bufferSizeLevel = 0;
        connectedInfo->bsrInfo.ueRnti = rnti;
        connectedInfo->isSchedulable = FALSE;
        Layer3LteCreateRadioBearer(node, interfaceIndex, rnti,
            connectedInfo, TRUE);

        break;
    }
    default:
    {
        ERROR_Assert(FALSE, "unexpected error");
        break;
    }
    }
}

// /**
// FUNCTION   :: Layer3LteRemoveConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Remove Connected info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: BOOL : TRUE:exist FALSE:not exist
// **/
BOOL Layer3LteRemoveConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    BOOL isSuccess =FALSE;

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    MapLteConnectionInfo* connInfoMap = &layer3Data->connectionInfoMap;
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);

    if (connInfo)
    {
#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LTE_STRING_CATEGORY_TYPE_CONNECTION_INFO","
                LTE_STRING_FORMAT_RNTI","
                "[remove],state,%d",
                rnti.nodeId, rnti.interfaceIndex,
                connInfo->state);
        }
#endif

        // cancel all timer
        Layer3LteCancelAllTimerForRrc(node, interfaceIndex, rnti);

        switch (Layer3LteGetConnectionState(node, interfaceIndex, rnti))
        {
        case LAYER3_LTE_CONNECTION_WAITING:
        {
            break;
        }
        case LAYER3_LTE_CONNECTION_CONNECTED:
        {
            // remove from ConnectedInfoMap & delete radio bearer
            Layer3LteDeleteRadioBearer(
                node, interfaceIndex, rnti, &connInfo->connectedInfo);

            // Notify layer2 scheduler the detach of node
            LteLayer2GetScheduler(node, interfaceIndex)->notifyDetach(rnti);
            
            break;
        }
        case LAYER3_LTE_CONNECTION_HANDOVER:
        {
            // remove from ConnectedInfoMap & delete radio bearer
            Layer3LteDeleteRadioBearer(
                node, interfaceIndex, rnti, &connInfo->connectedInfo);

            break;
        }
        default:
            ERROR_Assert(FALSE, "unexpected error");
        }

        // erase
        connInfoMap->erase(rnti);

        isSuccess = TRUE;
    }
    else
    {
        // do nothing
    }

    return isSuccess;

}

// /**
// FUNCTION   :: Layer3LtePowerOn
// LAYER      :: Layer3
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LtePowerOn(Node* node, int interfaceIndex)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

    if (layer3Data->layer3LteState == LAYER3_LTE_POWER_ON)
    {
        return;
    }

    layer3Data->establishmentStatData.numPowerOn++;

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s",
        LTE_STRING_CATEGORY_TYPE_POWER_ON);
#endif

    switch (stationType)
    {
        case LTE_STATION_TYPE_ENB:
        {
            Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_POWER_ON);
            break;
        }
        case LTE_STATION_TYPE_UE:
        {
            Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_RRC_IDLE);
            break;
        }
        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "LTE do not support LteStationType(%d)\n"
                "Please check LteStationType.\n"
                , stationType);
            ERROR_ReportError(errBuf);
            break;
        }
    }

    // Notify Power ON to every layer
    PdcpLteNotifyPowerOn(node, interfaceIndex);
    RlcLteNotifyPowerOn(node, interfaceIndex);
    MacLteNotifyPowerOn(node, interfaceIndex);
    PhyLteNotifyPowerOn(node, phyIndex);

    if (stationType == LTE_STATION_TYPE_UE)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_PROCESS_TYPE_START);
#endif
        layer3Data->establishmentStatData.lastStartTime = node->getNodeTime();
        PhyLteStartCellSearch(node, phyIndex);
    }
}

// /**
// FUNCTION   :: Layer3LtePowerOff
// LAYER      :: Layer3
// PURPOSE    :: Power OFF
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LtePowerOff(Node* node, int interfaceIndex)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s",
        LTE_STRING_CATEGORY_TYPE_POWER_OFF);
#endif
    if (layer3Data->layer3LteState == LAYER3_LTE_POWER_OFF)
    {
        return;
    }

    layer3Data->establishmentStatData.numPowerOff++;
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);

    // Notify Power OFF to every layer
    PhyLteNotifyPowerOff(node, phyIndex);
    MacLteNotifyPowerOff(node, interfaceIndex);
    RlcLteNotifyPowerOff(node, interfaceIndex);
    PdcpLteNotifyPowerOff(node, interfaceIndex);

    // clear Layer3Filtering
    LteLayer3Filtering* layer3Filtering =
        LteLayer2GetLayer3Filtering(node, interfaceIndex);
    layer3Filtering->remove();
    // claer LteRrcConfig only UE
    if (stationType == LTE_STATION_TYPE_UE)
    {
        LteRrcConfig* rrcConfig =
            GetLteRrcConfig(node, interfaceIndex);
        *rrcConfig = LteRrcConfig();
    }

    // NOTE: data for statistics and data read by config are not clear
    // Layer3DataLte::statData
    // Layer3DataLte::establishmentStatData
    // Layer3DataLte::waitRrcConnectedTime

    // Clear connected information
    MapLteConnectionInfo::iterator it_connInfo =
        layer3Data->connectionInfoMap.begin();
    while (it_connInfo != layer3Data->connectionInfoMap.end())
    {
        Layer3LteRemoveConnectionInfo(node, interfaceIndex,
            it_connInfo->first);
        it_connInfo = layer3Data->connectionInfoMap.begin();
    }
    ERROR_Assert(layer3Data->connectionInfoMap.empty(), "unexpected error");

    // change state
    Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_POWER_OFF);
}



// /**
// FUNCTION   :: Layer3LteRestart
// LAYER      :: RRC
// PURPOSE    :: Restart to search cells for re-attach
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void 
Layer3LteRestart(Node* node, int interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_LOST_DETECTION","
            LTE_STRING_FORMAT_RNTI","
            "[restart]",
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex);
    }
#endif

    // stop intra-freq measurement
    PhyLteIFPHStopMeasIntraFreq(node, phyIndex);
    // stop inter-freq measurement
    PhyLteIFPHStopMeasInterFreq(node, phyIndex);
    // reset
    Layer3LteMeasResetMeasurementValues(node, interfaceIndex);

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    Layer3LteState currentState = layer3Data->layer3LteState;
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);

    ERROR_Assert(currentState != LAYER3_LTE_POWER_OFF,
        "cannot restart during power off");

    // clear info on each layer
    PhyLteClearInfo(node, phyIndex, LTE_INVALID_RNTI);
    MacLteClearInfo(node, interfaceIndex, LTE_INVALID_RNTI);

    // claer LteRrcConfig only UE
    if (stationType == LTE_STATION_TYPE_UE)
    {
        LteRrcConfig* rrcConfig =
            GetLteRrcConfig(node, interfaceIndex);
        *rrcConfig = LteRrcConfig();
    }

    // Clear connected information (bearer info will be cleared)
    MapLteConnectionInfo::iterator it_connInfo =
        layer3Data->connectionInfoMap.begin();
    while (it_connInfo != layer3Data->connectionInfoMap.end())
    {
        Layer3LteRemoveConnectionInfo(node, interfaceIndex,
            it_connInfo->first);
        it_connInfo = layer3Data->connectionInfoMap.begin();
    }
    ERROR_Assert(layer3Data->connectionInfoMap.empty(), "unexpected error");

    // Change RRC state
    Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_RRC_IDLE);

    // Notify layer2 scheduler power on
    LteLayer2GetScheduler(node, interfaceIndex)->notifyPowerOn();
    // reset phy to poweron state
    PhyLteResetToPowerOnState(node, phyIndex);

    layer3Data->establishmentStatData.lastStartTime = node->getNodeTime();
    PhyLteStartCellSearch(node, phyIndex);
}



////////////////////////////////////////////////////////////////////////////
// eNB - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: layer3LteInitialize
// LAYER      :: RRC
// PURPOSE    :: Init RRC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + nodeInput      : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void Layer3LteInitialize(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    // init parameter
    layer3Data->layer3LteState = LAYER3_LTE_POWER_OFF;
    layer3Data->connectionInfoMap.clear();
    layer3Data->waitRrcConnectedTime = 0;
    layer3Data->waitRrcConnectedReconfTime = 0;
    layer3Data->ignoreHoDecisionTime = 0;

    // read config
    Layer3LteInitConfigurableParameters(node, interfaceIndex, nodeInput);

    // init statistics
    layer3Data->statData.averageRetryRrcConnectionEstablishment = 0;
    layer3Data->statData.averageTimeOfRrcConnectionEstablishment = 0;
    layer3Data->statData.numRrcConnectionEstablishment = 0;
    layer3Data->establishmentStatData.lastStartTime = 0;
    layer3Data->establishmentStatData.numPowerOn = 0;
    layer3Data->establishmentStatData.numPowerOff = 0;

    // Power ON
    Layer3LtePowerOn(node, interfaceIndex);

#ifdef LTE_LIB_USE_POWER_TIMER
    {
        int num = 0;
        BOOL wasFound = FALSE;
        clocktype retTime = 0;
        NodeAddress interfaceAddress =
            MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                node, node->nodeId, interfaceIndex);
        IO_ReadInt(node->nodeId,
            interfaceAddress,
            nodeInput,
            RRC_LTE_STRING_NUM_POWER_ON,
            &wasFound,
            &num);
        if (wasFound == TRUE)
        {
            if (num > 0)
            {
                for (int i = 0; i < num; i++)
                {
                    IO_ReadTimeInstance(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_POWER_ON_TIME,
                        i,
                        (i == 0),
                        &wasFound,
                        &retTime);
                    if (wasFound == TRUE)
                    {
                        Message* onTimer = MESSAGE_Alloc(
                            node, MAC_LAYER, MAC_PROTOCOL_LTE,
                            MSG_RRC_LTE_WaitPowerOnTimerExpired);
                        MESSAGE_SetInstanceId(onTimer, interfaceIndex);
                        MESSAGE_Send(node, onTimer, retTime);
                    }
                }
            }
        }

        IO_ReadInt(node->nodeId,
            interfaceAddress,
            nodeInput,
            RRC_LTE_STRING_NUM_POWER_OFF,
            &wasFound,
            &num);
        if (wasFound == TRUE)
        {
            if (num > 0)
            {
                for (int i = 0; i < num; i++)
                {
                    IO_ReadTimeInstance(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_POWER_OFF_TIME,
                        i,
                        (i == 0),
                        &wasFound,
                        &retTime);
                    if (wasFound == TRUE)
                    {
                        Message* offTimer = MESSAGE_Alloc(
                            node, MAC_LAYER, MAC_PROTOCOL_LTE,
                            MSG_RRC_LTE_WaitPowerOffTimerExpired);
                        MESSAGE_SetInstanceId(offTimer, interfaceIndex);
                        MESSAGE_Send(node, offTimer, retTime);
                    }
                }
            }
        }
    }
#endif // LTE_LIB_USE_POWER_TIMER
    // setup measurement configration
    Layer3LteSetupMeasConfig(node, interfaceIndex, nodeInput);
}

// /**
// FUNCTION   :: layer3LteFinalize
// LAYER      :: RRC
// PURPOSE    :: Finalize RRC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void Layer3LteFinalize(
    Node* node, int interfaceIndex)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    Layer3LtePrintStat(node, interfaceIndex, layer3Data->statData);

    layer3Data->connectionInfoMap.clear();
    delete layer3Data;
}

// /**
// FUNCTION   :: Layer3LteNotifyBsrFromMac
// LAYER      :: RRC
// PURPOSE    :: SAP for BSR Notification from eNB's MAC LAYER
// PARAMETERS ::
// + node           : Node*      : Pointer to node.
// + interfaceIndex : int        : Interface Index
// + bsrInfo        : LteBsrInfo : BSR Info Structure
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyBsrFromMac(
    Node* node, int interfaceIndex, const LteBsrInfo& bsrInfo)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    const LteRnti& ueRnti = bsrInfo.ueRnti;
    LteConnectionInfo* connectionInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, ueRnti);

    if (connectionInfo != NULL)
    {
        connectionInfo->connectedInfo.bsrInfo = bsrInfo;
    }
}

////////////////////////////////////////////////////////////////////////////
// UE - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteProcessRrcConnectedTimerExpired
// LAYER      :: RRC
// PURPOSE    :: Process RRC_CONNECTED Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteProcessRrcConnectedTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    // Get eNB's RNTI
    const LteRnti& enbRnti =
        Layer3LteGetRandomAccessingEnb(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,RRC_CONNECTED,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
        LTE_STRING_STATION_TYPE_ENB,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    // Add to ConnectedList
    Layer3LteChangeConnectionState(node, interfaceIndex, enbRnti,
        LAYER3_LTE_CONNECTION_CONNECTED);

    // Notify to MAC Layer
    MacLteNotifyRrcConnected(node, interfaceIndex);
    // Notify to PHY Layer
    PhyLteRrcConnectedNotification(node, phyIndex);

    layer3Data->statData.numRrcConnectionEstablishment++;
    clocktype establishmentTime =
        node->getNodeTime() - layer3Data->establishmentStatData.lastStartTime;
    layer3Data->statData.averageTimeOfRrcConnectionEstablishment +=
        establishmentTime;

    // RRC_IDLE -> RRC_CONNECTED
    Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_RRC_CONNECTED);

    // add route
    Layer3LteAddRoute(node, interfaceIndex, enbRnti);

    // setup measurement event condition observing information
    Layer3LteSetupMeasEventConditionTable(node, interfaceIndex);
    // init measurement
    Layer3LteInitMeasurement(node, interfaceIndex);
}



// /**
// FUNCTION   :: Layer3LteProcessRrcConnectedReconfTimerExpired
// LAYER      :: RRC
// PURPOSE    :: Process RRC_CONNECTED_RECONF Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteProcessRrcConnectedReconfTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    // Get eNB's RNTI
    const LteRnti& enbRnti =
        Layer3LteGetRandomAccessingEnb(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,RRC_CONNECTED_RECONF,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
        LTE_STRING_STATION_TYPE_ENB,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    // restore to connectedInfo
    Layer3LteChangeConnectionState(node, interfaceIndex, enbRnti,
        LAYER3_LTE_CONNECTION_CONNECTED);

    // Notify to MAC Layer
    MacLteNotifyRrcConnected(node, interfaceIndex, TRUE);
    // Notify to PHY Layer
    PhyLteRrcConnectedNotification(node, phyIndex, TRUE);

    // stat
    layer3Data->statData.numRrcConnectionEstablishment++;
    clocktype establishmentTime =
        node->getNodeTime() - layer3Data->establishmentStatData.lastStartTime;
    layer3Data->statData.averageTimeOfRrcConnectionEstablishment +=
        establishmentTime;

    // RRC_IDLE -> RRC_CONNECTED
    Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_RRC_CONNECTED);

    //
    // add-route process has already done prevent from dropping UL packets.
    //
    //// add route
    //Layer3LteAddRoute(node, interfaceIndex, enbRnti);

    // setup measurement event condition observing information
    Layer3LteSetupMeasEventConditionTable(node, interfaceIndex);
    // init measurement
    Layer3LteInitMeasurement(node, interfaceIndex);
}



// /**
// FUNCTION   :: Layer3LteAddRoute
// LAYER      :: Layer3
// PURPOSE    :: Add Route
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + oppositeRnti     : const LteRnti&    : Oppsite RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteAddRoute(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti)
{
    NodeAddress destAddr;
    NodeAddress destMask;
    NodeAddress nextHop;
    int outgoingInterfaceIndex;

    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    if (stationType == LTE_STATION_TYPE_ENB)
    {
        destAddr = 
            MAPPING_GetInterfaceAddressForInterface(
            node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        //destMask = MAPPING_GetSubnetMaskForInterface(
        //    node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        destMask = ANY_ADDRESS;
        nextHop = 
            MAPPING_GetInterfaceAddressForInterface(
            node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        outgoingInterfaceIndex = interfaceIndex;
    }
    else
    {
        destAddr = 0;   // default route
        destMask = 0;   // default route
        nextHop = 
            MAPPING_GetInterfaceAddressForInterface(
            node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        outgoingInterfaceIndex = interfaceIndex;
    }
    LteAddRouteToForwardingTable(node, destAddr, destMask,
        nextHop, outgoingInterfaceIndex);
}



// /**
// FUNCTION   :: Layer3LteDeleteRoute
// LAYER      :: Layer3
// PURPOSE    :: Delete Route
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + oppositeRnti     : const LteRnti&    : Oppsite RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteDeleteRoute(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti)
{
    NodeAddress destAddr;
    NodeAddress destMask;

    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    if (stationType == LTE_STATION_TYPE_ENB)
    {
        destAddr = 
            MAPPING_GetInterfaceAddressForInterface(
            node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        //destMask = MAPPING_GetSubnetMaskForInterface(
        //    node, oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
        destMask = ANY_ADDRESS;
    }
    else
    {
        destAddr = 0;   // default route
        destMask = 0;   // default route
    }
    LteDeleteRouteFromForwardingTable(node, destAddr, destMask);
}

// /**
// FUNCTION   :: Layer3LteDetachUE
// LAYER      :: RRC
// PURPOSE    :: Detach UE
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : const LteRnti& oppositeRnti : RNTI of detached UE
// RETURN     :: void : NULL
// **/
void Layer3LteDetachUE(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LTE_STRING_CATEGORY_TYPE_LOST_DETECTION","
            LTE_STRING_FORMAT_RNTI","
            "[detach UE]",
            oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
    }
#endif

    // remove route to the RNTI lost
    Layer3LteDeleteRoute(node, interfaceIndex, oppositeRnti);
    // DetachUE process must be ahead of Disconnect and Remove process
    EpcLteAppSend_DetachUE(node, interfaceIndex,
        oppositeRnti, LteLayer2GetRnti(node, interfaceIndex));
    // detach at phy layer
    PhyLteIndicateDisconnectToUe(node, phyIndex, oppositeRnti);
    // remove connection info
    Layer3LteRemoveConnectionInfo(node, interfaceIndex, oppositeRnti);
}



////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteNotifyLostDetection
// LAYER      :: RRC
// PURPOSE    :: Notify lost detection
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : LteRnti oppositeRnti : RNTI lost
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyLostDetection(
    Node* node, int interfaceIndex, LteRnti oppositeRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    std::string stName = stationType == LTE_STATION_TYPE_ENB ?
        LTE_STRING_STATION_TYPE_UE : LTE_STRING_STATION_TYPE_ENB;
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_LOST_DETECTION,
        stName.c_str(), oppositeRnti.nodeId, oppositeRnti.interfaceIndex);
#endif

    if (stationType == LTE_STATION_TYPE_ENB)
    {
        Layer3LteDetachUE(node, interfaceIndex, oppositeRnti);
    }
    else if (stationType == LTE_STATION_TYPE_UE)
    {
        // remove route to the RNTI lost
        Layer3LteDeleteRoute(node, interfaceIndex, oppositeRnti);
        // restart UE
        Layer3LteRestart(node, interfaceIndex);
    }
    else
    {
        ERROR_Assert(FALSE, "invalid station type");
    }
}



// /**
// FUNCTION   :: Layer3LteNotifyFailureRAToTargetEnbOnHandover
// LAYER      :: RRC
// PURPOSE    :: Notify lost detection
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyFailureRAToTargetEnbOnHandover(
    Node* node, int interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);

#ifdef LTE_LIB_LOG
    const LteRnti& ownRnti = LteLayer2GetRnti(node, interfaceIndex);
    LteConnectionInfo* handoverInfo =
        Layer3LteGetHandoverInfo(node, interfaceIndex, ownRnti);
    ERROR_Assert(handoverInfo, "unexpectedd error");
    HandoverParticipator& hoParticipator = handoverInfo->hoParticipator;

    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        LAYER3_LTE_MEAS_CAT_HANDOVER","
        LTE_STRING_FORMAT_RNTI","
        "HoEnd,"
        LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
        "isSuccess=,0",
        LTE_INVALID_RNTI.nodeId,
        LTE_INVALID_RNTI.interfaceIndex,
        hoParticipator.ueRnti.nodeId,
        hoParticipator.ueRnti.interfaceIndex,
        hoParticipator.srcEnbRnti.nodeId,
        hoParticipator.srcEnbRnti.interfaceIndex,
        hoParticipator.tgtEnbRnti.nodeId,
        hoParticipator.tgtEnbRnti.interfaceIndex);
#endif

    Layer3LteRestart(node, interfaceIndex);

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numHandoversFailed++;
}



////////////////////////////////////////////////////////////////////////////
//  Static functions
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteInitConfigurableParameters
// LAYER      :: Layer3
// PURPOSE    :: Initialize configurable parameters of Layer 3.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
static void Layer3LteInitConfigurableParameters(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
    char errBuf[MAX_STRING_LENGTH] = {0};
    char warnBuf[MAX_STRING_LENGTH] = {0};
    BOOL wasFound = FALSE;
    clocktype retTime = 0;
    double retDouble = 0.0;

    LteRrcConfig* rrcConfig =
        GetLteRrcConfig(node, interfaceIndex);
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);

    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            node, node->nodeId, interfaceIndex);

    switch (stationType)
    {
        case LTE_STATION_TYPE_ENB:
        {
            IO_ReadDouble(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_MEAS_FILTER_COEFFICIENT,
                        &wasFound,
                        &retDouble);
            if (wasFound == TRUE)
            {
                if (retDouble >= 0 &&
                    retDouble <= RRC_LTE_MAX_MEAS_FILTER_COEFFICIENT)
                {
                    rrcConfig->filterCoefficient = retDouble;
                }
                else
                {
                    sprintf(errBuf,
                        "RRC-LTE: FilterCoefficient "
                        "should be  %.1lf to %.1lf."
                        " Please check %s.",
                        0.0,
                        RRC_LTE_MAX_MEAS_FILTER_COEFFICIENT,
                        RRC_LTE_STRING_MEAS_FILTER_COEFFICIENT);
                    ERROR_ReportError(errBuf);
                }
            }
            else
            {
                sprintf(warnBuf,
                        "RRC-LTE: FilterCoefficient should be set. "
                        "Change FilterCoefficient to %.1lf.",
                        RRC_LTE_DEFAULT_MEAS_FILTER_COEFFICIENT);
                ERROR_ReportWarning(warnBuf);
                rrcConfig->filterCoefficient =
                    RRC_LTE_DEFAULT_MEAS_FILTER_COEFFICIENT;
            }
            IO_ReadTime(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_IGNORE_HO_DECISION_TIME,
                        &wasFound,
                        &retTime);
            if (wasFound == TRUE)
            {
                if (retTime >= 0 &&
                    retTime <= CLOCKTYPE_MAX)
                {
                    layer3Data->ignoreHoDecisionTime = retTime;
                }
                else
                {
                    sprintf(errBuf,
                        "RRC-LTE: IgnoreHoDecisionTime "
                        "should be  %.1lf to %.1lf."
                        " Please check %" TYPES_64BITFMT "d.",
                        0.0,
                        0.0,
                        (Int64)CLOCKTYPE_MAX);
                    ERROR_ReportError(errBuf);
                }
            }
            else
            {
                layer3Data->ignoreHoDecisionTime =
                    RRC_LTE_DEFAULT_HO_IGNORED_TIME;
            }
            break;
        }
        case LTE_STATION_TYPE_UE:
        {
            IO_ReadTime(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_WAIT_RRC_CONNECTED_TIME,
                        &wasFound,
                        &retTime);
            if (wasFound == TRUE)
            {
                if (retTime >= 0 && retTime <= CLOCKTYPE_MAX)
                {
                    layer3Data->waitRrcConnectedTime = retTime;
                }
                else
                {
                    sprintf(errBuf,
                        "RRC-LTE: Waiting RRC_CONNECTED Time "
                        "should not be negative integer."
                        " Please check %s.",
                        RRC_LTE_STRING_WAIT_RRC_CONNECTED_TIME);
                    ERROR_ReportError(errBuf);
                }
            }
            else
            {
                sprintf(warnBuf,
                    "RRC-LTE: Waiting RRC_CONNECTED Time should be set. "
                    "Change Waiting RRC_CONNECTED Time to %"
                    TYPES_64BITFMT "dmsec.",
                    RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_TIME/MILLI_SECOND);
                ERROR_ReportWarning(warnBuf);
                layer3Data->waitRrcConnectedTime =
                    RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_TIME;
            }
            IO_ReadTime(node->nodeId,
                        interfaceAddress,
                        nodeInput,
                        RRC_LTE_STRING_WAIT_RRC_CONNECTED_RECONF_TIME,
                        &wasFound,
                        &retTime);
            if (wasFound == TRUE)
            {
                if (retTime >= 0 && retTime <= CLOCKTYPE_MAX)
                {
                    layer3Data->waitRrcConnectedReconfTime = retTime;
                }
                else
                {
                    sprintf(errBuf,
                        "RRC-LTE: Waiting RRC_CONNECTED_RECONF Time "
                        "should not be negative integer."
                        " Please check %s.",
                        RRC_LTE_STRING_WAIT_RRC_CONNECTED_RECONF_TIME);
                    ERROR_ReportError(errBuf);
                }
            }
            else
            {
                sprintf(warnBuf,
                    "RRC-LTE: Waiting RRC_CONNECTED_RECONF Time should be "
                    "set. Change Waiting RRC_CONNECTED_RECONF Time to %"
                    TYPES_64BITFMT "d msec.",
                    RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_TIME/MILLI_SECOND);
                ERROR_ReportWarning(warnBuf);
                layer3Data->waitRrcConnectedReconfTime =
                    RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_RECONF_TIME;
            }
            break;
        }
        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "LTE do not support LteStationType(%d)\n"
                "Please check LteStationType.\n"
                , stationType);
            ERROR_ReportError(errBuf);
            break;
        }
    }
}

// /**
// FUNCTION   :: Layer3LtePrintStat
// LAYER      :: Layer3
// PURPOSE    :: Print Statictics.
// PARAMETERS ::
// + node             : Node*                    : Pointer to node.
// + interfaceIndex   : int                      : Interface index
// + stat             : const LteLayer3StatData& : Statistics Data.
// RETURN     :: void : NULL
// **/
static void Layer3LtePrintStat(
    Node* node, int interfaceIndex, const LteLayer3StatData& rrcStat)
{
    char buf[MAX_STRING_LENGTH] = {0};
    sprintf(buf, "Number of RRC Connection Establishment = %d",
        rrcStat.numRrcConnectionEstablishment);

    IO_PrintStat(node,
                 "Layer3",
                 "LTE RRC",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    if (!rrcStat.numRrcConnectionEstablishment)
    {
        sprintf(buf,
                "Average count of Retry RRC Connection Establishment = %d",
                rrcStat.averageRetryRrcConnectionEstablishment);
    }
    else
    {
        sprintf(buf,
                "Average count of Retry RRC Connection Establishment "
                    "= %.2lf",
                rrcStat.averageRetryRrcConnectionEstablishment /
                    (double)rrcStat.numRrcConnectionEstablishment);
    }

    IO_PrintStat(node,
                 "Layer3",
                 "LTE RRC",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    if (!rrcStat.numRrcConnectionEstablishment)
    {
        sprintf(buf,
                "Average time of RRC Connection Establishment[msec] = %"
                TYPES_64BITFMT "d",
                rrcStat.averageTimeOfRrcConnectionEstablishment);
    }
    else
    {
        sprintf(buf,
               "Average time of RRC Connection Establishment[msec] = %.2lf",
               rrcStat.averageTimeOfRrcConnectionEstablishment /
                    (double)rrcStat.numRrcConnectionEstablishment /
                    (double)MILLI_SECOND);
    }

    IO_PrintStat(node,
                 "Layer3",
                 "LTE RRC",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
}

// /**
// FUNCTION   :: Layer3LteCreateRadioBearer
// LAYER      :: Layer3
// PURPOSE    :: Create Radio Bearer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + rnti             : const LteRnti&    : RNTI
// + connectedInfo    : LteConnectedInfo* : Connected Info
// RETURN     :: void : NULL
// **/
static void Layer3LteCreateRadioBearer(
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LteConnectedInfo* connectedInfo,
    BOOL isTargetEnb)
{
    PairLteRadioBearer radioBearerPair(
        LTE_DEFAULT_BEARER_ID, LteRadioBearer());

    // Init PDCP Entity
    PdcpLteInitPdcpEntity(
        node, interfaceIndex, rnti, &(radioBearerPair.second.pdcpEntity),
        isTargetEnb);

    // Init RLC Entity
    LteRlcEntityType rlcEntityType = LTE_RLC_ENTITY_AM;
    LteRlcEntityDirect rlcEntityDirect = LTE_RLC_ENTITY_BI;
    LteRlcInitRlcEntity(node,
                        interfaceIndex,
                        rnti,
                        rlcEntityType,
                        rlcEntityDirect,
                        &(radioBearerPair.second.rlcEntity));

    std::pair<MapLteRadioBearer::iterator, bool> insertResult =
        connectedInfo->radioBearers.insert(radioBearerPair);

    // set RLC Entity address
    insertResult.first->second.rlcEntity.setRlcEntityToXmEntity();
}

// /**
// FUNCTION   :: Layer3LteDeleteRadioBearer
// LAYER      :: Layer3
// PURPOSE    :: Delete Radio Bearer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + rnti             : const LteRnti&    : RNTI
// + connectedInfo    : LteConnectedInfo* : Connected Info
// RETURN     :: void : NULL
// **/
static void Layer3LteDeleteRadioBearer(
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LteConnectedInfo* connectedInfo)
{
    MapLteRadioBearer::iterator itr;
    for (itr  = connectedInfo->radioBearers.begin();
        itr != connectedInfo->radioBearers.end();
        ++itr)
    {
        // Finalize PDCP Entity
        PdcpLteFinalizePdcpEntity(
            node, interfaceIndex, rnti, &((*itr).second.pdcpEntity));

        // Finalize RLC Entity
        LteRlcRelease(node, interfaceIndex, rnti, (*itr).first);
    }

    connectedInfo->radioBearers.clear();
}

// /**
// FUNCTION   :: Layer3LteSetState
// LAYER      :: Layer3
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : IN : Node*          : Pointer to node.
// + interfaceIndex     : IN : int            : Interface index
// + state              : IN : Layer3LteState : State
// RETURN     :: void : NULL
// **/
static void Layer3LteSetState(
    Node* node, int interfaceIndex, Layer3LteState state)
{
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    std::string before = GetLteLayer3StateName(layer3Data->layer3LteState);
    std::string after  = GetLteLayer3StateName(state);
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,%s=,%s,%s=,%s",
        LTE_STRING_CATEGORY_TYPE_SET_STATE,
        LTE_STRING_COMMON_TYPE_BEFORE, before.c_str(),
        LTE_STRING_COMMON_TYPE_AFTER, after.c_str());
#endif

    layer3Data->layer3LteState = state;
}



// /**
// FUNCTION   :: Layer3LteSetupMeasConfig
// LAYER      :: Layer3
// PURPOSE    :: Set Up Measurement Configuration
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + nodeInput      : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void Layer3LteSetupMeasConfig(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
    char buf[10*MAX_STRING_LENGTH];
    BOOL wasFound = false;

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    VarMeasConfig* varMeasConfig = &layer3Data->varMeasConfig;
    varMeasConfig->Clear();

    /* setup MeasObjectList ******************************************/
    // for each listenable channel
    int numChannels = PROP_NumberChannels(node);
    int phyIndex = LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
    int newMeasObjectId = 0;
    for (int i = 0; i < numChannels; i++)
    {
        if (PHY_CanListenToChannel(node, phyIndex, i))
        {
            // new entry
            MeasObject newMeasObjectEntry(newMeasObjectId++, i);
            // add
            varMeasConfig->measObjectList.push_back(newMeasObjectEntry);
        }
    }

#ifdef LTE_LIB_LOG
    {
        stringstream ss;
        ss << "[";
        for (ListMeasObject::iterator it = varMeasConfig->measObjectList.begin();
            it != varMeasConfig->measObjectList.end(); it++)
        {
            if (it != varMeasConfig->measObjectList.begin())
                ss << ",";
            ss << it->ToString();
        }
        ss << "]";
        Layer2DataLte* l2data = LteLayer2GetLayer2DataLte(node, interfaceIndex);
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "\"MeasObjectList: %s\"",
            l2data->myRnti.nodeId, l2data->myRnti.interfaceIndex,
            ss.str().c_str());
    }
#endif

    NodeAddress subnetAddress;
    subnetAddress = MAPPING_GetSubnetAddressForInterface(
                                node,
                                node->nodeId,
                                interfaceIndex);

    /* setup ReportConfigList ******************************************/
    // read RRC-LTE-MEAS-OBSERVING-EVENT-MASK-RSRP and check format
    char observingEventMaskRSRP[LAYER3_LTE_MEAS_EVENT_A_NUM + 1];
    IO_ReadString(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRP,
        &wasFound,
        buf);
    if (wasFound) {
        // invalid length
        if (strlen(buf) != (unsigned)(LAYER3_LTE_MEAS_EVENT_A_NUM)) {
            char errorMessage[10*MAX_STRING_LENGTH];
            sprintf(errorMessage,
                "invalid param: %s\n"
                "length must be %d",
                RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRP,
                LAYER3_LTE_MEAS_EVENT_A_NUM);
            ERROR_ReportError(errorMessage);
        }
        strcpy(observingEventMaskRSRP, buf);
        // check format
        for (size_t i = 0; i < strlen(observingEventMaskRSRP); i++) {
            // invalid format
            if (observingEventMaskRSRP[i] != '1' && observingEventMaskRSRP[i] != '0') {
                ERROR_ReportError(
                    "Error: " RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRP " is "
                    "incorrectly formatted.\n");
            }
        }
    }
    else
    {
  //      char errorMessage[10*MAX_STRING_LENGTH];
        //sprintf(errorMessage,
        //    "the param %s is needed.",
        //    RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK);
  //      ERROR_ReportError(errorMessage);

        // not error even if blank
        for (int i = 0; i < LAYER3_LTE_MEAS_EVENT_A_NUM; i++)
        {
            observingEventMaskRSRP[i] = '0';
        }
    }
    // read RRC-LTE-MEAS-OBSERVING-EVENT-MASK-RSRQ and check format
    char observingEventMaskRSRQ[LAYER3_LTE_MEAS_EVENT_A_NUM + 1];
    IO_ReadString(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRQ,
        &wasFound,
        buf);
    if (wasFound) {
        // invalid length
        if (strlen(buf) != (unsigned)(LAYER3_LTE_MEAS_EVENT_A_NUM)) {
            char errorMessage[10*MAX_STRING_LENGTH];
            sprintf(errorMessage,
                "invalid param: %s\n"
                "length must be %d",
                RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRQ,
                LAYER3_LTE_MEAS_EVENT_A_NUM);
            ERROR_ReportError(errorMessage);
        }
        strcpy(observingEventMaskRSRQ, buf);
        // check format
        for (size_t i = 0; i < strlen(observingEventMaskRSRQ); i++) {
            // invalid format
            if (observingEventMaskRSRQ[i] != '1' && observingEventMaskRSRQ[i] != '0') {
                ERROR_ReportError(
                    "Error: " RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK_RSRQ " is "
                    "incorrectly formatted.\n");
            }
        }
    }
    else
    {
  //      char errorMessage[10*MAX_STRING_LENGTH];
        //sprintf(errorMessage,
        //    "the param %s is needed.",
        //    RRC_LTE_STRING_MEAS_OBSERVING_EVENT_MASK);
  //      ERROR_ReportError(errorMessage);

        // not error even if blank
        for (int i = 0; i < LAYER3_LTE_MEAS_EVENT_A_NUM; i++)
        {
            observingEventMaskRSRQ[i] = '0';
        }
    }
    // read RRC-LTE-MEAS-REPORT-TIME-TO-TRIGGER
    clocktype timeToTrigger = LAYER3_LTE_MEAS_DEFAULT_TIME_TO_TRIGGER;    // default value
    IO_ReadTime(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_TIME_TO_TRIGGER,
        &wasFound,
        &timeToTrigger);
    ERROR_Assert(timeToTrigger >= LAYER3_LTE_MEAS_MIN_TIME_TO_TRIGGER,
        "TimeToTrigger is out of range");
    // read RRC-LTE-MEAS-REPORT-INTERVAL
    clocktype reportInterval = LAYER3_LTE_MEAS_DEFAULT_REPORT_INTERVAL;    // default value
    IO_ReadTime(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_REPORT_INTERVAL,
        &wasFound,
        &reportInterval);
    ERROR_Assert(reportInterval > LAYER3_LTE_MEAS_MIN_REPORT_INTERVAL,
        "ReportInterval is out of range");
    // read RRC-LTE-MEAS-REPORT-AMOUNT
    int reportAmount = LAYER3_LTE_MEAS_DEFAULT_REPORT_AMOUNT;    // default value
    IO_ReadInt(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_REPORT_AMOUNT,
        &wasFound,
        &reportAmount);
    ERROR_Assert(reportAmount >= LAYER3_LTE_MEAS_MIN_REPORT_AMOUNT,
        "ReportAmount is out of range");

    int newReportConfigId = 0;
    for (int measTypeIndex = 0; measTypeIndex < LAYER3_LTE_MEAS_MEASUREMENT_TYPE_NUM; measTypeIndex++)
    {
        // 0: RSRP_FOR_HO, 1: RSRQ_FOR_HO
        LteMeasurementType newMeasType = (measTypeIndex == 0) ? RSRP_FOR_HO : RSRQ_FOR_HO;
        char* observingEventMask = (measTypeIndex == 0) ? observingEventMaskRSRP : observingEventMaskRSRQ;

        for (int measEventIndex = 0; measEventIndex < MEAS_EVENT_TYPE_NUM; measEventIndex++)
        {
            // trigger to observe
            if (observingEventMask[measEventIndex] == '1')
            {
                MeasEventType newMeasEventType = (MeasEventType)measEventIndex;

                // new entry
                ReportConfig newReportConfigEntry;
                // set each field
                newReportConfigEntry.id = newReportConfigId++; 
                // new meas event
                MeasEvent newMeasEvent;
                {
                    // event type
                    newMeasEvent.eventType = newMeasEventType;
                    // quantity type
                    newMeasEvent.quantityType = newMeasType;
                    // time to trigger
                    newMeasEvent.timeToTrigger = timeToTrigger;
                    // event var
                    //Layer3LteGetMeasEventVar(
                    //    node, interfaceIndex, nodeInput,
                    //    newMeasEventType, newMeasType,
                    //    &newMeasEvent.eventVar, &newMeasEvent.hysteresis);
                    Layer3LteMeasGetMeasEventVar(
                        node, interfaceIndex, nodeInput,
                        newMeasEventType, newMeasType,
                        &newMeasEvent.eventVar, &newMeasEvent.hysteresis);

                }
                newReportConfigEntry.measEvent = newMeasEvent;
                // report amount
                newReportConfigEntry.reportAmount = reportAmount;
                // report interval
                newReportConfigEntry.reportInterval = reportInterval;
                // trigger quantity
                //newReportConfigEntry.triggerQuantity = this is always equal to newReportConfigEntry.measEvent.quantityType, so commented out.

                // add
                varMeasConfig->reportConfigList.push_back(newReportConfigEntry);
            }
        }    // for (int measEventIndex = 0; measEventIndex < MEAS_EVENT_TYPE_NUM; measEventIndex++)
    }    // for (int measTypeIndex = 0; measTypeIndex < LAYER3_LTE_MEAS_MEASUREMENT_TYPE_NUM; measTypeIndex++)

#ifdef LTE_LIB_LOG
    {
        stringstream ss;
        ss << "[";
        for (ListReportConfig::iterator it = varMeasConfig->reportConfigList.begin();
            it != varMeasConfig->reportConfigList.end(); it++)
        {
            if (it != varMeasConfig->reportConfigList.begin())
                ss << ",";
            ss << it->ToString();
        }
        ss << "]";
        Layer2DataLte* l2data = LteLayer2GetLayer2DataLte(node, interfaceIndex);
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "\"ReportConfigList: %s\"",
            l2data->myRnti.nodeId, l2data->myRnti.interfaceIndex,
            ss.str().c_str());
    }
#endif


    ///* setup MeasInfoList ******************************************/
    int newMeasInfoId = 0;
    // intersection of the above two list
    for (ListMeasObject::iterator it_measObject = varMeasConfig->measObjectList.begin();
        it_measObject != varMeasConfig->measObjectList.end(); it_measObject++)
    {
        for (ListReportConfig::iterator it_reportConfig = varMeasConfig->reportConfigList.begin();
            it_reportConfig != varMeasConfig->reportConfigList.end(); it_reportConfig++)
        {
            // new entry
            MeasInfo newMeasInfoEntry;
            // set each fields
            newMeasInfoEntry.id = newMeasInfoId++;
            newMeasInfoEntry.measObject = &(*it_measObject);
            newMeasInfoEntry.reportConfig = &(*it_reportConfig);
            // add
            varMeasConfig->measInfoList.push_back(newMeasInfoEntry);
        }
    }
    
#ifdef LTE_LIB_LOG
    {
        stringstream ss;
        ss << "[";
        for (ListMeasInfo::iterator it = varMeasConfig->measInfoList.begin();
            it != varMeasConfig->measInfoList.end(); it++)
        {
            if (it != varMeasConfig->measInfoList.begin())
                ss << ",";
            ss << it->ToString();
        }
        ss << "]";
        Layer2DataLte* l2data = LteLayer2GetLayer2DataLte(node, interfaceIndex);
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "\"MeasInfoList: %s\"",
            l2data->myRnti.nodeId, l2data->myRnti.interfaceIndex,
            ss.str().c_str());
    }
#endif

    /* setup QuantityConfig ******************************************/
    varMeasConfig->quantityConfig.filterCoefRSRP = LAYER3_LTE_MEAS_DEFAULT_FILTER_COEF_RSRP;    // default value
    IO_ReadInt(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_QUANTITY_CONFIG_FILTER_COEF_RSRP,
        &wasFound,
        &varMeasConfig->quantityConfig.filterCoefRSRP);
        ERROR_Assert(varMeasConfig->quantityConfig.filterCoefRSRP >= 0
            && varMeasConfig->quantityConfig.filterCoefRSRP
                <= RRC_LTE_MAX_MEAS_FILTER_COEFFICIENT,
            "FilterCoefRSRP is out of range");

    varMeasConfig->quantityConfig.filterCoefRSRQ = LAYER3_LTE_MEAS_DEFAULT_FILTER_COEF_RSRQ;    // default value
    IO_ReadInt(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_QUANTITY_CONFIG_FILTER_COEF_RSRQ,
        &wasFound,
        &varMeasConfig->quantityConfig.filterCoefRSRQ);
        ERROR_Assert(varMeasConfig->quantityConfig.filterCoefRSRQ >= 0
            && varMeasConfig->quantityConfig.filterCoefRSRQ
                <= RRC_LTE_MAX_MEAS_FILTER_COEFFICIENT,
            "FilterCoefRSRQ is out of range");

#ifdef LTE_LIB_LOG
    {
        Layer2DataLte* l2data = LteLayer2GetLayer2DataLte(node, interfaceIndex);
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "\"QuantityConfig: %s\"",
            l2data->myRnti.nodeId, l2data->myRnti.interfaceIndex,
            varMeasConfig->quantityConfig.ToString().c_str());
    }
#endif


    /* setup MeasCapConfig ******************************************/
    // type
    varMeasConfig->measGapConfig.type = LAYER3_LTE_MEAS_DEFAULT_GAP_TYPE;    // default value
    int gapTypeInt = 0;
    IO_ReadInt(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_GAP_CONFIG_TYPE,
        &wasFound,
        &gapTypeInt);
    if (wasFound)
    {
        if (gapTypeInt == 0)
            varMeasConfig->measGapConfig.type = MEAS_GAP_TYPE0;
        else if (gapTypeInt == 1)
            varMeasConfig->measGapConfig.type = MEAS_GAP_TYPE1;
        else
            ERROR_ReportError(
                "Error: " RRC_LTE_STRING_MEAS_GAP_CONFIG_TYPE " is "
                "incorrectly formatted.\n");
        
    }
    // offset
    varMeasConfig->measGapConfig.gapOffset = LAYER3_LTE_MEAS_DEFAULT_GAP_OFFSET;    // default value
    IO_ReadInt(
        node->nodeId,
        subnetAddress,
        nodeInput,
        RRC_LTE_STRING_MEAS_GAP_CONFIG_OFFSET,
        &wasFound,
        &varMeasConfig->measGapConfig.gapOffset);

#ifdef LTE_LIB_LOG
    {
        Layer2DataLte* l2data = LteLayer2GetLayer2DataLte(node, interfaceIndex);
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "\"MeasCapConfig: %s\"",
            l2data->myRnti.nodeId, l2data->myRnti.interfaceIndex,
            varMeasConfig->measGapConfig.ToString().c_str());
    }
#endif

}

// /**
// FUNCTION   :: Layer3LteSetupMeasEventConditionTable
// LAYER      :: RRC
// PURPOSE    :: Setup Event Condition Observing Information
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteSetupMeasEventConditionTable(Node *node,
                    UInt32 interfaceIndex)
{ 
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // measEventConditionTable
    MeasEventConditionTable* measEventConditionTable
        = &layer3Data->measEventConditionTable;
    measEventConditionTable->clear();
    // varMeasConfig
    VarMeasConfig* varMeasConfig = &layer3Data->varMeasConfig;

    // setup event condition table for each measurement information
    for (ListMeasInfo::iterator it = varMeasConfig->measInfoList.begin();
        it != varMeasConfig->measInfoList.end(); it++)
    {
        MapRntiMeasEventConditionInfo newMapRntiMeasEventConditionInfo;
        measEventConditionTable->insert(
            MeasEventConditionTable::value_type(
                it->id,
                newMapRntiMeasEventConditionInfo
            )
        );
    }

}

// /**
// FUNCTION   :: Layer3LteInitMeasurement
// LAYER      :: RRC
// PURPOSE    :: initialize measurement
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteInitMeasurement(
    Node* node, UInt32 interfaceIndex)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    // configure intra-freq measurement
    PhyLteIFPHConfigureMeasConfig(
        node, phyIndex,
        LAYER3_LTE_MEAS_DEFAULT_INTRA_FREQ_INTERVAL,
        LAYER3_LTE_MEAS_DEFAULT_INTRA_FREQ_OFFSET,
        layer3Data->varMeasConfig.quantityConfig.filterCoefRSRP,
        layer3Data->varMeasConfig.quantityConfig.filterCoefRSRQ,
        (layer3Data->varMeasConfig.measGapConfig.type == MEAS_GAP_TYPE0) ?
            40000000 : 80000000);

    // start intra-freq measurement
    PhyLteIFPHStartMeasIntraFreq(node, phyIndex);

    // start inter-freq measurement
    PhyLteIFPHStartMeasInterFreq(node, phyIndex);

}

// /**
// FUNCTION   :: Layer3LteIFHPNotifyL3FilteringUpdated
// LAYER      :: RRC
// PURPOSE    :: IF for PHY to notify about measurement result updated
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteIFHPNotifyL3FilteringUpdated(
                    Node *node,
                    UInt32 interfaceIndex)
{
    // this function works only on UE
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // VarMeasConfig
    VarMeasConfig* varMeasConfig = &layer3Data->varMeasConfig;

    // iterated by MeasInfoList
    for (ListMeasInfo::iterator it_info = varMeasConfig->measInfoList.begin();
        it_info != varMeasConfig->measInfoList.end(); it_info++)
    {
        // MeasInfo
        MeasInfo* measInfo = &(*it_info);

        // update condition table
        Layer3LteMeasUpdateConditionTable(node, interfaceIndex, measInfo);

        // check evnets and report if necessary
        Layer3LteMeasCheckEventToReport(node, interfaceIndex, measInfo);
    }

}

// /**
// FUNCTION   :: Layer3LteIFHPNotifyMeasurementReportReceived
// LAYER      :: RRC
// PURPOSE    :: IF for PHY to notify Measurement Repot Received to RRC
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + ueRnti           : const LteRnti&    : the source of the report
// + measurementReport: std::list<MeasurementReport>*: report
// RETURN     :: void : NULL
// **/
void Layer3LteIFHPNotifyMeasurementReportReceived(
                    Node *node,
                    UInt32 interfaceIndex,
                    const LteRnti& ueRnti,
                    std::list<MeasurementReport>* measurementReport)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[received measurement reports]",
            ueRnti.nodeId, ueRnti.interfaceIndex);
    }
#endif

    // discard messages from UE which doesn't connect to this eNB
    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, ueRnti);
    if (connInfo == NULL ||
        connInfo->state != LAYER3_LTE_CONNECTION_CONNECTED)
    {
#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[discard received measurement reports]",
                ueRnti.nodeId, ueRnti.interfaceIndex);
        }
#endif
        return;
    }

    // discard messages from UE which hasn't finished previous H.O.
    BOOL receivedEndMarker = Layer3LteGetTimerForRrc(
        node, interfaceIndex, ueRnti, MSG_MAC_LTE_X2_WaitEndMarker)
        == NULL;
    BOOL receivedPathSwitchRequestAck = Layer3LteGetTimerForRrc(
        node, interfaceIndex, ueRnti, MSG_MAC_LTE_S1_WaitPathSwitchReqAck)
        == NULL;
    if (!receivedEndMarker || !receivedPathSwitchRequestAck)
    {
#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[discard received measurement reports because"
                " the UE hasn't finished previous H.O. yet.]",
                ueRnti.nodeId, ueRnti.interfaceIndex);
        }
#endif
        return;
    }

    // HO decision
    LteRnti targetRnti = LTE_INVALID_RNTI;
    Layer3LteHandOverDecision(node, interfaceIndex, ueRnti,
        measurementReport, &targetRnti);
    if (targetRnti == LTE_INVALID_RNTI)
    {
        // the UE dosen't hand over
        return;
    }

    // send HandoverRequest to target eNB
    Layer2DataLte* layer2 = LteLayer2GetLayer2DataLte(node, interfaceIndex);
    HandoverParticipator hoParticipator(ueRnti, layer2->myRnti, targetRnti);
    EpcLteAppSend_HandoverRequest(node, interfaceIndex, hoParticipator);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        LAYER3_LTE_MEAS_CAT_HANDOVER","
        LTE_STRING_FORMAT_RNTI","
        "HoStart,"
        LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
        LTE_INVALID_RNTI.nodeId,
        LTE_INVALID_RNTI.interfaceIndex,
        hoParticipator.ueRnti.nodeId,
        hoParticipator.ueRnti.interfaceIndex,
        hoParticipator.srcEnbRnti.nodeId,
        hoParticipator.srcEnbRnti.interfaceIndex,
        hoParticipator.tgtEnbRnti.nodeId,
        hoParticipator.tgtEnbRnti.interfaceIndex);
#endif

    // set timer TRELOCprep
    Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_RELOCprep, RRC_LTE_DEFAULT_RELOC_PREP_TIME);
}

// /**
// FUNCTION   :: Layer3LteHandOverDecision
// LAYER      :: RRC
// PURPOSE    :: handover decision
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + ueRnti           : const LteRnti&    : the source of the report
// + measurementReport: std::list<MeasurementReport>*: report
// + targetRnti       : LteRnti*          : target eNB (if INVALID_RNTI
//                                          is set, doesn't hand over)
// RETURN     :: void : NULL
// **/
void Layer3LteHandOverDecision(
                    Node *node,
                    UInt32 interfaceIndex,
                    const LteRnti& ueRnti,
                    std::list<MeasurementReport>* measurementReport,
                    LteRnti* targetRnti)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    LteRnti tmpTgtRnti = LTE_INVALID_RNTI;

    ERROR_Assert(measurementReport->size() > 0, "a measurement report msg"
        "should contains one or more reports");

    // at least, target eNB's RSRP needs to exceed serving eNB's RSRP.
    double maxRsrpValue = measurementReport->begin()
        ->measResults.measResultServCell.rsrpResult;
    for (std::list<MeasurementReport>::iterator it =
        measurementReport->begin(); it != measurementReport->end(); ++it)
    {
        maxRsrpValue =
            max(it->measResults.measResultServCell.rsrpResult, maxRsrpValue);
    }

    // handover to eNB which has highest RSRP value
    for (std::list<MeasurementReport>::iterator it =
        measurementReport->begin(); it != measurementReport->end(); ++it)
    {
        ListMeasResultInfo& results = it->measResults.measResultNeighCells;
        for (ListMeasResultInfo::iterator it_result = results.begin();
            it_result != results.end(); ++it_result)
        {
            // skip nodes over our EPC subnet
            if (!EpcLteAppIsNodeOnTheSameEpcSubnet(
                node, it_result->rnti.nodeId))
            {
                continue;
            }

            // update max
            if (it_result->rsrpResult > maxRsrpValue)
            {
                maxRsrpValue = it_result->rsrpResult;
                tmpTgtRnti = it_result->rnti;
            }
        }
    }

    ERROR_Assert(tmpTgtRnti != LteLayer2GetRnti(node, interfaceIndex),
        "tried to handover to serving cell eNB");

    BOOL isDiscardHo = FALSE;
    LteConnectedInfo* connectedInfo =
        Layer3LteGetConnectedInfo(node, interfaceIndex, ueRnti);
    if (node->getNodeTime() <
        connectedInfo->connectedTime + RRC_LTE_DEFAULT_HO_IGNORED_TIME)
    {
        isDiscardHo = TRUE;
    }
    else
    {
        *targetRnti = tmpTgtRnti;
    }

#ifdef LTE_LIB_LOG
    char buf[MAX_STRING_LENGTH] = {0};
    if (isDiscardHo == TRUE)
    {
        sprintf(buf, "[handover decision is ignored]");
    }
    else
    {
        sprintf(buf, "[handover decision]");
    }

    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "%s,"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            buf,
            ueRnti.nodeId, ueRnti.interfaceIndex,
            node->nodeId, interfaceIndex,
            tmpTgtRnti.nodeId, tmpTgtRnti.interfaceIndex);
    }
#endif
}

// /**
// FUNCTION   :: Layer3LteAdmissionControl
// LAYER      :: RRC
// PURPOSE    :: admission control
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: BOOL : result
// **/
BOOL Layer3LteAdmissionControl(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    BOOL result = TRUE;

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[admission control],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
            "%s",
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex,
            result ? "TRUE" : "FALSE");
    }
#endif

    return result;
}

// /**
// FUNCTION   :: Layer3LteReceiveHoReq
// LAYER      :: RRC
// PURPOSE    :: process after receive handover request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoReq(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive handover request],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // never receive H.O. req of the UE which is handingover from this eNB
    // to another eNB.
    LteConnectionInfo* connInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
    BOOL isAcceptHoReq = connInfo == NULL ? TRUE : FALSE;
    BOOL admissionResult = Layer3LteAdmissionControl(
        node, interfaceIndex, hoParticipator);

    if (admissionResult == TRUE && isAcceptHoReq == TRUE)
    {
        // get connection config
        Layer2DataLte* layer2 =
            LteLayer2GetLayer2DataLte(node, interfaceIndex);
        RrcConnectionReconfiguration reconf;
        reconf.rrcConfig = *layer2->rrcConfig;

        // send HandoverRequestAck to source eNB
        EpcLteAppSend_HandoverRequestAck(
            node, interfaceIndex, hoParticipator, reconf);
        
        // prepare UE's connection info at handover info
        Layer3LteAddConnectionInfo(
            node, interfaceIndex, hoParticipator.ueRnti,
            LAYER3_LTE_CONNECTION_HANDOVER);
        Layer3LteSetHandoverParticipator(
            node, interfaceIndex, hoParticipator.ueRnti, hoParticipator);
        
        // set timer TX2WAIT_SN_STATUS_TRANSFER
        // (this should be after prepare UE's connction info)
        Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
            MSG_MAC_LTE_X2_WaitSnStatusTransfer,
            RRC_LTE_DEFAULT_X2_WAIT_SN_STATUS_TRANSFER_TIME);
        // set timer TWAIT_ATTACH_UE_BY_HO
        Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
            MSG_MAC_LTE_WaitAttachUeByHo,
            RRC_LTE_DEFAULT_WAIT_ATTCH_UE_BY_HO_TIME);

    }
    else
    {
        EpcLteAppSend_HoPreparationFailure(
            node, interfaceIndex, hoParticipator);
    }

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numHandoverRequestReceived++;
}

// /**
// FUNCTION   :: Layer3LteReceiveHoReqAck
// LAYER      :: RRC
// PURPOSE    :: process after receive handover request ack
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoReqAck(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    // cancel timer TRELOCprep
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_RELOCprep);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive handover request ack],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // send reconf to UE
    Layer3LteSendRrcConnReconfInclMobilityControlInfomation(
        node, interfaceIndex, hoParticipator, reconf);

    // prepare for data forwarding
    Layer3LtePrepareDataForwarding(node, interfaceIndex, hoParticipator);

    // send SN status transfer
    Layer3LteSendSnStatusTransfer(node, interfaceIndex, hoParticipator);

    // start data forwarding
    Layer3LteStartDataForwarding(node, interfaceIndex, hoParticipator);

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numHandoverRequestAckReceived++;
}

// /**
// FUNCTION   :: Layer3LteSendSnStatusTransfer
// LAYER      :: RRC
// PURPOSE    :: send SnStatusTransfer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteSendSnStatusTransfer(
    Node *node,
    UInt32 interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    // send SN Status Transfer
    LteConnectionInfo* handoverInfo = Layer3LteGetHandoverInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
    ERROR_Assert(handoverInfo, "unexpected error");
    LteConnectedInfo* connInfo = &handoverInfo->connectedInfo;
    MapLteRadioBearer* bearers = &connInfo->radioBearers;
    std::map<int, PdcpLteSnStatusTransferItem> snStatusMap;
    for (MapLteRadioBearer::iterator it = bearers->begin();
        it != bearers->end(); ++it)
    {
        // get SnStatusTransferItem
        PdcpLteSnStatusTransferItem snStatusTransferItem =
            PdcpLteGetSnStatusTransferItem(
            node, interfaceIndex, hoParticipator.ueRnti, it->first);
        // store
        snStatusMap.insert(std::map<int, PdcpLteSnStatusTransferItem>
            ::value_type(it->first, snStatusTransferItem));

#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[store sn status],"
                LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
                "bearerId,%d, nextPdcpRxSn,%d, nextPdcpTxSn,%d",
                LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
                hoParticipator.ueRnti.nodeId,
                hoParticipator.ueRnti.interfaceIndex,
                hoParticipator.srcEnbRnti.nodeId,
                hoParticipator.srcEnbRnti.interfaceIndex,
                hoParticipator.tgtEnbRnti.nodeId,
                hoParticipator.tgtEnbRnti.interfaceIndex,
                it->first,
                snStatusTransferItem.nextPdcpRxSn,
                snStatusTransferItem.nextPdcpTxSn);
        }
#endif
    }

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[send sn status transfer],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // send SnStatusTransferItems
    EpcLteAppSend_SnStatusTransfer(
        node, interfaceIndex, hoParticipator, snStatusMap);
}

// /**
// FUNCTION   :: Layer3LteReceiveSnStatusTransfer
// LAYER      :: RRC
// PURPOSE    :: process after receive SnStatusTransfer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + snStatusTransferItem : std::map<int, PdcpLteSnStatusTransferItem>& : 
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveSnStatusTransfer(
    Node *node,
    UInt32 interfaceIndex,
    const HandoverParticipator& hoParticipator,
    std::map<int, PdcpLteSnStatusTransferItem>& snStatusTransferItem)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);
        
    // cancel timer TX2WAIT_SN_STATUS_TRANSFER
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_X2_WaitSnStatusTransfer);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive sn status transfer],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // notify SnStatusTransfer to PDCP
    for (std::map<int, PdcpLteSnStatusTransferItem>::iterator it =
        snStatusTransferItem.begin(); it != snStatusTransferItem.end();
        ++it)
    {
        PdcpLteSetSnStatusTransferItem(node, interfaceIndex,
            hoParticipator.ueRnti, it->first, it->second);

#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[notify sn status to PDCP],"
                LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
                "bearerId,%d, nextPdcpRxSn,%d, nextPdcpTxSn,%d",
                LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
                hoParticipator.ueRnti.nodeId,
                hoParticipator.ueRnti.interfaceIndex,
                hoParticipator.srcEnbRnti.nodeId,
                hoParticipator.srcEnbRnti.interfaceIndex,
                hoParticipator.tgtEnbRnti.nodeId,
                hoParticipator.tgtEnbRnti.interfaceIndex,
                it->first,
                it->second.nextPdcpRxSn,
                it->second.nextPdcpTxSn);
        }
#endif
    }

    // set isSchedulable flag to ON
    Layer3LteGetConnectedInfo(node, interfaceIndex, hoParticipator.ueRnti)
        ->isSchedulable = TRUE;

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numSnStatusTransferReceived++;
}

// /**
// FUNCTION   :: Layer3LteSendRrcConnReconfInclMobilityControlInfomation
// LAYER      :: RRC
// PURPOSE    :: send reconfiguration to handovering UE
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteSendRrcConnReconfInclMobilityControlInfomation(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf)
{
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    RrcConnReconfInclMoblityControlInfomationMap* rrcConnReconfMap
        = phyLte->rrcConnReconfMap;

    ERROR_Assert(
        rrcConnReconfMap->find(hoParticipator.ueRnti)
        == rrcConnReconfMap->end(), "reconf to the UE already set");

    // register to map
    RrcConnReconfInclMoblityControlInfomation newRrcConnReconf;
    newRrcConnReconf.reconf = reconf;
    newRrcConnReconf.hoParticipator = hoParticipator;
    rrcConnReconfMap->insert(RrcConnReconfInclMoblityControlInfomationMap::
        value_type(hoParticipator.ueRnti, newRrcConnReconf));

    // set timer TX2RELOCoverall
    Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_X2_RELOCoverall, RRC_LTE_DEFAULT_X2_RELOC_OVERALL_TIME);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[send rrc conn. reconf. incl. MobilityControlInfomation],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif
}

// /**
// FUNCTION   :: Layer3LteNotifyRrcConnReconf
// LAYER      :: RRC
// PURPOSE    :: notify reconfiguration received
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyRrcConnReconf(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[notify rrc conn. reconf.],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // clear each layer information
    Layer3LtePrepareForHandoverExecution(
        node,
        interfaceIndex,
        hoParticipator);
    // prepare for random access for H.O.
    PhyLtePrepareForHandoverExecution(
        node,
        phyIndex,
        hoParticipator.tgtEnbRnti,
        reconf.rrcConfig);
    // random access
    RrcLteRandomAccessForHandoverExecution(
        node, interfaceIndex, hoParticipator.tgtEnbRnti);
}

// /**
// FUNCTION   :: Layer3LtePrepareForHandoverExecution
// LAYER      :: RRC
// PURPOSE    :: clear information for handover execution
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LtePrepareForHandoverExecution(Node* node, int interfaceIndex,
                    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_UE, LTE_STRING_STATION_TYPE_UE);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[prepare for handover execution],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // these two routing processes should be done in the same simTime.
    // delete route to src eNB
    Layer3LteDeleteRoute(node, interfaceIndex, hoParticipator.srcEnbRnti);
    // add route
    Layer3LteAddRoute(node, interfaceIndex, hoParticipator.tgtEnbRnti);

    // clear each layer
    PhyLteClearInfo(node, phyIndex, hoParticipator.srcEnbRnti);
    PhyLteChangeState(node, phyIndex,
        (PhyDataLte*)node->phyData[phyIndex]->phyVar,
        PHY_LTE_IDLE_RANDOM_ACCESS);
    MacLteClearInfo(node, interfaceIndex, hoParticipator.srcEnbRnti);
    MacLteSetState(node, interfaceIndex,
        MAC_LTE_IDEL);

    // call reestablishment
    LteConnectedInfo* connInfo = Layer3LteGetConnectedInfo(
        node, interfaceIndex, hoParticipator.srcEnbRnti);
    for (MapLteRadioBearer::iterator it = connInfo->radioBearers.begin();
        it != connInfo->radioBearers.end(); ++it)
    {
        LteRlcReEstablishment(
            node, interfaceIndex, hoParticipator.srcEnbRnti, it->first,
            hoParticipator.tgtEnbRnti);
        PdcpLteReEstablishment(
            node, interfaceIndex, hoParticipator.srcEnbRnti, it->first);
    }

    // stop measurement
    PhyLteIFPHStopMeasIntraFreq(node, phyIndex);
    PhyLteIFPHStopMeasInterFreq(node, phyIndex);
    // reset
    Layer3LteMeasResetMeasurementValues(node, interfaceIndex);

    // don't clear Layer3Filtering. use these results after H.O.
    //// clear Layer3Filtering
    //LteLayer3Filtering* layer3Filtering =
    //    LteLayer2GetLayer3Filtering(node, interfaceIndex);
    //layer3Filtering->remove();

    // cancel all timer
    Layer3LteCancelAllTimerForRrc(node, interfaceIndex,
        hoParticipator.srcEnbRnti);

    // store connectedInfo (after this, eNB info is managed under target eNB)
    Layer3LteChangeConnectionState(
        node, interfaceIndex, hoParticipator.srcEnbRnti,
        LAYER3_LTE_CONNECTION_HANDOVER, hoParticipator);

    // change state
    Layer3LteSetState(node, interfaceIndex, LAYER3_LTE_RRC_IDLE);
}

// /**
// FUNCTION   :: Layer3LtePrepareDataForwarding
// LAYER      :: RRC
// PURPOSE    :: clear information for handover execution
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LtePrepareDataForwarding(
    Node* node, int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[prepare data forwarding],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif
    
    // delete phy info
    PhyLteIndicateDisconnectToUe(
        node, phyIndex, hoParticipator.ueRnti);

    // call reestablishment
    LteConnectedInfo* connectedInfo = Layer3LteGetConnectedInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
    ERROR_Assert(connectedInfo, "unexpectedd error");
    for (MapLteRadioBearer::iterator it = 
        connectedInfo->radioBearers.begin();
        it != connectedInfo->radioBearers.end(); ++it)
    {
        LteRlcReEstablishment(
            node, interfaceIndex, hoParticipator.ueRnti, it->first,
            hoParticipator.tgtEnbRnti);
        PdcpLteReEstablishment(
            node, interfaceIndex, hoParticipator.ueRnti, it->first);
    }

    // save connected info
    Layer3LteChangeConnectionState(
        node, interfaceIndex, hoParticipator.ueRnti,
        LAYER3_LTE_CONNECTION_HANDOVER, hoParticipator);
}

// /**
// FUNCTION   :: Layer3LteGetHandoverInfo
// LAYER      :: RRC
// PURPOSE    :: get handover info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: LteConnectionInfo* : info
// **/
LteConnectionInfo* Layer3LteGetHandoverInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);

    ListLteRnti lteRntiList;
    Layer3LteGetConnectionList(node, interfaceIndex, &lteRntiList,
        LAYER3_LTE_CONNECTION_HANDOVER);

    if (stationType == LTE_STATION_TYPE_UE || rnti == LTE_INVALID_RNTI)
    {
        if (lteRntiList.size() == 0)
        {
            return NULL;
        }
        else if (lteRntiList.size() == 1)
        {
            LteConnectionInfo* connInfo =
                Layer3LteGetConnectionInfo(node, interfaceIndex,
                *(lteRntiList.begin()));
            ERROR_Assert(connInfo->state == LAYER3_LTE_CONNECTION_HANDOVER,
                "unexpected error");
            return connInfo;
        }
        else
        {
            ERROR_Assert(FALSE, "unexpected error");
        }
    }
    else if (stationType == LTE_STATION_TYPE_ENB)
    {
        ERROR_Assert(rnti != LTE_INVALID_RNTI, "unexpected error");
        LteConnectionInfo* connInfo =
            Layer3LteGetConnectionInfo(node, interfaceIndex, rnti);
        if (connInfo)
        {
            ERROR_Assert(connInfo->state == LAYER3_LTE_CONNECTION_HANDOVER,
                "unexpected error");
            return connInfo;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        ERROR_Assert(FALSE, "unexpected error");
    }

    return NULL; // never come here
}

// /**
// FUNCTION   :: Layer3LteStartDataForwarding
// LAYER      :: RRC
// PURPOSE    :: start data forwarding process
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteStartDataForwarding(Node* node, int interfaceIndex,
                              const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[start data forwarding],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // Notify starting data forwarding to PDCP by each bearer ID
    LteConnectionInfo* handoverInfo = Layer3LteGetHandoverInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
    ERROR_Assert(handoverInfo, "unexpectedd error");
    LteConnectedInfo* connInfo = &handoverInfo->connectedInfo;
    for (MapLteRadioBearer::iterator it = 
        connInfo->radioBearers.begin();
        it != connInfo->radioBearers.end(); ++it)
    {
        PdcpLteForwardBuffer(
            node,
            interfaceIndex,
            hoParticipator.ueRnti,
            it->first);

#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[request data forwarding to PDCP],"
                LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
                "bearerId,%d",
                LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
                hoParticipator.ueRnti.nodeId,
                hoParticipator.ueRnti.interfaceIndex,
                hoParticipator.srcEnbRnti.nodeId,
                hoParticipator.srcEnbRnti.interfaceIndex,
                hoParticipator.tgtEnbRnti.nodeId,
                hoParticipator.tgtEnbRnti.interfaceIndex,
                it->first);
        }
#endif
    }
}

// /**
// FUNCTION   :: Layer3LteSendForwardingDataList
// LAYER      :: RRC
// PURPOSE    :: send forwarding data list
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : UE's RNTI
// + bearerId       : int            : bearer ID of the message list
// + forwardingMsg  : std::list<Message*> : message list
// RETURN     :: void : NULL
// **/
void Layer3LteSendForwardingDataList(Node* node, int interfaceIndex,
    const LteRnti& ueRnti, int bearerId, std::list<Message*>* forwardingMsg)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    // get hanover info
    LteConnectionInfo* handoverInfo = Layer3LteGetHandoverInfo(
        node, interfaceIndex, ueRnti);
    ERROR_Assert(handoverInfo, "unexpectedd error");

#ifdef LTE_LIB_LOG
    {
        HandoverParticipator& hoParticipator = handoverInfo->hoParticipator;
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[send forwarding data list],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    // send SnStatusTransferItems
    for (std::list<Message*>::iterator it = forwardingMsg->begin();
        it != forwardingMsg->end(); ++it)
    {
        EpcLteAppSend_DataForwarding(
            node, interfaceIndex,
            handoverInfo->hoParticipator, bearerId, *it);
    }
}

// /**
// FUNCTION   :: Layer3LteReceiveDataForwarding
// LAYER      :: RRC
// PURPOSE    :: process after receive DataForwardin
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + forwardingMsg  : Message* : message
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveDataForwarding(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator,
    int bearerId,
    Message* forwardingMsg)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive forwarding data list],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    LteConnectionInfo* connInfo =
        Layer3LteGetConnectionInfo(
            node, interfaceIndex, hoParticipator.ueRnti);

    if (connInfo != NULL &&
        connInfo->state == LAYER3_LTE_CONNECTION_HANDOVER)
    {
        PdcpLteReceiveBuffer(
            node,
            interfaceIndex,
            hoParticipator.ueRnti,
            bearerId,
            forwardingMsg);
    }
    else
    {
        MESSAGE_Free(node, forwardingMsg);
        forwardingMsg = NULL;
    }
}

// /**
// FUNCTION   :: Layer3LteReceivePathSwitchRequestAck
// LAYER      :: RRC
// PURPOSE    :: receive path switch request ack
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void Layer3LteReceivePathSwitchRequestAck(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator,
    BOOL result)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);
        
    // cancel timer TS1WAIT_PATH_SWITCH_REQ_ACK
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_S1_WaitPathSwitchReqAck);

    BOOL isX2WaitEndMarkerRunning =
        Layer3LteCheckTimerForRrc(
            node, interfaceIndex, hoParticipator.ueRnti,
            MSG_MAC_LTE_X2_WaitEndMarker);
    if (isX2WaitEndMarkerRunning == FALSE)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "HoEnd,"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
            "isSuccess=,1",
            LTE_INVALID_RNTI.nodeId,
            LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
#endif
         // update stats
        EpcData* epc = EpcLteGetEpcData(node);
        epc->statData.numHandoversCompleted++;
    }

    // check result
    ERROR_Assert(result, "Switch DL path process should success in phase 2");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive path switch request ack],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
            "result,%s",
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex,
            result ? "TRUE" : "FALSE");
    }
#endif

    // send ue context release to src eNB
    EpcLteAppSend_UeContextRelease(
        node, interfaceIndex, hoParticipator);

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numPathSwitchRequestAckReceived++;
}

// /**
// FUNCTION   :: Layer3LteReceiveEndMarker
// LAYER      :: RRC
// PURPOSE    :: receive end marker
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveEndMarker(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive end marker],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

    LteConnectedInfo* connInfo = NULL;
    if (LteLayer2GetRnti(node, interfaceIndex) == hoParticipator.srcEnbRnti)
    {
        // get connectedInfo
        LteConnectionInfo* handoverInfo = Layer3LteGetHandoverInfo(
            node, interfaceIndex, hoParticipator.ueRnti);
        ERROR_Assert(handoverInfo, "unexpectedd error");
        connInfo = &handoverInfo->connectedInfo;
        ERROR_Assert(connInfo, "unexpectedd error");

        // Notify to PDCP
        for (MapLteRadioBearer::iterator it = 
            connInfo->radioBearers.begin();
            it != connInfo->radioBearers.end(); ++it)
        {
            PdcpLteNotifyEndMarker(
                node, interfaceIndex, hoParticipator.ueRnti, it->first);
        }

        // transfer end marker to tgt eNB
        EpcLteAppSend_EndMarker(
            node,
            interfaceIndex,
            hoParticipator.srcEnbRnti,
            hoParticipator.tgtEnbRnti,
            hoParticipator);
    }
    else 
    if (LteLayer2GetRnti(node, interfaceIndex) == hoParticipator.tgtEnbRnti)
    {
        // cancel timer TX2WAIT_END_MARKER
        Layer3LteCancelTimerForRrc(node, interfaceIndex,
            hoParticipator.ueRnti,
            MSG_MAC_LTE_X2_WaitEndMarker);

    BOOL isS1WaitPathSwitchReqAck =
        Layer3LteCheckTimerForRrc(
            node, interfaceIndex, hoParticipator.ueRnti,
            MSG_MAC_LTE_S1_WaitPathSwitchReqAck);
    if (isS1WaitPathSwitchReqAck == FALSE)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "HoEnd,"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
            "isSuccess=,1",
            LTE_INVALID_RNTI.nodeId,
            LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
#endif
        // update stats
        EpcData* epc = EpcLteGetEpcData(node);
        epc->statData.numHandoversCompleted++;
    }

        // get connectedInfo
       connInfo = Layer3LteGetConnectedInfo(
            node, interfaceIndex, hoParticipator.ueRnti);
       ERROR_Assert(connInfo, "unexpectedd error");

        // Notify to PDCP
        for (MapLteRadioBearer::iterator it = 
            connInfo->radioBearers.begin();
            it != connInfo->radioBearers.end(); ++it)
        {
            PdcpLteNotifyEndMarker(
                node, interfaceIndex, hoParticipator.ueRnti, it->first);
        }
    }
    else
    {
        ERROR_Assert(FALSE, "unexpectedd error");
    }

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numEndMarkerReceived++;
}

// /**
// FUNCTION   :: Layer3LteReceiveUeContextRelease
// LAYER      :: RRC
// PURPOSE    :: receive ue context release
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveUeContextRelease(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    // cancel timer TX2RELOCoverall
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_X2_RELOCoverall);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive ue context release],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif // LTE_LIB_LOG

    Layer3LteReleaseUeContext(node, interfaceIndex, hoParticipator);

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numUeContextReleaseReceived++;
}

// /**
// FUNCTION   :: Layer3LteReleaseUeContext
// LAYER      :: RRC
// PURPOSE    :: release ue context
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReleaseUeContext(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
    if (connectionInfo == NULL)
    {
        // already released
#ifdef LTE_LIB_LOG
        {
            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_HANDOVER","
                LTE_STRING_FORMAT_RNTI","
                "[release ue context (already released)],"
                LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
                LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
                hoParticipator.ueRnti.nodeId,
                hoParticipator.ueRnti.interfaceIndex,
                hoParticipator.srcEnbRnti.nodeId,
                hoParticipator.srcEnbRnti.interfaceIndex,
                hoParticipator.tgtEnbRnti.nodeId,
                hoParticipator.tgtEnbRnti.interfaceIndex);
        }
#endif // LTE_LIB_LOG
        return;
    }

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[release ue context],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif // LTE_LIB_LOG

    ERROR_Assert(Layer3LteGetConnectionState(
        node, interfaceIndex, hoParticipator.ueRnti) ==
        LAYER3_LTE_CONNECTION_HANDOVER, "unexpected error");
    // remove route to the RNTI lost
    Layer3LteDeleteRoute(node, interfaceIndex, hoParticipator.ueRnti);
    Layer3LteRemoveConnectionInfo(
        node, interfaceIndex, hoParticipator.ueRnti);
}

// /**
// FUNCTION   :: Layer3LteReceiveHoPreparationFailure
// LAYER      :: RRC
// PURPOSE    :: receive ho preparation failure
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoPreparationFailure(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    // station type guard
    LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex,
        LTE_STATION_TYPE_ENB, LTE_STRING_STATION_TYPE_ENB);

    // cancel timer TRELOCprep
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_RELOCprep);

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_HANDOVER","
            LTE_STRING_FORMAT_RNTI","
            "[receive handover preparation failure],"
            LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            hoParticipator.ueRnti.nodeId,
            hoParticipator.ueRnti.interfaceIndex,
            hoParticipator.srcEnbRnti.nodeId,
            hoParticipator.srcEnbRnti.interfaceIndex,
            hoParticipator.tgtEnbRnti.nodeId,
            hoParticipator.tgtEnbRnti.interfaceIndex);
    }
#endif

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        LAYER3_LTE_MEAS_CAT_HANDOVER","
        LTE_STRING_FORMAT_RNTI","
        "HoEnd,"
        LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR","
        "isSuccess=,0",
        LTE_INVALID_RNTI.nodeId,
        LTE_INVALID_RNTI.interfaceIndex,
        hoParticipator.ueRnti.nodeId,
        hoParticipator.ueRnti.interfaceIndex,
        hoParticipator.srcEnbRnti.nodeId,
        hoParticipator.srcEnbRnti.interfaceIndex,
        hoParticipator.tgtEnbRnti.nodeId,
        hoParticipator.tgtEnbRnti.interfaceIndex);
#endif

    // update stats
    EpcData* epc = EpcLteGetEpcData(node);
    epc->statData.numHandoversFailed++;
}
