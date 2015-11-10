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
 * \file mac_dot11-sta.cpp
 * \brief Station and DCF access routines.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "mac_dot11.h"
#include "mac_dot11-mgmt.h"
#include "mac_dot11-pc.h"
#include "mac_dot11n.h"
#include "mac_phy_802_11n.h"

//-------------------------------------------------------------------------
// Static Functions
//-------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCTSTransmitted.
//  PURPOSE:     Hold for reply after sending CTS.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the CTS
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               NodeAddress address
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationCTSTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11StationSetState(node, dot11, DOT11_S_WFDATA);
    MacDot11StationStartTimer(node, dot11, dot11->noResponseTimeoutDuration);
}// MacDot11StationCTSTransmitted


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationRTSTransmitted.
//  PURPOSE:     Hold for reply after sending RTS.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the RTS
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               NodeAddress address
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationRTSTransmitted(Node* node, MacDataDot11* dot11)
{
    clocktype holdForCts;

    MacDot11StationSetState(node, dot11, DOT11_S_WFCTS);

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector txVector;
        PHY_GetTxVector(node,
                        dot11->myMacData->phyNumber,
                        txVector);
        
        txVector.length = DOT11_SHORT_CTRL_FRAME_SIZE;

        holdForCts =
            dot11->extraPropDelay +
            dot11->sifs +
            PHY_GetTransmissionDuration(
                node, dot11->myMacData->phyNumber,
                txVector) +
            dot11->extraPropDelay +
            dot11->slotTime;
    }
    else
    {
    holdForCts =
        dot11->extraPropDelay +
        dot11->sifs +
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            PHY_GetTxDataRateType(node, dot11->myMacData->phyNumber),
                    DOT11_SHORT_CTRL_FRAME_SIZE)
                    +
        dot11->extraPropDelay +
        dot11->slotTime;
    }
/*********HT END****************************************************/
    MacDot11StationStartTimer(node, dot11, holdForCts);

}// MacDot11StationRTSTransmitted

//---------------------------Power-Save-Mode-Updates---------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StartTimerForNextAwakePeriod
//  PURPOSE:     After expiry of this timer STA start, to listen
//                  the Transmission channel
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static BOOL MacDot11StartTimerForNextAwakePeriod(
    Node* node,
    MacDataDot11* dot11)
{
    clocktype delay = dot11->listenIntervals * dot11->beaconInterval;
    unsigned char dtimCount = (unsigned char)dot11->beaconDTIMCount;
    unsigned char dtimPeriod = (unsigned char)dot11->beaconDTIMPeriod;

    if (MacDot11IsIBSSStation(dot11) ||
        MacDot11IsAp(dot11) ||
        (!(MacDot11IsBssStation(dot11)
        && MacDot11IsStationJoined(dot11)
        && MacDot11IsStationSupportPSMode(dot11))&&
        (dot11->ScanStarted == FALSE))){
        return FALSE;
    }
    if (dot11->nextAwakeTime > node->getNodeTime()){
        delay = dot11->nextAwakeTime - node->getNodeTime();
    }
    else{
        if (dot11->isReceiveDTIMFrame == TRUE){
            if (!dtimCount && !(dtimPeriod >= dot11->listenIntervals)) {
                delay = dtimPeriod * dot11->beaconInterval;
            }
            else {
                if (dtimCount < (unsigned char)dot11->listenIntervals){
                    delay = dtimCount * dot11->beaconInterval;
                }
            }// end of if-else dtimCount
        }// end of if dot11->isReceiveDTIMFrame

        delay = delay - DOT11_TIME_UNIT -
            (node->getNodeTime() - dot11->beaconExpirationTime);
        dot11->nextAwakeTime = node->getNodeTime() + delay;
    }// end of if - else

    if (delay <= 0) {
        return FALSE;
    }
    MacDot11StationStartTimerOfGivenType(
        node,
        dot11,
        delay,
        MSG_MAC_DOT11_PSStartListenTxChannel);

    dot11->awakeTimerSequenceNo = dot11->timerSequenceNumber;
    return TRUE;
}// end of MacDot11StartTimerForNextAwakePeriod

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartListening
//  PURPOSE:     start listening of Transmission channel
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationStartListening(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->phyStatus == MAC_DOT11_PS_SLEEP){
        if (DEBUG_PS_CHANGE_STATE_DOZE_TO_ACTIVE){
            MacDot11Trace(node, dot11, NULL,
                "Start Listening Channel");
        }

        DOT11_ManagementVars* mngmtVars =
            (DOT11_ManagementVars *) dot11->mngmtVars;
        unsigned int phyNumber = (unsigned)dot11->myMacData->phyNumber;
        MacDot11ManagementStartListeningToChannel(
            node,
            phyNumber,
            (short)mngmtVars->currentChannel,
            dot11);

       dot11->phyStatus = MAC_DOT11_PS_ACTIVE;
       dot11->beaconsMissed = 0;
       dot11->BO = 0;
       dot11->ACs[0].BO = 0;
    }
    return;
}// end of MacDot11StationStartListening

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStopListening
//  PURPOSE:     Stop listening of Transmission channel
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationStopListening(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->phyStatus == MAC_DOT11_PS_ACTIVE
         && MacDot11StationPhyStatus(node, dot11) == PHY_IDLE){

        if (DEBUG_PS_CHANGE_STATE_ACTIVE_TO_DOZE){
            MacDot11Trace(node, dot11, NULL,
                "Stop Listening Channel");
        }

        DOT11_ManagementVars* mngmtVars =
            (DOT11_ManagementVars *) dot11->mngmtVars;
        unsigned int phyNumber = (unsigned)dot11->myMacData->phyNumber;

        MacDot11ManagementStopListeningToChannel(
            node,
            phyNumber,
            (short)mngmtVars->currentChannel);
       dot11->phyStatus = MAC_DOT11_PS_SLEEP;
    }
    return;
}// end of MacDot11StationStopListening

//--------------------------------------------------------------------------
//  NAME:        MacDot11SetExchangeVariabe
//  PURPOSE:     Set data transmission exchange variable
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Message pointer
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11SetExchangeVariabe(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_LongControlFrame* hdr =
        (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(msg);

    if (MacDot11IsIBSSStationSupportPSMode(dot11)
        && !(hdr->frameFlags & DOT11_MORE_DATA_FIELD)){

        if ((hdr->destAddr == ANY_MAC802)
            && (dot11->noOfReceivedBroadcastATIMFrame > 0)){
           dot11->noOfReceivedBroadcastATIMFrame--;
        }else{
            DOT11_ReceivedATIMFrameList* receiveNodeList =
                dot11->receiveNodeList;
            while (receiveNodeList != NULL){
                if (receiveNodeList->nodeMacAddr
                    == hdr->sourceAddr ){
                    receiveNodeList->atimFrameReceive = FALSE;
                    break;
                }else{
                    receiveNodeList = receiveNodeList->next;
                }
            }// end of while
        }
    }// end of if DOT11_MORE_DATA_FIELD
}// end of MacDot11SetExchangeVariabe

BOOL MacDot11PSModeDropPacket(
    Node* node,
    MacDataDot11* dot11,
    Queue* queue,
    Message* msg,
    int* noOfPacketDrop)
{
    BOOL ret = TRUE;
    int incomingMsgSize = MESSAGE_ReturnPacketSize(msg);
    Message* newMsg = NULL;
    BOOL isMsgRetrieved = FALSE;
    *noOfPacketDrop = 0;

    while (incomingMsgSize > 0){
         newMsg = NULL;
         isMsgRetrieved = FALSE;
         isMsgRetrieved = queue->retrieve(
             &newMsg,
             0,
             DEQUEUE_PACKET,
             node->getNodeTime());
         if (isMsgRetrieved){
            incomingMsgSize -= MESSAGE_ReturnPacketSize(newMsg);

            //remove Mac Header
            MESSAGE_RemoveHeader(
            node,
            newMsg,
            sizeof(DOT11_FrameHdr),
            TRACE_DOT11);


            MacDot11StationInformNetworkOfPktDrop(
                node,
                dot11,
                newMsg);
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, newMsg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
            dot11->psModeMacQueueDropPacket++;
            (*noOfPacketDrop)++;
         }
         else{
             break;
         }
    }// end of while
    // if function return TRUE then drop the incoming packet also
    if (MacDot11BufferPacket(
        queue,
        msg,
        node->getNodeTime())){
        ret = FALSE;
        MacDot11StationInformNetworkOfPktDrop(
            node,
            dot11,
            msg);
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        dot11->psModeMacQueueDropPacket++;
    }
    return ret;
}// end of MacDot11PSModeDropPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationPSPOLLTransmitted.
//  PURPOSE:     Hold for reply after sending PS POLL.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the RTS
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationPSPOLLTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    clocktype holdForAck;

    dot11->psPollPacketsSent++;
    if (dot11->dot11TxFrameInfo!= NULL )
    {
          if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
          {
              MacDot11MgmtQueue_DequeuePacket(node,dot11);

              if ((dot11->currentACIndex >= DOT11e_AC_BK) &&
                  dot11->currentMessage ==
                           dot11->ACs[dot11->currentACIndex].frameToSend) {
                  dot11->ACs[dot11->currentACIndex].
                      totalNoOfthisTypeFrameDeQueued++;
                  dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                  dot11->ACs[dot11->currentACIndex].BO = 0;
                  dot11->ACs[dot11->currentACIndex].
                      currentNextHopAddress = INVALID_802ADDRESS;
                  if (dot11->ACs[dot11->currentACIndex].frameInfo!= NULL)
                  {
                      MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                      dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
                      MESSAGE_Free(node,dot11->currentMessage);
                      dot11->currentMessage = NULL;
                  }
              }
              dot11->dot11TxFrameInfo = NULL;
          }
     }
    MacDot11StationResetCW(node, dot11);

    MacDot11StationSetState(node, dot11, DOT11_S_WFPSPOLL_ACK);

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector txVector;
        PHY_GetTxVector(node,
                        dot11->myMacData->phyNumber,
                        txVector);
        holdForAck =
            dot11->extraPropDelay +
            dot11->sifs +
            PHY_GetTransmissionDuration(
                node, dot11->myMacData->phyNumber,
                txVector) +
            dot11->extraPropDelay +
            dot11->slotTime;

    }
    else
    {
    holdForAck =
        dot11->extraPropDelay +
        dot11->sifs +
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            PHY_GetTxDataRateType(node, dot11->myMacData->phyNumber),
            DOT11_SHORT_CTRL_FRAME_SIZE) +
        dot11->extraPropDelay +
        dot11->slotTime;
        }
