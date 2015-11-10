#include "epc_lte.h"
#include "epc_lte_app.h"

// /**
// FUNCTION::       EpcLtePrintStat
// LAYER::          EPC
// PURPOSE::        Print EPC statistics
// PARAMETERS::
//    + node:       pointer to the network node
// RETURN::         NULL
// **/
static
void EpcLtePrintStat(
        Node* node)
{
    EpcData* epc = EpcLteGetEpcData(node);
    if (epc)
    {
        char buf[MAX_STRING_LENGTH] = {0};
        sprintf(buf, "Number of handover request sent = %u",
            epc->statData.numHandoverRequestSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of handover request received = %u",
            epc->statData.numHandoverRequestReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of handover request acknowledgement sent = %u",
            epc->statData.numHandoverRequestAckSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of handover request acknowledgement received = %u",
            epc->statData.numHandoverRequestAckReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of SN status transfer sent = %u",
            epc->statData.numSnStatusTransferSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of SN status transfer received = %u",
            epc->statData.numSnStatusTransferReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of path switch request sent = %u",
            epc->statData.numPathSwitchRequestSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of path switch request received = %u",
            epc->statData.numPathSwitchRequestReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of path switch request acknowledgement sent = %u",
            epc->statData.numPathSwitchRequestAckSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of path switch request acknowledgement received = %u",
            epc->statData.numPathSwitchRequestAckReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of end marker sent = %u",
            epc->statData.numEndMarkerSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of end marker received = %u",
            epc->statData.numEndMarkerReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of UE context release sent = %u",
            epc->statData.numUeContextReleaseSent);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of UE context release received = %u",
            epc->statData.numUeContextReleaseReceived);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of handovers completed = %u",
            epc->statData.numHandoversCompleted);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);

        sprintf(buf, "Number of handovers failed = %u",
            epc->statData.numHandoversFailed);
            IO_PrintStat(node,
                 "Application",
                 "LTE handover",
                 ANY_DEST,
                 -1,
                 buf);
    }
}

// /**
// FUNCTION   :: EpcLteGetEpcData
// LAYER      :: EPC
// PURPOSE    :: get EPC data
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: EpcData*    : EPC data on specified node
// **/
EpcData*
EpcLteGetEpcData(Node* node)
{
    return node->epcData;
}

// /**
// FUNCTION   :: EpcLteInit
// LAYER      :: EPC
// PURPOSE    :: initialize EPC
// PARAMETERS ::
// + node                   : Node*       : Pointer to node.
// + interfaceIndex         : int         : interface index
// + sgwmmeNodeId           : NodeAddress : node ID of SGW/MME
// + sgwmmeInterfaceIndex   : int         : interface index of SGW/MME
// + tagetNodeId      : NodeAddress : Node ID
// RETURN     :: void : NULL
// **/
void
EpcLteInit(Node* node, int interfaceIndex,
                NodeAddress sgwmmeNodeId, int sgwmmeInterfaceIndex)
{
    // already created
    if (node->epcData)
    {
        return;
    }

    node->epcData = new EpcData();

    EpcData* epc = EpcLteGetEpcData(node);

    // if sgwmmeNodeId and sgwmmeInterfaceIndex are mine, I'm a SGWMME.
    epc->type = (node->nodeId == sgwmmeNodeId
        && interfaceIndex == sgwmmeInterfaceIndex) ?
        EPC_STATION_TYPE_SGWMME : EPC_STATION_TYPE_ENB;

    // specify my own interface used by epc
    epc->outgoingInterfaceIndex = interfaceIndex;

    // set sgw/mme infomation
    epc->sgwmmeRnti.nodeId = sgwmmeNodeId;
    epc->sgwmmeRnti.interfaceIndex = sgwmmeInterfaceIndex;

    // if SGWMME, init location info
    if (epc->type == EPC_STATION_TYPE_SGWMME)
    {
        epc->locationInfo = new EpcLteLocationInfo();
    }
    else
    {
        // if eNB, add route to SGWMME as default gateway
        NodeAddress destAddr = 0;   // default route
        NodeAddress destMask = 0;   // default route
        NodeAddress nextHop =
            MAPPING_GetInterfaceAddressForInterface(
            node, epc->sgwmmeRnti.nodeId, epc->sgwmmeRnti.interfaceIndex);
        int outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
        LteAddRouteToForwardingTable(node, destAddr, destMask,
            nextHop, outgoingInterfaceIndex);
    }
}

// /**
// FUNCTION   :: EpcLteFinalize
// LAYER      :: EPC
// PURPOSE    :: Finalize EPC
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: void        : NULL
// **/
void
EpcLteFinalize(Node* node)
{
    EpcLtePrintStat(node);
    EpcData* epc = EpcLteGetEpcData(node);
    if (epc)
    {
        if (epc->type == EPC_STATION_TYPE_SGWMME)
        {
            delete epc->locationInfo;
        }
        delete epc;
    }
}

