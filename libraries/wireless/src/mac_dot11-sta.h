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
 * \file mac_dot11-sta.h
 * \brief Station structures and helper functions.
 */

#ifndef MAC_DOT11_STA_H
#define MAC_DOT11_STA_H

#include "api.h"
#include "dvcs.h"
#include "mac_dot11.h"
#include "mac_dot11-ap.h"
#include "mac_dot11-hcca.h"
#include "mac_dot11s-frames.h"
#include "mac_dot11s.h"
#include "mac_dot11-pc.h"
#include "mac_dot11n.h"
#include "mac_phy_802_11n.h"


//#ifdef CYBER_LIB
//#undef CYBER_LIB
//#endif // CYBER_LIB

#ifdef CYBER_LIB
#include "mac_security.h"
#endif // CYBER_LIB

#include "network_ip.h"

#ifdef ENTERPRISE_LIB
#include "mpls_shim.h"
#endif // ENTERPRISE_LIB

#ifdef PAS_INTERFACE
#include "partition.h"
#include "packet_analyzer.h"
#endif

//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------
//---------------------------Power-Save-Mode-Updates---------------------//

// if DTIM count is 0 then current TIM frame is DTIM
#define MacDot11IsCurrentTIMFrameIsDTIM(dTIMCount) (dTIMCount == 0)

#define MacDot11IsBroadcastPacketBufferedAtAP(bitmapControl)\
    (bitmapControl & 0x01)


#define BEACON_TRANSMITTED  1
#define BEACON_RECEIVED     0

#define MAC_DOT11_PS_ACTIVE 1
#define MAC_DOT11_PS_SLEEP  0

void MacDot11StationCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11,
    BOOL backoff);

BOOL MacDot11StationAttemptToTransmitBeacon(
    Node* node,
    MacDataDot11* dot11);
void MacDot11StationTransmitBeacon(
    Node* node,
    MacDataDot11* dot11);
void MacDot11StationBeaconTransmittedOrReceived(
    Node* node,
    MacDataDot11* dot11,
    BOOL flag);

BOOL MacDot11StationHasATIMFrameToSend(
    Node* node,
    MacDataDot11* dot11);

void MacDot11IBSSStationStartBeaconTimer(
    Node* node,
    MacDataDot11* dot11);

BOOL MacDot11StationHasExchangeATIMFrame(
    MacDataDot11* dot11);

void MacDot11StationUnicastTransmitted(
    Node* node,
    MacDataDot11* dot11);

void MacDot11SetExchangeVariabe(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

void MacDot11StationStopListening(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationSleepIfNoData(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationStartListening(
    Node* node,
    MacDataDot11* dot11);

void MacDot11PSModePrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex);

static //inline//
void MacDot11StationCancelTimer(
    Node* node,
    MacDataDot11* dot11);

static //inline
void MacDot11StationResetCurrentMessageVariables(
    Node* node, MacDataDot11* const dot11);

BOOL MacDot11PSModeDropPacket(
    Node* node,
    MacDataDot11* dot11,
    Queue* queue,
    Message* msg,
    int* noOfPacketDrop);

void MacDot11ManagementReset(
    Node* node,
    MacDataDot11* dot11);

static //inline
void MacDot11StationSetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacStates state);

BOOL MacDot11StationWillExchangeMoreFrame(
    MacDataDot11* dot11);
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

void MacDot11eStationRetryForInternalCollision(
    Node* node,
    MacDataDot11* dot11,
    int acIndex = DOT11e_INVALID_AC);

void MacDot11eStationIncreaseCWForAC(
    Node* node,
    MacDataDot11* dot11,
    int acIndex = DOT11e_INVALID_AC);

BOOL MacDot11eACHasManagementFrameToSend(
    Node* node,
    MacDataDot11* dot11,
    int acIndex = DOT11e_INVALID_AC);

void MacDot11eStationResetCWForAC(
    Node* node,
    MacDataDot11* dot11,
    int acIndex = DOT11e_INVALID_AC);

void MacDot11StationRetransmit(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationTransmitFrame(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationTransmissionHasFinished(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationStartBeaconTimer(
    Node* node,
    MacDataDot11* dot11);

BOOL MacDot11StationCanHandleDueBeacon(
    Node* node,
    MacDataDot11* dot11);

void MacDot11StationProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

//--------------------------------------------------------------------------
// INLINE FUNCTIONS
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  Helper Functions
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
/*!
 * \brief  Start a join.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          None
 */
//--------------------------------------------------------------------------
void MacDot11ManagementStartJoin(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsPC
//  PURPOSE:     Determine if station is a Point Coordinator.
//  PARAMETERS:  const MacDataDot11* const dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE for Access Points and Point Coordinators
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsPC(
    const MacDataDot11 * const dot11)
{
    return (dot11->stationType == DOT11_STA_PC);
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11IsFrameFromMyAP
//  PURPOSE:     Determine if frame is from the AP or PC of the BSS
//               by matching source address.
//               DOT11_FROM_DS is not matched.
//
//  PARAMETERS:  const MacDataDot11* const dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address
//  RETURN:      TRUE if frame is from AP or PC
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsFrameFromMyAP(
    const MacDataDot11* const dot11,
    Mac802Address sourceAddr)
{
    return (sourceAddr == dot11->bssAddr);
}// MacDot11IsFrameFromMyAP


//--------------------------------------------------------------------------
// NAME         MacDot11IsWaitingForResponseState
// PURPOSE      Check if state is one that is waiting for response.
// PARAMETERS   DOT11_MacStates state
//                  The state to be validated.
// RETURN       BOOL
// NOTES        None
//-------------------------------------------------------------------------
static //inline
BOOL MacDot11IsWaitingForResponseState(DOT11_MacStates state)
{
   return( (DOT11_S_WFCTS <= state) &&
           (state <= DOT11_S_WFACK) );
}// MacDot11IsWaitingForResponseState


//--------------------------------------------------------------------------
// NAME         MacDot11IsTransmittingState
// PURPOSE      Check if state is one of transmit states
// PARAMETERS   DOT11_MacStates state
//                  The state to be validated.
// RETURN       BOOL
// NOTES        None
//-------------------------------------------------------------------------
static //inline
BOOL MacDot11IsTransmittingState(DOT11_MacStates state)
{
   return((state >= DOT11_X_RTS) &&
//---------------------------Power-Save-Mode-Updates---------------------//
            (state<=  DOT11_X_PSPOLL));
//---------------------------Power-Save-Mode-End-Updates-----------------//
           //(state <= DOT11_X_ACK) );
}// MacDot11IsTransmittingState

//------------------------------------------------------------
// NAME     ::    MacDot11IsCfpTransmittingState
// PURPOSE  ::    Check if state is one of transmit states
// PARAMETERS ::  DOT11_CfpStates cfpState
//                  The state to be validated.
// RETURN     ::  True for transmitting CFP State
// NOTES      ::  None
//----------------------------------------------------------------------
static //inline
BOOL MacDot11IsCfpTransmittingState(DOT11_CfpStates cfpState)
{
   return((cfpState >= DOT11_X_CFP_BROADCAST) &&
             (cfpState<=  DOT11_X_CFP_END));


}// MacDot11IsCfpTransmittingState

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsBssStation
//  PURPOSE:     Determine if station is a BSS station.
//               Other types of stations are AP, PC or IBSS station.
//               The station type is a user input.
//
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE for stations within a BSS
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsBssStation(
    const MacDataDot11* const dot11)
{
    return (dot11->stationType == DOT11_STA_BSS);
}// MacDot11IsBssStation

//--------------------HCCA-Updates Start---------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsAp
//  PURPOSE:     Determine if station is either an Access Point.
//
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE for Access Points and Point Coordinators
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsAp(
    const MacDataDot11* const dot11)
{
    return (dot11->stationType == DOT11_STA_AP ||
        dot11->stationType == DOT11_STA_HC ||
         dot11->stationType == DOT11_STA_PC);
}// MacDot11IsAp

//-------------------------------------------------------------------------
//  NAME:        MacDot11IsHC
//  PURPOSE:     Determine if station is a HC. Required for HCCA
//  PARAMETERS:  const MacDataDot11* const dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE for Access Points and Point Coordinators
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsHC(
    const MacDataDot11 * const dot11)
{
    return (dot11->stationType == DOT11_STA_HC);
} //MacDot11IsHC

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsInCap
//  PURPOSE:     Determine if within CAP.
//               Used to handle events accordingly.
//               Required for HCCA
//  PARAMETERS:  DOT11_MacStates state
//                  Current MAC state
//  RETURN:      TRUE if within contention free period
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11IsInCap(
    DOT11_MacStates state)
{
    return (state == DOT11e_CAP_START);
} // MacDot11IsInCap

//--------------------HCCA-Updates End-----------------------------------//

//---------------------------Power-Save-Mode-Updates---------------------//
static // inline/
BOOL MacDot11IsStationSupportPSMode(const MacDataDot11* const dot11)
{
    return((dot11->joinedAPSupportPSMode == TRUE)
        && (dot11->isPSModeEnabled == TRUE));
}// end of MacDot11IsStationSupportPSMode

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsIBSSStation
//  PURPOSE:     Determine if station is either an IBSS station.
//
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE for Access Points and Point Coordinators
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsIBSSStation(const MacDataDot11* const dot11)
{
    return (dot11->stationType == DOT11_STA_IBSS);
}// MacDot11IsIBSSStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsIBSSStationSupportPSMode
//  PURPOSE:     Determine station is IBSS station and PS mode enabled on it
//
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsIBSSStationSupportPSMode(const MacDataDot11* const dot11)
{
    return ((dot11->isPSModeEnabledInAdHocMode == TRUE)
        && MacDot11IsIBSSStation(dot11));
}// MacDot11IsIBSSStationSupportPSMode

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsATIMDurationActive
//  PURPOSE:     Determine station is IBSS station and PS mode enabled on it.
//               and Atim duration is active
//
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsATIMDurationActive(const MacDataDot11* const dot11)
{
    return ((dot11->isIBSSATIMDurationActive == TRUE)
        && MacDot11IsIBSSStationSupportPSMode(dot11));
}// MacDot11IsATIMDurationActive

BOOL Dot11_GetHTCapabilities(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_HT_CapabilityElement *htCapabilityElement,
    int sizeofPacket);

BOOL Dot11_GetHTOperation(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_HT_OperationElement *operElem,
    int initOffset);

//--------------------------------------------------------------------------
//  NAME:        MacDot11IfNeedEndATIMDuration
//  PURPOSE:     End atim duration active period.
//
//  PARAMETERS:   Node* node
//                  Pointer to node that sent the data
//               MacDataDot11 *  dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IfNeedEndATIMDuration( Node* node, MacDataDot11* dot11)
{
    if ((MacDot11IsTransmittingState(dot11->state)
        || MacDot11IsWaitingForResponseState(dot11->state))){
            return FALSE;
    }
    if (dot11->isIBSSATIMDurationNeedToEnd == TRUE){
        MacDot11StationCancelTimer(node, dot11);
        dot11->isIBSSATIMDurationNeedToEnd = FALSE;
        dot11->isIBSSATIMDurationActive = FALSE;

        if (dot11->currentMessage != NULL)
        {
                dot11->currentMessage = NULL;
        }
        if (dot11->dot11TxFrameInfo!= NULL &&
        dot11->dot11TxFrameInfo->frameType == DOT11_ATIM)
        {
            MESSAGE_Free(node,dot11->dot11TxFrameInfo->msg);
            MEM_free(dot11->dot11TxFrameInfo);
            dot11->dot11TxFrameInfo = NULL;
        }
        MacDot11StationResetCurrentMessageVariables(node, dot11);
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        if (!MacDot11StationWillExchangeMoreFrame(dot11)){
            // Go to sleep mode
            MacDot11StationStopListening(node, dot11);
        }
    }
    return TRUE;
}// MacDot11IfNeedEndATIMDuration

//--------------------------------------------------------------------------
//  NAME:        MacDot11IBSSStationDequeuePacketFromLocalQueue.
//  PURPOSE:     Dequeue packet from local queue.
//  PARAMETERS:  Node* node
//                  Pointer to node that sent the data
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline //
BOOL MacDot11IBSSStationDequeuePacketFromLocalQueue(
    Node* node,
    MacDataDot11* dot11,
    BOOL flag)
{
    if (MacDot11IsIBSSStationSupportPSMode(dot11)
        && (dot11->tempQueue != NULL)){
        Message* msg = NULL;
        BOOL isMsgRetrieved = FALSE;

        if ((flag == DOT11_X_UNICAST) && (dot11->tempAttachNodeList == NULL)){
            dot11->tempQueue = NULL;
            return isMsgRetrieved;
        }
        isMsgRetrieved = dot11->tempQueue->retrieve(
            &msg,
            0,
            DEQUEUE_PACKET,
            node->getNodeTime());
        if (isMsgRetrieved){
            dot11->currentMessage = msg;
            if (flag == DOT11_X_BROADCAST){
                dot11->noOfBufferedBroadcastPacket--;
                dot11->broadcastDataPacketSentToPSModeSTAs++;
                MESSAGE_Free(node, dot11->currentMessage);
            }
            if (flag == DOT11_X_UNICAST){
                dot11->tempAttachNodeList->noOfBufferedUnicastPacket--;
                dot11->unicastDataPacketSentToPSModeSTAs++;
                dot11->tempAttachNodeList = NULL;
            }
        }
        dot11->tempQueue = NULL;
        return isMsgRetrieved;
    }
    return TRUE;
}// end of MacDot11IBSSStationDequeuePacketFromLocalQueue

//---------------------------Power-Save-Mode-End-Updates-----------------//


//--------------------------------------------------------------------------
//  NAME:       MacDot11IsAssociationDynamic
//  PURPOSE:    Check if station is using dynamic association
//  PARAMETERS: const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:     BOOL
//  ASSUMPTION: None
//-------------------------------------------------------------------------
static //inline
BOOL MacDot11IsAssociationDynamic(
    const MacDataDot11* const dot11){

    return (dot11->associationType == DOT11_ASSOCIATION_DYNAMIC);
}// MacDot11IsAssociationDynamic


//--------------------------------------------------------------------------
//  NAME:       MacDot11IsStationJoined
//  PURPOSE:    Check if station is AP or joined (associated) to an AP
//  PARAMETERS: const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:     BOOL
//  ASSUMPTION: None
//-------------------------------------------------------------------------
static //inline
BOOL MacDot11IsStationJoined(
    const MacDataDot11* const dot11)
{
    if (MacDot11IsAp(dot11) ){
        // dot11s
        // For MPs, AP services start once initialization is complete.
        if (dot11->isMP) {
            return dot11->mp->state == DOT11s_S_INIT_COMPLETE;
        }

        // AP can always send
        return TRUE;

    } else if (MacDot11IsAssociationDynamic(dot11)){
        return ( (dot11->stationAuthenticated) &&
                 (dot11->stationAssociated) );
    } else
        return TRUE;
}// MacDot11IsStationJoined

//--------------------------------------------------------------------------
//  NAME:       MacDot11IsStationNonAPandJoined
//  PURPOSE:    Check if station is joined (associated) to an AP
//  PARAMETERS: const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:     BOOL
//  ASSUMPTION: None
//-------------------------------------------------------------------------
static //inline
BOOL MacDot11IsStationNonAPandJoined(
    const MacDataDot11* const dot11)
{
    if (!MacDot11IsAp(dot11) &&
        dot11->associationType == DOT11_ASSOCIATION_DYNAMIC){
        return ( (dot11->stationAuthenticated) &&
                 (dot11->stationAssociated) );
    }
    //Not an BSS station
    return FALSE;
}// MacDot11IsStationNonAPandJoined

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHasNetworkMsgToSend.
//  PURPOSE:     Station has a network message to send.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationHasNetworkMsgToSend(
    MacDataDot11* dot11)
{

    if (dot11->currentMessage == NULL)
        return FALSE;

    else
        return TRUE;

}// MacDot11StationHasNetworkMsgToSend


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHasFrameToSend
//  PURPOSE:     Station has a frame to send.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationHasFrameToSend(
    MacDataDot11* dot11)
{
//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsATIMDurationActive(dot11)){
        if (dot11->currentMessage != NULL)
            return TRUE;
        else
            return FALSE;
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//

    if (dot11->currentMessage == NULL && dot11->instantMessage == NULL)
        return FALSE;

    else
        return TRUE;

}// MacDot11StationHasFrameToSend
//------------------------------------------------------------
//  NAME:        MacDot11StationHasInstantFrameToSend
//  PURPOSE:     To check if Station has a Instant Frame to send
//               frame to send.
//  PARAMETERS:  MacDataDot11* dot11
//               Pointer to Dot11 structure
//  RETURN:      TRUE if Satation has any Instant frame to transmit
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationHasInstantFrameToSend(
    MacDataDot11* dot11)
{
    if (dot11->instantMessage == NULL)
        return FALSE;

    else
        return TRUE;
}// MacDot11StationHasInstantFrameToSend

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHasManagementFrameToSend
//  PURPOSE:     Station has a management frame to send.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationHasManagementFrameToSend(Node* node,
    MacDataDot11* dot11)
{
    if (dot11->dot11TxFrameInfo!= NULL)
    {
        if (((dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ
            && dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION) ||
            dot11->dot11TxFrameInfo->frameType == DOT11_PS_POLL)&&
            (dot11->currentMessage == dot11->dot11TxFrameInfo->msg))
            return TRUE;
        else
            return FALSE;
    }

    else
        return FALSE;

}// MacDot11StationHasManagementFrameToSend

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetCurrentMessage
//  PURPOSE:     Get the current message from network.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Message *
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
Message* MacDot11StationGetCurrentMessage(
    MacDataDot11* dot11)
{
    return (dot11->currentMessage);

}// MacDot11StationGetCurrentMessage

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetCurrentMessageSize
//  PURPOSE:     Get the frame size of the current message.
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Message *
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
int MacDot11StationGetCurrentMessageSize(
    MacDataDot11* dot11)
{
    return (DOT11nMessage_ReturnPacketSize(MacDot11StationGetCurrentMessage(dot11)));
}// MacDot11StationGetCurrentMessage

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsPhyIdle
//  PURPOSE:     Check if physical is idle.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
BOOL MacDot11IsPhyIdle(
    Node* node,
    MacDataDot11* dot11)
{
    return ( PHY_MediumIsIdle(node, dot11->myMacData->phyNumber));
}// MacDot11IsPhyIdle

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsFrameFromStationInMyBSS
//  PURPOSE:     Determine if frame is from station within the
//               same BSS.
//  PARAMETERS:  const MacDataDot11* const dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address
//               unsigned char frameFlags
//                  Frame flags
//  RETURN:      TRUE if frame is destined for AP/PC of the BSS
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsFrameFromStationInMyBSS(
    const MacDataDot11* const dot11,
    Mac802Address destAddr,
    unsigned char frameFlags)
{
    return ( (destAddr == dot11->bssAddr) &&
             (frameFlags & DOT11_TO_DS) );
}// MacDot11IsFrameFromStationInMyBSS

// /**
// FUNCTION   :: MacDot11StationUpdateMeanMeasurement
// LAYER      :: MAC
// PURPOSE    :: Update the measurement history and recalculate the
//               mean value
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot11*: Pointer to DOT16 structure
// + measInfo  : DOT11_SignalMeasurementInfo* : Pointer to the meas. hist.
// + signalMea : PhySignalMeasurement* : Pointer to the current measurement
// + meanMeas  : MacDot16SignalMeasurementInfo* : Pinter to the mean of meas
// RETURN     :: void : NULL
static
void MacDot11StationUpdateMeanMeasurement(Node* node,
           const MacDataDot11* const dot11,
           DOT11_SignalMeasurementInfo* measInfo,
           PhySignalMeasurement* signalMea,
           DOT11_SignalMeasurementInfo* meanMeas)
{

    int i;
    int oldestMeas = 0;
    clocktype oldestTimeStamp;
    int numActiveMeas = 0;

    // set those measurement out age to be inactive
    for (i = 0; i < DOT11_MAX_NUM_MEASUREMNT; i ++)
    {
        if (measInfo[i].measTime > 0 &&
            measInfo[i].isActive &&
            (measInfo[i].measTime + DOT11_MEASUREMENT_VALID_TIME) <
            node->getNodeTime())
        {
            measInfo[i].isActive = FALSE;
        }
     }

    // find one inactive or with the oldest time stamp
    oldestTimeStamp = node->getNodeTime();
    for (i = 0; i < DOT11_MAX_NUM_MEASUREMNT; i ++)
    {
        if (!measInfo[i].isActive)
        {
            // if find one inactive, it can be replaced with the new one
            oldestMeas = i;
            break;
        }
        else if (measInfo[i].measTime < oldestTimeStamp)
        {
            // if all are active find the one with oldest stamp
            oldestMeas = i;
            oldestTimeStamp = measInfo[i].measTime;
        }
    }

    // store the current meas info
    measInfo[oldestMeas].isActive = TRUE;
    measInfo[oldestMeas].cinr = signalMea->cinr;
    measInfo[oldestMeas].rssi = signalMea->rss;
    measInfo[oldestMeas].measTime = node->getNodeTime();

    // calculate the mean
    // both time and number of measurement are considered in this case
    meanMeas->cinr = 0;
    meanMeas->rssi = 0;
    for (i = 0; i < DOT11_MAX_NUM_MEASUREMNT; i ++)
    {
        if (measInfo[i].isActive)
        {
            meanMeas->cinr += measInfo[i].cinr;
            meanMeas->rssi += measInfo[i].rssi;
            numActiveMeas ++;
        }
    }

    // average it
    meanMeas->cinr /= numActiveMeas;
    meanMeas->rssi /= numActiveMeas;
    meanMeas->measTime = node->getNodeTime();
}

static
void MacDot11StationUpdateAPMeasurement(Node* node,
        const MacDataDot11* const dot11,
        DOT11_ApInfo* apInfo,
        const Message* const msg)
{
    PhySignalMeasurement* currMsgMea;
//    PhySignalMeasurement signalMeasVal;
    Message* newmsg = (Message*)msg;
    DOT11_SignalMeasurementInfo meanMeas;
    currMsgMea = (PhySignalMeasurement*) MESSAGE_ReturnInfo(newmsg);

    if (currMsgMea == NULL)
    {
        // no measurement data, do nothing
        return;
    }
    if (apInfo != NULL)
    {
        // update the measurement histroy and return the mean
        MacDot11StationUpdateMeanMeasurement(node,
              dot11,
              &(apInfo->measInfo[0]),
              currMsgMea,
              &meanMeas);

        apInfo->cinrMean = meanMeas.cinr;
        apInfo->rssMean = meanMeas.rssi;
        apInfo->lastMeaTime = node->getNodeTime();
    }
}

static
void MacDot11StationUpdateAPMeasurement(Node* node,
        const MacDataDot11* const dot11,
        const Message* const msg)
{
    MacDot11StationUpdateAPMeasurement(node,
        dot11, dot11->associatedAP,msg);

}

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsMyBroadcastDataFrame
//  PURPOSE:     Determine if broadcast data frame should be received.
//  PARAMETERS:  const MacDataDot11 * const dot11,
//                  Pointer to Dot11 structure
//               const Message * const msg
//                  Frame message
//  RETURN:      TRUE when BSS station receives from its AP/PC
//                       except when the source ip address was itself
//                    when AP/PC receives any broadcast in relay mode
//                    when IBSS station receives any broadcast
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsMyBroadcastDataFrame(
    Node* node,
    const MacDataDot11* const dot11,
    const Message* const msg)
{
    BOOL isMyFrame = FALSE;
    DOT11_FrameHdr* fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

    ERROR_Assert(fHdr->destAddr == ANY_MAC802,
        "MacDot11IsMyBroadcastDataFrame: Not a broadcast frame.\n");

   if (!(MacDot11IsIBSSStation(dot11)))
    {
         if (MacDot11IsAp(dot11))
        {
            if (fHdr->frameFlags & DOT11_TO_DS)
            {
                isMyFrame = TRUE;
            }
            else if (dot11->relayFrames &&
                ((fHdr->frameType == DOT11_DATA && !dot11->isQosEnabled)||
                (fHdr->frameType == DOT11_QOS_DATA && dot11->isQosEnabled)))
            {
                isMyFrame = TRUE;
            }
        }
        else
        {
            isMyFrame = FALSE;

            if (MacDot11IsStationNonAPandJoined(dot11) &&
                dot11->ScanStarted == FALSE &&
                MacDot11IsFrameFromMyAP(dot11,fHdr->sourceAddr))
            {
                isMyFrame = TRUE;
                //This is frame from my AP, update measurement
                MacDot11StationUpdateAPMeasurement(node, dot11, msg);
            }
            // Check that this is not own broadcast resent by the AP
            if (fHdr->address3 == dot11->selfAddr) {
                isMyFrame = FALSE;
            }
        }
    }
    else {
        if (fHdr->frameFlags & DOT11_FROM_DS)
        {
            isMyFrame = TRUE;
        }
        else
        {
            if (dot11->bssAddr != fHdr->address3)
             {
                  isMyFrame = FALSE;
             }
             else
             {
                isMyFrame = TRUE;
             }
        }
    }

    return isMyFrame;
}// MacDot11IsMyBroadcastDataFrame


//--------------------------------------------------------------------------
//  NAME:        MacDot11IsMyUnicastDataFrame
//  PURPOSE:     Determine if unicast data frame should be received.
//  PARAMETERS:  const MacDataDot11 * const dot11,
//                  Pointer to Dot11 structure
//               const Message * const msg
//                  Frame message
//  RETURN:      TRUE when BSS station receives from its AP/PC
//                    when AP/PC receives from BSS station
//                    when AP/PC receives directed frame in relay mode
//                    when IBSS station receives directed frame
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsMyUnicastDataFrame(
    Node* node,
    const MacDataDot11* const dot11,
    const Message* const msg)
{
    BOOL isMyFrame = FALSE;
    DOT11_FrameHdr* fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

//support fix for ticket#1087 start
    BOOL isMyAddr = FALSE;

    if (NetworkIpIsUnnumberedInterface(
            node, dot11->myMacData->interfaceIndex))
    {
        MacHWAddress linkAddr;
        Convert802AddressToVariableHWAddress(
                                node, &linkAddr, &(fHdr->destAddr));
        isMyAddr = MAC_IsMyAddress(node, &linkAddr);
    }
    else
    {
        isMyAddr = (dot11->selfAddr == fHdr->destAddr);
    }

    if (!isMyAddr){
        return FALSE;
    }

    //if (dot11->selfAddr != fHdr->destAddr) {
    //    return FALSE;
    //}
//support fix for ticket#1087 end

    if (!(MacDot11IsIBSSStation(dot11))) {
        if (MacDot11IsAp(dot11))
        {
            if (fHdr->frameFlags & DOT11_TO_DS)
            {
                isMyFrame = TRUE;
            }
            else if (dot11->relayFrames &&
                ((fHdr->frameType == DOT11_DATA && !dot11->isQosEnabled)||
                (fHdr->frameType == DOT11_QOS_DATA && dot11->isQosEnabled)))
            {
                isMyFrame = TRUE;
            }
        }
        else
        {
            // For a BSS station, check that the frame is from own AP
            if ((fHdr->frameFlags & DOT11_FROM_DS) &&
                 (fHdr->sourceAddr == dot11->bssAddr) ) {
                //This is frame from my AP, update measurement
                MacDot11StationUpdateAPMeasurement(node, dot11, msg);
                isMyFrame = TRUE;
            }
        }
    }
    else {
        // An IBSS station will process frames from any source.
        // As will an AP/PC in relay mode.
        if (fHdr->frameFlags & DOT11_FROM_DS)
        {
            isMyFrame = TRUE;
        }
        else
        {
             if (dot11->bssAddr != fHdr->address3)
             {
                  isMyFrame = FALSE;
             }
             else
             {
                isMyFrame = TRUE;
             }
        }
    }

    return isMyFrame;
}// MacDot11IsMyUnicastDataFrame


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationResetCurrentMessageVariables
//  PURPOSE:     Reset variables related to single message buffer at
//               the Dot11 interface.
//               Called after a broadcast is sent or when a unicast is
//               backed or dropped.
//
//  PARAMETERS:  Node* node
//                  Ponter to Node
//               MacDataDot11* const dot11
//                  Pointer to dot11 structure.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

static //inline
void MacDot11StationResetCurrentMessageVariables(
    Node* node,
    MacDataDot11* const dot11)
{

    if (dot11->dot11TxFrameInfo != NULL) {
        if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
        dot11->dot11TxFrameInfo = NULL;
    }
        dot11->currentMessage = NULL;

    dot11->currentNextHopAddress = ANY_MAC802;
    dot11->ipNextHopAddr = ANY_MAC802;
    dot11->ipDestAddr = ANY_MAC802;
    dot11->ipSourceAddr = ANY_MAC802;
    dot11->dataRateInfo = NULL;

    // dot11s. Reset current message frameInfo
    if (dot11->txFrameInfo != NULL) {
        Dot11s_MemFreeFrameInfo(node, &(dot11->txFrameInfo), FALSE);
    }

}// MacDot11StationResetCurrentMessageVariables


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationResetAckVariables
//  PURPOSE:     Reset variables once an ack is received.
//  PARAMETERS:  MacDataDot11* const dot11
//                  Pointer to dot11 structure.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationResetAckVariables(
    MacDataDot11* const dot11)
{
    dot11->SSRC = 0;
    dot11->SLRC = 0;
    dot11->waitingForAckOrCtsFromAddress = INVALID_802ADDRESS;
}// MacDot11StationResetAckVariables


//--------------------------------------------------------------------------
// NAME         MacDot11StationCheckHeaderSizes
// PURPOSE      Check for valid header sizes.
// PARAMETERS   None
// RETURN       None
// NOTES        None
//-------------------------------------------------------------------------
static //inline
void MacDot11StationCheckHeaderSizes()
{
    int shortSize = sizeof(DOT11_ShortControlFrame);
    int longSize = sizeof(DOT11_LongControlFrame);
    int Dot11HdrSize = sizeof(DOT11_FrameHdr);
    int Dot11eHdrSize = sizeof(DOT11e_FrameHdr);

//---------------------DOT11e--Updates------------------------------------//

    assert(shortSize <= DOT11_SHORT_CTRL_FRAME_SIZE);
    assert(longSize == DOT11_LONG_CTRL_FRAME_SIZE);
    assert(Dot11HdrSize == 28);
    assert(Dot11eHdrSize == 30);


//--------------------DOT11e-End-Updates---------------------------------//

}// MacDot11StationCheckHeaderSizes


//--------------------------------------------------------------------------
//  NAME:        MacDot11ReturnAccessCategory
//  PURPOSE:     Returns the correct AC for the priority send.
//               the Dot11 interface.
//
//  PARAMETERS:  int priority
//                  priority for which ACcounter has to be returened back.
//
//  RETURN:      int
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int MacDot11ReturnAccessCategory(int priority)
{
    int acIndex = 0;
    if (priority == 0)
{
        // If priority 0 then put it to BK
        acIndex = DOT11e_AC_BK;
    }
    else if (priority >= 1 && priority <= 2)
    {
        // If priority 1 - 2 then put it to BE
        acIndex = DOT11e_AC_BE;
        }
    else if (priority >= 3 && priority <= 5)
    {
        // If priority 3 - 5 then put it to VI
        acIndex = DOT11e_AC_VI;
        }
    else
    {
        // If priority 6 - 7 then put it to VO
        acIndex = DOT11e_AC_VO;
    }

    return acIndex;
}//MacDot11ReturnAccessCategory

//----------------------------------------------------------------------
// NAME       :: Dot11_ReadFromElement
// LAYER      :: MAC
// PURPOSE    :: Utility function to read a field from an IE.
// PARAMETERS ::
//   + element   : DOT11_Ie*     : pointer to IE
//   + readOffset : int*         : position from which to read
//   + field     : Type T        : field to be read
//   + length    : int           : length to be read, optional
// RETURN     :: void
// NOTES      :: Does not work when reading strings.
//                It is good practice to pass length to avoid surprises
//                due to sizeof(field) across platforms.
//----------------------------------------------------------------------
template <class T>
static
void Dot11_ReadFromElement(
    DOT11_Ie* element,
    int* readOffset,
    T* field,
    int length = 0)
{
    int fieldLength = (length == 0) ? (int)(sizeof(*field)) : length;

    ERROR_Assert(sizeof(*field) == fieldLength,
        "Dot11_ReadFromElement: "
        "size of field does not match length parameter.\n");
    ERROR_Assert(*readOffset + fieldLength <= element->length,
        "Dot11_ReadFromElement: "
        "read length goes beyond IE length.\n");

    memcpy(field, element->data + *readOffset, (size_t)(fieldLength));
    *readOffset += fieldLength;
}


//------------------------------------------------------------------------
// FUNCTION   :: Dot11_ParseCfsetIdElement
// LAYER      :: MAC
// PURPOSE    :: To parse CFSet ID element
// PARAMETERS ::
//   + node      : Node*         : pointer to node
//   + element   : DOT11_Ie*     : pointer to IE
//   + cfSet     :  DOT11_CFParameterSet*  : parsed mesh ID with
//                    null terminator
// RETURN     :: void
//-------------------------------------------------------------------------
static
void Dot11_ParseCfsetIdElement(
    Node* node,
    DOT11_Ie* element,
    DOT11_CFParameterSet* cfSet)
{
    int ieOffset = 0;
    int length = 0;
    DOT11_IeHdr ieHdr;
    Dot11_ReadFromElement(element, &ieOffset, &ieHdr,length);
    ERROR_Assert(
        ieHdr.id == DOT11_CF_PARAMETER_SET_ID,
        "Dot11_ParseCfsetIdElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    ERROR_Assert(
        ieHdr.length == sizeof(DOT11_CFParameterSet),
        "Dot11_ParseCfsetIdElement: "
        "CFParameterSet length is not equal.\n");
    memset(cfSet, 0, sizeof(DOT11_CFParameterSet));
    memcpy(cfSet, element->data + ieOffset, ieHdr.length);
    ieOffset += ieHdr.length;

    ERROR_Assert(ieOffset == element->length,
        "Dot11_ParseCfsetIdElement: all fields not parsed.\n") ;
}// Dot11_ParseCfsetIdElement


/// ---------------------------------------------------------------------
// FUNCTION   :: Dot11_ParseTimIdElemen
// LAYER      :: MAC
// PURPOSE    ::  To Parse the TIM Id Element from Beacon
// PARAMETERS ::
//  + node      : Node*         : pointer to node
//  + element   : DOT11_Ie*     : pointer to IE
//  + timFrame  : DOT11_TIMFrame*   : parsed Tim ID with null terminator
// RETURN     :: void
//-------------------------------------------------------------------------

static
void Dot11_ParseTimIdElement(
    Node* node,
    DOT11_Ie* element,
    DOT11_TIMFrame* timFrame)
{
    int ieOffset = 0;
    int length = 0;
    DOT11_IeHdr ieHdr;
    Dot11_ReadFromElement(element, &ieOffset, &ieHdr,length);
    ERROR_Assert(
        ieHdr.id == DOT11_PS_TIM_ELEMENT_ID_TIM,
        "Dot11_ParseTimIdElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    ERROR_Assert(
        ieHdr.length == sizeof(DOT11_TIMFrame),
        "Dot11_ParseTimIdElement: "
        "Tim Element length is not equal.\n");
    memcpy(timFrame, element->data + ieOffset, ieHdr.length);
    ieOffset += ieHdr.length;

    ERROR_Assert(ieOffset == element->length,
        "Dot11_ParseTimIdElement: all fields not parsed.\n") ;
}

/// ------------------------------------------------------------------
// FUNCTION   :: Dot11_GetCfsetFromBeacon
// LAYER      :: MAC
// PURPOSE    :: Receive and process a beacon frame if it has CfSet elements.
// PARAMETERS ::
//   + node      : Node*         : pointer to node
//   + dot11     : MacDataDot11* : pointer to Dot11 data structure
//   + msg       : Message*      : received beacon message
//   + cfSet     : DOT11_CFParameterSet* cfSet : wReturn the CfSet
// RETURN     ::BOOL
//-------------------------------------------------------------------

static
BOOL Dot11_GetCfsetFromBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_CFParameterSet* cfSet)
{
    BOOL isFound = FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_BEACON,
        "Dot11_GetCfsetFromBeacon: Wrong frame type.\n");

    int offset = sizeof(DOT11_BeaconFrame);
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
    while (ieHdr.id != DOT11_CF_PARAMETER_SET_ID)
    {
        offset += ieHdr.length + 2;
        if (offset >= rxFrameSize)
        {
            return isFound;
        }
        ieHdr = rxFrame + offset;
    }

    if (ieHdr.id == DOT11_CF_PARAMETER_SET_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11_ParseCfsetIdElement(node, &element, cfSet);
        isFound = TRUE;
    }
       return isFound;
}//Dot11_GetCfsetFromBeacon


