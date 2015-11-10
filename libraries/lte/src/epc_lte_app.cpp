#include "api.h"
#include "partition.h"
#include "app_util.h"

#include "epc_lte_common.h"
#include "log_lte.h"
#include "epc_lte.h"
#include "epc_lte_app.h"

// /**
// FUNCTION   :: EpcLteAppProcessEvent
// LAYER      :: EPC
// PURPOSE    :: process event for EPC
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void
EpcLteAppProcessEvent(Node *node, Message *msg)
{
    // get container
    EpcLteAppMessageContainer* container = 
        (EpcLteAppMessageContainer*)MESSAGE_ReturnInfo(
            msg, INFO_TYPE_LteEpcAppContainer);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, ANY_INTERFACE,
        LTE_STRING_LAYER_TYPE_EPC,
        EPC_LTE_APP_CAT_EPC_APP","
        LTE_STRING_FORMAT_RNTI","
        "[receive],"
        "src,"LTE_STRING_FORMAT_RNTI",dst,"LTE_STRING_FORMAT_RNTI","
        "type,%d",
        container->src.nodeId, container->src.interfaceIndex,
        container->src.nodeId, container->src.interfaceIndex,
        container->dst.nodeId, container->dst.interfaceIndex,
        container->type);
#endif

    // switch process by message type
    switch (container->type)
    {
    case EPC_MESSAGE_TYPE_ATTACH_UE:
    {
        EpcData* epc = EpcLteGetEpcData(node);
        ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
        ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
            "received unexpected EPC message");
        EpcLteAppMessageArgument_AttachUE arg;
        memcpy(&arg, container->value,
            sizeof(EpcLteAppMessageArgument_AttachUE));
        EpcLteProcessAttachUE(node, arg.ueRnti, arg.enbRnti);
        break;
    }
    case EPC_MESSAGE_TYPE_DETACH_UE:
    {
        EpcData* epc = EpcLteGetEpcData(node);
        ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");
        ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
            "received unexpected EPC message");
        EpcLteAppMessageArgument_DetachUE arg;
        memcpy(&arg, container->value,
            sizeof(EpcLteAppMessageArgument_DetachUE));
        EpcLteProcessDetachUE(node, arg.ueRnti, arg.enbRnti);
        break;
    }
    case EPC_MESSAGE_TYPE_HANDOVER_REQUEST:
    {
        EpcLteAppMessageArgument_HoReq* arg =
            (EpcLteAppMessageArgument_HoReq*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.tgtEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveHoReq(node,
            arg->hoParticipator.tgtEnbRnti.interfaceIndex,
            arg->hoParticipator);
        break;
    }
    case EPC_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE:
    {
        EpcLteAppMessageArgument_HoReqAck* arg =
            (EpcLteAppMessageArgument_HoReqAck*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.srcEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveHoReqAck(node,
            arg->hoParticipator.srcEnbRnti.interfaceIndex,
            arg->hoParticipator,
            arg->reconf);
        break;
    }
    case EPC_MESSAGE_TYPE_SN_STATUS_TRANSFER:
    {
        EpcLteAppMessageArgument_SnStatusTransfer* arg =
            (EpcLteAppMessageArgument_SnStatusTransfer*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.tgtEnbRnti.nodeId,
            "received unexpected EPC message");
        // restore
        std::map<int, PdcpLteSnStatusTransferItem> snStatusMap;
        for (int i = 0; i < arg->itemNum; i++)
        {
            snStatusMap.insert(std::map<int, PdcpLteSnStatusTransferItem>
                ::value_type(arg->items[i].bearerId,
                arg->items[i].snStatusTransferItem));
        }
        Layer3LteReceiveSnStatusTransfer(node,
            arg->hoParticipator.tgtEnbRnti.interfaceIndex,
            arg->hoParticipator,
            snStatusMap);
        break;
    }
    case EPC_MESSAGE_TYPE_DATA_FORWARDING:
    {
        EpcLteAppMessageArgument_DataForwarding* arg =
            (EpcLteAppMessageArgument_DataForwarding*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.tgtEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveDataForwarding(node,
            arg->hoParticipator.tgtEnbRnti.interfaceIndex,
            arg->hoParticipator,
            arg->bearerId,
            arg->message);
        break;
    }
    case EPC_MESSAGE_TYPE_PATH_SWITCH_REQUEST:
    {
        EpcLteAppMessageArgument_PathSwitchRequest* arg =
            (EpcLteAppMessageArgument_PathSwitchRequest*)container->value;
        EpcData* epc = EpcLteGetEpcData(node);
        ERROR_Assert(node->nodeId == epc->sgwmmeRnti.nodeId,
            "received unexpected EPC message");
        EpcLteProcessPathSwitchRequest(node, arg->hoParticipator);
        break;
    }
    case EPC_MESSAGE_TYPE_PATH_SWITCH_REQUEST_ACK:
    {
        EpcLteAppMessageArgument_PathSwitchRequestAck* arg =
           (EpcLteAppMessageArgument_PathSwitchRequestAck*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.tgtEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceivePathSwitchRequestAck(
            node, arg->hoParticipator.tgtEnbRnti.interfaceIndex,
            arg->hoParticipator, arg->result);
        break;
    }
    case EPC_MESSAGE_TYPE_END_MARKER:
    {
        EpcLteAppMessageArgument_EndMarker* arg =
            (EpcLteAppMessageArgument_EndMarker*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.srcEnbRnti.nodeId
            || node->nodeId == arg->hoParticipator.tgtEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveEndMarker(
            node, arg->hoParticipator.srcEnbRnti.interfaceIndex,
            arg->hoParticipator);
        break;
    }
    case EPC_MESSAGE_TYPE_UE_CONTEXT_RELEASE:
    {
        EpcLteAppMessageArgument_UeContextRelease* arg =
            (EpcLteAppMessageArgument_UeContextRelease*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.srcEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveUeContextRelease(
            node, arg->hoParticipator.srcEnbRnti.interfaceIndex,
            arg->hoParticipator);
        break;
    }
    case EPC_MESSAGE_TYPE_HO_PREPARATION_FAILURE:
    {
        EpcLteAppMessageArgument_HoPreparationFailure* arg =
            (EpcLteAppMessageArgument_HoPreparationFailure*)container->value;
        ERROR_Assert(node->nodeId == arg->hoParticipator.srcEnbRnti.nodeId,
            "received unexpected EPC message");
        Layer3LteReceiveHoPreparationFailure(
            node, arg->hoParticipator.srcEnbRnti.interfaceIndex,
            arg->hoParticipator);
        break;
    }
    default:
    {
        ERROR_Assert(false, "received EPC message of unknown type");
        break;
    }
    }

    // delete message
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: EpcLteAppSend
// LAYER      :: EPC
// PURPOSE    :: common sender called by sender of each API
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const LteRnti& : source of message
// + dst              : const LteRnti& : dest of message
// + type             : EpcMessageType : message type
// + payloadSize      : int            : payload size
// + payload          : char*          : pointer to payload
// + virtualPacketSize: int            : virtual size of packet
//                                        when -1 specified, actual size used
// RETURN     :: void : NULL
// **/
void
EpcLteAppSend(Node* node,
              int interfaceIndex,
              const LteRnti& src,
              const LteRnti& dst,
              EpcMessageType type,
              int payloadSize,
              char *payload,
              int virtualPacketSize)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    // pack EpcMessageType and payload to container
    int containerSize = sizeof(EpcLteAppMessageContainer) + payloadSize;
    EpcLteAppMessageContainer* container = 
        (EpcLteAppMessageContainer*)malloc(containerSize);
    container->src = src;
    container->dst = dst;
    container->type = type;
    container->length = payloadSize;
    if (payloadSize > 0)
    {
        memcpy(container->value, payload, payloadSize);
    }

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, ANY_INTERFACE,
        LTE_STRING_LAYER_TYPE_EPC,
        EPC_LTE_APP_CAT_EPC_APP","
        LTE_STRING_FORMAT_RNTI","
        "[send],"
        "src,"LTE_STRING_FORMAT_RNTI",dst,"LTE_STRING_FORMAT_RNTI","
        "type,%d,payloadSize,%d,virtualPacketSize,%d",
        container->dst.nodeId, container->dst.interfaceIndex,
        container->src.nodeId, container->src.interfaceIndex,
        container->dst.nodeId, container->dst.interfaceIndex,
        type,payloadSize,virtualPacketSize);
#endif

    // resolve source and destination address for EPC subnet
    NodeAddress sourceAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
        node, node->nodeId);
    NodeAddress destAddr = EpcLteAppGetNodeAddressOnEpcSubnet(
        node, dst.nodeId);

    EpcLteAppCommitToUdp(
        node,
        APP_EPC_LTE,
        sourceAddr,
        APP_EPC_LTE,
        destAddr,
        epc->outgoingInterfaceIndex,
        (char*)container,
        containerSize,
        APP_DEFAULT_TOS,
        EPC_LTE_APP_DELAY,
        TRACE_EPC_LTE,
        virtualPacketSize);

    // delete container
    free(container);

}

// /**
// FUNCTION   :: EpcLteAppCommitToUdp
// LAYER      :: EPC
// PURPOSE    :: send to UDP layer
// PARAMETERS ::
// RETURN     :: void : NULL
// **/
void
EpcLteAppCommitToUdp(
    Node *node,
    AppType appType,
    NodeAddress sourceAddr,
    short sourcePort,
    NodeAddress destAddr,
    int outgoingInterface,
    char *payload,
    int payloadSize,
    TosType priority,
    clocktype delay,
    TraceProtocolType traceProtocol,
    int virtualPacketSize)
{
    Message *msg;
    AppToUdpSend *info;
    ActionData acnData;

    msg = MESSAGE_Alloc(
              node,
              TRANSPORT_LAYER,
              TransportProtocol_UDP,
              MSG_TRANSPORT_FromAppSend);

    if (virtualPacketSize < 0)
    {
        virtualPacketSize = payloadSize;
    }

    // allocate virtual packet size
    MESSAGE_PacketAlloc(node, msg, 0, traceProtocol);
    MESSAGE_AddVirtualPayload(node, msg, virtualPacketSize);

    // empty payload
    //memcpy(MESSAGE_ReturnPacket(msg), payload, payloadSize);

    MESSAGE_InfoAlloc(node, msg, sizeof(AppToUdpSend));
    info = (AppToUdpSend*)MESSAGE_ReturnInfo(msg);

    SetIPv4AddressInfo(&info->sourceAddr, sourceAddr);
    info->sourcePort = sourcePort;

    SetIPv4AddressInfo(&info->destAddr, destAddr);
    info->destPort = (short) appType;

    info->priority = priority;
    info->outgoingInterface = outgoingInterface;

    info->ttl = IPDEFTTL;

    // set container
    char* infoContainer = MESSAGE_AppendInfo(
        node, msg, payloadSize, INFO_TYPE_LteEpcAppContainer);
    memcpy(infoContainer, payload, payloadSize);

    //Trace Information
    acnData.actionType = SEND;
    acnData.actionComment = NO_COMMENT;
    TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
        PACKET_OUT, &acnData);

    MESSAGE_Send(node, msg, delay);
}

// /**
// FUNCTION   :: EpcLteAppSend_AttachUE
// LAYER      :: EPC
// PURPOSE    :: API to send message AttachUE
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const LteRnti& : UE
// + dst              : const LteRnti& : eNB
// RETURN     :: void : NULL
// **/
void
EpcLteAppSend_AttachUE(Node* node,
                       int interfaceIndex,
                       const LteRnti& ueRnti,
                       const LteRnti& enbRnti)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    // set argument
    EpcLteAppMessageArgument_AttachUE arg;
    arg.ueRnti = ueRnti;
    arg.enbRnti = enbRnti;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  enbRnti,
                  epc->sgwmmeRnti,
                  EPC_MESSAGE_TYPE_ATTACH_UE,
                  sizeof(EpcLteAppMessageArgument_AttachUE),
                  (char*)&arg);
}

