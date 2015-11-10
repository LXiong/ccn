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

#include "api.h"
#include "layer2_lte.h"
#include "layer2_lte_rlc.h"
#include "layer3_lte.h"
#include "layer2_lte_pdcp.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"

static std::string GetLtePdcpStateName(PdcpLteState state)
{
    if (state == PDCP_LTE_POWER_OFF)
        return std::string("PDCP_POWER_OFF");
    else if (state == PDCP_LTE_POWER_ON)
        return std::string("PDCP_POWER_ON");
    else {
        ERROR_Assert(false, "");
        return "";
    }
}



#define PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(\
        node, interfaceIndex, rnti, hoStatus, category) (\
        lte::LteLog::InfoFormat(node, interfaceIndex,\
            LTE_STRING_LAYER_TYPE_PDCP,\
            category\
            LTE_STRING_FORMAT_RNTI","\
            "%d,%s",\
            rnti.nodeId,\
            rnti.interfaceIndex,\
            hoStatus,\
            __FUNCTION__)\
        )
#endif // LTE_LIB_LOG



//--------------------------------------------------------------------------
// define static function
//--------------------------------------------------------------------------
// /**
// FUNCTION::       PdcpLtePrintStat
// LAYER::          LAYER2 LTE PDCP
// PURPOSE::        Print Pdcp statistics
// PARAMETERS::
//    + node:       pointer to the network node
//    + iface:      interface index
//    + PdcpData:    Pdcp sublayer data structure
// RETURN::         NULL
// **/
static
void PdcpLtePrintStat(
        Node* node,
        int interfaceIndex,
        LtePdcpData* pdcpData);

// /**
// FUNCTION::       PdcpLteProcessPdcpSdu
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Process PDCP SDU
//                  + add header
//                  + send to RLC
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + dstRnti        : Destination RNTI
//    + bearerId       : Radio Bearer ID
//    + pdcpSdu        : PDCP SDU
// RETURN::         NULL
// **/
static
void PdcpLteProcessPdcpSdu(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    Message* pdcpSdu);



static
LtePdcpEntity* PdcpLteGetPdcpEntity(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId);



// /**
// FUNCTION::       PdcpLteExpiredDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        event10
//                  discard pdcp pdu from retransmission buffer
// PARAMETERS::
// + node           : Node*     : pointer to the network node
// + interfaceIndex : const int : index of interface
// + expiredTimer   : Message*  : expired discard timer message
// RETURN:: void : NULL
// **/
static
void PdcpLteExpiredDiscardTimer(
        Node* node,
        const int interfaceIndex,
        Message* expiredTimer);



// /**
// FUNCTION::       PdcpLteChangePdcpLteHoStatus
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        change ho status
// PARAMETERS::
// + node           : Node*            : pointer to the network node
// + interfaceIndex : const int        : index of interface
// + rnti           : const LteRnti&   : rnti
// + expiredTimer   : Message*         : expired discard timer message
// + before         : PdcpLteHoStatus& : current ho status
// + after          : PdcpLteHoStatus& : next ho status
// RETURN:: void : NULL
// **/
static
void PdcpLteChangePdcpLteHoStatus(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoStatus& before,
        PdcpLteHoStatus after);



// /**
// FUNCTION::       PdcpLteAddHeader
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        add pdcp header to pdcp sdu
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : const int      : index of interface
// + dstRnti           : const LteRnti& : destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + pdcpSdu        : Message*       : pdcp sdu
// + sn             : const UInt16   : pdcp pdu sn
// RETURN:: void : NULL
// **/
static
void PdcpLteAddHeader(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        Message* pdcpSdu,
        const UInt16 sn);



// /**
// FUNCTION::       PdcpLteSetDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        set discard Timer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const LteRnti&         : destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// + discardTimer   : map<UInt16, Message*>& : discard timer message
// + pdcpSdu        : Message*               : pdcp sdu
// + sn             : const UInt16           : pdcp pdu sn
// RETURN:: void : NULL
// **/
static
void PdcpLteSetDiscardTimer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        map<UInt16, Message*>& discardTimer,
        const UInt16 sn);



// /**
// FUNCTION::       PdcpLteIsUpdatedRxSnStatus
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        check whether received PDCP SN is in correct range,
//                  and update rx SN status
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const LteRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + hoManager      : PdcpLteHoManager&      : ho manager
// + rxSn           : const UInt16           : sn of received message
// + isReEstablishment : BOOL : whether pdcp pdu received is re-establishment
// RETURN:: BOOL : is updated rx sn status : TRUE
//                  otherwise              : FALSE
// **/
static
BOOL PdcpLteIsUpdatedRxSnStatus(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        const UInt16 rxSn,
        BOOL isReEstablishment);



// /**
// FUNCTION::       PdcpLteSendPdcpSduToUpperLayer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        send pdcp sdu to upper layer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const LteRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + pdcpEntity     : const LtePdcpEntity&   : pdcp entity
// + reorderingBuffer : list<Message*>&      : reordering buffer
// + itr : list<Message*>::iterator  : iterator of reordering buffer
// RETURN:: void : NULL
// **/
static
void PdcpLteSendPdcpSduToUpperLayer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        LtePdcpEntity* pdcpEntity,
        list<Message*>& reorderingBuffer,
        list<Message*>::iterator& itr);



// /**
// FUNCTION::       PdcpLteInsertMsgInForwardingList
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue msg from such buffer and insert forwarding list
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + rnti           : const LteRnti&         : RNTI
// + bearerId       : const int              : Radio Bearer ID
// + forwardingList : list<Message*>&        : forwarding list
// + hoManager      : PdcpLteHoManager&      : ho manager
// + bufferType     : PdcpLteHoBufferType    : buffer type of dequeued msg
// RETURN:: void : NULL
// **/
static
void PdcpLteInsertMsgInForwardingList(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        list<Message*>& forwardingList,
        PdcpLteHoManager& hoManager,
        PdcpLteHoBufferType bufferType);



// /**
// FUNCTION::       PdcpLteSendPdcpPduToRlc
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        send pdcp pdu to rlc
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const LteRnti&         : Destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// * pdcpPdu        : Message*               : pdcp pdu
// * sn             : UInt16&                : sn of pdcp pdu
// + retransmissionBuffer : list<Message*>&  : retransmission buffer
// RETURN:: void : NULL
// **/
static
void PdcpLteSendPdcpPduToRlc(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        Message* pdcpPdu,
        UInt16& sn,
        list<Message*>& retransmissionBuffer);



// /**
// FUNCTION::       PdcpLteProcessReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu or enqueue in reordering buffer,
//                  reorder reordering bufferor,
//                  and dequeue and send msg to upper layer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const LteRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + isReEstablishment : BOOL : whether pdcp pdu received is re-establishment
// + pdcpEntity     : LtePdcpEntity* : pointer to pdcp entity
// + reorderingBuffer : list<Message*>& : reordering buffer
// + pdcpPdu        : Message*       : pdcp pdu 
// RETURN:: void : NULL
// **/
static
void PdcpLteProcessReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        BOOL isReEstablishment,
        LtePdcpEntity* pdcpEntity,
        list<Message*>& reorderingBuffer,
        Message* pdcpPdu);



// /**
// FUNCTION::       PdcpLteEnqueueReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp pdu in reordering buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const LteRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : reordering buffer
// + itr ; list<Message*>::iterator& : iterator of reordering buffer
// + msg            : Message*       : pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRrnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        list<Message*>::iterator& itr,
        Message* msg);



// /**
// FUNCTION::       PdcpLteEnqueueRetransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const LteRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : retransmission buffer
// + msg            : Message*       : pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueRetransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg);



// /**
// FUNCTION::       PdcpLteEnqueueTransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp sdu in transmission buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + dstRnti        : const LteRnti& : Destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : transmission buffer
// + msg            : Message*       : pdcp sdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueTransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg);



// /**
// FUNCTION::       PdcpLteEnqueueHoldingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp sdu in holding buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + dstRnti        : const LteRnti& : Destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : holding buffer
// + msg            : Message*       : pdcp sdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueHoldingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg);



// /**
// FUNCTION::       PdcpLteDequeueReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp pdu in reordering buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + srcRnti        : const LteRnti&    : Source RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : reordering buffer
// + msg            : list<Message*>::iterator : iterator of pdcp pdu
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        list<Message*>::iterator& itr);



// /**
// FUNCTION::       PdcpLteDequeueRetransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : retransmission buffer
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueRetransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer);



// /**
// FUNCTION::       PdcpLteDequeueTransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp sdu in transmission buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : transmission buffer
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueTransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer);



// /**
// FUNCTION::       PdcpLteDequeueHoldingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp sdu in holding buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : holding buffer
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueHoldingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer);



// /**
// FUNCTION::       PdcpLteCancelDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        canceel discard timer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + discardTimer   : list<Message*>&   : discard timer
// + itr            : list<Message*>::iterator : iterator of discard timer
// RETURN:: Message* : dequeued message
// **/
static
void PdcpLteCancelDiscardTimer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        map<UInt16, Message*>& discardTimer,
        map<UInt16, Message*>::iterator& itr);



// /**
// FUNCTION::       PdcpLteRetransmissionBufferOverflow
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu due to retransmission buffer overflow
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + dstRnti        : const LteRnti&    : Destination RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + msg            : Message*          : discard pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteRetransmissionBufferOverflow(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        Message* msg);

// /**
// FUNCTION::       PdcpLteRlcTxBufferOverflow
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu due to RLC's tx buffer overflow
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + dstRnti        : const LteRnti&    : Destination RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + msg            : Message*          : discard pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteRlcTxBufferOverflow(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        Message* msg);



//--------------------------------------------------------------------------
// function
//--------------------------------------------------------------------------
// /**
// FUNCTION::       PdcpLteTransferPduFromRlcToUpperLayer
// LAYER::          MAC LTE PDCP
// PURPOSE::        Send PDU from RLC to PDCP
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + pdcpPdu:    PDCP PDU
// RETURN::         NULL
// **/
static void PdcpLteTransferPduFromRlcToUpperLayer(
    Node* node,
    int interfaceIndex,
    Message* pdcpPdu);



// /**
// FUNCTION::       PdcpLteInit
// LAYER::          MAC LTE PDCP
// PURPOSE::        Pdcp Initalization
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file
// RETURN::         NULL
// **/
void PdcpLteInit(Node* node,
                  unsigned int interfaceIndex,
                  const NodeInput* nodeInput)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);
    pdcpData->discardTimerDelay = PDCP_LTE_DEFAULT_DISCARD_TIMER_DELAY;
    pdcpData->bufferByteSize = PDCP_LTE_DEFAULT_BUFFER_BYTE_SIZE;

    // init parameter
    PdcpLteInitConfigurableParameters(node, interfaceIndex, nodeInput);

    // init stat
    pdcpData->stats.numPktsFromLowerLayer = 0;
    pdcpData->stats.numPktsFromUpperLayer = 0;
    pdcpData->stats.numPktsFromUpperLayerButDiscard = 0;
    pdcpData->stats.numPktsToLowerLayer = 0;
    pdcpData->stats.numPktsToUpperLayer = 0;
    // for retransmission buffer
    pdcpData->stats.numPktsEnqueueRetranamissionBuffer = 0;
    pdcpData->stats.numPktsDiscardDueToRetransmissionBufferOverflow = 0;
    pdcpData->stats.numPktsDiscardDueToRlcTxBufferOverflow = 0;
    pdcpData->stats.numPktsDiscardDueToDiscardTimerExpired = 0;
    pdcpData->stats.numPktsDequeueRetransmissionBuffer = 0;
    pdcpData->stats.numPktsDiscardDueToAckReceived;
    // for reordering buffer
    pdcpData->stats.numPktsEnqueueReorderingBuffer = 0;
    pdcpData->stats.numPktsDiscardDueToAlreadyReceived = 0;
    pdcpData->stats.numPktsDequeueReorderingBuffer = 0;
    pdcpData->stats.numPktsDiscardDueToInvalidPdcpSnReceived = 0;

    // init state
    pdcpData->pdcpLteState = PDCP_LTE_POWER_OFF;
}