//-------------------------------------------------------------------
// FUNCTION   :: Dot11_GetTimFromBeacon
// LAYER      :: MAC
// PURPOSE    :: Receive and process a beacon frame if it has TIM elements.
// PARAMETERS ::
//  + node      : Node*         : pointer to node
//  + dot11     : MacDataDot11* : pointer to Dot11 data structure
//  + msg       : Message*      : received beacon message
//  + timFrame  : DOT11_TIMFrame*  : Return the Tim
// RETURN     ::BOOL
//-----------------------------------------------------------------------
static
void Dot11_GetTimFromBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    int offset,
     DOT11_TIMFrame* timFrame)
{
    BOOL isFound=FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_BEACON,
        "Dot11_GetTimFromBeacon: Wrong frame type.\n");

    int rxFrameSize = DOT11nMessage_ReturnPacketSize(msg);

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;

    while (ieHdr.id != DOT11_PS_TIM_ELEMENT_ID_TIM)
    {
        offset += ieHdr.length + 2;
        if (offset >= rxFrameSize)
        {
            return;
        }
        ieHdr = rxFrame + offset;

    }

    if (ieHdr.id == DOT11_PS_TIM_ELEMENT_ID_TIM)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11_ParseTimIdElement(node, &element, timFrame);

        isFound = TRUE;
    }
}//Dot11_GetTimFromBeacon

//---------------------------------------------------------------------
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
    Message* msg);
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationInformNetworkOfPktDrop.
//  PURPOSE:     Inform Network layer that a packet is dropped because of
//               unsuccessful delivery.
//  PARAMETERS:  Node* node
//                  Pointer to node that was unable to deliver the network
//                   packet.
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                   Network packet that got dropped
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationInformNetworkOfPktDrop(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    MAC_NotificationOfPacketDrop(node,
                                 dot11->ipNextHopAddr,
                                 dot11->myMacData->interfaceIndex,
                                 msg);
}// MacDot11StationInformNetworkOfPktDrop


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetSeqNo.
//  PURPOSE:     Return the entry for given destAddr.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      DOT11_SeqNoEntry Pointer to Entry for given destAddr.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
DOT11_SeqNoEntry* MacDot11StationGetSeqNo(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    DOT11_SeqNoEntry* entry = dot11->seqNoHead;
    DOT11_SeqNoEntry* prev = NULL;

    if (!entry) {
        entry = (DOT11_SeqNoEntry*) MEM_malloc(sizeof(DOT11_SeqNoEntry));
        entry->ipAddress = destAddr;
        entry->fromSeqNo = 0;
        entry->toSeqNo = 0;
        entry->next = NULL;
        dot11->seqNoHead = entry;
        return entry;
    }

    while (entry) {
        if (entry->ipAddress == destAddr) {
            return entry;
        }
        else {
            prev = entry;
            entry = entry->next;
        }
    }

    entry = (DOT11_SeqNoEntry*)
        MEM_malloc(sizeof(DOT11_SeqNoEntry));
    entry->ipAddress = destAddr;
    entry->fromSeqNo = 0;
    entry->toSeqNo = 0;
    entry->next = NULL;

    prev->next = entry;

    return entry;
}// MacDot11StationGetSeqNo

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetSeqNo.
//  PURPOSE:     Return the entry for given destAddr which supports QoS.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      DOT11_SeqNoEntry Pointer to Entry for given destAddr.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
DOT11_SeqNoEntry* MacDot11StationGetSeqNo(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    int TID)
{
    DOT11_SeqNoEntry* entry = NULL;
    DOT11_SeqNoEntry** head = NULL;
    DOT11_SeqNoEntry* prev = NULL;
    int acIndex;
    int index;

    if (TID <8)
    {
         acIndex= MacDot11ReturnAccessCategory(TID);
         head = &(dot11->ACs[acIndex].seqNoHead);
         entry = *head;
         prev = NULL;
    }
    else
    {

         index = TID - DOT11e_HCCA_MIN_TSID;
         head = &(dot11->hcVars->staTSBufferItem[index]->seqNoHead);
         entry = *head;
         prev = NULL;
    }
    if (!entry) {
        entry = (DOT11_SeqNoEntry*) MEM_malloc(sizeof(DOT11_SeqNoEntry));
        entry->ipAddress = destAddr;
        entry->fromSeqNo = 0;
        entry->toSeqNo = 0;
        entry->next = NULL;
        *head = entry;
        return entry;
    }

    while (entry) {
        if (entry->ipAddress == destAddr) {
            return entry;
        }
        else {
            prev = entry;
            entry = entry->next;
        }
    }

    entry = (DOT11_SeqNoEntry*)
        MEM_malloc(sizeof(DOT11_SeqNoEntry));
    entry->ipAddress = destAddr;
    entry->fromSeqNo = 0;
    entry->toSeqNo = 0;
    entry->next = NULL;

    prev->next = entry;

    return entry;
}// MacDot11StationGetSeqNo

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCorrectSequenceNumber.
//  PURPOSE:     Check if ACK contains the right sequence number.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAdd
//                  Node address of station
//               int seqNo
//                  Sequence number of the frame that is being acked
//  RETURN:      TRUE if sequence number being ACKed is correct.
//               FALSE otherwise.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationCorrectSeqenceNumber(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    int seqNo)
{
    DOT11_SeqNoEntry* entry;

    entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr);

    ERROR_Assert(entry,
        "MacDot11CorrectSeqenceNumber: "
        "Sequence number entry not found.\n");

    return (entry->fromSeqNo == seqNo);
}// MacDot11StationCorrectSeqenceNumber
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCorrectSequenceNumber.
//  PURPOSE:     Check if ACK contains the right sequence number for QoS frames.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAdd
//                  Node address of station
//               int seqNo
//                  Sequence number of the frame that is being acked
//  RETURN:      TRUE if sequence number being ACKed is correct.
//               FALSE otherwise.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11StationCorrectSeqenceNumber(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    int seqNo,
    int TID)
{

    DOT11_SeqNoEntry* entry;

    entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr,TID);
    ERROR_Assert(entry,
        "MacDot11CorrectSeqenceNumber: "
        "Sequence number entry not found.\n");

    return (entry->fromSeqNo == seqNo);


}// MacDot11StationCorrectSeqenceNumber

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetFromAndToDsInHdr
//  PURPOSE:     Set the FromDS/ToDS flag in data frame header
//               depending on whether the AP/PC or BSS station is
//               the sender.
//
//               In case of relay, there could be a check for
//               destination which is outside BSS. However, in the
//               implementation, an IBSS station would ignore the
//               FromDS setting.
//
//  PARAMETERS:  const MacDataDot11 * const dot11,
//                  Pointer to Dot11 structure
//               const Message * const msg
//                  Frame message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationSetFromAndToDsInHdr(
    const MacDataDot11* const dot11,
    DOT11_FrameHdr* const fHdr)
{
    // Set to zero since AddHeader does not memset to 0
    // Modify when other flags are implemented.

    fHdr->frameFlags = 0;

    switch (fHdr->frameType)
    {
        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
        case DOT11_CF_POLL:
        case DOT11_CF_POLL_ACK:
        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
        case DOT11_CF_NULLDATA:
        case DOT11_CF_ACK:
            if (MacDot11IsAp(dot11)) {

                fHdr->frameFlags |= DOT11_FROM_DS;
            }
            else if (MacDot11IsBssStation(dot11)) {

                fHdr->frameFlags |= DOT11_TO_DS;
            }
            break;

        default:
            break;
    } //switch
}// MacDot11StationSetFromAndToDsInHdr

//---------------------DOT11e--Updates------------------------------------//

// /**
// FUNCTION   :: MacDot11eStationSetFromAndToDsInHdr
// LAYER      :: MAC
// PURPOSE    :: Set the FromDS/ToDS flag in data frame header
//               depending on whether the AP/PC or BSS station is
//               the sender.
//               In case of relay, there could be a check for
//               destination which is outside BSS. However, in the
//               implementation, an IBSS station would ignore the
//               FromDS setting.
// PARAMETERS ::
// + node      : Node* :Node being Initialized
// + dot11     : MacDataDot11* : Structure of dot11.
// + phyModel  : PhyModel : NPHY model used.
// RETURN     :: void : NULL
// **/
static //inline
void MacDot11eStationSetFromAndToDsInHdr(
    const MacDataDot11* const dot11,
    DOT11e_FrameHdr* const fHdr)
{
    // Set to zero since AddHeader does not memset to 0
    // Modify when other flags are implemented.
    fHdr->frameFlags = 0;

    switch (fHdr->frameType)
    {
        case DOT11_QOS_DATA:
        {
            if (MacDot11IsAp(dot11)) {

                fHdr->frameFlags |= DOT11_FROM_DS;
            }
            else if (MacDot11IsBssStation(dot11)) {

                fHdr->frameFlags |= DOT11_TO_DS;
            }
            break;
        }
        default:
            break;
    } //switch
}// MacDot11eStationSetFromAndToDsInHdr

//--------------------DOT11e-End-Updates---------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetFieldsInDataFrameHdr
//  PURPOSE:     Set frame type, frame flag and address fields of the
//               data frame being sent.
//
//  PARAMETERS:  const MacDataDot11 * dot11,
//                  Pointer to Dot11 structure
//               DOT11_FrameHdr * const fHdr,
//                  Frame header
//               Mac802Address destAddr
//                  Destination address
//               DOT11_MacFrameType frameType
//                  frame type
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationSetFieldsInDataFrameHdr(
    MacDataDot11*  dot11,
    DOT11_FrameHdr* const fHdr,
    Mac802Address destAddr,
    DOT11_MacFrameType frameType)
{
    fHdr->frameType = (unsigned char) frameType;
    MacDot11StationSetFromAndToDsInHdr(dot11, fHdr);
//---------------------------Power-Save-Mode-Updates---------------------//
    // SET more data field here
    if (fHdr->frameType == DOT11_CF_NULLDATA &&
        dot11->state == DOT11_CFP_START){

            if (dot11->currentMessage != NULL){
            fHdr->frameFlags |= DOT11_MORE_DATA_FIELD;
        }
    }
    else if (MacDot11IsIBSSStationSupportPSMode(dot11)){

    if (dot11->isMoreDataPresent == TRUE){
        fHdr->frameFlags |= DOT11_MORE_DATA_FIELD;
        }
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//

    fHdr->destAddr = destAddr;
    fHdr->sourceAddr = dot11->selfAddr;
    fHdr->address3 = dot11->bssAddr;
    fHdr->fragId = 0;

    if (dot11->selfAddr != dot11->bssAddr) {

        fHdr->address3 = dot11->ipDestAddr;
    }
    else if (MacDot11IsAp(dot11)) {

        fHdr->address3 = dot11->ipSourceAddr;
    }
    if (MacDot11IsIBSSStation(dot11))
     {
         fHdr->address3 = dot11->bssAddr;
     }
}// MacDot11StationSetFieldsInDataFrameHdr

//---------------------DOT11e--Updates------------------------------------//

// /**
// FUNCTION   :: MacDot11eStationSetFieldsInDataFrameHdr
// LAYER      :: MAC
// PURPOSE    :: Set frame type, frame flag and address fields of the
//               data frame being sent.
// PARAMETERS ::
// + const dot11 : const MacDataDot11 * : Pointer to Dot11 structure
// + const fHdr  : DOT11_FrameHdr* : Frame header
// + destAddr    : Mac802Address : Destination address
// + frameType   : DOT11e_MacFrameType:  Frame type
// RETURN     :: void : NULL
// **/
static //inline
void MacDot11eStationSetFieldsInDataFrameHdr(
    const MacDataDot11* const dot11,
    DOT11e_FrameHdr* const fHdr,
    Mac802Address destAddr,
    DOT11_MacFrameType frameType,
    int acIndex)
{
    fHdr->frameType = (unsigned char) frameType;
    MacDot11eStationSetFromAndToDsInHdr(dot11, fHdr);

    fHdr->destAddr = destAddr;
    fHdr->sourceAddr = dot11->selfAddr;
    fHdr->address3 = dot11->bssAddr;

    if (dot11->selfAddr != dot11->bssAddr) {

        fHdr->address3 = dot11->ipDestAddr;
    }
    else if (MacDot11IsAp(dot11)) {

        fHdr->address3 = dot11->ipSourceAddr;
    }
    if (MacDot11IsIBSSStation(dot11))
    {
         fHdr->address3 = dot11->bssAddr;
     }

    // Set the QoS Control Field Information
    memset(&(fHdr->qoSControl), 0, sizeof(DOT11e_QoSControl));


//--------------------HCCA-Updates Start---------------------------------//
    if (dot11->state == DOT11e_CAP_START )
    {
        if (MacDot11IsHC(dot11))
        {
            fHdr->qoSControl.TXOP = (UInt8)ceil(((Float64)
                dot11->hcVars->txopDurationGranted / MILLI_SECOND));

            fHdr->qoSControl.TID = (UInt16)dot11->hcVars->currentTSID;
            fHdr->qoSControl.EOSP = 1;
            fHdr->duration= (UInt16)MacDot11NanoToMicrosecond(
                dot11->hcVars->txopDurationGranted);


        }
        else
        {
            fHdr->qoSControl.TID = (UInt16)dot11->hcVars->currentTSID;
            fHdr->qoSControl.EOSP = 1;
            fHdr->qoSControl.TXOP = (UInt8)ceil(((Float64)
                dot11->hcVars->txopDurationGranted / MILLI_SECOND));
        }

        if (fHdr->qoSControl.TID==0)
            printf("Got TID 0");
    }
    else
    {
        fHdr->qoSControl.TID = (UInt16)dot11->ACs[acIndex].priority;
        fHdr->qoSControl.EOSP = 1;
        fHdr->qoSControl.TXOP = (UInt8)ceil(((Float64)
            dot11->ACs[dot11->currentACIndex].TXOPLimit / MILLI_SECOND));
    }
//--------------------HCCA-Updates End-----------------------------------//


    if (DEBUG_QA)
    {
        printf("DOT11e frame header TID %d TXOP %d  priority %d"
        "txlimit""%" TYPES_64BITFMT "d acIndex %u\n",
        fHdr->qoSControl.TID,
        fHdr->qoSControl.TXOP,
        dot11->ACs[acIndex].priority,
        dot11->ACs[acIndex].TXOPLimit,
        acIndex);
    }

}// MacDot11eStationSetFieldsInDataFrameHdr
//--------------------DOT11e-End-Updates---------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationWaitForNAV.
//  PURPOSE:     See if this node is holding for NAV.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if holding for NAV, FALSE otherwise.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
BOOL MacDot11StationWaitForNAV(
    Node* node,
    MacDataDot11* dot11)
{
    return (dot11->NAV > node->getNodeTime());
}// MacDot11StationWaitForNAV


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationPhyStatus
//  PURPOSE:     Get the physical status.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      The physical status of the medium
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
PhyStatusType MacDot11StationPhyStatus(
    Node* node,
    MacDataDot11* dot11)
{
    return PHY_GetStatus(node, dot11->myMacData->phyNumber);
}// MacDot11StationPhyStatus

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetState.
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
static //inline
void MacDot11StationSetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacStates state)
{
    dot11->prevState = dot11->state;
    dot11->state = state;
    //printf("\nNode id %d sim time %lld state %d",node->nodeId,*(node)->currentTime,dot11->state);
}// MacDot11StationSetState

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationResetCW.
//  PURPOSE:     Reset congestion window since transmission was successful.
//  PARAMETERS:  Node* node
//                  Pointer to node that is resetting its contention window
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
void MacDot11StationResetCW(
    Node* node,
    MacDataDot11* dot11)
{
//---------------------DOT11e--Updates------------------------------------//
    if (MacDot11IsQoSEnabled(node, dot11))
    {
        if (dot11->currentACIndex >= DOT11e_AC_BK)
        {
            dot11->ACs[dot11->currentACIndex].CW = dot11->ACs[dot11->currentACIndex].cwMin;
        } else {
            dot11->CW = dot11->cwMin;
        }

    } else {
        if (dot11->currentACIndex == 0)
            dot11->ACs[dot11->currentACIndex].CW = dot11->cwMin;
        dot11->CW = dot11->cwMin;
    }
//--------------------DOT11e-End-Updates---------------------------------//

}// MacDot11StationResetCW

