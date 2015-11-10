#ifndef _EPC_LTE_APP_H_
#define _EPC_LTE_APP_H_

#include <map>
#include "message.h"
#include "fileio.h"
#include "lte_common.h"
#include "layer3_lte.h"

#define EPC_LTE_APP_CAT_EPC_APP    "EpcApp"
#define EPC_LTE_APP_CAT_EPC_PROCESS     "EpcProcess"
#define EPC_LTE_APP_DELAY               0

// /**
// STRUCT      :: EpcLteAppMessageArgument_AttachUE
// DESCRIPTION :: argument for AttachUE
// **/
typedef struct struct_epc_lte_app_message_argument_attach_ue
{
    LteRnti ueRnti;
    LteRnti enbRnti;
} EpcLteAppMessageArgument_AttachUE;

// /**
// STRUCT      :: EpcLteAppMessageArgument_DetachUE
// DESCRIPTION :: argument for DetachUE
// **/
typedef struct struct_epc_lte_app_message_argument_detach_ue
{
    LteRnti ueRnti;
    LteRnti enbRnti;
} EpcLteAppMessageArgument_DetachUE;

// /**
// STRUCT      :: EpcLteAppMessageArgument_HoReq
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_lte_app_message_argument_ho_req
{
    HandoverParticipator hoParticipator;
} EpcLteAppMessageArgument_HoReq;

// /**
// STRUCT      :: EpcLteAppMessageArgument_HoReq
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_lte_app_message_argument_ho_req_ack
{
    HandoverParticipator hoParticipator;
    RrcConnectionReconfiguration reconf;    // including moblityControlInfo
} EpcLteAppMessageArgument_HoReqAck;

typedef struct struct_epc_lte_app_pair_bearerid_snstatus
{
    int bearerId;
    PdcpLteSnStatusTransferItem snStatusTransferItem;
} EpcLteAppPairBearerIdSnStatus;

// /**
// STRUCT      :: EpcLteAppMessageArgument_SnStatusTransfer
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_lte_app_message_argument_sn_status_transfer
{
    HandoverParticipator hoParticipator;
    int itemNum;
    EpcLteAppPairBearerIdSnStatus items[1];
} EpcLteAppMessageArgument_SnStatusTransfer;

// /**
// STRUCT      :: EpcLteAppMessageArgument_SnStatusTransfer
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_lte_app_message_argument_data_forwarding
{
    HandoverParticipator hoParticipator;
    int bearerId;
    Message* message;
} EpcLteAppMessageArgument_DataForwarding;

// /**
// STRUCT      :: EpcLteAppMessageArgument_PathSwitchRequest
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_lte_app_message_argument_path_switch_req
{
    HandoverParticipator hoParticipator;
} EpcLteAppMessageArgument_PathSwitchRequest;

// /**
// STRUCT      :: EpcLteAppMessageArgument_PathSwitchRequestAck
// DESCRIPTION :: argument for HoReqAck
// **/
typedef struct struct_epc_lte_app_message_argument_path_switch_req_ack
{
    HandoverParticipator hoParticipator;
    BOOL result;
} EpcLteAppMessageArgument_PathSwitchRequestAck;

// /**
// STRUCT      :: EpcLteAppMessageArgument_EndMarker
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_lte_app_message_argument_end_marker
{
    HandoverParticipator hoParticipator;
} EpcLteAppMessageArgument_EndMarker;

// /**
// STRUCT      :: EpcLteAppMessageArgument_UeContextRelease
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_lte_app_message_argument_ue_context_release
{
    HandoverParticipator hoParticipator;
} EpcLteAppMessageArgument_UeContextRelease;

// /**
// STRUCT      :: EpcLteAppMessageArgument_HoPreparationFailure
// DESCRIPTION :: argument for HoReq
// **/
typedef struct struct_epc_lte_app_message_argument_ho_preparation_failure
{
    HandoverParticipator hoParticipator;
} EpcLteAppMessageArgument_HoPreparationFailure;

// /**
// STRUCT      :: EpcLteAppMessageContainer
// DESCRIPTION :: common container for EPC message
// **/
typedef struct struct_epc_lte_app_message_container
{
    LteRnti src;
    LteRnti dst;
    EpcMessageType type;
    int length;
    char value[1];
} EpcLteAppMessageContainer;

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
EpcLteAppProcessEvent(Node *node, Message *msg);

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
              int virtualPacketSize = -1);

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
    int virtualPacketSize);

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
                       const LteRnti& enbRnti);

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
                       const LteRnti& enbRnti);

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
                    const HandoverParticipator& hoParticipator);

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
                    const RrcConnectionReconfiguration& reconf);

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
    std::map<int, PdcpLteSnStatusTransferItem>& snStatusTransferItem);

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
                            Message* forwardingMsg);

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
    const HandoverParticipator&hoParticipator);

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
    BOOL result);

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
                    const HandoverParticipator& hoParticipator);

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
    const HandoverParticipator&hoParticipator);

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
    const HandoverParticipator&hoParticipator);

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
    Node* node, NodeAddress targetNodeId);

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
    Node* node, NodeAddress targetNodeId);

#endif  // _EPC_LTE_APP_H_