// /**
// FUNCTION::       PdcpLteInitConfigurableParameters
// LAYER::          MAC LTE PDCP
// PURPOSE::        Initialize configurable parameters of LTE PDCP Layer.
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + nodeInput:  Input from configuration file
// RETURN::         NULL
// **/
void PdcpLteInitConfigurableParameters(Node* node,
    unsigned int interfaceIndex,
    const NodeInput* nodeInput)
{
    BOOL wasFound = false;
    char errBuf[MAX_STRING_LENGTH] = {0};
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);
    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            node, node->nodeId, interfaceIndex);
    clocktype retTime = 0;
    int retInt = 0;

    // LTE_PDCP_STRING_DISCARD_TIMER_DELAY
    IO_ReadTime(node->nodeId,
               interfaceAddress,
               nodeInput,
               LTE_PDCP_STRING_DISCARD_TIMER_DELAY,
               &wasFound,
               &retTime);

    if (wasFound)
    {
        if (retTime >= LTE_PDCP_MIN_DISCARD_TIMER_DELAY &&
            retTime < LTE_PDCP_MAX_DISCARD_TIMER_DELAY)
        {
            pdcpData->discardTimerDelay = retTime;
        }
        else
        {
            char timeBufMin[MAX_STRING_LENGTH] = {0};
            char timeBufMax[MAX_STRING_LENGTH] = {0};
            TIME_PrintClockInSecond(
                LTE_PDCP_MIN_DISCARD_TIMER_DELAY, timeBufMin);
            TIME_PrintClockInSecond(
                LTE_PDCP_MAX_DISCARD_TIMER_DELAY, timeBufMax);
            sprintf(errBuf,
                    "LTE-PDCP: Discard timer delay "
                    "should be %ssec to %ssec. ",
                    timeBufMin, timeBufMax);
            ERROR_ReportError(errBuf);
        }
    }
    else
    {
        // default
        pdcpData->discardTimerDelay = PDCP_LTE_DEFAULT_DISCARD_TIMER_DELAY;
    }

    // LTE_PDCP_STRING_DISCARD_TIMER_DELAY
    IO_ReadInt(node->nodeId,
               interfaceAddress,
               nodeInput,
               LTE_PDCP_STRING_BUFFER_BYTE_SIZE,
               &wasFound,
               &retInt);

    if (wasFound)
    {
        if (retInt >= LTE_PDCP_MIN_BUFFER_BYTE_SIZE &&
            retInt < LTE_PDCP_MAX_BUFFER_BYTE_SIZE)
        {
            pdcpData->bufferByteSize = retInt;
        }
        else
        {
            sprintf(errBuf,
                    "LTE-PDCP: Buffer Size "
                    "should be %d to %d.",
                    LTE_PDCP_MIN_BUFFER_BYTE_SIZE,
                    LTE_PDCP_MAX_BUFFER_BYTE_SIZE);
            ERROR_ReportError(errBuf);
        }
    }
    else
    {
        // default
        pdcpData->bufferByteSize = PDCP_LTE_DEFAULT_BUFFER_BYTE_SIZE;
    }

}

// /**
// FUNCTION::       PdcpLteFinalize
// LAYER::          MAC LTE PDCP
// PURPOSE::        Pdcp finalization function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void PdcpLteFinalize(Node* node, unsigned int interfaceIndex)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);
    PdcpLtePrintStat(node, interfaceIndex, pdcpData);

    delete pdcpData;
}



// /**
// FUNCTION::       PdcpLteProcessEvent
// LAYER::          MAC LTE PDCP
// PURPOSE::        Pdcp event handling function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + message     Message to be handled
// RETURN::         NULL
// **/
void PdcpLteProcessEvent(Node* node,
    unsigned int interfaceIndex,
    Message* msg)
{
    switch (msg->eventType)
    {
        case MSG_PDCP_LTE_DelayedPdcpSdu:
        {
            PdcpLteTransferPduFromRlcToUpperLayer(node, interfaceIndex, msg);
            break;
        }
        // event10: expired discard timer
        case MSG_PDCP_LTE_DiscardTimerExpired: {
            PdcpLteExpiredDiscardTimer(node, interfaceIndex, msg);
            MESSAGE_Free(node, msg);
            msg = NULL;
            break;
        }
        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Event Type(%d) is not supported "
                "by LTE Layer2 PDCP.", msg->eventType);
            ERROR_ReportError(errBuf);
            MESSAGE_Free(node, msg);
            msg = NULL;
            break;
        }
    }
}



// /**
// FUNCTION::       PdcpLteUpperLayerHasPacketToSend
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + pdcpData       : pointer of PDCP Protpcol Data
// RETURN::         NULL
// **/
void PdcpLteUpperLayerHasPacketToSend(
    Node* node,
    int interfaceIndex,
    LtePdcpData* pdcpData)
{
    Message* pdcpSdu = NULL;
    NodeAddress curNextHopAddr;
    MacHWAddress destHWAddr, tempDestHWAddr;
    int packetType;
    int networkType;
    TosType priority;

    while (MAC_OutputQueueIsEmpty(node, interfaceIndex) == FALSE)
    {
        // Check top packet of IP Queue
        MAC_OutputQueueTopPacket(
            node,
            interfaceIndex,
            &pdcpSdu,
            &tempDestHWAddr,
            &networkType,
            &priority);

        // Get top packet of IP Queue
        MAC_OutputQueueDequeuePacketForAPriority(
            node,
            interfaceIndex,
            &priority,
            &pdcpSdu,
            &curNextHopAddr,
            &destHWAddr,
            &networkType,
            &packetType);

        // Destination RNTI
        NodeAddress dstNodeId =
            MAPPING_GetNodeIdFromInterfaceAddress(
                node,
                curNextHopAddr);
        Int32 dstInterfaceIndex =
            MAPPING_GetInterfaceIndexFromInterfaceAddress(
                node,
                curNextHopAddr);
        LteRnti dstRnti(dstNodeId, dstInterfaceIndex);

        // Add Destination Info
        MacLteAddDestinationInfo(node,
                                 interfaceIndex,
                                 pdcpSdu,
                                 dstRnti);

#ifdef ADDON_DB
            // Add Stats DB Info for data packet
            LteMacAddStatsDBInfo(node, pdcpSdu);
            LteMacUpdateStatsDBInfo(node,
                                    pdcpSdu,
                                    MESSAGE_ReturnPacketSize(pdcpSdu),
                                    0,
                                    FALSE);
#endif

        if (pdcpData->pdcpLteState == PDCP_LTE_POWER_OFF)
        {
#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                pdcpSdu,
                node->macData[interfaceIndex]->phyNumber,
                interfaceIndex,
                MAC_Drop,
                node->macData[interfaceIndex]->macProtocol,
                TRUE,
                "Discard due to Power Off");
#endif
            pdcpData->stats.numPktsFromUpperLayerButDiscard++;
            MESSAGE_Free(node, pdcpSdu);
#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2(
                node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "Discard Packet because PDCP is Power OFF.");
#endif
            continue;
        }

        // // do not support broadcast address
        if (MAC_IsBroadcastHWAddress(&destHWAddr) == TRUE)
        {
#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                pdcpSdu,
                node->macData[interfaceIndex]->phyNumber,
                interfaceIndex,
                MAC_Drop,
                node->macData[interfaceIndex]->macProtocol,
                TRUE,
                "Discard Broadcast Packet");
#endif
            pdcpData->stats.numPktsFromUpperLayerButDiscard++;
            MESSAGE_Free(node, pdcpSdu);
#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2(
                node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "Discard Broadcast Packet.");
#endif
            continue;
        }

        LtePdcpEntity* pdcpEntity =
            PdcpLteGetPdcpEntity(
                node, interfaceIndex, dstRnti, LTE_DEFAULT_BEARER_ID);
        if (pdcpEntity == NULL)
        {
#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                pdcpSdu,
                node->macData[interfaceIndex]->phyNumber,
                interfaceIndex,
                MAC_Drop,
                node->macData[interfaceIndex]->macProtocol,
                TRUE,
                "Discard due to No Connection");
#endif
            pdcpData->stats.numPktsFromUpperLayerButDiscard++;
            MESSAGE_Free(node, pdcpSdu);
#ifdef LTE_LIB_LOG
            lte::LteLog::Debug2(
                node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "Discard Packet because of no connection.");
#endif
            continue;
        }

        // Process PDCP SDU
        PdcpLteProcessPdcpSdu(
            node, interfaceIndex, dstRnti,
            LTE_DEFAULT_BEARER_ID, pdcpSdu);
    }
}



// /**
// FUNCTION::       PdcpLteTransferPduFromRlcToUpperLayer
// LAYER::          MAC LTE PDCP
// PURPOSE::        Send PDU from RLC to PDCP
//
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
//    + pdcpPdu:    PDCP PDU
// RETURN::         NULL
// **/
static void PdcpLteTransferPduFromRlcToUpperLayer(
    Node* node,
    int interfaceIndex,
    Message* pdcpPdu)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);
#ifdef ADDON_DB
    if (MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        LteMacUpdateStatsDBInfo(node,
                                pdcpPdu,
                                MESSAGE_ReturnPacketSize(pdcpPdu),
                                0,
                                FALSE);
        HandleMacDBEvents(node,
                          pdcpPdu,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_SendToUpper,
                          node->macData[interfaceIndex]->macProtocol);

    }
    LteMacRemoveStatsDBInfo(node, pdcpPdu);
#endif
    MAC_HandOffSuccessfullyReceivedPacket(
        node, interfaceIndex, pdcpPdu, ANY_ADDRESS);

    pdcpData->stats.numPktsToUpperLayer++;
}



// /**
// FUNCTION::       PdcpLteReceivePduFromRlc
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Notify Upper layer has packet to send
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + srcRnti        : Source RNTI
//    + bearerId       : Radio Bearer ID
//    + pdcpPdu        : PDCP PDU
// RETURN::         NULL
// **/
void PdcpLteReceivePduFromRlc(
    Node* node,
    int interfaceIndex,
    const LteRnti& srcRnti,
    const int bearerId,
    Message* pdcpPdu,
    BOOL isReEstablishment)
{
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    LteLayer2ValidationData* valData =
        LteLayer2GetValidataionData(node, interfaceIndex, srcRnti);
    if (valData != NULL)
    {
        LtePdcpTxInfo* txInfo =
            (LtePdcpTxInfo*)
                MESSAGE_ReturnInfo(pdcpPdu, INFO_TYPE_LtePdcpTxInfo);
        ERROR_Assert(txInfo != NULL, "MESSAGE_ReturnInfo is failed.");
        clocktype delay = node->getNodeTime() - txInfo->txTime;
        lte::LteLog::InfoFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "PdcpPduDelay,"
            "%d,"
            "%llu",
            srcRnti.nodeId,
            delay);
    }
#endif // LTE_LIB_VALIDATION_LOG
#endif // LTE_LIB_LOG

    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    pdcpData->stats.numPktsFromLowerLayer++;

    // event2: receive pdcp pdu from rlc am
    ERROR_Assert(pdcpPdu, "pdcp pdu is NULL");
#ifdef ADDON_DB
    // reduce PDCP header size in stats DB control size info
    LteMacReduceStatsDBControlInfo(node,
                                   pdcpPdu,
                                   sizeof(LtePdcpHeader));
#endif

    LtePdcpEntity* pdcpEntity =
        PdcpLteGetPdcpEntity(node, interfaceIndex, srcRnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
    list<Message*>& reorderingBuffer = hoManager.reorderingBuffer;

#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, srcRnti, hoManager.hoStatus,
            "beginFunction,");
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "rxFromRlc,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%d,last:%d,next:%d,%s",
            srcRnti.nodeId,
            srcRnti.interfaceIndex,
            hoManager.hoStatus,
            LteLayer2GetPdcpSnFromPdcpHeader(pdcpPdu),
            hoManager.lastSubmittedPdcpRxSn,
            hoManager.nextPdcpRxSn,
            isReEstablishment ? "reEstablishment" : "");
#endif // LTE_LIB_LOG

    switch (hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
            // enqueue in reorderingBuffer, reorder buffer,
            // dequeue and send msg to upper layer
            PdcpLteProcessReorderingBuffer(node, interfaceIndex, srcRnti,
                    bearerId, isReEstablishment, pdcpEntity,
                    reorderingBuffer, pdcpPdu);
                break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING: {
            // get sn
            const UInt16 rxSn = LteLayer2GetPdcpSnFromPdcpHeader(pdcpPdu);
            // check whether received PDCP SN is in correct range,
            // and update rx SN status
            if (PdcpLteIsUpdatedRxSnStatus(node, interfaceIndex, srcRnti,
                        bearerId, hoManager, rxSn, isReEstablishment)) {
                // check whether reorderingBuffer is empty
                ERROR_Assert(reorderingBuffer.empty(),
                        "reordering buffer is not empty");
                // enqueue msg in reordering buffer
                reorderingBuffer.push_back(pdcpPdu);
                // insert reordering buffer in forwarding list
                list<Message*> forwardingList;
                PdcpLteInsertMsgInForwardingList(node, interfaceIndex,
                        srcRnti, bearerId, forwardingList, hoManager,
                        PDCP_LTE_REORDERING);
                // forward list to RRC
                Layer3LteSendForwardingDataList(node, interfaceIndex,
                        srcRnti, bearerId, &forwardingList);
            } else {
                // free pdcp sdu
                LteFreeMsg(node, &pdcpPdu);
                // update statistics
                ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                        ->stats.numPktsDiscardDueToInvalidPdcpSnReceived);
            }
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, srcRnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex, srcRnti,
            hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteNotifyPowerOn
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyPowerOn(Node* node, int interfaceIndex)
{
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_PDCP,
        "%s",
        LTE_STRING_CATEGORY_TYPE_POWER_ON);
#endif // LTE_LIB_LOG

    PdcpLteSetState(node, interfaceIndex, PDCP_LTE_POWER_ON);
}



