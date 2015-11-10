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


#ifndef MAC_STP_H
#define MAC_STP_H

typedef struct SwitchPort_ SwitchPort;
typedef struct SwitchStpSm_ SwitchStpSm;

typedef unsigned short StpBpduTime;


// Port states
typedef enum
{
    STP_PORT_S_DISCARDING,
    STP_PORT_S_LEARNING,
    STP_PORT_S_FORWARDING
} StpPortState;

// Port roles
typedef enum
{
    STP_ROLE_UNKNOWN = 0,
    STP_ROLE_ALTERNATEORBACKUP,
    STP_ROLE_ROOT,
    STP_ROLE_DESIGNATED,
    STP_ROLE_DISABLED
} StpPortRole;

// BPDU info
typedef enum
{
    STP_INFOIS_MINE,
    STP_INFOIS_AGED,
    STP_INFOIS_RECEIVED,
    STP_INFOIS_DISABLED
} StpInfoIs;

// Received BPDU type
typedef enum
{
    STP_SUPERIOR_DESIGNATED_MSG,
    STP_REPEATED_DESIGNATED_MSG,
    STP_CONFIRMED_ROOT_MSG,
    STP_OTHER_MSG
} StpReceivedMsgType;

// Comparison of IDs, priority vectors, times
typedef enum
{
    STP_FIRST_IS_BETTER,
    STP_BOTH_ARE_SAME,
    STP_FIRST_IS_WORSE,
    STP_BOTH_ARE_DIFFERENT
} StpComparisonType;

// Switch ID consists of priority and address
typedef struct
{
    unsigned short priority;
    //unsigned short priority:4,
    //               vlanId:12;

    Mac802Address ipAddr; 
} StpSwitchId;

#define SWITCH_ID_PRIORITY_MASK             0xff00
#define SWITCH_ID_VLAN_MASK                 0x0000

// Port ID consists of priority and number
typedef struct
{
    unsigned char priority;
    unsigned char portNumber;
    //unsigned short priority:4,
    //               portNumber:12;
} StpPortId;

#define SWITCH_PORT_ID_PRIORITY_MASK        0xf000
#define SWITCH_PORT_ID_NUMBER_MASK          0x0fff

// Priority consists of first four components of vector
typedef struct
{
    StpSwitchId rootSwId;
    UInt32 rootCost;
    StpSwitchId swId;
    StpPortId txPortId;
} StpPriority;

typedef struct
{
    StpPriority stpPriority;
    StpPortId rxPortId;
} StpVector;

// Time values carried in a BPDU
typedef struct
{
    clocktype messageAge;
    clocktype maxAge;
    clocktype helloTime;
    clocktype forwardDelay;
} StpTimes;

// Administrative value for point to point links
typedef enum
{
    SWITCH_ADMIN_P2P_AUTO,
    SWITCH_ADMIN_P2P_FORCE_TRUE,
    SWITCH_ADMIN_P2P_FORCE_FALSE
} SwitchPortAdminP2PType;

// -------------------------------------------------------
//  STP frame formats

#define STP_BPDU_ID                     0x0000

#define STP_PROTOCOL_VERSION            0x00
#define STP_BPDU_TYPE                   0x00

#define STP_RST_PROTOCOL_VERSION        0x02
#define STP_RST_TYPE                    0x02

#define STP_TC_FLAG                     0x01
#define STP_PROPOSAL_FLAG               0x02
#define STP_PORTROLE_MASK               0x0C
#define STP_PORTROLE_OFFSET             0x02
#define STP_LEARNING_FLAG               0x10
#define STP_FORWARDING_FLAG             0x20
#define STP_AGREEMENT_FLAG              0x40
#define STP_TC_ACK_FLAG                 0x80

#define STP_RST_VERSION1_LENGTH         0x00

#define STP_TCN_TYPE                    0x80

// BPDU times as in units of 1/256 second
#define STP_BPDU_TIME_UNIT              256
#define STP_BPDU_TIME_MAX               65535

// Type of BPDU
typedef enum
{
    STP_BPDUTYPE_INVALID,
    STP_BPDUTYPE_CONFIG,
    STP_BPDUTYPE_RST,
    STP_BPDUTYPE_TCN
} StpBpduType;

// Standard BPDU header, also TCN message
typedef struct
{
    unsigned short      protocolId;
    unsigned char       protocolVersion;
    unsigned char       bpduType;
} StpBpduHdr; // standard is 4 bytes

// RST BPDU structure
typedef struct
{
    StpBpduHdr       bpduHdr;
    unsigned char       flags;
    StpSwitchId      rootId;
    //char                padding1[2];
    UInt32       rootCost;
    StpSwitchId      swId;
    //char                padding2[2];
    StpPortId        portId;
    //char                padding3[2];
    StpBpduTime         messageAge;
    StpBpduTime         maxAge;
    StpBpduTime         helloTime;
    StpBpduTime         forwardDelay;
    unsigned char       version1Length;
} StpRstBpdu; // standard is 36 bytes