/*********HT END****************************************************/
    MacDot11StationStartTimer(node, dot11,  holdForAck);
}// MacDot11StationPSPOLLTransmitted

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationWillExchangeMoreFrame
//  PURPOSE:     Check that station need to exchange more packet.
//
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11StationWillExchangeMoreFrame(
    MacDataDot11* dot11)
{
    BOOL ret = FALSE;
    DOT11_ReceivedATIMFrameList* receiveNodeList = dot11->receiveNodeList;
    while ((ret == FALSE) && (receiveNodeList != NULL)){
        if (receiveNodeList->atimFrameReceive == TRUE){
            ret = TRUE;
            break;
        }
        receiveNodeList = receiveNodeList->next;
    }
    if (ret == FALSE){
        ret = MacDot11StationHasExchangeATIMFrame(dot11);
    }
    return ret;
}// end of
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAckTransmitted.
//  PURPOSE:     Ack sent. Check for packets in IP queue.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the ACK
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               NodeAddress address
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationAckTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->useDvcs) {

        // For smart antennas, go back to sending/receiving
        // omnidirectionally.

        PHY_UnlockAntennaDirection(
            node, dot11->myMacData->phyNumber);
    }

    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);

}// MacDot11StationAckTransmitted


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationUnicastedTransmitted.
//  PURPOSE:     Hold for ACK after data frame is transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the data
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//inline//
void MacDot11StationUnicastTransmitted(Node* node, MacDataDot11* dot11) {


    if (dot11->isHTEnable)
    {
        MacDot11nUnicastTransmitted(node, dot11);
        return;
    }
    clocktype holdForAck;

    MacDot11StationSetState(node, dot11, DOT11_S_WFACK);
    
    holdForAck =
        dot11->extraPropDelay +
        dot11->sifs +
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            PHY_GetTxDataRateType(node, dot11->myMacData->phyNumber),
            DOT11_SHORT_CTRL_FRAME_SIZE) +
        dot11->extraPropDelay +
        dot11->slotTime;

    MacDot11StationStartTimer(node, dot11, holdForAck);

}// MacDot11StationUnicastedTransmitted


//--------------------------------------------------------------------------
//  NAME:        MacDot11TransmissionHasFinished
//  PURPOSE:     Signal the end of transmission
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               NodeAddress address
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationTransmissionHasFinished(
    Node* node,
    MacDataDot11* dot11)
{
    switch (dot11->state) {
        case DOT11_X_ACK:
            MacDot11StationAckTransmitted(node, dot11);
            break;

        case DOT11_X_BROADCAST:

            // inc broadcast packets
            if (dot11->isHTEnable)
            {
                MacDot11nBroadcastTramsmitted(node, dot11);
                break;
            }
            
            if (dot11->dot11TxFrameInfo!= NULL )
            {
                if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
                {
                    if ((dot11->dot11TxFrameInfo->frameType >=
                                                   DOT11_ASSOC_REQ)
                        &&(dot11->dot11TxFrameInfo->frameType <=
                                                     DOT11_ACTION))
                    {
                        MacDot11ManagementIncrementSendFrame(node,
                                     dot11,
                                     dot11->dot11TxFrameInfo->frameType);
                        MacDot11ManagementFrameTransmitted(node, dot11);

                    }
                    if (dot11->dot11TxFrameInfo->frameType == DOT11_PROBE_REQ)
                    {
                        DOT11_ManagementVars * mngmtVars =
                            (DOT11_ManagementVars*) dot11->mngmtVars;
                        MacDot11ManagementStartTimerOfGivenType(
                                     node,
                                     dot11,
                                     mngmtVars->channelInfo->dwellTime,
                                     MSG_MAC_DOT11_Active_Scan_Short_Timer);
                        if (dot11->isHTEnable)
                        {
                            // Do nothing
                        }
                        else
                        {
                            MacDot11MgmtQueue_DequeuePacket(node,dot11);
                        }
                    }
                    if (dot11->dot11TxFrameInfo->frameType == DOT11_ATIM)
                    {
                        MESSAGE_Free(node,dot11->currentMessage);
                        dot11->currentMessage = NULL;
                        MEM_free(dot11->dot11TxFrameInfo);
                    }
                    if ((dot11->currentACIndex >= DOT11e_AC_BK)
                        && dot11->currentMessage ==
                        dot11->ACs[dot11->currentACIndex].frameToSend) {
                        dot11->ACs[dot11->currentACIndex].
                            totalNoOfthisTypeFrameDeQueued++;
                        dot11->ACs[dot11->currentACIndex].
                                          frameToSend = NULL;
                        dot11->ACs[dot11->currentACIndex].BO = 0;
                        dot11->ACs[dot11->currentACIndex].
                            currentNextHopAddress = INVALID_802ADDRESS;
                        if (dot11->ACs[dot11->currentACIndex].
                                                      frameInfo!= NULL)
                        {
                            MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                            dot11->ACs[dot11->currentACIndex].
                                                   frameInfo = NULL;
                        }
                    }
                    dot11->dot11TxFrameInfo = NULL;
                }
            }
            else
            {
                MacDot11IBSSStationDequeuePacketFromLocalQueue(
                    node,
                    dot11,
                    dot11->state);
             }

            dot11->broadcastPacketsSentDcf++;

            MacDot11StationResetCurrentMessageVariables(node, dot11);
//---------------------------Power-Save-Mode-End-Updates-----------------//
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationResetCW(node, dot11);
            MacDot11StationCheckForOutgoingPacket(node, dot11, TRUE);
            break;

        case DOT11_X_UNICAST:
            if (dot11->dot11TxFrameInfo!= NULL)
            {
                if ((dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ)
                    &&(dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)
                    &&(dot11->currentMessage == dot11->dot11TxFrameInfo->msg))
                {
                    MacDot11ManagementFrameTransmitted(node, dot11);
                }
            }
            MacDot11StationUnicastTransmitted(node, dot11);
            break;

        case DOT11_X_RTS:
            MacDot11StationRTSTransmitted(node, dot11);
            break;
        case DOT11_X_CTS:
            MacDot11StationCTSTransmitted(node, dot11);
            break;
        case DOT11_X_BEACON:
            // dot11s. Beacon transmitted.
            // After MP is done with the beacon frame,
            // hand processing over to the AP routine.
            if (dot11->isMP) {
                Dot11s_BeaconTransmitted(node, dot11);
            }

            MacDot11ApBeaconTransmitted(node, dot11);

            // If PC is working in contention Period mode
            if (MacDot11IsPC(dot11)){
              if ((dot11->apVars->lastDTIMCount != 0 )||
                  (dot11->pcVars->cfSet.cfpCount != 0)){
                 MacDot11StationResetCW(node, dot11);
                 MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
              }
            }
            else {
            MacDot11StationResetCW(node, dot11);
            MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            }
            break;

        case DOT11_X_MANAGEMENT:
            MacDot11ManagementFrameTransmitted(node, dot11);
            break;
        case DOT11_CFP_START:
            MacDot11CfpFrameTransmitted(node, dot11);
            break;
//--------------------HCCA-Updates Start---------------------------------//
        case DOT11e_CAP_START:
            MacDot11eCfpFrameTransmitted(node,dot11);
            break;
//--------------------HCCA-Updates End-----------------------------------//

//---------------------------Power-Save-Mode-Updates---------------------//
        case DOT11_X_PSPOLL:
            MacDot11StationPSPOLLTransmitted(node, dot11);
            break;
        case DOT11_X_IBSSBEACON:
            MacDot11StationBeaconTransmittedOrReceived(
                node,
                dot11,
                BEACON_TRANSMITTED);
            break;
//---------------------------Power-Save-Mode-End-Updates-----------------//
        default:
            ERROR_ReportError("MacDot11TransmissionHasFinished: "
                "Invalid transmit state.\n");
            break;
    }//switch//
}

//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11IBSSStationStartBeaconTimer.
//  PURPOSE:     Send beacon timeout event.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11IBSSStationStartBeaconTimer(
    Node* node,
    MacDataDot11* dot11)
{
    // a zero value disables the beacon
    if (dot11->beaconInterval <= 0)
        return;

    // Start beacon timer
    clocktype delay = dot11->beaconTime
        - node->getNodeTime()
        - dot11->delayUntilSignalAirborn
        - dot11->extraPropDelay;

    if (delay <= 0){
        delay = 1;
    }

    MacDot11StationStartTimerOfGivenType(node, dot11,
        delay, MSG_MAC_DOT11_Beacon);
    return;
} //MacDot11IBSSStationStartBeaconTimer

//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartBeaconTimer.
//  PURPOSE:     Send beacon timeout event.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11StationStartBeaconTimer(
    Node* node,
    MacDataDot11* dot11)
{
    clocktype delay;
    // a zero value disables the beacon
    if (dot11->beaconInterval <= 0)
        return;
    // Start beacon timer
    delay =
        dot11->beaconInterval
        - dot11->delayUntilSignalAirborn
        - dot11->extraPropDelay;

    if (delay <= 0){
        delay = 1;
    }

    MacDot11StationStartTimerOfGivenType(node, dot11, delay,
        MSG_MAC_DOT11_Beacon);

} //MacDot11StationStartBeaconTimer


//-------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------


