#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"

#include "lte_common.h"
#include "layer2_lte.h"
#include "layer2_lte_mac.h"
#include "layer3_lte.h"
#include "phy_lte_establishment.h"
#include "layer2_lte_establishment.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#include "epc_lte_common.h"
#include "epc_lte_app.h"

////////////////////////////////////////////////////////////////////////////
// define static function
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteTransmitRandomAccessPreamble
// LAYER      :: MAC
// PURPOSE    :: Transmit Random Access Preamble
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: void : NULL
// **/
static void MacLteTransmitRandomAccessPreamble(
    Node* node, int interfaceIndex);


////////////////////////////////////////////////////////////////////////////
// eNB - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyReceivedRaPreamble
// LAYER      :: MAC
// PURPOSE    :: Notify RA Preamble from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : const LteRnti& ueRnti : UE's RNTI
// RETURN     :: void : NULL
// **/
void MacLteNotifyReceivedRaPreamble(
    Node* node, int interfaceIndex, const LteRnti& ueRnti,
    BOOL isHandingoverRa)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_ENB,
        "This function should be called from only eNB.");
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Rx %s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE,
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif

    LteMacData* macData = LteLayer2GetLteMacData(node, interfaceIndex);

    // if H.O. flag included in RA is different from the state which
    // this eNB is managing connection info, this RA will be ignored.
    LteConnectionInfo* handoverInfo =
        Layer3LteGetHandoverInfo(node, interfaceIndex, ueRnti);
    if (handoverInfo != NULL && isHandingoverRa == FALSE)
    {
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Rx %s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE" is ignored",
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif // LTE_LIB_LOG
        return;
    }

    macData->statData.numberOfRecievingRaPreamble++;

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Tx %s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_GRANT,
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif // LTE_LIB_LOG
    PhyLteRaGrantTransmissionIndication(node, phyIndex, ueRnti);
    macData->statData.numberOfSendingRaGrant++;

    RrcLteNotifyNetworkEntryFromUE(node, interfaceIndex, ueRnti);
}

// /**
// FUNCTION   :: RrcLteNotifyRrcConnectionSetupComplete
// LAYER      :: RRC
// PURPOSE    :: Nortify RRCConnectionSetupComplete from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI (Destination)
// + ueRnti         : LteRnti : UE's RNTI  (Source)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRrcConnectionSetupComplete(
    Node* node, int interfaceIndex,
    const LteRnti& enbRnti, const LteRnti& ueRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_ENB,
        "This function should be called from only eNB.");
    const LteRnti& ownRnti = LteLayer2GetRnti(node, interfaceIndex);

    if (ownRnti != enbRnti)
    {
        // discard for other eNB's RrcConnectionSetupComplete
        return;
    }

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    layer3Data->statData.numRrcConnectionEstablishment++;

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,RRC_CONNECTED,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif

    // change connection state (waiting -> connected)
    Layer3LteChangeConnectionState(node, interfaceIndex, ueRnti,
        LAYER3_LTE_CONNECTION_CONNECTED);

    // add route
    Layer3LteAddRoute(node, interfaceIndex, ueRnti);

    // send EPC message AttachUE to SGW/MME
    EpcLteAppSend_AttachUE(node, interfaceIndex,
        ueRnti, LteLayer2GetRnti(node, interfaceIndex));
}