//--------------------------------------------------------------------------
//  NAME:        MacDot11PauseOtherAcsBo
//  PURPOSE:     pause the backoff counter of the other ACs having the packet
//  PARAMETERS:  Node* node
//                  Pointer to node that is stopping its own backoff.
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype backoff
//                  Total backoff elapsed till now
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11PauseOtherAcsBo(Node* node, MacDataDot11* dot11,
                             clocktype backoff)
{
    clocktype totalTime = 0;

    for (int acCounter = 0; acCounter < DOT11e_NUMBER_OF_AC; acCounter++){
        if (acCounter!=dot11->currentACIndex) {
            totalTime = (dot11->ACs[dot11->currentACIndex].AIFS -
                dot11->ACs[acCounter].AIFS) + backoff;
            if (totalTime>0 &&dot11->ACs[acCounter].frameToSend != NULL) {
                dot11->ACs[acCounter].BO = dot11->ACs[acCounter].BO - totalTime;
                if (dot11->ACs[acCounter].BO<0)
                {
                    dot11->ACs[acCounter].BO = 0;
                    MacDot11eStationRetryForInternalCollision(
                                node, dot11, acCounter);
                }
            }
        }
    }
} //MacDot11PauseOtherAcsBo

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationResetBackoffAndAckVariables
//  PURPOSE:     (Re)set some DCF variables during transit between
//               DCF and PCF.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11StationResetBackoffAndAckVariables(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->BO = 0;
    MacDot11StationResetCW(node, dot11);
    MacDot11StationResetAckVariables(dot11);

    if (dot11->state == DOT11_CFP_START){
        if (dot11->currentACIndex >= DOT11e_AC_BK) {
            MacDot11PauseOtherAcsBo(
                 node, dot11, dot11->ACs[dot11->currentACIndex].BO);
            dot11->ACs[dot11->currentACIndex].BO = 0;
        }
    }
}// MacDot11StationResetBackoffAndAckVariables



//--------------------------------------------------------------------------
//  Timer Functions
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartTimerOfGivenType
//  PURPOSE:     Set a timer of given message/event type.
//
//               Similar to MacDot11StartTimer which sets timer for
//               MSG_MAC_TimerExpired only.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype timerDelay
//                  Delay
//               int timerType
//                  Delay for the timer
//               timer type
//                  type of timer
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationStartTimerOfGivenType(
    Node* node,
    MacDataDot11* dot11,
    clocktype timerDelay,
    int timerType)
{
    Message* newMsg;
    UInt32 sequenceNumber = 0;

    if (timerType == MSG_MAC_DOT11_Beacon)
    {
        dot11->beaconSequenceNumber++;
        sequenceNumber = dot11->beaconSequenceNumber;
    }
    else
    {
        dot11->timerSequenceNumber++;
        sequenceNumber = dot11->timerSequenceNumber;
    }

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_DOT11,
                           timerType);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(sequenceNumber));
    *((Int32*)(MESSAGE_ReturnInfo(newMsg))) = sequenceNumber;
    newMsg->originatingNodeId = node->nodeId;

    MESSAGE_Send(node, newMsg, timerDelay);
}// MacDot11StationStartTimerOfGivenType

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartTimerOfGivenType
//  PURPOSE:     Set a timer of given message/event type.
//
//               Similar to MacDot11StartTimer which sets timer for
//               MSG_MAC_TimerExpired only.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype timerDelay
//                  Delay
//               int timerType
//                  Delay for the timer
//               timer type
//                  type of timer
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationStartStationCheckTimer(
    Node* node,
    MacDataDot11* dot11,
    clocktype timerDelay,
    int timerType)
{
    Message* newMsg;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_DOT11,
                           timerType);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);

    MESSAGE_Send(node, newMsg, timerDelay);
}// MacDot11StationStartTimerOfGivenType

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartTimer.
//  PURPOSE:     Set various timers in Dot11.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype timerDelay
//                  Duration of the timer being set.
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11StationStartTimer(
    Node* node,
    MacDataDot11* dot11,
    clocktype timerDelay)
{
    Message *newMsg;

    dot11->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_DOT11,
                           MSG_MAC_TimerExpired);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(dot11->timerSequenceNumber));
    *((int*)(MESSAGE_ReturnInfo(newMsg))) = dot11->timerSequenceNumber;

    newMsg->originatingNodeId = node->nodeId;

    MESSAGE_Send(node, newMsg, timerDelay);

}// MacDot11StationStartTimer



//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCancelTimer.
//  PURPOSE:     Cancel the current timer.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationCancelTimer(
    Node* node,
    MacDataDot11* dot11)
{
    // Has the effect of making the current timer "obsolete".
    dot11->timerSequenceNumber++;

}// MacDot11StationCancelTimer

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCancelBeaconTimer.
//  PURPOSE:     Cancel the beacon timer.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationCancelBeaconTimer(
    Node* node,
    MacDataDot11* dot11)
{
    // Has the effect of making the timer "obsolete".
    dot11->beaconSequenceNumber++;

}// MacDot11StationCancelBeaconTimer

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationIncreaseCW.
//  PURPOSE:     Increase control window since node has to retransmit.
//  PARAMETERS:  Node* node
//                  Pointer to node thatis increasing its contention window
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
void MacDot11StationIncreaseCW(
    Node* node,
    MacDataDot11* dot11)
{
    // DOT11e Updates
    if (MacDot11IsQoSEnabled(node, dot11))
    {
        if (dot11->currentACIndex >= DOT11e_AC_BK)
        {
            dot11->ACs[dot11->currentACIndex].CW =
                MIN((((dot11->ACs[dot11->currentACIndex].CW + 1) * 2) - 1),
                    dot11->ACs[dot11->currentACIndex].cwMax);
        }else {
            dot11->CW = MIN((((dot11->CW + 1) * 2) - 1), dot11->cwMax);
        }

    } else {
        dot11->CW = MIN((((dot11->CW + 1) * 2) - 1), dot11->cwMax);
        if (dot11->currentACIndex != DOT11e_INVALID_AC)
        dot11->ACs[dot11->currentACIndex].CW  = dot11->CW;
    }
}// MacDot11StationIncreaseCW

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetBackoffIfZero.
//  PURPOSE:     Set the backoff counter for this node.
//  PARAMETERS:  Node* node
//                  Pointer to node that is being backed off.
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
void MacDot11StationSetBackoffIfZero(
    Node* node,
    MacDataDot11* dot11,
    int currentACIndex = DOT11e_INVALID_AC)
{
    // DOT11e Updates

        if (MacDot11IsQoSEnabled(node, dot11)) {
            if (currentACIndex != DOT11e_INVALID_AC) {
                 if (dot11->ACs[currentACIndex].BO ==0){
                    dot11->ACs[currentACIndex].BO =
                        (RANDOM_nrand(dot11->seed) %
                        (dot11->ACs[currentACIndex].CW + 1)) *
                        dot11->ACs[currentACIndex].slotTime;
                 }
             }
              else {
                    if (dot11->BO == 0) {
                        dot11->BO = (RANDOM_nrand(dot11->seed) %
                                (dot11->CW + 1))
                                * dot11->slotTime;
                    }
             }
        }
    else {
        if (currentACIndex != DOT11e_INVALID_AC) {

            if (dot11->ACs[currentACIndex].BO ==0){
                 dot11->ACs[currentACIndex].BO = (RANDOM_nrand(dot11->seed) %
                        (dot11->ACs[currentACIndex].CW + 1)) *
                        dot11->ACs[currentACIndex].slotTime;
                 dot11->BO = dot11->ACs[currentACIndex].BO;
            }
        }
        else {
            if (dot11->BO == 0) {
//---------------------------Power-Save-Mode-Updates---------------------//
                if (MacDot11IsATIMDurationActive(dot11)){
                    dot11->BO =
                        (RANDOM_nrand(dot11->seed) % (dot11->cwMin + 1))
                        * dot11->slotTime;
                }else{
                     dot11->BO =
                         (RANDOM_nrand(dot11->seed) % (dot11->CW + 1))
                         * dot11->slotTime;
                }
//---------------------------Power-Save-Mode-End-Updates-----------------//
            }
        }
    }
}// MacDot11StationSetBackoffIfZero


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationContinueBackoff.
//  PURPOSE:     Continues decreasing backoff counter.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationContinueBackoff(Node* node, MacDataDot11* dot11)
{
    dot11->lastBOTimeStamp = node->getNodeTime();

    if (dot11->BO < 0) {

        char errString[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];

        ctoa(dot11->BO, clockStr);
        sprintf(errString,
            "MacDot11StationContinueBackoff: "
            "Node %u continuing backoff to a negative timer: %s\n",
            node->nodeId, clockStr);
        ERROR_ReportError(errString);
    }
}// MacDot11StationContinueBackoff

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationPauseBackoff.
//  PURPOSE:     Stop the backoff counter once carrier is sensed.
//  PARAMETERS:  Node* node
//                  Pointer to node that is stopping its own backoff.
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationPauseBackoff(Node* node, MacDataDot11* dot11)
{

clocktype backoff = (node->getNodeTime() - dot11->lastBOTimeStamp);
backoff = (backoff/dot11->slotTime)*dot11->slotTime;
    if (dot11->BO > 0){
        dot11->BO = dot11->BO - backoff;
    }
    if (dot11->BO < 0) {
        dot11->BO = 0;
    }

    // DOT11e Updates
    if (MacDot11IsQoSEnabled(node, dot11)) {
        if (dot11->currentACIndex >= DOT11e_AC_BK){
            dot11->ACs[dot11->currentACIndex].BO = dot11->BO;
            if (dot11->isHTEnable)
            {
                MacDot11nPauseOtherAcsBo(node,dot11,backoff);
            }
            else
            {
            MacDot11PauseOtherAcsBo(node,dot11,backoff);
        }
    }
    }

    else{
        if (dot11->currentACIndex != DOT11e_INVALID_AC)
        dot11->ACs[dot11->currentACIndex].BO = dot11->BO;
    }
}// MacDot11StationPauseBackoff
//--------------------HCCA-Updates Start---------------------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11HcAttempToStartCap
//  PURPOSE:     Attempt to go into DIFS or EIFS state.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
BOOL MacDot11HcAttempToStartCap(
    Node* node,
    MacDataDot11* dot11)
{
    if ((dot11->isHCCAEnable)&&(MacDot11IsHC(dot11)))
    {
        if ((dot11->hcVars->capMaxDuration -
                dot11->hcVars->capPeriodExpired) <= 0)
                return FALSE;

        clocktype currtime = node->getNodeTime();
        clocktype capEndTime;
        if ((dot11->beaconTime - dot11->sifs) <
            (currtime + dot11->hcVars->capMaxDuration -
             dot11->hcVars->capPeriodExpired))
        {
            capEndTime = dot11->beaconTime - dot11->sifs;
        }
        else
        {
            capEndTime = currtime + dot11->hcVars->capMaxDuration -
                dot11->hcVars->capPeriodExpired;
        }

/*********HT START*************************************************/
        clocktype delay = 0;
        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector rxVector1;
            MAC_PHY_TxRxVector rxVector2;
            PHY_GetRxVector(node,
                            dot11->myMacData->phyNumber,
                            rxVector1);
            PHY_GetRxVector(node,
                            dot11->myMacData->phyNumber,
                            rxVector2);
            rxVector1.length = (size_t)sizeof(DOT11e_FrameHdr);
            rxVector2.length = (size_t)DOT11e_HCCA_MIN_TXOP_DATAFRAME;

            delay = dot11->sifs
        + PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
                        rxVector1)
                    + dot11->extraPropDelay
                    + dot11->sifs
                    + PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        rxVector2)
                    + dot11->extraPropDelay;

        }
        else
        {
             delay = dot11->sifs
                 + PHY_GetTransmissionDuration(
                node, dot11->myMacData->phyNumber,
                PHY_GetRxDataRateType(node,
                    dot11->myMacData->phyNumber),
            sizeof(DOT11e_FrameHdr))
        + dot11->extraPropDelay
        + dot11->sifs
        + PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber),
            DOT11e_HCCA_MIN_TXOP_DATAFRAME)
        + dot11->extraPropDelay;

        }
/*********HT END****************************************************/

        if ((currtime+ delay) > capEndTime)
            return FALSE;

        if (MacDot11IsTSsHasMessage(node,dot11)||
            (!(MacDot11eHCPollListIsEmpty(node,dot11)) &&
            dot11->hcVars->hasPollableStations))
        {
            MacDot11StationSetState(node, dot11, DOT11e_WF_CAP_START);
            dot11->hcVars->capStartTime = node->getNodeTime();
            dot11->hcVars->capEndTime = capEndTime;
            MacDot11StationStartTimer(node, dot11, (dot11->txPifs));

            if (DEBUG_HCCA)
            {
                char timeStr[100];
                ctoa(node->getNodeTime(), timeStr);
                printf("MacDot11HcAttempToStartCap: Time=%s\n",timeStr);
            }
            return TRUE;
        }
    }
    return FALSE;
}
//--------------------HCCA-Updates End---------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState
//  PURPOSE:     Attempt to go into DIFS or EIFS state.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL mustWaitForNAV = FALSE;
    clocktype navTimeoutTime  = 0;

    if (MacDot11IsQoSEnabled(node, dot11)
        && dot11->currentMessage!= NULL &&
        dot11->dot11TxFrameInfo->frameType != DOT11_BEACON) {
        MacDot11eSetCurrentMessage(node,dot11);
    }

    if (!dot11->useDvcs) {
        if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);

            return;
        }

        // Should disregard NAV in Extended Ifs Mode. section 9.2.3.4
        if (!dot11->IsInExtendedIfsMode){
            if (MacDot11StationWaitForNAV(node, dot11)) {

                // Set timer to wait for NAV to finish.
                mustWaitForNAV = TRUE;
                navTimeoutTime = dot11->NAV;
            }
        }
        else {
            dot11->NAV = 0;
        }
    }
    else {
        BOOL haveDirection = FALSE;
        double directionAngleToSend = 0.0;

        if (MacDot11StationPhyStatus(node, dot11) == PHY_RECEIVING) {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);

            return;
        }

        if (dot11->currentNextHopAddress != ANY_MAC802) {
            AngleOfArrivalCache_GetAngle(
                &dot11->directionalInfo->angleOfArrivalCache,
                dot11->currentNextHopAddress,
                node->getNodeTime(),
                &haveDirection,
                &directionAngleToSend);
        }

        if (haveDirection) {
            PHY_SetSensingDirection(
                node, dot11->myMacData->phyNumber, directionAngleToSend);
        }

        if (MacDot11StationPhyStatus(node, dot11) == PHY_SENSING) {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);
            return;
        }

        ERROR_Assert(MacDot11StationPhyStatus(node, dot11) == PHY_IDLE,
                     "Current state should be IDLE");

        if (((haveDirection) &&
             (DirectionalNav_SignalIsNaved(
                 &dot11->directionalInfo->directionalNav,
                 directionAngleToSend, node->getNodeTime())))
                  ||
            ((!haveDirection) &&
             (!DirectionalNav_ThereAreNoNavs(
                 &dot11->directionalInfo->directionalNav, node->getNodeTime()))))
        {
            // Should disregard NAV in Extended Ifs Mode. section 9.2.3.4
            if (!dot11->IsInExtendedIfsMode){
                mustWaitForNAV = TRUE;
                DirectionalNav_GetEarliestNavExpirationTime(
                    &dot11->directionalInfo->directionalNav,
                    node->getNodeTime(),
                    &navTimeoutTime);
            }
            else {
                dot11->NAV = 0;
            }
        }
    }

    if (mustWaitForNAV) {
        // Set timer to wait for NAV to finish.
        MacDot11StationSetState(node, dot11, DOT11_S_WFNAV);
        MacDot11StationStartTimer(node, dot11, (navTimeoutTime - node->getNodeTime()));
    }
    else {
        clocktype extraWait = 0;
        MacDot11StationSetState(node, dot11, DOT11_S_WF_DIFS_OR_EIFS);
        if (node->getNodeTime() < dot11->noOutgoingPacketsUntilTime) {
            extraWait = dot11->noOutgoingPacketsUntilTime - node->getNodeTime();
        }

        if (dot11->IsInExtendedIfsMode) {

//---------------------DOT11e--Updates------------------------------------//
            // for DOT11e set with AIFS...
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                if (dot11->currentACIndex != DOT11e_INVALID_AC)
                {
                    
/*********HT START*************************************************/
                    clocktype extendedIfsDelay;
                    if (dot11->isHTEnable)
                    {
                         MAC_PHY_TxRxVector tempTxRxVector 
                              =dot11->txVectorForLowestDataRate;
                         tempTxRxVector.length =
                             DOT11_SHORT_CTRL_FRAME_SIZE;
                         extendedIfsDelay = dot11->sifs +
                                 PHY_GetTransmissionDuration(
                                     node, dot11->myMacData->phyNumber,
                                     tempTxRxVector) +
                                     dot11->ACs[dot11->currentACIndex].AIFS;
                    }
                    else
                    {
                        extendedIfsDelay = dot11->sifs +
                    PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        MAC_PHY_LOWEST_MANDATORY_RATE_TYPE,
                        DOT11_SHORT_CTRL_FRAME_SIZE) +
                        dot11->ACs[dot11->currentACIndex].AIFS;
                    }
/*********HT END****************************************************/                

                   MacDot11StationStartTimer(
                    node, dot11, (extraWait + extendedIfsDelay));
//--------------------DOT11e-End-Updates---------------------------------//

                } else {
                    // for dot11e if access category is not used, say for
                    // management frames resume usual operation;

                    MacDot11StationStartTimer(
                        node, dot11, (extraWait + dot11->extendedIfsDelay));
                }
            } else {
                   MacDot11StationStartTimer(
                       node,dot11, (extraWait + dot11->extendedIfsDelay));
            }

        }
        else {
//---------------------DOT11e--Updates------------------------------------//
            // for DOT11e set with AIFS...
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                if (dot11->currentACIndex != DOT11e_INVALID_AC)
                {
                    if (dot11->isHTEnable)
                    {
                        clocktype timerDelay = 0;
                        if (MacDot11StationHasInstantFrameToSend(dot11)
                            || dot11->beaconSet)
                        {
                            timerDelay = extraWait + dot11->txDifs;
                        }
                        else
                        {
                            timerDelay = extraWait
                                + dot11->ACs[dot11->currentACIndex].AIFS
                                - dot11->delayUntilSignalAirborn;
                        }
                    MacDot11StationStartTimer(
                            node, dot11, timerDelay);
                    }
                    else
                    {
                        MacDot11StationStartTimer(
                            node, dot11,
                            (extraWait
                                + dot11->ACs[dot11->currentACIndex].AIFS
                                - dot11->delayUntilSignalAirborn));
                    }
                    return;
                }
                // if access category is not used, say for
                // management frames use difs time;
            }
//--------------------DOT11e-End-Updates---------------------------------//

            MacDot11StationStartTimer(
                node, dot11, (extraWait + dot11->txDifs));
        }
    }

}// MacDot11AttemptToGoIntoWaitForDifsOrEifsState

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAttemptToInitiateNavPeriod
//  PURPOSE:     Set NAV for a BSS station so that the station waits
//               for the beacon.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if NAV has been set.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11StationAttemptToInitiateNavPeriod(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL haveSetNAV = FALSE;
    clocktype newNAV;
    clocktype currentTime;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE
        || MacDot11IsTransmittingState(dot11->state)
        || MacDot11IsWaitingForResponseState(dot11->state))
    {
        return FALSE;
    }

    switch (dot11->state) {
        case DOT11_S_IDLE:
        case DOT11_S_WFNAV:
        case DOT11_S_WF_DIFS_OR_EIFS:
        case DOT11_S_BO:
        case DOT11_S_WFJOIN:
        {
            currentTime = node->getNodeTime();

            // Protect against previous missed beacons
            while (dot11->beaconTime + dot11->cfpMaxDuration < currentTime)
            {
                dot11->beaconTime += dot11->beaconInterval;
            }

            newNAV = dot11->beaconTime + EPSILON_DELAY;

            if (newNAV > dot11->NAV) {
                if (dot11->state == DOT11_S_BO) {
                        dot11->BO = 0;
                }
                dot11->NAV = newNAV;
                MacDot11StationStartTimer(node, dot11, newNAV - currentTime);
                MacDot11StationSetState(node, dot11, DOT11_S_WFNAV);
            }
            dot11->beaconIsDue = FALSE;
            haveSetNAV = TRUE;

            break;
        }

        default:
            // In either transmit or wait-for state
            break;
    } //switch

    return haveSetNAV;

}// MacDot11StationAttemptToInitiateNavPeriod


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationPhyStatusIsNowIdleStartSendTimers
//  PURPOSE:     Start send timer since PHY indicates idle state.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationPhyStatusIsNowIdleStartSendTimers(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->waitForProbeDelay)
    {
        MacDot11StationCancelTimer(node, dot11);
        dot11->waitForProbeDelay = FALSE;
    }

    switch (dot11->state) {
        case DOT11_S_IDLE:
        case DOT11_S_HEADER_CHECK_MODE:
        {
            if (dot11->beaconIsDue
                && MacDot11StationCanHandleDueBeacon(node, dot11)) {
                return;
            }
            else if (!MacDot11StationHasFrameToSend(dot11) ) {
                if (dot11->stationAssociated && (!MacDot11IsAp(dot11)) &&
                    //(dot11->associatedAP->measInfo->rssi <
                    (dot11->associatedAP->rssMean <
                    dot11->thresholdSignalStrength)&&
                    dot11->ScanStarted == FALSE)
                {
                    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                    MacDot11ManagementStartJoin(node, dot11);
                }
                else
                {
                    //check if we have packet to send
                    MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);

                }

                // Nothing to send. Stay Idle.
            }
            else {
                // Start up sending process again.
                // Note: uses current Backoff delay.
//--------------------HCCA-Updates Start---------------------------------//
                if (!MacDot11HcAttempToStartCap(node,dot11)){
                    MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(
                        node, dot11);
                }
//--------------------HCCA-Updates End-----------------------------------//
            }
            break;
        }
        case DOT11_CFP_START:
        {
            MacDot11CfpPhyStatusIsNowIdleStartSendTimers(node, dot11);
            break;
        }