// /**
// FUNCTION   :: PdcpLteNotifyPowerOff
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power OFF
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyPowerOff(Node* node, int interfaceIndex)
{
    // Traversing RNTI-Bearer tree to reset entity is done at Layer 3.
    // This function shold clear things
    // independent of upper layer information.

    PdcpLteSetState(node, interfaceIndex, PDCP_LTE_POWER_OFF);
}



// /**
// FUNCTION   :: PdcpLteSetState
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// + state              : PdcpLteState : State
// RETURN     :: void : NULL
// **/
void PdcpLteSetState(Node* node, int interfaceIndex, PdcpLteState state)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

#ifdef LTE_LIB_LOG
    std::string before = GetLtePdcpStateName(pdcpData->pdcpLteState);
    std::string after  = GetLtePdcpStateName(state);
    lte::LteLog::Debug2Format(
        node,
        interfaceIndex,
        "PdcpLteSetState",
        "Change state(before->after), %s, %s",
        before.c_str(), after.c_str());
#endif // LTE_LIB_LOG

    pdcpData->pdcpLteState = state;

}



// /**
// FUNCTION   :: PdcpLteInitPdcpEntity
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Init PDCP Entity
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : int            : Interface index
// + rnti               : const LteRnti& : RNTI
// + pdcpEntiry         : LtePdcpEntity* : PDCP Entity
// + isTargetEnb        : BOOL           : whether target eNB or not
// RETURN     :: void : NULL
// **/
void PdcpLteInitPdcpEntity(
    Node* node,
    int interfaceIndex,
    const LteRnti& rnti,
    LtePdcpEntity* pdcpEntity,
    BOOL isTargetEnb)
{
    pdcpEntity->pdcpSN = 0;
    pdcpEntity->lastPdcpSduSentToNetworkLayerTime = 0;

    switch (LteLayer2GetStationType(node, interfaceIndex)) {
        case LTE_STATION_TYPE_ENB: {
            pdcpEntity->hoManager.hoStatus = isTargetEnb
                ? PDCP_LTE_TARGET_E_NB_IDLE : PDCP_LTE_SOURCE_E_NB_IDLE;
            break;
        }
        case LTE_STATION_TYPE_UE: {
            pdcpEntity->hoManager.hoStatus = PDCP_LTE_UE_IDLE;
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid node type");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "initPdcpEntity,"
            LTE_STRING_FORMAT_RNTI","
            "%d,lastRx:%d,nextRx:%d,nextTx:%d",
            rnti.nodeId,
            rnti.interfaceIndex,
            pdcpEntity->hoManager.hoStatus,
            pdcpEntity->hoManager.lastSubmittedPdcpRxSn,
            pdcpEntity->hoManager.nextPdcpRxSn,
            pdcpEntity->GetNextPdcpTxSn());
#endif // LTE_LIB_LOG

}



// /**
// FUNCTION   :: PdcpLteFinalizePdcpEntity
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Finalize PDCP Entity
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : int            : Interface index
// + rnti               : const LteRnti& : RNTI
// + pdcpEntiry         : LtePdcpEntity* : PDCP Entity
// RETURN     :: void : NULL
// **/
void PdcpLteFinalizePdcpEntity(
    Node* node,
    int interfaceIndex,
    const LteRnti& rnti,
    LtePdcpEntity* pdcpEntity)
{
    // free all of the messages in buffers managed hoStatus

    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;

    list<Message*>::iterator itr = hoManager.reorderingBuffer.begin();
    list<Message*>::iterator itrEnd = hoManager.reorderingBuffer.end();

    // clear reordering buffer
    for (; itr != itrEnd; ++itr) {
        if ((*itr) != NULL) {
            LteFreeMsg(node, &(*itr));
        } else {
            // do nothing
        }
    }
    hoManager.reorderingBuffer.clear();

    // clear retransmission buffer
    for (itr = hoManager.retransmissionBuffer.begin(),
            itrEnd = hoManager.retransmissionBuffer.end();
            itr != itrEnd; ++itr) {
        if ((*itr) != NULL) {
            LteFreeMsg(node, &(*itr));
        } else {
            // do nothing
        }
    }
    hoManager.retransmissionBuffer.clear();

    // clear transmission buffer
    for (itr = hoManager.transmissionBuffer.begin(),
            itrEnd = hoManager.transmissionBuffer.end();
            itr != itrEnd; ++itr) {
        if ((*itr) != NULL) {
            LteFreeMsg(node, &(*itr));
        } else {
            // do nothing
        }
    }
    hoManager.transmissionBuffer.clear();

    // clear holding buffer
    for (itr = hoManager.holdingBuffer.begin(),
            itrEnd = hoManager.holdingBuffer.end();
            itr != itrEnd; ++itr) {
        if ((*itr) != NULL) {
            LteFreeMsg(node, &(*itr));
        } else {
            // do nothing
        }
    }
    hoManager.holdingBuffer.clear();

    // clear discard timer
    for (map<UInt16, Message*>::iterator
            discardTimerItr = hoManager.discardTimer.begin(),
            discardTimerItrEnd = hoManager.discardTimer.end();
            discardTimerItr != discardTimerItrEnd; ++discardTimerItr) {
        if (discardTimerItr->second) {
            MESSAGE_CancelSelfMsg(node, discardTimerItr->second);
            discardTimerItr->second = NULL;
        } else {
            // do nothing
        }
    }
    hoManager.discardTimer.clear();

    // reset sn status
    pdcpEntity->hoManager.lastSubmittedPdcpRxSn = PDCP_LTE_SEQUENCE_LIMIT;
    pdcpEntity->hoManager.nextPdcpRxSn = 0;
    pdcpEntity->SetNextPdcpTxSn(0);

    // clear statistics
    hoManager.numEnqueuedRetransmissionMsg = 0;
    hoManager.byteEnqueuedRetransmissionMsg = 0;

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "finalizePdcpEntity,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%s,lastRx:%d,nextRx:%d,nextTx:%d",
            rnti.nodeId,
            rnti.interfaceIndex,
            hoManager.hoStatus,
            ((hoManager.reorderingBuffer.empty()
              && hoManager.retransmissionBuffer.empty()
              && hoManager.transmissionBuffer.empty()
              && hoManager.holdingBuffer.empty()
              && hoManager.discardTimer.empty())
             ? "cleared" : "unCleared"),
            hoManager.lastSubmittedPdcpRxSn,
            hoManager.nextPdcpRxSn,
            pdcpEntity->GetNextPdcpTxSn());
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       LtePdcpEntity::GetAndSetNextPdcpTxSn
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        get current "next PDCP tx SN"
//                  and set next PDCP tx SN to next value
// PARAMETERS::
// + void           :
// RETURN:: UInt16 : current "next PDCP tx SN"
// **/
const UInt16 LtePdcpEntity::GetAndSetNextPdcpTxSn()
{
       ERROR_Assert(pdcpSN <= PDCP_LTE_SEQUENCE_LIMIT,
               "failed to GetAndSetNextPdcpTxSn()");
       // get current "next PDCP tx SN"
       UInt16 returnVal = pdcpSN;
       // set next PDCP tx SN to next value
       pdcpSN = (pdcpSN == PDCP_LTE_SEQUENCE_LIMIT) ? 0 : pdcpSN + 1;
       return returnVal;
}



//--------------------------------------------------------------------------
// Static function
//--------------------------------------------------------------------------
static
void PdcpLtePrintStat(
        Node* node,
        int interfaceIndex,
        LtePdcpData* pdcpData)
{
    char buf[MAX_STRING_LENGTH] = {0};
    sprintf(buf, "Number of packets from Upper Layer = %d",
        pdcpData->stats.numPktsFromUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of packets from Upper Layer but discard = %d",
        pdcpData->stats.numPktsFromUpperLayerButDiscard);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of packets to Lower Layer = %d",
        pdcpData->stats.numPktsToLowerLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of packets from Lower Layer = %d",
        pdcpData->stats.numPktsFromLowerLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of packets to Upper Layer = %d",
        pdcpData->stats.numPktsToUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets enqueued "
            "in retransmission buffer = %d",
        pdcpData->stats.numPktsEnqueueRetranamissionBuffer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "due to retransmission buffer overflow = %d",
        pdcpData->stats.numPktsDiscardDueToRetransmissionBufferOverflow);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "due to RLC's tx buffer overflow = %d",
        pdcpData->stats.numPktsDiscardDueToRlcTxBufferOverflow);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "from retransmission buffer due to discard timer expired = %d",
        pdcpData->stats.numPktsDiscardDueToDiscardTimerExpired);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets dequeued "
            "from retransmission buffer = %d",
        pdcpData->stats.numPktsDequeueRetransmissionBuffer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "due to ack received = %d",
        pdcpData->stats.numPktsDiscardDueToAckReceived);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets enqueued "
            "in reordering buffer = %d",
        pdcpData->stats.numPktsEnqueueReorderingBuffer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "due to already received = %d",
        pdcpData->stats.numPktsDiscardDueToAlreadyReceived);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets dequeued "
            "from reordering buffer = %d",
        pdcpData->stats.numPktsDequeueReorderingBuffer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    sprintf(buf, "Number of data packets discarded "
            "from reordering buffer = %d",
        pdcpData->stats.numPktsDiscardDueToInvalidPdcpSnReceived);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE PDCP",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
}



// /**
// FUNCTION::       PdcpLteProcessPdcpSdu
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        Process PDCP SDU
//                  + add header
//                  + send to RLC
// PARAMETERS::
//    + node           : pointer to the network node
//    + interfaceIndex : index of interface
//    + dstRnti        : Destination RNTI
//    + bearerId       : Radio Bearer ID
//    + pdcpSdu        : PDCP SDU
// RETURN::         NULL
// **/
static
void PdcpLteProcessPdcpSdu(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    Message* pdcpSdu)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    LtePdcpEntity* pdcpEntity =
        PdcpLteGetPdcpEntity(node, interfaceIndex, dstRnti, bearerId);

    ERROR_Assert(pdcpEntity != NULL, "PdcpLteGetPdcpEntity return NULL!");

    pdcpData->stats.numPktsFromUpperLayer++;

// event1: received first sent pdcp sdu from upper layer
    ERROR_Assert(pdcpSdu, "pdcp sdu is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, dstRnti, hoManager.hoStatus,
            "beginFunction,");
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "rxFromUpperLayer,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            dstRnti.nodeId,
            dstRnti.interfaceIndex,
            hoManager.hoStatus);
#endif // LTE_LIB_LOG

    switch (hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_UE_IDLE: {
            UInt32 rlcTxBufferSize = LteRlcSendableByteInTxQueue(
                node, interfaceIndex, dstRnti, bearerId);
            UInt32 pdcpPduSize =
                MESSAGE_ReturnPacketSize(pdcpSdu) + sizeof(LtePdcpHeader);
            // check whether RLC's tx buffer is full
            if (rlcTxBufferSize + pdcpPduSize > LTE_RLC_DEF_MAXBUFSIZE)
            {
                PdcpLteRlcTxBufferOverflow(
                    node, interfaceIndex, dstRnti,
                    bearerId, hoManager, pdcpSdu);
            }
            // check whether retransmission buffer is full
            else if ((hoManager.numEnqueuedRetransmissionMsg
                        < PDCP_LTE_REORDERING_WINDOW)
                    && hoManager.byteEnqueuedRetransmissionMsg
                    + MESSAGE_ReturnPacketSize(pdcpSdu)
                    + sizeof(LtePdcpHeader)
                    <= pdcpData->bufferByteSize) {
                // get and set next PDCP SN
                UInt16 sn = pdcpEntity->GetAndSetNextPdcpTxSn();
                // add pdcp pdu header
                PdcpLteAddHeader(node, interfaceIndex, dstRnti,
                        bearerId, pdcpSdu, sn);
                // send PDCP PDU to RLC,
                // enqueue it in retransmission buffer
                // and set discard timer
                PdcpLteSendPdcpPduToRlc(node, interfaceIndex, dstRnti,
                        bearerId, pdcpSdu, sn,
                        hoManager.retransmissionBuffer);
            } else {
                // discard pdcp sdu due to retransmission buffer overflow
                PdcpLteRetransmissionBufferOverflow(
                        node, interfaceIndex, dstRnti,
                        bearerId, hoManager, pdcpSdu);
            }
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_UE_BUFFERING: {
            // enqueue pdcp sdu in transmission buffer
            PdcpLteEnqueueTransmissionBuffer(node, interfaceIndex, dstRnti,
                    bearerId, hoManager,
                    hoManager.transmissionBuffer, pdcpSdu);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING: {
            // check whether buffer is empty
            ERROR_Assert(hoManager.reorderingBuffer.empty(),
                    "transmission buffer is not empty");
            ERROR_Assert(hoManager.retransmissionBuffer.empty(),
                    "transmission buffer is not empty");
            ERROR_Assert(hoManager.transmissionBuffer.empty(),
                    "transmission buffer is not empty");
            // enqueue pdcp sdu in transmission buffer
            PdcpLteEnqueueTransmissionBuffer(node, interfaceIndex, dstRnti,
                    bearerId, hoManager,
                    hoManager.transmissionBuffer, pdcpSdu);
            // insert transmission buffer in forwarding list
            list<Message*> forwardingList;
            PdcpLteInsertMsgInForwardingList(node, interfaceIndex, dstRnti,
                    bearerId, forwardingList, hoManager,
                    PDCP_LTE_TRANSMISSION);
            // forward list to RRC
            Layer3LteSendForwardingDataList(node, interfaceIndex,
                    dstRnti, bearerId, &forwardingList);
            break;
        }
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER: {
            // enqueue pdcp sdu in holding buffer
            PdcpLteEnqueueHoldingBuffer(node, interfaceIndex, dstRnti,
                    bearerId, hoManager, hoManager.holdingBuffer, pdcpSdu);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
                    dstRnti, hoManager.hoStatus, "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, dstRnti, hoManager.hoStatus,
            "endFunction,");
#endif // LTE_LIB_LOG
}



static
LtePdcpEntity* PdcpLteGetPdcpEntity(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId)
{
    LtePdcpEntity* ret = NULL;

    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, dstRnti);

    if (connectionInfo == NULL ||
        connectionInfo->state == LAYER3_LTE_CONNECTION_WAITING)
    {
        return NULL;
    }

    MapLteRadioBearer* radioBearers =
        &(connectionInfo->connectedInfo.radioBearers);

    MapLteRadioBearer::iterator itr;
    itr = radioBearers->find(bearerId);
    if (itr != radioBearers->end())
    {
        ret = &((*itr).second.pdcpEntity);
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf, "Not found Radio Bearer. "
            "Node : %d Interface : %d Bearer : %d",
            dstRnti.nodeId,
            dstRnti.interfaceIndex,
            bearerId);
        ERROR_ReportError(errBuf);
    }

    return ret;
}



