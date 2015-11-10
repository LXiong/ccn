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


#ifndef MAC_GVRP_H
#define MAC_GVRP_H

#include "mac_garp.h"

// The GVRP (GARP VLAN Registration Protocol) implementation
// follows the sample code of IEEE 802.1Q, 1998.
// The principal difference in approach is that end-stations
// do not participate in joins and leaves. Such joins/leaves
// originate from the switch port itself through user
// specification of the port VLAN IDs.


// /**
// CONSTANT    :: SWITCH_GVRP_VLANS_MAX : 10
// DESCRIPTION :: Default value of maximum VLANs kept by database.
// **/
//
// This value indicates the maximum number of VLANs that
// could be part of the member set of the switch.
// This value can also be modified via user input.
#define SWITCH_GVRP_VLANS_MAX               10

// /**
// CONSTANT    :: GVRP_UNUSED_GID_INDEX : SWITCH_VLAN_ID_MAX + 1
// DESCRIPTION :: Index of unused GID machine.
// **/
#define GVRP_UNUSED_GID_INDEX               (SWITCH_VLAN_ID_MAX + 1)



// /**
// CONSTANT    :: GVRP_FRAME_PRIORITY : SWITCH_FRAME_PRIORITY_MAX
// DESCRIPTION :: Priority of a GARP PDU.
// **/
#define GVRP_FRAME_PRIORITY                 SWITCH_FRAME_PRIORITY_MAX

#define GVRP_BASE_VALUE                     256

#define GVRP_PRINT_STATS_DEFAULT            FALSE

// /**
// ENUM        :: GvrpAttributeType
// DESCRIPTION :: Type of attributes used by GVRP.
// **/
typedef enum
{
    GVRP_ALL_ATTRIBUTES,
    GVRP_VLAN_ATTRIBUTE
} GvrpAttributeType;

// /**
// STRUCT      :: GvrpGvfMsg
// DESCRIPTION :: Fields of a VLAN attribute message.
// **/
typedef struct
{
    GvrpAttributeType attribute;
    unsigned char length;
    GarpGidEvent event;
    VlanId key1;
} GvrpGvfMsg;

// /**
// STRUCT      :: GvrpStat
// DESCRIPTION :: GVRP statistics variables.
// **/
typedef struct
{
    Mac802Address portAddr;
    unsigned joinEmptyReceived;
    unsigned joinInReceived;
    //unsigned emptyReceived;
    unsigned leaveInReceived;
    unsigned leaveEmptyReceived;
    unsigned leaveAllReceived;
    unsigned joinEmptyTransmitted;
    unsigned joinInTransmitted;
    //unsigned emptyTransmitted;
    unsigned leaveInTransmitted;
    unsigned leaveEmptyTransmitted;
    unsigned leaveAllTransmitted;
} GvrpStat;

// /**
// STRUCT      :: SwitchGvrp
// DESCRIPTION :: GVRP variables.
// **/
struct SwitchGvrp_
{
    // An instance of GARP for this application.
    Garp garp;
    unsigned vlanId;

    // Internal VLAN database of GVRP.
    VlanId* gvd;
    unsigned numGvdEntries;
    unsigned lastGvdUsedPlus1;
    unsigned vlansMax;

    // Stats for each portGid
    unsigned numPorts;
    GvrpStat* stats;
    BOOL printStats;

    Mac802Address GVRP_Group_Address;
};


// -------------------------------------------------------------------------
// Interface functions

// /**
// FUNCTION   :: SwitchGvrp_GetGvrpData
// LAYER      :: Mac
// PURPOSE    :: Get GVRP data of a switch.
// PARAMETERS ::
// + sw        : MacSwitch*    : Pointer to switch data.
// RETURN     :: None.
// **/
SwitchGvrp* SwitchGvrp_GetGvrpData(
    MacSwitch* sw);

// /**
// FUNCTION   :: SwitchGvrp_EnablePort
// LAYER      :: Mac
// PURPOSE    :: GARP layer function.
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// + sw        : MacSwitch*  : Pointer to switch data.
// + thePort   : SwitchPort* : Pointer to a port.
// RETURN     :: None.
// **/
// Enable a disabled port in GVRP context.
void SwitchGvrp_EnablePort(
    Node* node,
    MacSwitch* sw,
    SwitchPort* thePort);

// /**
// FUNCTION   :: SwitchGvrp_DisablePort
// LAYER      :: Mac
// PURPOSE    :: Disable a Enabled port in GVRP context.
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// + sw        : MacSwitch*  : Pointer to switch data.
// + thePort   : SwitchPort* : Pointer to a port.
// RETURN     :: None.
// **/
void SwitchGvrp_DisablePort(
    Node* node,
    MacSwitch* sw,
    SwitchPort* thePort);

// /**
// FUNCTION   :: SwitchGvrp_Init
// LAYER      :: Mac
// PURPOSE    :: Initialize GVRP application.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + sw        : MacSwitch*       : Pointer to switch data.
// + nodeInput : const NodeInput* : Pointer to nodeInput.
// RETURN     :: None.
// **/
void SwitchGvrp_Init(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput);

// /**
// FUNCTION   :: SwitchGvrp_Finalize
// LAYER      :: Mac
// PURPOSE    :: Print stats and release allocated memory.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + sw        : MacSwitch*       : Pointer to switch data.
// RETURN     :: None.
// **/
void SwitchGvrp_Finalize(
    Node* node,
    MacSwitch* sw);

void SwitchGvrp_Trace(
    Node* node,
    GarpGid* portGid,
    Message *msg,
    const char* flag);

#endif // MAC_GVRP_H