// /**
// FUNCTION   :: EpcLteProcessAttachUE
// LAYER      :: EPC
// PURPOSE    :: process for message AttachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + ueRnti    : const LteRnti& : UE
// + enbRnti   : const LteRnti& : eNodeB
// RETURN     :: void : NULL
// **/
void
EpcLteProcessAttachUE(Node* node,
                      const LteRnti& ueRnti, const LteRnti& enbRnti)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
    ERROR_Assert(epc->type == EPC_STATION_TYPE_SGWMME,
        "This function should be called on SGW/MME");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, ANY_INTERFACE,
            LTE_STRING_LAYER_TYPE_EPC,
            EPC_LTE_APP_CAT_EPC_PROCESS","
            LTE_STRING_FORMAT_RNTI","
            "[process attach ue],"
            "UE,"LTE_STRING_FORMAT_RNTI",eNB,"LTE_STRING_FORMAT_RNTI,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            ueRnti.nodeId, ueRnti.interfaceIndex,
            enbRnti.nodeId, enbRnti.interfaceIndex);
    }
#endif

    EpcLteLocationInfo* locationInfo = epc->locationInfo;

    EpcLteLocationInfo::iterator it = locationInfo->find(enbRnti);
    if (it == locationInfo->end())
    {
        // create new entry for this eNB
        EpcLteUeList emptyList;
        locationInfo->insert(EpcLteLocationInfo::value_type(
            enbRnti, emptyList));
    }
    // traverse location info tree
    for (it = locationInfo->begin(); it != locationInfo->end(); it++)
    {
        EpcLteUeList* ueList = &it->second;
        EpcLteUeList::iterator it_ue = ueList->find(ueRnti);

        if (it_ue != ueList->end())
        {
            // the UE is already registered under the eNB
            if (it->first == enbRnti)
            {
                // do nothing;
            }
            // the UE is registered under another eNB
            else
            {
                // remove
                ueList->erase(it_ue);
                // refresh routing table
                NodeAddress destAddr = 
                    MAPPING_GetInterfaceAddressForInterface(
                    node, ueRnti.nodeId, ueRnti.interfaceIndex);
                //NodeAddress destMask = MAPPING_GetSubnetMaskForInterface(
                //    node, ueRnti.nodeId, ueRnti.interfaceIndex);
                NodeAddress destMask = ANY_ADDRESS;
                LteDeleteRouteFromForwardingTable(node, destAddr, destMask);
            }
        }
        else
        {
            if (it->first == enbRnti)
            {
                // register
                ueList->insert(ueRnti);
                // refresh routing table
                NodeAddress destAddr = 
                    MAPPING_GetInterfaceAddressForInterface(
                    node, ueRnti.nodeId, ueRnti.interfaceIndex);
                //NodeAddress destMask = MAPPING_GetSubnetMaskForInterface(
                //    node, ueRnti.nodeId, ueRnti.interfaceIndex);
                NodeAddress destMask = ANY_ADDRESS;
                NodeAddress nextHop = 
                    EpcLteAppGetNodeAddressOnEpcSubnet(node, enbRnti.nodeId);
                int outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
                LteAddRouteToForwardingTable(node, destAddr, destMask,
                    nextHop, outgoingInterfaceIndex);
            }
        }
    }
}

// /**
// FUNCTION   :: EpcLteProcessDetachUE
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + ueRnti    : const LteRnti& : UE
// + enbRnti   : const LteRnti& : eNodeB
// RETURN     :: void : NULL
// **/
void
EpcLteProcessDetachUE(Node* node,
                      const LteRnti& ueRnti, const LteRnti& enbRnti)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
    ERROR_Assert(epc->type == EPC_STATION_TYPE_SGWMME,
        "This function should be called on SGW/MME");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, ANY_INTERFACE,
            LTE_STRING_LAYER_TYPE_EPC,
            EPC_LTE_APP_CAT_EPC_PROCESS","
            LTE_STRING_FORMAT_RNTI","
            "[process detach ue],"
            "UE,"LTE_STRING_FORMAT_RNTI",eNB,"LTE_STRING_FORMAT_RNTI,
            LTE_INVALID_RNTI.nodeId, LTE_INVALID_RNTI.interfaceIndex,
            ueRnti.nodeId, ueRnti.interfaceIndex,
            enbRnti.nodeId, enbRnti.interfaceIndex);
    }
#endif

    EpcLteLocationInfo* locationInfo = epc->locationInfo;

    EpcLteLocationInfo::iterator it = locationInfo->find(enbRnti);
    if (it == locationInfo->end())
    {
        // enbRnti's entry is not exist.
        // do nothing
        return;
    }
    else
    {
        EpcLteUeList* ueList = &it->second;
        EpcLteUeList::iterator it_ue = ueList->find(ueRnti);
        if (it_ue == ueList->end())
        {
            // ueRnti's entry is not exist.
            // do nothing
            return;
        }
        else
        {
            // remove
            ueList->erase(it_ue);
            // refresh routing table
            NodeAddress destAddr =
               MAPPING_GetInterfaceAddressForInterface(
               node, ueRnti.nodeId, ueRnti.interfaceIndex);
            NodeAddress destMask = ANY_ADDRESS;
            LteDeleteRouteFromForwardingTable(node, destAddr, destMask);
        }
    }
}