// /**
// FUNCTION   :: EpcLteAppSend_DetachUE
// LAYER      :: EPC
// PURPOSE    :: API to send message DetachUE
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const LteRnti& : UE
// + dst              : const LteRnti& : eNB
// RETURN     :: void : NULL
// **/
void
EpcLteAppSend_DetachUE(Node* node,
                       int interfaceIndex,
                       const LteRnti& ueRnti,
                       const LteRnti& enbRnti)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    // set argument
    EpcLteAppMessageArgument_DetachUE arg;
    arg.ueRnti = ueRnti;
    arg.enbRnti = enbRnti;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  enbRnti,
                  epc->sgwmmeRnti,
                  EPC_MESSAGE_TYPE_DETACH_UE,
                  sizeof(EpcLteAppMessageArgument_DetachUE),
                  (char*)&arg);
}

// /**
// FUNCTION   :: EpcLteAppSend_HandoverRequest
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequest
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_HandoverRequest(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_HoReq arg;

    // set participators
    arg.hoParticipator = hoParticipator;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  arg.hoParticipator.srcEnbRnti,
                  arg.hoParticipator.tgtEnbRnti,
                  EPC_MESSAGE_TYPE_HANDOVER_REQUEST,
                  sizeof(EpcLteAppMessageArgument_HoReq),
                  (char*)&arg);

    // update stats
    epc->statData.numHandoverRequestSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_HandoverRequestAck
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& : reconfiguration
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_HandoverRequestAck(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_HoReqAck arg;
    // set participators
    arg.hoParticipator = hoParticipator;
    // set rrc config
    arg.reconf = reconf;  // copy

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  arg.hoParticipator.tgtEnbRnti,
                  arg.hoParticipator.srcEnbRnti,
                  EPC_MESSAGE_TYPE_HANDOVER_REQUEST_ACKNOWLEDGE,
                  sizeof(EpcLteAppMessageArgument_HoReqAck),
                  (char*)&arg);

    // update stats
    epc->statData.numHandoverRequestAckSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_SnStatusTransfer
