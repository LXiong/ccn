#ifndef _EPC_LTE_H_
#define _EPC_LTE_H_

#include "node.h"
#include "epc_lte_common.h"

// /**
// FUNCTION   :: EpcLteGetEpcData
// LAYER      :: EPC
// PURPOSE    :: get EPC data
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: EpcData*    : EPC data on specified node
// **/
EpcData*
EpcLteGetEpcData(Node* node);

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
                NodeAddress sgwmmeNodeId, int sgwmmeInterfaceIndex);

// /**
// FUNCTION   :: EpcLteFinalize
// LAYER      :: EPC
// PURPOSE    :: Finalize EPC
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// RETURN     :: void        : NULL
// **/
void
EpcLteFinalize(Node* node);

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
                      const LteRnti& ueRnti, const LteRnti& enbRnti);
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
                      const LteRnti& ueRnti, const LteRnti& enbRnti);

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
    const HandoverParticipator& hoParticipator);

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
    const HandoverParticipator& hoParticipator);

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
    BOOL result);

#endif    // _EPC_LTE_H_