// /**
// FUNCTION   :: EpcLteProcessPathSwitchRequest
// LAYER      :: EPC
// PURPOSE    :: process for message DetachUE
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void
EpcLteProcessPathSwitchRequest(
    Node* node,
    const HandoverParticipator& hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
    ERROR_Assert(epc->type == EPC_STATION_TYPE_SGWMME,
        "This function should be called on SGW/MME");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, ANY_INTERFACE,
            LTE_STRING_LAYER_TYPE_EPC,
            EPC_LTE_APP_CAT_EPC_PROCESS","
            LTE_STRING_FORMAT_RNTI","
            "[process path switch request],"
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

    // switch DL path
    BOOL result = EpcLteSwitchDLPath(node, hoParticipator);

    // send PathSwitchRequestAck
    EpcLteSendPathSwitchRequestAck(node, hoParticipator, result);

    // update stats
    epc->statData.numPathSwitchRequestReceived++;
}

// /**
// FUNCTION   :: EpcLteSwitchDLPath
// LAYER      :: EPC
// PURPOSE    :: switch DL path
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: BOOL : result
// **/
BOOL
EpcLteSwitchDLPath(
    Node* node,
    const HandoverParticipator& hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
    ERROR_Assert(epc->type == EPC_STATION_TYPE_SGWMME,
        "This function should be called on SGW/MME");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, ANY_INTERFACE,
            LTE_STRING_LAYER_TYPE_EPC,
            EPC_LTE_APP_CAT_EPC_PROCESS","
            LTE_STRING_FORMAT_RNTI","
            "[switch dl path],"
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

    // result of this process
    BOOL result = TRUE;

    // send end marker to src eNB
    EpcLteAppSend_EndMarker(
        node,
        epc->sgwmmeRnti.interfaceIndex,
        epc->sgwmmeRnti,
        hoParticipator.srcEnbRnti,
        hoParticipator);

    const LteRnti& ueRnti = hoParticipator.ueRnti;
    EpcLteLocationInfo* locationInfo = epc->locationInfo;
    NodeAddress destAddr;
    NodeAddress destMask;
    NodeAddress nextHop;
    int outgoingInterfaceIndex;

    // src eNB
    EpcLteLocationInfo::iterator it;
    it = locationInfo->find(hoParticipator.srcEnbRnti);
    ERROR_Assert(it != locationInfo->end(),
        "src eNB is not registered in loationInfo");

    // UE
    EpcLteUeList* ueListAtSrcEnb = &it->second;
    EpcLteUeList::iterator it_ue = 
        ueListAtSrcEnb->find(ueRnti);
    ERROR_Assert(it_ue != ueListAtSrcEnb->end(),
        "UE is not registered in loationInfo");

    // delete from src eNB
    ueListAtSrcEnb->erase(it_ue);
    // refresh routing table
    destAddr = 
        MAPPING_GetInterfaceAddressForInterface(
        node, ueRnti.nodeId, ueRnti.interfaceIndex);
    destMask = ANY_ADDRESS;
    LteDeleteRouteFromForwardingTable(node, destAddr, destMask);

    // tgt eNB (if it doesn't exist, create new info)
    it = locationInfo->find(hoParticipator.tgtEnbRnti);
    if (it == locationInfo->end())
    {
        locationInfo->insert(EpcLteLocationInfo::value_type(
            hoParticipator.tgtEnbRnti , EpcLteUeList()));
        it = locationInfo->find(hoParticipator.tgtEnbRnti);
    }
    EpcLteUeList* ueListAtTgtEnb = &it->second;

    // register to tgt eNB
    ueListAtTgtEnb->insert(ueRnti);
    // refresh routing table
    destAddr = 
        MAPPING_GetInterfaceAddressForInterface(
        node, ueRnti.nodeId, ueRnti.interfaceIndex);
    destMask = ANY_ADDRESS;
    nextHop = EpcLteAppGetNodeAddressOnEpcSubnet(
        node, hoParticipator.tgtEnbRnti.nodeId);
    outgoingInterfaceIndex = epc->outgoingInterfaceIndex;
    LteAddRouteToForwardingTable(node, destAddr, destMask,
        nextHop, outgoingInterfaceIndex);

    return result;
}

// /**
// FUNCTION   :: EpcLteSendPathSwitchRequestAck
// LAYER      :: EPC
// PURPOSE    :: send path switch request ack
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void
EpcLteSendPathSwitchRequestAck(
    Node* node,
    const HandoverParticipator& hoParticipator,
    BOOL result)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
    ERROR_Assert(epc->type == EPC_STATION_TYPE_SGWMME,
        "This function should be called on SGW/MME");

#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, ANY_INTERFACE,
            LTE_STRING_LAYER_TYPE_EPC,
            EPC_LTE_APP_CAT_EPC_PROCESS","
            LTE_STRING_FORMAT_RNTI","
            "[send path switch request ack],"
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

    // send EPC message PathSwitchRequestAck to tgt eNB
    EpcLteAppSend_PathSwitchRequestAck(
        node, epc->sgwmmeRnti.interfaceIndex, hoParticipator, result);
}