// /**
// FUNCTION   :: PdcpLteReceivePdcpStatusReportFromRlc
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event3
//               cancel such discard timer
//               and dequeue retransmission buffer head PDCP PDU
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + srcRnti            : const LteRnti& : source RNTI
// + bearerId           : const int      : Radio Bearer ID
// + rxAckSn            : const UInt16   : SN of received PDCP ACK
// RETURN     :: void : NULL
// **/
void PdcpLteReceivePdcpStatusReportFromRlc(
    Node* node,
    const int interfaceIndex,
    const LteRnti& srcRnti,
    const int bearerId,
    const UInt16 rxAckSn)
{
    LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(
            node, interfaceIndex, srcRnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;

#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, srcRnti, hoManager.hoStatus,
            "beginFunction,");
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "rxAck,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%d",
            srcRnti.nodeId,
            srcRnti.interfaceIndex,
            hoManager.hoStatus,
            rxAckSn);
#endif // LTE_LIB_LOG

    switch (hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE: {
            // cancel discard timer
            map<UInt16, Message*>& discardTimer = hoManager.discardTimer;
            map<UInt16, Message*>::iterator discardTimerItr
                = discardTimer.find(rxAckSn);
            if (discardTimerItr != discardTimer.end()) {
                PdcpLteCancelDiscardTimer(node, interfaceIndex, srcRnti,
                        bearerId, hoManager, discardTimer, discardTimerItr);
            } else {
                // do nothing
                // discard timer is not managed
                // after ho execution or discard timer was expired.
            }

            // dequeue msg until discard timer of retransmission
            // buffer head msg is not managed
            list<Message*>& retransmissionBuffer
                = hoManager.retransmissionBuffer;
            while (!retransmissionBuffer.empty()
                    && (discardTimer.find(LteLayer2GetPdcpSnFromPdcpHeader(
                                retransmissionBuffer.front()))
                        == discardTimer.end())) {
                // dequeue msg from retransmission buffer
                Message* dequeuedMsg = PdcpLteDequeueRetransmissionBuffer(
                        node, interfaceIndex, srcRnti, bearerId, hoManager,
                        retransmissionBuffer);
                // update statistics
                ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                        ->stats.numPktsDiscardDueToAckReceived);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "discardDueToAckRx,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,%d",
                    srcRnti.nodeId,
                    srcRnti.interfaceIndex,
                    hoManager.hoStatus,
                    LteLayer2GetPdcpSnFromPdcpHeader(dequeuedMsg));
#endif // LTE_LIB_LOG
                // free msg
                LteFreeMsg(node, &dequeuedMsg);
            }
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_UE_BUFFERING: {
            // do nothing
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, srcRnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, srcRnti, hoManager.hoStatus,
            "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteGetSnStatusTransferItem
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event4
//               return sn status transfer item
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: PdcpLteSnStatusTransferItem : sn status transfer item
// **/
PdcpLteSnStatusTransferItem PdcpLteGetSnStatusTransferItem(
    Node* node,
    const int interfaceIndex,
    const LteRnti& rnti,
    const int bearerId)
{
    LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(
            node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, pdcpEntity->hoManager.hoStatus,
            "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = pdcpEntity->hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ: {
            // change status
            PdcpLteChangePdcpLteHoStatus(node, interfaceIndex, rnti,
                    bearerId, hoStatus, PDCP_LTE_SOURCE_E_NB_BUFFERING);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "getSnStatusTransferItem,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,lastRx:%d,nextRx:%d,nextTx:%d",
                    rnti.nodeId,
                    rnti.interfaceIndex,
                    pdcpEntity->hoManager.hoStatus,
                    pdcpEntity->hoManager.lastSubmittedPdcpRxSn,
                    pdcpEntity->hoManager.nextPdcpRxSn,
                    pdcpEntity->GetNextPdcpTxSn());

            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, rnti,
                    pdcpEntity->hoManager.hoStatus,
                    "endFunction,");
#endif // LTE_LIB_LOG
            return PdcpLteSnStatusTransferItem(
                    PdcpLteReceiveStatus(
                        pdcpEntity->hoManager.lastSubmittedPdcpRxSn),
                    pdcpEntity->hoManager.nextPdcpRxSn,
                    pdcpEntity->GetNextPdcpTxSn());
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
                    rnti, pdcpEntity->hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }

    return PdcpLteSnStatusTransferItem();
}



// /**
// FUNCTION   :: PdcpLteSetSnStatusTransferItem
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event5
//               set arg to own sn status transfer item
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + item : PdcpLteSnStatusTransferItem& : set sn status transfer item
// RETURN     :: void : NULL
// **/
void PdcpLteSetSnStatusTransferItem(
    Node* node,
    const int interfaceIndex,
    const LteRnti& rnti,
    const int bearerId,
    PdcpLteSnStatusTransferItem& item)
{
    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, pdcpEntity->hoManager.hoStatus,
            "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = pdcpEntity->hoManager.hoStatus) {
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_CONNECTED: {
            // update sn status
            pdcpEntity->hoManager.lastSubmittedPdcpRxSn
                = item.receiveStatus.GetLastSubmittedPdcpRxSn();
            pdcpEntity->hoManager.nextPdcpRxSn = item.nextPdcpRxSn;
            pdcpEntity->SetNextPdcpTxSn(item.nextPdcpTxSn);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "updateSnStatus,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,lastRx:%d,nextRx:%d,nextTx:%d",
                    rnti.nodeId,
                    rnti.interfaceIndex,
                    pdcpEntity->hoManager.hoStatus,
                    pdcpEntity->hoManager.lastSubmittedPdcpRxSn,
                    pdcpEntity->hoManager.nextPdcpRxSn,
                    pdcpEntity->GetNextPdcpTxSn());
#endif // LTE_LIB_LOG

            // change status
            PdcpLteChangePdcpLteHoStatus(node, interfaceIndex, rnti,
                    bearerId, hoStatus,
                    ((hoStatus == PDCP_LTE_TARGET_E_NB_IDLE)
                     ? PDCP_LTE_TARGET_E_NB_FORWARDING
                     : PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER));
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
                    rnti, pdcpEntity->hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex, rnti,
            pdcpEntity->hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteForwardBuffer
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event6
//               make list of msgs in Buffer
//               and call function for forwarding
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteForwardBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId)
{
    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, pdcpEntity->hoManager.hoStatus,
            "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = pdcpEntity->hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_BUFFERING: {

            // insert transmission buffer in forwarding list
            list<Message*> forwardingList;
            PdcpLteInsertMsgInForwardingList(
                    node, interfaceIndex, rnti, bearerId, forwardingList,
                    pdcpEntity->hoManager, PDCP_LTE_REORDERING);
            PdcpLteInsertMsgInForwardingList(
                    node, interfaceIndex, rnti, bearerId, forwardingList,
                    pdcpEntity->hoManager, PDCP_LTE_RETRANSMISSION);
            PdcpLteInsertMsgInForwardingList(
                    node, interfaceIndex, rnti, bearerId, forwardingList,
                    pdcpEntity->hoManager, PDCP_LTE_TRANSMISSION);

            // forward list to RRC
            Layer3LteSendForwardingDataList(node, interfaceIndex,
                    rnti, bearerId, &forwardingList);

            // change status
            PdcpLteChangePdcpLteHoStatus(
                    node, interfaceIndex, rnti, bearerId, hoStatus,
                    PDCP_LTE_SOURCE_E_NB_FORWARDING);

            break;
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
                    rnti, pdcpEntity->hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, pdcpEntity->hoManager.hoStatus,
            "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteReceiveBuffer
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event7
//               enqueue forwarded msg in such buffer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + forwardedMsg       : Message*       : forwarded msg from source eNB
// RETURN     :: void : NULL
// **/
void PdcpLteReceiveBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        Message* forwardedMsg)
{
    ERROR_Assert(forwardedMsg, "forwarded msg is NULL");
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager =  pdcpEntity->hoManager;

    PdcpLteHoBufferType bufferType
        = *((PdcpLteHoBufferType*) MESSAGE_ReturnInfo(
                    forwardedMsg, (UInt16) INFO_TYPE_LtePdcpBufferType));
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex, rnti,
            hoManager.hoStatus, "beginFunction,");

    stringstream log;
    if (bufferType == PDCP_LTE_TRANSMISSION) {
        log << "-";
    } else {
        log << LteLayer2GetPdcpSnFromPdcpHeader(forwardedMsg);
    }
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "rxForwarded,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%s",
            rnti.nodeId,
            rnti.interfaceIndex,
            hoManager.hoStatus,
            log.str().c_str());
