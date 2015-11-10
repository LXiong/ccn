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
 * \file mac_dot11-mib.cpp
 * \brief MIB routines.
 */

#include "api.h"
#include "mac_dot11.h"
#include "mac_dot11-mib.h"

//-------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------


//--------------------------------------------------------------------------
//  NAME:        MacDot11MibGetSSID
//  PURPOSE:     Get stations SSID.
//
//  PARAMETERS:  const MacDataDot11 * dot11
//                  Pointer to Dot11 structure
//  RETURN:      SSID
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
const char * MacDot11MibGetSSID(
    const MacDataDot11* dot11)
{
    return &(dot11->stationMIB->dot11DesiredSSID[0]);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11MibSetSSID
//  PURPOSE:     Set station SSID.
//
//  PARAMETERS:  const MacDataDot11 * dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11MibSetSSID(
    const MacDataDot11* dot11,
    const char * ssid)
{
    memcpy((void *)dot11->stationMIB->dot11DesiredSSID, ssid, sizeof(ssid));

}

//--------------------------------------------------------------------------
//  NAME:        MacDot11MibGetMib
//  PURPOSE:     Get a MIB parameter.
//
//  PARAMETERS:  const MacDataDot11 * dot11
//                  Pointer to Dot11 structure
//  RETURN:      MIB parameter
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
const char * MacDot11MibGetMib(
    const MacDataDot11* const dot11)
{
    return "";
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11MibSetMib
//  PURPOSE:     Set a MIB parameter.
//
//  PARAMETERS:  const MacDataDot11 * dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11MibSetMib(
    const MacDataDot11* const dot11)
{
    return;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11MibInit
//  PURPOSE:    Init MIB parameters.
//
//  PARAMETERS:  Node node
//                  Pointer to node
//               const NodeInput* nodeInput
//                  Pointer to node input
//               const MacDataDot11 * dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11MibInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType)
{

    char retString[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    Address address;

    DOT11_StationConfigTable* stationMIB =
       (DOT11_StationConfigTable*) MEM_malloc(sizeof(DOT11_StationConfigTable));

    memset(stationMIB, 0, sizeof(DOT11_StationConfigTable));

    dot11->stationMIB = stationMIB;

    // SSID
    // Format is :
    //      MAC-DOT11-SSID <value>
    // Default is MAC-DOT11-SSID (value NULL)
    //
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
        "MAC-DOT11-SSID",
        &wasFound,
        retString);

    if (!wasFound) {
        strcpy(dot11->stationMIB->dot11DesiredSSID, DOT11_DEFAULT_SSID);
    }
    else {
        ERROR_Assert(strlen(retString) < DOT11_SSID_MAX_LENGTH,
                        "MAC-DOT11-SSID value is too big");
        strcpy(dot11->stationMIB->dot11DesiredSSID, retString);
    }

    dot11->stationMIB->dot11BeaconPeriod =
        MacDot11ClocktypeToTUs(dot11->beaconInterval);

    dot11->stationMIB->dot11StationID = dot11->selfAddr;

}