//--------------------------------------------------------------------------
//  NAME:        MacDot11eStationRetryForInternalCollision.
//  PURPOSE:     Retransmit frame when CTS or ACK timeout occurs.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  AC index number with default value DOT11e_INVALID_AC
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eStationRetryForInternalCollision(
    Node* node,
    MacDataDot11* dot11,
    int acIndex)
{
    BOOL UseShortPacketRetryCounter;
    BOOL RetryTheTransmission = FALSE;

    ERROR_Assert(!dot11->isHTEnable,"HT Disabled");
    ERROR_Assert(acIndex >= DOT11e_AC_BK,
        "MacDot11eStationRetryForInternalCollision: "
        "the received AC index is incorrect. \n");

    // Long retry count only applies to data frames greater
    // than RTS_THRESH.  RTS uses short retry count.
    //Support Fix: RTS Threshold should be applied on Mac packet
    UseShortPacketRetryCounter =
        (MESSAGE_ReturnPacketSize(dot11->ACs[acIndex].frameToSend) <=
          dot11->rtsThreshold);

    // If not greater than maximum retry count allowed, retransmit frame
    if (UseShortPacketRetryCounter)
    {
        dot11->ACs[acIndex].QSRC = dot11->ACs[acIndex].QSRC + 1;

        if (dot11->ACs[acIndex].QSRC < dot11->shortRetryLimit) {
            RetryTheTransmission = TRUE;
        }
        else
        {
            RetryTheTransmission = FALSE;
            dot11->ACs[acIndex].QSRC = 0;
        }
    }
    else {
        dot11->ACs[acIndex].QLRC++;

        if (dot11->ACs[acIndex].QLRC < dot11->longRetryLimit) {
            RetryTheTransmission = TRUE;
        }
        else {
            RetryTheTransmission = FALSE;
            dot11->ACs[acIndex].QLRC = 0;
        }
    }

    if (RetryTheTransmission)
    {
        MacDot11eStationIncreaseCWForAC(node, dot11, acIndex);

        MacDot11StationSetBackoffIfZero(
                node,
                dot11,
                acIndex);
    }
    else {
        // Exceeded maximum retry count allowed, so drop frame
        dot11->pktsDroppedDcf++;

#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, dot11->ACs[acIndex].frameToSend, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif


        // Drop frame from queue
        MacDot11Trace(node, dot11, NULL, "Drop, exceeds retransmit count");

        if (!MacDot11eACHasManagementFrameToSend(node, dot11, acIndex))
        {
//--------------------Power-Save-Mode not supported in 802.11e-------------//
            //MacDot11IBSSStationDequeuePacketFromLocalQueue(
            //    node,
            //    dot11,
            //    DOT11_X_UNICAST);

            char* frametype = (char *)MESSAGE_ReturnPacket
                (dot11->ACs[acIndex].frameToSend);
            if (*frametype == DOT11_DATA)
            {
                MESSAGE_RemoveHeader(node,
                            dot11->ACs[acIndex].frameToSend,
                            sizeof(DOT11_FrameHdr),
                            TRACE_DOT11);
            }
            else
            {
                MESSAGE_RemoveHeader(node,
                            dot11->ACs[acIndex].frameToSend,
                            sizeof(DOT11e_FrameHdr),
                            TRACE_DOT11);
            }

            //inform network of packet drop
            MAC_NotificationOfPacketDrop(node,
                                 dot11->ACs[acIndex].currentNextHopAddress,
                                 dot11->myMacData->interfaceIndex,
                                 dot11->ACs[acIndex].frameToSend);

            if (dot11->ACs[acIndex].frameInfo != NULL)
            {
                if (dot11->ACs[acIndex].frameToSend ==
                                          dot11->ACs[acIndex].frameInfo->msg)
                {
                    dot11->ACs[acIndex].frameInfo = NULL;
                }
            }

            dot11->ACs[acIndex].totalNoOfthisTypeFrameDeQueued++;
            dot11->ACs[acIndex].frameToSend = NULL;
            dot11->ACs[acIndex].BO = 0;
            dot11->ACs[acIndex].currentNextHopAddress = INVALID_802ADDRESS;
            MEM_free(dot11->ACs[acIndex].frameInfo);
            dot11->ACs[acIndex].frameInfo = NULL;

        }
      else if (MacDot11eACHasManagementFrameToSend(node, dot11, acIndex))
      {
            MacDot11StationInformManagementOfPktDrop(node,
                                             dot11,
                                             dot11->ACs[acIndex].frameInfo);


            dot11->ACs[acIndex].totalNoOfthisTypeFrameDeQueued++;

            MESSAGE_Free(node,dot11->ACs[acIndex].frameToSend);
            dot11->ACs[acIndex].frameToSend = NULL;

            dot11->ACs[acIndex].BO = 0;
            dot11->ACs[acIndex].currentNextHopAddress = INVALID_802ADDRESS;

            MEM_free(dot11->ACs[acIndex].frameInfo);
            dot11->ACs[acIndex].frameInfo = NULL;

            MacDot11MgmtQueue_DequeuePacket(node,dot11);
        }

        MacDot11eStationResetCWForAC(node, dot11, acIndex);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11eStationIncreaseCWForAC.
//  PURPOSE:     Increase control window for specific AC since node has to
//               retransmit.
//  PARAMETERS:  Node* node
//                  Pointer to node thatis increasing its contention window
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  AC index number with default value DOT11e_INVALID_AC
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eStationIncreaseCWForAC(
    Node* node,
    MacDataDot11* dot11,
    int acIndex)
{
    // DOT11e Updates
    if (MacDot11IsQoSEnabled(node, dot11))
    {
        if (acIndex >= DOT11e_AC_BK)
        {
            dot11->ACs[acIndex].CW =
                            MIN((((dot11->ACs[acIndex].CW + 1) * 2) - 1),
                            dot11->ACs[acIndex].cwMax);
        }
    }
}// MacDot11eStationIncreaseCWForAC


//--------------------------------------------------------------------------
//  NAME:        MacDot11eACHasManagementFrameToSend
//  PURPOSE:     AC has a management frame to send.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  AC index number with default value DOT11e_INVALID_AC
//  RETURN:      BOOL
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
BOOL MacDot11eACHasManagementFrameToSend(
    Node* node,
    MacDataDot11* dot11,
    int acIndex)
{
    if (dot11->ACs[acIndex].frameInfo != NULL)
    {
        if ((dot11->ACs[acIndex].frameInfo->frameType >= DOT11_ASSOC_REQ
            && dot11->ACs[acIndex].frameInfo->frameType <= DOT11_ACTION ||
            dot11->ACs[acIndex].frameInfo->frameType == DOT11_PS_POLL)&&
            (dot11->ACs[acIndex].frameToSend ==
                                         dot11->ACs[acIndex].frameInfo->msg))
            return TRUE;
        else
            return FALSE;
    }
    else
        return FALSE;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11eStationResetCWForAC.
//  PURPOSE:     Reset congestion window for specific AC
//  PARAMETERS:  Node* node
//                  Pointer to node that is resetting its contention window
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  AC index number with default value DOT11e_INVALID_AC
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------

void MacDot11eStationResetCWForAC(
    Node* node,
    MacDataDot11* dot11,
    int acIndex)
{
    if (MacDot11IsQoSEnabled(node, dot11))
    {
        if (acIndex >= DOT11e_AC_BK)
        {
            dot11->ACs[acIndex].CW = dot11->ACs[acIndex].cwMin;
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationRetransmit.
//  PURPOSE:     Retransmit frame when CTS or ACK timeout occurs.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11StationRetransmit(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL UseShortPacketRetryCounter;
    BOOL RetryTheTransmission = FALSE;

    ERROR_Assert(MacDot11StationHasFrameToSend(dot11),
        "MacDot11StationRetransmit: "
        "There is no message in the local buffer.\n");

    // Long retry count only applies to data frames greater
    // than RTS_THRESH.  RTS uses short retry count.
    //Support Fix: RTS Threshold should be applied on Mac packet
    UseShortPacketRetryCounter =
        ((dot11->state == DOT11_S_WFCTS) ||
         (MacDot11StationGetCurrentMessageSize(dot11) <=
          dot11->rtsThreshold));

    // If not greater than maximum retry count allowed, retransmit frame
    if (UseShortPacketRetryCounter) {
        dot11->SSRC++;
        if (MacDot11IsQoSEnabled(node, dot11) && dot11->currentACIndex >= DOT11e_AC_BK){
            dot11->ACs[dot11->currentACIndex].QSRC = dot11->SSRC;
        }
        if (dot11->SSRC < dot11->shortRetryLimit) {
            RetryTheTransmission = TRUE;

            if ((dot11->useDvcs) &&
                (dot11->SSRC >= dot11->directionalShortRetryLimit))
            {
                AngleOfArrivalCache_Delete(
                    &dot11->directionalInfo->angleOfArrivalCache,
                    dot11->currentNextHopAddress);
            }
        }
        else {
            RetryTheTransmission = FALSE;
            dot11->SSRC = 0;
        }
    }
    else {
        dot11->SLRC++;
        if (MacDot11IsQoSEnabled(node, dot11) && dot11->currentACIndex >= DOT11e_AC_BK){
            dot11->ACs[dot11->currentACIndex].QLRC = dot11->SLRC;
        }
        if (dot11->SLRC < dot11->longRetryLimit) {
            RetryTheTransmission = TRUE;
        }
        else {
            RetryTheTransmission = FALSE;
            dot11->SLRC = 0;
        }
    }


    if (RetryTheTransmission) {
        MacDot11StationIncreaseCW(node, dot11);
            MacDot11StationSetBackoffIfZero(
                node,
                dot11,
                dot11->currentACIndex);
        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(node, dot11);

        // dot11s. Retransmit notification.
        // Inform MP of retransmit event
        if (dot11->isMP && dot11->txFrameInfo != NULL) {
                Dot11s_PacketRetransmitEvent(node, dot11, dot11->txFrameInfo);
        }
    }
    else {
        // Exceeded maximum retry count allowed, so drop frame
        dot11->pktsDroppedDcf++;

        // Drop frame from queue
        MacDot11Trace(node, dot11, NULL, "Drop, exceeds retransmit count");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, dot11->currentMessage, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        if (dot11->isMP && dot11->txFrameInfo != NULL) {
            Dot11s_PacketDropEvent(node, dot11, dot11->txFrameInfo);
            if (dot11->txFrameInfo->SA == dot11->selfAddr) {
                if (dot11->txFrameInfo->frameType == DOT11_MESH_DATA) {
                    MESSAGE_RemoveHeader(node,
                        dot11->currentMessage,
                        DOT11s_DATA_FRAME_HDR_SIZE,
                        TRACE_DOT11);
                }
                else if (dot11->txFrameInfo->frameType == DOT11_DATA)
                {
                    MESSAGE_RemoveHeader(node,
                      dot11->currentMessage,
                      DOT11_DATA_FRAME_HDR_SIZE,
                      TRACE_DOT11);
                }

                MacDot11StationInformNetworkOfPktDrop(
                    node, dot11, dot11->currentMessage);
            }
          else
          {
                MESSAGE_Free(node, dot11->currentMessage);
          }
        }
     else if (!MacDot11StationHasManagementFrameToSend(node,dot11) &&
            MacDot11StationHasNetworkMsgToSend(dot11)){
//---------------------------Power-Save-Mode-Updates---------------------//
            MacDot11IBSSStationDequeuePacketFromLocalQueue(
                node,
                dot11,
                DOT11_X_UNICAST);
//---------------------------Power-Save-Mode-End-Updates-----------------//
            if (node->macData[dot11->myMacData->interfaceIndex]->macStats)
            {
                BOOL isCtrlFrame = FALSE;
                BOOL isMyFrame = FALSE;
                BOOL isAnyFrame = FALSE;
                int headerSize = -1;
                Mac802Address destAddr;
                NodeId destId;

                MacDot11GetPacketProperty(
                    node,
                    dot11->currentMessage,
                    dot11->myMacData->interfaceIndex,
                    headerSize,
                    destAddr,
                    destId,
                    isCtrlFrame,
                    isMyFrame,
                    isAnyFrame);
        
                int destIdAsInt = destId;
                if (!isMyFrame || isAnyFrame)
                {
                    destIdAsInt = -1;
                }
                
                node->macData[dot11->myMacData->interfaceIndex]->stats->AddFrameDroppedSenderDataPoints(
                    node, destIdAsInt, dot11->myMacData->interfaceIndex,
                    MESSAGE_ReturnPacketSize(dot11->currentMessage));
            }
            char* frametype = (char *)MESSAGE_ReturnPacket
                (dot11->currentMessage);
            if (*frametype == DOT11_DATA)
            {
                MESSAGE_RemoveHeader(node,
                            dot11->currentMessage,
                            sizeof(DOT11_FrameHdr),
                            TRACE_DOT11);
            }
            else
            {
                MESSAGE_RemoveHeader(node,
                            dot11->currentMessage,
                            sizeof(DOT11e_FrameHdr),
                            TRACE_DOT11);
            }
            MacDot11StationInformNetworkOfPktDrop(
                node,
                dot11,
                dot11->currentMessage);
            if (dot11->dot11TxFrameInfo != NULL) {
                if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
                dot11->dot11TxFrameInfo = NULL;
            }
            if (dot11->currentACIndex >= DOT11e_AC_BK) {
                dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameDeQueued++;
                dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                dot11->ACs[dot11->currentACIndex].BO = 0;
                dot11->ACs[dot11->currentACIndex].currentNextHopAddress =
                    INVALID_802ADDRESS;
                MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
            }
        }
      else if (MacDot11StationHasManagementFrameToSend(node,dot11))
        {
            MacDot11StationInformManagementOfPktDrop(node,
                                                     dot11,
                                                     dot11->dot11TxFrameInfo);
            if (dot11->dot11TxFrameInfo != NULL &&
                dot11->dot11TxFrameInfo->frameType == DOT11_ATIM)
            {
                MESSAGE_Free(node,dot11->dot11TxFrameInfo->msg);
                MEM_free(dot11->dot11TxFrameInfo);
                dot11->dot11TxFrameInfo = NULL;
                dot11->currentMessage = NULL;
            }
            else
            {
                if (dot11->currentACIndex >= DOT11e_AC_BK) {
                    dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameDeQueued++;
                    dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                    dot11->ACs[dot11->currentACIndex].BO = 0;
                    dot11->ACs[dot11->currentACIndex].currentNextHopAddress =
                       INVALID_802ADDRESS;
                    MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                    dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
                }
                MacDot11MgmtQueue_DequeuePacket(node,dot11);
                dot11->dot11TxFrameInfo = NULL;
                MESSAGE_Free(node,dot11->currentMessage);
            }

        }
        if (dot11->useDvcs) {
            AngleOfArrivalCache_Delete(
                &dot11->directionalInfo->angleOfArrivalCache,
                dot11->currentNextHopAddress);
        }
        MacDot11StationResetCurrentMessageVariables(node, dot11);
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationResetCW(node, dot11);
        MacDot11StationCheckForOutgoingPacket(node, dot11, TRUE);
    }
} // MacDot11StationRetransmit

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitFrame.
//  PURPOSE:     Initiate transmit of RTS or data frame.
//  PARAMETERS:  Node* node that wants to send a data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11StationTransmitFrame(
    Node* node,
    MacDataDot11* dot11)
{

    ERROR_Assert(MacDot11StationPhyStatus(node, dot11) == PHY_IDLE,
        "MacDot11TransmitFrame: "
        "Physical status is not IDLE.\n");

    ERROR_Assert(MacDot11StationHasFrameToSend(dot11),
        "MacDot11TransmitFrame: "
        "There is no message in the local buffer.\n");

    if (MacDot11StationHasInstantFrameToSend(dot11)){

        MacDot11StationTransmitInstantFrame(node, dot11);
        dot11->instantMessage = NULL;

    }
    else{
    // Check if there is a management frame to send
        if (MacDot11StationHasManagementFrameToSend(node,dot11)){
            if (MacDot11IsIBSSStationSupportPSMode(dot11)
                && !MacDot11IsATIMDurationActive(dot11)){
                // can not transmit atim frame
                    if (dot11->dot11TxFrameInfo!= NULL &&
                        dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
                    {
                        MacDot11ManagementResetSendFrame(
                        node,
                        dot11);

                        MacDot11StationSetState(
                        node,
                        dot11,
                        DOT11_S_IDLE);

                        MacDot11StationResetCW(node, dot11);
                        return;
                    }
                    else if (dot11->dot11TxFrameInfo== NULL)
                    {
                        MacDot11ManagementResetSendFrame(
                            node,
                            dot11);

                        MacDot11StationSetState(
                            node,
                            dot11,
                            DOT11_S_IDLE);

                        MacDot11StationResetCW(node, dot11);
                        return;
                    }
        }
        dot11->dataRateInfo =
        MacDot11StationGetDataRateEntry(
        node,
        dot11,
        dot11->currentNextHopAddress);
        ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                     dot11->currentNextHopAddress,
                     "Address does not match");
        MacDot11StationAdjustDataRateForNewOutgoingPacket(node, dot11);

        if ((dot11->currentNextHopAddress == ANY_MAC802) ||
            (dot11->dot11TxFrameInfo->frameType == DOT11_PS_POLL)||
            (dot11->dot11TxFrameInfo->frameType == DOT11_BEACON))
        {
            if (dot11->isHTEnable)
             {
                 MacDot11nSendMpdu(node, dot11);
             }
             else
             {
            MacDot11StationTransmitDataFrame(node, dot11);
        }
        }
        else
        {
           int packetSize = MacDot11StationGetCurrentMessageSize(dot11);
           if (dot11->isHTEnable)
          {
             packetSize += dot11->dot11TxFrameInfo->ampduOverhead;
          }

           if ((packetSize > dot11->rtsThreshold)&&
                (dot11->currentNextHopAddress != ANY_MAC802)){
                MacDot11StationTransmitRTSFrame(node, dot11);
            }else{
                  if (dot11->isHTEnable)
                 {
                     MacDot11nSendMpdu(node, dot11);
                 }
                 else
                 {
                MacDot11StationTransmitDataFrame(node, dot11);
            }
        }
        }
    } else {
        //Support Fix: RTS Threshold should be applied on Mac packet
        int packetSize = MacDot11StationGetCurrentMessageSize(dot11);
        if (dot11->isHTEnable)
        {
            packetSize += dot11->dot11TxFrameInfo->ampduOverhead;
        }
            dot11->dataRateInfo = MacDot11StationGetDataRateEntry(
                node, dot11, dot11->currentNextHopAddress);
            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                        dot11->currentNextHopAddress,
                        "Address does not match");
            MacDot11StationAdjustDataRateForNewOutgoingPacket(node,
                                                              dot11);

            if ((packetSize > dot11->rtsThreshold) &&
                    (dot11->currentNextHopAddress != ANY_MAC802)){
                MacDot11StationTransmitRTSFrame(node, dot11);
            }else{
                if (dot11->isHTEnable)
                 {
                     MacDot11nSendMpdu(node, dot11);
                 }
                 else
                 {
                MacDot11StationTransmitDataFrame(node, dot11);
            }
        }
    }
    }
}// MacDot11StationTransmitFrame

//---------------------------Power-Save-Mode-Updates---------------------//

static // inline//
BOOL MacDot11IsBeaconMissed(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL ret = TRUE;

    //Don't count the beacon missed while scanning.
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
    if (mngmtVars->state >= DOT11_S_M_SCAN &&
            mngmtVars->state < DOT11_S_M_SCAN_FAILED)
    {
        ret = FALSE;
    }

    if (!MacDot11IsStationSupportPSMode(dot11))
        return ret;

    if (dot11->phyStatus == MAC_DOT11_PS_SLEEP)
        ret = FALSE;
   return ret;
}// end of MacDot11IsBeaconMissed
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCanHandleDueBeacon
//  PURPOSE:     For a AP, attempt to send the beacon.
//               For a STA, set NAV.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if the due beacon could be handled.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
BOOL MacDot11StationCanHandleDueBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    ERROR_Assert(dot11->beaconIsDue,
        "MacDot11CanHandleDueApBeacon: Beacon is not due.\n");

//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsIBSSStationSupportPSMode(dot11)){
        return MacDot11StationAttemptToTransmitBeacon(node, dot11);
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//

    if (MacDot11IsAp(dot11))
    {
        if (MacDot11IsPC(dot11) &&
            dot11->cfpState == DOT11_S_CFP_WFBEACON)
        {
            MacDot11ApTransmitBeacon(node, dot11);
            return TRUE;
        }
        else
        {
            return MacDot11ApAttemptToTransmitBeacon(node, dot11);
        }
    }
    else if (MacDot11IsBssStation(dot11)) {
        if (MacDot11IsAssociationDynamic(dot11)){
            if (dot11->beaconsMissed > DOT11_BEACON_MISSED_MAX){
                // Start New scanning: if there is any manamement frame wait
                // for some and then go for channel scanning.
                if (dot11->stationAssociated == TRUE)
                {

                    // Reset station Management
                    MacDot11ManagementReset(node, dot11);
                    dot11->beaconsMissed = 0;

                    if (DEBUG_BEACON) {
                        char errString[MAX_STRING_LENGTH];

                        sprintf(errString,
                            "MacDot11StationCanHandleDueBeacon:"
                            " Station %d did not hear a beacon.\n",
                            node->nodeId);

                        ERROR_ReportWarning(errString);
                    }
            }
            } else {

                // Protect against previous missed beacons
                if (dot11->beaconTime < node->getNodeTime())
                    dot11->beaconTime += dot11->beaconInterval;
//---------------------------Power-Save-Mode-Updates---------------------//
                if (MacDot11IsBeaconMissed(node, dot11))
                    dot11->beaconsMissed++;
//---------------------------Power-Save-Mode-End-Updates-----------------//
            }

            dot11->beaconIsDue = FALSE;

            // Start timer for next expected beacon
            MacDot11StationStartBeaconTimer(node, dot11);

            if (DEBUG_BEACON) {
                char timeStr[100];

                ctoa(node->getNodeTime(), timeStr);

                printf("%d TIMEOUT beacon frame at %s\n", node->nodeId,
                    timeStr);
            }

            return TRUE;
        }
    }
    else {
        ERROR_ReportError("MacDot11CanHandleDueApBeacon: "
            "Beacon should not become due for stations other "
            "than an AP or BSS station or PC.\n");
        return FALSE;
    }

    return FALSE;

}// MacDot11StationCanHandleDueBeacon

//---------------------------Power-Save-Mode-Updates---------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHasExchangeATIMFrame
//  PURPOSE:     Check that AP has been buffered the packet for this STA.
//
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11StationHasExchangeATIMFrame(
    MacDataDot11* dot11)
{
    BOOL ret = FALSE;
    DOT11_AttachedNodeList* attachNodeList = dot11->attachNodeList;
    DOT11_ReceivedATIMFrameList *receiveNodeList = dot11->receiveNodeList;
    if (dot11->noOfReceivedBroadcastATIMFrame > 0
        || dot11->transmitBroadcastATIMFrame == TRUE){
        ret = TRUE;
    }
    while ((ret == FALSE) && (attachNodeList != NULL)){
        if (( attachNodeList->atimFramesACKReceived == TRUE)
            && (attachNodeList->noOfBufferedUnicastPacket > 0)){
            ret = TRUE;
            break;
        }
        attachNodeList = attachNodeList->next;
    }

    while ((ret == FALSE) && receiveNodeList){
        if (receiveNodeList->atimFrameReceive == TRUE){
            ret = TRUE;
            break;
        }
        receiveNodeList = receiveNodeList->next;
    }

    return ret;
}// end of MacDot11StationHasExchangeATIMFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsUnicastPacketBufferedAtAP
//  PURPOSE:     Check that AP has been buffered the packet for this STA.
//
//  PARAMETERS:  unsigned char* TIMElementPointer
//                  Pointer to beacon frame
//               DOT11_PS_TIMFrame* timFrame
//                  pointer of tim frame
//               int associationId
//                  assign association id for this STA
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

BOOL MacDot11IsUnicastPacketBufferedAtAP(
    unsigned char* TIMElementPointer,
    DOT11_PS_TIMFrame* timFrame,
    int associationId){

//    The Partial Virtual Bitmap field consists of octets numbered N1 through N2
//    of the traffic indication virtual bitmap, where N1 is the largest even
//    number such that bits numbered 1 through (N1 * 8) - 1 in the bitmap are
//    all 0 and N2 is the smallest number such that bits numbered (N2 + 1) -?8
//    through 2007 in the bitmap are all 0. In this case, the Bitmap Offset
//    subfield value contains the number [N1/2], and the Length field will be
//    set to (N2 - N1) + 4.


    BOOL result = FALSE;
    unsigned char bitMapOffset = (unsigned char)(timFrame->bitmapControl >> 1);
    int N1 = bitMapOffset * 2;// beaconOffset = N1/2;
    int N2 = N1 + timFrame->length - 4;//length = (N2 - N1) + 4;
    int bitNo = associationId % 8;
    int octetNo = associationId / 8;

    // Check weather the bit is set or not for the received association ID.
    if ((octetNo >= N1 && octetNo <= N2)){
        result = ((TIMElementPointer[octetNo - N1]
              >> bitNo) & 0x01);
    }// end of if

   return result;
}// end of MacDot11IsUnicastPacketBufferedAtAP

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSleepIfNoData
//  PURPOSE:     Station checks its status, if data present at both end STA
//               and AP then first transmit data then receive the data from
//               AP. If both the end is idle then stop listening the
//               transmission channel.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationSleepIfNoData (
    Node* node,
    MacDataDot11* dot11)
{
    if ((dot11->apHasBroadcastDataToSend == TRUE)
        || (dot11->currentMessage != NULL)
        || dot11->apHasUnicastDataToSend == TRUE){
        // Receive broadcast packet from AP
    }

        else{
            if ((dot11->beaconExpirationTime != 0)
                && (dot11->beaconIsDue == FALSE)
                && !dot11->beaconsMissed){
                if (MacDot11StartTimerForNextAwakePeriod(node, dot11)){
                    MacDot11StationStopListening(node, dot11);
                }
            }// end of  if dot11->beaconExpirationTime
        }// end of if-else
}// end of MacDot11StationSleepIfNoData

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHasATIMFrameToSend
//  PURPOSE:     Attempt to send a ATIM frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if ATIM frame presents for any node
//  ASSUMPTION:  Where DIFS has elapsed or is in back-off, no
//               additional delay is necessary.
//--------------------------------------------------------------------------
BOOL MacDot11StationHasATIMFrameToSend(
    Node* node,
    MacDataDot11* dot11)
{
     BOOL ret = FALSE;
     Mac802Address destinationNodeAddress;
     DOT11_AttachedNodeList *attachNodeList = dot11->attachNodeList;

       if ((dot11->transmitBroadcastATIMFrame == FALSE )
            && (dot11->noOfBufferedBroadcastPacket > 0)){
            // Send the broadcast ATIM frame
            destinationNodeAddress = ANY_MAC802;
        }
        else{
            while (attachNodeList != NULL){
                if ((attachNodeList->atimFramesSend == FALSE)&&
                    (attachNodeList->noOfBufferedUnicastPacket > 0)){
                    destinationNodeAddress = attachNodeList->nodeMacAddr;
                    break;
                } // end of if
                attachNodeList = attachNodeList->next;
            } // end of while attachNodeList
        }// end of if- else

    if (destinationNodeAddress != INVALID_802ADDRESS)
    {
        if (dot11->dot11TxFrameInfo!= NULL &&
        dot11->dot11TxFrameInfo->frameType == DOT11_ATIM)
        {
            ret = TRUE;
        }
        else
        {
            ret = MacDot11ManagementBuildATIMFrame(
                  node,
                  dot11,
                  destinationNodeAddress);
        }
        if (ret == TRUE)
        {
            if (destinationNodeAddress != ANY_MAC802)
            {
                attachNodeList->atimFramesSend = TRUE;
            }
        }
    }
    return ret;
}// end of MacDot11StationHasATIMFrameToSend

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAttemptToTransmitBeacon
//  PURPOSE:     Attempt to send a beacon.
//               Checks permissible states and sets the beacon timer.
//               Where timer has elapsed, makes a call to transmit beacon.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if states permit or beacon transmit occurs
//  ASSUMPTION:  Where DIFS has elapsed or is in back-off, no
//               additional delay is neccary.
//               The beacon can have precedance when waiting for CTS.
//--------------------------------------------------------------------------
BOOL MacDot11StationAttemptToTransmitBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL couldTransmit = FALSE;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE
        || MacDot11IsTransmittingState(dot11->state)
        || MacDot11IsWaitingForResponseState(dot11->state))
    {
        return FALSE;
    }
    switch (dot11->state) {
        case DOT11_S_IDLE:
        case DOT11_S_NAV_RTS_CHECK_MODE:
        case DOT11_S_WF_DIFS_OR_EIFS:
        {
            MacDot11StationSetState(node, dot11, DOT11_S_WFIBSSJITTER);
            clocktype delay = (clocktype)
                (RANDOM_nrand(dot11->seed) % DOT11_BEACON_JITTER);
            delay += dot11->delayUntilSignalAirborn + dot11->extraPropDelay;
            MacDot11StationStartTimer(
                node,
                dot11,
                delay);
            couldTransmit = TRUE;
        }
            break;

        case DOT11_S_BO:
            // Have waited enough, can transmit immediately
            dot11->BO = 0;
            MacDot11StationTransmitBeacon(node, dot11);
            couldTransmit = TRUE;
            break;

        case DOT11_S_WFIBSSBEACON:
            MacDot11StationTransmitBeacon(node, dot11);
            couldTransmit = TRUE;
            break;
        case DOT11_S_WFIBSSJITTER:
            MacDot11StationSetState(node, dot11, DOT11_S_WFIBSSBEACON);
            MacDot11StationStartTimer(node, dot11, dot11->txSifs);
            break;
        default:
            break;
    } //switch
    return couldTransmit;
}// MacDot11StationAttemptToTransmitBeacon

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAddIBSSParameterSet
//  PURPOSE:     Add IBSS parameter set in to beacon.
//
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               unsigned char* beacon
//               beacon data pointer
//  RETURN:      int
//  ASSUMPTION:  NONE
//--------------------------------------------------------------------------
int MacDot11StationAddIBSSParameterSet(
    MacDataDot11* dot11,
    unsigned char* beacon)
{
    DOT11_PS_IBSSParameter* ibssParameter = NULL;

    ERROR_Assert(beacon != NULL, "MacDot11StationAddIBSSParameterSet \
        Beacon frame can not be null\n");

    ibssParameter = ( DOT11_PS_IBSSParameter* )beacon;

    ibssParameter->elementId = DOT11_PS_TIM_ELEMENT_ID_IBSS_PARAMETER_SET;
    ibssParameter->length  = (unsigned char)sizeof(unsigned short);
    ibssParameter->atimWindow = dot11->atimDuration;

    return(ibssParameter->length + 2); // + 2,for element id and length
}// end of MacDot11StationAddIBSSParameterSet


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitBeacon
//  PURPOSE:     Send the beacon.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationTransmitBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    Message* beaconMsg;
    DOT11_BeaconFrame* beacon;
    clocktype currentTime;
    int dataRateType = 0;
    int length = 0;
    unsigned char beaconData[DOT11_MAX_BEACON_SIZE]= {0, 0};
    // Check if we can transmit
    if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) ||
         (!MacDot11IsPhyIdle(node, dot11)) ||
         (MacDot11StationWaitForNAV(node, dot11)))
    {
        MacDot11Trace(node, dot11, NULL,
            "Beacon delayed - not idle or waiting for NAV.");

        // Either wait for timer to expire or for
        // StartSendTimers to reattempt transmission
        MacDot11StationCancelTimer(node, dot11);
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        return;
    }
    if (MacDot11IsIBSSStation(dot11)){
        length += MacDot11StationAddIBSSParameterSet(
            dot11, beaconData + length);
    }// end of if
    currentTime = node->getNodeTime();

    MacDot11StationCancelTimer(node, dot11);

    // Set the data rate