#endif // LTE_LIB_LOG

    switch (hoManager.hoStatus) {
        case PDCP_LTE_TARGET_E_NB_FORWARDING: {
            // enqueue msg in target buffer
            switch (bufferType) {
                case PDCP_LTE_REORDERING: {
                    list<Message*>& reorderingBuffer
                        = hoManager.reorderingBuffer;
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // enqueue in reorderingBuffer, reorder buffer,
                    // dequeue and send msg to upper layer
                    PdcpLteProcessReorderingBuffer(
                            node, interfaceIndex, rnti, bearerId, TRUE,
                            pdcpEntity, reorderingBuffer, forwardedMsg);
                    break;
                }
                case PDCP_LTE_RETRANSMISSION: {
                    list<Message*>& retransmissionBuffer
                        = hoManager.retransmissionBuffer;
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // check whether retransmission buffer is full
                    if ((hoManager.numEnqueuedRetransmissionMsg
                                < PDCP_LTE_REORDERING_WINDOW)
                            && hoManager.byteEnqueuedRetransmissionMsg
                            + MESSAGE_ReturnPacketSize(forwardedMsg)
                            <= pdcpData->bufferByteSize) {
                        // enqueue msg in retransmission buffer
                        PdcpLteEnqueueRetransmissionBuffer(
                                node, interfaceIndex, rnti, bearerId,
                                hoManager, retransmissionBuffer,
                                forwardedMsg);
                    } else {
                        // discard forwarded PDCP PDU
                        // due to retransmission buffer overflow
                        PdcpLteRetransmissionBufferOverflow(
                                node, interfaceIndex, rnti, bearerId,
                                hoManager, forwardedMsg);
                    }
                    break;
                }
                case PDCP_LTE_TRANSMISSION: {
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // enqueue msg in transmission buffer
                    PdcpLteEnqueueTransmissionBuffer(
                            node, interfaceIndex, rnti, bearerId, hoManager,
                            hoManager.transmissionBuffer, forwardedMsg);
                    break;
                }
                default: {
                    ERROR_Assert(FALSE, "Invalid buffer type");
                    break;
                }
            }
            break;
        }
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER: {
            switch (bufferType) {
                // check whether retransmission buffer and
                // retransmission bufferall is empty
                ERROR_Assert(hoManager.retransmissionBuffer.empty(),
                        "retransmission buffer is not empty");
                ERROR_Assert(hoManager.transmissionBuffer.empty(),
                        "transmission buffer is not empty");
                // enqueue msg in target buffer
                case PDCP_LTE_REORDERING: {
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // enqueue in reordering buffer, reorder buffer,
                    // dequeue and send msg to upper layer
                    PdcpLteProcessReorderingBuffer(
                            node, interfaceIndex, rnti, bearerId, TRUE,
                            pdcpEntity, hoManager.reorderingBuffer,
                            forwardedMsg);
                    break;
                }
                case PDCP_LTE_RETRANSMISSION: {
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // get PDCP SN
                    UInt16 sn
                        = LteLayer2GetPdcpSnFromPdcpHeader(forwardedMsg);
                    // check whether retransmission buffer is full
                    if ((hoManager.numEnqueuedRetransmissionMsg
                                < PDCP_LTE_REORDERING_WINDOW)
                            && hoManager.byteEnqueuedRetransmissionMsg
                            + MESSAGE_ReturnPacketSize(forwardedMsg)
                            <= pdcpData->bufferByteSize) {
                        // send PDCP PDU to RLC,
                        // enqueue it in retransmission buffer
                        // and set discard timer
                        PdcpLteSendPdcpPduToRlc(node, interfaceIndex, rnti,
                                bearerId, forwardedMsg, sn,
                                hoManager.retransmissionBuffer);
                    } else {
                        // discard PDCP PDU
                        // due to retransmission buffer overflow
                        PdcpLteRetransmissionBufferOverflow(
                                node, interfaceIndex, rnti, bearerId,
                                hoManager, forwardedMsg);
                    }
                    break;
                }
                case PDCP_LTE_TRANSMISSION: {
                    // remove info
                    MESSAGE_RemoveInfo(
                            node, forwardedMsg, INFO_TYPE_LtePdcpBufferType);
                    // check whether retransmission buffer is full
                    if ((hoManager.numEnqueuedRetransmissionMsg
                                < PDCP_LTE_REORDERING_WINDOW)
                            && hoManager.byteEnqueuedRetransmissionMsg
                            + MESSAGE_ReturnPacketSize(forwardedMsg)
                            + sizeof(LtePdcpHeader)
                            <= pdcpData->bufferByteSize) {
                        // get and set next PDCP SN
                        UInt16 sn = pdcpEntity->GetAndSetNextPdcpTxSn();
                        // add PDCP header
                        PdcpLteAddHeader(node, interfaceIndex, rnti,
                                bearerId, forwardedMsg, sn);
                        // check transmission buffer
                        ERROR_Assert(hoManager.transmissionBuffer.empty(),
                                "transmission buffer is not empty");
                        // send PDCP PDU to RLC,
                        // enqueue it in retransmission buffer
                        // and set discard timer
                        PdcpLteSendPdcpPduToRlc(node, interfaceIndex, rnti,
                                bearerId, forwardedMsg, sn,
                                hoManager.retransmissionBuffer);
                    } else {
                        // discard PDCP SDU
                        // due to retransmission buffer overflow
                        PdcpLteRetransmissionBufferOverflow(
                                node, interfaceIndex, rnti, bearerId,
                                hoManager, forwardedMsg);
                    }
                    break;
                }
                default: {
                    ERROR_Assert(FALSE, "Invalid buffer type");
                    break;
                }
            }
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, rnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteNotifyRrcConnected
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event8
//               dequeue msg from buffer and send upper/lower layer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyRrcConnected(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus,
            "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = hoManager.hoStatus) {
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_UE_BUFFERING: {
            // check whether retransmission buffer is empty
            list<Message*>& retransmissionBuffer
                = hoManager.retransmissionBuffer;
            Message* peekedPdcpPdu = NULL;
            if (!retransmissionBuffer.empty()) {
                for (list<Message*>::iterator
                        itr = retransmissionBuffer.begin(),
                        itrEnd = retransmissionBuffer.end();
                        itr != itrEnd; ++itr) {
                    // peek PDCP PDU in retransmission buffer
                    peekedPdcpPdu = *itr;
                    ERROR_Assert(peekedPdcpPdu,
                            "msg in retransmission buffer is NULL");
                    // send PDCP PDU to rlc
                    LteRlcReceiveSduFromPdcp(node, interfaceIndex,
                            rnti, bearerId,
                            MESSAGE_Duplicate(node, peekedPdcpPdu));
                    ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                            ->stats.numPktsToLowerLayer);
#ifdef LTE_LIB_LOG
                    {
                        UInt32 sendByte
                            = MESSAGE_ReturnPacketSize(peekedPdcpPdu);
                        lte::LteLog::InfoFormat(node, interfaceIndex,
                                LTE_STRING_LAYER_TYPE_PDCP,
                                "txToRlc,"
                                LTE_STRING_FORMAT_RNTI","
                                "%d,%d,%d",
                                rnti.nodeId,
                                rnti.interfaceIndex,
                                hoManager.hoStatus,
                                LteLayer2GetPdcpSnFromPdcpHeader(
                                    peekedPdcpPdu),
                                sendByte);
                    }
#endif // LTE_LIB_LOG
                    // set discard timer
                    PdcpLteSetDiscardTimer(node, interfaceIndex, rnti,
                            bearerId, hoManager.discardTimer,
                            LteLayer2GetPdcpSnFromPdcpHeader(peekedPdcpPdu));
                }
            }

            list<Message*>& transmissionBuffer
                = hoManager.transmissionBuffer;
            UInt16 sn = PDCP_LTE_INVALID_PDCP_SN;
            Message* dequeuedMsg = NULL;
            while (!transmissionBuffer.empty()) {
                // dequeue msg from transmission buffer
                dequeuedMsg = PdcpLteDequeueTransmissionBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        transmissionBuffer);

                UInt32 rlcTxBufferSize = LteRlcSendableByteInTxQueue(
                    node, interfaceIndex, rnti, bearerId);
                UInt32 pdcpPduSize =
                    MESSAGE_ReturnPacketSize(dequeuedMsg)
                        + sizeof(LtePdcpHeader);
                // check whether RLC's tx buffer is full
                if (rlcTxBufferSize + pdcpPduSize > LTE_RLC_DEF_MAXBUFSIZE)
                {
                    PdcpLteRlcTxBufferOverflow(
                        node, interfaceIndex, rnti,
                        bearerId, hoManager, dequeuedMsg);
                }
                // check whether retransmission buffer is full
                else if ((hoManager.numEnqueuedRetransmissionMsg
                            < PDCP_LTE_REORDERING_WINDOW)
                        && hoManager.byteEnqueuedRetransmissionMsg
                        + MESSAGE_ReturnPacketSize(dequeuedMsg)
                        + sizeof(LtePdcpHeader)
                        <= pdcpData->bufferByteSize) {
                    // get and set next PDCP SN
                    sn = pdcpEntity->GetAndSetNextPdcpTxSn();
                    // add pdcp header
                    PdcpLteAddHeader(node, interfaceIndex, rnti, bearerId,
                            dequeuedMsg, sn);
                    // send PDCP PDU to RLC,
                    // enqueue it in retransmission buffer
                    // and set discard timer
                    PdcpLteSendPdcpPduToRlc(node, interfaceIndex, rnti,
                            bearerId, dequeuedMsg, sn,
                            hoManager.retransmissionBuffer);
                } else {
                    // discard pdcp sdu due to retransmission buffer overflow
                    PdcpLteRetransmissionBufferOverflow(node, interfaceIndex,
                            rnti, bearerId, hoManager, dequeuedMsg);
                }
            }

            // change status
            PdcpLteChangePdcpLteHoStatus(
                    node, interfaceIndex, rnti, bearerId, hoStatus,
                    (hoStatus == PDCP_LTE_TARGET_E_NB_FORWARDING)
                    ? PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER
                    : PDCP_LTE_UE_IDLE);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, rnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteNotifyEndMarker
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event9
//               target eNB or UE dequeue msg from buffer
//               and send upper/lower layer
//               or source eNB is to be finalize
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyEndMarker(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL (end marker notified)");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus,
            "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_FORWARDING: {
            // change status
            PdcpLteChangePdcpLteHoStatus(node, interfaceIndex, rnti,
                    bearerId, hoStatus, PDCP_LTE_SOURCE_E_NB_FORWARDING_END);
            break;
        }
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER: {
            list<Message*>& holdingBuffer = hoManager.holdingBuffer;
            ERROR_Assert(hoManager.transmissionBuffer.empty(),
                    "transmission buffer is not empty");
            UInt16 sn = PDCP_LTE_INVALID_PDCP_SN;
            Message* dequeuedMsg = NULL;
            while (!holdingBuffer.empty()) {
                // dequeue msg from holding buffer
                dequeuedMsg = PdcpLteDequeueHoldingBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        hoManager.holdingBuffer);

                UInt32 rlcTxBufferSize = LteRlcSendableByteInTxQueue(
                    node, interfaceIndex, rnti, bearerId);
                UInt32 pdcpPduSize =
                    MESSAGE_ReturnPacketSize(dequeuedMsg)
                        + sizeof(LtePdcpHeader);
                // check whether RLC's tx buffer is full
                if (rlcTxBufferSize + pdcpPduSize > LTE_RLC_DEF_MAXBUFSIZE)
                {
                    PdcpLteRlcTxBufferOverflow(
                        node, interfaceIndex, rnti,
                        bearerId, hoManager, dequeuedMsg);
                }
                // check whether retransmission buffer is full
                else if ((hoManager.numEnqueuedRetransmissionMsg
                            < PDCP_LTE_REORDERING_WINDOW)
                        && hoManager.byteEnqueuedRetransmissionMsg
                        + MESSAGE_ReturnPacketSize(dequeuedMsg)
                        + sizeof(LtePdcpHeader)
                        <= pdcpData->bufferByteSize) {
                        // get and set next PDCP SN
                        sn = pdcpEntity->GetAndSetNextPdcpTxSn();
                        // add header
                        PdcpLteAddHeader(node, interfaceIndex, rnti,
                                bearerId, dequeuedMsg, sn);
                        // send PDCP PDU to RLC,
                        // enqueue it in retransmission buffer
                        // and set discard timer
                        PdcpLteSendPdcpPduToRlc(node, interfaceIndex, rnti,
                                bearerId, dequeuedMsg, sn,
                                hoManager.retransmissionBuffer);
                } else {
                    // discard pdcp sdu due to retransmission buffer overflow
                    PdcpLteRetransmissionBufferOverflow(node, interfaceIndex,
                            rnti, bearerId, hoManager, dequeuedMsg);
                }
            }
            // change status
            PdcpLteChangePdcpLteHoStatus(
                    node, interfaceIndex, rnti, bearerId, hoStatus,
                    PDCP_LTE_SOURCE_E_NB_IDLE);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_UE_IDLE:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, rnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteNotifyRlcReset
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event11
//               discard msg in retransmission buffer
//               and send msg in reordering buffer to upper layer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyRlcReset(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId)
{
    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex, rnti,
            pdcpEntity->hoManager.hoStatus, "beginFunction,");
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "notifyRlcReset,"
                LTE_STRING_FORMAT_RNTI","
                "%d",
                rnti.nodeId,
                rnti.interfaceIndex,
                pdcpEntity->hoManager.hoStatus);