//--------------------HCCA-Updates Start---------------------------------//
        case DOT11e_CAP_START:
        {
            MacDot11eCfpPhyStatusIsNowIdleStartSendTimers(node,dot11);
            break;
        }
//--------------------HCCA-Updates End-----------------------------------//
        // In these states, we don't care about the state of the phy.
        case DOT11_S_WFCTS:
        // DOT11_S_WFDATA:
        case DOT11_S_WFDATA:
        // DOT11_S_WFACK:
        case DOT11_S_WFACK:
        // DOT11_S_WFJOIN
        case DOT11_S_WFJOIN:
       // DOT11_S_WFJOIN
        case DOT11_S_WFMANAGEMENT:

        // These states can't happen.
        // Currently Disabled
        case DOT11_S_NAV_RTS_CHECK_MODE:
        // default:

        case DOT11_S_WFNAV:
//---------------------------Power-Save-Mode-Updates---------------------//
        case DOT11_S_WFIBSSBEACON:
        case DOT11_S_WFIBSSJITTER:
        case DOT11_S_WFPSPOLL_ACK:
//---------------------------Power-Save-Mode-End-Updates-----------------//
        // default:
            break;

        case DOT11_S_WF_DIFS_OR_EIFS:
        // default:

        case DOT11_S_BO:
        // default:

        case DOT11_S_WFBEACON:
        // default:

        default:
            ERROR_ReportError("MacDot11StationPhyStatusIsNowIdleStartSendTimers: "
                "Invalid state encountered.\n");
            break;
    }//switch//

}//MacDot11StationPhyStatusIsNowIdleStartSendTimers//


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCancelSomeTimersBecauseChannelHasBecomeBusy
//  PURPOSE:     Cancel timers since the channel is busy.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationCancelSomeTimersBecauseChannelHasBecomeBusy(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->MayReceiveProbeResponce = TRUE;

    if (dot11->waitForProbeDelay)
    {
        MacDot11StationCancelTimer(node, dot11);
        dot11->waitForProbeDelay = FALSE;
    }

    switch (dot11->state)
    {
        case DOT11_S_WF_DIFS_OR_EIFS:
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);
            break;

        case DOT11_S_BO:
            MacDot11StationPauseBackoff(node, dot11);
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);
            break;

        case DOT11_S_WFNAV:
            // Timer no longer need timer for NAV (may need to
            // set the timer if the channel becomes busy again.
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);
            break;

        case DOT11_S_WFBEACON:
            if (MacDot11IsAp(dot11) ) {
                // Cancel the PIFS beacon timer
                MacDot11StationCancelTimer(node, dot11);
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            }
            break;
//---------------------------Power-Save-Mode-Updates---------------------//
        case DOT11_S_WFDATA:
        case DOT11_S_WFIBSSBEACON:
            {
                MacDot11StationCancelTimer(node, dot11);
                MacDot11StationSetState (node, dot11, DOT11_S_IDLE);
                break;
            }
//---------------------------Power-Save-Mode-End-Updates-----------------//
        case DOT11_CFP_START:
            MacDot11CfpCancelSomeTimersBecauseChannelHasBecomeBusy
                (node, dot11);
            break;
//--------------------HCCA-Updates Start---------------------------------//
        case DOT11e_WF_CAP_START:
                MacDot11StationCancelTimer(node, dot11);
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            break;

        case DOT11e_CAP_START:
            MacDot11eCfpCancelSomeTimersBecauseChannelHasBecomeBusy(
                node, dot11);
            break;
//--------------------HCCA-Updates End-----------------------------------//

        default:
            // Can't happen or ignore.
            break;
    }//switch//

}//MacDot11StationCancelSomeTimersBecauseChannelHasBecomeBusy//

static void MacDot11ResetTxRxVector(MAC_PHY_TxRxVector &txRxVector)
{
        txRxVector.format = MODE_HT_GF;
        txRxVector.chBwdth = CHBWDTH_20MHZ;
        txRxVector.length = 0;
        txRxVector.sounding = FALSE;
        txRxVector.containAMPDU = FALSE;
        txRxVector.gi = GI_LONG;
        txRxVector.mcs = 0;
        txRxVector.stbc = 0;
        txRxVector.numEss = 0;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetDataRateEntry.
//  PURPOSE:     Returns the data rate entry for given destAddr.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      DOT11_DataRateEntry Pointer to Entry for given destAddr.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
DOT11_DataRateEntry* MacDot11StationGetDataRateEntry(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    DOT11_DataRateEntry* entry = dot11->dataRateHead;
    DOT11_DataRateEntry* prev = NULL;

    if (!entry) {
        entry = (DOT11_DataRateEntry*)
                MEM_malloc(sizeof(DOT11_DataRateEntry));
/*********HT START*************************************************/
        memset(entry,0,sizeof(DOT11_DataRateEntry));
        MacDot11ResetTxRxVector(entry->txVector);
/*********HT END*************************************************/

        entry->ipAddress = destAddr;

        if (destAddr != ANY_MAC802) {
            entry->dataRateType = dot11->lowestDataRateType;
/*********HT START*************************************************/
            entry->txVector = dot11->txVectorForLowestDataRate;

            // Set the mcs index as per what received.
            // entry->txVector.mcs = dot11->mcsIndexToUse;
/*********HT END*************************************************/
        }
        else {
            entry->dataRateType = dot11->highestDataRateTypeForBC;
/*********HT START*************************************************/
            entry->txVector = dot11->txVectorForBC;

            // entry->txVector.mcs = dot11->mcsIndexToUse;
/*********HT END*************************************************/
        }
        entry->firstTxAtNewRate = FALSE;
        entry->numAcksFailed = 0;
        entry->numAcksInSuccess = 0;
        entry->dataRateTimer = node->getNodeTime() + dot11->rateAdjustmentTime;
        entry->next = NULL;
        dot11->dataRateHead = entry;

        return entry;
    }

    while (entry) {
        if (entry->ipAddress == destAddr) {

            return entry;
        }
        else {

            prev = entry;
            entry = entry->next;
        }
    }

    entry = (DOT11_DataRateEntry*)
            MEM_malloc(sizeof(DOT11_DataRateEntry));

/*********HT START*************************************************/
        memset(entry,0,sizeof(DOT11_DataRateEntry));
        MacDot11ResetTxRxVector(entry->txVector);
/*********HT END*************************************************/

    entry->ipAddress = destAddr;

    if (destAddr != ANY_MAC802) {
        entry->dataRateType = dot11->lowestDataRateType;
/*********HT START*************************************************/
            entry->txVector = dot11->txVectorForLowestDataRate;

            // Set the mcs index as per what received.
            // entry->txVector.mcs = dot11->mcsIndexToUse;
/*********HT END*************************************************/
    }
    else {
        entry->dataRateType = dot11->highestDataRateTypeForBC;
/*********HT START*************************************************/
            entry->txVector = dot11->txVectorForBC;

            // entry->txVector.mcs = dot11->mcsIndexToUse;
/*********HT END*************************************************/
    }

    entry->firstTxAtNewRate = FALSE;
    entry->numAcksFailed = 0;
    entry->numAcksInSuccess = 0;
    entry->dataRateTimer = node->getNodeTime() + dot11->rateAdjustmentTime;
    entry->next = NULL;

    prev->next = entry;

    return entry;

}// MacDot11StationGetDataRateEntry


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAdjustDataRateForNewOutgoingPacket
//  PURPOSE:     When a packet is dequeued to the local buffer,
//               check for and adjust data rate.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationAdjustDataRateForNewOutgoingPacket(
    Node* node,
    MacDataDot11* dot11)
{
    if ((dot11->dataRateInfo->ipAddress != ANY_MAC802 &&
          dot11->dataRateInfo->dataRateTimer < node->getNodeTime()) ||
         (dot11->dataRateInfo->numAcksInSuccess ==
          DOT11_NUM_ACKS_FOR_RATE_INCREASE) )
    {
        int newDataRate 
            = MIN(dot11->dataRateInfo->dataRateType + 1,
                dot11->highestDataRateType);
        if (dot11->dataRateInfo->dataRateType
            != dot11->highestDataRateType)
        {
            dot11->dataRateInfo->firstTxAtNewRate = TRUE;
            dot11->dataRateInfo->dataRateType = newDataRate;
        }
        dot11->dataRateInfo->numAcksFailed = 0;
        dot11->dataRateInfo->numAcksInSuccess = 0;
        dot11->dataRateInfo->dataRateTimer =
            node->getNodeTime() + dot11->rateAdjustmentTime;
//*********HT START*************************************************/
        if (dot11->isHTEnable) {
            MacDot11nAdjMscForOutgoingPacket(
                node, dot11, dot11->dataRateInfo);
        }
/*********HT END*************************************************/
    }

}// MacDot11StationAdjustDataRateForNewOutgoingPacket


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAdjustDataRateForResponseTimeout
//  PURPOSE:     When a RTS or CTS timeout occurs, check and
//               adjust values related to data rate.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationAdjustDataRateForResponseTimeout(
    Node* node,
    MacDataDot11* dot11)
{
    if ((dot11->dataRateInfo->numAcksFailed ==
         DOT11_NUM_ACKS_FOR_RATE_DECREASE) ||
        (dot11->dataRateInfo->numAcksFailed == 1 &&
         dot11->dataRateInfo->firstTxAtNewRate))
    {
        dot11->dataRateInfo->dataRateType =
            MAX(dot11->dataRateInfo->dataRateType - 1,
                dot11->lowestDataRateType);

        dot11->dataRateInfo->numAcksFailed = 0;
        dot11->dataRateInfo->numAcksInSuccess = 0;
        dot11->dataRateInfo->firstTxAtNewRate = FALSE;
        dot11->dataRateInfo->dataRateTimer =
            node->getNodeTime() + dot11->rateAdjustmentTime;
        
        if (dot11->isHTEnable) {
            MacDot11nAdjMscForResponseTimeout(
                node, dot11, dot11->dataRateInfo);
        }
    }

}// MacDot11StationAdjustDataRateForResponseTimeout