/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           (size_t)DOT11_SHORT_CTRL_FRAME_SIZE,
                           DOT11_BEACON, 
                           ANY_MAC802, 
                           NULL);
        PHY_SetTxVector(node, 
                        dot11->myMacData->phyNumber,
                        tempTxVector);
    }
    else
    {
    dataRateType = dot11->highestDataRateTypeForBC;
    PHY_SetTxDataRateType(node, dot11->myMacData->phyNumber,
        dataRateType);
    }
/*********HT END****************************************************/

    // Build beacon frame (message)
    beaconMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, beaconMsg,
        (int)sizeof(DOT11_BeaconFrame) + length, TRACE_DOT11);

    beacon = (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(beaconMsg);
    memset(beacon, 0, sizeof(DOT11_BeaconFrame));

    beacon->hdr.frameType = DOT11_BEACON;
    beacon->hdr.frameFlags = 0;
    beacon->hdr.duration = 0;
    beacon->hdr.destAddr = ANY_MAC802;
    beacon->hdr.sourceAddr = dot11->selfAddr;
    beacon->hdr.address3 = dot11->selfAddr;

    beacon->timeStamp = currentTime + dot11->delayUntilSignalAirborn;
    beacon->beaconInterval =
        MacDot11ClocktypeToTUs(dot11->beaconInterval);

    if (length > 0){
        memcpy((unsigned char* )beacon + sizeof(DOT11_BeaconFrame),
            beaconData, (size_t)length);
    }

    MacDot11StationSetState(node, dot11, DOT11_X_IBSSBEACON);
    MacDot11StationStartTransmittingPacket(node, dot11, beaconMsg,
        dot11->delayUntilSignalAirborn);
}// MacDot11StationTransmitBeacon

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationBeaconTransmittedOrReceived
//  PURPOSE:     Set variables and states after the transmit of beacon.
//               Set timer for next beacon.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationBeaconTransmittedOrReceived(
    Node* node,
    MacDataDot11* dot11,
    BOOL flag)
{

    dot11->beaconIsDue = FALSE;
    dot11->stationHasReceivedBeconInAdhocMode = TRUE;
    if (flag == BEACON_TRANSMITTED){
        dot11->beaconPacketsSent++;
        if (DEBUG_PS && DEBUG_PS_IBSS_BEACON)
            MacDot11Trace(node, dot11, NULL, "Transmitted IBSS Beacon");
    }
    else{
        dot11->beaconPacketsGot++;
        if (DEBUG_PS && DEBUG_PS_IBSS_BEACON)
            MacDot11Trace(node, dot11, NULL, "Received IBSS Beacon");
    }

    MacDot11StationCancelTimer(node, dot11);

    MacDot11StationResetBackoffAndAckVariables(node, dot11);

    // Set state to transmit ATIM frame,
    MacDot11StationCancelTimer(node, dot11);
    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    
    dot11->isIBSSATIMDurationActive = TRUE;
    
    MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
}// MacDot11StationBeaconTransmittedOrReceived

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsElementIdPresent
//  PURPOSE:     Search element id from the beacon.
//
//  PARAMETERS:  DOT11_BeaconFrame* beacon
//                  Received beacon message pointer
//               DOT11_PS_TIMFrame elementId
//  RETURN:      unsigned char pointer
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
unsigned char* MacDot11IsElementIdPresent(
    DOT11_BeaconFrame* beacon,
    DOT11_PS_ElementId elementId,
    int length)
{
    unsigned char* tempBeacon = (unsigned char* )beacon +
        sizeof(DOT11_BeaconFrame);
    unsigned char* retPointer = NULL;

    // here element id data is present in TLV(Tag, Length, Value) format
    while (tempBeacon < ((unsigned char* )beacon + length)){
        if (*tempBeacon == (unsigned char)elementId){
            retPointer = tempBeacon;
            break;
        }// end of if
       tempBeacon += (*(tempBeacon + 1)) + 2;
    }// end of while
    return retPointer;
}// end of MacDot11IsElementIdPresent


