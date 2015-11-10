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

#ifndef _MAC_LTE_PDCP_H_
#define _MAC_LTE_PDCP_H_

#include <node.h>

//--------------------------------------------------------------------------
// Enumulation
//--------------------------------------------------------------------------
typedef enum {
    PDCP_LTE_POWER_OFF,
    PDCP_LTE_POWER_ON,
    PDCP_LTE_STATUS_NUM
} PdcpLteState;



// /**
// ENUM         :: PdcpLteHoStatus
// DESCRIPTION  :: ho status
// **/
typedef enum {
    PDCP_LTE_SOURCE_E_NB_IDLE,
    PDCP_LTE_SOURCE_E_NB_WAITING_SN_STATUS_TRANSFER_REQ,
    PDCP_LTE_SOURCE_E_NB_BUFFERING,
    PDCP_LTE_SOURCE_E_NB_FORWARDING,
    PDCP_LTE_SOURCE_E_NB_FORWARDING_END,
    PDCP_LTE_TARGET_E_NB_IDLE,
    PDCP_LTE_TARGET_E_NB_FORWARDING,
    PDCP_LTE_TARGET_E_NB_CONNECTED,
    PDCP_LTE_TARGET_E_NB_WAITING_END_MARKER,
    PDCP_LTE_UE_IDLE,
    PDCP_LTE_UE_BUFFERING
} PdcpLteHoStatus;



// /**
// ENUM         :: PdcpLteHoBufferType
// DESCRIPTION  :: buffer type for ho
// **/
typedef enum {
    PDCP_LTE_REORDERING,
    PDCP_LTE_RETRANSMISSION,
    PDCP_LTE_TRANSMISSION,
} PdcpLteHoBufferType;

//--------------------------------------------------------------------------
// Constant
//--------------------------------------------------------------------------
// /**
// CONSTANT    :: PDCP_LTE_SEQUENCE_LIMIT : 0xFFF(4095)
// DESCRIPTION :: pdcp sequence number limit
// **/

#define PDCP_LTE_SEQUENCE_LIMIT (0xFFF)
#define PDCP_LTE_REORDERING_WINDOW (2048) // ((0xFFF) / 2)
#define PDCP_LTE_INVALID_PDCP_SN (PDCP_LTE_SEQUENCE_LIMIT + 1)

//  According to the specifications,
//  select from ms50, ms100, ms150, ms300, ms500, ms750, ms1500
#define PDCP_LTE_DEFAULT_DISCARD_TIMER_DELAY (500 * MILLI_SECOND)
#define PDCP_LTE_DEFAULT_BUFFER_BYTE_SIZE (50000) // 50KB

// parameter strings
#define LTE_PDCP_STRING_DISCARD_TIMER_DELAY "PDCP-LTE-DISCARD-TIMER-DELAY"
#define LTE_PDCP_STRING_BUFFER_BYTE_SIZE "PDCP-LTE-BUFFER-BYTE-SIZE"

// parameter LIMIT
#define LTE_PDCP_MIN_DISCARD_TIMER_DELAY (1*MILLI_SECOND)
#define LTE_PDCP_MAX_DISCARD_TIMER_DELAY (CLOCKTYPE_MAX)
#define LTE_PDCP_MIN_BUFFER_BYTE_SIZE (0)
#define LTE_PDCP_MAX_BUFFER_BYTE_SIZE (INT_MAX)

//--------------------------------------------------------------------------
// Struct
//--------------------------------------------------------------------------
// /**
// STRUCT:: LtePdcpHeader
// DESCRIPTION:: PDCP Header
// **/
typedef struct
{
    //unsigned char pdcpSN;         // uchar=8bit=XRRRSSSS
                                    // X=DetaOrCtrlFlag
                                    // S=PDCPSN R=RESERVED=0
    //unsigned char pdcpSNCont;
    UInt8 octet[2];
} LtePdcpHeader;



// /**
// STRUCT       ::PdcpLteReceiveStatus
// DESCRIPTION  :: for SN status transfer request
// **/
typedef struct PdcpLteReceiveStatus {
    // managed only the last submitted PDCP received SN
    // in place of the bitmap
    UInt16 lastSubmittedPdcpRxSn;

    PdcpLteReceiveStatus(
            UInt16 argLastSubmittedPdcpRxSn)
        : lastSubmittedPdcpRxSn(argLastSubmittedPdcpRxSn)
    {
    }

   // get last submitted PDCP rx SN
   UInt16 GetLastSubmittedPdcpRxSn(void) {return lastSubmittedPdcpRxSn;}
} PdcpLteReceiveStatus;