//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11AttachNodeAtStation
//  PURPOSE:     Add node information at Station
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address desNodeAddress
//                  destination node address
//  RETURN:      pointer of DOT11_AttachedNodeList
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
DOT11_AttachedNodeList* MacDot11AttachNodeAtStation(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address desNodeAddress)
{
    DOT11_AttachedNodeList* tempAttachNodeList;
    Queue*      queue = NULL;
    tempAttachNodeList = (DOT11_AttachedNodeList*)
        MEM_malloc(sizeof(DOT11_AttachedNodeList));
    ERROR_Assert(tempAttachNodeList != NULL,
        "Can not allocate memory\n");
    memset(tempAttachNodeList, 0, sizeof(DOT11_AttachedNodeList));

    tempAttachNodeList->nodeMacAddr = desNodeAddress;
    tempAttachNodeList->atimFramesSend = FALSE;
    tempAttachNodeList->atimFramesACKReceived = FALSE;
    tempAttachNodeList->noOfBufferedUnicastPacket = 0;
    tempAttachNodeList->next = NULL;
    // Setup the Unicast queue here
    queue = new Queue; // "FIFO"
    ERROR_Assert(queue != NULL,\
    "STA: Unable to allocate memory for station queue");
    queue->SetupQueue(node, "FIFO", dot11->unicastQueueSize, 0, 0, 0, FALSE);
    tempAttachNodeList->queue = queue;
    return tempAttachNodeList;
}// end of MacDot11AttachNodeAtStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11ReceiveNodeAtStation
//  PURPOSE:     Add node information, after received the ATIM frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address desNodeAddress
//                  destination node address
//  RETURN:      pointer of DOT11_ReceivedATIMFrameList
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
DOT11_ReceivedATIMFrameList* MacDot11ReceiveNodeAtStation(
    MacDataDot11* dot11,
    Mac802Address desNodeAddress)
{
    DOT11_ReceivedATIMFrameList* tempReceiveNodeList;
    tempReceiveNodeList = (DOT11_ReceivedATIMFrameList*)
        MEM_malloc(sizeof(DOT11_ReceivedATIMFrameList));
    ERROR_Assert(tempReceiveNodeList != NULL,
        "Can not allocate memory\n");
    memset(tempReceiveNodeList, 0, sizeof(DOT11_ReceivedATIMFrameList));

    tempReceiveNodeList->nodeMacAddr = desNodeAddress;
    tempReceiveNodeList->atimFrameReceive = TRUE;
    tempReceiveNodeList->next = NULL;
    return tempReceiveNodeList;
}// end of MacDot11ReceiveNodeAtStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11SetReceiveATIMFrame
//  PURPOSE:     Process Received ATIM frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address recvNodeAddress
//                  source node address
//  RETURN:      NONE
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
void MacDot11SetReceiveATIMFrame(
    MacDataDot11* dot11,
    Mac802Address recvNodeAddress)
{
    DOT11_ReceivedATIMFrameList* receiveNodeList = dot11->receiveNodeList;
    if (receiveNodeList == NULL){
        // Create and Add the node in to link list
        receiveNodeList = MacDot11ReceiveNodeAtStation(
            dot11,
            recvNodeAddress);
        dot11->receiveNodeList = receiveNodeList;
    }else{
        while (receiveNodeList != NULL){
            if (receiveNodeList->nodeMacAddr == recvNodeAddress){
               receiveNodeList->atimFrameReceive = TRUE;
               break;
            }else{
                if (receiveNodeList->next == NULL){
                    // Create and Add the node in to link list
                    receiveNodeList->next = MacDot11ReceiveNodeAtStation(
                        dot11,
                        recvNodeAddress);
                    receiveNodeList = receiveNodeList->next;
                    break;
                }
                else{
                    receiveNodeList = receiveNodeList->next;
                }
            }// end of if - else
        }// end of while
    }// end of if -else
    return;
}// end of MacDot11SetReceiveATIMFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11BufferPacket
//  PURPOSE:     insert the packet in to Unicast queue
//  PARAMETERS:  Queue* queue
//                  pointer to queue
//               Message* msg
//                  Pointer to message
//               clocktype currentTime
//                  current time of the node
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
BOOL MacDot11BufferPacket(
    Queue* queue,
    Message* msg,
    clocktype currentTime)
{
    BOOL    queueIsFull = FALSE;
    queue->insert(
        msg,
        NULL,
        &queueIsFull,
        currentTime);
    return queueIsFull;
}// end of MacDot11BufferPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationBufferedUnicastPacketInIBSSMode
//  PURPOSE:     insert the packet in to Unicast queue
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
void MacDot11StationBufferedUnicastPacketInIBSSMode(
    Node* node,
    MacDataDot11* dot11)
{
    // check queue is present for this node
    // if queue is not present then first setup it
    // buffered this packet

    DOT11_AttachedNodeList* attachNodeList = dot11->attachNodeList;
    if (attachNodeList == NULL){
        // Create and Add the node in to link list
        attachNodeList = MacDot11AttachNodeAtStation(
            node,
            dot11,
            dot11->ACs[dot11->currentACIndex].currentNextHopAddress);
        dot11->attachNodeList = attachNodeList;
    }
    else{
        while (attachNodeList != NULL){
            if (attachNodeList->nodeMacAddr ==
                 dot11->ACs[dot11->currentACIndex].currentNextHopAddress){
                // insert the packet
                break;
            }
            else{
                if (attachNodeList->next == NULL){
                    // Create and Add the node in to link list
                    attachNodeList->next = MacDot11AttachNodeAtStation(
                        node,
                        dot11,
                        dot11->ACs[dot11->currentACIndex].currentNextHopAddress);
                    attachNodeList = attachNodeList->next;
                    break;
                }
                else{
                    attachNodeList = attachNodeList->next;
                }
            }
        }// end of while
    }// end of if- else
    // insert the packet
    if (!MacDot11BufferPacket(
        attachNodeList->queue,
        dot11->ACs[dot11->currentACIndex].frameToSend,
        node->getNodeTime())){
        attachNodeList->noOfBufferedUnicastPacket++;
    }
    else{
        BOOL ret;
        int noOfPacketDrop = 0;
        ret = MacDot11PSModeDropPacket(
            node,
            dot11,
            attachNodeList->queue,
            dot11->ACs[dot11->currentACIndex].frameToSend,
            &noOfPacketDrop);
        attachNodeList->noOfBufferedUnicastPacket -= noOfPacketDrop;
        if (ret == TRUE){
            attachNodeList->noOfBufferedUnicastPacket++;
        }
    }
    return;
}// end of MacDot11StationBufferedUnicastPacketInIBSSMode

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetCurrentMessageVariables
//  PURPOSE:     Set variables related to single message buffer at
//               the Dot11 interface.
//
//  PARAMETERS:  MacDataDot11* const dot11
//                  Pointer to dot11 structure.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationSetCurrentMessageVariables(
    Node* node, MacDataDot11* const dot11)
{

//    int interfaceIndex = dot11->myMacData->interfaceIndex;
    dot11->ipSourceAddr = dot11->selfAddr;
    dot11->ipDestAddr = dot11->currentNextHopAddress;

    // For an MAP, ensure that SA, DA are set appropriately

    if (dot11->isMP)
    {
        if (dot11->txFrameInfo != NULL)
        {
            dot11->ipSourceAddr = dot11->txFrameInfo->SA;
            dot11->ipDestAddr = dot11->txFrameInfo->DA;
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
}// MacDot11StationSetCurrentMessageVariables
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetMsgPriority
//  PURPOSE:     Set variables related to single message buffer at
//               the Dot11 interface.
//
//  PARAMETERS:  MacDataDot11* const dot11
//                  Pointer to dot11 structure.
//               Message* msg
//
//  RETURN:      TosType
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
TosType MacDot11StationGetMsgPriority(
    Node* node, MacDataDot11* const dot11, Message* msg)
{
    IpHeaderType* ipHeader;
    TosType priority = 0;

    // Note: Only IP4/IPv6 and MPLS assumed here.
    if (dot11->myMacData->mplsVar == NULL)  {
        // Treat as an IP message
        ipHeader =
            (IpHeaderType*)MESSAGE_ReturnPacket(msg);
    }
#ifdef ENTERPRISE_LIB
    else {
        // Treat this packet as an MPLS message
        char *aHeader;
        aHeader = (MESSAGE_ReturnPacket(msg)
            + sizeof(Mpls_Shim_LabelStackEntry));
        ipHeader = (IpHeaderType*)aHeader;
    }
#endif // ENTERPRISE_LIB

    if (IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) == IPVERSION4)
    {

        // Get the Precedence value from IP header TOS type.
        // Shift ECN and Priority then get the precedence
        priority = (IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len) >> (2 + 3));
    }

    else
    {

        // Get the Precedence value from IPv6 header
        ip6_hdr* ipv6Header = NULL;

        ipv6Header = (ip6_hdr* ) MESSAGE_ReturnPacket(msg);

        priority = ip6_hdrGetClass(ipv6Header->ipv6HdrVcf) >> 5;
    }


    return priority;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CalculatePriority
//  PURPOSE:     Set variables related to single message buffer at
//               the Dot11 interface.
//
//  PARAMETERS:  MacDataDot11* const dot11
//                  Pointer to dot11 structure.
//               Message* msg
//
//  RETURN:      TosType
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int MacDot11CalculatePriority(int priority)
{
    if (priority == 0)
    {
        priority =0;
    }
    if (priority ==2 || priority==1)
    {
        priority = 1;
    }
    if (priority <=5 && priority>=3)
    {
        priority = 3;
    }
    if (priority ==7 || priority==6)
    {
        priority = 6;
    }
    return priority;
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11IsDestinationQoSEnable
//  PURPOSE:     Move a packet into the right mode and in right AC
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address NextHopAddress
//                  Next hop adress of the peeked packet
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static BOOL MacDot11IsDestinationQosEnable(Node* node,
                                     MacDataDot11* dot11,
                                     Mac802Address tempNextHopAddress)
    {
       BOOL sendQOSFrame = FALSE;
        if (MacDot11IsAp(dot11))
        {
            // Get the type of Associated STA
            // Check whether it is QSTA , if yes send frame with
            // dot11e frame header else send frame with dot11 frame
            // header

            DOT11_ApStationListItem* stationItem =
            MacDot11ApStationListGetItemWithGivenAddress(
                node,
                dot11,
                tempNextHopAddress);

            // It may be the case where QAP's QAP->AP/QAP are in Ad hoc
            // mode thus assocition response cannot be retrieved.

            if (stationItem)
            {
                if (MacDot11IsQoSEnabled(node, dot11) &&
                    stationItem->data->flags)
                {
                    sendQOSFrame = TRUE;
                    return(sendQOSFrame);
                }
                else
                {
                    sendQOSFrame = FALSE;
                    return(sendQOSFrame);

                }
            }
            return(sendQOSFrame);

         } else {
            // The station is QoS enabled and it is associated with
            // QAP then send the packet with dot11e frame header
            // else send with dot11 frame header.

            if (MacDot11IsQoSEnabled(node, dot11) &&
            MacDot11IsAssociatedWithQAP(node, dot11))
            {
                sendQOSFrame = TRUE;
                return(sendQOSFrame);
            }
            else
            {
                if (MacDot11IsQoSEnabled(node, dot11)&&
                    MacDot11IsIBSSStation(dot11))
                {
                    sendQOSFrame = TRUE;
                    return(sendQOSFrame);
                }
                else
                {
                    sendQOSFrame = FALSE;
                    return(sendQOSFrame);
                }

            }
        }
    }
//--------------------------------------------------------------------------
//  NAME:        MacDot11OtherAcsHaveNoData
//  PURPOSE:     Check Other Acs has packet or not
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int AcIndex
//                  current acIndex for which packet is fetched
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static//inline//
BOOL MacDot11OtherAcsHaveData(Node* node,
    MacDataDot11* dot11,int AcIndex)
{
    int NoPacket = 0;
    BOOL Flag = TRUE;
    for (int CurrentAcCount=0;CurrentAcCount<4;CurrentAcCount++)
    {
        if (CurrentAcCount != AcIndex &&
            dot11->ACs[CurrentAcCount].frameInfo == NULL)
            NoPacket++;
    }
    if (NoPacket == 3)
    {
        Flag = FALSE;
    }
    return(Flag);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11ClassifyPacket
//  PURPOSE:     Move a packet into the right mode and in right AC
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Pointer to the message
//               TosType Currpriority
//                  current priority for which the message has been fetched
//               Mac802Address NextHopAddress
//                  Next hop adress of the peeked packet
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

static//inline//
BOOL MacDot11ClassifyPacket(Node* node,
    MacDataDot11* dot11,Message* msg,TosType queuePriority,
    Mac802Address NextHopAddress,
    BOOL SendQosFrame)
{
    BOOL dequeued = FALSE;
    int networkType;
    int interfaceIndex = dot11->myMacData->interfaceIndex;

//---------------------DOT11e--Updates------------------------------------//
//--------------------HCCA-Updates Start---------------------------------//
    Message* tempMsg = NULL;
    Mac802Address tempNextHopAddress = NextHopAddress ;
    Message* newMsg;
    TosType priority;
    int acIndex;

    BOOL isACHasMsgForCurrentAccessCategory = FALSE;
    BOOL isEDCAPacket = FALSE;
    BOOL isHCCAPacket = FALSE;
    int highestPriority = 7;
    if (tempNextHopAddress == ANY_MAC802 &&
        (MacDot11IsAp(dot11)) &&
        MacDot11IsQoSEnabled(node, dot11))
    {
        priority = MAC_GetPacketsPriority(node,msg);
        acIndex = MacDot11ReturnAccessCategory(priority) ;
    }
    else
    {
        if (SendQosFrame)
        {
              priority = MAC_GetPacketsPriority(node,msg);
              acIndex = MacDot11ReturnAccessCategory(priority) ;
        }
        else
        {
            priority = 0;
            acIndex = DOT11e_AC_BK;
        }
    }
    //Find if this a HCCA packet
    if (dot11->isHCCAEnable &&
        (MacDot11IsHC(dot11) || dot11->associatedWithHC) &&
        (priority == highestPriority ||
         priority >= DOT11e_HCCA_LOWEST_PRIORITY )) {

        //check if it is a broadcast packet
        if (tempNextHopAddress == ANY_MAC802) {

            //It is a broadcast packet
            //tempNextHopAddress = ANY_DEST;
            isHCCAPacket = FALSE;
        }
        else {
            isHCCAPacket = TRUE;
        }
    }

    //If it is HC check if the STA supports HCCA
    if (MacDot11IsHC(dot11) && isHCCAPacket) {
        DOT11_ApStationListItem* stationItem =
        MacDot11ApStationListGetItemWithGivenAddress(
            node,
            dot11,
            tempNextHopAddress);
        if (stationItem) {
            if (stationItem->data->flags) {
                isHCCAPacket = TRUE;
            }
            else
            {
                isHCCAPacket = FALSE;
            }
        }
        else {
            isHCCAPacket = FALSE;
        }
    }

    // Find if this an EDCA packet or DCF packet
    if (dot11->ACs[acIndex].frameToSend!= NULL) {
        isACHasMsgForCurrentAccessCategory = TRUE;
    } else {
        isACHasMsgForCurrentAccessCategory = FALSE;
    }

    // If it is not a HCCA packet then it is an EDCA packet
    // check if the AC has space and the state is correct to dequeue
    if (!isHCCAPacket && !isACHasMsgForCurrentAccessCategory){
        isEDCAPacket = TRUE;
    }

    //It is a HCCA packet, dequeue it and put in TS Buffer
    if (isHCCAPacket){
        if (!MacDot11eIsTSBuffreFull(node,dot11,priority)) {
            dequeued =MAC_OutputQueueDequeuePacketForAPriority(
                            node,
                            interfaceIndex,
                            queuePriority,
                            &(tempMsg),
                            &(tempNextHopAddress),
                            &networkType);

            if (dequeued) {
                if (MacDot11IsBssStation(dot11)) {

                    if (!MAC_IsBroadcastMac802Address(&tempNextHopAddress) &&
                        tempNextHopAddress != dot11->bssAddr){
                        Message* msg = MESSAGE_Duplicate(node, tempMsg);

                        MacDot11StationInformNetworkOfPktDrop(node,
                                                          dot11,
                                                          msg);
                    }
                }
                MacDot11eStationMoveAPacketFromTheNetworkToTheTSsBuffer
                (node,
                dot11,
                (unsigned)priority,
                tempMsg,
                tempNextHopAddress);

                dot11->pktsToSend++;

                //It can potentially take more messages
                return TRUE;
            }
            else {
                //unable to dequeue
                return FALSE;
            }
        }
        else {
            //Buffer is full cannot take in more packets of this priority
            return FALSE;
        }
    }

    // This is EDCA packet dequeue it to the corresponding AC
    if (isEDCAPacket == TRUE) {
        if ((!isACHasMsgForCurrentAccessCategory )) {

            dequeued =MAC_OutputQueueDequeuePacketForAPriority(
                                node,
                                interfaceIndex,
                                queuePriority,
                                        &(tempMsg),
                                        &(tempNextHopAddress),
                                &(dot11->ACs[acIndex].networkType));

            if (dequeued) {
                dot11->ipNextHopAddr = tempNextHopAddress;
                dot11->ipDestAddr = tempNextHopAddress;
                if (MacDot11IsBssStation(dot11)) {
                    if (!MAC_IsBroadcastMac802Address(&tempNextHopAddress) &&
                        tempNextHopAddress != dot11->bssAddr){
                        Message* msg = MESSAGE_Duplicate(node, tempMsg);

                        MacDot11StationInformNetworkOfPktDrop(node,
                                                          dot11,
                                                          msg);
                    }
                   tempNextHopAddress = dot11->bssAddr;
            }

            dot11->ACs[acIndex].frameInfo =
                (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
            memset(dot11->ACs[acIndex].frameInfo, 0, sizeof(DOT11_FrameInfo));
            dot11->ACs[acIndex].frameInfo->msg = tempMsg;
            dot11->ACs[acIndex].frameToSend = tempMsg;
            dot11->ACs[acIndex].frameInfo->RA = tempNextHopAddress;
            dot11->ACs[acIndex].frameInfo->TA = dot11->selfAddr;
            dot11->ACs[acIndex].frameInfo->SA = dot11->selfAddr;
            dot11->ACs[acIndex].frameInfo->DA = dot11->ipDestAddr;
            dot11->ACs[acIndex].frameInfo->insertTime =
                                            node->getNodeTime();

            if (SendQosFrame)
            {
                dot11->ACs[acIndex].frameInfo->frameType = DOT11_QOS_DATA;
                dot11->ACs[acIndex].priority = priority;
                newMsg = dot11->ACs[acIndex].frameInfo->msg;
                MESSAGE_AddHeader(node,
                        newMsg,
                        sizeof(DOT11e_FrameHdr),
                        TRACE_DOT11);


                MacDot11eStationSetFieldsInDataFrameHdr(dot11,
                                                    (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(newMsg),
                                                    dot11->ACs[acIndex].frameInfo->RA,
                                                    dot11->ACs[acIndex].frameInfo->frameType,
                                                    acIndex);

                if (((!((MacDot11StationPhyStatus(node, dot11) == 
                    PHY_IDLE) &&
                    (MacDot11IsPhyIdle(node, dot11)))))||
                    MacDot11StationWaitForNAV(node, dot11)||
                    MacDot11OtherAcsHaveData(node,dot11,acIndex))
                {

                    MacDot11StationSetBackoffIfZero(node,dot11,acIndex);
                }

             }
             else
             {
                //for DCF data processing in AC[BE]
                dot11->ACs[acIndex].frameInfo->frameType = DOT11_DATA;
                newMsg = dot11->ACs[acIndex].frameInfo->msg;

                MESSAGE_AddHeader(node,
                        newMsg,
                        sizeof(DOT11_FrameHdr),
                        TRACE_DOT11);


                MacDot11StationSetFieldsInDataFrameHdr(
                    dot11,
                    (DOT11_FrameHdr*) MESSAGE_ReturnPacket(newMsg),
                    dot11->ACs[acIndex].frameInfo->RA,
                    dot11->ACs[acIndex].frameInfo->frameType);

                    if (!((MacDot11StationPhyStatus(node, dot11) == PHY_IDLE) &&
                    (MacDot11IsPhyIdle(node, dot11)))||
                        MacDot11StationWaitForNAV(node, dot11))
                    {

                        MacDot11StationSetBackoffIfZero(node,dot11,acIndex);
                    }
                }


                dot11->ACs[acIndex].frameInfo->msg = newMsg;

                dot11->pktsToSend++;

                //Reset AC veriables
                dot11->ACs[acIndex].priority = priority;
                dot11->ACs[acIndex].totalNoOfthisTypeFrameQueued++;
                dot11->ACs[acIndex].QSRC = 0;
                dot11->ACs[acIndex].QLRC = 0;
                if (dot11->ACs[acIndex].CW == 0) {
                    dot11->ACs[acIndex].CW = dot11->ACs[acIndex].cwMin;
                }


                if (DEBUG_QA)
                {
                    unsigned char* frame_type =
                        (unsigned char*) MESSAGE_ReturnPacket(dot11->ACs[acIndex].frameInfo->msg);
                    printf(" Node %u ac[ %d] used priority %d for frame %x\n",
                        node->nodeId,
                        acIndex,
                        dot11->ACs[acIndex].priority,*frame_type);
                }

                //Return true if the next packet could be a potentially HCCA packet
                if (dot11->isHCCAEnable &&
                    (MacDot11IsHC(dot11) || dot11->associatedWithHC) &&
                    (queuePriority == highestPriority ||
                    queuePriority >= DOT11e_HCCA_LOWEST_PRIORITY )){
                    return TRUE;
                }
            }
        }
        return FALSE; //No need to try to dequeue packet of same
                      //priority as we have a single packet buffer
    }
//--------------------HCCA-Updates End---------------------------------//
//--------------------DOT11e-End-Updates---------------------------------//
    return FALSE;
}//MacDot11ClassifyPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer
//  PURPOSE:     Move a packet from the network layer to the buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
BOOL MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL dequeued = FALSE;
    BOOL peek = FALSE;
    BOOL isACHasMsg = FALSE;
    int interfaceIndex = dot11->myMacData->interfaceIndex;
    Message* tempMsg = NULL;
    Mac802Address tempNextHopAddress;
    TosType priority = 0;
    int networkType = 0;
    BOOL SendQOSFrame = FALSE;

    if (dot11->isHTEnable)
    {
            MacDot11nClassifyPacket(node,
                                            dot11);

            BOOL isAChasMsg = MacDot11nIsACsHasMessage(dot11);
            if (dot11->state == DOT11_S_IDLE)
            {
                if (isAChasMsg && !dot11->beaconSet)
                {
                     //MacDot11nSetCurrentAC(node, dot11);
                     MacDot11eSetCurrentMessage(node, dot11);
                }
                else
                {
                        return FALSE;
                }
            }
            return TRUE;
    }
    else if (MacDot11IsQoSEnabled(node, dot11)) {

        // if Qos enabled then put the packet to proper AC or to correct Traffic stream
        //using Traffic Classifier

        // This is used to get the highest priority packets.
        // However, this is dependent on No. of Network Queues.
        // The best performance is achieved if there is 8 (0-7) queues.
        // If no of queues is less than the highestPriority(7) then
        // all the packtes of higher priorities will
        // pile up in the last queue, treated as maximum queue.

        // First get the heighest priority queue no, It holds all the
        // heighest priority packets specified in application.

        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        Scheduler *schedulerPtr =
                            ip->interfaceInfo[interfaceIndex]->scheduler;
        int highestPriority = schedulerPtr->numQueue() - 1;
        int currPriority = highestPriority;
        while (currPriority >= 0){
            tempMsg = NULL;
            peek = MAC_OutputQueueTopPacketForAPriority(
                                    node,
                                    interfaceIndex,
                                    (TosType) currPriority,
                                    &(tempMsg),
                                    &(tempNextHopAddress));
            if (peek == TRUE)
            {
                SendQOSFrame = MacDot11IsDestinationQosEnable(
                    node,
                    dot11,
                    tempNextHopAddress);
                if (tempNextHopAddress == ANY_MAC802 &&
                    dot11->stationType == DOT11_STA_BSS)
                {
                    SendQOSFrame = TRUE;
                }
                if (SendQOSFrame)
                {
                    if (!MacDot11ClassifyPacket(node,dot11,tempMsg,
                                (TosType) currPriority,
                                tempNextHopAddress,SendQOSFrame)){
                        currPriority--;
                    }
                }
                else{
                        MacDot11ClassifyPacket(node,
                                            dot11,
                                            tempMsg,
                                            (TosType)currPriority,
                                            tempNextHopAddress,
                                            SendQOSFrame);
                        break;

                 }
            }
            else
            {
                currPriority--;
            }
        }
    }
    else {
            ERROR_Assert(dot11->currentMessage == NULL,
            "MacDot11MoveAPacketFromTheNetworkLayerToTheLocalBuffer: "
            "A packet exists in the local buffer.\n");
            tempMsg = NULL;
           dequeued =
               MAC_OutputQueueTopPacket(
                                        node,
                                        interfaceIndex,
                                        &(tempMsg),
                                        &(tempNextHopAddress),
                                        &networkType,
                                        &priority);

       if (MacDot11IsAPSupportPSMode(dot11)||MacDot11IsIBSSStationSupportPSMode(dot11))
       {
           if (dequeued)
           {
               if (dot11->ACs[0].frameToSend == NULL)
               {
                   dot11->currentACIndex = 0;
                   dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
                   MAC_OutputQueueDequeuePacket(
                        node,
                        interfaceIndex,
                        &(dot11->ACs[dot11->currentACIndex].frameToSend),
                        &(dot11->ACs[dot11->currentACIndex].currentNextHopAddress),
                        &networkType, &priority);

                   MESSAGE_AddHeader(node,
                                    dot11->ACs[dot11->currentACIndex].frameToSend,
                                    sizeof(DOT11_FrameHdr),
                                    TRACE_DOT11);


                   MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                                (DOT11_FrameHdr*) MESSAGE_ReturnPacket(
                                                dot11->ACs[dot11->currentACIndex].frameToSend),
                                                dot11->ACs[dot11->currentACIndex].currentNextHopAddress,
                                                DOT11_DATA);

               }
               else
               {
                   dequeued = FALSE;
               }
           }
       }
       else
       {
           if (dequeued)
           {
               MacDot11ClassifyPacket(node,dot11,tempMsg,
                                priority,tempNextHopAddress,FALSE);

            }
        }
   }

//---------------------------Power-Save-Mode-Updates---------------------//

if (MacDot11IsAPSupportPSMode(dot11)&& dequeued)
{
    while (dequeued)
    {
        if (dot11->ACs[dot11->currentACIndex].currentNextHopAddress == ANY_MAC802)
        {
            if (MacDOT11APIsAnySTAInPSMode(dot11))
            {
                // Buffer the broadcast packet in to broadcast queue At AP
                if (!MacDot11BufferPacket(
                    dot11->broadcastQueue,
                    dot11->ACs[dot11->currentACIndex].frameToSend,
                    node->getNodeTime())){
                    dot11->noOfBufferedBroadcastPacket++;
                }
                else{
                    BOOL ret;
                    int noOfPacketDrop = 0;
                    MESSAGE_RemoveHeader(node,
                                 dot11->ACs[dot11->currentACIndex].frameToSend,
                                 sizeof(DOT11_FrameHdr),
                                 TRACE_DOT11);


                    ret = MacDot11PSModeDropPacket(
                        node,
                        dot11,
                        dot11->broadcastQueue,
                        dot11->ACs[dot11->currentACIndex].frameToSend,
                        &noOfPacketDrop);

                    dot11->noOfBufferedBroadcastPacket -= noOfPacketDrop;

                    if (ret == TRUE){
                        dot11->noOfBufferedBroadcastPacket++;
                    }
                }// end of else
                dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                dot11->ACs[dot11->currentACIndex].BO = 0;
                dequeued = FALSE;
                dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
            }
            else
            {

                dot11->ipNextHopAddr =
                    dot11->ACs[dot11->currentACIndex].currentNextHopAddress;
                if (MacDot11IsBssStation(dot11)) {
                    if (!MAC_IsBroadcastMac802Address(&dot11->ACs[
                            dot11->currentACIndex].currentNextHopAddress) &&
                        dot11->ACs[
                            dot11->currentACIndex].currentNextHopAddress
                        != dot11->bssAddr){
                        Message* msg = MESSAGE_Duplicate(node,
                            dot11->ACs[dot11->currentACIndex].frameToSend);

                        MacDot11StationInformNetworkOfPktDrop(node,
                                                          dot11,
                                                          msg);
                    }
                   dot11->ACs[dot11->currentACIndex].currentNextHopAddress =
                       dot11->bssAddr;
                    }

                dot11->ACs[dot11->currentACIndex].frameInfo =
                    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
                memset(dot11->ACs[dot11->currentACIndex].frameInfo,0,
                                                  sizeof(DOT11_FrameInfo));
                dot11->ACs[dot11->currentACIndex].frameInfo->msg =
                    dot11->ACs[dot11->currentACIndex].frameToSend;
                dot11->ACs[dot11->currentACIndex].frameInfo->RA =
                    dot11->ACs[dot11->currentACIndex].currentNextHopAddress;
                dot11->ACs[dot11->currentACIndex].frameInfo->TA =
                                                         dot11->selfAddr;
                dot11->ACs[dot11->currentACIndex].frameInfo->SA =
                                                        dot11->selfAddr;
                dot11->ACs[dot11->currentACIndex].frameInfo->DA =
                          dot11->ACs[dot11->currentACIndex].frameInfo->RA;
                dot11->ACs[dot11->currentACIndex].frameInfo->insertTime =
                                                          node->getNodeTime();
                dot11->ACs[dot11->currentACIndex].frameInfo->frameType =
                                                                 DOT11_DATA;

                dot11->pktsToSend++;
                dot11->ACs[dot11->currentACIndex].priority = priority;
                dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameQueued++;
                dot11->ACs[dot11->currentACIndex].QSRC = 0;
                dot11->ACs[dot11->currentACIndex].QLRC = 0;
                    if (dot11->ACs[dot11->currentACIndex].CW == 0) {
                        dot11->ACs[dot11->currentACIndex].CW =
                            dot11->ACs[dot11->currentACIndex].cwMin;
                    }

                    MacDot11StationSetBackoffIfZero(
                        node,
                        dot11,
                        dot11->currentACIndex);

                break;
            }// end of if-else MacDOT11IsAnySTAInPSMode
        }
        else
        {
          if (MacDOT11APBufferPacketAtSTAQueue(
                node,
                dot11))
          {
          }
          else
          {

              dot11->ipNextHopAddr =
                  dot11->ACs[dot11->currentACIndex].currentNextHopAddress;
                if (MacDot11IsBssStation(dot11)) {
                    if (!MAC_IsBroadcastMac802Address(&dot11->ACs[
                            dot11->currentACIndex].currentNextHopAddress) &&
                        dot11->ACs[
                            dot11->currentACIndex].currentNextHopAddress
                        != dot11->bssAddr){
                        Message* msg = MESSAGE_Duplicate(node,
                            dot11->ACs[dot11->currentACIndex].frameToSend);

                        MacDot11StationInformNetworkOfPktDrop(node,
                                                          dot11,
                                                          msg);
                    }
                   dot11->ACs[dot11->currentACIndex].currentNextHopAddress =
                                                             dot11->bssAddr;
                    }

                dot11->ACs[dot11->currentACIndex].frameInfo =
                    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
                 memset(dot11->ACs[dot11->currentACIndex].frameInfo,0, sizeof(DOT11_FrameInfo));
                dot11->ACs[dot11->currentACIndex].frameInfo->msg =
                    dot11->ACs[dot11->currentACIndex].frameToSend;

                dot11->ACs[dot11->currentACIndex].frameInfo->RA =
                    dot11->ACs[dot11->currentACIndex].currentNextHopAddress;
                dot11->ACs[dot11->currentACIndex].frameInfo->TA =
                                                             dot11->selfAddr;
                dot11->ACs[dot11->currentACIndex].frameInfo->SA =
                                                           dot11->selfAddr;
                dot11->ACs[dot11->currentACIndex].frameInfo->DA =
                            dot11->ACs[dot11->currentACIndex].frameInfo->RA;
                dot11->ACs[dot11->currentACIndex].frameInfo->insertTime =
                                                           node->getNodeTime();
                dot11->ACs[dot11->currentACIndex].frameInfo->frameType =
                                                                 DOT11_DATA;

                dot11->pktsToSend++;
                dot11->ACs[dot11->currentACIndex].priority = priority;
                dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameQueued++;
                dot11->ACs[dot11->currentACIndex].QSRC = 0;
                dot11->ACs[dot11->currentACIndex].QLRC = 0;
                if (dot11->ACs[dot11->currentACIndex].CW == 0) {
                    dot11->ACs[dot11->currentACIndex].CW =
                        dot11->ACs[dot11->currentACIndex].cwMin;
                }

                MacDot11StationSetBackoffIfZero(
                    node,
                    dot11,
                    dot11->currentACIndex);
                //add this message to the corresponding AC

                break;
            }// end of if-else MacDOT11IsSTAInPSMode
        }// end of if-else dot11->currentNextHopAddress == ANY_MAC802

        dot11->currentACIndex = 0;
        dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
        dequeued = FALSE;
        dequeued =MAC_OutputQueueDequeuePacket(
                node,
                interfaceIndex,
                         &(dot11->ACs[dot11->currentACIndex].frameToSend),
                         &(dot11->ACs[dot11->currentACIndex].currentNextHopAddress),
                &networkType, &priority);
        if (dequeued)
        {
            //to be done
            MESSAGE_AddHeader(node,
                                    dot11->ACs[dot11->currentACIndex].frameToSend,
                                    sizeof(DOT11_FrameHdr),
                                    TRACE_DOT11);


             MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                                (DOT11_FrameHdr*) MESSAGE_ReturnPacket(
                                                dot11->ACs[dot11->currentACIndex].frameToSend),
                                                dot11->ACs[dot11->currentACIndex].currentNextHopAddress,
                                                DOT11_DATA);

        }

    }// end of infinite loop while 1
}// end of MacDOT11IsAP
if (MacDot11IsIBSSStationSupportPSMode(dot11)&& dequeued){
     while (dequeued)
    {
        if (dot11->ACs[dot11->currentACIndex].currentNextHopAddress == ANY_MAC802)
        {
            // Buffer the broadcast packet in to broadcast queue At STA
            if (!MacDot11BufferPacket(
                dot11->broadcastQueue,
                (dot11->ACs[dot11->currentACIndex].frameToSend),
                node->getNodeTime())){
                dot11->noOfBufferedBroadcastPacket++;
            }
            else{
                BOOL ret;
                int noOfPacketDrop = 0;

                MESSAGE_RemoveHeader(node,
                                 dot11->ACs[dot11->currentACIndex].frameToSend,
                                 sizeof(DOT11_FrameHdr),
                                 TRACE_DOT11);

                ret = MacDot11PSModeDropPacket(
                    node,
                    dot11,
                    dot11->broadcastQueue,
                    (dot11->ACs[dot11->currentACIndex].frameToSend),
                    &noOfPacketDrop);
                dot11->noOfBufferedBroadcastPacket -= noOfPacketDrop;
                if (ret == TRUE){
                    dot11->noOfBufferedBroadcastPacket++;
                }
            }
        }
        else
        {
            MacDot11StationBufferedUnicastPacketInIBSSMode(
                node,
                dot11);
        }// end of if-else dot11->currentNextHopAddress == ANY_MAC802
        dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
        dot11->ACs[dot11->currentACIndex].BO = 0;
        dequeued = FALSE;
        dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;

        dequeued =MAC_OutputQueueDequeuePacket(
                node,
                interfaceIndex,
                                                &(dot11->ACs[dot11->currentACIndex].frameToSend),
                                                &(dot11->ACs[dot11->currentACIndex].currentNextHopAddress),
                &networkType, &priority);
        if (dequeued)
        {
            //to be done
            MESSAGE_AddHeader(node,
                                    dot11->ACs[dot11->currentACIndex].frameToSend,
                                    sizeof(DOT11_FrameHdr),
                                    TRACE_DOT11);


              MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                                (DOT11_FrameHdr*) MESSAGE_ReturnPacket(
                                                dot11->ACs[dot11->currentACIndex].frameToSend),
                                                dot11->ACs[dot11->currentACIndex].currentNextHopAddress,
                                                DOT11_DATA);

        }
    }// end of infinite loop while 1
}// end of if isPowerSaveModeEnabledInAdHocMode


    // if Qos enabled then retrieve the packet using proper EDCA internal
    // contention algorithm to send a packet.
    isACHasMsg = MacDot11IsACsHasMessage(node,dot11);

    if (MacDot11IsQoSEnabled(node, dot11))
    {
        if (dot11->state == DOT11_S_IDLE)
        {
            dot11->currentMessage = NULL;
            // Now use EDCA-TOXP Scheduler to retrive the frame which wins the
            // the transmission opportunity.
            if (isACHasMsg == TRUE && !dot11->beaconSet)
                    MacDot11eSetCurrentMessage(node,dot11);
            else
                return FALSE;
        }
    }
    else
    {
        // Now use EDCA-TOXP Scheduler to retrive the frame which wins the
        // the transmission opportunity.
        if (isACHasMsg == TRUE && dot11->currentMessage == NULL)
            MacDot11SetCurrentMessage(node,dot11);
        else
            return FALSE;
    }
//--------------------DOT11e-End-Updates---------------------------------//
// If MP, relay or route the packet
    if (dot11->isMP && (dot11->currentMessage != NULL))
    {
        if (!(dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ &&
            dot11->dot11TxFrameInfo->frameType <= DOT11_DEAUTHENTICATION)) {

            MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
            dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
            dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
            dot11->ACs[dot11->currentACIndex].BO = 0;
            dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
            dot11->dot11TxFrameInfo = NULL;


            MESSAGE_RemoveHeader(node,
                             dot11->currentMessage,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);

            Dot11s_ReceiveNetworkLayerPacket(node, dot11,
                dot11->currentMessage,
                dot11->currentNextHopAddress,
                networkType,
                priority);

            return FALSE;
        }
    }
    return TRUE;
}// MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationMoveAPacketFromTheDataQueueToTheLocalBuffer
//  PURPOSE:     Move a packet from local data queue to the buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11StationMoveAPacketFromTheDataQueueToTheLocalBuffer(
    Node* node,
    MacDataDot11* dot11)
{
    ERROR_Assert(dot11->currentMessage == NULL,
        "MacDot11MoveAPacketFromTheDataQueueToTheLocalBuffer: "
        "A packet exists in the local buffer.\n");

    BOOL dequeued = FALSE;
//    int interfaceIndex = dot11->myMacData->interfaceIndex;

    dequeued =
        Dot11sDataQueue_DequeuePacket(node, dot11, &(dot11->txFrameInfo));


    if (dequeued == FALSE) {
        return FALSE;
    }
    ERROR_Assert(dot11->txFrameInfo->msg->headerProtocols[
                             dot11->txFrameInfo->msg->numberOfHeaders - 1]
                             == TRACE_DOT11,
                         "DOT11 header not present");

    dot11->currentMessage = dot11->txFrameInfo->msg;
    dot11->currentNextHopAddress = dot11->txFrameInfo->RA;
    dot11->ipNextHopAddr = dot11->currentNextHopAddress;
    dot11->currentACIndex = DOT11e_INVALID_AC;

    dot11->ipDestAddr = dot11->txFrameInfo->DA;
    dot11->ipSourceAddr = dot11->txFrameInfo->SA;
    dot11->dataRateInfo =
        MacDot11StationGetDataRateEntry(node, dot11,
        dot11->currentNextHopAddress);

    ERROR_Assert(
        dot11->dataRateInfo->ipAddress == dot11->currentNextHopAddress,
        "MacDot11StationMoveAPacketFromTheDataQueueToTheLocalBuffer: "
        "Address does not match");

    MacDot11StationAdjustDataRateForNewOutgoingPacket(node, dot11);

    return TRUE;

}// MacDot11StationMoveAPacketFromTheDataQueueToTheLocalBuffer