//--------------------------------------------------------------------------
//  NAME:        MacDot11ParsePSModeElementID
//  PURPOSE:     Parse PS mode element ID data.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               unsigned char* TIMElementPointer
//                  Received unsigned char pointer
//               Message* msg
//                  Received beacon message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ParsePSModeElementID(
    Node* node,
    MacDataDot11* dot11,
    unsigned char* TIMElementPointer,
    Message* msg)
{
    DOT11_PS_TIMFrame* timFrame = (DOT11_PS_TIMFrame* )TIMElementPointer;
    ERROR_Assert(TIMElementPointer != NULL,
        "MacDot11ParsePSModeElementID, Received frame is not TIM frame");

    // Store beacon info
    dot11->beaconDTIMCount = timFrame->dTIMCount;
    dot11->beaconDTIMPeriod = timFrame->dTIMPeriod;
    dot11->beaconExpirationTime = dot11->beaconTime;

    TIMElementPointer += sizeof(DOT11_PS_TIMFrame);

    dot11->apHasUnicastDataToSend = FALSE;
    dot11->apHasBroadcastDataToSend = FALSE;
    if (MacDot11IsCurrentTIMFrameIsDTIM(timFrame->dTIMCount)){
        if (DEBUG_PS && DEBUG_PS_AP_BEACON)
            MacDot11Trace(node, dot11, msg,"Station Received DTIM Frame Beacon");

        dot11->psModeDTIMFrameReceived++;
        if (MacDot11IsBroadcastPacketBufferedAtAP(timFrame->bitmapControl)){
            dot11->apHasBroadcastDataToSend = TRUE;
        }
    }
    else{
        if (DEBUG_PS && DEBUG_PS_AP_BEACON)
            MacDot11Trace(node, dot11, msg,"Station Received TIM Frame Beacon");

        dot11->psModeTIMFrameReceived++;
    }
    // If Unicast Packet bufferd at AP then set the variable APhasDataToSend.
    if (MacDot11IsUnicastPacketBufferedAtAP(
        TIMElementPointer,
        timFrame,
        dot11->assignAssociationId )){
        dot11->apHasUnicastDataToSend = TRUE;
    }

    MacDot11StationSleepIfNoData (node, dot11);
    return;
}// end of MacDot11ParsePSModeElementID
//---------------------------Power-Save-Mode-End-Updates-----------------//