// /**
// STRUCT       :: PdcpLteSnStatusTransferItem
// DESCRIPTION  :: for sn status transfer request
// **/
typedef struct PdcpLteSnStatusTransferItem {
    PdcpLteReceiveStatus receiveStatus;
    const UInt16 nextPdcpRxSn;
    const UInt16 nextPdcpTxSn;

    PdcpLteSnStatusTransferItem(
            PdcpLteReceiveStatus argReceiveStatus,
            const UInt16 argNextPdcpRxSn,
            const UInt16 argNextPdcpTxSn)
        : receiveStatus(argReceiveStatus),
        nextPdcpRxSn(argNextPdcpRxSn),
        nextPdcpTxSn(argNextPdcpTxSn)
    {
    }

    PdcpLteSnStatusTransferItem()
        : receiveStatus(0),
        nextPdcpRxSn(0),
        nextPdcpTxSn(0)
    {
    }

    struct PdcpLteSnStatusTransferItem& operator=(
        const struct PdcpLteSnStatusTransferItem& another)
    {
        // For supressing warning C4512
        ERROR_Assert(FALSE,"PdcpLteSnStatusTransferItem::operator= cannot be called");
        return *this;
    }
} PdcpLteSnStatusTransferItem;



// /**
// STRUCT       :: PdcpLteDiscardTimerInfo
// DESCRIPTION  :: infomation for discard timer msg
// **/
typedef struct PdcpLteDiscardTimerInfo {
    const LteRnti dstRnti;
    const int bearerId;
    const UInt16 sn;

    PdcpLteDiscardTimerInfo(
            const LteRnti argDstRnti,
            const int argBearerId,
            const UInt16 argSn)
        : dstRnti(argDstRnti), bearerId(argBearerId), sn(argSn)
    {
    }

    struct PdcpLteDiscardTimerInfo& operator=(
        const struct PdcpLteDiscardTimerInfo& another)
    {
        // For supressing warning C4512
        ERROR_Assert(FALSE,"PdcpLteDiscardTimerInfo::operator= cannot be called");
        return *this;
    }
} PdcpLteDiscardTimerInfo;



// /**
// STRUCT       :: PdcpLteHoManager
// DESCRIPTION  :: pdcp lte ho manager
// **/
typedef struct PdcpLteHoManager
{
    PdcpLteHoStatus hoStatus;

    UInt16 lastSubmittedPdcpRxSn;
    UInt16 nextPdcpRxSn;

    // managed buffer
    list<Message*> reorderingBuffer;
    list<Message*> retransmissionBuffer;
    list<Message*> transmissionBuffer;
    list<Message*> holdingBuffer;

    map<UInt16,Message*> discardTimer;

    // statistics
    UInt16 numEnqueuedRetransmissionMsg;
    UInt32 byteEnqueuedRetransmissionMsg;

    PdcpLteHoManager()
        : // hoStatus(),
            lastSubmittedPdcpRxSn(PDCP_LTE_SEQUENCE_LIMIT),
            nextPdcpRxSn(0),
            numEnqueuedRetransmissionMsg(0),
            byteEnqueuedRetransmissionMsg(0)
    {
    }
} PdcpLteHoManager;



// /**
// STRUCT:: LtePdcpEntity
// DESCRIPTION:: PDCP Entity
// **/
typedef struct
{
    UInt16 pdcpSN; // Sequence Number

    // For processing delayed PDCP PDU sent to upper layer
    clocktype lastPdcpSduSentToNetworkLayerTime;

   PdcpLteHoManager hoManager;

   // incremented next PDCP tx SN
   void SetNextPdcpTxSn(UInt16 nextPdcpTxSn) {
       ERROR_Assert(nextPdcpTxSn <= PDCP_LTE_SEQUENCE_LIMIT, "failed to SetNextPdcpTxSn()");
       pdcpSN = nextPdcpTxSn;
   }
   // get "next PDCP SN" and set to next value
   const UInt16 GetAndSetNextPdcpTxSn(void);
   // get next PDCP tx SN
   const UInt16 GetNextPdcpTxSn(void) {return pdcpSN;}
} LtePdcpEntity;

typedef struct
{
    clocktype txTime;
} LtePdcpTxInfo;