// LAYER      :: EPC
// PURPOSE    :: API to send message SnStatusTransfer
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// + snStatusTransferItem : std::map<int, PdcpLteSnStatusTransferItem>& : 
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_SnStatusTransfer(
    Node *node,
    UInt32 interfaceIndex,
    const HandoverParticipator& hoParticipator,
    std::map<int, PdcpLteSnStatusTransferItem>& snStatusTransferItem)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    int msgSize = sizeof(EpcLteAppMessageArgument_SnStatusTransfer) +
        snStatusTransferItem.size() * sizeof(EpcLteAppPairBearerIdSnStatus);
    EpcLteAppMessageArgument_SnStatusTransfer* arg =
        (EpcLteAppMessageArgument_SnStatusTransfer*)malloc(msgSize);

    // store
    arg->hoParticipator = hoParticipator;
    arg->itemNum = snStatusTransferItem.size();
    int itemIndex = 0;
    for (std::map<int, PdcpLteSnStatusTransferItem>::iterator it =
        snStatusTransferItem.begin(); it != snStatusTransferItem.end();
        ++it)
    {
        arg->items[itemIndex].bearerId = it->first;
        memcpy(&arg->items[itemIndex].snStatusTransferItem,
            &it->second, sizeof(PdcpLteSnStatusTransferItem));
        itemIndex++;
    }

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  arg->hoParticipator.srcEnbRnti,
                  arg->hoParticipator.tgtEnbRnti,
                  EPC_MESSAGE_TYPE_SN_STATUS_TRANSFER,
                  msgSize,
                  (char*)arg);

    // del
    free(arg);

    // update stats
    epc->statData.numSnStatusTransferSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_DataForwarding