static
void Dot11_ParseHTCapabilitiesElement(
    Node* node,
    DOT11_Ie* element,
    DOT11_HT_CapabilityElement *htCapabilityElement)
{
    int ieOffset = 0;
    int length = 0;
    DOT11_IeHdr ieHdr;
    Dot11_ReadFromElement(element, &ieOffset, &ieHdr,length);
    ERROR_Assert(
        ieHdr.id == DOT11N_HT_CAPABILITIES_ELEMENT_ID,
        "Dot11_ParseCfsetIdElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    ERROR_Assert(
        ieHdr.length == sizeof(DOT11_HT_CapabilityElement),
        "Dot11_ParseCfsetIdElement: "
        "CFParameterSet length is not equal.\n");
    memset(htCapabilityElement, 0, sizeof(DOT11_HT_CapabilityElement));
    memcpy(htCapabilityElement, element->data + ieOffset, ieHdr.length);
    ieOffset += ieHdr.length;

    ERROR_Assert(ieOffset == element->length,
        "Dot11_ParseCfsetIdElement: all fields not parsed.\n") ;
}// Dot11_ParseCfsetIdElement


BOOL Dot11_GetHTCapabilities(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_HT_CapabilityElement *htCapabilityElement,
    int sizeofPacket)
{
    BOOL isFound = FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    int offset = sizeofPacket;
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        return isFound;
    }

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;

    // The first expected IE is cfSet
    while (ieHdr.id != DOT11N_HT_CAPABILITIES_ELEMENT_ID)
    {
        offset += ieHdr.length + 2;
        if (offset >= rxFrameSize)
        {
            return isFound;
        }
        ieHdr = rxFrame + offset;
    }

    if (ieHdr.id == DOT11N_HT_CAPABILITIES_ELEMENT_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11_ParseHTCapabilitiesElement(node, &element, htCapabilityElement);
        isFound = TRUE;
    }
       return isFound;
}//Dot11_GetCfsetFromBeacon

