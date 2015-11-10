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

/*!
 * \file mac_dot11-hcca.cpp
 * \brief Hybrid controller and HCCA access routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "mac_dot11-sta.h"
#include "mac_dot11.h"
#include "mac_dot11-hcca.h"
#include "mac_dot11-mgmt.h"

#include "if_queue.h"


//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcPollListResetAtStartOfBeacon
//  PURPOSE:     Reset values of the poll list at start of new beacon period
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eHcPollListResetAtStartOfBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    int tsSeq = DOT11e_HCCA_MAX_TSID - DOT11e_HCCA_MIN_TSID;
    int endTsSeq = 0;
    DOT11e_HcVars* hc = dot11->hcVars;
    Dot11e_HCCA_cappollListStationItem* next;
    Dot11e_HCCA_cappollListStationItem* first;

    dot11->hcVars->capPeriodExpired=0;

    if (!MacDot11eHCPollListIsEmpty(node, dot11))
    {
        dot11->hcVars->hasPollableStations = TRUE;
    }

    if (hc->allStationsPolledOnce == TRUE)
    {
        dot11->hcVars->currentTSID = DOT11e_HCCA_MAX_TSID;
        hc->allStationsPolledOnce = FALSE;
        hc->allCurrTSIDStationsPolled = FALSE;
        hc->currPolledItem = NULL;
    }
    else
    {
        //All stations are not polled in last DTIM
        //Only reset the TS's that got polled fully
        endTsSeq = dot11->hcVars->currentTSID - DOT11e_HCCA_MIN_TSID + 1;
    }

    for (; tsSeq >=endTsSeq ; tsSeq--)
    {
        hc->cfPollList->TS[tsSeq].firstPolledItem = NULL;
        if (!MacDot11eHCPollListTSIDIsEmpty(node, dot11,
               hc->cfPollList,tsSeq + DOT11e_HCCA_MIN_TSID))
        {

            next = first = MacDot11eHCPollTSIDListGetFirstStation(node,
                dot11,tsSeq + DOT11e_HCCA_MIN_TSID);
            do
            {
                next->data->nullDataCount = 0;
                next = MacDot11eHCPollTSIDListGetNextStation(node, dot11,
                            next, tsSeq + DOT11e_HCCA_MIN_TSID);
            } while (next != first);
        }
    }
}//MacDot11eHcPollListResetAtStartOfBeacon

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListResetForAllPolled
//  PURPOSE:     Reset values of the poll list once all stations are polled.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Count of Pollable stations
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eHCPollListResetForAllPolled(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11e_HcVars* hc = dot11->hcVars;
    hc->currentTSID = DOT11e_HCCA_MAX_TSID;
    hc->allStationsPolledOnce = TRUE;
    dot11->hcVars->hasPollableStations = FALSE;
    int tsSeq = DOT11e_HCCA_MAX_TSID -DOT11e_HCCA_MIN_TSID;
    Dot11e_HCCA_cappollListStationItem* next;
    Dot11e_HCCA_cappollListStationItem* first;

    for (; tsSeq >=0 ; tsSeq--)
    {
        //reset the first polled item.
        hc->cfPollList->TS[tsSeq].firstPolledItem = NULL;

        //Find if this tsid has pollable stations
        if (!MacDot11eHCPollListTSIDIsEmpty(node, dot11,
               hc->cfPollList, tsSeq + DOT11e_HCCA_MIN_TSID))
        {
            next = first =
                MacDot11eHCPollTSIDListGetFirstStation(node, dot11,
                    tsSeq + DOT11e_HCCA_MIN_TSID);
            do
            {
                if (next->data->nullDataCount == 0)
                {
                    dot11->hcVars->hasPollableStations = TRUE;
                    break;
                }
                next = MacDot11eHCPollTSIDListGetNextStation(node, dot11,
                            next, tsSeq + DOT11e_HCCA_MIN_TSID);

            } while (next != first);
        }
    }
}//MacDot11eHCPollListResetForAllPolled


//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcPollListGetItemWithGivenAddress
//  PURPOSE:     Retrieve the list item related to given address
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address lookupAddress
//                  Address to lookup
//              int tsid
//                  Traffic identifier value
//  RETURN:      List item corresponding to address
//               NULL otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
Dot11e_HCCA_cappollListStationItem*
MacDot11eHcPollListGetItemWithGivenAddress(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address lookupAddress,
    int tsid)
{
    DOT11e_HcVars* hc = dot11->hcVars;
    Dot11e_HCCA_cappollListStationItem* next = NULL;
    Dot11e_HCCA_cappollListStationItem* first;
    int tsSeq = tsid - DOT11e_HCCA_MIN_TSID;

    if (!MacDot11eHCPollListTSIDIsEmpty(
        node, dot11, hc->cfPollList, tsSeq + DOT11e_HCCA_MIN_TSID))
    {
        next = first = MacDot11eHCPollTSIDListGetFirstStation(
            node, dot11, tsSeq + DOT11e_HCCA_MIN_TSID);

        do {
            if (next->data->macAddr == lookupAddress) {
                return next;
            }
            next = MacDot11eHCPollTSIDListGetNextStation(
                   node, dot11, next, tsSeq + DOT11e_HCCA_MIN_TSID);

        } while (next != first);

    }

    return NULL;
}//MacDot11eHcPollListGetItemWithGivenAddress

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcSendsPolls
//  PURPOSE:     Determine if HC delivery mode is one in which it sends
//               polls and has a polling list.
//
//  PARAMETERS:  const DOT11e_HcVars* const hc
//                  HC structure
//  RETURN:      TRUE if HC operates in one of the polling modes
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

BOOL MacDot11eCfpHcSendsPolls(
    const DOT11e_HcVars* const hc)
{
    ERROR_Assert(hc != NULL,
        "MacDot11eCfpHcSendsPolls: HC variable is NULL.\n");


    return ( (hc->deliveryMode == DOT11e_HC_POLL_AND_DELIVER ) ||
             (hc->deliveryMode == DOT11e_HC_POLL_ONLY) );
}//MacDot11eCfpHcSendsPolls

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcPollListUpdateForUnicastDataReceived
//  PURPOSE:     Update poll list when HC receives a unicast from BSS
//               station.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Node address of unicast sender station
//              int tsid
//                  Traffic identifier value
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11eHcPollListUpdateForUnicastDataReceived(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address sourceAddr,
    int tsid)
{
    Dot11e_HCCA_cappollListStationItem* matchedListItem;

   if (!(MacDot11IsHC(dot11))) {
        return;
    }

   matchedListItem = MacDot11eHcPollListGetItemWithGivenAddress(
            node, dot11, sourceAddr, tsid);

    if (matchedListItem) {
        matchedListItem->data->nullDataCount = 0;
    }
}//MacDot11eHcPollListUpdateForUnicastDataReceived

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpCheckForOutgoingPacket.
//  PURPOSE:     Check if a packet needs to be dequeued.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11eCfpCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11)
{

    MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
        node, dot11);

}//MacDot11eCfpCheckForOutgoingPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationProcessAck
//  PURPOSE:     Station receives an ack.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address of frame containing ack
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationProcessAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr)
{
    int tsid = dot11->currentTsid;

    if (dot11->waitingForAckOrCtsFromAddress == INVALID_802ADDRESS
        || dot11->waitingForAckOrCtsFromAddress != sourceAddr)
    {
        return;
    }

    DOT11_SeqNoEntry *entry;

    // Reset retry counts since frame is acknowledged.
    MacDot11StationResetAckVariables(dot11);

    if (dot11->TxopMessage != NULL &&
        dot11->cfprevstate != DOT11e_X_CFP_NULLDATA)
    {
        if (!(MacDot11IsHC(dot11)))
        {
            MacDot11eRetrieveFirstPacketofTSIDFromTSsBuffer(
                        node,dot11,tsid);
        }

        MAC_MacLayerAcknowledgement(node, dot11->myMacData->interfaceIndex,
            dot11->TxopMessage, sourceAddr);

        // Remove frame from queue since already acknowledged.
        MESSAGE_Free(node, dot11->TxopMessage);

        dot11->TxopMessage = NULL;
        dot11->hcVars->nextHopAddress = ANY_MAC802;
    }

    // Update exchange sequence number.
    entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr, dot11->currentTsid);
    ERROR_Assert(entry,
        "MacDot11eCfpStationProcessAck: "
        "Unable to find sequence number entry.\n");

    entry->toSeqNo += 1;

    dot11->ipNextHopAddr = ANY_MAC802;
    dot11->ipDestAddr = ANY_MAC802;
    dot11->ipSourceAddr = ANY_MAC802;
    dot11->dataRateInfo = NULL;

    MacDot11StationCancelTimer(node, dot11);
    MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);

}//MacDot11eCfpStationProcessAck

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcProcessUnicastData
//  PURPOSE:     Receive a directed unicast frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      TRUE is unicast is acceptable
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11eHcProcessUnicastData(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    BOOL unicastIsProcessed = FALSE;
    DOT11e_FrameHdr* fHdr = (DOT11e_FrameHdr*)MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    char macAdder[24];
    int tsid = fHdr->qoSControl.TID;
    if (DEBUG_HCCA)
    {
        MacDot11MacAddressToStr(macAdder,&fHdr->destAddr);
        printf("\n%d received from %s with value of Duration/ID "
            "field is%d \n",
                node->nodeId,
                macAdder,
                fHdr->duration);
    }
    if (fHdr->qoSControl.TID < DOT11e_HCCA_MIN_TSID)
    {
        if (DEBUG_HCCA)
        {
            printf("\n Unknown TID, orig=%d,seq=%d\n",
                msg->originatingNodeId,msg->sequenceNumber);
        }
        ERROR_ReportError("MacDot11eHcProcessUnicastData:"
                           "Unknown TID received");
    }
    if (MacDot11StationCorrectSeqenceNumber(node,
                                            dot11,
                                            sourceAddr,
                                            fHdr->seqNo,
                                            fHdr->qoSControl.TID))
    {
        MacDot11Trace(node, dot11, msg, "Receive");

        dot11->totalNoOfQoSCFDataFramesReceived++;
        dot11->totalNoOfQoSDataFrameReceived++;

        dot11->hcVars->dataRateTypeForAck =
            PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

        MacDot11StationHandOffSuccessfullyReceivedUnicast(node, dot11, msg);
        unicastIsProcessed = TRUE;
    }
    else {
        MacDot11Trace(node, dot11, NULL, "Drop, wrong sequence number");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        unicastIsProcessed = FALSE;
        MESSAGE_Free(node, msg);
    }

    MacDot11eHcPollListUpdateForUnicastDataReceived(
        node,dot11,sourceAddr, tsid);

    MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);

    MacDot11StationCancelTimer(node, dot11);

    dot11->cfpAckIsDue = TRUE;

    return unicastIsProcessed;
}//MacDot11eHcProcessUnicastData


//--------------------------------------------------------------------------
//  NAME:        MacDot11eSetStationParametersforNullData
//  PURPOSE:     HC receives NULL Data from station. Update station
//               parameters for null data received.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Address of the station
//               int tsid
//                  TS ID for which null data is received.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eSetStationParametersforNullData(Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    int tsid)
{
    Dot11e_HCCA_cappollListStationItem *tmpLink =
        MacDot11eHcPollListGetItemWithGivenAddress(
            node,dot11,sourceAddr,tsid);
    if (tmpLink != NULL)
    {
        tmpLink->data->nullDataCount++;
    }
}//MacDot11eSetStationParametersforNullData


//--------------------------------------------------------------------------
//  NAME:        MacDot11eStationSetState.
//  PURPOSE:     Set the state of the node.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacStates state
//                  The state that is being set to.
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
//static //inline
void MacDot11eStationSetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11e_CapStates state)
{
    dot11->cfprevstate = dot11->cfstate;
    dot11->cfstate = state;
}// MacDot11StationSetState


//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcSendPollToStation
//  PURPOSE:     HC sends poll to station.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddress
//                  Address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static void
MacDot11eHcSendPollToStation(Node* node,
                        MacDataDot11* dot11,
                        Mac802Address destAddress)
{
    DOT11e_FrameHdr* hdr;
    Message* pktToPhy;
    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                    pktToPhy,
                    sizeof(DOT11e_FrameHdr),
                    TRACE_DOT11);

    hdr = (DOT11e_FrameHdr*)MESSAGE_ReturnPacket(pktToPhy);

    MacDot11eStationSetFieldsInDataFrameHdr(dot11,hdr,destAddress,
        DOT11_QOS_CF_POLL,dot11->currentACIndex);

    dot11->waitingForAckOrCtsFromAddress = hdr->destAddr;
    if (DEBUG_HCCA)
    {
        char timeStr[100];
        ctoa(node->getNodeTime(), timeStr);
        char macAdder[24];
        MacDot11MacAddressToStr(macAdder,&hdr->destAddr);

        printf("\n%d Sending Poll to %s at %s with Duration %d\n",
                node->nodeId,
                macAdder,
                timeStr, hdr->duration);

    }

    MacDot11eCfpHcSetTransmitState(dot11,DOT11_QOS_CF_POLL,hdr->destAddr);
    dot11->hcVars->polledTime = node->getNodeTime();

    MacDot11StationStartTransmittingPacket(
        node, dot11, pktToPhy, dot11->sifs);
}//MacDot11eHcSendPollToStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcHandleNullData
//  PURPOSE:     Handle NULL Data.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address of frame containing null data
//              Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHcHandleNullData(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    Message *msg)
{
    DOT11e_FrameHdr* hdr = (DOT11e_FrameHdr*)MESSAGE_ReturnPacket(msg);
    MacDot11eSetStationParametersforNullData(node, dot11,
        sourceAddr, hdr->qoSControl.TID);

    dot11->totalQoSNullDataReceived++;

    MacDot11StationCancelTimer(node, dot11);

    MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);

    // Update sequence number
    if (MacDot11StationCorrectSeqenceNumber(node,
                                            dot11,
                                            sourceAddr,
                                            hdr->seqNo,
                                            hdr->qoSControl.TID))
    {
    DOT11_SeqNoEntry* entry;
    dot11->TSID = hdr->qoSControl.TID;
    dot11->currentTsid = dot11->TSID;
    entry = MacDot11StationGetSeqNo(node,
                                    dot11,
                                    sourceAddr,
                                    hdr->qoSControl.TID);
    ERROR_Assert(entry,
        "MacDot11eHcHandleNullData: "
        "Unable to create or find sequence number entry.\n");
    entry->fromSeqNo += 1;
    }
    dot11->cfpAckIsDue = TRUE;
    dot11->hcVars->dataRateTypeForAck =
        PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

}//MacDot11eHcHandleNullData


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpSetState
//  PURPOSE:     Set the CFP state. Save previous state.
//  PARAMETERS:  MacDataDot11 * const dot11
//                  Pointer to dot11 structure.
//               DOT11_CfpStates state
//                  New CFP state to be set
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpSetState(
    MacDataDot11* const dot11,
    DOT11e_CapStates state)
{
    dot11->cfprevstate = dot11->cfstate;
    dot11->cfstate = state;

}//MacDot11eCfpSetState


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcTransmitAck
//  PURPOSE:     Transmit an ack in case HC receives unicast data
//               from QSTA that does not support Q-Ack.
//
//               Note that ACK is not CF-Ack.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcTransmitAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    Message* pktToPhy;
    DOT11_ShortControlFrame sHdr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_SHORT_CTRL_FRAME_SIZE, TRACE_DOT11);

    sHdr.frameType = DOT11_ACK;
    sHdr.destAddr = destAddr;
    sHdr.duration = 0;

    if (DEBUG_HCCA)
    {
        char macAdder[24];
        MacDot11MacAddressToStr(macAdder,&sHdr.destAddr);
        printf("HC transmit ACK to %s\n",macAdder);
    }
    memcpy(MESSAGE_ReturnPacket(pktToPhy), &(sHdr),
           DOT11_SHORT_CTRL_FRAME_SIZE);

    MacDot11eCfpHcSetTransmitState(dot11,DOT11_ACK,sHdr.destAddr);

    MacDot11StationStartTransmittingPacket(
        node, dot11, pktToPhy, dot11->sifs);
}//MacDot11eCfpHcTransmitAck


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpSetEndVariables
//  PURPOSE:     Set states and variables when ending CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpSetEndVariables(
    Node* node,
     MacDataDot11* dot11)
{
    MacDot11StationCancelTimer(node, dot11);
    dot11->cfpEndSequenceNumber = 0;

    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);

    MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
}//MacDot11eCfpSetEndVariables

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHandleCapEnd
//  PURPOSE:     End the CAP. Stations end the cap after the ack is received.
//               HC end the cap when cap duration is expired or no more
//               stations to poll.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eCfpHandleCapEnd(
    Node* node,
    MacDataDot11* dot11)
{
    if (!MacDot11IsInCap(dot11->state)) {
        return;
    }

    if (MacDot11IsHC(dot11)) {
        MacDot11Trace(node, dot11, NULL, "HC CAP End ");

        dot11->hcVars->txopDurationGranted = 0;
        dot11->hcVars->pollingToHc = TRUE;
        MacDot11eHcSendPollToStation(node,dot11,dot11->selfAddr);
    }
    else if (MacDot11IsBssStation(dot11)) {
        MacDot11Trace(node, dot11, NULL, "Station CF End timer");

        MacDot11eCfpSetEndVariables(node, dot11);
        //MacDot11StationCancelTimer(node, dot11);
        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);

    }
}//MacDot11CfpHandleCapEnd


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcHasSufficientTime
//  PURPOSE:     Determine if HC has sufficient time to send a frame type.
//               The frame type determines the frame exchange sequence
//               that would occur. If time was insufficient, the HC would
//               end the CAP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame types
//               clocktype sendDelay
//                  Delay before sending the frame
//               Mac802Address destAddr
//                  Destination address of frame type
//               int dataRateType
//                  Data rate type to use for calculating the duration
//  RETURN:      TRUE if there is sufficient time in CFP to send the frame
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpHcHasSufficientTime(
    Node* node,
     MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype sendDelay,
    Mac802Address destAddr,
    int dataRateType)
{
    clocktype txopRequiredDuration = 0;
    clocktype currentMessageDuration;
    int dataRateTypeForResponse= dataRateType;

    clocktype cfpBalanceDuration =
        dot11->hcVars->capEndTime - node->getNodeTime();

    switch (frameType) {
        case DOT11_QOS_DATA:
        case DOT11_QOS_DATA_POLL:

            currentMessageDuration =
                dot11->hcVars->txopDurationGranted;
            // Unicast needs
            //  sendDelay + msg time + SIFS + ack
            txopRequiredDuration =
                sendDelay
                + currentMessageDuration
                + dot11->extraPropDelay
                + dot11->sifs
                + PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateTypeForResponse,
                    sizeof(DOT11_ShortControlFrame))
                + dot11->extraPropDelay;

            break;

        case DOT11_QOS_CF_POLL:
            {
            // HC poll needs
            //  sendDelay + Poll + SIFS + Data + SIFS + Ack

            clocktype minTxop = PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateTypeForResponse,
                    100 + sizeof(DOT11e_FrameHdr) +
                    sizeof(DOT11_ShortControlFrame))
                + sendDelay
                + dot11->extraPropDelay
                + dot11->sifs
                + dot11->extraPropDelay;

            clocktype maxTxop = PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateTypeForResponse,
                    DOT11_FRAG_THRESH + sizeof(DOT11e_FrameHdr) +
                    sizeof(DOT11_ShortControlFrame))
                + sendDelay
                + dot11->extraPropDelay
                + dot11->sifs
                + dot11->extraPropDelay;

            if (cfpBalanceDuration > maxTxop)
            {
                txopRequiredDuration = maxTxop;
            }
            else if (cfpBalanceDuration > minTxop)
            {
                txopRequiredDuration = cfpBalanceDuration - EPSILON_DELAY;
            }
            else
            {
                txopRequiredDuration = minTxop;
            }

            break;
            }
        default:
            ERROR_ReportError("MacDot11CfpHcHasSufficientTime: "
                "Unknown frame type.\n");
            break;
    } //switch

     if (txopRequiredDuration <= cfpBalanceDuration)
    {
        dot11->hcVars->txopDurationGranted = txopRequiredDuration;
        return TRUE;
    }
     else
         return FALSE;

}//MacDot11CfpHcHasSufficientTime


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcAdjustFrameTypeForTime
//  PURPOSE:     Adjust frame type intended to be sent for balance CAP.
//               In case time is insufficient, possible transitions are
//                  data+poll   ->  poll
//                  data        ->  end
//                  poll        ->  end
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//               DOT11_MacFrameType* frameToSend
//                  Frame type
//               clocktype sendDelay
//                  Delay before transmit
//               int dataRateType
//                  Data rate at which to transmit
//  RETURN:      Modified frame type in case time is insufficient.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpHcAdjustFrameTypeForTime(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address* destAddr,
    DOT11_MacFrameType* frameToSend,
    clocktype sendDelay,
    int* dataRateType)
{
    if (*frameToSend == DOT11_CF_NONE) {
        return;
    }

   if (! MacDot11CfpHcHasSufficientTime(node, dot11,
            *frameToSend, sendDelay, *destAddr,*dataRateType))
    {
        switch (*frameToSend) {
            case DOT11_QOS_DATA_POLL:
                *frameToSend = DOT11_QOS_CF_POLL ;
                if (! MacDot11CfpHcHasSufficientTime(node, dot11,
                        *frameToSend, sendDelay, *destAddr,*dataRateType))
                {
                    *frameToSend = DOT11_CF_NONE;
                }
                break;

            case DOT11_QOS_CF_POLL:
            case DOT11_QOS_DATA:
                *frameToSend = DOT11_CF_NONE;
                dot11->hcVars->capPeriodExpired=
                    dot11->hcVars->capMaxDuration;
                break;

            default:
                ERROR_ReportError("MacDot11CfpHcAdjustFrameTypeForTime: "
                    "Unexpected frame type.\n");
                break;
        } //switch
    }
}//MacDot11CfpHcAdjustFrameTypeForTime


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcGetTxDataRate
//  PURPOSE:     Determine transmit data rate based on frame type,
//               destination address and ack destination (if ack is due).
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const  MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address of frame
//               DOT11_MacFrameType frameToSend
//                  Frame type to send
//               int* dataRateType
//                  Data rate to use for transmit.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpHcGetTxDataRate(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address destAddr,
    DOT11_MacFrameType frameToSend,
    int* dataRateType)
{
    DOT11e_HcVars* hc = dot11->hcVars;

    *dataRateType = dot11->lowestDataRateType;

    if (frameToSend == DOT11_CF_NONE) {
        return;
    }

    switch (frameToSend) {

        case DOT11_QOS_DATA:
            *dataRateType = dot11->dataRateInfo->dataRateType;
            break;

        case DOT11_QOS_DATA_POLL:
        case DOT11_QOS_CF_POLL:
            *dataRateType = dot11->highestDataRateTypeForBC;
            break;

        case DOT11_QOS_CF_NULLDATA:
        case DOT11_CF_ACK:
        default:
            ERROR_ReportError("MacDot11CfpHcGetTxDataRate: "
                "Unknown or unexpected frame type.\n");
            break;
    }

    if (dot11->cfpAckIsDue) {
        // It may occur that the destination for Ack is
        // also the destination for the current frame and
        // the current data rate is higher.
        // A conservative approach is taken here.
        *dataRateType = MIN(*dataRateType, hc->dataRateTypeForAck);
    }
}//MacDot11CfpHcGetTxDataRate

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcTransmitPoll
//  PURPOSE:     HC transmit poll
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const  MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address of frame
//               DOT11_MacFrameType frameToSend
//                  Frame type to send
//               clocktype sendDelay
//                  Delay with which poll has to be send
//               Dot11e_HCCA_cappollListStationItem **stationItem
//
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

static
void MacDot11eHcTransmitPoll(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address *destAddr,
    DOT11_MacFrameType *frameToSend,
    clocktype sendDelay,
    Dot11e_HCCA_cappollListStationItem **stationItem)
{
    int tsid = dot11->hcVars->currentTSID;
    Dot11e_HCCA_cappollListStationItem* station = NULL;
    BOOL endCAP = FALSE;

    while (!endCAP)
    {
        if (!MacDot11eHCPollListTSIDIsEmpty(node,
            dot11,
            dot11->hcVars->cfPollList,
            tsid))
        {
            station = MacDot11eHCPollTSIDListGetNextStationToPoll(node,
                dot11, tsid);
        }
        if (station != NULL)
        {
            dot11->hcVars->currentTSID = tsid;
            *stationItem = station;
            *destAddr = station->data->macAddr;
            *frameToSend = DOT11_QOS_CF_POLL;
            break;
        }
        else if (dot11->TxopMessage != NULL ||
            MacDot11IsTSHasMessageforTSID(node, dot11, tsid))
        {
            if (dot11->TxopMessage == NULL)
            {
                dot11->hcVars->currentTSID = tsid;
                MacDot11eRetrieveFirstPacketofTSIDFromTSsBuffer(
                            node,dot11,dot11->hcVars->currentTSID);
            }
            dot11->dataRateInfo =
                MacDot11StationGetDataRateEntry(node, dot11,
                dot11->hcVars->nextHopAddress);

            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                dot11->hcVars->nextHopAddress,
                "Address does not match");

            MacDot11StationAdjustDataRateForNewOutgoingPacket(
                node, dot11);

            dot11->hcVars->txopDurationGranted  =
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dot11->dataRateInfo->dataRateType,
                    MESSAGE_ReturnPacketSize(dot11->TxopMessage) +
                    sizeof(DOT11e_FrameHdr))+
                    dot11->extraPropDelay + dot11->sifs +
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dot11->dataRateInfo->dataRateType,
                    DOT11_SHORT_CTRL_FRAME_SIZE) +
                dot11->extraPropDelay;



            *destAddr = dot11->hcVars->nextHopAddress;
            *frameToSend = DOT11_QOS_DATA;
            dot11->hcVars->currentTSID = tsid;
            break;
        }
        else
        {
            // Current TS is polled or empty go to next TS
            if (tsid == DOT11e_HCCA_MIN_TSID)
            {
                //All TS polled. Start again
                MacDot11eHCPollListResetForAllPolled( node, dot11);
                tsid = dot11->hcVars->currentTSID;
                if (!dot11->hcVars->hasPollableStations &&
                   !MacDot11IsTSsHasMessage(node,dot11))
                {
                    endCAP = TRUE;
                }

            }
            else
            {
                tsid--;
            }
            station = NULL;
            dot11->hcVars->currPolledItem = NULL;
            dot11->hcVars->allCurrTSIDStationsPolled = FALSE;
        }
    }

    if (endCAP)
    {
        *frameToSend = DOT11_CF_NONE;
    }
}//MacDot11eHcTransmitPoll



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcTransmit
//  PURPOSE:     Governing routine for HC transmits during CAP.
//               Determines the send delay to use and calls the
//               configured delivery algorithm to determine what
//               is to be sent.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType receivedFrameType
//                  Received frame, none in case the previous frame
//                  sent by the PC was a beacon, a broadcast, or
//                  there was a timeout
//               clocktype timerDelay
//                  Timer delay, 0 if not applicable.
//  PARAMETERS:
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpHcTransmit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType receivedFrameType,
    clocktype timerDelay)
{
    DOT11e_HcVars* hc = dot11->hcVars;
    clocktype sendDelay;
    DOT11_MacFrameType frameType = DOT11_CF_NONE;
    Mac802Address destAddr;
    Dot11e_HCCA_cappollListStationItem* stationItem = NULL;
    int dataRateType;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) {
        MacDot11Trace(node, dot11, NULL, "CFP HC transmit, phy not idle.");
        return;
    }

    // In case of timeout, at HC, the timer delay is PIFS.
    // In that case, a token delay of 1 is set for transmit.
    if (timerDelay >= dot11->sifs) {
        sendDelay = EPSILON_DELAY;
    }
    else {
        sendDelay = dot11->sifs - timerDelay;
    }
    switch (dot11->cfstate)
    {
        case DOT11e_S_CFP_NONE:
                //frameType = DOT11_QOS_CF_POLL;
            MacDot11eHcTransmitPoll(
                node,
                dot11,
                &destAddr,
                &frameType,
                sendDelay,
                &stationItem);
            break;

        case DOT11e_S_CFP_WF_ACK:

            MacDot11eHcTransmitPoll(
                node,
                dot11,
                &destAddr,
                &frameType,
                sendDelay,
                &stationItem);
            break;

        case DOT11e_S_CFP_WF_DATA:
        case DOT11e_S_CFP_WF_DATA_ACK:


            MacDot11eHcTransmitPoll(
                node,
                dot11,
                &destAddr,
                &frameType,
                sendDelay,
                &stationItem);
            break;
        case DOT11e_X_QoS_CFP_POLL_To_HC:
            frameType = DOT11_QOS_DATA;
            destAddr = hc->nextHopAddress;
            break;
        default:
            ERROR_ReportError("MacDot11CfpHcTransmit: "
                "Unknown CAP state for HC POLL-AND-DELIVER mode.\n");
            break;
    } //switch


    MacDot11CfpHcGetTxDataRate(
        node, dot11, destAddr, frameType, &dataRateType);

    MacDot11CfpHcAdjustFrameTypeForTime(node, dot11,
        &destAddr, &frameType, sendDelay, &dataRateType);



    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        dataRateType);

    switch (frameType) {

        case DOT11_QOS_DATA_POLL:
        case DOT11_QOS_DATA_POLL_ACK :

            break;

        case DOT11_QOS_DATA :
        case DOT11_QOS_CF_DATA_ACK:

                MacDot11eCfpHcTransmitUnicast(node, dot11,
                    frameType,sendDelay);
                break;

        case DOT11_QOS_CF_POLL:
        case DOT11_QOS_POLL_ACK:

            if (!hc->pollingToHc)
            {
                dot11->hcVars->currPolledItem = stationItem;
            }

            MacDot11eHcSendPollToStation(node, dot11,destAddr);
            break;

        case DOT11_CF_NONE:
            //End cap
            MacDot11eCfpHandleCapEnd(node,dot11);
            break;

        default:
            ERROR_ReportError("MacDot11CfpHcTransmit: "
                "Unknown frame type.\n");
            break;
    } //switch

}//MacDot11CfpHcTransmit


//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcReceiveUnicast
//  PURPOSE:     Depending on the type of directed frame, process it
//               and arrange to respond appropriately
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHcReceiveUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_FrameHdr* fHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) fHdr->frameType;
    BOOL isProcessed;

    switch (frameType) {

        case DOT11_QOS_CF_DATA_ACK:
            MacDot11eCfpStationProcessAck(node, dot11, sourceAddr);
            dot11->totalNoOfQoSCFAckReceived++;
            // no break; continue to process DATA

        case DOT11_QOS_DATA:
            isProcessed = MacDot11eHcProcessUnicastData(node, dot11, msg);
            if (dot11->cfpAckIsDue)
                MacDot11eCfpHcTransmitAck(node,dot11,sourceAddr);
            else
                MacDot11CfpHcTransmit(node,dot11,frameType,0);
            break;

        case DOT11_QOS_CF_NULLDATA:
            MacDot11eHcHandleNullData( node, dot11, sourceAddr,msg);
            MESSAGE_Free(node, msg);

            if (dot11->cfpAckIsDue)
            {
                MacDot11eCfpHcTransmitAck(node,dot11,sourceAddr);
            }
            else
            {
                MacDot11CfpHcTransmit(node,dot11,frameType,0);
            }

            break;

        default:
            ERROR_ReportError("MacDot11CfpStationProcessUnicast: "
                "Received unknown unicast frame type.\n");
            break;
    } //switch

}//MacDot11eHcReceiveUnicast

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcTransmitUnicast
//  PURPOSE:     Transmit a unicast frame during CFP.
//               An ack and/or a poll may piggyback on the data frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//               Mac802Address destAddr
//                  Destination address
//               clocktype delay
//                  Delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcTransmitUnicast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11e_FrameHdr* fHdr;
    DOT11_SeqNoEntry *entry;

    ERROR_Assert(dot11->TxopMessage != NULL,
        "MacDot11CfpHcTransmitUnicast: Current message is NULL.\n");

    pktToPhy = MESSAGE_Duplicate(node, dot11->TxopMessage);

    if (pktToPhy!=NULL)
    {
        MESSAGE_AddHeader(node,
                    pktToPhy,
                    sizeof(DOT11e_FrameHdr),
                    TRACE_DOT11);

        fHdr = (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);


        MacDot11eStationSetFieldsInDataFrameHdr(dot11, fHdr,
            dot11->hcVars->nextHopAddress, frameType,dot11->currentACIndex);

        //Enough for receiving Ack
        fHdr->duration = (UInt16)MacDot11NanoToMicrosecond(
            dot11->extraPropDelay + dot11->sifs +
            PHY_GetTransmissionDuration(
                node, dot11->myMacData->phyNumber,
                dot11->dataRateInfo->dataRateType,
                DOT11_SHORT_CTRL_FRAME_SIZE) +
            dot11->extraPropDelay);

        dot11->TSID = fHdr->qoSControl.TID;
        dot11->currentTsid = dot11->TSID;
        entry = MacDot11StationGetSeqNo(node,
                                        dot11,
                                        dot11->hcVars->nextHopAddress,
                                        fHdr->qoSControl.TID);
        ERROR_Assert(entry,
            "MacDot11eCfpHcTransmitUnicast: "
            "Unable to find or create sequence number entry.\n");
        fHdr->seqNo = entry->toSeqNo;

        dot11->waitingForAckOrCtsFromAddress = dot11->hcVars->nextHopAddress;
        if (DEBUG_HCCA)
        {
             char macAdder[24];
            MacDot11MacAddressToStr(macAdder,&fHdr->destAddr);
            printf("\n%d sending Data frame to %s with value of Duration/ID"
                " field is%d \n",
                    node->nodeId,
                    macAdder,
                    fHdr->duration);
        }
        if (MESSAGE_ReturnPacketSize(pktToPhy) >
            DOT11_FRAG_THRESH)
        {
            // Fragmentation not currently supported
            ERROR_ReportError("MacDot11CfpHcTransmitUnicast: "
                "Fragmentation not supported.\n");
        }
        else {
            fHdr->fragId = 0;
            MacDot11eCfpHcSetTransmitState(
                dot11, frameType, dot11->hcVars->nextHopAddress);

            MacDot11StationStartTransmittingPacket(
                node, dot11, pktToPhy, delay);
        }
    }
}//MacDot11eCfpHcTransmitUnicast
//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcUnicastTransmitted
//  PURPOSE:     Set states and timers once a unicast has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcUnicastTransmitted(
    Node* node,
    MacDataDot11* dot11)
{

    MacDot11eCfpHcSetPostTransmitState(node,dot11);
    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);

}//MacDot11eCfpHcUnicastTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcAckTransmitted
//  PURPOSE:     Continue in PCF by setting an SIFS timer
//               after transmit of an ACK.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcAckTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11eCfpHcSetPostTransmitState(node,dot11);

    MacDot11StationStartTimer(node, dot11, dot11->txSifs);

}//MacDot11eCfpHcAckTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcPollSelfTransmitted
//  PURPOSE:     Handle states and timers once a Cf Poll
//               has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcPollSelfTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->hcVars->txopDurationGranted != 0)
    {
        MacDot11eCfpHcSetPostTransmitState(node,dot11);

        MacDot11StationStartTimer(node, dot11, dot11->txSifs);
    }
    else
    {
        // CFP end timer expires but unable to send CF End.
        // Switch to DCF state.
        MacDot11eCfpSetEndVariables(node, dot11);
        dot11->hcVars->capPeriodExpired +=
        node->getNodeTime()-dot11->hcVars->capStartTime;
        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
    }
}//MacDot11eCfpHcPollSelfTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcPollTransmitted
//  PURPOSE:     Handle states and timers once a Cf Poll
//               has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcPollTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11eCfpHcSetPostTransmitState(node,dot11);
    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);

}//MacDot11eCfpHcPollTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationSetPostTransmitState
//  PURPOSE:     Set that state of BSS station during CFP after frame
//               transmission is complete.
//
//               Also reset ack flag is the sent frame was an ack type.
//
//  PARAMETERS:   MacDataDot11 * const dot11
//                  Pointer to Dot11 strcuture
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationSetPostTransmitState(
     MacDataDot11* const dot11)
{
    switch (dot11->cfstate) {

        case DOT11e_X_CFP_UNICAST:
        case DOT11e_X_CFP_NULLDATA:
            if (DEBUG_HCCA)
            {
                printf("cfp state has been changed to DOT11e_S_CFP_WF_ACK");
            }
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_WF_ACK);
            break;

        case DOT11e_X_ACK:
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
            break;

        default:
            ERROR_ReportError("MacDot11eCfpStationSetPostTransmitState: "
                "Unknown transmit state.\n");
            break;

    } //switch


    switch (dot11->cfpLastFrameType) {
        case DOT11_QOS_CF_DATA_ACK:
        case DOT11_ACK:
            dot11->cfpAckIsDue = FALSE;
            break;
        default:
            break;
    } //switch

}//MacDot11eCfpStationSetPostTransmitState
//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationUnicastTransmitted
//  PURPOSE:     Set states/timers after a station transmits a unicast
//               during CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationUnicastTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->dataPacketsSentCfp++;

    MacDot11eCfpStationSetPostTransmitState(dot11);
    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);

}//MacDot11eCfpStationUnicastTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationAckTransmitted
//  PURPOSE:     Set state once an ack transmit is complete.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationAckTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11eCfpStationSetPostTransmitState(dot11);

}//MacDot11eCfpStationAckTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationNullDataTransmitted
//  PURPOSE:     Set state once a nulldata transmit is complete.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationNullDataTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11eCfpStationSetPostTransmitState(dot11);
    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);

}//MacDot11eCfpStationNullDataTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpFrameTransmitted
//  PURPOSE:     Once PHY indicates that a frame has been transmitted
//               during CFP, call the appropriate function for post
//               transmit processing.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eCfpFrameTransmitted(
    Node* node,
    MacDataDot11* dot11)
{

    switch (dot11->cfstate) {

    case DOT11e_X_QoS_DATA_ACK:
    case DOT11e_X_CFP_UNICAST:

        if (MacDot11IsHC(dot11)) {
            MacDot11eCfpHcUnicastTransmitted(node, dot11);
        }
        else {
            MacDot11eCfpStationUnicastTransmitted(node, dot11);
        }
        break;

    case DOT11e_X_POLL_DATA_ACK :
        if (MacDot11IsHC(dot11)) {
            MacDot11eCfpHcAckTransmitted(node, dot11);
        }
        else {
            ERROR_ReportError("MacDot11eCfpFrameTransmitted: "
            "Unknown CFP state.\n");
        }
        break;

     case DOT11e_X_QoS_DATA_POLL:
     case DOT11e_X_QoS_CFP_POLL:
         MacDot11eCfpHcPollTransmitted(node, dot11);
         break;
     case DOT11e_X_QoS_CFP_POLL_To_HC:
        MacDot11eCfpHcPollSelfTransmitted(node, dot11);
        break;

    case DOT11e_X_ACK:
      if (MacDot11IsHC(dot11))
      {
          MacDot11eCfpHcAckTransmitted(node, dot11);
          MacDot11eCfpCheckForOutgoingPacket(node,dot11);
      }
      else
      {
          MacDot11eCfpStationAckTransmitted(node, dot11);
      }
      break;

    case DOT11e_X_CFP_NULLDATA:
        if (!MacDot11IsHC(dot11))
        MacDot11eCfpStationNullDataTransmitted(node, dot11);
       break;

    default:
        ERROR_ReportError("MacDot11eCfpFrameTransmitted: "
            "Unknown CFP state.\n");
        break;

    } //switch

}//MacDot11eCfpFrameTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcSetPostTransmitState
//  PURPOSE:     Set that state of HC during CFP after frame
//               transmission is complete.
//
//               Also reset ack flag is the sent frame was an ack type.
//
//  PARAMETERS:  Node* node
//                  Ponter to Node structure
//               MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcSetPostTransmitState(Node* node,
     MacDataDot11* const dot11 )
{

    clocktype currentTime = node->getNodeTime();
    switch (dot11->cfstate) {


        case DOT11e_X_CFP_UNICAST:
            if (DEBUG_HCCA)
            {
                printf("cfp state has been changed to DOT11e_S_CFP_WF_ACK");
            }
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_WF_ACK);
            break;

        case DOT11e_X_QoS_CFP_POLL:
            if (DEBUG_HCCA)
            {
                printf("cfp state has ben changed to DOT11e_S_CFP_WF_DATA");
            }
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_WF_DATA);
            break;

        case DOT11e_X_QoS_CFP_POLL_To_HC:
            //MacDot11eCfpSetState(dot11,DOT11e_X_CFP_UNICAST);
            break;

        case DOT11e_X_QoS_DATA_ACK:

            if ((currentTime - dot11->hcVars->polledTime) <
                dot11->hcVars->txopDurationGranted)
            {
                if (DEBUG_HCCA)
                {
                    printf("cfp state has ben changed to"
                        " DOT11e_S_CFP_WF_DATA");
                }
                MacDot11eCfpSetState(dot11, DOT11e_S_CFP_WF_DATA);
            }
            else
            {
                MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
                //MacDot11eSetContentionFreeMode(node, dot11);
            }
            break;

        case DOT11e_X_ACK:
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcSetPostTransmitState: "
                "Unknown transmit state.\n");
            break;

    } //switch

  switch (dot11->cfpLastFrameType) {
        case DOT11_QOS_CF_DATA_ACK:
        case DOT11_QOS_DATA_POLL_ACK:
        case DOT11_QOS_POLL_ACK:
        case DOT11_ACK:
            dot11->cfpAckIsDue = FALSE;
            break;
        default:
            break;
    } //switch
}//MacDot11eCfpHcSetPostTransmitState

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationRetransmit
//  PURPOSE:     CFP Station Retransmit
//
//  PARAMETERS:  Node* node
//                  Ponter to Node structure
//               MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

void MacDot11eCfpStationRetransmit(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL RetryTheTransmission = FALSE;

    ERROR_Assert(dot11->TxopMessage != NULL,
        "MacDot11eCfpStationRetransmit: "
        "There is no message to retransmit.\n");

    // Use Long retry count for data frames.
    dot11->SLRC++;
    if (dot11->SLRC < dot11->longRetryLimit) {
        RetryTheTransmission = TRUE;
    }
    else {
        RetryTheTransmission = FALSE;
        dot11->SLRC = 0;
    }


    if (!RetryTheTransmission){
        // Exceeded maximum retry count allowed, so drop frame
        dot11->pktsDroppedDcf++;

        // Drop frame from queue
        MacDot11Trace(node, dot11, NULL, "Drop, exceeds retransmit count");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, dot11->TxopMessage, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        MESSAGE_Free(node, dot11->TxopMessage);
        dot11->TxopMessage = NULL;

        dot11->hcVars->nextHopAddress = ANY_MAC802;
        dot11->ipNextHopAddr = ANY_MAC802;
        dot11->ipDestAddr = ANY_MAC802;
        dot11->ipSourceAddr = ANY_MAC802;
        dot11->dataRateInfo = NULL;
    }
} // MacDot11eCfpStationRetransmit

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHandleTimeout
//  PURPOSE:     Handle timeouts during CFP depending on state and
//               occurance at PC or station.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eCfpHandleTimeout(
    Node* node,
    MacDataDot11* dot11)
{
     switch (dot11->cfstate) {
        case DOT11e_S_CFP_NONE:
            ERROR_Assert(MacDot11IsHC(dot11),
                "MacDot11eCfpHandleTimeout: "
                "Unexpected station timeout in CFP_NONE state.\n");
           MacDot11CfpHcTransmit(node, dot11, DOT11_CF_NONE,
               dot11->txSifs);

            break;


        case DOT11e_S_CFP_WF_ACK:
            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                         dot11->hcVars->nextHopAddress,
                         "MacDot11eCfpHandleTimeout: "
                         "Address does not match.\n");

            MacDot11StationResetAckVariables(dot11);

            if (MacDot11IsHC(dot11))
            {
                MacDot11Trace(node, dot11, NULL,
                    "HC timeout waiting for ack.");

                dot11->dataRateInfo->numAcksFailed++;
                MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);

                // Continue with transmit
                MacDot11CfpHcTransmit(node, dot11, DOT11_CF_NONE, 0);
            }
            else
            {
                MacDot11Trace(node, dot11, NULL,
                    "Station timeout waiting for ack.");
                dot11->TxopMessage = NULL;

                // For stations, End CAP
                MacDot11eCfpHandleCapEnd(node,dot11);
            }
            break;

        case DOT11e_S_CFP_WF_DATA_ACK:
            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                         dot11->currentNextHopAddress,
                         "MacDot11eCfpHandleTimeout: "
                         "Address does not match.\n");

            dot11->dataRateInfo->numAcksFailed++;
            MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);

            MacDot11StationResetAckVariables(dot11);

            // no break

        case DOT11e_S_CFP_WF_DATA:
            ERROR_Assert(MacDot11IsHC(dot11),
                "MacDot11CfpHandleTimeout: "
                "Non-HC times out waiting for data.\n");

            MacDot11Trace(node, dot11, NULL,
                "HC timeout waiting for data (+ack).");

            // Continue with transmit
            MacDot11CfpHcTransmit(node, dot11, DOT11_CF_NONE, 0);
            break;

        case DOT11e_X_CFP_UNICAST:
               MacDot11CfpHcTransmit(node,dot11, DOT11_QOS_DATA,0);
               break;

        case DOT11e_X_QoS_CFP_POLL_To_HC:
            MacDot11CfpHcTransmit(node,dot11, DOT11_QOS_DATA,0);
            break;

        default:
            ERROR_ReportError("MacDot11eCfpHandleTimeout: "
                "Unknown CFP timeout.\n");
            break;
    } //switch
}//MacDot11eCfpHandleTimeout

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcSetTransmitState
//  PURPOSE:     Set the state of the HC based on the frame about to
//               be transmitted during CFP.
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType * const frameType
//                  Frame type
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      Adjusted frame type
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcSetTransmitState(
     MacDataDot11* const dot11,
    DOT11_MacFrameType frameType,
    Mac802Address destAddr)
{
    dot11->cfpLastFrameType = frameType;


    switch (frameType) {

        case DOT11_QOS_DATA:
        case DOT11_QOS_CF_DATA_ACK:

                MacDot11eCfpSetState(
                    dot11,DOT11e_X_CFP_UNICAST);
            break;

        case DOT11_QOS_DATA_POLL:
        case DOT11_QOS_DATA_POLL_ACK:
            if (DEBUG_HCCA)
            {
                printf("state has been changed to DOT11e_X_QoS_CFP_POLL");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_QoS_CFP_POLL);
            break;

        case DOT11_QOS_CF_POLL:
        case DOT11_QOS_POLL_ACK :
            if (dot11->hcVars->pollingToHc==TRUE)
            {
                if (DEBUG_HCCA)
                {
                    printf("cfp has been changed to"
                        " DOT11e_X_QoS_CFP_POLL_To_HC");
                }
                MacDot11eCfpSetState(dot11, DOT11e_X_QoS_CFP_POLL_To_HC);
                dot11->hcVars->pollingToHc=FALSE;
            }

            else
            {
                if (DEBUG_HCCA)
                {
                    printf("state has been changed to "
                        "DOT11e_X_QoS_CFP_POLL");
                }
                MacDot11eCfpSetState(dot11, DOT11e_X_QoS_CFP_POLL);
            }
            break;
        case DOT11_ACK:
            if (DEBUG_HCCA)
            {
                printf("cfp state has been changed to DOT11e_X_ACK\n");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_ACK);
            break;

        default:
            ERROR_ReportError("MacDot11eCfpHcSetTransmitState: "
                "Unknown frame type.\n");
            break;

    } //switch

}//MacDot11eCfpHcSetTransmitState
//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpPrintStats
//  PURPOSE:     Print PCF related statistics.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to DOT11 structure
//               int interfaceIndex
//                  interfaceIndex, Interface index
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
//static
void MacDot11eCfpPrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    // Print HCCA stats for HC and BSS station.
    // Skip stats if station is not part of coordinated subnet
    //if (!dot11->printCfpStatistics) {
   //     return;
   // }

    if (MacDot11IsHC(dot11)) {
        MacDot11eCfpHcPrintStats(node, dot11, interfaceIndex);
    }
    else if (MacDot11IsBssStation(dot11) &&
        (dot11->totalNoOfQoSCFDataFrameSend > 0 ||
        dot11->totalNoOfQoSDataCFPollsReceived > 0 ||
              dot11->totalNoOfQoSCFPollsReceived > 0))
    {
        MacDot11eCfpStationPrintStats(node, dot11, interfaceIndex);
    }
}//MacDot11eCfpPrintStats

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcPrintStats
//  PURPOSE:     Print CFP statistics for a PC.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int interfaceIndex
//                  Interface index
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcPrintStats(
    Node* node,
     MacDataDot11* dot11,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "HCCA: Polls transmitted = %d",
           dot11->totalNoOfQoSCFPollsTransmitted );
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: Data packets transmitted = %d",
        dot11->totalNoOfQoSCFDataFrameSend);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: Data packets received = %d",
        dot11->totalNoOfQoSCFDataFramesReceived);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: NullData received = %d",
        dot11->totalQoSNullDataReceived);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

}//MacDot11eCfpHcPrintStats


//--------------------------------Polling Functions------------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListTSIDIsEmpty
//  PURPOSE:     See if the HCCA poll list of particular TSID is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapPollList* list
//                  Pointer to polling list
//               int TSIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      TRUE if HCCA poll list size is zero
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
bool MacDot11eHCPollListTSIDIsEmpty(
     Node* node,
     MacDataDot11* dot11,
     DOT11e_HCCA_CapPollList* list,
     int tsIdentifier)
{
     int tsSeq = tsIdentifier - DOT11e_HCCA_MIN_TSID;
     return dot11->hcVars->cfPollList->TS[tsSeq].size == 0;

}//MacDot11eHCPollListTSIDIsEmpty
//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListIsEmpty
//  PURPOSE:     See if the HCCA poll list of particular TSID is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if HCCA poll list size is zero
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
//inline
bool MacDot11eHCPollListIsEmpty(
     Node* node,
     MacDataDot11* dot11)
{
    return dot11->hcVars->cfPollList->size == 0;

}//MacDot11eHCPollListIsEmpty

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCTSIDPollListGetSize
//  PURPOSE:     Query the size of particular TSID poll list
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int TSIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      Number of items in HCCA poll list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int MacDot11eHCTSIDPollListGetSize(
    Node* node,
    MacDataDot11* dot11,
    int TSIdentifier)
{
    int tsSeq = TSIdentifier - DOT11e_HCCA_MIN_TSID;
    return dot11->hcVars->cfPollList->TS[tsSeq].size;

}//MacDot11eHCTSIDPollListGetSize

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListInsert
//  PURPOSE:     Insert an item to the end of the list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_HCCA_CapPollList* list
//                  Pointer to polling list
//               DOT11_HCCA_CapPollStation* data
//                  Poll station data for item to insert
//               int TSIdentifier
//                  TSID value for which in list,STA has to be inserted
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11eHCPollListInsert(
    Node* node,
    MacDataDot11* dot11,
    DOT11e_HCCA_CapPollList* list,
    DOT11e_HCCA_CapPollStation* data,
    int tsIdentifier )
{
    int tsSeq = tsIdentifier - DOT11e_HCCA_MIN_TSID;
    Dot11e_HCCA_cappollListStationItem *station =
        ( Dot11e_HCCA_cappollListStationItem* )
               MEM_malloc( sizeof( Dot11e_HCCA_cappollListStationItem ) );
    station->data = data;
    station->next = NULL;

    if (list->TS[tsSeq].size == 0){
        station->prev = NULL;
        list->TS[tsSeq].last = station;
        list->TS[tsSeq].first = list->TS[tsSeq].last;
    }
    else{
        station->prev = list->TS[tsSeq].last;
        list->TS[tsSeq].last->next = station;
        list->TS[tsSeq].last = station;
    }
    list->TS[tsSeq].size++;
    list->size++;
    dot11->hcVars->hasPollableStations = TRUE;

}//MacDot11eHCPollListInsert
//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListStationItemRemove
//  PURPOSE:     Remove an station item from the poll list of given TSID.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapPollList* list
//                  Pointer to polling list
//               Dot11e_HCCA_cappollListStationItem* station
//                  Station item to remove
//               int tSIdentifier
//                  TSID value for which STA needs to be removed
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHCPollListStationItemRemove(
    Node* node,
     MacDataDot11* dot11,
    DOT11e_HCCA_CapPollList* list,
    Dot11e_HCCA_cappollListStationItem* station,
    int tSIdentifier){

    Dot11e_HCCA_cappollListStationItem* nextstation;
    int tsSeq = tSIdentifier - DOT11e_HCCA_MIN_TSID;

    if (station == NULL) {
            return;
            }


    if (list->TS[tsSeq].size == 0) {
        return;
    }

    nextstation = station->next;

    if (list->TS[tsSeq].size == 1) {
        list->TS[tsSeq].first = list->TS[tsSeq].last = NULL;
    }
    else if (list->TS[tsSeq].first == station) {
        list->TS[tsSeq].first = station->next;
        list->TS[tsSeq].first->prev = NULL;
    }
    else if (list->TS[tsSeq].last == station) {
        list->TS[tsSeq].last = station->prev;
        if (list->TS[tsSeq].last != NULL) {
            list->TS[tsSeq].last->next = NULL;
        }
    }
    else {
        station->prev->next = station->next;
        if (station->prev->next != NULL) {
            station->next->prev = station->prev;
        }
    }

    list->TS[tsSeq].size--;
    list->size--;

    if (station->data != NULL) {
        MEM_free(station->data);
    }

    MEM_free(station);

}//MacDot11eHCPollListStationItemRemove

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListFree
//  PURPOSE:     Free the entire list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList* list
//                  Pointer to polling list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHCPollListFree(
    Node* node,
     MacDataDot11* dot11,
    DOT11e_HCCA_CapPollList* list)
{
    Dot11e_HCCA_cappollListStationItem *station;
    Dot11e_HCCA_cappollListStationItem *tempStation;
    int tsSeq;
    for (tsSeq = 0 ; tsSeq < DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED ; tsSeq++) {
        station = list->TS[tsSeq].first;
        while (station) {
            tempStation = station;
            station = station->next;
            MEM_free(tempStation->data);
            MEM_free(tempStation);
        }

    }

}//MacDot11eHCPollListFree


//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetFirstStation
//  PURPOSE:     Retrieve the first item in the poll list
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int tsIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      First list item
//               NULL for empty list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
Dot11e_HCCA_cappollListStationItem * MacDot11eHCPollTSIDListGetFirstStation(
     Node* node,
     MacDataDot11* dot11,
     int tsIdentifier)
{
    int tsSeq = tsIdentifier - DOT11e_HCCA_MIN_TSID;
    return (dot11->hcVars->cfPollList->TS[tsSeq].first);

}//MacDot11eHCPollTSIDListGetFirstStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetNextStation
//  PURPOSE:     Retrieve the next item given the previous item.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Dot11e_HCCA_cappollListStationItem *prev
//                  Pointer to prev item.
//               int tsIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      First item when previous item is NULL
//                          when previous item is the last
//                          when there is only one item in the list
//               NULL if the list is empty
//               Next item after previous item
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
Dot11e_HCCA_cappollListStationItem * MacDot11eHCPollTSIDListGetNextStation(
    Node* node,
    MacDataDot11* dot11,
    Dot11e_HCCA_cappollListStationItem *prev,
    int tsIdentifier)
{
    int tsSeq = tsIdentifier - DOT11e_HCCA_MIN_TSID;
    Dot11e_HCCA_cappollListStationItem* next = NULL;
    DOT11e_HCCA_CapPollList* pollList = dot11->hcVars->cfPollList;

    if (prev == pollList->TS[tsSeq].last || prev == NULL) {
        next = pollList->TS[tsSeq].first;
    }
    else {
        next = prev->next;
    }

    return next;
}//MacDot11eHCPollTSIDListGetNextStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetNextStationToPoll
//  PURPOSE:     Retrieve the next station to poll for a given TSID
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int tsIdentifier
//                  TSID value
//  RETURN:      Next statiion to be polled
//               NULL if the list is empty
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
Dot11e_HCCA_cappollListStationItem*
MacDot11eHCPollTSIDListGetNextStationToPoll(
    Node* node,
    MacDataDot11* dot11,
    int tsIdentifier)
{
    int tsSeq = tsIdentifier - DOT11e_HCCA_MIN_TSID;
    Dot11e_HCCA_cappollListStationItem* next = NULL;
    Dot11e_HCCA_cappollListStationItem* itemToPoll = NULL;
    Dot11e_HCCA_TrafficStream* tsPollList =
        &dot11->hcVars->cfPollList->TS[tsSeq];

    if (tsPollList->size > 0 && !dot11->hcVars->allCurrTSIDStationsPolled)
    {
        next = MacDot11eHCPollTSIDListGetNextStation(node,dot11,
            dot11->hcVars->currPolledItem,tsIdentifier);
        while (next != tsPollList->firstPolledItem)
        {
            if (next->data->nullDataCount == 0)
            {
                itemToPoll = next;
                break;
            }
            else
            {
                // Received NULL Data, only poll after next beacon
                if (tsPollList->firstPolledItem == NULL)
                {
                    tsPollList->firstPolledItem = next;
                }
            }
            next = MacDot11eHCPollTSIDListGetNextStation(node, dot11,
                next, tsIdentifier);
        }

        if (next == tsPollList->firstPolledItem)
        {
            dot11->hcVars->allCurrTSIDStationsPolled=TRUE;
            itemToPoll = NULL;
        }
    }

    if (tsPollList->firstPolledItem == NULL && itemToPoll != NULL)
    {
        tsPollList->firstPolledItem = itemToPoll;
    }
    return itemToPoll;

}//MacDot11eHCPollTSIDListGetNextStationToPoll


//------------------------dowlink list function-----------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcDownlinkListInit
//  PURPOSE:     Initialize the HCCA poll link list data structure.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapPollList **list
//                  Pointer to HCCA polling list
//  RETURN:      Poll list with memory allocated
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHcDownlinkListInit(
    Node* node,
     MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList** list){

    DOT11e_HCCA_CapDownlinkPollList* tmpList;
    tmpList = (DOT11e_HCCA_CapDownlinkPollList*)
        MEM_malloc(sizeof(DOT11e_HCCA_CapDownlinkPollList));

    Queue* queue=NULL;
    queue=new Queue;
    ERROR_Assert(queue != NULL,\
            "DOT11e: Unable to allocate memory for HCCA downlink queue");
    queue->SetupQueue(node, "FIFO", DEFAULT_APP_QUEUE_SIZE, 0, 0, 0, FALSE);

        tmpList->next= tmpList->prev= NULL;
        tmpList->queue=queue;
        tmpList->macAddr=0;

    *list=tmpList;

}//MacDot11eHcDownlinkListInit
//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCDownLinkListInsert
//  PURPOSE:     Insert an item to the end of the list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapDownlinkPollList** station
//                  Pointer to polling list
//               Mac802Address destinationAddr
//                  Destination address
//               Message* msg
//                  Pointer to received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHCDownLinkListInsert(
    Node* node,
    MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList** station,
    Mac802Address destinationAddr,
    Message* msg)
{
    DOT11e_HCCA_CapDownlinkPollList *tmpstation2,*tmpstation1;
    BOOL queueIsFull = FALSE;

    tmpstation1 = *station;

    if (*station == NULL)
    {
        *station=(DOT11e_HCCA_CapDownlinkPollList*)
            MEM_malloc( sizeof( DOT11e_HCCA_CapDownlinkPollList* ) );

        MacDot11eHcDownlinkListInit(node,dot11,station);

        (*station)->prev=NULL;
        (*station)->next=NULL;
        (*station)->macAddr=destinationAddr;

        (*station)->queue->SetupQueue(
            node, "FIFO", DEFAULT_APP_QUEUE_SIZE, 0, 0, 0, FALSE);

        (*station)->queue->insert(
            msg,
            NULL,
            &queueIsFull,
            node->getNodeTime());
    }
    else
    {
        while (tmpstation1!=NULL)
            tmpstation1= tmpstation1->next;

        if (tmpstation1->prev->macAddr == destinationAddr)
        {

            (*station)->queue->insert(
                msg,
                NULL,
                &queueIsFull,
                node->getNodeTime());

        }
        else
        {
        tmpstation2=(DOT11e_HCCA_CapDownlinkPollList*)
            MEM_malloc( sizeof( DOT11e_HCCA_CapDownlinkPollList* ) );

        tmpstation2->macAddr=destinationAddr;
        tmpstation2->queue->insert(
            msg,
            NULL,
            &queueIsFull,
            node->getNodeTime());
        tmpstation2->next=NULL;
        tmpstation2->prev=tmpstation1;
        tmpstation1->next=tmpstation2;
        }

    }

}//MacDot11eHCDownLinkListInsert

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCDownLinkListIsEmpty
//  PURPOSE:     Remove an station and all its packets from the poll list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapDownlinkPollList* station
//                  Pointer to polling list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

BOOL MacDot11eHCDownLinkListIsEmpty(
    Node* node,
     MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList* station)
{
    if (station == NULL)
        return TRUE;
    else
        return FALSE;
}//End of MacDot11eHCDownLinkListIsEmpty



//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCDownLinkListGetFirstNode
//  PURPOSE:     Remove an station and all its packets from the poll list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      DOT11e_HCCA_CapDownlinkPollList*
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

DOT11e_HCCA_CapDownlinkPollList* MacDot11eHCDownLinkListGetFirstNode(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11e_HCCA_CapDownlinkPollList *tmpLink=NULL;
    DOT11e_HCCA_CapDownlinkPollList *tmpLink1 =
        dot11->hcVars->cfDownlinkList;

    if (!MacDot11eHCDownLinkListIsEmpty(
        node,dot11,dot11->hcVars->cfDownlinkList))

    {
        while (tmpLink1->prev!=NULL)
            {
                tmpLink1->prev=tmpLink1->prev->prev;
            }
        tmpLink= tmpLink1->prev->next;
    }

     return tmpLink;
}//MacDot11eHCDownLinkListGetFirstNode


//--------------------------------------------------------------------------
// NAME:       MacDot11eHCInit
// PURPOSE:    Initialize HC, read user input.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             phyModel  PhyModel
//                  NPHY model used.
//             const NodeInput *nodeInput
//                  ponter to NodeInput
// RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eHCInit(
    Node* node,
    MacDataDot11* dot11,
    PhyModel phyModel,
    const NodeInput *nodeInput,
    NetworkType networkType,
    in6_addr *ipv6SubnetAddr,
    unsigned int prefixLength)
{
    char input[MAX_STRING_LENGTH];
    int capLimit;
    BOOL wasFound = FALSE;
    Address address;

    // Initialize HC related variables
    dot11->hcVars = (DOT11e_HcVars*) MEM_malloc(sizeof(DOT11e_HcVars));
    ERROR_Assert(dot11->hcVars != NULL,
    "MacDot11eHCInit: Unable to allocate HC structure.\n");

    //Set the PIFS duration
    dot11->pifs = dot11->sifs + dot11->slotTime;
    dot11->txPifs = dot11->txSifs + dot11->slotTime;

    dot11->cfpTimeout = 2 * dot11->extraPropDelay + dot11->pifs;

    memset(dot11->hcVars, 0, sizeof(DOT11e_HcVars));


    dot11->hcVars->deliveryMode =
            (DOT11e_HcDeliveryMode) DOT11e_HC_DELIVERY_MODE_DEFAULT;

    dot11->hcVars->broadcastsHavePriority =
        DOT11_BROADCASTS_HAVE_PRIORITY_DURING_CFP;
    dot11->hcVars->cfPollList = (DOT11e_HCCA_CapPollList *)
        MEM_malloc(sizeof(DOT11e_HCCA_CapPollList));
    memset(dot11->hcVars->cfPollList,0,sizeof(DOT11e_HCCA_CapPollList));
    dot11->hcVars->cfPollList->TS[0].tsid = DOT11e_HCCA_MIN_TSID;
    dot11->hcVars->cfPollList->TS[1].tsid = DOT11e_HCCA_MIN_TSID + 1;
    dot11->hcVars->cfPollList->TS[2].tsid = DOT11e_HCCA_MIN_TSID + 2;
    dot11->hcVars->cfPollList->TS[3].tsid = DOT11e_HCCA_MIN_TSID + 3;

    dot11->hcVars->cfDownlinkList = NULL;
    dot11->hcVars->allStationsPolledOnce = FALSE;
    dot11->hcVars->allCurrTSIDStationsPolled = FALSE;
    dot11->hcVars->currPolledItem = NULL;
    dot11->hcVars->currentTSID = DOT11e_HCCA_MAX_TSID;
    dot11->hcVars->hasPollableStations = FALSE;

    dot11->hcVars->dataRateTypeForAck = 0;

    // Only AP can be a Hybrid Coordinator.
    // User input e.g.
    //       [0.6] MAC-DOT11-HC  YES | NO
    // If not specified, the default is NO

    NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11e-HC",
        &wasFound,
        input);
    if (wasFound)
    {
        if (!strcmp(input, "YES"))
        {
            if (!MacDot11IsAp(dot11)){
                ERROR_ReportError("MacDot11Init: "
                    "HC can be collacated ony at the AP\n"
                    "non-AP QSTA can't become an HC\n");
            }
            else
            {
                dot11->stationType = DOT11_STA_HC;
            }
        }
        else if (!strcmp(input, "NO"))
        {
            // Do nothing.
        }
        else
        {
            ERROR_ReportError("MacDot11Init: "
                    "Invalid value for MAC-DOT11e-HC in configuration file.\n"
                    "Expecting YES or NO.\n");

        }

    }

    if (dot11->stationType == DOT11_STA_HC)
    {
        IO_ReadInt(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11e-CAP-LIMIT",
            &wasFound,
            &capLimit);

        if (wasFound)
        {
            dot11->hcVars->capMaxDuration = MacDot11TUsToClocktype(
                (unsigned short) capLimit);
            ERROR_Assert((dot11->hcVars->capMaxDuration > 0 &&
                dot11->hcVars->capMaxDuration < dot11->beaconInterval),
                "MacDot11eHCInit: "
                "Value of MAC-DOT11e-CAP-LIMIT "
                "should be greater than zero and"
                "less than the beacon interval\n");
        }
        else
        {
            dot11->hcVars->capMaxDuration = DOT11e_HCCA_CFP_MAX_DURATION_DEFAULT;
        }
    }

    for (int tsCounter = 0;
        tsCounter < DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED; tsCounter++)
    {
        dot11->hcVars->staTSBufferItem[tsCounter] =
            (DOT11e_HCCA_TSDataBuffer* )
            MEM_malloc(sizeof(DOT11e_HCCA_TSDataBuffer));

        ERROR_Assert(dot11->hcVars->staTSBufferItem[tsCounter] != NULL,
            "MacDot11Init: Unable to allocate TS  structure.\n");

        memset(dot11->hcVars->staTSBufferItem[tsCounter], 0,
            sizeof(DOT11e_HCCA_TSDataBuffer));

        dot11->hcVars->staTSBufferItem[tsCounter]->first= NULL;
        dot11->hcVars->staTSBufferItem[tsCounter]->last = NULL;
        dot11->hcVars->staTSBufferItem[tsCounter]->size = 0;
        dot11->hcVars->staTSBufferItem[tsCounter]->tsStatus = DOT11e_TS_None;
    }

}//MacDot11eHCInit

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpCancelSomeTimersBecauseChannelHasBecomeBusy
//  PURPOSE:     To cancel timers when channel changes from idle to
//               receiving state.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eCfpCancelSomeTimersBecauseChannelHasBecomeBusy(
    Node* node,
    MacDataDot11* dot11)
{
    switch (dot11->cfstate) {
        case DOT11e_S_CFP_NONE:
            break;

        case DOT11e_S_CFP_WF_DATA:
        case DOT11e_S_CFP_WF_DATA_ACK:
            // Assume that the HC is getting the expected frame.
            MacDot11StationCancelTimer(node, dot11);
            MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
            break;

        case DOT11e_S_CFP_WF_ACK:
            // Assume that the expected frame is coming in.
            if (MacDot11IsHC(dot11))
            {
                MacDot11StationCancelTimer(node, dot11);
                MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);
            }
            else
            {
                MacDot11StationCancelTimer(node, dot11);
            }
            break;

        default:
            break;
    } //switch

}//MacDot11eCfpCancelSomeTimersBecauseChannelHasBecomeBusy

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpPhyStatusIsNowIdleStartSendTimers
//  PURPOSE:     To restart send once the channel transits from busy
//               to idle. Applied only to a PC during PCF.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11eCfpPhyStatusIsNowIdleStartSendTimers(
    Node* node,
    MacDataDot11* dot11)
{

    if (MacDot11IsHC(dot11)) {
        switch (dot11->cfstate) {
            case DOT11e_S_CFP_NONE:
                // Start a SIFS timer
                MacDot11StationStartTimer(node, dot11, dot11->txSifs);
                break;

            case DOT11e_S_CFP_WF_DATA:
            case DOT11e_S_CFP_WF_ACK:
            case DOT11e_S_CFP_WF_DATA_ACK:
                break;

            default:
                ERROR_ReportError(
                    "MacDot11eCfpPhyStatusIsNowIdleStartSendTimers: "
                    "Unexpected busy state.\n");
                break;
        } //switch
    }
    else
    {
        switch (dot11->cfstate) {
            case DOT11e_S_CFP_WF_ACK:
            case DOT11e_S_CFP_WF_DATA_ACK:
                //ACK lost
                MacDot11StationResetAckVariables(dot11);
                MacDot11eCfpHandleCapEnd(node,dot11);
                break;
            default:
                break;
        }
    }
}//MacDot11eCfpPhyStatusIsNowIdleStartSendTimers
//--------------------------------------------------------------------------
// NAME:       MacDot11eCFPReceivePacketFromPhy
// PURPOSE:    Receive Packet from Phy in HCCA mode during CAP/CFP.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             Message msg
//                  Pointer to message.
// RETURN:      None.
// ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eCFPReceivePacketFromPhy (
        Node* node,
        MacDataDot11* dot11,
        Message* msg){

    DOT11_ShortControlFrame* sHdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    Mac802Address destAddr = sHdr->destAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) sHdr->frameType;

    if (destAddr == dot11->selfAddr) {
        if (!MacDot11IsHC(dot11) && dot11->cfstate ==DOT11e_S_CFP_WF_ACK &&
            frameType != DOT11_ACK)
        {
            //ACK not received
            MacDot11StationResetAckVariables(dot11);
        }
        switch (frameType) {
            case DOT11_QOS_DATA_POLL:
            case DOT11_QOS_CF_POLL:
            if (MacDot11IsHC(dot11))
            {
                ERROR_ReportError("MacDot11eCFPReceivePacketFromPhy: "
                        "Invalid frame received at HC: Poll frame\n");
                MESSAGE_Free(node, msg);
            }
            else
            {
                MacDot11Trace(node, dot11, msg, "CAP POLL Receive");
                if (MacDot11IsFrameFromMyAP(dot11,((DOT11e_FrameHdr *)
                        MESSAGE_ReturnPacket(msg))->sourceAddr))
                {
                    MacDot11eCfpStationReceiveUnicast(node, dot11, msg);
                }
            }
            break;

            case DOT11_ACK:
                MacDot11Trace(node, dot11, msg, "Receive");

                ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                         dot11->hcVars->nextHopAddress,
                         "Address does not match");

                MacDot11eCfpStationProcessAck(node, dot11,
                        dot11->hcVars->nextHopAddress);
                MESSAGE_Free(node, msg);

                dot11->totalNoOfQoSCFAckReceived++;
                if (MacDot11IsHC(dot11))
                {
                    MacDot11CfpHcTransmit(node,dot11,frameType,0);
                }
                else
                {
                    MacDot11eCfpHandleCapEnd(node,dot11);
                }
            break;

            case DOT11_QOS_CF_DATA_ACK:
            case DOT11_QOS_DATA:
                if (MacDot11IsHC(dot11))
                {
                    MacDot11eHcReceiveUnicast(node, dot11, msg);
                }
                else
                {
                    if (MacDot11IsFrameFromMyAP(dot11,((DOT11e_FrameHdr *)
                           MESSAGE_ReturnPacket(msg))->sourceAddr))
                    {
                        MacDot11eCfpStationReceiveUnicast(node, dot11, msg);
                    }
                }
                break;

            case DOT11_QOS_DATA_POLL_ACK:
            case DOT11_QOS_POLL_ACK:
                if (MacDot11IsHC(dot11))
                {
                    ERROR_ReportError("MacDot11eCFPReceivePacketFromPhy: "
                        "Invalid received frame at HC: Data Poll"
                        "Ack or Poll Ack\n");
                    MESSAGE_Free(node, msg);
                }
                else
                {
                    if (MacDot11IsFrameFromMyAP(dot11,((DOT11e_FrameHdr *)
                           MESSAGE_ReturnPacket(msg))->sourceAddr))
                    {
                        MacDot11eCfpStationReceiveUnicast(node, dot11, msg);
                    }
                }
                break;

            case DOT11_QOS_CF_NULLDATA:
                if (MacDot11IsHC(dot11))
                {
                    MacDot11eHcReceiveUnicast(node, dot11, msg);
                }
                else
                {
                    ERROR_ReportError("MacDot11eCFPReceivePacketFromPhy: "
                            "Invalid frame received at QSTA: Null Data frame"
                            "\n");
                    MESSAGE_Free(node, msg);
                }

                break;

            default:
                //Ignore. Frame may be from overlapping BSS
                MESSAGE_Free(node, msg);
                break;
        } //switch
    }
    else if (destAddr == ANY_MAC802) {
        //Receive broadcast frame in CAP
        //Ignore. Frame may be from overlapping BSS
        if (!MacDot11IsHC(dot11) && dot11->cfstate ==DOT11e_S_CFP_WF_ACK)
        {
            //ACK not received
            MacDot11StationResetAckVariables(dot11);
            MacDot11eCfpHandleCapEnd(node,dot11);
        }
        MESSAGE_Free(node,msg);
    }
    else{
        if (!MacDot11IsHC(dot11) && dot11->cfstate ==DOT11e_S_CFP_WF_ACK)
        {
            //ACK not received
            MacDot11StationResetAckVariables(dot11);
            MacDot11eCfpHandleCapEnd(node,dot11);
        }
        //Add code to support Q-Ack
        MESSAGE_Free(node,msg);
    }
 }//MacDot11eCFPReceivePacketFromPhy

//--------------------------------------------------------------------------
// NAME:       MacDot11eRetrieveFirstPacketofTSIDFromTSsBuffer
// PURPOSE:    To get the first packet from the TSs Buffer.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             int tsId
//                  Traffic stream ID
// RETURN:      first message.
// ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eRetrieveFirstPacketofTSIDFromTSsBuffer(
       Node* node,
       MacDataDot11* dot11,
       int tsId)
{
    int index = tsId - DOT11e_HCCA_MIN_TSID;
    if ((dot11->hcVars->staTSBufferItem[index]->size > 0)
        && (dot11->hcVars->staTSBufferItem[index]->first!=NULL))
    {
        if (MacDot11IsHC(dot11))
        {
            dot11->hcVars->nextHopAddress =
                dot11->hcVars->staTSBufferItem[index]->first->frameInfo->RA;
        }
        else
        {
            dot11->hcVars->nextHopAddress = dot11->bssAddr;
        }

        dot11->TxopMessage =
            dot11->hcVars->staTSBufferItem[index]->first->frameInfo->msg;
        DOT11e_HCCA_TSDataBufferItem* tItem =
            dot11->hcVars->staTSBufferItem[index]->first;

        dot11->hcVars->staTSBufferItem[index]->size--;
        dot11->hcVars->staTSBufferItem[index]->first =
            dot11->hcVars->staTSBufferItem[index]->first->next;
        if (dot11->hcVars->staTSBufferItem[index]->size <= 1)
        {
            dot11->hcVars->staTSBufferItem[index]->last =
                dot11->hcVars->staTSBufferItem[index]->first;
        }
        MEM_free(tItem->frameInfo);
        MEM_free(tItem);
    }
    else
    {
        dot11->TxopMessage =  NULL;
    }
}//MacDot11eRetrieveFirstPacketofTSIDFromTSsBuffer

//--------------------------------------------------------------------------
// NAME:       MacDot11ePeekFirstPacketofTSIDFromTSsBuffer
// PURPOSE:    To peek the first packet from the TSs Buffer.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             int tsId
//                  Traffic stream ID
// RETURN:      first message.
// ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11ePeekFirstPacketofTSIDFromTSsBuffer(
       Node* node,
       MacDataDot11* dot11,
       int tsId)
{
    if (dot11->TxopMessage!=NULL)
    {
        ERROR_Assert(FALSE,"TxopMessage not NULL");

    }
    int index = tsId - DOT11e_HCCA_MIN_TSID;
    if ((dot11->hcVars->staTSBufferItem[index]->size > 0)
        && (dot11->hcVars->staTSBufferItem[index]->first!=NULL))
    {
        if (MacDot11IsHC(dot11))
        {
            dot11->hcVars->nextHopAddress =
              dot11->hcVars->staTSBufferItem[index]->first->frameInfo->RA;
        }
        else
        {
            dot11->hcVars->nextHopAddress = dot11->bssAddr;
        }

        dot11->TxopMessage =
            dot11->hcVars->staTSBufferItem[index]->first->frameInfo->msg;
    }
}//MacDot11ePeekFirstPacketofTSIDFromTSsBuffer


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationHasSufficientTime
//  PURPOSE:     Determine if station has sufficient time to send frame type
//               The frame type determines the frame exchange sequence
//               that would occur. If time was insufficient, the station
//               would not send data bu would send an ack/nulldata.
//               send a CF-End to terminate the CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame types
//               clocktype sendDelay
//                  Delay before sending the frame
//               int* dataRateType
//                  Data rate to use for transmit.
//  RETURN:      TRUE if there is sufficient time in CFP to send the frame
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11eCfpStationHasSufficientTime(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype sendDelay,
    int dataRateType)
{
    clocktype txopBalanceDuration=0;
    clocktype txopRequiredDuration = 0;
    clocktype currentMessageDuration;
    clocktype currentTime = node->getNodeTime();
    clocktype TxopEndTime=0;

    // Potential time for an Ack frame
    TxopEndTime =
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->lowestDataRateType,
            DOT11_SHORT_CTRL_FRAME_SIZE)
        + dot11->extraPropDelay
        + dot11->slotTime;

    txopBalanceDuration = dot11->hcVars->txopDurationGranted-(currentTime
        - dot11->hcVars->txopStartTime);

    switch (frameType) {
        case DOT11_QOS_DATA:
        case DOT11_CF_DATA_ACK:
            currentMessageDuration =
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    MESSAGE_ReturnPacketSize(dot11->TxopMessage) +
                    sizeof(DOT11e_FrameHdr));

            // Station data needs sendDelay + msg time + SIFS + CfEnd
            txopRequiredDuration =
                sendDelay
                + currentMessageDuration
                + dot11->extraPropDelay
                + dot11->sifs
                + TxopEndTime;
            break;

        case DOT11_CF_NULLDATA:
        case DOT11_CF_ACK:
            // Need sendDelay + ack + SIF + CfEnd
            txopRequiredDuration =
                sendDelay
                + PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                     sizeof(DOT11e_FrameHdr))
                + dot11->extraPropDelay
                + dot11->sifs
                + TxopEndTime;
            break;

        default:
            ERROR_ReportError("MacDot11eCfpStationHasSufficientTime: "
                "Unknown frame type.\n");
            break;

    } //switch

    if (DEBUG)
    {
        if (txopRequiredDuration > txopBalanceDuration)
        {
             printf("MacDot11eCfpStationHasSufficientTime: "
                 "insufficient time to send the frame\n");
        }
    }


    return txopRequiredDuration <= txopBalanceDuration;

}//MacDot11eCfpStationHasSufficientTime

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpAdjustFrameTypeForAck
//  PURPOSE:     Adjust the PCF frame type in case an ack is to
//               piggy-back on it.
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType * const frameType
//                  Frame type
//  RETURN:      Adjusted frame type
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpAdjustFrameTypeForAck(
    const MacDataDot11* const dot11,
    DOT11_MacFrameType* const frameType)
{
    if (*frameType == DOT11_CF_NONE) {
        return;
    }

    if (dot11->cfpAckIsDue) {
        // Could use bit patterns of frame types (*frameType++)
        //
        switch (*frameType) {

            case DOT11_QOS_DATA:
                *frameType = DOT11_CF_ACK;
                break;

            case DOT11_QOS_DATA_POLL:
                *frameType = DOT11_QOS_DATA_POLL_ACK;
                break;

            case DOT11_QOS_CF_POLL:
                *frameType = DOT11_QOS_POLL_ACK;
                break;

            default:
                break;

        } //switch
    } //if
}//MacDot11eCfpAdjustFrameTypeForAck
//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationSetTransmitState
//  PURPOSE:     Set the state of the BSS station based on the frame
//               about to be transmitted during CAP.
//
//  PARAMETERS:  MacDataDot11* const dot11
//                   Pointer to dot11 structure
//               DOT11_MacFrameType frameType
//                   Frame type
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationSetTransmitState(
     MacDataDot11* const dot11,
    DOT11_MacFrameType frameType)
{
    dot11->cfpLastFrameType = frameType;

    switch (frameType) {

        case DOT11_QOS_DATA:
            if (DEBUG_HCCA)
            {
                printf("cfp has been changed to DOT11e_X_CFP_UNICAST\n");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_CFP_UNICAST);
            break;
        case DOT11_QOS_CF_DATA_ACK:
            if (DEBUG_HCCA)
            {
                printf("cfp state has been changed to "
                    "DOT11e_X_QoS_DATA_ACK");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_QoS_DATA_ACK);
            break;

        case DOT11_QOS_CF_NULLDATA:
            if (DEBUG_HCCA)
            {
                printf("state has been changed to DOT11e_X_CFP_NULLDATA\n");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_CFP_NULLDATA);
            break;

        case DOT11_ACK:
            if (DEBUG_HCCA)
            {
                printf("cfp state has been changed to DOT11e_X_ACK\n");
            }
            MacDot11eCfpSetState(dot11, DOT11e_X_ACK);
            break;

        default:
            ERROR_ReportError("MacDot11eCfpStationSetTransmitState: "
                "Unknown frame type.\n");
            break;

    } //switch
}//MacDot11eCfpStationSetTransmitState

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationTransmitAck
//  PURPOSE:     Transmit an ack to HC in case of normal Ack mode.
//               Note that ACK is not CF-Ack.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype delay
//                  Delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmitAck(
    Node* node,
    MacDataDot11* dot11,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11_ShortControlFrame sHdr;
    char macAdder[24];

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_SHORT_CTRL_FRAME_SIZE, TRACE_DOT11);

    sHdr.frameType = DOT11_ACK;
    sHdr.destAddr = dot11->bssAddr;
    sHdr.duration = 0;
    if (DEBUG_HCCA)
    {
        MacDot11MacAddressToStr(macAdder,&dot11->bssAddr);
        printf("%d send ACK to %s\n",node->nodeId,macAdder);
    }

    if (DEBUG_HCCA)
    {
        MacDot11MacAddressToStr(macAdder,&sHdr.destAddr);
        printf("\n%d sending ACK to %s \n",
                node->nodeId,
                macAdder);
    }

    memcpy(MESSAGE_ReturnPacket(pktToPhy), &(sHdr),
           DOT11_SHORT_CTRL_FRAME_SIZE);

    MacDot11eCfpStationSetTransmitState(dot11,DOT11_ACK);
    MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);

}//MacDot11eCfpStationTransmitAck
//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationTransmitNullData
//  PURPOSE:     Transmit of QoS null data by a BSS station.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type to send
//               clocktype delay
//                  Delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmitNullData(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11e_FrameHdr* fHdr;
    Mac802Address destAddr = dot11->bssAddr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_DATA_FRAME_HDR_SIZE, TRACE_DOT11);
    fHdr = (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);

    MacDot11eStationSetFieldsInDataFrameHdr(
        dot11,
        fHdr,
        destAddr,
        frameType,
        dot11->currentACIndex);
    fHdr->address3 = dot11->bssAddr;
    fHdr->duration = 0;
    DOT11_SeqNoEntry* entry;
        entry = MacDot11StationGetSeqNo(node,
                                        dot11,
                                        fHdr->destAddr,
                                        fHdr->qoSControl.TID);
    ERROR_Assert(entry,
        "MacDot11eCfpStationTransmitUnicast: "
        "Unable to find or create sequence number entry.\n");
    fHdr->seqNo = entry->toSeqNo;
    if (DEBUG_HCCA)
    {
        char macAdder[24];
        MacDot11MacAddressToStr(macAdder,&fHdr->destAddr);
        printf("\n%d sending Null Data to %s with value of Duration/ID"
            " field is%d \n",
                node->nodeId,
                macAdder,
                fHdr->duration);
    }
    dot11->hcVars->nextHopAddress = dot11->bssAddr;
    dot11->hcVars->txopStartTime= 0;

    dot11->dataRateInfo =
            MacDot11StationGetDataRateEntry(node, dot11,
                dot11->hcVars->nextHopAddress);

    ERROR_Assert(dot11->dataRateInfo->ipAddress ==
        dot11->hcVars->nextHopAddress,
        "Address does not match");

    MacDot11StationAdjustDataRateForNewOutgoingPacket(node, dot11);

    dot11->waitingForAckOrCtsFromAddress = dot11->hcVars->nextHopAddress;

    MacDot11eCfpStationSetTransmitState(dot11, frameType);

    MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);

}//MacDot11eCfpStationTransmitNullData


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationTransmitUnicast
//  PURPOSE:     Transmit a unicast frame in response to poll.
//               An ack may piggyback on the data frame.
//  PARAMETERS:  Frame type to send
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//               clocktype delay
//                  Delay to be sent
//               int tsid
//                  Traffic Stream ID
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmitUnicast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay,
    int tsid)
{
    Message* pktToPhy;
    DOT11e_FrameHdr* fHdr;
    DOT11_SeqNoEntry* entry;

    ERROR_Assert(dot11->TxopMessage != NULL,
        "MacDot11eCfpStationTransmitUnicast: "
        "Transmit of data when message is NULL.\n");

    pktToPhy = MESSAGE_Duplicate(node, dot11->TxopMessage);
    MESSAGE_AddHeader(node,
                      pktToPhy,
                      sizeof(DOT11e_FrameHdr),
                      TRACE_DOT11);

    fHdr = (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);

    MacDot11eStationSetFieldsInDataFrameHdr(dot11, fHdr,
        dot11->hcVars->nextHopAddress, frameType,dot11->currentACIndex);

    //Enough for receiving Ack
    fHdr->duration = (UInt16) MacDot11NanoToMicrosecond(
        dot11->extraPropDelay + dot11->sifs +
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->dataRateInfo->dataRateType,
            DOT11_SHORT_CTRL_FRAME_SIZE) +
        dot11->extraPropDelay);

        dot11->TSID = fHdr->qoSControl.TID;
        dot11->currentTsid = dot11->TSID;
        entry = MacDot11StationGetSeqNo(node,
                                        dot11,
                                        fHdr->destAddr,
                                        fHdr->qoSControl.TID);

    ERROR_Assert(entry,
        "MacDot11eCfpStationTransmitUnicast: "
        "Unable to find or create sequence number entry.\n");
    fHdr->seqNo = entry->toSeqNo;

    dot11->waitingForAckOrCtsFromAddress = fHdr->destAddr;
    if (DEBUG_HCCA)
    {
        char macAdder[24];
        MacDot11MacAddressToStr(macAdder,&fHdr->destAddr);
        printf("\n%d sending to %s with value of Duration/ID "
               "field is%d \n",
                node->nodeId,
                macAdder,
                fHdr->duration);
    }
    if (MESSAGE_ReturnPacketSize(dot11->TxopMessage) >
        DOT11_FRAG_THRESH)
    {
        // Fragmentation not currently supported
        ERROR_ReportError("MacDot11eCfpStationTransmitUnicast: "
            "Fragmentation not supported.\n");
    }
    else {
        fHdr->fragId = 0;
        MacDot11eCfpStationSetTransmitState(dot11, frameType);
        MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
    }
}//MacDot11eCfpStationTransmitUnicast


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationTransmit
//  PURPOSE:     Determine the frame to transmit based on timer delay,
//               available balance CFP duration and received frame type.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Received frame type
//               clocktype timerDelay
//                  Delay to be sent
//               int tsid
//                  Traffic Stream ID
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType receivedFrameType,
    clocktype timerDelay,
    int tsid)
{
    clocktype sendDelay;
    DOT11_MacFrameType frameType = DOT11_CF_NONE;
    int dataRateType;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) {
        MacDot11Trace(node, dot11, NULL,
            "CFP Station transmit, phy not idle.");
        return;
    }

    // The timerDelay should be 0 for a station
    if (timerDelay < dot11->sifs) {
        sendDelay = dot11->sifs - timerDelay;
    }
    else {
        sendDelay = EPSILON_DELAY;
    }

    dataRateType = PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

    switch (receivedFrameType) {
        case DOT11_QOS_CF_POLL:
        case DOT11_QOS_POLL_ACK:
            // Send a data frame if possible
            // else respond with a null data response.
            if (dot11->TxopMessage == NULL &&
                !MacDot11IsTSHasMessageforTSID(node,dot11,tsid)) {
                frameType = DOT11_QOS_CF_NULLDATA;
                dot11->currentTsid = tsid;
            }
            else {
                ERROR_Assert(dot11->TxopMessage == NULL,
                        "TxopMessage not NULL");
                frameType = DOT11_QOS_DATA;
                MacDot11ePeekFirstPacketofTSIDFromTSsBuffer(
                    node,dot11,tsid);

                dot11->dataRateInfo =
                    MacDot11StationGetDataRateEntry(node, dot11,
                        dot11->hcVars->nextHopAddress);

                    ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                        dot11->hcVars->nextHopAddress,
                        "Address does not match");

                MacDot11StationAdjustDataRateForNewOutgoingPacket(
                    node, dot11);

                if (! MacDot11eCfpStationHasSufficientTime(
                    node, dot11,frameType, sendDelay, dataRateType))
                {
                    dot11->TxopMessage = NULL;
                    frameType = DOT11_QOS_CF_NULLDATA;
                    dot11->currentTsid = tsid;
                }
                else
                {
                    dot11->currentTsid = tsid;
                }
            }
            break;

        case DOT11_QOS_DATA:
            // Send a CF-ACK. An option is to send DOT11_ACK.
            frameType = DOT11_ACK;
            break;

        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
            //Not implemented in this version.
        default:
            ERROR_ReportError("MacDot11eCfpStationTransmit: "
                "Unexpected received frame type.\n");
            break;
    } //switch


    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        dataRateType);

    switch (frameType) {
        case DOT11_QOS_DATA:
        case DOT11_QOS_CF_DATA_ACK:
            MacDot11eCfpStationTransmitUnicast(node, dot11,
               frameType, sendDelay, tsid);
            break;

        case DOT11_QOS_CF_NULLDATA:
            MacDot11eCfpStationTransmitNullData(node, dot11,
                frameType, sendDelay);
            break;

        case DOT11_ACK:
            MacDot11eCfpStationTransmitAck(node,dot11,sendDelay);
            break;
        default:
            ERROR_ReportError("MacDot11CfpStationTransmit: "
                "Unknown frame type.\n");
            break;
    } //switch

}//MacDot11eCfpStationTransmit
//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationProcessUnicast
//  PURPOSE:     Receive a directed unicast frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      TRUE is unicast is acceptable
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11eCfpStationProcessUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    BOOL unicastIsProcessed = FALSE;
    DOT11e_FrameHdr* fHdr =
       (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    if (DEBUG_HCCA)
    {
         char macAdder[24];
        MacDot11MacAddressToStr(macAdder,&fHdr->destAddr);
        printf("\n%d received from %s with value of Duration/ID"
            " field is%d \n",
                node->nodeId,
                macAdder,
                fHdr->duration);
    }
    if (MacDot11StationCorrectSeqenceNumber(node,
                                            dot11,
                                            sourceAddr,
                                            fHdr->seqNo,
                                            fHdr->qoSControl.TID))
    {
        MacDot11Trace(node, dot11, msg, "Receive");
        dot11->totalNoOfQoSDataFrameReceived++;
        dot11->totalNoOfQoSCFDataFramesReceived++;

        dot11->hcVars->dataRateTypeForAck =
            PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

        MacDot11StationHandOffSuccessfullyReceivedUnicast(node, dot11, msg);

        unicastIsProcessed = TRUE;
    }
    else {
        MacDot11Trace(node, dot11, NULL, "Drop, wrong sequence number");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        unicastIsProcessed = FALSE;
        MESSAGE_Free(node, msg);
    }

    MacDot11eCfpSetState(dot11, DOT11e_S_CFP_NONE);

    dot11->cfpAckIsDue = TRUE;

    return unicastIsProcessed;
}//MacDot11eCfpStationProcessUnicast


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationReceiveUnicast
//  PURPOSE:     Depending on the type of directed frame, process it
//               and arrange to respond appropriately
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eCfpStationReceiveUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11e_FrameHdr* fHdr =
        (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) fHdr->frameType;

    switch (frameType) {

        case DOT11_QOS_DATA_POLL_ACK:
            MacDot11eCfpStationProcessAck(node, dot11, sourceAddr);
            dot11->totalNoOfQoSCFAckReceived++;
            // no break; continue to process DOT11_QOS_DATA_POLL

        case DOT11_QOS_DATA_POLL:
            MacDot11Trace(node, dot11, msg, "Receive");
            dot11->totalNoOfQoSDataCFPollsReceived++;

            //Reset NAV for poll
            dot11->NAV = 0;

            dot11->hcVars->currentTSID = fHdr->qoSControl.TID;
            dot11->hcVars->txopStartTime = node->getNodeTime();
            dot11->hcVars->txopDurationGranted =
                MacDot11TUsToClocktype(fHdr->qoSControl.TXOP);

            MacDot11eCfpStationProcessUnicast(node, dot11, msg);

            MacDot11eCfpStationTransmit(
                node,
                dot11,
                frameType,
                0,
                fHdr->qoSControl.TID);
            break;

        case DOT11_QOS_POLL_ACK:
            MacDot11eCfpStationProcessAck(node, dot11, sourceAddr);
            dot11->totalNoOfQoSCFAckReceived++;
            // no break; continue to process DOT11_CF_POLL

        case DOT11_QOS_CF_POLL:
            MacDot11Trace(node, dot11, msg, "Receive");

            MacDot11eCfpCheckForOutgoingPacket(node,dot11);

            //Reset NAV for poll
            dot11->NAV = 0;
            dot11->TxopMessage= NULL;

            dot11->hcVars->currentTSID = fHdr->qoSControl.TID;
            dot11->totalNoOfQoSCFPollsReceived++;
            dot11->hcVars->txopStartTime = node->getNodeTime();
            dot11->hcVars->txopDurationGranted =
                MacDot11TUsToClocktype(fHdr->qoSControl.TXOP);
            MacDot11eCfpStationTransmit(
                node, dot11, frameType, 0,fHdr->qoSControl.TID);
            MESSAGE_Free(node, msg);
            break;

        case DOT11_QOS_CF_DATA_ACK:
            MacDot11eCfpStationProcessAck(node, dot11, sourceAddr);
            dot11->totalNoOfQoSCFAckReceived++;
            // no break; continue to process DATA

        case DOT11_QOS_DATA:
            MacDot11eCfpStationProcessUnicast(node, dot11, msg);

            MacDot11eCfpStationTransmit(
                node, dot11, frameType, 0,fHdr->qoSControl.TID);
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationProcessUnicast: "
                "Received unknown unicast frame type.\n");
            break;
    } //switch
}//MacDot11eCfpStationReceiveUnicast

//----------------------------------------------------------------
// FUNCTION   :: MacDot11IsTSsHasMessage
// LAYER      :: MAC
// PURPOSE    ::
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + dot11     : MacDataDot11* : Pointer to the MacData structure
// RETURN     :: BOOL : TRUE or FALSE
//----------------------------------------------------------------
BOOL MacDot11IsTSsHasMessage(Node* node, MacDataDot11* dot11)
{

    for (int tsCounter = DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED-1;
         tsCounter >=0; tsCounter--)
    {
        if (dot11->hcVars->staTSBufferItem[tsCounter]->size > 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// ----------------------------------------------------------------
// FUNCTION   :: MacDot11IsTSHasMessageforTSID
// LAYER      :: MAC
// PURPOSE    ::
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + dot11     : MacDataDot11* : Pointer to the MacData structure
// + tsid      : int : TS ID
// RETURN     :: BOOL : TRUE or FALSE
// ----------------------------------------------------------------
BOOL MacDot11IsTSHasMessageforTSID(Node* node, MacDataDot11* dot11, int tsid)
{
    int index = tsid - DOT11e_HCCA_MIN_TSID;
    return dot11->hcVars->staTSBufferItem[index]->size > 0;

}//MacDot11IsTSHasMessageforTSID

//--------------------------------------------------------------------------
// NAME:       MacDot11eIsTSBuffreFull
// PURPOSE:    To check the TS buffer is Full.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             int priority
//                  priority
// RETURN:      none.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------

BOOL MacDot11eIsTSBuffreFull(Node* node, MacDataDot11* dot11, int priority)
{
    unsigned int index = (unsigned)(priority - DOT11e_HCCA_LOWEST_PRIORITY);
    if (dot11->hcVars->staTSBufferItem[index]->size >=
        DOT11e_HCCA_TSBUFFER_SIZE)
        return TRUE;
    else
        return FALSE;

}// MacDot11eIsTSBuffreFull

//--------------------------------------------------------------------------
// NAME:       MacDot11eStationMoveAPacketFromTheNetworkToTheTSsBuffer
// PURPOSE:    To insert packet in TS buffer.
// PARAMETERS  Node* node
//                Node being initialized.
//             MacDataDot11* dot11
//                  Pointer to Dot11 structure
//             int priority
//                  priority
//             Message* msg
//                  message
//             Mac802Address nextHop
//                  next hop Address
// RETURN:      none.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eStationMoveAPacketFromTheNetworkToTheTSsBuffer(
    Node* node,
    MacDataDot11* dot11,
    unsigned int priority,
    Message* msg,
    Mac802Address nextHop)
{

        DOT11e_HCCA_TSDataBufferItem* dataItem = NULL;
        unsigned int index = priority - DOT11e_HCCA_LOWEST_PRIORITY;
        DOT11e_HcVars* hcVars = dot11->hcVars;
        unsigned int tsid = priority + DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED;

        dataItem = (DOT11e_HCCA_TSDataBufferItem*)
                    MEM_malloc(sizeof(DOT11e_HCCA_TSDataBufferItem));

        dataItem->frameInfo = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));

        dataItem->frameInfo->msg = msg;
        dataItem->frameInfo->RA = nextHop;
        dataItem->frameInfo->TA = dot11->selfAddr;
        dataItem->frameInfo->SA = dot11->selfAddr;
        dataItem->frameInfo->DA = nextHop;
        dataItem->frameInfo->insertTime = node->getNodeTime();
        dataItem->frameInfo->frameType = DOT11_QOS_DATA;

        if (hcVars->staTSBufferItem[index]->size == 0)
        {
            hcVars->staTSBufferItem[index]->first =
                hcVars->staTSBufferItem[index]->last =
                dataItem;
            dataItem->next = dataItem->prev = NULL;
            hcVars->staTSBufferItem[index]->size++;
        }
        else
        {
            dataItem->prev = hcVars->staTSBufferItem[index]->last;
            dataItem->next = NULL;
            hcVars->staTSBufferItem[index]->last->next = dataItem;
            hcVars->staTSBufferItem[index]->last = dataItem;
            hcVars->staTSBufferItem[index]->size++;
        }

        if (hcVars->staTSBufferItem[index]->tsStatus == DOT11e_TS_None)
        {
            hcVars->staTSBufferItem[index]->trafficSpec.info.tsid =
                (tsid & 0x0000000F);
            hcVars->staTSBufferItem[index]->trafficSpec.info.priority =
                (priority & 0x00000007);
            hcVars->staTSBufferItem[index]->trafficSpec.info.direction =
                DOT11e_UPLINK;
            hcVars->staTSBufferItem[index]->
                trafficSpec.info.accessPolicy = 1;

            if (!MacDot11IsAp(dot11))
            {
                //call TS initiate for this TSID
                if (MacDot11ManagementAttemptToInitiateADDTS(
                    node, dot11, tsid ))
                {
                    hcVars->staTSBufferItem[index]->tsStatus =
                        DOT11e_TS_Requested;
                }
            }
            else
            {
                hcVars->staTSBufferItem[index]->tsStatus =
                    DOT11e_TS_Active;
            }
        }

}//MacDot11eStationMoveAPacketFromTheNetworkToTheTSsBuffer


//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationPrintStats
//  PURPOSE:     Print DOT11e CFP statistics for a station in BSS.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int interfaceIndex
//                  Interface index
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationPrintStats(
    Node* node,
     MacDataDot11* dot11,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "HCCA: Polls received = %d",
           dot11->totalNoOfQoSCFPollsReceived );
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: Data packets transmitted = %d",
        dot11->totalNoOfQoSCFDataFrameSend);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: Data packets received = %d",
        dot11->totalNoOfQoSCFDataFramesReceived);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "HCCA: NullData transmitted = %d",
        dot11->totalQoSNullDataTransmitted);
    IO_PrintStat(node, "MAC", DOT11e_MAC_STATS_LABEL,
        ANY_DEST, interfaceIndex, buf);

}//MacDot11eCfpStationPrintStats
