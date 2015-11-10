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
 * \file mac_dot11-hcca.h
 * \brief Hybrid Controller structures and inline routines.
 *
 */

#ifndef MAC_DOT11_HCCA_H
#define MAC_DOT11_HCCA_H

#include "mac_dot11.h"
#include "mac_dot11-sta.h"
#include "api.h"

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListIsEmpty
//  PURPOSE:     See if the HCCA poll list of particular TSID is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//                  TSID value for which list of STA has to be checked
//  RETURN:      TRUE if HCCA poll list size is zero
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
bool MacDot11eHCPollListIsEmpty(
     Node* node,
     MacDataDot11* dot11);
//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcHasSufficientTime
//  PURPOSE:     Determine if PC has sufficient time to send a frame type.
//               The frame type determines the frame exchange sequence
//               that would occur. If time was insufficient, the PC would
//               attempt to send a CF-End to terminate the CFP.
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
//              int dataRateType
//                  Data rate type
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
    int dataRateType);

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
    MacDataDot11* dot11);

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
    MacDataDot11* dot11);
//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcPollListResetAtStartOfBeacon
//  PURPOSE:     Reset values of the poll list at start of CFP
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11eHcPollListResetAtStartOfBeacon(
    Node* node,
     MacDataDot11* dot11);
//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHcPrintStats
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
    int interfaceIndex);

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
    int interfaceIndex);

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
    clocktype delay);


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
    Mac802Address destAddr);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcHandleNullData
//  PURPOSE:     Station receives an ack as part of CF-End/Data/DataPoll/Poll.
//               Station cannot get a pure ack.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address of frame containing ack
//              Message *msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHcHandleNullData(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    Message *msg);

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
    int interfaceIndex);

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
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11eHcPollListUpdateForUnicastDataReceived(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address sourceAddr);

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
     int TSIdentifier);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcSendsPolls
//  PURPOSE:     Determine if HC delivery mode is one in which it sends
//               polls and has a polling list.
//
//  PARAMETERS:  DOT11e_HcVars* const hc
//                  HC structure
//  RETURN:      TRUE if HC operates in one of the polling modes
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11eCfpHcSendsPolls(
    const DOT11e_HcVars* const hc);

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
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCDownLinkListIsEmpty
//  PURPOSE:     Find if downlink list is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapDownlinkPollList* station
//                  Poiner to the poll list
//  RETURN:      TRUE if list is empty
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11eHCDownLinkListIsEmpty(
    Node* node,
    MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList* station);

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
    int TSIdentifier);

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
    int TsIdentifier );

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
//                  List item to remove
//               int TSIdentifier
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
    int TSIdentifier);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollListFree
//  PURPOSE:     Free the entire list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapPollList* list
//                  Pointer to polling list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHCPollListFree(
     Node* node,
     MacDataDot11* dot11,
     DOT11e_HCCA_CapPollList* list);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetFirstStation
//  PURPOSE:     Retrieve the first item in the poll list
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int TSIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      First list item
//               NULL for empty list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
Dot11e_HCCA_cappollListStationItem * MacDot11eHCPollTSIDListGetFirstStation(
     Node* node,
     MacDataDot11* dot11,
     int TsIdentifier);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetNextStation
//  PURPOSE:     Retrieve the next item given the previous item.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Dot11e_HCCA_cappollListStationItem *prev
//                  Pointer to prev item.
//               int TSIdentifier
//                  TSID value for which list of STA has to be checked
//  RETURN:      First item when previous item is NULL
//                          when previous item is the last
//                          when there is only one item in the list
//               NULL if the list is empty
//               Next item after previous item
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static inline
Dot11e_HCCA_cappollListStationItem * MacDot11eHCPollTSIDListGetNextStation(
    Node* node,
    MacDataDot11* dot11,
    Dot11e_HCCA_cappollListStationItem *prev,
    int TsIdentifier);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCPollTSIDListGetNextStationToPoll
//  PURPOSE:     Retrieve the next station to poll for a given TSID
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int TsIdentifier
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
    int TsIdentifier);

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
//                  sent by the HC was a beacon, a broadcast, or
//                  there was a timeout
//               clocktype delay
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
    clocktype timerDelay);

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
//               DOT11_MacFrameType* frameToSend
//                  Frame to send
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
    int* dataRateType);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpStationSetTransmitState
//  PURPOSE:     Set the state of the BSS station based on the frame
//               about to be transmitted during CAP.
//
//  PARAMETERS:  MacDataDot11* const dot11
//                  poinetr to dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationSetTransmitState(
     MacDataDot11* const dot11,
    DOT11_MacFrameType frameType);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHcDownlinkListInit
//  PURPOSE:     Initialize the HCCA poll link list data structure.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapPollList **list
//                  Pointer to HCCA polling list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHcDownlinkListInit(
    Node* node,
     MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList** list);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eHCDownLinkListInsert
//  PURPOSE:     Insert an item to the end of the list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11e_HCCA_CapDownlinkPollList** station
//                  Pointer to polling list
//               Mac802Address sourceAddr
//                  Poll station data for item to insert
//               Message* msg
//                  Message received
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eHCDownLinkListInsert(
    Node* node,
    MacDataDot11* dot11,
    DOT11e_HCCA_CapDownlinkPollList** station,
    Mac802Address sourceAddr,
    Message* msg);

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
// RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eHCInit(
    Node* node,
    MacDataDot11* dot11,
    PhyModel phyModel,
    const NodeInput *nodeInput,
    NetworkType networkType = NETWORK_IPV4,
   in6_addr* ipv6SubnetAddr = 0,
   unsigned int prefixLength = 0);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHcSetPostTransmitState
//  PURPOSE:     Set that state of HC during CAP after frame
//               transmission is complete.
//
//               Also reset ack flag is the sent frame was an ack type.
//
//  PARAMETERS:  Node* node
//                  Pointer to the node
//               MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpHcSetPostTransmitState(Node* node,
     MacDataDot11* const dot11 );

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
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpAdjustFrameTypeForAck
//  PURPOSE:     Adjust the HCCA frame type in case an ack is to
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
    DOT11_MacFrameType* const frameType);

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
    DOT11e_CapStates state);

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
     MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11eCfpHandleCapEnd
//  PURPOSE:     Handle a timer if a PC was unable to send a CF End
//               or a STA did not receive a CF End
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11eCfpHandleCapEnd(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationTransmit
//  PURPOSE:     Determine the frame to transmit based on timer delay,
//               available balance CFP duration and received frame type.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType receivedFrameType
//                  Received frame type
//               clocktype timerDelay
//                  Delay to be sent
//              int tsid
//                  Traffic stream ID
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType receivedFrameType,
    clocktype timerDelay,
    int tsid);

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
//              int tsid
//                  Traffic stream ID
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11eCfpStationTransmitUnicast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay,
    int tsid);

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
    MacDataDot11* dot11);

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
       int tsId);

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
        Message* msg);

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
    Message* msg);

// -------------------------------------------------------------------------
// FUNCTION   :: MacDot11IsTSsHasMessage
// LAYER      :: MAC
// PURPOSE    ::
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + dot11     : MacDataDot11* : Pointer to the MacData structure
// RETURN     :: BOOL : TRUE or FALSE
// -------------------------------------------------------------------------
BOOL MacDot11IsTSsHasMessage(Node* node,
                             MacDataDot11* dot11);

// -------------------------------------------------------------------------
// FUNCTION   :: MacDot11IsTSHasMessageforTSID
// LAYER      :: MAC
// PURPOSE    ::
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + dot11     : MacDataDot11* : Pointer to the MacData structure
// + tsid      : int : TS ID
// RETURN     :: BOOL : TRUE or FALSE
// -------------------------------------------------------------------------
BOOL MacDot11IsTSHasMessageforTSID(Node* node,
                                   MacDataDot11* dot11,
                                   int tsid);

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
BOOL MacDot11eIsTSBuffreFull(Node* node,
                             MacDataDot11* dot1,
                             int priority);

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
    Mac802Address nextHop);

#endif