BOOL Dot11_GetHTOperation(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_HT_OperationElement *operElem,
    int initOffset)
{
    BOOL isFound = FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    
    int offset = initOffset;
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize) {
        return isFound;
    }

    char* rxFrame = (char *) frameHdr;

    // The first expected IE is cfSet
    while (*(rxFrame+offset) != DOT11N_HT_OPERATION_ELEMENT_ID)
    {
        offset += *(rxFrame + offset + 1) + 2;
        if (offset >= rxFrameSize)
        {
            return isFound;
        }
    }

    assert (*(rxFrame+offset) == DOT11N_HT_OPERATION_ELEMENT_ID);
    
    operElem->FromPacket((const char*)(rxFrame + offset + 2), 
                            (size_t)(*(rxFrame + offset + 1)));
    isFound = TRUE;

    if (DEBUG_BEACON) {
        char debugStr[MAX_STRING_LENGTH];
        operElem->Print(debugStr, MAX_STRING_LENGTH);
        printf("Getting HT Operation Element: %s\n", debugStr); 
    }
    
    return isFound;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationProcessBeacon
//  PURPOSE:     A station receives a CFP beacon from within the BSS.
//               Set CFP end timers and enter PCF.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received beacon message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    int       offset;
    dot11->pcfEnable = FALSE;
    clocktype beaconTimeStamp = 0;
    clocktype newNAV = 0;
    clocktype currentTime;
    DOT11_BeaconFrame* beacon =
        (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(msg);
    DOT11_CFParameterSet cfSet  ;
    DOT11_TIMFrame timFrame = {0};
    clocktype beaconTransmitTime;
    dot11->beaconInterval =
             MacDot11TUsToClocktype((unsigned short)beacon->beaconInterval);
 // Record end time used for sufficient time computation.
   if (Dot11_GetCfsetFromBeacon(node, dot11, msg, &cfSet))
    {
      dot11->pcfEnable = TRUE;
      offset = ( sizeof(DOT11_BeaconFrame)+ sizeof(DOT11_CFParameterSet)+
      DOT11_IE_HDR_LENGTH);
      Dot11_GetTimFromBeacon(node, dot11, msg, offset, &timFrame);
      dot11->DTIMCount =  timFrame.dTIMCount;
      beaconTimeStamp = beacon->timeStamp;
      dot11->cfpMaxDuration = MacDot11TUsToClocktype(cfSet.cfpMaxDuration);
      dot11->DTIMCount = timFrame.dTIMCount;
      dot11->DTIMperiod = timFrame.dTIMPeriod;
      dot11->cfpRepetionInterval = ( (dot11->beaconInterval)
                    *(cfSet.cfpPeriod)*(timFrame.dTIMPeriod));


/*********HT START*************************************************/
        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector rxVector;
            PHY_GetRxVector(node,
                            dot11->myMacData->phyNumber,
                            rxVector);
            beaconTransmitTime = node->getNodeTime() -
                                            PHY_GetTransmissionDuration(
                                                node,
                                                dot11->myMacData->phyNumber,
                                                rxVector);
        }
        else
        {
       int dataRateType = PHY_GetRxDataRateType(node,
                                           dot11->myMacData->phyNumber);

       beaconTransmitTime = node->getNodeTime() -
                                        PHY_GetTransmissionDuration(
                                        node, dot11->myMacData->phyNumber,
                                        dataRateType,
                                        MESSAGE_ReturnPacketSize(msg));
        }
/*********HT END****************************************************/

       newNAV = beaconTransmitTime;

      if ((unsigned short)cfSet.cfpBalDuration > 0)
      {
        newNAV +=
            MacDot11TUsToClocktype((unsigned short)cfSet.cfpBalDuration)
            + EPSILON_DELAY;
      }
    }

   // If it is CFP Start Beacon
   if (dot11->pcfEnable)
   {
        if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0)
        {
              dot11->beaconPacketsGotCfp++;
              dot11->lastCfpBeaconTime = beacon->timeStamp;
             if (newNAV > dot11->NAV) {
                dot11->NAV = newNAV;
             }
            else if (newNAV < dot11->NAV)
            {
            // NAV is set for more than the CFP balance duration.
            // This is unlikely, unless a previous beacon from
            // another BSS had a longer CF period.
            MacDot11Trace(node, dot11, NULL, "Beacon NAV < set NAV.");
             }
            // Enter CFP if station is Pollable
            if (dot11->pollType == DOT11_STA_POLLABLE_POLL)
            {
             dot11->cfpEndTime = beaconTransmitTime +
                      MacDot11TUsToClocktype
                             ((unsigned short)cfSet.cfpBalDuration);
             MacDot11CfpResetBackoffAndAckVariables(node, dot11);
             MacDot11StationSetState(node, dot11, DOT11_CFP_START);
             MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
             currentTime = node->getNodeTime();
             // Start timer for CFP end, in case CF End frame is missed.
             MacDot11StationStartTimerOfGivenType(node, dot11,
                                                dot11->cfpEndTime - currentTime,
                                                MSG_MAC_DOT11_CfpEnd);
              dot11->cfpEndSequenceNumber = dot11->timerSequenceNumber;
            }
        }
        // If not CFP Start Beacon
        else
        {
            if (dot11->state == DOT11_CFP_START)
             {
                MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
             }
        /// Protection if CfStart Beacon was Missed by this Station
            else if (cfSet.cfpBalDuration != 0 &&
                dot11->pollType ==DOT11_STA_POLLABLE_POLL )
             {
               // Set NAV for CFP duration.
               if (newNAV > dot11->NAV) {
                   dot11->NAV = newNAV;
                }
               else if (newNAV < dot11->NAV)
               {
                // NAV is set for more than the CFP balance duration.
                // This is unlikely, unless a previous beacon from
                // another BSS had a longer CF period.
                MacDot11Trace(node, dot11, NULL, "Beacon NAV < set NAV.");
               }
               // Enter CFP
               MacDot11CfpResetBackoffAndAckVariables(node, dot11);
               MacDot11StationSetState(node, dot11, DOT11_CFP_START);
               MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
               currentTime = node->getNodeTime();
               dot11->cfpEndTime = beaconTransmitTime+
                      MacDot11TUsToClocktype
                             ((unsigned short)cfSet.cfpBalDuration);
               // Start timer for CFP end, in case CF End frame is missed.
               MacDot11StationStartTimerOfGivenType(node, dot11,
                                             dot11->cfpEndTime - currentTime,
                                             MSG_MAC_DOT11_CfpEnd);
               dot11->cfpEndSequenceNumber = dot11->timerSequenceNumber;
            }
            else
            {
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            }
        }

    }// end pcfEnable
     else{
         MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
     }
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationProcessBeacon
//  PURPOSE:     A station receives a beacon from within the BSS.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received beacon message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11StationProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{

    DOT11_BeaconFrame* beacon =
        (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(msg);
    clocktype beaconTimeStamp = beacon->timeStamp;
    dot11->lastBeaconRecieve = beaconTimeStamp;
    dot11->beaconTime = beaconTimeStamp;
    dot11->beaconsMissed = 0;

    dot11->beaconPacketsGot++;
    // Reset backoff variable before starting CFP only
    // Should not be done for DCF beacons
    MacDot11CfpStationProcessBeacon(node, dot11, msg);
    MacDot11StationCancelBeaconTimer(node, dot11);
    MacDot11StationCancelTimer(node, dot11);

    if (MacDot11IsAssociationDynamic(dot11)) {
        MacDot11ManagementProcessFrame(node, dot11, msg);
        if (dot11->state != DOT11_CFP_START)
        {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        }
    }

//---------------------------Power-Save-Mode-Updates---------------------//

    if (dot11->isPSModeEnabled == TRUE){
        unsigned char* TIMElementPointer = NULL;
        int length = MESSAGE_ReturnPacketSize(msg);

        TIMElementPointer = MacDot11IsElementIdPresent(
            beacon,
            DOT11_PS_TIM_ELEMENT_ID_TIM,
            length);
        if (TIMElementPointer != NULL){
            MacDot11ParsePSModeElementID(
                node,
                dot11,
                TIMElementPointer,
                msg);
        }
        else{
            if (MacDot11IsStationSupportPSMode(dot11)){
                char errString[MAX_STRING_LENGTH];

                sprintf(errString,
                    "Node Id: %d: Joined AP does not support PS mode.\n",
                    node->nodeId);

                ERROR_ReportWarning(errString);
                dot11->joinedAPSupportPSMode = FALSE;
            }
        }
    }// end of  if dot11->isPSModeEnabled
//---------------------------Power-Save-Mode-End-Updates-----------------//

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        DOT11_HT_CapabilityElement htCapabilityElement;
        BOOL isPresent = Dot11_GetHTCapabilities(
            node,
            dot11,
            msg,
            &htCapabilityElement,
             sizeof(DOT11_BeaconFrame));
        if (isPresent)
        {
            // TBD:
            // After association process, what to do with the HT 
            // capability element in the beacon

            dot11->mcsIndexToUse = 
                        (UInt8)htCapabilityElement.
                                 supportedMCSSet.mcsSet.maxMcsIdx;
            MacDot11nUpdNeighborHtInfo(node, dot11, 
                      beacon->hdr.sourceAddr, htCapabilityElement);
        }
    }
/*********HT END****************************************************/

    // Set new beacon interval
    dot11->beaconInterval =
        MacDot11TUsToClocktype((unsigned short )beacon->beaconInterval);

    // Start timer for next expected beacon
    MacDot11StationStartBeaconTimer(node, dot11);

    if (DEBUG_BEACON) {
        char timeStr[100];

        ctoa(node->getNodeTime(), timeStr);

        printf("%d RECEIVED beacon frame at %s\n", node->nodeId,
            timeStr);
    }
}// MacDot11StationProcessBeacon