// LAYER      :: EPC
// PURPOSE    :: API to send message DataForwarding
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// + bearerId         : int      : bearerId
// + forwardingMsg    : Message* : message
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_DataForwarding(
                            Node* node,
                            int interfaceIndex,
                            const HandoverParticipator& hoParticipator,
                            int bearerId,
                            Message* forwardingMsg)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_DataForwarding arg;

    // store
    arg.hoParticipator = hoParticipator;
    arg.bearerId = bearerId;
    arg.message = forwardingMsg;

    int virtualPacketSize = sizeof(HandoverParticipator)
        + sizeof(int) + MESSAGE_ReturnPacketSize(forwardingMsg);

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  arg.hoParticipator.srcEnbRnti,
                  arg.hoParticipator.tgtEnbRnti,
                  EPC_MESSAGE_TYPE_DATA_FORWARDING,
                  virtualPacketSize,
                  (char*)&arg);
}

// /**
// FUNCTION   :: EpcLteAppSend_PathSwitchRequest
// LAYER      :: EPC
// PURPOSE    :: API to send message PathSwitchRequest
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_PathSwitchRequest(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_PathSwitchRequest arg;
    // set participators
    arg.hoParticipator = hoParticipator;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  hoParticipator.tgtEnbRnti,
                  epc->sgwmmeRnti,
                  EPC_MESSAGE_TYPE_PATH_SWITCH_REQUEST,
                  sizeof(EpcLteAppMessageArgument_PathSwitchRequest),
                  (char*)&arg);

    // update stats
    epc->statData.numPathSwitchRequestSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_PathSwitchRequestAck
// LAYER      :: EPC
// PURPOSE    :: API to send message PathSwitchRequestAck
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_PathSwitchRequestAck(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator&hoParticipator,
    BOOL result)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_PathSwitchRequestAck arg;

    // store
    arg.hoParticipator = hoParticipator;
    arg.result = result;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  epc->sgwmmeRnti,
                  hoParticipator.tgtEnbRnti,
                  EPC_MESSAGE_TYPE_PATH_SWITCH_REQUEST_ACK,
                  sizeof(EpcLteAppMessageArgument_PathSwitchRequestAck),
                  (char*)&arg);

    // update stats
    epc->statData.numPathSwitchRequestAckSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_EndMarker