// /**
// FUNCTION   :: RrcLteNotifyRrcConnectionReconfComplete
// LAYER      :: RRC
// PURPOSE    :: Nortify RRCConnectionReconfComplete from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI (Destination)
// + ueRnti         : LteRnti : UE's RNTI  (Source)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRrcConnectionReconfComplete(
    Node* node, int interfaceIndex,
    const LteRnti& enbRnti, const LteRnti& ueRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_ENB,
        "This function should be called from only eNB.");
    const LteRnti& ownRnti = LteLayer2GetRnti(node, interfaceIndex);

    if (ownRnti != enbRnti)
    {
        // discard for other eNB's RrcConnectionSetupComplete
        return;
    }

    // stat
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    layer3Data->statData.numRrcConnectionEstablishment++;

    // before restore, get hoParticipator by handoverInfo
    HandoverParticipator hoParticipator = Layer3LteGetHandoverParticipator(
        node, interfaceIndex, ueRnti);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,RRC_CONNECTED_RECONF,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif

    // Restore to ConnectedInfo (handover -> connected)
    Layer3LteChangeConnectionState(
        node, interfaceIndex, ueRnti, LAYER3_LTE_CONNECTION_CONNECTED);

    // add route
    Layer3LteAddRoute(node, interfaceIndex, ueRnti);

    // send EPC message PathSwitchRequest to SGW/MME
    EpcLteAppSend_PathSwitchRequest(node, interfaceIndex, hoParticipator);

    // cancel timer TWAIT_ATTACH_UE_BY_HO
    Layer3LteCancelTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_WaitAttachUeByHo);
        
    // set timer TX2WAIT_END_MARKER
    Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_X2_WaitEndMarker,
        RRC_LTE_DEFAULT_X2_WAIT_END_MARKER_TIME);
        
    // set timer TS1WAIT_PATH_SWITCH_REQ_ACK
    Layer3LteSetTimerForRrc(node, interfaceIndex, hoParticipator.ueRnti,
        MSG_MAC_LTE_S1_WaitPathSwitchReqAck,
        RRC_LTE_DEFAULT_S1_WAIT_PATH_SWITCH_REQ_ACK_TIME);
}



////////////////////////////////////////////////////////////////////////////
// eNB - API for MAC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: RrcLteNotifyNetworkEntryFromUE
// LAYER      :: RRC
// PURPOSE    :: Notify network entry from UE
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + ueRnti         : LteRnti : UE's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteNotifyNetworkEntryFromUE(
    Node* node, int interfaceIndex, const LteRnti& ueRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_ENB,
        "This function should be called from only eNB.");

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,Entry,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
        LTE_STRING_STATION_TYPE_UE,
        ueRnti.nodeId, ueRnti.interfaceIndex);
#endif

    LteConnectionInfo* connectionInfo =
        Layer3LteGetConnectionInfo(node, interfaceIndex, ueRnti);
    if (connectionInfo)
    {
        ERROR_Assert(connectionInfo->state == LAYER3_LTE_CONNECTION_WAITING
            || connectionInfo->state == LAYER3_LTE_CONNECTION_HANDOVER,
            "unexpected error");
#ifdef LTE_LIB_LOG
        lte::LteLog::WarnFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,already exist in WaitingConnectedList,%s=,"
            LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
            LTE_STRING_STATION_TYPE_ENB,
            ueRnti.nodeId, ueRnti.interfaceIndex);
#endif
    }
    else
    {
        // Add to WaitingList
        Layer3LteAddConnectionInfo(node, interfaceIndex, ueRnti,
            LAYER3_LTE_CONNECTION_WAITING);
    }
}

// /**
// FUNCTION   :: MacLteProcessRaGrantWaitingTimerExpired
// LAYER      :: MAC
// PURPOSE    :: Process RA Grant waiting Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessRaGrantWaitingTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

    const LteRnti& enbRnti =
        Layer3LteGetRandomAccessingEnb(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Cannot Rx %s,t-RaWaitingTimer expired",
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_GRANT);
#endif

    // notify to PHY
    PhyLteRaGrantWaitingTimerTimeoutNotification(node, interfaceIndex);

    LteMacData* macData = LteLayer2GetLteMacData(node, interfaceIndex);
    LteMacConfig* macConfig =
        PhyLteGetMacConfigInEstablishment(node, interfaceIndex);
    ERROR_Assert(macConfig != NULL, "");

    if (macData->preambleTransmissionCounter > macConfig->raPreambleTransMax)
    {
        // Failed
        Layer3LteRemoveConnectionInfo(node, interfaceIndex, enbRnti);
        RrcLteNotifyRandomAccessResults(
            node, interfaceIndex, enbRnti, FALSE);
        macData->preambleTransmissionCounter = 1;
        MacLteSetState(node, interfaceIndex, MAC_LTE_IDEL);
    }
    else
    {
        // Retransmit
        clocktype nRand = RANDOM_nrand(macData->seed);
        clocktype backoffTime = (nRand % macConfig->raBackoffTime);

        MacLteSetTimerForMac(
            node, interfaceIndex,
            MSG_MAC_LTE_RaBackoffWaitingTimerExpired, backoffTime);
        MacLteSetState(node, interfaceIndex, MAC_LTE_RA_BACKOFF_WAITING);
    }
}