//--------------------------------------------------------------------------
//  NAME:        MacDot11StaMoveAPacketFromTheMgmtQueueToTheLocalBuffer
//  PURPOSE:     Move a packet from management queue to the buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11StaMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
    Node* node,
    MacDataDot11* dot11)
{
    int currentACIndex = 0;
    if (dot11->isQosEnabled)
        currentACIndex = 3;

    if (dot11->ACs[currentACIndex].frameToSend != NULL) {
        return FALSE;
    }

    BOOL dequeued = FALSE;
    dot11->managementFrameDequeued = FALSE;
    DOT11_FrameInfo* tempFrameInfo = NULL;
    dequeued =
        MacDot11MgmtQueue_PeekPacket(node, dot11, &(tempFrameInfo));

    if (dequeued == FALSE) {
        return FALSE;
    }


    //TBD: Statistics capture
    // dot11->pktsToSend++; will be interpreted as
    //      802.11MAC,Packets from network
    // There is no corresponding MGMT, packets to send.
    // Using managementPacketsSent is interpreted as
    //      Management packets sent to channel
    // and this frame could be dropped.

    if (tempFrameInfo!= NULL)
    {
        dot11->ipNextHopAddr = tempFrameInfo->RA;
        if (MacDot11IsBssStation(dot11)
            && dot11->bssAddr!= dot11->selfAddr) {
            tempFrameInfo->RA = dot11->bssAddr;
        }

        dot11->ACs[currentACIndex].frameInfo =
                                              tempFrameInfo;
        dot11->ACs[currentACIndex].frameToSend =
                                          tempFrameInfo->msg;
        dot11->ACs[currentACIndex].priority = 8;

        //Reset AC veriables
        dot11->ACs[currentACIndex].priority = 8;
        dot11->ACs[currentACIndex].totalNoOfthisTypeFrameQueued++;
        dot11->ACs[currentACIndex].QSRC = 0;
        dot11->ACs[currentACIndex].QLRC = 0;
        if (dot11->ACs[currentACIndex].CW == 0) {
            dot11->ACs[currentACIndex].CW =
                       dot11->ACs[currentACIndex].cwMin;
        }
        MacDot11StationSetBackoffIfZero(
            node,
            dot11,
            currentACIndex);
    }
    return TRUE;
} //MacDot11StaMoveAPacketFromTheMgmtQueueToTheLocalBuffer

//--------------------------------------------------------------------------
//  NAME:        MacDot11sStationMoveAPacketFromTheMgmtQueueToTheLocalBuffer
//  PURPOSE:     Move a packet from management queue to the buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11sStationMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
    Node* node,
    MacDataDot11* dot11)
{
    ERROR_Assert(dot11->currentMessage == NULL,
        "MacDot11MoveAPacketFromTheMgmtQueueToTheLocalBuffer: "
        "A packet exists in the local buffer.\n");

    BOOL dequeued = FALSE;

//    int interfaceIndex = dot11->myMacData->interfaceIndex;

    dequeued =
        Dot11sMgmtQueue_DequeuePacket(node, dot11, &(dot11->txFrameInfo));

    if (dequeued == FALSE) {
        return FALSE;
    }

    dot11->currentMessage = dot11->txFrameInfo->msg;
    dot11->currentNextHopAddress = dot11->txFrameInfo->RA;
    dot11->ipNextHopAddr = dot11->currentNextHopAddress;
     dot11->currentACIndex = DOT11e_INVALID_AC;

    // For a BSS station, change next hop address to be that of the AP
    if (MacDot11IsBssStation(dot11)) {
        dot11->currentNextHopAddress = dot11->bssAddr;
    }

    dot11->ipSourceAddr = dot11->txFrameInfo->SA;
    dot11->ipDestAddr = dot11->txFrameInfo->DA;

    dot11->dataRateInfo =
        MacDot11StationGetDataRateEntry(node, dot11,
        dot11->currentNextHopAddress);

    ERROR_Assert(
        dot11->dataRateInfo->ipAddress == dot11->currentNextHopAddress,
        "MacDot11sStationMoveAPacketFromTheMgmtQueueToTheLocalBuffer: "
        "Address does not match");

    MacDot11StationAdjustDataRateForNewOutgoingPacket(node, dot11);

    return TRUE;

} //MacDot11sStationMoveAPacketFromTheMgmtQueueToTheLocalBuffer

//---------------------------Power-Save-Mode-Updates---------------------//
//  NAME:        MacDot11DequeueUnicastBufferPacket.
//  PURPOSE:     Dequeue the buffered unicast packet,
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message** msg
//                  The retrieved msg
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static // inline//
BOOL MacDot11DequeueUnicastBufferPacket(
    Node* node,
    MacDataDot11* dot11,
    Message** msg)
{
    BOOL isMsgRetrieved = FALSE;
    DOT11_AttachedNodeList* attachNodeList = dot11->attachNodeList;

    while (attachNodeList != NULL){
        if ((attachNodeList->atimFramesACKReceived == TRUE)
            && (attachNodeList->noOfBufferedUnicastPacket > 0)){

                // dequeue the unicast packet
            isMsgRetrieved = attachNodeList->queue->retrieve(
                msg,
                0,
                PEEK_AT_NEXT_PACKET,
                node->getNodeTime());
            if (isMsgRetrieved ){

               dot11->tempQueue = attachNodeList->queue;
               dot11->tempAttachNodeList = attachNodeList;

                if ((attachNodeList->noOfBufferedUnicastPacket - 1) == 0){
                    dot11->isMoreDataPresent = FALSE;
                }else{
                    dot11->isMoreDataPresent = TRUE;
                }
               dot11->currentNextHopAddress = attachNodeList->nodeMacAddr;
               break;
            }
            // send this unicast packet
        }
        attachNodeList = attachNodeList->next;
    }

    return isMsgRetrieved;
}// end of MacDot11DequeueUnicastBufferPacket


//---------------------------Power-Save-Mode-End-Updates-----------------//

//  NAME:        MacDot11DequeueBufferPacket.
//  PURPOSE:     Dequeue the buffered packet,
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message** msg
//                  The retrieved msg
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static // inline//
BOOL MacDot11DequeueBufferPacket(
    Node* node,
    MacDataDot11* dot11,
    Message** msg)
{
    BOOL isMsgRetrieved = FALSE;
    *msg = NULL;
    if (dot11->currentMessage == NULL){
        if (MacDot11IsIBSSStationSupportPSMode(dot11))
        {
            // first dequue broadcast packet
            if ((dot11->transmitBroadcastATIMFrame == TRUE )
                && (dot11->noOfBufferedBroadcastPacket > 0)){
                // dequeue the broadcast packet
                    dot11->managementFrameDequeued = FALSE;
                isMsgRetrieved = dot11->broadcastQueue->retrieve(
                    msg,
                    0,
                    PEEK_AT_NEXT_PACKET,
                    node->getNodeTime());

                if (isMsgRetrieved){
                    dot11->tempQueue = dot11->broadcastQueue;
                    // Set more data bit here if required, for broadcast packet

                    if ((dot11->noOfBufferedBroadcastPacket - 1) == 0){
                        dot11->isMoreDataPresent = FALSE;
                    }else{
                        dot11->isMoreDataPresent = TRUE;
                    }
                    dot11->currentNextHopAddress = ANY_MAC802;
                }
            }
            else{
                // dequeue the unicast packet
                isMsgRetrieved = MacDot11DequeueUnicastBufferPacket(
                    node,
                    dot11,
                    msg);
            }// end of if - else
        }
   }
    return isMsgRetrieved;
}// end of MacDot11DequeueBufferPacket
//---------------------------Power-Save-Mode-End-Updates-----------------//


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationSetPhySensingDirectionForNextOutgoingPacket
//  PURPOSE:     Set the sensing direction for the next outgoing packet
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationSetPhySensingDirectionForNextOutgoingPacket(
    Node *node,
    MacDataDot11 *dot11)
{
    BOOL haveDirection = FALSE;
    double directionAngleToSend = 0.0;

    // dot11s. Set Phy sensing direction - dequeue.
    if (dot11->currentMessage == NULL) {
        MacDot11StationCheckForOutgoingPacket(node,dot11,FALSE);
    }

    if ((MacDot11StationHasFrameToSend(dot11)) &&
        (dot11->currentNextHopAddress != ANY_MAC802)) {

        AngleOfArrivalCache_GetAngle(
            &dot11->directionalInfo->angleOfArrivalCache,
            dot11->currentNextHopAddress,
            node->getNodeTime(),
            &haveDirection,
            &directionAngleToSend);
    }

    if (haveDirection) {
        PHY_SetSensingDirection(
            node, dot11->myMacData->phyNumber, directionAngleToSend);
    }

}// MacDot11StationSetPhySensingDirectionForNextOutgoingPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationUpdateForSuccessfullyReceivedAck
//  PURPOSE:     Clear message in buffer and related variables
//               once unicast is acknowledged.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Address of the source
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11StationUpdateForSuccessfullyReceivedAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr)
{
    DOT11_SeqNoEntry *entry;

    // dot11s. Update for received ack.
    if (dot11->isMP && dot11->txFrameInfo != NULL)
    {
        Dot11s_PacketAckEvent(node, dot11, dot11->txFrameInfo);

        if (dot11->txFrameInfo->SA == dot11->selfAddr) {
            MAC_MacLayerAcknowledgement(node,
                dot11->myMacData->interfaceIndex,
                dot11->currentMessage, sourceAddr);
        }
    }
    else {
        MAC_MacLayerAcknowledgement(node, dot11->myMacData->interfaceIndex,
            dot11->currentMessage, sourceAddr);
    }

    // Reset retry counts since frame is acknowledged.
    MacDot11StationResetAckVariables(dot11);

    // Remove frame from queue since already acknowledged.
//---------------------------Power-Save-Mode-Updates---------------------//
    BOOL ret = MacDot11IBSSStationDequeuePacketFromLocalQueue(
        node,
        dot11,
        DOT11_X_UNICAST);
    if (dot11->currentMessage != NULL){
        if (ret){
            MESSAGE_Free(node, dot11->currentMessage);
            //Transmit frame Info
            if ((dot11->currentACIndex >= DOT11e_AC_BK)
                && (dot11->txFrameInfo == NULL)) {
                dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameDeQueued++;
                dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                dot11->ACs[dot11->currentACIndex].BO = 0;
                dot11->ACs[dot11->currentACIndex].BO = 0;
                dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;

                if (dot11->ACs[dot11->currentACIndex].frameInfo!= NULL)
                {
                    MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                    dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
                    dot11->dot11TxFrameInfo = NULL;
                }
            }
        }

        MacDot11StationResetCurrentMessageVariables(node, dot11);

        if (!(dot11->state == DOT11_CFP_START)){
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        }
        if (dot11->QosFrameSent)
        {
            entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr,dot11->TID);
        }
        else
        {
            entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr);
        }
        ERROR_Assert(entry,
            "MacDot11UpdateForSuccessfullyReceivedAck: "
            "Unable to find sequence number entry.\n");

        entry->toSeqNo += 1;

        MacDot11StationResetCW(node, dot11);
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // Update exchange sequence number.
}// MacDot11StationUpdateForSuccessfullyReceivedAck


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationGetLastSignalsAngleOfArrivalFromRadio
//  PURPOSE:     Get the last signal's AOA from the radio
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      AOA in double
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
double MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(
    Node *node,
    MacDataDot11 *dot11)
{
    int phyIndex = dot11->myMacData->phyNumber;
    return PHY_GetLastSignalsAngleOfArrival(node, phyIndex);

}// MacDot11StationGetLastSignalsAngleOfArrivalFromRadio


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationUpdateDirectionCacheWithCurrentSignal
//  PURPOSE:     Update the direction of the neighbor in the cache
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destinationAddress
//                  Destination address
//               BOOL* nodeIsInCache
//                  Returns whether the node is in cache
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationUpdateDirectionCacheWithCurrentSignal(
    Node *node,
    MacDataDot11 *dot11,
    Mac802Address destinationAddress,
    BOOL* nodeIsInCache)
{
    AngleOfArrivalCache_Update(
        &dot11->directionalInfo->angleOfArrivalCache,
        destinationAddress,
        MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(node, dot11),
        nodeIsInCache,
        (node->getNodeTime() + dot11->directionalInfo->defaultCacheExpirationDelay),
        node->getNodeTime());
}//MacDot11StationUpdateDirectionCacheWithCurrentSignal


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationAddDirectionCacheWithCurrentSignal
//  PURPOSE:     Add an entry for the neighbor to the cache
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destinationAddress
//                  Destination address
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationAddDirectionCacheWithCurrentSignal(
    Node *node,
    MacDataDot11* dot11,
    Mac802Address destinationAddress)
{
    AngleOfArrivalCache_Add(
        &dot11->directionalInfo->angleOfArrivalCache,
        destinationAddress,
        MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(node, dot11),
        (node->getNodeTime() +
        dot11->directionalInfo->defaultCacheExpirationDelay));
}//MacDot11StationAddDirectionCacheWithCurrentSignal


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHandOffSuccessfullyReceivedBroadcast
//  PURPOSE:     Strip Dot11 header and forward message to upper layer.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationHandOffSuccessfullyReceivedBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_LongControlFrame* lHdr =
        (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = lHdr->sourceAddr;

    if (lHdr->frameType == DOT11_QOS_DATA)
    {
        if (dot11->isHTEnable)
        {
           MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(DOT11n_FrameHdr),
                             TRACE_DOT11);
        }
        else
        {
        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(DOT11e_FrameHdr),
                             TRACE_DOT11);
    }
    }
    // dot11s. Handoff BC.
    else if (lHdr->frameType == DOT11_MESH_DATA)
    {
        // The decision to hand off a copy to the
        // upper layer is based on configuration
        // and handled within the called function.
        Dot11s_ReceiveDataBroadcast(node, dot11, msg);

        return;
    }
    else
    {
        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);
    }

    MAC_HandOffSuccessfullyReceivedPacket(node,
        dot11->myMacData->interfaceIndex, msg, &sourceAddr);

}// MacDot11StationHandOffSuccessfullyReceivedBroadcast


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationHandOffSuccessfullyReceivedUnicast
//  PURPOSE:     Remove unicast header, forward the message to upper layer
//               and update sequence number in list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationHandOffSuccessfullyReceivedUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_SeqNoEntry *entry;
    int currTid = -1;
    unsigned char frameType = DOT11_DATA;
    DOT11_LongControlFrame* lHdr =
        (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = lHdr->sourceAddr;
    BOOL handOff = TRUE;

    BOOL isAmsduMsg = FALSE;


    if (dot11->isHTEnable)
    {
        DOT11n_FrameHdr* dot11nHdr  =
            (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(msg);

        isAmsduMsg = dot11nHdr->qoSControl.AmsduPresent;

    }

    BOOL isDecrypt = FALSE;
    BOOL protection = FALSE;

    if ((lHdr->frameType == DOT11_QOS_DATA) &&
                        (dot11->isHTEnable) &&
                        (dot11->isAmsduEnable))
    {

        frameType = DOT11_QOS_DATA;
       DOT11n_FrameHdr* Hdr =
        (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(msg);
       currTid = Hdr->qoSControl.TID;

        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(DOT11n_FrameHdr),
                             TRACE_DOT11);
        isDecrypt = TRUE;


    }
    else if (lHdr->frameType == DOT11_QOS_DATA)
    {
        frameType = DOT11_QOS_DATA;
        if (dot11->isHTEnable)
        {
              DOT11n_FrameHdr* Hdr =
            (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(msg);
           currTid = Hdr->qoSControl.TID;

            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(DOT11n_FrameHdr),
                                 TRACE_DOT11);
        }
        else
        {
       DOT11e_FrameHdr* Hdr =
        (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(msg);
       currTid = Hdr->qoSControl.TID;

        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(DOT11e_FrameHdr),
                             TRACE_DOT11);
        }
        isDecrypt = TRUE;
    }
    else if (lHdr->frameType == DOT11_MESH_DATA)
    {
        // dot11s. Receive mesh data unicast.
        // This call is placed here for sequence
        // number updation, later in this function.
        Dot11s_ReceiveDataUnicast(node, dot11, msg);
        handOff = FALSE;
    }
    else
    {
        if (dot11->isMP) {
            Dot11s_ReceiveDataUnicast(node, dot11, msg);
            handOff = FALSE;
        }
        else {
#ifdef CYBER_LIB
            if (lHdr->frameFlags & DOT11_PROTECTED_FRAME)
            {
                protection = TRUE;
            }
#endif // CYBER_LIB

            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(DOT11_FrameHdr),
                                 TRACE_DOT11);
#ifdef CYBER_LIB
            if (node->macData[dot11->myMacData->interfaceIndex]->encryptionVar
               != NULL)
            {
                MAC_SECURITY* secType = (MAC_SECURITY*)
                    node->macData[dot11->myMacData->interfaceIndex]
                    ->encryptionVar;
                if (*secType == MAC_SECURITY_WEP)
                {

                    isDecrypt =
                        WEPDecryptPacket(node, msg,
                                         dot11->myMacData->interfaceIndex,
                                         sourceAddr, protection);
                }
                else if (*secType == MAC_SECURITY_CCMP)
                {

                    isDecrypt =
                        CCMPDecryptPacket(node, msg,
                                          dot11->myMacData->interfaceIndex,
                                          sourceAddr, protection);
                }
                else
                {
                    ERROR_Assert(FALSE, "Unknown Security Protocol");
                }
            }
            else if (!protection)
            {
                //If WEP/CCMP is OFF and Protection is also OFF
                isDecrypt = TRUE;
            }
            else
            {
                ERROR_Assert(FALSE, "Got encrypted data. Unable to decrypt "
                             "as no security protocol is enabled");
            }
#endif // CYBER_LIB
        }
    }

#ifdef CYBER_LIB
    if (isDecrypt)
    {
        if (handOff && isAmsduMsg)
        {
            Message* listItem = MESSAGE_UnpackMessage(node,msg,0,1);

            Mac802Address SA;

            while (listItem != NULL)
            {

                msg = listItem;
                aMsduSubFrameHdr* Hdr =
                             (aMsduSubFrameHdr*) MESSAGE_ReturnPacket(msg);
                SA = Hdr->SA;
                MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(aMsduSubFrameHdr),
                             TRACE_AMSDU_SUB_HDR);

                listItem = listItem->next; 
                MAC_HandOffSuccessfullyReceivedPacket(node,
                                dot11->myMacData->interfaceIndex, msg, &SA);
                                
                }
            
        }
        
        else if (handOff)
        {
            MAC_HandOffSuccessfullyReceivedPacket(
                node, dot11->myMacData->interfaceIndex, msg, &sourceAddr);
        }
    }
    else
    {
        if (protection == TRUE)
        {
            ERROR_Assert((dot11->isMP == FALSE) && (lHdr->frameType != DOT11_QOS_DATA),
                "MacDot11StationHandOffSuccessfullyReceivedUnicast: "
                "MAC security not supported for 802.11s.\n");
            MESSAGE_Free(node, msg);
        }
        else if (handOff)
        {
            MESSAGE_Free(node, msg);
        }
    }
#else // CYBER_LIB

    if (handOff && isAmsduMsg)
        {
            Message* listItem = MESSAGE_UnpackMessage(node,msg,0,1);

            Mac802Address SA;

            while (listItem != NULL)
            {
                msg = listItem;
                aMsduSubFrameHdr* Hdr =
                             (aMsduSubFrameHdr*) MESSAGE_ReturnPacket(msg);
                
                SA = Hdr->SA;
                MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(aMsduSubFrameHdr),
                             TRACE_AMSDU_SUB_HDR);

                listItem = listItem->next; 
                MAC_HandOffSuccessfullyReceivedPacket(node,
                                dot11->myMacData->interfaceIndex, msg, &SA);
                                
                }
            
        }    


    else if (handOff) {
        MAC_HandOffSuccessfullyReceivedPacket(node,
            dot11->myMacData->interfaceIndex, msg, &sourceAddr);
    }
#endif // CYBER_LIB

    // Update sequence number
    if (frameType == DOT11_QOS_DATA)
    {
         dot11->totalNoOfQoSDataFrameReceived++;
         entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr,currTid);
    }
    else
    {
         dot11->totalNoOfnQoSDataFrameReceived++;
         entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr);
    }

    ERROR_Assert(entry,
        "MacDot11HandOffSuccessfullyReceivedUnicast: "
        "Unable to create or find sequence number entry.\n");
    entry->fromSeqNo += 1;

}// MacDot11StationHandOffSuccessfullyReceivedUnicast