// LAYER      :: EPC
// PURPOSE    :: API to send message EndMarker
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + src              : const LteRnti& : source of message
// + dst              : const LteRnti& : dest of message
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_EndMarker(
                    Node *node,
                    UInt32 interfaceIndex,
                    const LteRnti& src,
                    const LteRnti& dst,
                    const HandoverParticipator& hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_EndMarker arg;
    // set participators
    arg.hoParticipator = hoParticipator;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  src,
                  dst,
                  EPC_MESSAGE_TYPE_END_MARKER,
                  sizeof(EpcLteAppMessageArgument_EndMarker),
                  (char*)&arg);

    // update stats
    epc->statData.numEndMarkerSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_UeContextRelease
// LAYER      :: EPC
// PURPOSE    :: API to send message UeContextRelease
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_UeContextRelease(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator&hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_UeContextRelease arg;
    // set participators
    arg.hoParticipator = hoParticipator;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  hoParticipator.tgtEnbRnti,
                  hoParticipator.srcEnbRnti,
                  EPC_MESSAGE_TYPE_UE_CONTEXT_RELEASE,
                  sizeof(EpcLteAppMessageArgument_UeContextRelease),
                  (char*)&arg);

    // update stats
    epc->statData.numUeContextReleaseSent++;
}

// /**
// FUNCTION   :: EpcLteAppSend_HoPreparationFailure
// LAYER      :: EPC
// PURPOSE    :: API to send message HoPreparationFailure
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : UInt32   : Interface index
// + hoParticipator   : const HandoverParticipator& : participators of H.O.
// RETURN     :: void : NULL
// **/
void EpcLteAppSend_HoPreparationFailure(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator&hoParticipator)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    EpcLteAppMessageArgument_HoPreparationFailure arg;
    // set participators
    arg.hoParticipator = hoParticipator;

    // send
    EpcLteAppSend(node,
                  interfaceIndex,
                  hoParticipator.tgtEnbRnti,
                  hoParticipator.srcEnbRnti,
                  EPC_MESSAGE_TYPE_HO_PREPARATION_FAILURE,
                  sizeof(EpcLteAppMessageArgument_HoPreparationFailure),
                  (char*)&arg);
}

// /**
// FUNCTION   :: EpcLteAppGetNodeAddressOnEpcSubnet
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*       : Pointer to node.
// + targetNodeId      : NodeAddress : Node ID
// RETURN     :: NodeAddress : address of specified node on EPC subnet
// **/
NodeAddress EpcLteAppGetNodeAddressOnEpcSubnet(
    Node* node, NodeAddress targetNodeId)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    const LteRnti& rntiOnEpcSubnet = epc->sgwmmeRnti;

    NodeAddress subnetAddress = MAPPING_GetSubnetAddressForInterface(
        node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
    NodeAddress subnetMask = MAPPING_GetSubnetMaskForInterface(
        node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
    int numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(subnetMask);

    NodeAddress addr = MAPPING_GetInterfaceAddressForSubnet(
        node, targetNodeId, subnetAddress, numHostBits);

    // invalid address returned
    if (addr == ANY_ADDRESS)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr,
            "cannot get valid address on EPC subnet (0x%x) for Node%d.",
            subnetAddress, targetNodeId);
        ERROR_Assert(FALSE, errStr);
    }

    return addr;
}

// /**
// FUNCTION   :: EpcLteAppIsNodeOnTheSameEpcSubnet
// LAYER      :: EPC
// PURPOSE    :: API to send message HandoverRequestAck
// PARAMETERS ::
// + node             : Node*       : Pointer to node.
// + targetNodeId      : NodeAddress : Node ID
// RETURN     :: BOOL : result
// **/
BOOL EpcLteAppIsNodeOnTheSameEpcSubnet(
    Node* node, NodeAddress targetNodeId)
{
    EpcData* epc = EpcLteGetEpcData(node);
    ERROR_Assert(epc, "EPC subnet should be specified to send EPC app");

    const LteRnti& rntiOnEpcSubnet = epc->sgwmmeRnti;

    NodeAddress subnetAddress = MAPPING_GetSubnetAddressForInterface(
        node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
    NodeAddress subnetMask = MAPPING_GetSubnetMaskForInterface(
        node, rntiOnEpcSubnet.nodeId, rntiOnEpcSubnet.interfaceIndex);
    int numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(subnetMask);

    NodeAddress addr = MAPPING_GetInterfaceAddressForSubnet(
        node, targetNodeId, subnetAddress, numHostBits);

    // valid address or not
    return (addr != ANY_ADDRESS);
}