// /**
// FUNCTION   :: MacLteProcessRaBackoffWaitingTimerExpired
// LAYER      :: MAC
// PURPOSE    :: Process RA Backoff waiting Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessRaBackoffWaitingTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

    MacLteTransmitRandomAccessPreamble(node, interfaceIndex);
}

////////////////////////////////////////////////////////////////////////////
// UE - API for RRC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteStartRandomAccess
// LAYER      :: MAC
// PURPOSE    :: Random Access Start Point
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: void : NULL
// **/
void MacLteStartRandomAccess(Node* node, int interfaceIndex)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

#ifdef LTE_LIB_LOG
    const LteRnti& enbRnti =
        Layer3LteGetRandomAccessingEnb(node, interfaceIndex);
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_PROCESS_TYPE_START,
        LTE_STRING_STATION_TYPE_ENB,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    MacLteTransmitRandomAccessPreamble(node, interfaceIndex);
    LteMacData* macData = LteLayer2GetLteMacData(node, interfaceIndex);
    macData->statData.numberOfEstablishment++;
}



// RRC ConnectedNotification
// /**
// FUNCTION   :: MacLteNotifyRrcConnected
// LAYER      :: MAC
// PURPOSE    :: Notify RRC_CONNECTED from UE's RRC Layer
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// + handingover    : BOOL  : whether handingover
// RETURN     :: void : NULL
// **/
void MacLteNotifyRrcConnected(Node* node, int interfaceIndex,
                              BOOL handingover)
{
    // Do nothing at Phase 1
}



////////////////////////////////////////////////////////////////////////////
// UE - API for MAC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: RrcLteNotifyRandomAccessResults
// LAYER      :: RRC
// PURPOSE    :: Notify Random Access Results from MAC Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI
// + isSuccess      : BOOL    : TRUE:Success / FALSE:Failure
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRandomAccessResults(
    Node* node, int interfaceIndex, const LteRnti& enbRnti, BOOL isSuccess)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

#ifdef LTE_LIB_LOG
    if (isSuccess == TRUE)
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_RESULT_TYPE_SUCCESS,
            LTE_STRING_STATION_TYPE_ENB,
            enbRnti.nodeId, enbRnti.interfaceIndex);
    }
    else
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_RESULT_TYPE_FAILURE,
            LTE_STRING_STATION_TYPE_ENB,
            enbRnti.nodeId, enbRnti.interfaceIndex);
    }