//--------------------------------------------------------------------------
/*!
 * \brief  Start a managment timer

 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param timerDelay clocktype      : Delay time

 * \return          None..
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementStartTimerOfGivenType(
    Node* node,
    MacDataDot11* dot11,
    clocktype timerDelay,int TimerType)
{
    Message* newMsg;

    dot11->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(
                node,
                MAC_LAYER,
                MAC_PROTOCOL_DOT11,
                TimerType);

    MESSAGE_SetInstanceId(
        newMsg,
        (short) dot11->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(dot11->timerSequenceNumber));

    *((int*)MESSAGE_ReturnInfo(newMsg)) = dot11->timerSequenceNumber;

    MESSAGE_Send(
        node,
        newMsg,
        timerDelay);

    dot11->managementSequenceNumber = dot11->timerSequenceNumber;

}// MacDot11ManagementStartTimerOfGivenType

//--------------------------------------------------------------------------
//  NAME:        MacDot11ManagementStartTimerOfGivenFrameType.
//  PURPOSE:     Start Management timers.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               unsigned char frameType
//                  frame type
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11ManagementStartTimerOfGivenFrameType(Node* node,
                                  MacDataDot11* dot11,
                                  DOT11_MacFrameType frameType)
{
    clocktype timerDelay;
    //int timerType;

    switch(frameType)
    {
        case DOT11_ASSOC_REQ:
        {
            timerDelay = DOT11_MANAGEMENT_ASSOC_RESPONSE_TIMEOUT;

            MacDot11ManagementStartTimerOfGivenType(
                        node,
                        dot11,
                        timerDelay,
                        MSG_MAC_DOT11_Management_Association);
            break;
        }

        case DOT11_REASSOC_REQ:
        {
            timerDelay = DOT11_MANAGEMENT_ASSOC_RESPONSE_TIMEOUT;
            MacDot11ManagementStartTimerOfGivenType(
                         node,
                         dot11,
                         timerDelay,
                         MSG_MAC_DOT11_Management_Reassociation);
            break;
        }

        case DOT11_PROBE_REQ:
        {
            timerDelay = DOT11_MANAGEMENT_PROBE_RESPONSE_TIMEOUT;
            MacDot11ManagementStartTimerOfGivenType(
                          node,
                          dot11,
                          timerDelay,
                          MSG_MAC_DOT11_Management_Probe);
            break;
        }

        case DOT11_AUTHENTICATION:
        {
            timerDelay = DOT11_MANAGEMENT_AUTH_RESPONSE_TIMEOUT;
            MacDot11ManagementStartTimerOfGivenType(
                           node,
                           dot11,
                           timerDelay,
                           MSG_MAC_DOT11_Management_Authentication);
            break;
        }
        default:
        {
            break;
        }

    }
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationProcessAck.
//  PURPOSE:     Process local ACKs.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *Ack
//                  Acknowledgement message
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------

static //inline//
void MacDot11StationProcessAck(
    Node* node,
    MacDataDot11* dot11,
    Message *Ack)
{
    Mac802Address sourceAddr;
    sourceAddr = dot11->waitingForAckOrCtsFromAddress;

    // Reset retry counts since frame is acknowledged
    MacDot11StationResetAckVariables(dot11);
//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsATIMDurationActive(dot11)
        && (dot11->currentNextHopAddress != ANY_MAC802)){
        DOT11_AttachedNodeList* attachNodeList = dot11->attachNodeList;
             MESSAGE_Free(node,dot11->currentMessage);
             dot11->currentMessage = NULL;
             if (dot11->dot11TxFrameInfo!= NULL &&
                 dot11->dot11TxFrameInfo->frameType ==
                 DOT11_ATIM)
             {
                 MEM_free(dot11->dot11TxFrameInfo);
             }
             dot11->dot11TxFrameInfo = NULL;
             dot11->managementFrameDequeued = FALSE;
        while (attachNodeList != NULL){
            if ((attachNodeList->nodeMacAddr == dot11->currentNextHopAddress )
                && dot11->attachNodeList->atimFramesSend){
                dot11->atimFramesACKReceived++;
                attachNodeList->atimFramesACKReceived = TRUE;
                break;
            }
            attachNodeList = attachNodeList->next;
        } //end of  while
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
        return;
    }

    if (!MacDot11StationHasFrameToSend(dot11)
        && ( dot11->state != DOT11_S_WFPSPOLL_ACK)) {
//---------------------------Power-Save-Mode-End-Updates-----------------//
        ERROR_ReportError("MacDot11StationProcessAck: "
            "There is no message in local buffer.\n");
    }

    if (dot11->useDvcs) {

        // For smart antennas, go back to sending/receiving
        // omnidirectionally.

        PHY_UnlockAntennaDirection(
            node, dot11->myMacData->phyNumber);

    }

    switch (dot11->state) {
        case DOT11_S_WFACK: {
            MacDot11StationResetCW(node, dot11);
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                if (dot11->dot11TxFrameInfo!= NULL)
                {
                    if ((dot11->dot11TxFrameInfo->frameType == DOT11_ACTION)
                        && (MacDot11IsAp(dot11)))
                    {
                        DOT11e_ADDTS_Response_frame* rxRspFrame =
                            (DOT11e_ADDTS_Response_frame*)MESSAGE_ReturnPacket(
                            dot11->dot11TxFrameInfo->msg);
                        DOT11e_HCCA_CapPollStation* data = NULL;
                        data = (DOT11e_HCCA_CapPollStation*)
                        MEM_malloc(sizeof(DOT11e_HCCA_CapPollStation));
                        memset(data,0,sizeof(DOT11e_HCCA_CapPollStation));
                        data->macAddr = rxRspFrame->hdr.destAddr;

                       MacDot11eHCPollListInsert(
                        node,
                        dot11,
                        dot11->hcVars->cfPollList,
                        data,
                        dot11->TSPEC.info.tsid);

                    }

                    if (dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ &&
                        dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)
                    {
                        if (dot11->currentMessage != NULL &&
                        dot11->currentMessage == dot11->dot11TxFrameInfo->msg){
                             MESSAGE_Free(node, dot11->currentMessage);

                         MacDot11StationSetState(node,
                                                 dot11,
                                                 DOT11_S_IDLE);
                         if (dot11->isHTEnable)
                         {
                         }
                         else
                         {
                             MacDot11MgmtQueue_DequeuePacket(node,dot11);
                         }
                             dot11->managementFrameDequeued = FALSE;
                             MacDot11StationResetCW(node, dot11);
                             if (!MacDot11IsAp(dot11))
                             MacDot11ManagementStartTimerOfGivenFrameType(
                             node,
                             dot11,
                             dot11->dot11TxFrameInfo->frameType);
                             dot11->dot11TxFrameInfo = NULL;


                         MacDot11StationResetCurrentMessageVariables(node,
                                                                    dot11);
                            if ((dot11->currentACIndex >= DOT11e_AC_BK)) {
                            dot11->ACs[dot11->currentACIndex].
                                totalNoOfthisTypeFrameDeQueued++;
                            dot11->ACs[dot11->currentACIndex].frameToSend =
                                                                        NULL;
                                dot11->ACs[dot11->currentACIndex].BO = 0;
                            dot11->ACs[dot11->currentACIndex].
                                currentNextHopAddress = INVALID_802ADDRESS;
                            if (dot11->ACs[dot11->currentACIndex].
                                                          frameInfo!= NULL)
                                {
                                    MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                                    dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
                                }

                            }

                        MacDot11StationCheckForOutgoingPacket(node,
                                                              dot11,
                                                              TRUE);

                        }
                    }
                    else
                    {
                        MacDot11StationUpdateForSuccessfullyReceivedAck(
                                        node,
                                        dot11,
                                        sourceAddr);

                        dot11->unicastPacketsSentDcf++;

                        MacDot11StationCheckForOutgoingPacket(
                            node,
                            dot11,
                            TRUE);
                    }
                }
                else
                {
                    MacDot11StationUpdateForSuccessfullyReceivedAck(
                        node,
                        dot11,
                        sourceAddr);

                    dot11->unicastPacketsSentDcf++;

                    MacDot11StationCheckForOutgoingPacket(
                        node,
                        dot11,
                        TRUE);
                }
            break;
        }
        case DOT11_S_WFPSPOLL_ACK:{
             MacDot11StationCancelTimer(node, dot11);
        }
        default: {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);

            MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            break;
        }
    }//switch//

}// MacDot11StationProcessAck


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationExaminePotentialIncomingMessage
//  PURPOSE:     Examine incoming message.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               PhyStatusType phyStatus
//                  Status of the physical medium
//               clocktype receiveDuration
//                  Receive duration
//               const Message* thePacketIfItGetsThrough
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationExaminePotentialIncomingMessage(
    Node* node,
    MacDataDot11* dot11,
    PhyStatusType phyStatus,
    clocktype receiveDuration,
    const Message* thePacketIfItGetsThrough)
{
    
    if (dot11->isHTEnable)
    {
    
        MAC_PHY_TxRxVector rxVector;
            PHY_GetRxVector(node,
                            dot11->myMacData->phyNumber,
                            rxVector);

            if (rxVector.containAMPDU)
            {
                dot11->IsInExtendedIfsMode = TRUE;
                return;
            }
    }

    DOT11_ShortControlFrame* hdr = (DOT11_ShortControlFrame*)
        MESSAGE_ReturnHeader(thePacketIfItGetsThrough, 1);

    dot11->potentialIncomingMessage = thePacketIfItGetsThrough;

    ERROR_Assert(phyStatus == PHY_RECEIVING,
        "MacDot11ExaminePotentialIncomingMessage: "
        "Physical is not in RECEIVING state.\n");

    dot11->IsInExtendedIfsMode = TRUE;

    if (dot11->state == DOT11_S_NAV_RTS_CHECK_MODE) {

        // The RTS (Request To Send) is getting a CTS so continue the
        // NAV wait and cancel the timer that would reset the NAV.

        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationCancelTimer(node, dot11);
    }
//---------------------DOT11e--Updates------------------------------------//
    if (dot11->stopReceivingAfterHeaderMode) {
        if ((hdr->frameType == DOT11_DATA ||
             hdr->frameType == DOT11_QOS_DATA) &&
            (!MacDot11IsWaitingForResponseState(dot11->state)))
        {
            clocktype headerCheckDelay;
            if (dot11->isHTEnable)
            {
                MAC_PHY_TxRxVector rxVector;
                PHY_GetRxVector(node,
                                dot11->myMacData->phyNumber,
                                rxVector);

                rxVector.length = (size_t)DOT11_SHORT_CTRL_FRAME_SIZE;

                 headerCheckDelay =
                    PHY_GetTransmissionDuration(node,
                        dot11->myMacData->phyNumber,
                        rxVector);            
            }
            else
            {
                 headerCheckDelay =
                PHY_GetTransmissionDuration(node,
                    dot11->myMacData->phyNumber,
                    PHY_GetRxDataRateType(
                        node, dot11->myMacData->phyNumber),
                    DOT11_SHORT_CTRL_FRAME_SIZE);
            }

            MacDot11StationSetState(node, dot11, DOT11_S_HEADER_CHECK_MODE);
            MacDot11StationStartTimer(node, dot11, headerCheckDelay);

        } else if (dot11->state == DOT11_S_HEADER_CHECK_MODE) {

            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCancelTimer(node, dot11);
        }
    }


}//MacDot11ExaminePotentialIncomingMessage//

//--------------------------------------------------------------------------
//  TX Helper Functions
//--------------------------------------------------------------------------

//--------------------HCCA-Updates Start---------------------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCanTransmitAck
//  PURPOSE:     Check if station can transmit Ack frame. If HCCA is enabled
//               a station should transmit Ack even if NAV is set.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE - if can transmit
//               FALSE - if unable to transmit
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11StationCanTransmitAck(
    Node* node,
    MacDataDot11* dot11)
{

    // Check if we can transmit
    if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) ||
         (!MacDot11IsPhyIdle(node, dot11)))
    {
        //This should not occure in case of Ack.
        MacDot11Trace(node, dot11, NULL,
            "Frame delayed - not idle.");

        // Either wait for timer to expire or for
        // StartSendTimers to reattempt transmission
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        return FALSE;
    }

    // Cancel existing timers
    MacDot11StationCancelTimer(node, dot11);

    return TRUE;

}// MacDot11StationCanTransmitAck
//--------------------HCCA-Updates End-----------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationCanTransmit
//  PURPOSE:     Check if station can transmit
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE - if can transmit
//               FALSE - if unable to transmit
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11StationCanTransmit(
    Node* node,
    MacDataDot11* dot11)
{

    // Check if we can transmit
    if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) ||
         (!MacDot11IsPhyIdle(node, dot11)) ||
         (MacDot11StationWaitForNAV(node, dot11)) )
    {
        MacDot11Trace(node, dot11, NULL,
            "Frame delayed - not idle or waiting for NAV.");

        // Either wait for timer to expire or for
        // StartSendTimers to reattempt transmission
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        return FALSE;
    }

    // Cancel existing timers
    MacDot11StationCancelTimer(node, dot11);

    return TRUE;

}// MacDot11StationCanTransmit

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationCanTransmit
//  PURPOSE:     Check if station can transmit
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE - if can transmit
//               FALSE - if unable to transmit
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpStationCanTransmit(
    Node* node,
    MacDataDot11* dot11)
{

    // Check if we can transmit
    if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) ||
         (!MacDot11IsPhyIdle(node, dot11)) )

    {
        MacDot11Trace(node, dot11, NULL,
            "Frame delayed - not idle or waiting for NAV.");

        // Either wait for timer to expire or for
        // StartSendTimers to reattempt transmission

        MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);


        return FALSE;
    }

    // Cancel existing timers
    MacDot11StationCancelTimer(node, dot11);

    return TRUE;

}// MacDot11CfpStationCanTransmit

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartTransmittingPacket
//  PURPOSE:     Start transmitting packet.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* packet
//                  Packet to be transmitted
//               clocktype delay
//                  Delay before transmission
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationStartTransmittingPacket(
    Node* node,
    MacDataDot11* dot11,
    Message* packet,
    clocktype delay)
{
    if (!MAC_InterfaceIsEnabled(node, dot11->myMacData->interfaceIndex))
    {
        //Interface is  not enabled discard the packet and set state to IDLE
        //Current message is not freed.

        if (dot11->state == DOT11_CFP_START)
        {
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
        }
        else if (dot11->state == DOT11e_CAP_START)
        {
            dot11->cfprevstate = dot11->cfstate;
            dot11->cfstate = DOT11e_S_CFP_NONE;
        }
        else
        {
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        }

        if (dot11->currentNextHopAddress == ANY_MAC802)
        {
            if (packet == dot11->currentMessage &&
                MacDot11IsIBSSStationSupportPSMode(dot11))
            {
                MESSAGE_Free(node, packet);
            }
            packet = NULL;
        }
        else
        {
            MESSAGE_Free(node, packet);
        }
        return;
    }

     if (DEBUG) {
        char timeStr[100], delayStr[100];
        char macAdder[24];
        DOT11_LongControlFrame* hdr =
            (DOT11_LongControlFrame*)MESSAGE_ReturnPacket(packet);

        ctoa(node->getNodeTime(), timeStr);
        ctoa(delay, delayStr);


        MacDot11MacAddressToStr(macAdder,&hdr->destAddr);
        printf("[%d] Sending to %20s (size %d) at %s %s state:%d\n",
               node->nodeId,
               macAdder, DOT11nMessage_ReturnPacketSize(packet),
               timeStr, delayStr,
               dot11->state);
    }
         MacDot11Trace(node, dot11, packet, "Send");

//---------------------DOT11e--Updates------------------------------------//

    if (MacDot11IsQoSEnabled(node, dot11))
    {
        // Update specific statistics
        if (dot11->isHTEnable)
        {
            //do not update statistics here as in case of ampdu, header cannot
            //be accessed
        }
        else
        {
        DOT11_ShortControlFrame* hdr =
            (DOT11_ShortControlFrame*)MESSAGE_ReturnPacket(packet);
//--------------------HCCA-Updates Start---------------------------------//
        if (dot11->state == DOT11e_CAP_START)
            {
                switch(hdr->frameType)
                {
                    case DOT11_QOS_DATA:
                    dot11->totalNoOfQoSCFDataFrameSend++;
                    dot11->totalNoOfQoSDataFrameSend++;
                    break;

                case  DOT11_DATA:
                    dot11->totalNoOfnQoSDataFrameSend++;
                    break;

                case DOT11_QOS_CF_NULLDATA:
                    dot11->totalQoSNullDataTransmitted++;
                    break;

                case DOT11_ACK:
                case DOT11_CF_ACK:
                case DOT11_QOS_DATA_POLL_ACK:
                case DOT11_QOS_CF_DATA_ACK:
                    dot11->totalNoOfQoSCFAckTransmitted++;
                    break;

                case DOT11_QOS_CF_POLL:
                    dot11->totalNoOfQoSCFPollsTransmitted++;
                    break;

                case DOT11_QOS_DATA_POLL:
                    dot11->totalNoOfQoSDataCFPollsTransmitted++;
                    break;
                }
            }
//--------------------HCCA-Updates End-----------------------------------//
        else{

            if (hdr->frameType == DOT11_QOS_DATA)
            {
                 // Update the statistics accordingly
                 dot11->totalNoOfQoSDataFrameSend++;

                // Set the CurrentACIndex to invalid to
                // retrieve again the appropriate AC.

                //dot11->currentACIndex = DOT11e_INVALID_AC;

            } else if (hdr->frameType == DOT11_DATA) {

                if (MacDot11IsQoSEnabled(node, dot11))
                {
                    dot11->totalNoOfnQoSDataFrameSend++;
                }
            }
        }
        }//htenable
    }
//--------------------DOT11e-End-Updates---------------------------------//

#ifdef ADDON_DB
    HandleMacDBEvents(
        node, packet, dot11->myMacData->phyNumber,
        dot11->myMacData->interfaceIndex,
        MAC_SendToPhy, MAC_PROTOCOL_DOT11);
#endif
        
    BOOL isCtrlFrame = FALSE;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;
    int headerSize = -1;
    Mac802Address destAddr;
    NodeId destID;
    MacDot11GetPacketProperty(
        node,
        packet,
        dot11->myMacData->interfaceIndex,
        headerSize,
        destAddr,
        destID,
        isCtrlFrame,
        isMyFrame,
        isAnyFrame);

    MacDot11UpdateStatsSend(
        node,
        packet,
        dot11->myMacData->interfaceIndex,
        isAnyFrame,
        isMyFrame,
        !isCtrlFrame,
        destAddr);

    PHY_StartTransmittingSignal(
        node, dot11->myMacData->phyNumber, packet, TRUE, delay);

#ifdef PAS_INTERFACE
#ifndef _WIN32
    if (PAS_LayerCheck(node, dot11->myMacData->interfaceIndex, PACKET_SNIFFER_DOT11))
    {
//      PAS_CreateFrames(node, packet, node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
        PAS_CreateFrames(node, packet);
    }
#endif
#endif

}// MacDot11StationStartTransmittingPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationStartTransmittingPacketDirectionally
//  PURPOSE:     Start transmitting packet directionally
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* packet
//                  Packet to be transmitted
//               clocktype delay
//                  Delay before transmission
//               double directionAngle
//                  Direction of transmission
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationStartTransmittingPacketDirectionally(
    Node* node,
    MacDataDot11* dot11,
    Message* packet,
    clocktype delay,
    double directionAngle)
{
    BOOL isCtrlFrame = FALSE;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;
    int headerSize = -1;
    Mac802Address destAddr;
    NodeId destID;
    MacDot11GetPacketProperty(
        node,
        packet,
        dot11->myMacData->interfaceIndex,
        headerSize,
        destAddr,
        destID,
        isCtrlFrame,
        isMyFrame,
        isAnyFrame);
    
    if (!MAC_InterfaceIsEnabled(node, dot11->myMacData->interfaceIndex))
    {
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MESSAGE_Free(node, packet);

        // dot11s. Packet could be beacon, RTS/CTS/Ack/Other.
        if (dot11->isMP && dot11->txFrameInfo != NULL)
        {
            MacDot11Trace(node, dot11, NULL, "Interface fault: dropping packet");
#ifdef ADDON_DB
           
            HandleMacDBEvents(        
                node, packet, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
            if (node->macData[dot11->myMacData->interfaceIndex]->macStats)
            {
                if (!isMyFrame || isAnyFrame)
                {
                    destID = -1;
                }

                node->macData[dot11->myMacData->interfaceIndex]->stats->AddFrameDroppedSenderDataPoints(
                    node, destID, dot11->myMacData->interfaceIndex,
                    MESSAGE_ReturnPacketSize(packet));
            }
            MacDot11StationResetCurrentMessageVariables(node, dot11);
            MacDot11StationResetAckVariables(dot11);
        }

        return;
    }

    if (DEBUG) {
        char timeStr[100], delayStr[100];
        char macAdder[24];
        DOT11_LongControlFrame* hdr =
            (DOT11_LongControlFrame *)MESSAGE_ReturnPacket(packet);

        ctoa(node->getNodeTime(), timeStr);
        ctoa(delay, delayStr);

        MacDot11MacAddressToStr(macAdder,&hdr->destAddr);
        printf("[%d] Sending to %20s (size %d) at %s %s\n", node->nodeId,
               macAdder, DOT11nMessage_ReturnPacketSize(packet),
               timeStr, delayStr);
    }

    MacDot11Trace(node, dot11, packet, "Send");

#ifdef ADDON_DB
    HandleMacDBEvents(
        node, packet, dot11->myMacData->phyNumber,
        dot11->myMacData->interfaceIndex,
        MAC_SendToPhy, MAC_PROTOCOL_DOT11);
#endif

    MacDot11UpdateStatsSend(
        node,
        packet,
        dot11->myMacData->interfaceIndex,
        isAnyFrame,
        isMyFrame,
        !isCtrlFrame,
        destAddr);
    
    PHY_StartTransmittingSignalDirectionally(
        node, dot11->myMacData->phyNumber,  packet, TRUE, delay,
        directionAngle);
}// MacDot11StationStartTransmittingPacketDirectionally


//--------------------------------------------------------------------------
//  TX Functions
//--------------------------------------------------------------------------
//------------------------------------------------------------
//  NAME      :  MacDot11StationTransmitInstantFrame
//  PURPOSE   :  Station transmit a Instant frame.
//  PARAMETERS:  node* node
//                 Pointer to Node
//                 MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None.
//---------------------------------------------------------------
 static
  void   MacDot11StationTransmitInstantFrame(Node* node,
        MacDataDot11* dot11)
  {
        int dataRateType;
        int setDataRateType;

        BOOL canTransmit = FALSE;
        DOT11_LongControlFrame* hdr = ((DOT11_LongControlFrame*)
        MESSAGE_ReturnPacket(dot11->instantMessage));


        setDataRateType = dot11->highestDataRateTypeForBC;

/*********HT START*************************************************/
        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector tempTxVector;
            tempTxVector = MacDot11nSetTxVector(
                               node, 
                               dot11, 
                               DOT11nMessage_ReturnPacketSize(
                                   dot11->instantMessage),
                                   DOT11_BEACON, 
                                   ANY_MAC802, 
                                   NULL);
            PHY_SetTxVector(node, 
                            dot11->myMacData->phyNumber,
                            tempTxVector);
        }
        else
        {
        PHY_SetTxDataRateType(node, dot11->myMacData->phyNumber,
        setDataRateType);
             dataRateType = 
                 PHY_GetTxDataRateType(
                        node,
                        dot11->myMacData->phyNumber);
        ERROR_Assert(dataRateType == dot11->highestDataRateTypeForBC,
                     "Wrong data rate");
        }
/*********HT END****************************************************/ 


        if (MacDot11IsAp(dot11) && hdr->frameType == DOT11_BEACON)
        {
            canTransmit = MacDot11CanTransmitBeacon(
                            node,
                            dot11,
                            DOT11nMessage_ReturnPacketSize(dot11->instantMessage));
           if (canTransmit)
           {
                MacDot11StationSetState(node, dot11, DOT11_X_BEACON);

                if (DEBUG_BEACON)
                {
                     char timeStr[100];
                     ctoa(node->getNodeTime(), timeStr);
                     printf("\n%d Transmitting Beacon "
                           "\t  at %s n",
                            node->nodeId,
                            timeStr);
                }

                MacDot11StationStartTransmittingPacket(node, dot11,
                                                    dot11->instantMessage,
                                    dot11->delayUntilSignalAirborn);
           }
           else{
               MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
           }
        }
        else{
              MacDot11StationStartTransmittingPacket(node, dot11, dot11->instantMessage,
              dot11->delayUntilSignalAirborn);
              MacDot11StationSetState(node, dot11, DOT11_X_BROADCAST);
        }

  }//MacDot11StationTransmitInstantFrame
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitRTSFrame.
//  PURPOSE:     Send RTS to reserve channel for data frame.
//  PARAMETERS:  Node* node that wants to reserve channel for data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationTransmitRTSFrame(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_LongControlFrame* hdr;
    Message* pktToPhy;
    Mac802Address destAddr;
    int dataRateType;
/*********HT START*************************************************/
    MAC_PHY_TxRxVector txVector; 
    MAC_PHY_TxRxVector tempTxVectorForUnicast;
/*********HT END****************************************************/
    clocktype packetTransmissionDuration = 0;

    
    destAddr = dot11->currentNextHopAddress;
    if (dot11->dataRateInfo == NULL || (dot11->currentNextHopAddress !=
        dot11->dataRateInfo->ipAddress))
    {
        dot11->dataRateInfo =
            MacDot11StationGetDataRateEntry(node, dot11,
                dot11->currentNextHopAddress);
    }

    ERROR_Assert(dot11->currentNextHopAddress ==
                 dot11->dataRateInfo->ipAddress,
                 "Address does not match");

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           DOT11_LONG_CTRL_FRAME_SIZE,
                           DOT11_RTS, 
                           destAddr, 
                           NULL);
        PHY_SetTxVector(node, 
                        dot11->myMacData->phyNumber,
                        tempTxVector);
        PHY_GetTxVector(node,
                        dot11->myMacData->phyNumber,
                        txVector);

    }
    else
    {
    PHY_SetTxDataRateType(node,
        dot11->myMacData->phyNumber,
        MIN(dot11->dataRateInfo->dataRateType,
            dot11->highestDataRateTypeForBC));
    dataRateType = PHY_GetTxDataRateType(node, dot11->myMacData->phyNumber);
    }