//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
/*!
 * \brief  Print PS MODE statistics.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param interfaceIndex  int       :Interface index

 * \return          NONE            : TRUE if states permit or transmit occurs
 */
//--------------------------------------------------------------------------
void MacDot11PSModePrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    if (MacDot11IsIBSSStationSupportPSMode(dot11)){

        sprintf(buf, "Management ATIM Frames Sent = %d",
            dot11->atimFramesSent);

        IO_PrintStat(
            node,
            "MAC",
            DOT11_MANAGEMENT_STATS_LABEL,
            ANY_DEST,
            interfaceIndex,
            buf);

        sprintf(buf, "Management ATIM Frames Received = %d",
            dot11->atimFramesReceived);

        IO_PrintStat(
            node,
            "MAC",
            DOT11_MANAGEMENT_STATS_LABEL,
            ANY_DEST,
            interfaceIndex,
            buf);

        sprintf(buf, "PS Mode Broadcast Data Packets Sent = %d",
            dot11->broadcastDataPacketSentToPSModeSTAs);

        IO_PrintStat(
            node,
            "MAC",
            DOT11_MAC_STATS_LABEL,
            ANY_DEST,
            interfaceIndex,
            buf);

        sprintf(buf, "PS Mode Unicast Data Packets Sent = %d",
            dot11->unicastDataPacketSentToPSModeSTAs);

        IO_PrintStat(
            node,
            "MAC",
            DOT11_MAC_STATS_LABEL,
            ANY_DEST,
            interfaceIndex,
            buf);

            sprintf(buf, "MAC Layer Queue Drop Packet = %d",
                dot11->psModeMacQueueDropPacket);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
    }// end of if isPSModeEnabledInAdHocMode
    else{
        if (dot11->isPSModeEnabled == TRUE){

            sprintf(buf, "PS Poll Requests Sent = %d",
                dot11->psPollPacketsSent);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode DTIM Frames Received = %d",
                dot11->psModeDTIMFrameReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode TIM Frames Received = %d",
                dot11->psModeTIMFrameReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

        }// end of if isPSModeEnabled

        if (MacDot11IsAPSupportPSMode(dot11)){

            sprintf(buf, "MAC Layer Queue Drop Packet = %d",
                dot11->psModeMacQueueDropPacket);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode DTIM Frames Sent = %d",
                dot11->psModeDTIMFrameSent);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode TIM Frames Sent = %d",
                dot11->psModeTIMFrameSent);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Poll Requests Received = %d",
                dot11->psPollRequestsReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode Broadcast Data Packets Sent = %d",
                dot11->broadcastDataPacketSentToPSModeSTAs);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

            sprintf(buf, "PS Mode Unicast Data Packets Sent = %d",
                dot11->unicastDataPacketSentToPSModeSTAs);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MAC_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }// end of if
    }// end of if - else
}// end of MacDot11PSModePrintStats
//---------------------------Power-Save-Mode-End-Updates-----------------//


void MacDot11StationCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11,
    BOOL backoff)
{
    
    if (dot11->isHTEnable)
    {
        MacDot11nCheckForOutgoingPacket(node, dot11, backoff);
        return;
    }
    
    if (backoff)
    {
      MacDot11StationSetBackoffIfZero(
                node, dot11,dot11->currentACIndex);
    }
    if (dot11->ManagementResetPending == TRUE)
    {
        dot11->ManagementResetPending = FALSE;
        MacDot11ManagementReset(node, dot11);
    }

    //put right condition
    if (dot11->stationAssociated &&
        (!MacDot11IsAp(dot11))&&
        (dot11->associatedAP->rssMean < dot11->thresholdSignalStrength)
        && dot11->state == DOT11_S_IDLE &&
        dot11->currentMessage == NULL
        && dot11->ScanStarted == FALSE)
    {
        MacDot11ManagementStartJoin(
                                    node,
                                    dot11);
    }
//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsIBSSStationSupportPSMode(dot11)){
        Message* msg;

        if (!MacDot11IfNeedEndATIMDuration(node, dot11)){
            return;
        }

        if (MacDot11IsATIMDurationActive(dot11)){
            // Check if node has buffered broadcast or unicast packet
            if (MacDot11StationHasATIMFrameToSend(node, dot11)){
                // Have something to send. Set Backoff if not already set and
                // start sending process
                if (DEBUG_PS && DEBUG_PS_ATIM){
                    MacDot11Trace(node, dot11, dot11->currentMessage,
                        "Send ATIM Frame");
                }
                MacDot11StationSetBackoffIfZero(node, dot11);
                MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(
                    node,
                    dot11);
            }
            return;
        }

        if (!MacDot11StationHasNetworkMsgToSend(dot11)){
            MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
                node,
                dot11);

            if (MacDot11DequeueBufferPacket(node, dot11, &msg)){
                dot11->currentMessage = msg;
                dot11->pktsToSend++;
                // For a BSS station, change next hop address to be that of the AP
                dot11->ipNextHopAddr = dot11->currentNextHopAddress;
                dot11->currentACIndex = DOT11e_INVALID_AC;
            }

        }// end of
    }// end of if MacDot11IsIBSSStationSupportPSMode

    if (MacDot11IsAPSupportPSMode(dot11)){
        if (dot11->currentMessage == NULL){
            MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
                node,
                dot11);
        }
        //if (dot11->dot11TxFrameInfo!= NULL &&
        //    dot11->dot11TxFrameInfo->frameType >= DOT11_DEAUTHENTICATION)
        if (dot11->dot11TxFrameInfo == NULL)
        {
            if (dot11->apVars->isBroadcastPacketTxContinue == TRUE){
                BOOL isMsgRetrieved;
                Message *msg;
                // dequeue the broadcast packet
                isMsgRetrieved = dot11->broadcastQueue->retrieve(
                    &msg,
                    0,
                    DEQUEUE_PACKET,
                    node->getNodeTime());

                if (isMsgRetrieved){
                    dot11->noOfBufferedBroadcastPacket--;
                    dot11->broadcastDataPacketSentToPSModeSTAs++;
                    // Set more data bit here if required, for broadcast packet.
                    if (dot11->noOfBufferedBroadcastPacket == 0){
                        dot11->isMoreDataPresent = FALSE;
                        dot11->apVars->isBroadcastPacketTxContinue = FALSE;
                    }else{
                        dot11->isMoreDataPresent = TRUE;
                    }
                        dot11->currentNextHopAddress = ANY_MAC802;

                    dot11->currentMessage = msg;
                    dot11->pktsToSend++;
                    dot11->currentACIndex = DOT11e_INVALID_AC;
                // For a BSS station, change next hop address to be that of the AP
                    dot11->ipNextHopAddr = dot11->currentNextHopAddress;
                }
            }
        }
    }// end of  if isAPSupportPSMode
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // dot11s
    MacDot11StaMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
            node, dot11);
        // dot11s. Check outgoing from Mgmt queue.
    if (dot11->currentMessage == NULL && dot11->ACs[0].frameToSend == NULL) {
        MacDot11sStationMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
            node, dot11);
    }

    // Ignore if station is not joined
    if (MacDot11IsStationJoined(dot11) &&
        dot11->currentMessage == NULL &&
        dot11->ScanStarted == FALSE)
    {
        // dot11s. Check outgoing from Data queue.
        // Dequeue from local data queue first, then network layer.
        MacDot11StationMoveAPacketFromTheDataQueueToTheLocalBuffer(
            node, dot11);

        if (dot11->currentMessage == NULL) {
            dot11->currentACIndex = DOT11e_INVALID_AC;
            MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
                node, dot11);
        }
    }
    else if (dot11->currentMessage == NULL
        && MacDot11IsACsHasMessage(node,dot11))
    {

       if (dot11->state == DOT11_S_IDLE)
       {
           if (MacDot11IsQoSEnabled(node, dot11)) {
               MacDot11eSetCurrentMessage(node,dot11);
           }
           else
           {
               MacDot11SetCurrentMessage(node,dot11);
           }

        }
    }

    if ((dot11->beaconIsDue) &&
         (MacDot11StationCanHandleDueBeacon(node, dot11))) {
        return;
    }
//--------------------HCCA-Updates Start---------------------------------//
    if (MacDot11HcAttempToStartCap(node,dot11)){
        return;
    }
//--------------------HCCA-Updates End-----------------------------------//

//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsBssStation(dot11)&& MacDot11IsStationJoined(dot11)
        && MacDot11IsStationSupportPSMode(dot11)&&
        dot11->ScanStarted == FALSE){
            if ((dot11->apHasBroadcastDataToSend == FALSE)
                && (dot11->currentMessage == NULL)
                && dot11->apHasUnicastDataToSend == TRUE){
                    // send PS Poll frame to AP
                    MacDot11StationTransmitPSPollFrame(node,
                                                       dot11,
                                                       dot11->bssAddr);
                }
            else{
                    MacDot11StationSleepIfNoData(node, dot11);
            }

        if (dot11->state == DOT11_X_PSPOLL)
            return;
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//
    if (dot11->currentMessage == NULL)
    {
        return;
    }

    // If the Queue is NonEmpty, or we are currently working on transmitting
    // a message (currentMessage)
//---------------------------Power-Save-Mode-Updates---------------------//
    if ((dot11->phyStatus == MAC_DOT11_PS_ACTIVE) && (backoff ||
        dot11->BO != 0 ||
        MacDot11StationHasFrameToSend(dot11))) {
//---------------------------Power-Save-Mode-End-Updates-----------------//
        // Have something to send. Set Backoff if not already set and
        // start sending process
                MacDot11StationSetBackoffIfZero(
                    node, dot11,dot11->currentACIndex);

        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(node, dot11);
    }
    else {
        if (dot11->phyStatus == MAC_DOT11_PS_ACTIVE){
        ERROR_Assert(dot11->BO == 0,
            "MacDot11CheckForOutgoingPacket: "
            "Backoff should be zero to enter IDLE state.\n");
        }
        if (dot11->state != DOT11_S_WFACK)
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationCancelTimer(node, dot11);
    }

}// MacDot11StationCheckForOutgoingPacket