#endif // LTE_LIB_LOG

    switch (pdcpEntity->hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE: {
            PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
            list<Message*>& reorderingBuffer = hoManager.reorderingBuffer;
            // dequeue and send PDCP SDU to upper layer
            // till reordering buffer is empty
            list<Message*>::iterator itr = reorderingBuffer.begin();
            while (!reorderingBuffer.empty()) {
                PdcpLteSendPdcpSduToUpperLayer(node, interfaceIndex, rnti,
                        bearerId, pdcpEntity, reorderingBuffer, itr);
            }

            // cancel all discard timer
            map<UInt16, Message*>& discardTimer
                = pdcpEntity->hoManager.discardTimer;
            map<UInt16, Message*>::iterator discardTimerItr
                = discardTimer.begin();
            while (!discardTimer.empty()) {
                PdcpLteCancelDiscardTimer(node, interfaceIndex, rnti,
                        bearerId, hoManager, discardTimer, discardTimerItr);
            }

            // dequeue and discard PDCP PDU
            // till retransmission buffer is empty
            list<Message*>& retransmissionBuffer
                = hoManager.retransmissionBuffer;
            while (!retransmissionBuffer.empty()) {
                // dequeue msg from retransmission buffer
                Message* dequeuedMsg = PdcpLteDequeueRetransmissionBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        retransmissionBuffer);
                // free msg
                LteFreeMsg(node, &dequeuedMsg);
            }

            // reset sn status
            pdcpEntity->hoManager.lastSubmittedPdcpRxSn
                = PDCP_LTE_SEQUENCE_LIMIT;
            pdcpEntity->hoManager.nextPdcpRxSn = 0;
            pdcpEntity->SetNextPdcpTxSn(0);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING: {
            // do nothing
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
                    rnti, pdcpEntity->hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            printf("%d", pdcpEntity->hoManager.hoStatus);fflush(stdout);
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex, rnti,
            pdcpEntity->hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PdcpLteReEstablishment
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event12
//               begin buffering received msg
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteReEstablishment(
    Node* node,
    const int interfaceIndex,
    const LteRnti& rnti,
    const int bearerId)
{
    LtePdcpEntity* pdcpEntity
        = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
            rnti, hoManager.hoStatus, "beginFunction,");
#endif // LTE_LIB_LOG

    switch (PdcpLteHoStatus& hoStatus = hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_UE_IDLE: {
            // cancel all discard timers 
            map<UInt16, Message*>& discardTimer = hoManager.discardTimer;
            map<UInt16, Message*>::iterator itr = discardTimer.begin();
            while (!discardTimer.empty()) {
                PdcpLteCancelDiscardTimer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        discardTimer, itr);
            }
            // change status
            PdcpLteChangePdcpLteHoStatus(
                    node, interfaceIndex, rnti, bearerId, hoStatus,
                    (hoStatus == PDCP_LTE_SOURCE_E_NB_IDLE)
                    ? PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ
                    : PDCP_LTE_UE_BUFFERING);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, rnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, rnti, hoManager.hoStatus, "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteChangePdcpLteHoStatus
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        change ho status
// PARAMETERS::
// + node           : Node*            : pointer to the network node
// + interfaceIndex : const int        : index of interface
// + rnti           : const LteRnti&   : rnti
// + expiredTimer   : Message*         : expired discard timer message
// + before         : PdcpLteHoStatus& : current ho status
// + after          : PdcpLteHoStatus& : next ho status
// RETURN:: void : NULL
// **/
static
void PdcpLteChangePdcpLteHoStatus(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoStatus& before,
        PdcpLteHoStatus after)
{
    ERROR_Assert((before == PDCP_LTE_SOURCE_E_NB_IDLE && after
                == PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ)
            || (before == PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ
                && after == PDCP_LTE_SOURCE_E_NB_BUFFERING)
            || (before == PDCP_LTE_SOURCE_E_NB_BUFFERING
                && after == PDCP_LTE_SOURCE_E_NB_FORWARDING)
            || (before == PDCP_LTE_SOURCE_E_NB_FORWARDING
                && after == PDCP_LTE_SOURCE_E_NB_FORWARDING_END)
            || (before == PDCP_LTE_TARGET_E_NB_IDLE
                && (after == PDCP_LTE_TARGET_E_NB_FORWARDING
                    || after == PDCP_LTE_TARGET_E_NB_CONNECTED))
            || (before == PDCP_LTE_TARGET_E_NB_FORWARDING
                && after == PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER)
            || (before == PDCP_LTE_TARGET_E_NB_CONNECTED
                && after == PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER)
            || (before == PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER
                && after == PDCP_LTE_SOURCE_E_NB_IDLE)
            || (before == PDCP_LTE_UE_IDLE
                && after == PDCP_LTE_UE_BUFFERING)
            || (before == PDCP_LTE_UE_BUFFERING
                && after ==  PDCP_LTE_UE_IDLE),
            "change status error");
#ifdef LTE_LIB_LOG
    UInt16 copyedBefore = before;
#endif // LTE_LIB_LOG
    before = after;
#ifdef LTE_LIB_LOG
    {
        LtePdcpEntity* pdcpEntity
            = PdcpLteGetPdcpEntity(node, interfaceIndex, rnti, bearerId);
        ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "changeHoStatus,"
                LTE_STRING_FORMAT_RNTI","
                "%d,<-%d",
                rnti.nodeId,
                rnti.interfaceIndex,
                pdcpEntity->hoManager.hoStatus,
                copyedBefore);
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteAddHeader
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        add pdcp header to pdcp sdu
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : const int      : index of interface
// + dstRnti           : const LteRnti& : destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + pdcpSdu        : Message*       : pdcp sdu
// + sn             : const UInt16   : pdcp pdu sn
// RETURN:: void : NULL
// **/
static
void PdcpLteAddHeader(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        Message* pdcpSdu,
        const UInt16 sn)
{
    ERROR_Assert(pdcpSdu, "pdcpSdu is NULL");

    MESSAGE_AddHeader(node, pdcpSdu, sizeof(LtePdcpHeader), TRACE_PDCP_LTE);
    LtePdcpHeader* pdcpHeader
        = (LtePdcpHeader*) MESSAGE_ReturnPacket(pdcpSdu);
    ERROR_Assert(sn >> 12 == 0, "PDCP");
    UInt8 high = sn >> 8;
    UInt8 low = sn & 0xFF;

    // Add SN to header
    pdcpHeader->octet[0] = high;
    pdcpHeader->octet[1] = low;
    // NOTE : Only SN is supported now.

    LtePdcpTxInfo* txInfo = (LtePdcpTxInfo*) MESSAGE_AddInfo(
            node, pdcpSdu, sizeof(LtePdcpTxInfo), INFO_TYPE_LtePdcpTxInfo);
    ERROR_Assert(txInfo != NULL, "MESSAGE_AddInfo is failed.");
    txInfo->txTime = node->getNodeTime();
}



// /**
// FUNCTION::       PdcpLteSetDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        set discard Timer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const LteRnti&         : destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// + discardTimer   : map<UInt16, Message*>& : discard timer message
// + pdcpSdu        : Message*               : pdcp sdu
// + sn             : const UInt16           : pdcp pdu sn
// RETURN:: void : NULL
// **/
static
void PdcpLteSetDiscardTimer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        map<UInt16, Message*>& discardTimer,
        const UInt16 sn)
{
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);

    Message* discardTimerMsg = MESSAGE_Alloc(
            node, MAC_LAYER,
            MAC_PROTOCOL_LTE, MSG_PDCP_LTE_DiscardTimerExpired);
    assert(discardTimerMsg);
    MESSAGE_SetInstanceId(discardTimerMsg, interfaceIndex);

    // add info
    PdcpLteDiscardTimerInfo tinfo = PdcpLteDiscardTimerInfo(
           dstRnti, bearerId, sn);
    LteAddMsgInfo(node, interfaceIndex, discardTimerMsg,
            INFO_TYPE_LtePdcpDiscardTimerInfo,
            tinfo);
    MESSAGE_Send(node, discardTimerMsg,
            pdcpData->discardTimerDelay);

    // manage discard timer
    if (!discardTimer.insert(
                pair<UInt16, Message*>(sn, discardTimerMsg)).second) {
        ERROR_Assert(FALSE, "discard timer is already managed");
    } else {
        // do nothing
    }
#ifdef LTE_LIB_LOG
    {
        LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(
                node, interfaceIndex, dstRnti, bearerId);
        ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");

        stringstream log;
        log << "[";
        map<UInt16, Message*>::const_iterator debugItrBegin
            = discardTimer.begin();
        map<UInt16, Message*>::const_iterator debugItrEnd
            = discardTimer.end();
        ERROR_Assert(!pdcpEntity->hoManager.retransmissionBuffer.empty(),
                "retransmission buffer is empty");
        map<UInt16, Message*>::const_iterator debugItrTimerHead
            = ((debugItrTimerHead = discardTimer.lower_bound(
                            LteLayer2GetPdcpSnFromPdcpHeader(
                                pdcpEntity->
                                hoManager.retransmissionBuffer.front())))
                    == debugItrEnd) ? debugItrBegin : debugItrTimerHead;
        map<UInt16, Message*>::const_iterator debugItr = debugItrTimerHead;
        if (debugItrTimerHead != debugItrEnd) {
            do {
                log << debugItr->first << " ";
                debugItr = (++debugItr == debugItrEnd)
                    ? debugItrBegin : debugItr;
            } while (debugItr != debugItrTimerHead);
        } else {
            // do nothing
        }
        log << "]";
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "setDiscardTimer,"
                LTE_STRING_FORMAT_RNTI","
                "%d,%d,->%s",
                dstRnti.nodeId,
                dstRnti.interfaceIndex,
                pdcpEntity->hoManager.hoStatus,
                sn,
                log.str().c_str());
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteIsUpdatedRxSnStatus
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        check whether received PDCP SN is in correct range,
//                  and update rx SN status
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const LteRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + hoManager      : PdcpLteHoManager&      : ho manager
// + rxSn           : const UInt16           : sn of received message
// + isReEstablishment : BOOL : whether pdcp pdu received is re-establishment
// RETURN:: BOOL : is updated rx sn status : TRUE
//                  otherwise              : FALSE
// **/
static
BOOL PdcpLteIsUpdatedRxSnStatus(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        const UInt16 rxSn,
        BOOL isReEstablishment)
{
    UInt16& lastSubmittedPdcpRxSn = hoManager.lastSubmittedPdcpRxSn;
    UInt16& nextPdcpRxSn = hoManager.nextPdcpRxSn;

    if (isReEstablishment) {
        // do nothing
    } else if ((rxSn - lastSubmittedPdcpRxSn > PDCP_LTE_REORDERING_WINDOW)
            || ((lastSubmittedPdcpRxSn - rxSn >= 0)
                && (lastSubmittedPdcpRxSn - rxSn
                    < PDCP_LTE_REORDERING_WINDOW))) {
        return FALSE;
    } else if (nextPdcpRxSn - rxSn > PDCP_LTE_REORDERING_WINDOW) {
        nextPdcpRxSn = rxSn + 1;
    } else if (rxSn >= nextPdcpRxSn) {
        nextPdcpRxSn = (rxSn == PDCP_LTE_SEQUENCE_LIMIT) ? 0 : rxSn + 1;
    } else {
        // do nothing
    }
#ifdef LTE_LIB_LOG
    {
        LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(
                node, interfaceIndex, srcRnti, bearerId);
        ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");

        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "updateRxSnStatus,"
                LTE_STRING_FORMAT_RNTI","
                "%d,rx:%d,last:%d,next:%d,%s",
                srcRnti.nodeId,
                srcRnti.interfaceIndex,
                hoManager.hoStatus,
                rxSn,
                lastSubmittedPdcpRxSn,
                nextPdcpRxSn,
                isReEstablishment ? "reEstablishment" : "");
    }
#endif // LTE_LIB_LOG
    return TRUE;
}



// /**
// FUNCTION::       PdcpLteExpiredDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        event10
//                  discard pdcp pdu from retransmission buffer
// PARAMETERS::
// + node           : Node*     : pointer to the network node
// + interfaceIndex : const int : index of interface
// + expiredTimer   : Message*  : expired discard timer message
// RETURN:: void : NULL
// **/
static
void PdcpLteExpiredDiscardTimer(
        Node* node,
        const int interfaceIndex,
        Message* expiredTimer)
{
    ERROR_Assert(expiredTimer, "expiredTimer is NULL");

    PdcpLteDiscardTimerInfo* info
        = (PdcpLteDiscardTimerInfo*) MESSAGE_ReturnInfo(expiredTimer,
                (UInt16) INFO_TYPE_LtePdcpDiscardTimerInfo);
    ERROR_Assert(info, "pdcpLteDiscardTimerInfo is NULL");

    LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(node, interfaceIndex,
            info->dstRnti, info->bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;

#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(node, interfaceIndex,
            info->dstRnti, hoManager.hoStatus, "beginFunction,");
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "expireDiscardTimer,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%d",
            info->dstRnti.nodeId,
            info->dstRnti.interfaceIndex,
            hoManager.hoStatus,
            info->sn);
#endif // LTE_LIB_LOG

    switch (hoManager.hoStatus) {
        case PDCP_LTE_SOURCE_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER:
        case PDCP_LTE_UE_IDLE: {
            map<UInt16, Message*>& discardTimer = hoManager.discardTimer;
            map<UInt16, Message*>::iterator itr
                = discardTimer.find(info->sn);
            ERROR_Assert(itr != hoManager.discardTimer.end(),
                    "discard timer is not managed");
            PdcpLteCancelDiscardTimer(node, interfaceIndex, info->dstRnti,
                    info->bearerId, hoManager, discardTimer, itr);

            // dequeue msg from retransmission buffer
            Message* dequeuedMsg = PdcpLteDequeueRetransmissionBuffer(
                    node, interfaceIndex, info->dstRnti, info->bearerId,
                    hoManager, hoManager.retransmissionBuffer);
            // update statistics
            ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                    ->stats.numPktsDiscardDueToDiscardTimerExpired);
            // free msg
            LteFreeMsg(node, &dequeuedMsg);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "discardDueToDiscardTimerExpired,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,%d",
                    info->dstRnti.nodeId,
                    info->dstRnti.interfaceIndex,
                    hoManager.hoStatus,
                    info->sn);
#endif // LTE_LIB_LOG
            // notify RLC of discard msg of this sn if exist
            LteRlcDiscardRlcSduFromTransmissionBuffer(node, interfaceIndex,
                    info->dstRnti, info->bearerId, info->sn);
            break;
        }
        case PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ:
        case PDCP_LTE_SOURCE_E_NB_BUFFERING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING:
        case PDCP_LTE_SOURCE_E_NB_FORWARDING_END:
        case PDCP_LTE_TARGET_E_NB_IDLE:
        case PDCP_LTE_TARGET_E_NB_FORWARDING:
        case PDCP_LTE_TARGET_E_NB_CONNECTED:
        case PDCP_LTE_UE_BUFFERING: {
#ifdef LTE_LIB_LOG
            PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
                    node, interfaceIndex, info->dstRnti, hoManager.hoStatus,
                    "invalidStateTransition,");
#endif // LTE_LIB_LOG
            ERROR_Assert(FALSE, "Invalid state transition");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid hoStatus");
            break;
        }
    }
