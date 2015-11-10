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
 * \file mac_dot11-pc.h
 * \brief Point Controller structures and inline routines.
 *
 */

#ifndef MAC_DOT11_PC_H
#define MAC_DOT11_PC_H

#include "api.h"
#include "mac_dot11.h"
#include "mac_dot11-ap.h"
//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------

//--------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------------
BOOL MacDot11CfPollListIsEmpty(Node* node,
     MacDataDot11* dot11,DOT11_CfPollList* list);

void MacDot11CfPollListItemRemove(Node* node,
     MacDataDot11* dot11,DOT11_CfPollList* list,
    DOT11_CfPollListItem* listItem);

DOT11_CfPollListItem* MacDot11CfPollListGetItemWithGivenAddress(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address lookupAddress);

void MacDot11CfpPcBeaconTransmitted(
    Node* node, MacDataDot11* dot11);

void MacDot11CfpFrameTransmitted(
    Node* node, MacDataDot11* dot11);

void MacDot11CfpCancelSomeTimersBecauseChannelHasBecomeBusy(
    Node* node, MacDataDot11* dot11);

void MacDot11CfpPhyStatusIsNowIdleStartSendTimers(
    Node* node, MacDataDot11* dot11);

void MacDot11CfpReceivePacketFromPhy(
    Node* node, MacDataDot11* dot11, Message* msg);

void MacDot11CfpStationInitiateCfPeriod(
    Node* node, MacDataDot11* dot11);

void MacDot11ProcessCfpBeacon(
    Node* node, MacDataDot11* dot11, Message* msg);

void MacDot11ProcessCfpEnd(
    Node* node, MacDataDot11* dot11, Message* msg);

void MacDot11CfpHandleTimeout(
    Node* node, MacDataDot11* dot11);

void MacDot11CfpHandleEndTimer(
    Node* node, MacDataDot11* dot11);

void MacDot11cfPcPollListAddStation(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address stationAddress
    );

void MacDot11CfpInit(
    Node* node, const NodeInput* nodeInput, MacDataDot11* dot11,
    SubnetMemberData* subnetList, int nodesInSubnet,
    int subnetListIndex, NodeAddress subnetAddress, int numHostBits,
    NetworkType networkType = NETWORK_IPV4,
    in6_addr* ipv6SubnetAddr = 0,
    unsigned int prefixLength = 0);

void MacDot11CfpPrintStats(
    Node* node, MacDataDot11* dot11, int interfaceIndex);

void MacDot11CfpFreeStorage(
    Node* node, int interfaceIndex);

void MacDot11CfPollListUpdateForUnicastReceived(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr);

void MacDot11CfpPcCheckBeaconOverlap(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

//--------------------------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpSetState
//  PURPOSE:     Set the CFP state. Save previous state.
//  PARAMETERS:  MacDataDot11 * const dot11
//                  Pointer to dot11 structure.
//               DOT11_CfpStates state
//                  New CFP state to be set
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpSetState(
    MacDataDot11* const dot11,
    DOT11_CfpStates state)
{
    dot11->cfpPrevState = dot11->cfpState;
    dot11->cfpState = state;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11IsInCfp
//  PURPOSE:     Determine if within contention free period.
//               Used to handle events accordingly.
//
//  PARAMETERS:  DOT11_MacStates state
//                  Current DCF state
//  RETURN:      TRUE if within contention free period
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11IsInCfp(
    DOT11_MacStates state)
{
    return (state == DOT11_CFP_START);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcSendsPolls
//  PURPOSE:     Determine if PC delivery mode is one in which it sends
//               polls and has a polling list.
//
//  PARAMETERS:  const DOT11_PcVars * const pc
//                  PC structure
//  RETURN:      TRUE if PC operates in one of the polling modes
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpPcSendsPolls(
    const DOT11_PcVars* const pc)
{
    ERROR_Assert(pc != NULL,
        "MacDot11CfpPcSendsPolls: PC variable is NULL.\n");

    return ( (pc->deliveryMode == DOT11_PC_POLL_AND_DELIVER) ||
             (pc->deliveryMode == DOT11_PC_POLL_ONLY) );
}


#endif /*MAC_DOT11_PC_H*/