#endif

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    LteConnectionInfo* handoverInfo =
        Layer3LteGetHandoverInfo(node, interfaceIndex);

    if (isSuccess == TRUE)
    {
        if (handoverInfo)
        {
            // wait RRC_CONNECTED_RECONF
            clocktype waitRrcConnectedReconfTime =
                layer3Data->waitRrcConnectedReconfTime;
            Layer3LteSetTimerForRrc(
                node, interfaceIndex, enbRnti,
                MSG_RRC_LTE_WaitRrcConnectedReconfTimerExpired,
                waitRrcConnectedReconfTime);
        }
        else
        {
            // wait RRC_CONNECTED
            clocktype waitRrcConnectedTime =
                layer3Data->waitRrcConnectedTime;
            Layer3LteSetTimerForRrc(
                node, interfaceIndex, enbRnti,
                MSG_RRC_LTE_WaitRrcConnectedTimerExpired,
                waitRrcConnectedTime);
        }
    }
    else
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_PROCESS_TYPE_RESTART);
#endif // LTE_LIB_LOG
        if (handoverInfo)
        {
            // notify failure to RRC
            Layer3LteNotifyFailureRAToTargetEnbOnHandover(
                node, interfaceIndex);
        }
        else
        {
            layer3Data->statData.averageRetryRrcConnectionEstablishment++;
            PhyLteStartCellSearch(node, interfaceIndex);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
// UE - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyReceivedRaGrant
// LAYER      :: MAC
// PURPOSE    :: Notify RA Grant from UE's PHY Layer
// PARAMETERS ::
// + node           : Node*      : Pointer to node.
// + interfaceIndex : int        : Interface Index
// + raGrant        : LteRaGrant : RA Grant Structure
// RETURN     :: void : NULL
// **/
void MacLteNotifyReceivedRaGrant(
    Node* node, int interfaceIndex, const LteRaGrant& raGrant)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

    LteRnti enbRnti;

    LteMacData* macData =
        LteLayer2GetLteMacData(node, interfaceIndex);

    macData->statData.numberOfRecievingRaGrant++;

    Message* grantWaitingTimer =
        MacLteGetTimerForMac(
            node, interfaceIndex, MSG_MAC_LTE_RaGrantWaitingTimerExpired);

    if (macData->macState == MAC_LTE_RA_GRANT_WAITING)
    {
        enbRnti =
            Layer3LteGetRandomAccessingEnb(node, interfaceIndex);
        MacLteCancelTimerForMac(node, interfaceIndex,
            MSG_MAC_LTE_RaGrantWaitingTimerExpired, grantWaitingTimer);
    }
    else if (macData->macState == MAC_LTE_RA_BACKOFF_WAITING)
    {
        enbRnti =
            Layer3LteGetRandomAccessingEnb(node, interfaceIndex);
        LteMapMessage::iterator itr =
            macData->mapMacTimer.find(
                MSG_MAC_LTE_RaBackoffWaitingTimerExpired);
        ERROR_Assert(itr != macData->mapMacTimer.end(),
            "t-RaBackoffWaitingTimer is not set.");
        MacLteCancelTimerForMac(node, interfaceIndex,
            MSG_MAC_LTE_RaBackoffWaitingTimerExpired, (*itr).second);
    }
    else if (macData->macState == MAC_LTE_DEFAULT_STATUS)
    {
        enbRnti =
            Layer3LteGetConnectedEnb(node, interfaceIndex);
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf,
            "Do not support receiving RA Grant in this MAC State(%d)",
            macData->macState);
        ERROR_ReportError(errBuf);
    }
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Rx %s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_GRANT,
        LTE_STRING_STATION_TYPE_ENB,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    if (macData->macState == MAC_LTE_RA_GRANT_WAITING ||
       macData->macState == MAC_LTE_RA_BACKOFF_WAITING)
    {
        RrcLteNotifyRandomAccessResults(
            node, interfaceIndex, enbRnti, TRUE);

        MacLteSetNextTtiTimer(node, interfaceIndex);

        MacLteSetState(node, interfaceIndex, MAC_LTE_DEFAULT_STATUS);
    }
}

// /**
// FUNCTION   :: MacLteNotifyReceivedRaGrant
// LAYER      :: MAC
// PURPOSE    :: Notify RA Grant from UE's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + isSuccess      : BOOL    : RA Grant Structure
// + enbRnti        : LteRnti : eNB's RNTI (if isSuccess == FALSE,
//                              do not need enbRnti)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyCellSelectionResults(
    Node* node, int interfaceIndex, BOOL isSuccess, const LteRnti& enbRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    if (isSuccess == TRUE)
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_RESULT_TYPE_SUCCESS,
            LTE_STRING_STATION_TYPE_ENB,
            enbRnti.nodeId, enbRnti.interfaceIndex);
    }
    else
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_RESULT_TYPE_FAILURE);
    }
#endif

    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    if (isSuccess == TRUE)
    {
        LteConnectionInfo* connectionInfo =
            Layer3LteGetConnectionInfo(node, interfaceIndex, enbRnti);
        if (connectionInfo)
        {
            ERROR_Assert(connectionInfo->state ==
                LAYER3_LTE_CONNECTION_WAITING, "unexpected error");
#ifdef LTE_LIB_LOG
            lte::LteLog::WarnFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                "%s,already exist in WaitingConnectedList,%s=,"
                LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT,
                LTE_STRING_STATION_TYPE_ENB,
                enbRnti.nodeId, enbRnti.interfaceIndex);
#endif
            // do nothing
        }
        else
        {
            Layer3LteAddConnectionInfo(node, interfaceIndex, enbRnti,
                LAYER3_LTE_CONNECTION_WAITING);
        }

        // start RA
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_PROCESS_TYPE_START,
            LTE_STRING_STATION_TYPE_ENB,
            enbRnti.nodeId, enbRnti.interfaceIndex);
#endif
        MacLteStartRandomAccess(node, interfaceIndex);
    }
    else
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_PROCESS_TYPE_RESTART);
#endif
        layer3Data->statData.averageRetryRrcConnectionEstablishment++;
        PhyLteStartCellSearch(node, phyIndex);
    }
}