#ifdef LTE_LIB_LOG
    PDCP_LTE_PRINT_DEBUG_STATION_TRANSITION(
            node, interfaceIndex, info->dstRnti, hoManager.hoStatus,
            "endFunction,");
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteSendPdcpSduToUpperLayer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        send pdcp sdu to upper layer
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + srcRnti        : const LteRnti&         : source RNTI
// + bearerId       : const int              : Radio Bearer ID
// + pdcpEntity     : const LtePdcpEntity&   : pdcp entity
// + reorderingBuffer : list<Message*>&      : reordering buffer
// + itr : list<Message*>::iterator  : iterator of reordering buffer
// RETURN:: void : NULL
// **/
static
void PdcpLteSendPdcpSduToUpperLayer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        LtePdcpEntity* pdcpEntity,
        list<Message*>& reorderingBuffer,
        list<Message*>::iterator& itr)
{
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    ERROR_Assert(itr != reorderingBuffer.end(),
            "msg is not found in reordering buffer");

    // check whether reordering buffer is empty
    if (reorderingBuffer.empty()) {
        return;
    } else {
        // do nothing
    }

    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
    UInt16& lastSubmittedPdcpRxSn = hoManager.lastSubmittedPdcpRxSn;

    // dequeue msg from reordering buffer
    Message* dequeuedMsg = PdcpLteDequeueReorderingBuffer(
            node, interfaceIndex, srcRnti, bearerId, hoManager,
            hoManager.reorderingBuffer, itr); 
    // get sn from pdcp header
    UInt16 dequeuedSn = LteLayer2GetPdcpSnFromPdcpHeader(dequeuedMsg);
    // remove pdcp header
    MESSAGE_RemoveHeader(
            node, dequeuedMsg, sizeof(LtePdcpHeader), TRACE_PDCP_LTE);
    // send pdcp sdu to upper layer
    if (pdcpEntity->lastPdcpSduSentToNetworkLayerTime < node->getNodeTime()) {
        pdcpEntity->lastPdcpSduSentToNetworkLayerTime = node->getNodeTime();
        PdcpLteTransferPduFromRlcToUpperLayer(
                node, interfaceIndex, dequeuedMsg);
    } else {
        ++pdcpEntity->lastPdcpSduSentToNetworkLayerTime;
        MESSAGE_SetLayer(dequeuedMsg, MAC_LAYER, bearerId);
        MESSAGE_SetEvent(dequeuedMsg, MSG_PDCP_LTE_DelayedPdcpSdu);
        MESSAGE_SetInstanceId(dequeuedMsg, interfaceIndex);
        MESSAGE_Send(node, dequeuedMsg,
                pdcpEntity->lastPdcpSduSentToNetworkLayerTime
                - node->getNodeTime());
    }
    // update last submitted pdcp rx sn
    lastSubmittedPdcpRxSn = dequeuedSn;
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "txToUpperLayer,"
                LTE_STRING_FORMAT_RNTI","
                "%d,last:%d,next:%d",
                srcRnti.nodeId,
                srcRnti.interfaceIndex,
                hoManager.hoStatus,
                lastSubmittedPdcpRxSn,
                hoManager.nextPdcpRxSn);
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteInsertMsgInForwardingList
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue msg from such buffer and insert forwarding list
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + rnti           : const LteRnti&         : RNTI
// + bearerId       : const int              : Radio Bearer ID
// + forwardingList : list<Message*>&        : forwarding list
// + hoManager      : PdcpLteHoManager&      : ho manager
// + forwardingType : PdcpLteHoForwardingType:forwarding type of dequeued msg
// RETURN:: void : NULL
// **/
static
void PdcpLteInsertMsgInForwardingList(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        list<Message*>& forwardingList,
        PdcpLteHoManager& hoManager,
        PdcpLteHoBufferType bufferType)
{
    switch (bufferType) {
        case PDCP_LTE_REORDERING: {
            list<Message*>& reorderingBuffer = hoManager.reorderingBuffer;
            while (!reorderingBuffer.empty()) {
                // dequeue msg from reordering buffer
                list<Message*>::iterator itr = reorderingBuffer.begin();
                Message* dequeuedMsg = PdcpLteDequeueReorderingBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        reorderingBuffer, itr); 
                // add info
                LteAddMsgInfo(node, interfaceIndex, dequeuedMsg,
                        INFO_TYPE_LtePdcpBufferType, bufferType);
                // insert list
                forwardingList.push_back(dequeuedMsg);
            }
            break;
        }
        case PDCP_LTE_RETRANSMISSION: {
            list<Message*>& retransmissionBuffer
                = hoManager.retransmissionBuffer;
            while (!retransmissionBuffer.empty()) {
                // dequeue msg from retransmission buffer
                Message* dequeuedMsg = PdcpLteDequeueRetransmissionBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        retransmissionBuffer);
                // add info
                LteAddMsgInfo(node, interfaceIndex, dequeuedMsg,
                        INFO_TYPE_LtePdcpBufferType, bufferType);
                // insert list
                forwardingList.push_back(dequeuedMsg);
            }
            ERROR_Assert(hoManager.discardTimer.empty(),
                    "discard timer is managed when buffering");
            break;
        }
        case PDCP_LTE_TRANSMISSION: {
            list<Message*>& transmissionBuffer
                = hoManager.transmissionBuffer;
            Message* dequeuedMsg = NULL;
            while (!transmissionBuffer.empty()) {
                // dequeue msg from transmission buffer
                dequeuedMsg = PdcpLteDequeueTransmissionBuffer(
                        node, interfaceIndex, rnti, bearerId, hoManager,
                        hoManager.transmissionBuffer);
                // add info
                LteAddMsgInfo(node, interfaceIndex, dequeuedMsg,
                        INFO_TYPE_LtePdcpBufferType, bufferType);
                // insert list
                forwardingList.push_back(dequeuedMsg);
            }
            break;
        }
        default: {
            ERROR_Assert(FALSE, "invalid buffer type");
        }
    }
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "InsertForwardingList,"
                LTE_STRING_FORMAT_RNTI","
                "%d,num:%d",
                rnti.nodeId,
                rnti.interfaceIndex,
                hoManager.hoStatus,
                forwardingList.size());
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteSendPdcpPduToRlc
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        send pdcp pdu to rlc
// PARAMETERS::
// + node           : Node*                  : pointer to the network node
// + interfaceIndex : int                    : index of interface
// + dstRnti        : const LteRnti&         : Destination RNTI
// + bearerId       : const int              : Radio Bearer ID
// * pdcpPdu        : Message*               : pdcp pdu
// * sn             : UInt16&                : sn of pdcp pdu
// + retransmissionBuffer : list<Message*>&  : retransmission buffer
// RETURN:: void : NULL
// **/
static
void PdcpLteSendPdcpPduToRlc(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        Message* pdcpPdu,
        UInt16& sn,
        list<Message*>& retransmissionBuffer)
{
    ERROR_Assert(pdcpPdu, "pdcpPdu is NULL");
    LtePdcpEntity* pdcpEntity = PdcpLteGetPdcpEntity(node, interfaceIndex,
            dstRnti, bearerId);
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;

    // send pdcp pdu to rlc
    LteRlcReceiveSduFromPdcp(node, interfaceIndex, dstRnti, bearerId,
            MESSAGE_Duplicate(node, pdcpPdu));
    LtePdcpData* pdcpData = LteLayer2GetLtePdcpData(node, interfaceIndex);
    ++pdcpData->stats.numPktsToLowerLayer;
#ifdef LTE_LIB_LOG
    {
        UInt32 sendByte = MESSAGE_ReturnPacketSize(pdcpPdu);
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "txToRlc,"
                LTE_STRING_FORMAT_RNTI","
                "%d,%d,%d",
                dstRnti.nodeId,
                dstRnti.interfaceIndex,
                hoManager.hoStatus,
                sn,
                sendByte);
    }
#endif // LTE_LIB_LOG
    // check whether discard timer is not managed
    ERROR_Assert(hoManager.discardTimer.find(sn)
            == hoManager.discardTimer.end(),
            "discard timer is already managed");
    // enqueue msg in retransmission buffer
    PdcpLteEnqueueRetransmissionBuffer(node, interfaceIndex, dstRnti,
            bearerId, hoManager, retransmissionBuffer, pdcpPdu);
    // set discard timer
    PdcpLteSetDiscardTimer(node, interfaceIndex, dstRnti, bearerId,
            hoManager.discardTimer, sn);
}



// /**
// FUNCTION::       PdcpLteProcessReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu or enqueue in reordering buffer,
//                  reorder reordering bufferor,
//                  and dequeue and send msg to upper layer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const LteRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + isReEstablishment : BOOL : whether pdcp pdu received is re-establishment
// + pdcpEntity     : LtePdcpEntity* : pointer to pdcp entity
// + reorderingBuffer : list<Message*>& : reordering buffer
// + pdcpPdu        : Message*       : pdcp pdu 
// RETURN:: void : NULL
// **/
static
void PdcpLteProcessReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        BOOL isReEstablishment,
        LtePdcpEntity* pdcpEntity,
        list<Message*>& reorderingBuffer,
        Message* pdcpPdu)
{
    ERROR_Assert(pdcpEntity, "pdcpEntity is NULL");
    PdcpLteHoManager& hoManager = pdcpEntity->hoManager;
    // get sn
    const UInt16 rxSn = LteLayer2GetPdcpSnFromPdcpHeader(pdcpPdu);

    // check whether received PDCP SN is in correct range,
    // and update rx SN status
    if (PdcpLteIsUpdatedRxSnStatus(node, interfaceIndex, srcRnti,
                bearerId, hoManager, rxSn, isReEstablishment)) {
        // find insert position
        UInt16& lastSubmittedPdcpRxSn = hoManager.lastSubmittedPdcpRxSn;
        list<Message*>::iterator itr = reorderingBuffer.begin();
        list<Message*>::iterator itrEnd = reorderingBuffer.end();
        UInt16 checkSn = PDCP_LTE_INVALID_PDCP_SN;
        if (reorderingBuffer.empty()) {
            // do nothing
        } else {
            while (itr != itrEnd &&
                    ((((checkSn = LteLayer2GetPdcpSnFromPdcpHeader(*itr))
                       < lastSubmittedPdcpRxSn)
                      ? (checkSn + PDCP_LTE_SEQUENCE_LIMIT + 1)
                      : checkSn)
                     < ((rxSn < lastSubmittedPdcpRxSn)
                         ? (rxSn + PDCP_LTE_SEQUENCE_LIMIT + 1)
                         : rxSn))) {
                ++itr;
            }

        }
        // check whether msg of this sn is already enqueued
        // in reordering buffer
        if (rxSn == checkSn) {
            // update statistics
            ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                    ->stats.numPktsDiscardDueToAlreadyReceived);
            // discard received pdcp pdu
            LteFreeMsg(node, &pdcpPdu);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "discardDueToAlreadyRx,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,%d",
                    srcRnti.nodeId,
                    srcRnti.interfaceIndex,
                    hoManager.hoStatus,
                    rxSn);
#endif // LTE_LIB_LOG
        } else {
            // enqueue msg in reordering buffer
            PdcpLteEnqueueReorderingBuffer(node, interfaceIndex, srcRnti,
                    bearerId, hoManager, reorderingBuffer, itr, pdcpPdu);
        }

        // check whether this msg is re-establishment
        if (isReEstablishment) {
            // do nothing
        } else {
            // get lower bound SN
            checkSn = LteLayer2GetPdcpSnFromPdcpHeader(*itr);
            // dequeue msg less than received SN from reordering buffer
            itr = reorderingBuffer.begin();
            while (itr != reorderingBuffer.end()
                    && ((LteLayer2GetPdcpSnFromPdcpHeader(*itr)
                            != checkSn))) {
                // dequeue and send msg to upper layer
                PdcpLteSendPdcpSduToUpperLayer(
                        node, interfaceIndex, srcRnti,
                        bearerId, pdcpEntity, reorderingBuffer, itr);
            }
        }

        // dequeue msg consecutive sn from reordering buffer
        if (itr != reorderingBuffer.end() && (!isReEstablishment
                    || (isReEstablishment
                        && (rxSn == lastSubmittedPdcpRxSn + 1
                            || rxSn == lastSubmittedPdcpRxSn
                            - PDCP_LTE_SEQUENCE_LIMIT)))) {
            do {
                // dequeue and send msg to upper layer
                PdcpLteSendPdcpSduToUpperLayer(
                        node, interfaceIndex, srcRnti,
                        bearerId, pdcpEntity, reorderingBuffer, itr);
            } while ((itr != reorderingBuffer.end())
                    && (((lastSubmittedPdcpRxSn
                                == PDCP_LTE_SEQUENCE_LIMIT)
                            ? 0 : lastSubmittedPdcpRxSn + 1)
                        == LteLayer2GetPdcpSnFromPdcpHeader(*itr)));
        } else {
            // do nothing
        }
    } else {
        // free pdcp sdu
        LteFreeMsg(node, &pdcpPdu);
        // update statistics
        ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
                ->stats.numPktsDiscardDueToInvalidPdcpSnReceived);
    }
}



