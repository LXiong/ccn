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
 * \file mac_dot11-ap.h
 * \brief Access Point structures and inline routines.
 */

#ifndef MAC_DOT11_AP_H
#define MAC_DOT11_AP_H

#include "api.h"
#include "dvcs.h"
#include "mac_dot11-sta.h"
#include "mac_dot11-pc.h"

//-------------------------------------------------------------------------
// #define's
#define DOT11_AP_ASSIGN_MAX_ASSOCIATION_Id  2047
//---------------------------Power-Save-Mode-Updates---------------------//
#define MacDot11IsAPSupportPSMode(dot11)    (MacDot11IsAp(dot11)\
    && (dot11->apVars->isAPSupportPSMode == TRUE))
#define MacDOT11APIsAnySTAInPSMode(dot11)   (MacDot11IsAp(dot11)\
    && (dot11->isAnySTASupportPSMode == TRUE))
//---------------------------Power-Save-Mode-End-Updates-----------------//

//-------------------------------------------------------------------------

//--------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------------
void MacDot11ApInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    SubnetMemberData* subnetList,
    int nodesInSubnet,
    int subnetListIndex,
    NodeAddress subnetAddress,
    int numHostBits,
    NetworkType networkType = NETWORK_IPV4,
    in6_addr* ipv6SubnetAddr = 0,
    unsigned int prefixLength = 0);

void MacDot11BeaconInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType);

void MacDot11ApProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

void MacDot11ApBeaconTransmitted(
    Node* node,
    MacDataDot11* dot11);

BOOL MacDot11CanTransmitBeacon(Node* node,
    MacDataDot11* dot11, int beaconFrameSize);

void MacDot11CfpParameterSetUpdate( Node* node,
    MacDataDot11* dot11  ) ;

void Dot11_CreateCfParameterSet(
                    Node* node,
                    DOT11_Ie* element,
                DOT11_CFParameterSet* cfSet);

void MacDot11AddQBSSLoadElementToBeacon( Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize);

void MacDot11AddEDCAParametersetToBeacon(Node* node,
                          MacDataDot11* dot11,
                          DOT11_Ie* element,
                          DOT11_MacFrame* txFrame,
                          int* txFrameSize);

void MacDot11AddHTCapability(Node* node,
                             MacDataDot11* dot11,
                             DOT11_Ie* element,
                             DOT11_MacFrame* txFrame,
                             int* txFrameSize);

void MacDot11AddHTOperation(Node* node,
                            MacDataDot11* dot11,
                            const DOT11_HT_OperationElement* operElem,
                            DOT11_MacFrame* txFrame,
                            int* txFrameSize);

void MacDot11AddTrafficSpecToframe(Node* node,
                MacDataDot11* dot11,
                DOT11_Ie* element,
                DOT11_MacFrame* txFrame,
                int* txFrameSize,
                TrafficSpec* TSPEC);

void MacDot11ApTransmitBeacon(Node* node,
                MacDataDot11* dot11);

void MacDot11DTIMupdate( Node* node,
    MacDataDot11* dot11  );

void MacDot11CfpCountUpdate(
            Node* node,
            MacDataDot11* dot11  );

BOOL MacDot11ApAttemptToTransmitBeacon(
    Node* node,
    MacDataDot11* dot11);

int MacDot11ApStationListAddStation(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address stationAddress,
    unsigned char qoSEnabled = 0,
    unsigned short listenInterval = 0,
    BOOL psModeEnabled = FALSE);

BOOL MacDOT11APBufferPacketAtSTAQueue(
    Node* node,
    MacDataDot11* dot11);

BOOL MacDot11ApStationListIsEmpty(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list);

DOT11_ApStationListItem * MacDot11ApStationListGetFirst(
    Node* node,
    MacDataDot11* dot11);

DOT11_ApStationListItem * MacDot11ApStationListGetNextItem(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationListItem *prev);

void MacDot11ApStationListItemRemove(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list,
    DOT11_ApStationListItem* listItem);
//---------------------------Power-Save-Mode-Updates---------------------//
BOOL MacDot11APDequeueUnicastPacket(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceNodeAddress);
//---------------------------Power-Save-Mode-End-Updates-----------------//
//-------------------------------------------------------------------------
// Inline Functions
//-------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  NAME:        MacDot11ApPrintStats
//  PURPOSE:     Print Ap related statistics.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int interfaceIndex
//                  interfaceIndex, Interface index
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11ApPrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{

//    char buf[MAX_STRING_LENGTH];

//    sprintf(buf, "Beacons received = %d",
//           dot11->beaconPacketsGot);
//    IO_PrintStat(node, "MAC", "DOT11AP", ANY_DEST, interfaceIndex, buf);

//    sprintf(buf, "Beacons sent = %d",
//           dot11->beaconPacketsSent);
//    IO_PrintStat(node, "MAC", "DOT11AP", ANY_DEST, interfaceIndex, buf);

}

//---------------------DOT11e--Updates------------------------------------//

//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListGetSize
//  PURPOSE:     Query the size of station list
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Number of items in station list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
int MacDot11ApStationListGetSize(
    Node* node,
    MacDataDot11* dot11);

// /**
// FUNCTION   :: MacDot11ApStationListGetItemWithGivenAddress
// LAYER      :: MAC
// PURPOSE    :: Retrieve the list item related to given address
// PARAMETERS ::
// + node      : Node* :Node Pointer
// + dot11     : MacDataDot11* : Structure of dot11.
// + lookupAddress : Mac802Address : Lookup Node Address
// RETURN     :: DOT11_ApStationListItem* :
// **/
DOT11_ApStationListItem* MacDot11ApStationListGetItemWithGivenAddress(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address lookupAddress);

//--------------------DOT11e-End-Updates---------------------------------//

#endif /*MAC_DOT11_AP_H*/