// RRC Connection Setup Complete notification
// /**
// FUNCTION   :: RrcLteNotifyDetectingBetterCell
// LAYER      :: RRC
// PURPOSE    :: Notify Detecting Better Cell from PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteNotifyDetectingBetterCell(
    Node* node, int interfaceIndex, const LteRnti& enbRnti)
{
#ifdef LTE_LIB_USE_ONOFF
#ifdef LTE_LIB_LOG
    const LteRnti& curRnti =
        Layer3LteGetConnectedEnb(node, interfaceIndex);
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,Notify,CurrentENB=,"LTE_STRING_FORMAT_RNTI","
        "FoundENB=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_BETTER_CELL,
        curRnti.nodeId, curRnti.interfaceIndex,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    Layer3LtePowerOff(node, interfaceIndex);
    Layer3LtePowerOn(node, interfaceIndex);
#endif
}

////////////////////////////////////////////////////////////////////////////
// static function
////////////////////////////////////////////////////////////////////////////
static void MacLteTransmitRandomAccessPreamble(
    Node* node, int interfaceIndex)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    const LteRnti& enbRnti =
        Layer3LteGetRandomAccessingEnb(node, interfaceIndex);
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_MAC,
        "%s,Tx %s,%s=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE,
        LTE_STRING_STATION_TYPE_ENB,
        enbRnti.nodeId, enbRnti.interfaceIndex);
#endif

    LteMacData* macData = LteLayer2GetLteMacData(node, interfaceIndex);
    LteMacConfig* macConfig =
        PhyLteGetMacConfigInEstablishment(node, phyIndex);
    ERROR_Assert(macConfig != NULL, "");

    double receivedTargetPower =
        macConfig->raPreambleInitialReceivedTargetPower +
        MAC_LTE_DEFAULT_RA_DELTA_PREAMBLE +
        (macData->preambleTransmissionCounter - 1) *
        macConfig->raPowerRampingStep;

    int preambleIndex =
        RANDOM_nrand(macData->seed) % MAC_LTE_RA_PREAMBLE_INDEX_MAX;

    LteRaPreamble raPreamble;
    raPreamble.preambleReceivedTargetPower = receivedTargetPower;
    raPreamble.raPRACHMaskIndex = MAC_LTE_DEFAULT_RA_PRACH_MASK_INDEX;
    raPreamble.raPreambleIndex = preambleIndex;
    raPreamble.ueRnti = LteLayer2GetRnti(node, interfaceIndex);

    // Indicate RA Preamble transmission
    PhyLteRaPreambleTransmissionIndication(
        node, phyIndex,
        LTE_LAYER2_DEFAULT_USE_SPECIFIED_DELAY,
        LTE_LAYER2_DEFAULT_DELAY_UNTIL_AIRBORN,
        &raPreamble);
    macData->statData.numberOfSendingRaPreamble++;

    macData->preambleTransmissionCounter++;
    MacLteSetState(node, interfaceIndex, MAC_LTE_RA_GRANT_WAITING);

    // Set t-RaGrantWaiting
    clocktype raGrantWaitingTime =
        macConfig->raResponseWindowSize * MILLI_SECOND; // PHY_DEBBUG
    MacLteSetTimerForMac(node, interfaceIndex,
        MSG_MAC_LTE_RaGrantWaitingTimerExpired, raGrantWaitingTime);
}



// /**
// FUNCTION   :: RrcLteRandomAccessForHandoverExecution
// LAYER      :: RRC
// PURPOSE    :: random access for handover execution
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : target eNB's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteRandomAccessForHandoverExecution(
    Node* node, int interfaceIndex, const LteRnti& enbRnti)
{
    LteStationType stationType =
        LteLayer2GetStationType(node, interfaceIndex);
    ERROR_Assert(stationType == LTE_STATION_TYPE_UE,
        "This function should be called from only UE.");

    LteConnectionInfo* handoverInfo =
        Layer3LteGetHandoverInfo(node, interfaceIndex, enbRnti);
    ERROR_Assert(handoverInfo, "unexpected error");

    // start RA
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_PROCESS_TYPE_START,
            LTE_STRING_STATION_TYPE_ENB,
            enbRnti.nodeId, enbRnti.interfaceIndex);
#endif // LTE_LIB_LOG
    MacLteStartRandomAccess(node, interfaceIndex);
}