// /**
// FUNCTION::       PdcpLteEnqueueReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp pdu in reordering buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + srcRnti        : const LteRnti& : Source RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : reordering buffer
// + itr ; list<Message*>::iterator& : iterator of reordering buffer
// + msg            : Message*       : pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        list<Message*>::iterator& itr,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    // enqueue msg in reordering buffer
    itr = buffer.insert(itr, msg);
    ERROR_Assert(buffer.size() <= PDCP_LTE_REORDERING_WINDOW,
            "reordering buffer overflow");
#ifdef LTE_LIB_LOG
    {
        stringstream log;
        log << "[";
        for (list<Message*>::const_iterator debugItr = buffer.begin(),
                debugItrEnd = buffer.end();
                debugItr != debugItrEnd; ++debugItr) {
            log << LteLayer2GetPdcpSnFromPdcpHeader(*debugItr) << " ";
        }
        log << "]";

        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "enqReoBuf,"
                LTE_STRING_FORMAT_RNTI","
                "%d,%d,->%s",
                srcRnti.nodeId,
                srcRnti.interfaceIndex,
                hoManager.hoStatus,
                LteLayer2GetPdcpSnFromPdcpHeader(msg),
                log.str().c_str());
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteEnqueueRetransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + rnti           : const LteRnti& : RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : retransmission buffer
// + msg            : Message*       : pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueRetransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    //enqueue in retransmission buffer
    buffer.push_back(msg);
    // update statistics
    ++hoManager.numEnqueuedRetransmissionMsg;
    hoManager.byteEnqueuedRetransmissionMsg += MESSAGE_ReturnPacketSize(msg);
    ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
            ->stats.numPktsEnqueueRetranamissionBuffer);
#ifdef LTE_LIB_LOG
        {
            stringstream log;
            log << "[";
            for (list<Message*>::const_iterator debugItr = buffer.begin(),
                    debugItrEnd = buffer.end();
                    debugItr != debugItrEnd; ++debugItr) {
                log << LteLayer2GetPdcpSnFromPdcpHeader(*debugItr) << " ";
            }
            log << "]";

            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "enqReTxBuf,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,num:%d,byte:%d,%d,->%s",
                    rnti.nodeId,
                    rnti.interfaceIndex,
                    hoManager.hoStatus,
                    hoManager.numEnqueuedRetransmissionMsg,
                    hoManager.byteEnqueuedRetransmissionMsg,
                    LteLayer2GetPdcpSnFromPdcpHeader(msg),
                    log.str().c_str());
        }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteEnqueueTransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp sdu in transmission buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + dstRnti        : const LteRnti& : Destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : transmission buffer
// + msg            : Message*       : pdcp sdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueTransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    // enqueue msg in transmission buffer
    buffer.push_back(msg);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "enqTxBuf,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,num:%d",
                    dstRnti.nodeId,
                    dstRnti.interfaceIndex,
                    hoManager.hoStatus,
                    hoManager.transmissionBuffer.size());
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteEnqueueHoldingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        enqueue pdcp sdu in holding buffer
// PARAMETERS::
// + node           : Node*          : pointer to the network node
// + interfaceIndex : int            : index of interface
// + dstRnti        : const LteRnti& : Destination RNTI
// + bearerId       : const int      : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>& : holding buffer
// + msg            : Message*       : pdcp sdu
// RETURN:: void : NULL
// **/
static
void PdcpLteEnqueueHoldingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    // enqueue msg in holding buffer
    buffer.push_back(msg);
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_PDCP,
                    "enqHoldBuf,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d,num:%d",
                    dstRnti.nodeId,
                    dstRnti.interfaceIndex,
                    hoManager.hoStatus,
                    hoManager.holdingBuffer.size());
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteDequeueReorderingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp pdu in reordering buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + srcRnti        : const LteRnti&    : Source RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : reordering buffer
// + msg            : list<Message*>::iterator : iterator of pdcp pdu
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueReorderingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& srcRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer,
        list<Message*>::iterator& itr)
{
    // dequeue msg from reordering buffer
    Message* dequeuedMsg = *itr;
    ERROR_Assert(dequeuedMsg, "dequeue msg is NULL");
    buffer.erase(itr++);
#ifdef LTE_LIB_LOG
    {
        stringstream log;
        log << "[";
        for (list<Message*>::const_iterator debugItr = buffer.begin(),
                debugItrEnd = buffer.end();
                debugItr != debugItrEnd; ++debugItr) {
            log << LteLayer2GetPdcpSnFromPdcpHeader(*debugItr) << " ";
        }
        log << "]";

        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "deqReoBuf,"
                LTE_STRING_FORMAT_RNTI","
                "%d,%d,<-%s",
                srcRnti.nodeId,
                srcRnti.interfaceIndex,
                hoManager.hoStatus,
                LteLayer2GetPdcpSnFromPdcpHeader(dequeuedMsg),
                log.str().c_str());
    }
#endif // LTE_LIB_LOG
    return dequeuedMsg;
}



// /**
// FUNCTION::       PdcpLteDequeueRetransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp pdu in retransmission buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : retransmission buffer
// RETURN:: Message* : dequeued message
// **/
static
Message* PdcpLteDequeueRetransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer)
{
    // dequeue msg from retransmission buffer
    Message* dequeuedMsg = buffer.front();
    ERROR_Assert(dequeuedMsg, "dequeue msg is NULL");
    buffer.pop_front();
    // update statistics
    --hoManager.numEnqueuedRetransmissionMsg;
    hoManager.byteEnqueuedRetransmissionMsg
        -= MESSAGE_ReturnPacketSize(dequeuedMsg);
    ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
            ->stats.numPktsDequeueRetransmissionBuffer);

#ifdef LTE_LIB_LOG
    {
        stringstream log;
        log << "[";
        for (list<Message*>::const_iterator debugItr = buffer.begin(),
                debugItrEnd = buffer.end();
                debugItr != debugItrEnd; ++debugItr) {
            log << LteLayer2GetPdcpSnFromPdcpHeader(*debugItr) << " ";
        }
        log << "]";

        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "deqReTxBuf,"
                LTE_STRING_FORMAT_RNTI","
                "%d,num:%d,byte:%d,%d,<-%s",
                rnti.nodeId,
                rnti.interfaceIndex,
                hoManager.hoStatus,
                hoManager.numEnqueuedRetransmissionMsg,
                hoManager.byteEnqueuedRetransmissionMsg,
                LteLayer2GetPdcpSnFromPdcpHeader(dequeuedMsg),
                log.str().c_str());
    }
#endif // LTE_LIB_LOG
    return dequeuedMsg;
}



// /**
// FUNCTION::       PdcpLteDequeueTransmissionBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp sdu in transmission buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : transmission buffer
// RETURN:: Message* : dequeued message
// **/
static
Message*  PdcpLteDequeueTransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer)
{
    // dequeue msg from transmission buffer
    Message* dequeuedMsg = buffer.front();
    ERROR_Assert(dequeuedMsg, "msg is NULL");
    buffer.pop_front();
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "deqTxBuf,"
                LTE_STRING_FORMAT_RNTI","
                "%d,num:%d",
                rnti.nodeId,
                rnti.interfaceIndex,
                hoManager.hoStatus,
                hoManager.transmissionBuffer.size());
    }
#endif // LTE_LIB_LOG
    return dequeuedMsg;
}



// /**
// FUNCTION::       PdcpLteDequeueHoldingBuffer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        dequeue pdcp sdu in holding buffer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + dstRnti        : const LteRnti&    : destination RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + buffer         : list<Message*>&   : holding buffer
// RETURN:: Message* : dequeued message
// **/
static
Message*  PdcpLteDequeueHoldingBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        list<Message*>& buffer)
{
    // dequeue msg from holding buffer
    Message* dequeuedMsg = buffer.front();
    ERROR_Assert(dequeuedMsg, "dequeue msg is NULL");
    buffer.pop_front();
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "deqHoldBuf,"
                LTE_STRING_FORMAT_RNTI","
                "%d,num:%d",
                dstRnti.nodeId,
                dstRnti.interfaceIndex,
                hoManager.hoStatus,
                hoManager.holdingBuffer.size());
    }
#endif // LTE_LIB_LOG
    return dequeuedMsg;
}



// /**
// FUNCTION::       PdcpLteCancelDiscardTimer
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        canceel discard timer
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + rnti           : const LteRnti&    : RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + discardTimer   : list<Message*>&   : discard timer
// + itr            : list<Message*>::iterator : iterator of discard timer
// RETURN:: Message* : dequeued message
// **/
static
void PdcpLteCancelDiscardTimer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        map<UInt16, Message*>& discardTimer,
        map<UInt16, Message*>::iterator& itr)
{
    ERROR_Assert(itr != discardTimer.end(), "discard timer is not found");
    ERROR_Assert(itr->second, "discard timer is already canceled");
    MESSAGE_CancelSelfMsg(node, itr->second);
    itr->second = NULL;
#ifdef LTE_LIB_LOG
    UInt16 sn = itr->first;
#endif // LTE_LIB_LOG
    discardTimer.erase(itr++);
#ifdef LTE_LIB_LOG
    {
        stringstream log;
        log << "[";
        map<UInt16, Message*>::const_iterator debugItrBegin
            = discardTimer.begin();
        map<UInt16, Message*>::const_iterator debugItrEnd
            = discardTimer.end();
        if (!hoManager.retransmissionBuffer.empty()) {
            map<UInt16, Message*>::const_iterator debugItrTimerHead
                = ((debugItrTimerHead = discardTimer.lower_bound(
                                LteLayer2GetPdcpSnFromPdcpHeader(
                                    hoManager.retransmissionBuffer.front())))
                        == debugItrEnd) ? debugItrBegin : debugItrTimerHead;
            map<UInt16, Message*>::const_iterator debugItr
                = debugItrTimerHead;
            if (debugItrTimerHead != debugItrEnd) {
                do {
                    log << debugItr->first << " ";
                    debugItr = (++debugItr == debugItrEnd)
                        ? debugItrBegin : debugItr;
                } while (debugItr != debugItrTimerHead);
            } else {
                // do nothing
            }
        } else {
            // do nothing
        }
        log << "]";
        lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_PDCP,
                "cancelDiscardTimer,"
                LTE_STRING_FORMAT_RNTI","
                "%d,%d,<-%s",
                rnti.nodeId,
                rnti.interfaceIndex,
                hoManager.hoStatus,
                sn,
                log.str().c_str());
    }
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION::       PdcpLteRetransmissionBufferOverflow
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu due to retransmission buffer overflow
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + dstRnti        : const LteRnti&    : Destination RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + pdcpPdu        : Message*          : discard pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteRetransmissionBufferOverflow(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    // update statistics
    ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
            ->stats.numPktsDiscardDueToRetransmissionBufferOverflow);
    // free msg
    LteFreeMsg(node, &msg);
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "discardDueToReTxBufOverflow,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            dstRnti.nodeId,
            dstRnti.interfaceIndex,
            hoManager.hoStatus);
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::       PdcpLteRlcTxBufferOverflow
// LAYER::          LTE Layer2 PDCP
// PURPOSE::        discard pdcp pdu due to RLC's tx buffer overflow
// PARAMETERS::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + dstRnti        : const LteRnti&    : Destination RNTI
// + bearerId       : const int         : Radio Bearer ID
// + hoManager      : PdcpLteHoManager& : ho manager
// + msg            : Message*          : discard pdcp pdu
// RETURN:: void : NULL
// **/
static
void PdcpLteRlcTxBufferOverflow(
        Node* node,
        const int interfaceIndex,
        const LteRnti& dstRnti,
        const int bearerId,
        PdcpLteHoManager& hoManager,
        Message* msg)
{
    ERROR_Assert(msg, "msg is NULL");
    // update statistics
    ++(LteLayer2GetLtePdcpData(node, interfaceIndex)
            ->stats.numPktsDiscardDueToRlcTxBufferOverflow);
    // free msg
    LteFreeMsg(node, &msg);
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_PDCP,
            "discardDueToRlcTxBufOverflow,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            dstRnti.nodeId,
            dstRnti.interfaceIndex,
            hoManager.hoStatus);
#endif // LTE_LIB_LOG
}