/*********HT END****************************************************/

    

    if (!MacDot11StationHasFrameToSend(dot11) ) {
        ERROR_ReportError("MacDot11StationTransmitRTSFrame: "
            "There is no message in the local buffer.\n");
    }

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        DOT11_LONG_CTRL_FRAME_SIZE,
                        TRACE_DOT11);

    hdr = (DOT11_LongControlFrame*)MESSAGE_ReturnPacket(pktToPhy);
    memset(hdr, 0, sizeof(DOT11_LongControlFrame));
    hdr->frameType = DOT11_RTS;

    hdr->sourceAddr = dot11->selfAddr;
    hdr->destAddr = destAddr;

    dot11->waitingForAckOrCtsFromAddress = destAddr;

    if (dot11->isHTEnable
        && dot11->dot11TxFrameInfo->frameType == DOT11_QOS_DATA
        && dot11->dot11TxFrameInfo->baActive)
    {
        hdr->duration = 
           MacDot11NanoToMicrosecond(MacDot11nGetNavDuration(node,
                                     dot11,
                                     dot11->dot11TxFrameInfo));

    }
    else if (MacDot11StationGetCurrentMessage(dot11) != NULL) 
    {
        UInt16 fragThreshold = 0;
        if (dot11->isHTEnable)
        {
            fragThreshold = DOT11N_FRAG_THRESH;
        }
        else
        {
             fragThreshold = DOT11_FRAG_THRESH;
        }

        UInt32 pktSize = MacDot11StationGetCurrentMessageSize(dot11);
        if (dot11->isHTEnable)
        {
            pktSize += dot11->dot11TxFrameInfo->ampduOverhead;
        }

         if (pktSize <= fragThreshold)
         {
//---------------------DOT11e--Updates------------------------------------//

            if (MacDot11IsQoSEnabled(node, dot11))
            {
/*********HT START*************************************************/                
                if (dot11->isHTEnable)
                {
                    
                    tempTxVectorForUnicast = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           pktSize,
                           dot11->dot11TxFrameInfo->frameType, 
                           destAddr, 
                           NULL);
                    packetTransmissionDuration = 
                    PHY_GetTransmissionDuration(node, 
                                                dot11->myMacData->phyNumber,
                                                tempTxVectorForUnicast);
                }
                else
                {
             packetTransmissionDuration =
                PHY_GetTransmissionDuration(
                            node,
                            dot11->myMacData->phyNumber,
                    dot11->dataRateInfo->dataRateType,
                            DOT11nMessage_ReturnPacketSize(
                                MacDot11StationGetCurrentMessage(dot11)));
                }
/*********HT END****************************************************/
             }
             else
             {
              packetTransmissionDuration =
                PHY_GetTransmissionDuration(
                      node,
                      dot11->myMacData->phyNumber,
                    dot11->dataRateInfo->dataRateType,
                      DOT11nMessage_ReturnPacketSize(
                          MacDot11StationGetCurrentMessage(dot11)));
            }

// NOTE: For DOT11e the header duration would be the Transmission Opertunity
// duration. However, it is not set in the header field as currently it is
// considered one packet queue Access Category and thus not possible to
// calculate No. of packet remaining with same priority needs to be
// tranmitted. So in the duration field only one packets transmission
// duration along with CTS,IFS,ACK is considered.

//--------------------DOT11e-End-Updates---------------------------------//
/*********HT START*************************************************/
            if (dot11->isHTEnable)
            {
                MAC_PHY_TxRxVector tempVector1 = MacDot11nSetTxVector(
                                             node, 
                                             dot11, 
                                             DOT11_SHORT_CTRL_FRAME_SIZE,
                                             DOT11_CTS, 
                                             destAddr, 
                                             &txVector);
                
                
                UInt32 ackSize = DOT11_SHORT_CTRL_FRAME_SIZE;
                DOT11_MacFrameType ackType = DOT11_ACK;
                if (dot11->dot11TxFrameInfo->isAMPDU)
                {
                    ackSize = sizeof(DOT11n_BlockAckControlFrame);
                    ackType = DOT11N_BLOCK_ACK;
                }
                
                MAC_PHY_TxRxVector tempVector2 = MacDot11nSetTxVector(
                                             node,
                                             dot11,
                                             ackSize,
                                             ackType,
                                             destAddr,
                                             &tempTxVectorForUnicast);
                hdr->duration = (UInt16)
                    MacDot11NanoToMicrosecond(
                        dot11->extraPropDelay + dot11->sifs +
                        PHY_GetTransmissionDuration(
                            node, dot11->myMacData->phyNumber,
                            tempVector1) +
                        dot11->extraPropDelay + dot11->sifs +
                        packetTransmissionDuration +
                        dot11->extraPropDelay + dot11->sifs +
                        PHY_GetTransmissionDuration(
                            node, dot11->myMacData->phyNumber,
                            tempVector2) +
                        dot11->extraPropDelay);
             
            }
            else
            {
            hdr->duration = (UInt16)
                MacDot11NanoToMicrosecond(
                    dot11->extraPropDelay + dot11->sifs +
                    PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        dataRateType,
                        DOT11_SHORT_CTRL_FRAME_SIZE) +
                    dot11->extraPropDelay + dot11->sifs +
                    packetTransmissionDuration +
                    dot11->extraPropDelay + dot11->sifs +
                    PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        dot11->dataRateInfo->dataRateType,
                        DOT11_SHORT_CTRL_FRAME_SIZE) +
                    dot11->extraPropDelay);
             }
/*********HT END****************************************************/

        }
        else {
            // Fragmentation Needs to be rewritten. (Jay)
            ERROR_ReportError("MacDot11StationTransmitRTSFrame: "
                "Fragmentation not currently supported.\n");
        }
    }

    MacDot11StationSetState(node, dot11, DOT11_X_RTS);

    dot11->rtsPacketsSentDcf++;

#if 0
    StatsDb* db = node->partitionData->statsDb;
    if (db->statsAggregateTable->createMacAggregateTable)
    {
        dot11->aggregateStats->controlFramesSent++;
        dot11->aggregateStats->controlBytesSent +=
            DOT11_LONG_CTRL_FRAME_SIZE;
    }
#endif

    if (!dot11->useDvcs) {
        MacDot11StationStartTransmittingPacket(
            node, dot11, pktToPhy, dot11->delayUntilSignalAirborn);
    }
    else {
        BOOL isCached = FALSE;
        double directionToSend = 0.0;

        AngleOfArrivalCache_GetAngle(
            &dot11->directionalInfo->angleOfArrivalCache,
            destAddr,
            node->getNodeTime(),
            &isCached,
            &directionToSend);

        if (isCached) {
            MacDot11StationStartTransmittingPacketDirectionally(
                node,
                dot11,
                pktToPhy, dot11->delayUntilSignalAirborn,
                directionToSend);
            PHY_LockAntennaDirection(node, dot11->myMacData->phyNumber);
        }
        else {
            // Don't know which way to send, send omni.

            MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy,
                                             dot11->delayUntilSignalAirborn);
        }
    }

}// MacDot11StationTransmitRTSFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitCTSFrame.
//  PURPOSE:     Transmit a CTS frame.
//  PARAMETERS:  Node* node
//                  Pointer to node that is transmitting CTS frame
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* Rts
//                  Pointer holding the RTS
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationTransmitCTSFrame(Node* node, MacDataDot11* dot11,
                                      Message* Rts)
{
    Mac802Address destAddr;
    Message* pktToPhy;
    DOT11_LongControlFrame* hdr =
        (DOT11_LongControlFrame*)MESSAGE_ReturnPacket(Rts);
    DOT11_ShortControlFrame* newHdr;

/*********HT START*************************************************/
    int dataRateType = 0;
    MAC_PHY_TxRxVector rxVector;
    
    if (dot11->isHTEnable)
    {
        PHY_GetRxVector(node,
                        dot11->myMacData->phyNumber,
                        rxVector);
        rxVector.length = DOT11_SHORT_CTRL_FRAME_SIZE;
    }
    else
    {
        dataRateType= PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);
    }
/*********HT END****************************************************/

    destAddr = hdr->sourceAddr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        DOT11_SHORT_CTRL_FRAME_SIZE,
                        TRACE_DOT11);

    newHdr = (DOT11_ShortControlFrame *) MESSAGE_ReturnPacket(pktToPhy);
    memset(newHdr, 0, sizeof(DOT11_ShortControlFrame));

    newHdr->frameType = DOT11_CTS;
    newHdr->destAddr = destAddr;

    // Subtract RTS transmit time.
/*********HT START*************************************************/
    // Check if length in rxVector = DOT11_SHORT_CTRL_FRAME_SIZE
    
    if (dot11->isHTEnable)
    {
        newHdr->duration = (UInt16)
            MacDot11NanoToMicrosecond(
                MacDot11MicroToNanosecond(hdr->duration)
                - dot11->extraPropDelay
                - dot11->sifs
                - PHY_GetTransmissionDuration(
                      node, dot11->myMacData->phyNumber,
                     rxVector));

            // Subtract off CTS transmit time from the total duration
            // to get the CTS timeout time.
            dot11->noResponseTimeoutDuration =
                MacDot11MicroToNanosecond(newHdr->duration)
                - dot11->extraPropDelay - dot11->sifs
                - PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    rxVector)
                - dot11->extraPropDelay
                + dot11->slotTime + EPSILON_DELAY;
    }
    else
    {
    newHdr->duration = (UInt16)
        MacDot11NanoToMicrosecond(
            MacDot11MicroToNanosecond(hdr->duration)
            - dot11->extraPropDelay
            - dot11->sifs
            - PHY_GetTransmissionDuration(
                  node, dot11->myMacData->phyNumber,
                  dataRateType,
                  DOT11_SHORT_CTRL_FRAME_SIZE));

    // Subtract off CTS transmit time from the total duration
    // to get the CTS timeout time.
    dot11->noResponseTimeoutDuration =
        MacDot11MicroToNanosecond(newHdr->duration)
        - dot11->extraPropDelay - dot11->sifs
        - PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dataRateType,
            DOT11_SHORT_CTRL_FRAME_SIZE)
        - dot11->extraPropDelay
        + dot11->slotTime + EPSILON_DELAY;
    }
/*********HT END****************************************************/

    MacDot11StationSetState(node, dot11, DOT11_X_CTS);

    dot11->ctsPacketsSentDcf++;

#if 0
    StatsDb* db = node->partitionData->statsDb;
    if (db->statsAggregateTable->createMacAggregateTable)
    {
        dot11->aggregateStats->controlFramesSent++;
        dot11->aggregateStats->controlBytesSent +=
            DOT11_SHORT_CTRL_FRAME_SIZE;
    }
#endif

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           DOT11_SHORT_CTRL_FRAME_SIZE,
                           DOT11_CTS, 
                           destAddr, 
                           &rxVector);
        PHY_SetTxVector(node, 
                        dot11->myMacData->phyNumber,
                        tempTxVector);
    }
    else
    {
    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        MIN(dataRateType, dot11->highestDataRateTypeForBC));
    }
/*********HT END****************************************************/

    if (!dot11->useDvcs) {
        MacDot11StationStartTransmittingPacket(
            node, dot11, pktToPhy, dot11->sifs);
    }
    else
    {
        MacDot11StationStartTransmittingPacketDirectionally(
            node,
            dot11,
            pktToPhy,
            dot11->sifs,
            MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(
            node,
            dot11));

        PHY_LockAntennaDirection(node, dot11->myMacData->phyNumber);
    }
}// MacDot11StationTransmitCTSFrame


//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitAck
//  PURPOSE:     Transmit ACK for frames.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address detsAddr
//                  Node address of destination station
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationTransmitAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    Message* pktToPhy;
    DOT11_ShortControlFrame* newHdr;

/*********HT START*************************************************/
    int dataRateType;
    MAC_PHY_TxRxVector rxVector;
    if (dot11->isHTEnable)
    {
        PHY_GetRxVector(node,
                        dot11->myMacData->phyNumber,
                        rxVector);
    }
    else
    {
        dataRateType=
        PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);
    }
/*********HT END****************************************************/

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        DOT11_SHORT_CTRL_FRAME_SIZE,
                        TRACE_DOT11);

    newHdr = (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(pktToPhy);
    memset(newHdr, 0, sizeof(DOT11_ShortControlFrame));

    newHdr->frameType = DOT11_ACK;
    newHdr->destAddr = destAddr;

    newHdr->duration = 0;

    MacDot11StationSetState(node, dot11, DOT11_X_ACK);

    dot11->ackPacketsSentDcf++;

#if 0
    StatsDb* db = node->partitionData->statsDb;
    if (db->statsAggregateTable->createMacAggregateTable)
    {
        dot11->aggregateStats->controlFramesSent++;
        dot11->aggregateStats->controlBytesSent +=
            DOT11_SHORT_CTRL_FRAME_SIZE;
    }
#endif

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           DOT11_SHORT_CTRL_FRAME_SIZE,
                           DOT11_ACK, 
                           destAddr, 
                           &rxVector);
        PHY_SetTxVector(node, 
                        dot11->myMacData->phyNumber,
                        tempTxVector);
    }
    else
    {
        PHY_SetTxDataRateType(node,
                              dot11->myMacData->phyNumber,
                              dataRateType);
    }
/*********HT END****************************************************/

    if (!dot11->useDvcs) {
        MacDot11StationStartTransmittingPacket(
            node,
            dot11,
            pktToPhy,
            dot11->sifs);
    }
    else
    {
        MacDot11StationStartTransmittingPacketDirectionally(
            node, dot11, pktToPhy, dot11->sifs,
            MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(
            node,
            dot11));
    }
}// MacDot11StationTransmitAck
//--------------------------------------------------------------------------
//  NAME:        MacDot11ManagementIncrementSendFrame.
//  PURPOSE:     Transmit the data frame.
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void
MacDot11ManagementIncrementSendFrame(Node* node,
                                     MacDataDot11* dot11,
                                     DOT11_MacFrameType frameType);
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitDataFrame.
//  PURPOSE:     Transmit the data frame.
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------

static //inline//
void MacDot11StationTransmitDataFrame(
    Node* node,
    MacDataDot11* dot11)
{
    Mac802Address destAddr;
 
/*********HT START*************************************************/
    int dataRateType;
    MAC_PHY_TxRxVector txVector;
/*********HT END****************************************************/


//---------------------DOT11e--Updates------------------------------------//

    BOOL sendQOSFrame = FALSE;
    BOOL ARPPacket = FALSE;

//--------------------DOT11e-End-Updates----------------------------------//
    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11StationTransmitDataFrame: "
        "There is no message in the local buffer.\n");

    destAddr = dot11->currentNextHopAddress;
    if (dot11->dataRateInfo == NULL)
    {
        dot11->dataRateInfo =
            MacDot11StationGetDataRateEntry(
                node,
                dot11,
                dot11->currentNextHopAddress);
    }
    ERROR_Assert(dot11->currentNextHopAddress ==
                 dot11->dataRateInfo->ipAddress,
                 "Address does not match");

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        ERROR_Assert(FALSE,"HT Disabled");
    }
    else
    {
    PHY_SetTxDataRateType(node, dot11->myMacData->phyNumber,
                          dot11->dataRateInfo->dataRateType);
    dataRateType = PHY_GetTxDataRateType(node, dot11->myMacData->phyNumber);
    }
/*********HT END****************************************************/

    ERROR_Assert(dot11->currentMessage != NULL,
                 "MacDot11StationTransmitDataFrame: "
                 "There is no message in the local buffer.\n");

    if (destAddr == ANY_MAC802) {
        Message* dequeuedPacket;
        DOT11_FrameHdr* hdr;
//---------------------------Power-Save-Mode-Updates---------------------//
        if (MacDot11IsIBSSStationSupportPSMode(dot11)){
            dequeuedPacket = MESSAGE_Duplicate(node, dot11->currentMessage);
        }
        else{
            dequeuedPacket = dot11->currentMessage;
        }
//---------------------------Power-Save-Mode-End-Updates-----------------//


//---------------------DOT11e--Updates------------------------------------//

        // Broadcast messages are not sent using QoS Frame Header

//--------------------DOT11e-End-Updates---------------------------------//
        hdr = (DOT11_FrameHdr*)MESSAGE_ReturnPacket(dequeuedPacket);
        if (!dot11->beaconSet){
            hdr->duration = 0;
        }
        if (MacDot11IsAp(dot11) && dot11->dot11TxFrameInfo != NULL){
            if (dot11->dot11TxFrameInfo->frameType == DOT11_BEACON)
            {
                if (DEBUG_BEACON)
                {
                     char timeStr[100];
                     ctoa(node->getNodeTime(), timeStr);
                     printf("\n%d Transmitting Beacon "
                           "\t  at %s n",
                            node->nodeId,
                            timeStr);
                }
             MacDot11StationSetState(node, dot11, DOT11_X_BEACON);
            }
            else
            {
             MacDot11StationSetState(node, dot11, DOT11_X_BROADCAST);
            }
        }
        else
        {
            MacDot11StationSetState(node, dot11, DOT11_X_BROADCAST);
        }

        ERROR_Assert(dataRateType == dot11->highestDataRateTypeForBC,
                     "Wrong data rate");

        if (MacDot11IsAPSupportPSMode(dot11))
        {
            if (dot11->apVars->isBroadcastPacketTxContinue == TRUE)
            {
                hdr->frameFlags |= DOT11_MORE_DATA_FIELD;
            }
            else
            {
                hdr->frameFlags &= 0xDF;
            }
        }

        MacDot11StationStartTransmittingPacket(node, dot11, dequeuedPacket,
            dot11->delayUntilSignalAirborn);
    }
    else {
        clocktype transmitDelay = 0;
        DOT11_SeqNoEntry *entry;
        DOT11_FrameHdr* hdr;
        DOT11e_FrameHdr* Qhdr;
        Message* pktToPhy = MESSAGE_Duplicate(node, dot11->currentMessage);

//---------------------DOT11e--Updates------------------------------------//
#ifdef CYBER_LIB
            BOOL isEncrypt = FALSE;
            if (node->macData[dot11->myMacData->interfaceIndex]->encryptionVar
               != NULL)
            {
                MAC_SECURITY* secType = (MAC_SECURITY*)
                    node->macData[dot11->myMacData->interfaceIndex]
                    ->encryptionVar;

                if (*secType == MAC_SECURITY_WEP)
                {
                    EncryptionData* encData = (EncryptionData*)
                        node->macData[dot11->myMacData->interfaceIndex]
                        ->encryptionVar;
                        if (dot11->dot11TxFrameInfo!= NULL &&
                        (dot11->dot11TxFrameInfo->frameType == DOT11_DATA))
                        {
                            MESSAGE_RemoveHeader(node,
                             pktToPhy,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);
                        }

                        if (LlcIsEnabled(node, dot11->myMacData->interfaceIndex))
                        {
                            LlcHeader* llc;
                            llc = (LlcHeader*) MESSAGE_ReturnPacket(pktToPhy);
                            if (llc && llc->etherType == PROTOCOL_TYPE_ARP)
                            {
                                ARPPacket = TRUE;
                            }
                        }
                        if (dot11->dot11TxFrameInfo!= NULL &&
                            (dot11->dot11TxFrameInfo->frameType == DOT11_DATA)&&
                            (!ARPPacket))
                        {
                    isEncrypt =
                        WEPEncryptPacket(node, pktToPhy,
                                         dot11->myMacData->interfaceIndex,
                                         destAddr);
                        }
                    if (dot11->dot11TxFrameInfo!= NULL &&
                        (dot11->dot11TxFrameInfo->frameType == DOT11_DATA))
                        {
                            MESSAGE_AddHeader(node,
                             pktToPhy,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);

                            MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                                        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy),
                                                        dot11->dot11TxFrameInfo->RA,
                                                        dot11->dot11TxFrameInfo->frameType);

                        }
                    if (isEncrypt)
                    {
                        transmitDelay += encData->processingDelay;
                    }
                }
                else if (*secType == MAC_SECURITY_CCMP)
                {
                    EncryptionData* encData = (EncryptionData*)
                        node->macData[dot11->myMacData->interfaceIndex]
                        ->encryptionVar;
                        if (dot11->dot11TxFrameInfo!= NULL &&
                        (dot11->dot11TxFrameInfo->frameType == DOT11_DATA ))
                        {
                            MESSAGE_RemoveHeader(node,
                             pktToPhy,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);
                        }
                         if (LlcIsEnabled(node, dot11->myMacData->interfaceIndex))
                        {
                             LlcHeader* llc;
                             llc = (LlcHeader*) MESSAGE_ReturnPacket(pktToPhy);
                             if (llc && llc->etherType == PROTOCOL_TYPE_ARP)
                             {
                                  ARPPacket = TRUE;
                             }
                        }
                        if (dot11->dot11TxFrameInfo!= NULL &&
                        (dot11->dot11TxFrameInfo->frameType == DOT11_DATA)&&
                        (!ARPPacket))
                        {
                    isEncrypt =
                        CCMPEncryptPacket(node, pktToPhy,
                                          dot11->myMacData->interfaceIndex,
                                          destAddr);
                        }
                    if (dot11->dot11TxFrameInfo!= NULL &&
                        (dot11->dot11TxFrameInfo->frameType == DOT11_DATA))
                        {
                            MESSAGE_AddHeader(node,
                             pktToPhy,
                             sizeof(DOT11_FrameHdr),
                             TRACE_DOT11);

                            MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                                        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy),
                                                        dot11->dot11TxFrameInfo->RA,
                                                        dot11->dot11TxFrameInfo->frameType);

                        }
                    if (isEncrypt)
                    {
                        transmitDelay += encData->processingDelay;
                    }
                }
                else
                {
                    ERROR_Assert(FALSE, "Unknown Security Protocol");
                }
            }
#endif  // CYBER_LIB
            // For data frames, need to add header.
#ifdef CYBER_LIB
if ((dot11->txFrameInfo != NULL
            && dot11->txFrameInfo->frameType != DOT11_DATA))
{
                if (isEncrypt)
                {
                    ERROR_ReportError(
                        "MacDot11StationTransmitDataFrame: "
                        "802.11s does not currently support "
                        "MAC security.\n");
                }
}
else if (MacDot11IsQoSEnabled(node, dot11)&& isEncrypt)
{
    ERROR_ReportError(
                   "MacDot11StationTransmitDataFrame: "
                   "802.11e does not currently support "
                   "MAC security.\n");
}
#endif  // CYBER_LIB

                // For mesh data frames, MAC header has been added.
                // Need to fill in duration and sequence number.
                // For required fields, DOT11_FrameHdr matches
                // DOT11s_FrameHdr.
                hdr = (DOT11_FrameHdr*)MESSAGE_ReturnPacket(pktToPhy);

                if (MacDot11IsAPSupportPSMode(dot11))
                {
                    if (dot11->isMoreDataPresent == TRUE)
                    {
                        hdr->frameFlags |= DOT11_MORE_DATA_FIELD;
                    }
                    else
                    {
                        hdr->frameFlags &= 0xDF;
                    }
                }


#ifdef CYBER_LIB
            if (isEncrypt)
            {
                ERROR_Assert(dot11->isMP == FALSE,
                    "MacDot11StationTransmitDataFrame: "
                    "MAC security not supported for 802.11s.\n");

                hdr->frameFlags |= DOT11_PROTECTED_FRAME;
            }
#endif  // CYBER_LIB

    if (dot11->dot11TxFrameInfo!= NULL)
    {
        if ((dot11->dot11TxFrameInfo->frameType == DOT11_DATA) ||
            ((dot11->dot11TxFrameInfo->frameType >=DOT11_ASSOC_REQ) &&
            (dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)) ||
            (dot11->dot11TxFrameInfo->frameType == DOT11_PS_POLL))
            {
                MacDot11ManagementIncrementSendFrame(
                    node,
                    dot11,
                    dot11->dot11TxFrameInfo->frameType);

                dot11->TID = 0;
                entry = MacDot11StationGetSeqNo(node, dot11, destAddr);
                dot11->QosFrameSent = FALSE;
            }
        else
            {

                Qhdr = (DOT11e_FrameHdr*)hdr;

                 dot11->TID = Qhdr->qoSControl.TID;
                 sendQOSFrame = TRUE;
                 Qhdr->fragId = 0;

            entry = MacDot11StationGetSeqNo(node,
                                            dot11,
                                            destAddr,
                                            dot11->TID);
            dot11->QosFrameSent = TRUE;

        }
    }
        else
        {
        //ps mode
        dot11->TID = 0;
            entry = MacDot11StationGetSeqNo(node, dot11, destAddr);
                dot11->QosFrameSent = FALSE;
        }

//--------------------DOT11e-End-Updates---------------------------------//

        dot11->waitingForAckOrCtsFromAddress = destAddr;
        if (hdr->frameType != DOT11_PS_POLL)
        {

           ERROR_Assert(entry,
              "MacDot11StationTransmitDataFrame: "
              "Unable to find or create sequence number entry.\n");
           hdr->seqNo = entry->toSeqNo;
        }

        hdr->duration = (UInt16)
            MacDot11NanoToMicrosecond(
                dot11->extraPropDelay + dot11->sifs +
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    DOT11_SHORT_CTRL_FRAME_SIZE) +
                dot11->extraPropDelay);

        int fragThreshold = 0;
           fragThreshold = DOT11_FRAG_THRESH;
        if (MESSAGE_ReturnPacketSize(dot11->currentMessage) >
            fragThreshold)
        {
/*********HT END****************************************************/
            // Fragmentation Needs to be rewritten. (Jay)
            ERROR_ReportError("MacDot11StationTransmitDataFrame: "
                "Fragmentation is not currently supported.\n");
        }
        else {
             if (hdr->frameType == DOT11_PS_POLL){
                    if (DEBUG_PS && DEBUG_PS_PSPOLL){
                        MacDot11Trace(node, dot11, NULL,
                            "Station Transmitted PS POLL Frame");
                    }
                    MacDot11StationSetState(node, dot11, DOT11_X_PSPOLL);
              }
              else
                MacDot11StationSetState(node, dot11, DOT11_X_UNICAST);

            // RTS Threshold should be applied on Mac packet
            if ((MESSAGE_ReturnPacketSize(pktToPhy)>
                dot11->rtsThreshold)&&
               (hdr->frameType != DOT11_PS_POLL))
            {
                // Since using RTS-CTS, data packets have to wait
                // an additional SIFS.
                transmitDelay += dot11->sifs;
            }
            else {
                // Not using RTS-CTS, so don't need to wait for SIFS
                // since already waited for DIFS or BO.
                transmitDelay += dot11->delayUntilSignalAirborn;
            }

            if (!dot11->useDvcs) {
                MacDot11StationStartTransmittingPacket(
                    node, dot11, pktToPhy, transmitDelay);
            }
            else {
                BOOL isCached = FALSE;
                double directionToSend = 0.0;

                AngleOfArrivalCache_GetAngle(
                    &dot11->directionalInfo->angleOfArrivalCache,
                    destAddr,
                    node->getNodeTime(),
                    &isCached,
                    &directionToSend);

                if (isCached) {
                    MacDot11StationStartTransmittingPacketDirectionally(
                        node, dot11, pktToPhy, transmitDelay,
                        directionToSend);

                    PHY_LockAntennaDirection(node,
                                             dot11->myMacData->phyNumber);
                }
                else {
                    // Don't know which way to send, send omni.
                    MacDot11StationStartTransmittingPacket(
                        node, dot11, pktToPhy, transmitDelay);
                }
            }
        }
    }
}// MacDot11StationTransmitDataFrame


#endif /*MAC_DOT11_STA_H*/