// Configuration BPDU structure
typedef struct
{
    StpBpduHdr       bpduHdr;
    unsigned char       flags;
    StpSwitchId      rootId;
    //char                padding1[2];
    UInt32       rootCost;
    StpSwitchId      swId;
    //char                padding2[2];
    StpPortId        portId;
    //char                padding3[2];
    StpBpduTime         messageAge;
    StpBpduTime         maxAge;
    StpBpduTime         helloTime;
    StpBpduTime         forwardDelay;
} StpConfigBpdu; // standard is 35 bytes


// -------------------------------------------------------
//  State machine

typedef BOOL (* SwitchStpSm_CheckStateIsInRange) (
    Node*, MacSwitch*, SwitchPort*, SwitchStpSm*);

typedef void (* SwitchStpSm_EnterState) (
    Node*, MacSwitch*, SwitchPort*, SwitchStpSm*);

typedef BOOL (* SwitchStpSm_TransitState) (
    Node*, MacSwitch*, SwitchPort*, SwitchStpSm*);

typedef BOOL (* SwitchStpSm_Iterate) (
    Node*, MacSwitch*, SwitchPort*, SwitchStpSm*);

struct SwitchStpSm_
{
    int state;
    BOOL stateHasChanged;

    SwitchStpSm_CheckStateIsInRange sm_CheckStateIsInRange;
    SwitchStpSm_EnterState sm_EnterState;
    SwitchStpSm_TransitState sm_ChangeState;
    SwitchStpSm_Iterate sm_Iterate;
};


// -------------------------------------------------------
//  Switch Protocol Entity

#define STP_SWITCH_PRIORITY_MIN             0
#define STP_SWITCH_PRIORITY_MAX             61440
#define STP_SWITCH_PRIORITY_DEFAULT         32768
#define STP_SWITCH_PRIORITY_MULTIPLE        4096

#define STP_PORT_PRIORITY_MIN               0
#define STP_PORT_PRIORITY_MAX               240
#define STP_PORT_PRIORITY_DEFAULT           128
#define STP_PORT_PRIORITY_MULTIPLE          16

#define STP_PATH_COST_MIN                   1
#define STP_PATH_COST_MAX                   2147483647
#define STP_PATH_COST_MAX_DEFAULT           200000000

#define STP_BANDWIDTH_MAX                   1e10

// Spanning Tree timer values
#define STP_MAX_AGE_MIN                     (6  * SECOND)
#define STP_MAX_AGE_MAX                     (40 * SECOND)
#define STP_MAX_AGE_DEFAULT                 (20 * SECOND)

#define STP_HELLO_TIME_MIN                  (1  * SECOND)
#define STP_HELLO_TIME_MAX                  (10 * SECOND)
#define STP_HELLO_TIME_DEFAULT              (2  * SECOND)

#define STP_FORWARD_DELAY_MIN               (4  * SECOND)
#define STP_FORWARD_DELAY_MAX               (30 * SECOND)
#define STP_FORWARD_DELAY_DEFAULT           (15 * SECOND)

#define STP_TX_HOLD_COUNT_MAX               10
#define STP_TX_HOLD_COUNT_MIN               1
#define STP_TX_HOLD_COUNT_DEFAULT           3

#define STP_MIGRATE_TIME                    (3 * SECOND)

#define STP_AGEING_TIME_MIN                 (10 * SECOND)
#define STP_AGEING_TIME_MAX                 (1000000 * SECOND)
#define STP_AGEING_TIME_DEFAULT             (300 * SECOND)

#define STP_VERSION_DEFAULT                 2

#define STP_DIAMETER_DEFAULT                7



// -------------------------------------------------------
// Public functions

// Receive a BPDU from the MAC protocol specific layer.
// Validate the BPDU and extract the message values.
void SwitchStp_ReceiveBpdu(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg);

// Execute STP state machines and age out database entries
// at every tick. The tick has a granularity of 1 S.
void SwitchStp_TickTimerEvent(
    Node *node,
    MacSwitch* sw);

// Print a port's stats and free STP related memory.
void SwitchStp_PortFinalize(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port);

// Print switch STP related stats and free switch SM.
void SwitchStp_Finalize(
    Node *node,
    MacSwitch* sw);

// Initialize variables in case the user specifies
// that STP should not be run on this switch.
void SwitchStp_SetVarsWhenStpIsOff(
    Node *node,
    MacSwitch* sw);

// Read spanning tree specific port variables
// from user input.
void SwitchStp_PortInit(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput,
    int portNumber);

// Reads switch specific spanning tree user input.
void SwitchStp_Init(
    Node *node,
    MacSwitch* sw,
    const NodeInput *nodeInput);

// Common entry routine for trace.
// Creates and writes to a file called "switch.trace".
void SwitchStp_Trace(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message *msg,
    const char* flag);

// Initialize trace setting from user configuration file
void SwitchStp_TraceInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput);

// Trigger the functions that will generate the appropriate
// topology change indications at the other ports of the
// switch when a port is disabled.
void SwitchStp_DisableInterface(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port);

// Resets variables and iterates the state machines to
// determine port roles when an interface is enabled.
void SwitchStp_EnableInterface(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port);

// -------------------------------------------------------

#endif /* MAC_STP_H */