// /**
// STRUCT:: LtePdcpStats
// DESCRIPTION:: PDCP statistics
// **/
typedef struct
{
    UInt32 numPktsFromUpperLayer;// Number of data packets from Upper Layer
    UInt32 numPktsFromUpperLayerButDiscard; // Number of data packets from
                                            // Upper Layer
    UInt32 numPktsToLowerLayer; // Number of data packets to Lower Layer
    UInt32 numPktsFromLowerLayer; // Number of data packets from Lower Layer
    UInt32 numPktsToUpperLayer; // Number of data packets to Upper Layer
    // Number of data packets enqueued in retransmission buffer
    UInt32 numPktsEnqueueRetranamissionBuffer;
    // Number of data packets discarded
    // due to retransmission buffer overflow
    UInt32 numPktsDiscardDueToRetransmissionBufferOverflow;
    // Number of data packets discarded
    // due to RLC's tx buffer overflow
    UInt32 numPktsDiscardDueToRlcTxBufferOverflow;
    // Number of data packets discarded from retransmission buffer
    // due to discard timer expired
    UInt32 numPktsDiscardDueToDiscardTimerExpired;
    // Number of data packets dequeued from retransmission buffer
    UInt32 numPktsDequeueRetransmissionBuffer;
    // Number of data packets discarded due to ack received
    UInt32 numPktsDiscardDueToAckReceived;
    // Number of data packets enqueued in reordering buffer
    UInt32 numPktsEnqueueReorderingBuffer;
    // Number of data packets discarded due to already received
    UInt32 numPktsDiscardDueToAlreadyReceived;
    // Number of data packets dequeued from reordering buffer
    UInt32 numPktsDequeueReorderingBuffer;
    // Number of data packets discarded from reordering buffer
    // due to invalid PDCP SN received
    UInt32 numPktsDiscardDueToInvalidPdcpSnReceived;
} LtePdcpStats;

// /**
// STRUCT:: LtePdcpData
// DESCRIPTION:: PDCP sublayer structure
// **/
typedef struct struct_lte_pdcp_data
{
    LtePdcpStats stats;

    PdcpLteState pdcpLteState;

    clocktype discardTimerDelay;
    UInt32 bufferByteSize;
} LtePdcpData;

//--------------------------------------------------------------------------
// Function
//--------------------------------------------------------------------------
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
    const NodeInput* nodeInput);

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
    const NodeInput* nodeInput);

// /**
// FUNCTION::       PdcpLteFinalize
// LAYER::          MAC LTE PDCP
// PURPOSE::        Pdcp finalization function
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: interdex of interface
// RETURN::         NULL
// **/
void PdcpLteFinalize(Node* node, unsigned int interfaceIndex);

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
    Message* msg);

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
    LtePdcpData* pdcpData);



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
    BOOL isReEstablishment);



// /**
// FUNCTION   :: PdcpLteNotifyPowerOn
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyPowerOn(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: PdcpLteNotifyPowerOn
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: Power OFF
// PARAMETERS ::
// + node               : Node*        : Pointer to node.
// + interfaceIndex     : int          : Interface index
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyPowerOff(Node* node, int interfaceIndex);

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
void PdcpLteSetState(Node* node, int interfaceIndex, PdcpLteState state);

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
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LtePdcpEntity* pdcpEntity, BOOL isTargetEnb);



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
    Node* node, int interfaceIndex,
    const LteRnti& rnti, LtePdcpEntity* pdcpEntiry);



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
// + rxAckSn            : const UInt16   : SN of received msg
// RETURN     :: void : NULL
// **/
void PdcpLteReceivePdcpStatusReportFromRlc(
    Node* node,
    const int interfaceIndex,
    const LteRnti& srcRnti,
    const int bearerId,
    const UInt16 rxAckSn);



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
    const int bearerId);



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
    PdcpLteSnStatusTransferItem& item);



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
        const int bearerId);



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
        Message* forwardedMsg);



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
        const int bearerId);



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
        const int bearerId);



// /**
// FUNCTION   :: PdcpLteNotifyRlcReset
// LAYER      :: LTE Layer2 PDCP
// PURPOSE    :: event11
//               discard msg in retransmission buffer
//               and send msg in reordering buffer to upper layer
// // PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rRnti              : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// RETURN     :: void : NULL
// **/
void PdcpLteNotifyRlcReset(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId);



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
void PdcpLteReEstablishment (
    Node* node,
    const int interfaceIndex,
    const LteRnti& rnti,
    const int bearerId);
#endif // _MAC_LTE_PDCP_H_

