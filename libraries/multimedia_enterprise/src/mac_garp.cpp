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


#include "api.h"
#include "partition.h"

#include "mac_switch.h"
#include "mac_garp.h"
#include "mac_gvrp.h"

//-------------------------------------------------------------------------
// Function prototypes
static
void GarpGip_PropagateJoin(
    Node* node,
    GarpGid* portGid,
    unsigned gidIndex);

static
void GarpGip_PropagateLeave(
    Node* node,
    GarpGid* portGid,
    unsigned gidIndex);

static
void GarpGid_LeaveAll(
    GarpGid* portGid);

//-------------------------------------------------------------------------
// Library routines


// NAME:        Garp_AssignInfoField
//
// PURPOSE:     Assign info field to a timer.
//
// PARAMETERS:  Pointer to info field.
//              Garp data.
//              Port number.
//              Timer type.
//
// RETURN:      None.

static
void Garp_AssignInfoField(
    GarpGidTimerInfo* info,
    Garp* garp,
    unsigned short portNumber,
    GarpGidTimerType timerType)
{
    info->garp = garp;
    info->portNumber = portNumber;
    switch (timerType)
    {
        case GARP_JoinTimer:
            info->eventFn = GarpGid_JoinTimerExpired;
            break;

        case GARP_HoldTimer:
            info->eventFn = GarpGid_HoldTimerExpired;
            break;

        case GARP_LeaveTimer:
            info->eventFn = GarpGid_LeaveTimerExpired;
            break;

        case GARP_LeaveAllTimer:
            info->eventFn = GarpGid_LeaveAllTimerExpired;
            break;

        default:
            ERROR_ReportError(
                "Garp_AssignInfoField: "
                "Garp gets unknown timer.\n");
            break;
    }
}


// NAME:        Garp_StartRandomTimer
//
// PURPOSE:     Start a random timer of a given type.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Interface index.
//              Port number.
//              Timer type.
//              Delay.
//
// RETURN:      None.

static
void Garp_StartRandomTimer(
    Node* node,
    Garp* garp,
    int interfaceIndex,
    unsigned short instanceId,
    GarpGidTimerType timerType,
    clocktype delay)
{
    clocktype randomDelay;
    Message* newMsg = NULL;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_SWITCH,
                           MSG_MAC_GarpTimerExpired);
    MESSAGE_SetInstanceId(newMsg, (short)interfaceIndex);
    MESSAGE_InfoAlloc(node, newMsg, sizeof(GarpGidTimerInfo));

    Garp_AssignInfoField(
        (GarpGidTimerInfo*) MESSAGE_ReturnInfo(newMsg),
        garp, instanceId, timerType);

    // Randomize between 0.5 to 1 times the given delay.
    //
    randomDelay = (clocktype)
        ((0.5 + (RANDOM_erand(garp->randomTimerSeed) / 2.0)) * delay);

    MESSAGE_Send(node, newMsg, randomDelay);
}


// NAME:        Garp_StartTimer
//
// PURPOSE:     Start a timer of a given type.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Interface index.
//              Port number.
//              Timer type.
//              Delay.
//
// RETURN:      None.

static
void Garp_StartTimer(
    Node* node,
    Garp* garp,
    int interfaceIndex,
    unsigned short instanceId,
    GarpGidTimerType timerType,
    clocktype delay)
{
    Message* newMsg = NULL;

    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_SWITCH,
                           MSG_MAC_GarpTimerExpired);
    MESSAGE_SetInstanceId(newMsg, (short)interfaceIndex);
    MESSAGE_InfoAlloc(node, newMsg, sizeof(GarpGidTimerInfo));

    Garp_AssignInfoField(
        (GarpGidTimerInfo*) MESSAGE_ReturnInfo(newMsg),
        garp, instanceId, timerType);

    MESSAGE_Send(node, newMsg, delay);
}


// NAME:        Garp_DecodeReceivedEvent
//
// PURPOSE:     Decode event of a received message.
//
// PARAMETERS:  Received event.
//
// RETURN:      Decoded event.

void Garp_DecodeReceivedEvent(
    GarpGidEvent event,
    GarpGidEvent* decodedEvent)
{
    switch (event)
    {
        case GARP_GID_TX_LEAVE_ALL:
            *decodedEvent = GARP_GID_RX_LEAVE_ALL;
            break;

        case GARP_GID_TX_JOIN_EMPTY:
            *decodedEvent = GARP_GID_RX_JOIN_EMPTY;
            break;

        case GARP_GID_TX_JOIN_IN:
            *decodedEvent = GARP_GID_RX_JOIN_IN;
            break;

        case GARP_GID_TX_LEAVE_EMPTY:
            *decodedEvent = GARP_GID_RX_LEAVE_EMPTY;
            break;

        case GARP_GID_TX_LEAVE_IN:
            *decodedEvent = GARP_GID_RX_LEAVE_IN;
            break;

        case GARP_GID_TX_EMPTY:
            *decodedEvent = GARP_GID_RX_EMPTY;
            break;

        case GARP_GID_TX_LEAVE_ALL_RANGE:
            *decodedEvent = GARP_GID_RX_LEAVE_ALL_RANGE;
            break;

        case GARP_GID_NULL:
        default:
            *decodedEvent = GARP_GID_NULL;
            break;
    }
}


// -------------------------------------------------------------------------
// GID : Timer Processing

// NAME:        GarpGid_DoActions
//
// PURPOSE:     Carry out actions accumulated in this invocation of GID.
//
// PARAMETERS:  Node data.
//              Port gid.
//
// RETURN:      None.

static
void GarpGid_DoActions(
    Node* node,
    GarpGid* portGid)
{
    // Time to start join timer for this port GID.
    // Update lastToTransmit flag and set txPending flag TRUE,
    // as this port is going to send a GARP PDU.
    //
    if (portGid->canStartJoinTimer)
    {
        portGid->lastToTransmit = portGid->lastTransmitted;
        portGid->txPending = TRUE;
        portGid->canStartJoinTimer = FALSE;
    }

    // Schedule transmission as situation demands.
    //
    if (!portGid->holdTx)
    {
        if (portGid->canScheduleTxNow)
        {
            if (!portGid->txNowScheduled)
            {
                GarpGid_JoinTimerExpired(
                    node, portGid->garp, portGid->portNumber);
            }
            portGid->canScheduleTxNow = FALSE;
        }
        else if ((portGid->txPending
                    || (portGid->leaveallCountdown == 0))
                && (!portGid->joinTimerRunning))
        {
            Garp_StartRandomTimer(
                node,
                portGid->garp,
                portGid->interfaceIndex,
                portGid->portNumber,
                GARP_JoinTimer,
                portGid->joinTimeout);

            portGid->joinTimerRunning = TRUE;
        }
    }

    // Start leave timer if required.
    //
    if (portGid->canStartLeaveTimer && (!portGid->leaveTimerRunning))
    {
        Garp_StartTimer(
            node,
            portGid->garp,
            portGid->interfaceIndex,
            portGid->portNumber,
            GARP_LeaveTimer,
            portGid->leaveTimeout4);

        SwitchGvrp_Trace(node, portGid, NULL, "Leave Timer scheduled ...");
    }

    portGid->canStartLeaveTimer = FALSE;
}


// NAME:        GarpGip_DoActions
//
// PURPOSE:     Carry out actions accumulated in this invocation of GARP
//              for all the port in GIP ring.
//
// PARAMETERS:  Node data.
//              Port gid.
//
// RETURN:      None.

static
void GarpGip_DoActions(
    Node* node,
    GarpGid* portGid)
{
    GarpGid* thisPortGid = portGid;

    do
    {
        GarpGid_DoActions(node, thisPortGid);
    } while ((thisPortGid = thisPortGid->nextInConnectedRing) != portGid);
}


// -------------------------------------------------------------------------
// GIDTT: GID Protocol Transition Tables

// Applicant states.
//
enum ApplicantStates
{
    GID_Va,  // Very anxious, active                      // 0
    GID_Aa,  // Anxious,      active                      // 1
    GID_Qa,  // Quiet,        active                      // 2
    GID_La,  // Leaving,      active                      // 3
    GID_Vp,  // Very anxious, passive                     // 4
    GID_Ap,  // Anxious,      passive                     // 5
    GID_Qp,  // Quiet,        passive                     // 6
    GID_Vo,  // Very anxious  observer                    // 7
    GID_Ao,  // Anxious       observer                    // 8
    GID_Qo,  // Quiet         observer                    // 9
    GID_Lo,  // Leaving       observer                    // 10

    GID_Von, // Very anxious  observer, non-participant   // 11
    GID_Aon, // Anxious       observer, non-participant   // 12
    GID_Qon  // Quiet         observer, non-participant   // 13
};

// Registrar states.
//
enum RegistrarStates
{
    // Normal registration.
    GID_Inn,                                    // 0
    GID_Lv, GID_L3, GID_L2, GID_L1,             // 1  2  3  4
    GID_Mt,                                     // 5

    // Registration fixed.
    GID_Inr,                                    // 6
    GID_Lvr, GID_L3r, GID_L2r, GID_L1r,         // 7  8  9 10
    GID_Mtr,                                    // 11

    // Registration forbidden.
    GID_Inf,                                    // 12
    GID_Lvf, GID_L3f, GID_L2f, GID_L1f,         // 13 14 15 16
    GID_Mtf                                     // 17
};

enum {GARP_GID_APPLICANT_STATES = GID_Qon + 1}; // for array sizing
enum {GARP_GID_REGISTRAR_STATES = GID_Mtf + 1}; // for array sizing

// GID Timer indications
//
enum
{
    GID_Nt = 0,     // No timer action
    GID_Jt = 1,     // Can start join timer
    GID_Lt = 1      // Can start leave timer
};

// GID Applicant Tx related indications
//
enum
{
    GID_Nm = 0,     // No message to transmit
    GID_Jm,         // Transmit a Join
    GID_Lm,         // Transmit a Leave
    GID_Em          // Transmit an Empty
};

// GID Registrar related indications
//
enum
{
    GID_Ni = 0,     // No indication
    GID_Li = 1,     // Leave indication
    GID_Ji = 2      // Join indication
};


// -------------------------------------------------------------------------
// GIDTT : Transition Table Structure

// An element of applicant transition table
//
typedef struct
{
    unsigned newAppState        : 5;
    unsigned canStartJoinTimer  : 1;
} GarpGidApplicantTtEntry;

// An element of registrar transition table
//
typedef struct
{
    unsigned newRegState        : 5;
    unsigned indications        : 2;
    unsigned canStartLeaveTimer : 1;
} GarpGidRegistrarTtEntry;

// An element of applicant transmit table.
//
typedef struct
{
    unsigned newAppState        : 5;
    unsigned msgToTransmit      : 2;
    unsigned canStartJoinTimer  : 1;
} GarpGidApplicantTxTtEntry;

// An element of registrar leave timer table.
//
typedef struct
{
    unsigned newRegState        : 5;
    unsigned leaveIndication    : 1;
    unsigned canStartLeaveTimer : 1;
} GarpGidRegistrarLeaveTimerEntry;


// General applicant transition table.
//
static GarpGidApplicantTtEntry
       applicant_tt[GARP_GID_RX_EVENT_COUNT  + GARP_GID_REQ_EVENTS +
                    GARP_GID_AMGT_EVENTS + GARP_GID_RMGT_EVENTS]
                   [GARP_GID_APPLICANT_STATES] =
{
    {   // GID event GARP_GID_NULL
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Ap, GID_Nt},{GID_Vp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_RX_LEAVE_EMPTY
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},{GID_Vo, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Lo, GID_Jt},{GID_Lo, GID_Jt},{GID_Lo, GID_Jt},{GID_Vo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Von,GID_Nt},{GID_Von,GID_Nt}
    },

    {   // GID event GARP_GID_RX_LEAVE_IN
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Va, GID_Nt},{GID_Vp, GID_Jt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Lo, GID_Nt},{GID_Lo, GID_Nt},{GID_Lo, GID_Jt},{GID_Vo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Von,GID_Nt},{GID_Von,GID_Nt}
    },

    {   // GID event GARP_GID_RX_EMPTY
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Va, GID_Nt},{GID_Va, GID_Jt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Vo, GID_Nt},{GID_Vo, GID_Nt},{GID_Vo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Von,GID_Nt},{GID_Von,GID_Nt}
    },

    {   // GID event GARP_GID_RX_JOIN_EMPTY
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Va, GID_Nt},{GID_Va, GID_Jt},{GID_Vo, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Vo, GID_Nt},{GID_Vo, GID_Jt},{GID_Vo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Von,GID_Nt},{GID_Von,GID_Jt}
    },

    {   // GID event GARP_GID_RX_JOIN_IN
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_Qa, GID_Nt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Ap, GID_Nt},{GID_Qp, GID_Nt},{GID_Qp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Qo, GID_Nt},{GID_Ao, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Aon,GID_Nt},{GID_Qon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_JOIN,
        // Note:  This join request handles repeated joins, i.e. joins for
        //        states that are already in. Does not provide feedback for
        //        joins that are forbidden by management controls, the
        //        expectation is that this table will not be directly used
        //        by new management requests.
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_Va, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Ap, GID_Nt},{GID_Qp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vp, GID_Jt},{GID_Ap, GID_Jt},{GID_Qp, GID_Nt},{GID_Vp, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_LEAVE.
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_La, GID_Nt},{GID_La, GID_Nt},{GID_La, GID_Jt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_NORMAL_OPERATION
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Vp, GID_Nt},{GID_Vp, GID_Jt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Va, GID_Nt},{GID_Va, GID_Nt},{GID_Va, GID_Jt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Va, GID_Nt},{GID_Va, GID_Nt},{GID_Va, GID_Jt}
    },

    {   // GID event GARP_GID_NO_PROTOCOL
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt},{GID_Von,GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt},{GID_Von,GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_NORMAL_REGISTRATION.
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Ap, GID_Nt},{GID_Vp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_FIX_REGISTRATION.
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Ap, GID_Nt},{GID_Vp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    },

    {   // GID event GARP_GID_FORBID_REGISTRATION.
        //
        // GID_Va         GID_Aa           GID_Qa           GID_La
        {GID_Va, GID_Nt},{GID_Aa, GID_Nt},{GID_Qa, GID_Nt},{GID_La, GID_Nt},

        // GID_Vp         GID_Ap           GID_Qp
        {GID_Vp, GID_Nt},{GID_Ap, GID_Nt},{GID_Vp, GID_Nt},

        // GID_Vo         GID_Ao           GID_Qo           GID_Lo
        {GID_Vo, GID_Nt},{GID_Ao, GID_Nt},{GID_Qo, GID_Nt},{GID_Lo, GID_Nt},

        // GID_Von        GID_Aon          GID_Qon
        {GID_Von,GID_Nt},{GID_Aon,GID_Nt},{GID_Qon,GID_Nt}
    }
};


// General Registrar Transition Table
//
static GarpGidRegistrarTtEntry
       registrar_tt[GARP_GID_RX_EVENT_COUNT  + GARP_GID_REQ_EVENTS +
                    GARP_GID_AMGT_EVENTS + GARP_GID_RMGT_EVENTS]
                   [GARP_GID_REGISTRAR_STATES] =
{
    {   // GID event GARP_GID_NULL
        //
        //GID_Inn
        { GID_Inn, GID_Ni,  GID_Nt},
        //GID_Lv                      GID_L3
        { GID_Lv,  GID_Ni,  GID_Nt}, {GID_L3,  GID_Ni,  GID_Nt},
        //GID_L2                      GID_L1
        { GID_L2,  GID_Ni,  GID_Nt}, {GID_L1,  GID_Ni,  GID_Nt},
        //GID_Mt
        { GID_Mt,  GID_Ni,  GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_RX_LEAVE_EMPTY
        //
        //GID_Inn
        { GID_Lv, GID_Ni,GID_Lt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Lvr,GID_Ni,GID_Lt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Lvf,GID_Ni,GID_Lt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_RX_LEAVE_IN
        //
        //GID_Inn
        { GID_Lv, GID_Ni,GID_Lt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Lvr,GID_Ni,GID_Lt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Lvf,GID_Ni,GID_Lt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_RX_EMPTY
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_RX_JOIN_EMPTY
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Inn,GID_Ni,GID_Nt},  {GID_Inn,GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_Inn,GID_Ni,GID_Nt},  {GID_Inn,GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Inn,GID_Ji,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Inr,GID_Ni,GID_Nt},  {GID_Inr,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_Inr,GID_Ni,GID_Nt},  {GID_Inr,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Inr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Inf,GID_Ni,GID_Nt},  {GID_Inf,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_Inf,GID_Ni,GID_Nt},  {GID_Inf,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Inf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_RX_JOIN_IN
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Inn,GID_Ni,GID_Nt},  {GID_Inn,GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_Inn,GID_Ni,GID_Nt},  {GID_Inn,GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Inn,GID_Ji,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Inr,GID_Ni,GID_Nt},  {GID_Inr,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_Inr,GID_Ni,GID_Nt},  {GID_Inr,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Inr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Inf,GID_Ni,GID_Nt},  {GID_Inf,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_Inf,GID_Ni,GID_Nt},  {GID_Inf,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Inf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_JOIN
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_LEAVE
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_NORMAL_OPERATION.
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_NO_PROTOCOL.
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_NORMAL_REGISTRATION
        //
        //GID_Inn
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mt, GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inn,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lv, GID_Ni,GID_Nt},  {GID_L3, GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2, GID_Ni,GID_Nt},  {GID_L1, GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mt, GID_Li,GID_Nt},

        //GID_Inf
        { GID_Inn,GID_Ji,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lv, GID_Ji,GID_Nt},  {GID_L3, GID_Ji,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2, GID_Ji,GID_Nt},  {GID_L1, GID_Ji,GID_Nt},
        //GID_Mtf
        { GID_Mt, GID_Ni,GID_Nt}
    },

    {   // GID event GARP_GID_FIX_REGISTRATION
        //
        //GID_Inn
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mt
        { GID_Mtr,GID_Ji,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Ni,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Ni,GID_Nt},  {GID_L3r,GID_Ni,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Ni,GID_Nt},  {GID_L1r,GID_Ni,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Ni,GID_Nt},

        //GID_Inf
        { GID_Inr,GID_Ji,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvr,GID_Ji,GID_Nt},  {GID_L3r,GID_Ji,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2r,GID_Ji,GID_Nt},  {GID_L1r,GID_Ji,GID_Nt},
        //GID_Mtf
        { GID_Mtr,GID_Ji,GID_Nt}
    },

    {   // GID event GARP_GID_FORBID_REGISTRATION
        //
        //GID_Inn
        { GID_Inf,GID_Li,GID_Nt},
        //GID_Lv                    GID_L3
        { GID_Lvf,GID_Li,GID_Nt},  {GID_L3f,GID_Li,GID_Nt},
        //GID_L2                    GID_L1
        { GID_L2f,GID_Li,GID_Nt},  {GID_L1f,GID_Li,GID_Nt},
        //GID_Mt
        { GID_Mtf,GID_Ni,GID_Nt},

        //GID_Inr
        { GID_Inr,GID_Li,GID_Nt},
        //GID_Lvr                   GID_L3r
        { GID_Lvr,GID_Li,GID_Nt},  {GID_L3r,GID_Li,GID_Nt},
        //GID_L2r                   GID_L1r
        { GID_L2r,GID_Li,GID_Nt},  {GID_L1r,GID_Li,GID_Nt},
        //GID_Mtr
        { GID_Mtr,GID_Li,GID_Nt},

        //GID_Inf
        { GID_Inf,GID_Ni,GID_Nt},
        //GID_Lvf                   GID_L3f
        { GID_Lvf,GID_Ni,GID_Nt},  {GID_L3f,GID_Ni,GID_Nt},
        //GID_L2f                   GID_L1f
        { GID_L2f,GID_Ni,GID_Nt},  {GID_L1f,GID_Ni,GID_Nt},
        //GID_Mtf
        { GID_Mtf,GID_Ni,GID_Nt}
    }
};


// GIDTT : Applicant Transmit Table
//
static GarpGidApplicantTxTtEntry
       applicantTxTt[GARP_GID_APPLICANT_STATES] =
{
    //GID_Va
    { GID_Aa, GID_Jm,GID_Jt},
    //GID_Aa
    { GID_Qa, GID_Jm,GID_Nt},
    //GID_Qa
    { GID_Qa, GID_Nm,GID_Nt},
    //GID_La
    { GID_Vo, GID_Lm,GID_Nt},

    //GID_Vp
    { GID_Aa, GID_Jm,GID_Jt},
    //GID_Ap
    { GID_Qa, GID_Jm,GID_Nt},
    //GID_Qp
    { GID_Qp, GID_Nm,GID_Nt},

    //GID_Vo
    { GID_Vo, GID_Nm,GID_Nt},
    //GID_Ao
    { GID_Ao, GID_Nm,GID_Nt},
    //GID_Qo
    { GID_Qo, GID_Nm,GID_Nt},
    //GID_Lo
    { GID_Vo, GID_Nm,GID_Nt},

    //GID_Von
    { GID_Von,GID_Nm,GID_Nt},
    //GID_Aon
    { GID_Aon,GID_Nm,GID_Nt},
    //GID_Qon
    { GID_Qon,GID_Nm,GID_Nt}
};


// GIDTT : Registrar Leave Timer Table
//
static GarpGidRegistrarLeaveTimerEntry
       registrarLeaveTimerTable[GARP_GID_REGISTRAR_STATES] =
{
    //GID_Inn
    { GID_Inn,GID_Ni,GID_Nt},
    //GID_Lv                    GID_L3
    { GID_L3, GID_Ni,GID_Lt},  {GID_L2, GID_Ni,GID_Lt},
    //GID_L2                    GID_L1
    { GID_L1, GID_Ni,GID_Lt},  {GID_Mt, GID_Li,GID_Nt},
    //GID_Mt
    { GID_Mt, GID_Ni,GID_Nt},

    //GID_Inr
    { GID_Inr,GID_Ni,GID_Nt},
    //GID_Lvr                   GID_L3r
    { GID_L3r,GID_Ni,GID_Lt},  {GID_L2r,GID_Ni,GID_Lt},
    //GID_L2r                   GID_L1r
    { GID_L1r,GID_Ni,GID_Lt},  {GID_Mtr,GID_Ni,GID_Nt},
    //GID_Mtr
    { GID_Mtr,GID_Ni,GID_Nt},

    //GID_Inf
    { GID_Inf,GID_Ni,GID_Nt},
    //GID_Lvf                   GID_L3f
    { GID_L3f,GID_Ni,GID_Lt},  {GID_L2f,GID_Ni,GID_Lt},
    //GID_L2f                   GID_L1f
    { GID_L1f,GID_Ni,GID_Lt},  {GID_Mtf,GID_Ni,GID_Nt},
    //GID_Mtf
    { GID_Mtf,GID_Ni,GID_Nt}
};


// GIDTT : State Reporting Tables
//
static GarpGid_ApplicantState
       applicantStateTable[GARP_GID_APPLICANT_STATES] =
{
    // GID_Va            GID_Aa           GID_Qa         GID_La
    GARP_GID_VeryAnxious,GARP_GID_Anxious,GARP_GID_Quiet,GARP_GID_Leaving,

    // GID_Vp            GID_Ap           GID_Qp
    GARP_GID_VeryAnxious,GARP_GID_Anxious,GARP_GID_Quiet,

    // GID_Vo            GID_Ao           GID_Qo         GID_Lo
    GARP_GID_VeryAnxious,GARP_GID_Anxious,GARP_GID_Quiet,GARP_GID_Leaving,

    // GID_Von           GID_Aon          GID_Qon
    GARP_GID_VeryAnxious,GARP_GID_Anxious,GARP_GID_Quiet
};

static GarpGid_ApplicantMgt applicantMgtTable[GARP_GID_APPLICANT_STATES] =
{
    // GID_Va       GID_Aa          GID_Qa          GID_La
    GARP_GID_Normal,GARP_GID_Normal,GARP_GID_Normal,GARP_GID_Normal,

    // GID_Vp       GID_Ap          GID_Qp
    GARP_GID_Normal,GARP_GID_Normal,GARP_GID_Normal,

    // GID_Vo       GID_Ao          GID_Qo          GID_Lo
    GARP_GID_Normal,GARP_GID_Normal,GARP_GID_Normal,GARP_GID_Normal,

    // GID_Von          GID_Aon             GID_Qon
    GARP_GID_NoProtocol,GARP_GID_NoProtocol,GARP_GID_NoProtocol
};

static GarpGid_RegistrarState
       registrarStateTable[GARP_GID_REGISTRAR_STATES] =
{
    // GID_Inn
    GARP_GID_In,
    // GID_Lv       GID_L3          GID_L2          GID_L1
    GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave,
    // GID_Mt
    GARP_GID_Empty,

    // GID_Inr
    GARP_GID_In,
    // GID_Lvr      GID_L3r         GID_L2r         GID_L1r
    GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave,
    // GID_Mtr
    GARP_GID_Empty,

    // GID_Inf
    GARP_GID_In,
    // GID_Lvf      GID_L3f         GID_L2f         GID_L1f
    GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave, GARP_GID_Leave,
    // GID_Mtf
    GARP_GID_Empty
};

static GarpGid_RegistrarMgt registrarMgtTable[GARP_GID_REGISTRAR_STATES] =
{
    // GID_Inn
    GARP_GID_NormalRegistration,
    // GID_Lv                       GID_L3
    GARP_GID_NormalRegistration,    GARP_GID_NormalRegistration,
    // GID_L2                       GID_L1
    GARP_GID_NormalRegistration,    GARP_GID_NormalRegistration,
    // GID_Mt
    GARP_GID_NormalRegistration,

    // GID_Inr
    GARP_GID_RegistrationFixed,
    // GID_Lvr                      GID_L3r
    GARP_GID_RegistrationFixed,     GARP_GID_RegistrationFixed,
    // GID_L2r                      GID_L1r
    GARP_GID_RegistrationFixed,     GARP_GID_RegistrationFixed,
    // GID_Mtr
    GARP_GID_RegistrationFixed,

    // GID_Inf
    GARP_GID_RegistrationForbidden,
    // GID_Lvf                      GID_L3f
    GARP_GID_RegistrationForbidden, GARP_GID_RegistrationForbidden,
    // GID_L2f                      GID_L1f
    GARP_GID_RegistrationForbidden, GARP_GID_RegistrationForbidden,
    // GID_Mtf
    GARP_GID_RegistrationForbidden
};

static BOOL registrar_in_table[GARP_GID_REGISTRAR_STATES] =
{
    // GID_Inn   GID_Lv   GID_L3   GID_L2   GID_L1   GID_Mt
    TRUE,        TRUE,    TRUE,    TRUE,    TRUE,    FALSE,

    // GID_Inr   GID_Lvr  GID_L3r  GID_L2r  GID_L1r  GID_Mtr
    TRUE,        TRUE,    TRUE,    TRUE,    TRUE,    TRUE,

    // GID_Inf   GID_Lvf  GID_L3f  GID_L2f  GID_L1f  GID_Mtf
    FALSE,       FALSE,   FALSE,   FALSE,   FALSE,   FALSE
};


// -------------------------------------------------------------------------
// GIDTT : Receive Events, User Requests & MGT Processing

// NAME:        GarpGidTt_Event
//
// PURPOSE:     Handles receive events and join or leave requests.
//
// PARAMETERS:  Node data.
//              Port gid.
//
// RETURN:      GarpGidEvent.

static
GarpGidEvent GarpGidTt_Event(
    GarpGid* portGid,
    GarpGidMachine* machine,
    GarpGidEvent event)
{
    GarpGidApplicantTtEntry* atransition;
    GarpGidRegistrarTtEntry* rtransition;

    // Process applicant and registrar state machine
    // with given GID event.
    //
    atransition = &applicant_tt[event][machine->applicant];
    rtransition = &registrar_tt[event][machine->registrar];

    // Update applicant and registrar state.
    //
    machine->applicant = atransition->newAppState;
    machine->registrar = rtransition->newRegState;

    if ((event == GARP_GID_JOIN) && (atransition->canStartJoinTimer))
    {
        portGid->canScheduleTxNow = TRUE;
    }

    // Set join and leave timer flags if required.
    //
    portGid->canStartJoinTimer  = portGid->canStartJoinTimer ||
                                  atransition->canStartJoinTimer;
    portGid->canStartLeaveTimer = portGid->canStartLeaveTimer ||
                                  rtransition->canStartLeaveTimer;

    // Give proper indication resulting from processing this event.
    //
    switch (rtransition->indications)
    {
        case GID_Ji:
            return GARP_GID_JOIN;
            break;
        case GID_Li:
            return GARP_GID_LEAVE;
            break;
        case GID_Ni:
        default:
            return GARP_GID_NULL;
            break;
    }
}


// NAME:        GarpGidTt_In
//
// PURPOSE:     Checks if a registrar is in 'In' state.
//
// PARAMETERS:  Machine to check.
//
// RETURN:      FALSE, if GARP_GID_Empty.
//              TRUE, otherwise.

static
BOOL GarpGidTt_In(GarpGidMachine* machine)
{
    return registrar_in_table[machine->registrar];
}


// -------------------------------------------------------------------------
// GIDTT : Transmit Messages

// NAME:        GarpGidTt_Tx
//
// PURPOSE:     Determine type of message to be send for an attribute.
//
// PARAMETERS:  Port gid.
//              State machine of attribute.
//
// RETURN:      Possible type of message.

static
GarpGidEvent GarpGidTt_Tx(
    GarpGid* portGid,
    GarpGidMachine* machine)
{
    unsigned msg;
    unsigned rin = GARP_GID_Empty;
    GarpGidApplicantTxTtEntry* aTransmit;

    // Check applicant transmit table to determine type
    // of message to transmit.
    // Check registrar state if message is to be transmitted.
    //
    aTransmit = &applicantTxTt[machine->applicant];

    if ((msg = aTransmit->msgToTransmit) != GID_Nm)
    {
        rin = registrarStateTable[machine->registrar];
    }

    // Change applicant state and schedule join timer if required.
    //
    machine->applicant = aTransmit->newAppState;
    portGid->canStartJoinTimer  = portGid->canStartJoinTimer ||
                                  aTransmit->canStartJoinTimer;

    // Return the type of GID event to transmit for this attribute.
    //
    switch (msg)
    {
        case GID_Jm:
            return rin != GARP_GID_Empty ?
                       GARP_GID_TX_JOIN_IN  : GARP_GID_TX_JOIN_EMPTY;
            break;
        case GID_Lm:
            return rin != GARP_GID_Empty ?
                       GARP_GID_TX_LEAVE_IN : GARP_GID_TX_LEAVE_EMPTY;
            break;
        case GID_Em:
            return GARP_GID_TX_EMPTY;
            break;
        case GID_Nm:
        default:
            return GARP_GID_NULL;
            break;
    }
}


// -------------------------------------------------------------------------
// GIDTT : Leave Timer Processing

// NAME:        GarpGidTt_LeaveTimerExpiry
//
// PURPOSE:     Initiate required actions after leave timer expires.
//
// PARAMETERS:  Port gid.
//              State machine of an attribute.
//
// RETURN:      GarpGidEvent.

static
GarpGidEvent GarpGidTt_LeaveTimerExpiry(
    GarpGid* portGid,
    GarpGidMachine* machine)
{
    GarpGidRegistrarLeaveTimerEntry *rtransition;

    // Check registrar leave timer table and update registrar state.
    //
    rtransition = &registrarLeaveTimerTable[machine->registrar];
    machine->registrar = rtransition->newRegState;

    // Set leave timer flag, if required.
    //
    portGid->canStartLeaveTimer = portGid->canStartLeaveTimer ||
                                  rtransition->canStartLeaveTimer;

    // Return leave event if registrar table gives a leave indication.
    //
    return (rtransition->leaveIndication == GID_Li) ?
                 GARP_GID_LEAVE : GARP_GID_NULL;
}


// -------------------------------------------------------------------------
// GIDTT : State Reporting

// NAME:        GarpGidTt_MachineActive
//
// PURPOSE:     Check if a state machine is active.
//
// PARAMETERS:  State machine of an attribute.
//
// RETURN:      TRUE, if active.
//              FALSE, otherwise.

static
BOOL GarpGidTt_MachineActive(GarpGidMachine* machine)
{
    if ((machine->applicant == GID_Vo) &&
        (machine->registrar == GID_Mt))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


// NAME:        GarpGidTt_States
//
// PURPOSE:     Assign applicant & registrar states for an attribute.
//
// PARAMETERS:  State machine of an attribute.
//
// RETURN:      Possible states.

void GarpGidTt_States(GarpGidMachine* machine, GarpGidStates* state)
{
    state->applicantState = applicantStateTable[machine->applicant];

    state->applicantMgt   = applicantMgtTable[machine->applicant];

    state->registrarState = registrarStateTable[machine->registrar];

    state->registrarMgt   = registrarMgtTable[machine->registrar];
}


// -------------------------------------------------------------------------
// Useful Functions.

// NAME:        GarpGid_RegisteredHere
//
// PURPOSE:     Check if an attribute is registered here.
//
// PARAMETERS:  Port gid.
//
// RETURN:      TRUE, if registered here.
//              FALSE, otherwise.

static
BOOL GarpGid_RegisteredHere(
    GarpGid* portGid,
    unsigned gidIndex)
{
    GarpGidStates state;

    GarpGidTt_States(&portGid->machines[gidIndex], &state);

    return state.registrarState != GARP_GID_Empty ||
            state.registrarMgt == GARP_GID_RegistrationFixed;
}


// NAME:        GarpGid_FindPort
//
// PURPOSE:     Find port gid for given port number.
//
// PARAMETERS:  First port gid.
//              Port number.
//
// RETURN:      TRUE, if found with pointer to port gid.
//              FALSE, otherwise.

BOOL GarpGid_FindPort(
    GarpGid* firstGid,
    unsigned short portNumber,
    GarpGid** gid)
{
    GarpGid* nextGid = firstGid;

    // First port yet to create
    if (firstGid == NULL)
    {
        return FALSE;
    }

    while (nextGid->portNumber != portNumber)
    {
        nextGid = nextGid->nextInPortRing;
        if (nextGid == firstGid)
        {
            return FALSE;
        }
    }
    *gid = nextGid;
    return TRUE;
}


// NAME:        GarpGid_NextPort
//
// PURPOSE:     Find next port gid for given port gid.
//
// PARAMETERS:  Port gid.
//
// RETURN:      Pointer to port gid.

static
GarpGid* GarpGid_NextPort(
    GarpGid* portGid)
{
    return portGid->nextInPortRing;
}


// -------------------------------------------------------------------------
// GID : Management functions

// NAME:        GarpGid_ReadAttributeState
//
// PURPOSE:     Read applicant and registrar states for a given attribute.
//
// PARAMETERS:  Port gid.
//              attribute index.
//
// RETURN:      States of the attribute.

static
void GarpGid_ReadAttributeState(
    GarpGid* portGid,
    unsigned index,
    GarpGidStates* state)
{
    GarpGidTt_States(&portGid->machines[index], state);
}


// NAME:        GarpGid_FindUnused
//
// PURPOSE:     Find an machine, not used by any port in ring,
//              from all the machines maintained by GarpGid.
//              Start the search at GID index fromIndex, and
//              search till gidlastIndex.
//
// PARAMETERS:  Garp data.
//
// RETURN:      TRUE, if machine found.
//              FALSE, otherwise

BOOL GarpGid_FindUnused(
    Garp* garp,
    unsigned fromIndex,
    unsigned* foundIndex)
{
    GarpGid* checkGid = garp->gid;
    unsigned gidIndex = fromIndex;

    for (;;)
    {
        // Machine is active in this port GID.
        // Skip to next machine and start from the first GID in port ring.
        //
        if (GarpGidTt_MachineActive(&checkGid->machines[gidIndex]))
        {
            // Machine crossed the last GID index.
            // So all machines are currently active.
            //
            if (++gidIndex >= garp->lastGidIndex)
            {
                return FALSE;
            }

            checkGid = garp->gid;
        }
        else if ((checkGid = checkGid->nextInPortRing)
                    == garp->gid)
        {
            // Machine corresponding to this gidIndex is
            // inactive at all port GIDs.
            //
            *foundIndex = gidIndex;
            return TRUE;
        }
    }
}


// -------------------------------------------------------------------------
// GID : Receive Processing

// NAME:        GarpGid_ReceivePdu
//
// PURPOSE:     Process a received PDU. If a GID instance for this
//              application and port number is found and is enabled,
//              pass the PDU to the application to parse it.
//
// PARAMETERS:  Node data
//              Garp data.
//              Port received the message.
//              Received message.
//
// RETURN:      None.

static
void GarpGid_ReceivePdu(
    Node* node,
    Garp* garp,
    unsigned short portNumber,
    Message* pdu)
{
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        if (portGid->isEnabled)
        {
            garp->receiveFn(node, garp, portGid, pdu);
            GarpGip_DoActions(node, portGid);

            SwitchGvrp_Trace(node, portGid, NULL, "AttribState");
        }
    }
    else
    {
        // Message cannot be processed as port is unavailable.
        //
        MESSAGE_Free(node, pdu);
    }
}


// -------------------------------------------------------------------------
// GID : Transmit Processing

// NAME:        GarpGid_NextTx
//
// PURPOSE:     Check to see if a leaveall should be sent If so,
//              return GARP_GID_TX_LEAVE_ALL. Otherwise, scan the
//              GID machines for messsages that require transmission.
//
// PARAMETERS:  Node data
//              Port gid.
//
// RETURN:      GarpGidEvent with index of the machine.

GarpGidEvent GarpGid_NextTx(
    Node* node,
    GarpGid* portGid,
    unsigned* index)
{
    unsigned checkIndex;
    unsigned stopAfter;
    GarpGidEvent msg = GARP_GID_NULL;

    if (portGid->holdTx)
    {
        return GARP_GID_NULL;
    }

    if (portGid->leaveallCountdown == 0)
    {
        // Periodic leave all timer has fired. Send a leaveall message
        // for all the attribute to neighbours. Set leaveallCountdown
        // that will control leave all timer for next period.
        //
        portGid->leaveallCountdown = GARP_GID_LEAVEALL_COUNT;

        // Precess Leaveall event as port sending it.
        //
        GarpGid_LeaveAll(portGid);
        portGid->txPending = TRUE;
        portGid->lastToTransmit = portGid->lastTransmitted;

        return GARP_GID_TX_LEAVE_ALL;
    }

    if (!portGid->txPending)
    {
        return GARP_GID_NULL;
    }

    // Assign checkIndex with next index of state machine. If index
    // exceeds attributes range, start from begining.
    //
    checkIndex = portGid->lastTransmitted + 1;
    if (checkIndex > (portGid->garp->lastGidIndex - 1))
    {
        checkIndex = 0;
    }

    stopAfter = portGid->lastToTransmit;
    if (stopAfter < checkIndex)
    {
        stopAfter = portGid->garp->lastGidIndex;
    }

    for ( ; ; checkIndex++)
    {
        if (checkIndex > stopAfter)
        {
            if (stopAfter == portGid->lastToTransmit)
            {
                portGid->txPending = FALSE;
                return GARP_GID_NULL;
            }
            else if (stopAfter == (portGid->garp->lastGidIndex))
            {
                checkIndex = 0;
                stopAfter = portGid->lastToTransmit;
            }
        }

        // Check if attribute has any message to send.
        //
        if ((msg = GarpGidTt_Tx(portGid, &portGid->machines[checkIndex]))
                != GARP_GID_NULL)
        {
            *index = portGid->lastTransmitted = checkIndex;
            portGid->machines[portGid->untransmitMachine].applicant =
                portGid->machines[checkIndex].applicant;

            portGid->txPending = (checkIndex != portGid->lastToTransmit);
            return msg;
        }
    } // end for
}


// NAME:        GarpGid_Untransmit
//
// PURPOSE:     Initiate necessary actions if some messages
//              cannot be transmitted in current PDU.
//              Set the txPending flag to True.
//
// PARAMETERS:  Port gid.
//
// RETURN:      None.

void GarpGid_Untransmit(
    GarpGid* portGid)
{
    // Collect applicant states to untransmitted machine as PDU
    // has crossed maximum length.
    //
    portGid->machines[portGid->lastTransmitted].applicant =
        portGid->machines[portGid->untransmitMachine].applicant;

    // Update last transmitted flag.
    //
    if (portGid->lastTransmitted == 0)
    {
        portGid->lastTransmitted = portGid->garp->lastGidIndex - 1;
    }
    else
    {
        portGid->lastTransmitted--;
    }

    portGid->txPending = TRUE;
}


// -------------------------------------------------------------------------
// GID : Requested Event Processing

// NAME:        GarpGid_LeaveAll
//
// PURPOSE:     Process all the attributes with leave all event
//              at this port.
//
// PARAMETERS:  Port gid.
//
// RETURN:      None.

static
void GarpGid_LeaveAll(
    GarpGid* portGid)
{
    unsigned i;
    Garp* garp = portGid->garp;

    // Process all the machine by GID leave all event.
    //
    for (i = 0; i < garp->lastGidIndex; i++)
    {
        (void) GarpGidTt_Event(
            portGid, &portGid->machines[i], GARP_GID_RX_LEAVE_EMPTY);
    }
}


// NAME:        GarpGid_ReceiveLeaveAll
//
// PURPOSE:     Port received a leave all message. Initiate necessary
//              actions for this port.
//
// PARAMETERS:  Port gid.
//
// RETURN:      None.

void GarpGid_ReceiveLeaveAll(
    GarpGid* portGid)
{
    // Received leave all message. So restart this
    // participants leave all timer.
    //
    portGid->leaveallCountdown = GARP_GID_LEAVEALL_COUNT;

    GarpGid_LeaveAll(portGid);
}


// NAME:        GarpGid_ReceiveMsg
//
// PURPOSE:     Process the state machine of a received attribute.
//              Generate and propagate Join/Leave indications as necessary.
//
// PARAMETERS:  Node data.
//              Port gid.
//              Equivalent gid index of received attribute.
//              Received event of this attribute.
//
// RETURN:      None.

void GarpGid_ReceiveMsg(
    Node* node,
    GarpGid* portGid,
    unsigned index,
    GarpGidEvent msgEvent)
{
    GarpGidMachine* machine;
    GarpGidEvent event;

    machine = &portGid->machines[index];
    event = GarpGidTt_Event(portGid, machine, msgEvent);

    if (event == GARP_GID_JOIN)
    {
        portGid->garp->joinIndicationFn(
            node, portGid->garp, portGid, index);
        GarpGip_PropagateJoin(node, portGid, index);
    }
    else if (event == GARP_GID_LEAVE)
    {
        portGid->garp->leaveIndicationFn(
            node, portGid->garp, portGid, index);
        GarpGip_PropagateLeave(node, portGid, index);
    }
}


// NAME:        GarpGid_JoinRequest
//
// PURPOSE:     Process the state machine of an attribute with
//              GARP_GID_JOIN event.
//
// PARAMETERS:  Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      None.

static
void GarpGid_JoinRequest(
    GarpGid* portGid,
    unsigned gidIndex)
{
    (void)(GarpGidTt_Event(
        portGid, &portGid->machines[gidIndex], GARP_GID_JOIN));
}


// NAME:        GarpGid_LeaveRequest
//
// PURPOSE:     Process the state machine of an attribute with
//              GARP_GID_LEAVE event.
//
// PARAMETERS:  Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      None.

static
void GarpGid_LeaveRequest(
    GarpGid* portGid,
    unsigned gidIndex)
{
    (void)(GarpGidTt_Event(
        portGid, &portGid->machines[gidIndex], GARP_GID_LEAVE));
}


// NAME:        GarpGid_RegistrarIn
//
// PURPOSE:     Check if a registrar is in 'In' state.
//
// PARAMETERS:  Pointer to machine.
//
// RETURN:      TRUE, if registrar state is 'In'.
//              FALSE, otherwise

static
BOOL GarpGid_RegistrarIn(
    GarpGidMachine* machine)
{
    return GarpGidTt_In(machine);
}


// NAME:        GarpGid_LeaveTimerExpired
//
// PURPOSE:     Leave timer expired for the port.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGid_LeaveTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;
    unsigned gidIndex;


    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        portGid->leaveTimerRunning = FALSE;

        SwitchGvrp_Trace(node, portGid, NULL, "Leave timer Expired");

        // Leave timer expired. Check the leave timer transition
        // table for each attributes to find expiry of registration.
        //
        for (gidIndex = 0; gidIndex < portGid->garp->lastGidIndex;
                gidIndex++)
        {
            if (GarpGidTt_LeaveTimerExpiry(
                        portGid,&portGid->machines[gidIndex]) ==
                GARP_GID_LEAVE)
            {
                portGid->garp->leaveIndicationFn(
                    node, portGid->garp, portGid, gidIndex);
                GarpGip_PropagateLeave(node, portGid, gidIndex);
            }
        }

        // Reschedule leave timer, if required, for any attribute.
        // As leave timer deregisters attributes in four steps,
        // schedule multiple expiries.
        //
        // if (portGid->canStartLeaveTimer && (!portGid->leaveTimerRunning))
        // {
        //      Garp_StartTimer(
        //                node,
        //                portGid->garp,
        //                portGid->interfaceIndex,
        //                portGid->portNumber,
        //                GARP_LeaveTimer,
        //                portGid->leaveTimeout4);
        //
        //      portGid->leaveTimerRunning = TRUE;
        //
        //      SwitchGvrp_Trace(node, portGid, NULL,
        //              "Leave Timer scheduled");
        //
        // portGid->canStartLeaveTimer = FALSE;

        GarpGip_DoActions(node, portGid);
    }
}


// NAME:        GarpGid_LeaveAllTimerExpired
//
// PURPOSE:     Leave all timer expired for the port.
//              Initiate necessary actions.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGid_LeaveAllTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        if (portGid->leaveallCountdown > 1)
        {
            // One step of the multi-step leave all
            // timer expiry. Schedule another for next step.
            //
            portGid->leaveallCountdown--;
            Garp_StartTimer(
                node,
                portGid->garp,
                portGid->interfaceIndex,
                portGid->portNumber,
                GARP_LeaveAllTimer,
                portGid->leaveallTimeoutN);
        }
        else
        {
            clocktype initialLeaveAllTimerDelay;
            MacSwitch* sw = Switch_GetSwitchData(node);

            SwitchGvrp_Trace(node, portGid, NULL, "Leave All timer Expired");

            // Periodic leave all timer expired.
            // Reset leaveallCountdown flag.
            //
            portGid->leaveallCountdown = 0;
            portGid->canStartJoinTimer = TRUE;

            // Schedule join timer as periodic leave all timer has
            // indicated a leave all event transmission.
            //
            if ((!portGid->joinTimerRunning)
                    && (!portGid->holdTx))
            {
                Garp_StartRandomTimer(
                    node,
                    portGid->garp,
                    portGid->interfaceIndex,
                    portGid->portNumber,
                    GARP_JoinTimer,
                    portGid->joinTimeout);

                portGid->canStartJoinTimer = FALSE;
                portGid->joinTimerRunning = TRUE;
            }

            // Schedule another leave all timer for next period.
            //
            initialLeaveAllTimerDelay = (clocktype)
                ((1 + (RANDOM_erand(garp->leaveAllTimerSeed) / 2.0)) * sw->leaveallTime)
                    / GARP_GID_LEAVEALL_COUNT;

                Garp_StartTimer(
                    node,
                    portGid->garp,
                    portGid->interfaceIndex,
                    portGid->portNumber,
                    GARP_LeaveAllTimer,
                    initialLeaveAllTimerDelay);
                    //portGid->leaveallTimeoutN);
        }
    }
}


// NAME:        GarpGid_JoinTimerExpired
//
// PURPOSE:     Join timer expired for the port.
//              Initiate necessary actions.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGid_JoinTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        SwitchGvrp_Trace(node, portGid, NULL, "Join timer Expired");

        portGid->joinTimerRunning = FALSE;

        if (portGid->isEnabled)
        {
            // Join timer expired and port is enabled for
            // transmission. Call transmission process to
            // create and send a PDU.
            //
            garp->transmitFn(node, garp, portGid);

            // Message sent. Set hold transmission flag and
            // schedule a hold timer.
            //
            portGid->holdTx = TRUE;
            Garp_StartTimer(
                node,
                portGid->garp,
                portGid->interfaceIndex,
                portGid->portNumber,
                GARP_HoldTimer,
                portGid->holdTimeout);

            SwitchGvrp_Trace(node, portGid, NULL, "AttribState");
        }
    }
}


// NAME:        GarpGid_HoldTimerExpired
//
// PURPOSE:     Hold timer expired for the port. Call GID to
//              initiate necessary actions.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGid_HoldTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        portGid->holdTx = FALSE;
        GarpGid_DoActions(node, portGid);
    }
}


// -------------------------------------------------------------------------
// GIP : Propagate Single Attribute

// NAME:        GarpGip_PropagateJoin
//
// PURPOSE:     Propagates a join indication. Send join requests
//              to other ports if required.
//
// PARAMETERS:  Node data.
//              Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      None.

static
void GarpGip_PropagateJoin(
    Node* node,
    GarpGid* portGid,
    unsigned gidIndex)
{
    unsigned joiningMembers = 0;
    GarpGid* toPortGid = NULL;

    if (portGid->isConnected)
    {
        joiningMembers = (portGid->garp->gip[gidIndex] += 1);

        if (joiningMembers <= 2)
        {
            toPortGid = portGid;

            while ((toPortGid = toPortGid->nextInConnectedRing) != portGid)
            {
                if ((joiningMembers == 1)
                        || (GarpGid_RegisteredHere(toPortGid, gidIndex)))
                {
                    GarpGid_JoinRequest(toPortGid, gidIndex);

                    toPortGid->garp->joinPropagatedFn(
                        node,
                        portGid->garp,
                        portGid, gidIndex);
                }
            }
        }
    }
}


// NAME:        GarpGip_PropagateLeave
//
// PURPOSE:     Propagate a leave indication for a single attribute.
//              Send leave requests to other ports if required.
//
// PARAMETERS:  Node data.
//              Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      None.

static
void GarpGip_PropagateLeave(
    Node* node,
    GarpGid* portGid,
    unsigned gidIndex)
{
    unsigned remainingMembers;
    GarpGid* toPortGid = NULL;

    if (portGid->isConnected)
    {
        remainingMembers = (portGid->garp->gip[gidIndex] -= 1);

        if (remainingMembers <= 1)
        {
            toPortGid = portGid;

            while ((toPortGid = toPortGid->nextInConnectedRing) != portGid)
            {
                if ((remainingMembers == 0)
                        || (GarpGid_RegisteredHere(toPortGid, gidIndex)))
                {
                    SwitchGvrp_Trace(node, toPortGid, NULL,
                        "Leave Propagated.");

                    GarpGid_LeaveRequest(toPortGid, gidIndex);

                    toPortGid->garp->leavePropagatedFn(
                        node,
                        portGid->garp,
                        portGid, gidIndex);
                }
            }
        }
    }
}


// NAME:        GarpGip_PropagatesTo
//
// PURPOSE:     Check if propagation is required.
//
// PARAMETERS:  Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      TRUE, if propagation required.
//              FALSE, otherwise.

static
BOOL GarpGip_PropagatesTo(
    GarpGid* portGid,
    unsigned gidIndex)
{
    GarpGid* toPortGid = portGid;

    if (portGid->isConnected)
    {
        while ((toPortGid = toPortGid->nextInPortRing) != portGid)
        {
            if (GarpGid_RegisteredHere(toPortGid, gidIndex))
            {
                return TRUE;
            }
        }

        return FALSE;
    }
    else
    {
        return FALSE;
    }
}


// -------------------------------------------------------------------------
//  GIP : Creation And Destruction

// NAME:        Garp_Init
//
// PURPOSE:     Initialize random seeds
//
// PARAMETERS:  Node, Garp, and interfaceIndex

void Garp_Init(
    Node* node,
    Garp* garp)
{
    RANDOM_SetSeed(garp->randomTimerSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_GARP);
    RANDOM_SetSeed(garp->leaveAllTimerSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_GARP,
                   1);
}

// NAME:        GarpGip_CreateGip
//
// PURPOSE:     Create a new instance of GIP, allocating space for
//              propagation counts for up to maximum attributes.
//
// PARAMETERS:  Maximum attribute to handle.
//
// RETURN:      TRUE, if Created.

BOOL GarpGip_CreateGip(
    unsigned maxAttributes,
    unsigned** gip)
{
    unsigned* aGip =
        (unsigned*) MEM_malloc(sizeof(unsigned) * maxAttributes);
    memset(aGip, 0, sizeof(unsigned) * maxAttributes);
    *gip = aGip;

    return TRUE;
}


// NAME:        GarpGip_DestroyGip
//
// PURPOSE:     Release allocated Gip space for a garp application.
//
// PARAMETERS:  Pointer to gip.
//
// RETURN:      None.

void GarpGip_DestroyGip(
    void* gip)
{
    MEM_free(gip);
}


// -------------------------------------------------------------------------
// GIP : Connect, disconnect Ports

// NAME:        GarpGip_ConnectIntoRing
//
// PURPOSE:     Connecte a port in connected ring.
//
// PARAMETERS:  Port GarpGid.
//
// RETURN:      None.

static
void GarpGip_ConnectIntoRing(
    GarpGid* portGid)
{
    GarpGid* firstConnected = NULL;
    GarpGid* lastConnected = NULL;

    portGid->isConnected = TRUE;
    portGid->nextInConnectedRing = portGid;

    firstConnected = portGid;

    do
    {
        firstConnected = firstConnected->nextInPortRing;
    } while (!firstConnected->isConnected);

    portGid->nextInConnectedRing = firstConnected;

    lastConnected = firstConnected;

    while (lastConnected->nextInConnectedRing != firstConnected)
    {
        lastConnected = lastConnected->nextInConnectedRing;
    }

    lastConnected->nextInConnectedRing = portGid;
}


// NAME:        GarpGip_ConnectPort
//
// PURPOSE:     If a GID instance for this application and port number
//              is found, is enabled, and is not already connected,
//              then connect that port into the GIP propagation ring.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGip_ConnectPort(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;
    unsigned gidIndex;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        if ((!portGid->isEnabled) || (portGid->isConnected))
        {
            return;
        }

        // Port is enabled but not connected into GIP propagation ring.
        // Connect this port in GIP propagation ring.
        //
        GarpGip_ConnectIntoRing(portGid);

        // Reset flags before any further transmission.
        //
        portGid->lastTransmitted = 0;
        portGid->lastToTransmit = garp->lastGidIndex;

        SwitchGvrp_Trace(node, portGid, NULL, "Port connected into ring");

        // Check all GID machine if join request or join
        // propagation is required. Issue a join propagation
        // for those machines that are registered here.
        //
        for (gidIndex = 0; gidIndex < garp->lastGidIndex;
                gidIndex++)
        {
            if (GarpGip_PropagatesTo(portGid, gidIndex))
            {
                GarpGid_JoinRequest(portGid, gidIndex);
            }

            if (GarpGid_RegisteredHere(portGid, gidIndex))
            {
                GarpGip_PropagateJoin(node, portGid, gidIndex);
            }
        }

        // Carry out the actions accumulated to all ports
        // connected to GIP ring and set isConnected flag.
        //
        GarpGip_DoActions(node, portGid);
        portGid->isConnected = TRUE;

        SwitchGvrp_Trace(node, portGid, NULL, "PortState");
    }

}


// NAME:        GarpGip_DisconnectFromRing
//
// PURPOSE:     Disconnect a port from connected ring.
//
// PARAMETERS:  Port gid.
//
// RETURN:      None.

static
void GarpGip_DisconnectFromRing(
    GarpGid* portGid)
{
    GarpGid* firstConnected = NULL;
    GarpGid* lastConnected = NULL;

    firstConnected = portGid->nextInConnectedRing;
    portGid->nextInConnectedRing = portGid;
    portGid->isConnected = FALSE;

    lastConnected = firstConnected;

    while (lastConnected->nextInConnectedRing != portGid)
    {
        lastConnected = lastConnected->nextInConnectedRing;
    }

    lastConnected->nextInConnectedRing = firstConnected;
}


// NAME:        GarpGip_DisconnectPort
//
// PURPOSE:     Disconnect a port when it is disabled; it is unable
//              to send/receive PDUs.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGip_DisconnectPort(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;
    unsigned gidIndex;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        if ((!portGid->isEnabled) || (!portGid->isConnected))
        {
            return;
        }

        SwitchGvrp_Trace(node, portGid, NULL, "Port disconnected from ring");

        // Port is going to disconnect from GIP ring. Process GID
        // machines to propagate or request leave indications.
        //
        for (gidIndex = 0; gidIndex < garp->lastGidIndex;
                gidIndex++)
        {
            if (GarpGip_PropagatesTo(portGid, gidIndex))
            {
                GarpGid_LeaveRequest(portGid, gidIndex);
            }

            if (GarpGid_RegisteredHere(portGid, gidIndex))
            {
                GarpGip_PropagateLeave(node, portGid, gidIndex);
            }
        }

        // Carry out actions accumulated at all ports connected into
        // GIP ring. Finally, disconnect the port from GIP ring.
        //
        GarpGip_DoActions(node, portGid);
        GarpGip_DisconnectFromRing(portGid);
        portGid->isConnected = FALSE;
    }
}


//-------------------------------------------------------------------------
// Process received frame for GARP application.

// NAME:        Garp_HandleReceivedFrameFromMac
//
// PURPOSE:     Garp receives a PDU from MAC. Pass the PDU to proper
//              garp application as indicated by destination group address.
//
// PARAMETERS:  Node data.
//              Switch data.
//              PDU.
//              Source addess.
//              Destination addess.
//              Incoming interface.
//              Vlan id of incoming PDU.
//              Priority of incoming PDU.
//
// RETURN:      None.

void Garp_HandleReceivedFrameFromMac(
    Node* node,
    MacSwitch* sw,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    int incomingInterfaceIndex,
    VlanId vlanId,
    TosType priority)
{
    SwitchPort* port = Switch_GetPortDataFromInterface(
                              sw, incomingInterfaceIndex);

    // The switch is GARP enabled.
    // Forward frame to proper GARP application.
    //


    if (MAC_IsIdenticalMac802Address(&destAddr,
                                     &sw->gvrp->GVRP_Group_Address))
    {
            GarpGid_ReceivePdu(
                node, &(sw->gvrp->garp), port->portNumber, msg);
    }
    else{
            ERROR_ReportError(
                "Garp_HandleReceivedFrameFromMac: "
                "Unknown GARP application.\n");
    }
}



//--------------------------------------------------------------------------
// Interface layer routines

// NAME:        Garp_Layer
//
// PURPOSE:     Garp layer routine.
//
// PARAMETERS:  Node data.
//              Interface index.
//              Self message.
//
// RETURN:      None.

void Garp_Layer(
    Node* node,
    int interfaceIndex,
    Message* msg)
{
    GarpGidTimerInfo* info = (GarpGidTimerInfo*) MESSAGE_ReturnInfo(msg);

    // Call respective event function as indicated by info field.
    //
    info->eventFn(node, info->garp, info->portNumber);

    MESSAGE_Free(node, msg);
}


// -------------------------------------------------------------------------
// Initialization

// NAME:        GarpGid_CreateGidMachines
//
// PURPOSE:     Create gid machines for a port for each garp application.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port gid.
//
// RETURN:      TRUE, if created.

static
BOOL GarpGid_CreateGidMachines(
    Node* node,
    Garp* garp,
    GarpGid* portGid,
    GarpGidMachine** machines)
{
    GarpGidMachine* aMachine = NULL;
    GarpGidMachine* tempMachine = NULL;
    unsigned mCount;

    // Number of GarpGid machine should be equal to
    // (Max Vlans + untransmittedMachine).
    //
    aMachine = (GarpGidMachine*)
        MEM_malloc(sizeof(GarpGidMachine) * (garp->maxGidIndex + 2));
    *machines = aMachine;

    // Initialize all allocated machines. Initially applicants
    // are very anxious observers and registrars are empty.
    //
    tempMachine = aMachine;
    for (mCount = 0; mCount <= garp->maxGidIndex; mCount++)
    {
        tempMachine->applicant = GID_Vo;
        tempMachine->registrar = GID_Mt;
        tempMachine++;
    }
    return TRUE;
}


// NAME:        GarpGid_CreateGid
//
// PURPOSE:     Creates a new instance of GID.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Interface index.
//              Port number
//
// RETURN:      TRUE, if created.

static
BOOL GarpGid_CreateGid(
    Node* node,
    Garp* garp,
    int interfaceIndex,
    unsigned short portNumber,
    GarpGid** gid)
{
    clocktype initialLeaveAllTimerDelay;

    MacSwitch* sw = Switch_GetSwitchData(node);

    GarpGid* portGid = (GarpGid*) MEM_malloc(sizeof(GarpGid));
    memset(portGid, 0, sizeof(GarpGid));
    *gid = portGid;

    portGid->garp = garp;
    portGid->interfaceIndex = interfaceIndex;
    portGid->portNumber = portNumber;
    portGid->nextInPortRing = portGid;
    portGid->nextInConnectedRing = portGid;

    // Unused and non-functional.
    // Retained for comptability with code in the standard.
    //
    portGid->isPointToPoint = TRUE;

    // Assign different timers read previously.
    // Note: These timer values are assumed to be same across
    //       the switched network.
    //
    portGid->joinTimeout = sw->joinTime;
    portGid->leaveallCountdown = GARP_GID_LEAVEALL_COUNT;
    portGid->leaveTimeout4 = sw->leaveTime / GARP_GID_LEAVEALL_COUNT;
    portGid->leaveallTimeoutN = sw->leaveallTime / GARP_GID_LEAVEALL_COUNT;
    portGid->holdTimeout = GARP_GID_HOLD_TIME_DEFAULT;

    // Number of GarpGid machine should be equal to
    // (MaxVlans + untransmittedMachine).
    //
    GarpGid_CreateGidMachines(node, garp, portGid, &portGid->machines);

    portGid->lastTransmitted = garp->lastGidIndex;
    portGid->lastToTransmit = garp->lastGidIndex;

    // untransmitMachine should be reserved at very end
    // of GID machine array.
    //
    portGid->untransmitMachine = garp->maxGidIndex + 1;

    // Schedule periodic Leaveall Timer.
    //
    initialLeaveAllTimerDelay = (clocktype)
        ((1 + (RANDOM_erand(garp->leaveAllTimerSeed) / 2.0)) * sw->leaveallTime)
            / GARP_GID_LEAVEALL_COUNT;

    Garp_StartTimer(
        node,
        portGid->garp,
        portGid->interfaceIndex,
        portGid->portNumber,
        GARP_LeaveAllTimer,
        initialLeaveAllTimerDelay);
        //portGid->leaveallTimeoutN);

    return TRUE;
}


// NAME:        GarpGid_ManageAttribute
//
// PURPOSE:     Initialize the machine of configured attribute and take
//              necessary actions as required. The directive can be one
//              of the followed management events:
//                  GARP_GID_NORMAL_OPERATION
//                  GARP_GID_NO_PROTOCOL
//                  GARP_GID_NORMAL_REGISTRATION
//                  GARP_GID_FIX_REGISTRATION
//                  GARP_GID_FORBID_REGISTRATION.
//
// PARAMETERS:  Node data.
//              Port gid.
//              GarpGid index of the attribute.
//
// RETURN:      TRUE, if created.

void GarpGid_ManageAttribute(
    Node* node,
    GarpGid* portGid,
    unsigned index,
    GarpGidEvent directive)
{
    GarpGidMachine* machine;
    GarpGidEvent event;

    machine = &portGid->machines[index];
    event = GarpGidTt_Event(portGid, machine, directive);

    if (event == GARP_GID_JOIN)
    {
        portGid->garp->joinIndicationFn(
            node, portGid->garp, portGid, index);
        GarpGip_PropagateJoin(node, portGid, index);
    }
    else if (event == GARP_GID_LEAVE)
    {
        portGid->garp->leaveIndicationFn(
            node, portGid->garp, portGid, index);
        GarpGip_PropagateLeave(node, portGid, index);
    }
}


// NAME:        GarpGid_AddPort
//
// PURPOSE:     Add a new port to the port ring.
//
// PARAMETERS:  Pointer to existing ports.
//              Pointer to new ports.
//
// RETURN:      Modified pointer to all ports.

static
GarpGid* GarpGid_AddPort(
    GarpGid* existingPorts,
    GarpGid* newPortGid)
{
    GarpGid* prior;
    GarpGid* next;
    unsigned short newPortNumber;

    if (existingPorts != NULL)
    {
        newPortNumber = newPortGid->portNumber;
        next = existingPorts;

        for (;;)
        {
            prior = next;
            next = prior->nextInPortRing;
            if (prior->portNumber <= newPortNumber)
            {
                if ((next->portNumber <= prior->portNumber)
                        || (next->portNumber > newPortNumber))
                {
                    break;
                }
            }
            else // if (prior->portNumber > newPortNumber)
            {
                if ((next->portNumber <= prior->portNumber)
                        && (next->portNumber > newPortNumber))
                {
                    break;
                }
            }
        }

        if (prior->portNumber == newPortNumber)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "GarpGid_AddPort: "
                "Port %u already exists ", newPortNumber);
            ERROR_ReportError(errStr);
        }

        prior->nextInPortRing = newPortGid;
        newPortGid->nextInPortRing = next;
    }

    newPortGid->isEnabled = TRUE;
    return newPortGid;
}


// NAME:        GarpGid_CreatePort
//
// PURPOSE:     Creates a new instance of GID, allocating space
//              for GID machines as required by the application,
//              adding the port to the ring of ports and signalling
//              the application that the new port has been created.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Interface index.
//              Number of port to be created.
//
// RETURN:      TRUE, if created.
//              FALSE, otherwise.

BOOL GarpGid_CreatePort(
    Node* node,
    Garp* garp,
    int interfaceIndex,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;

    if (!GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        if (GarpGid_CreateGid(
                node, garp, interfaceIndex, portNumber, &portGid))
        {
            garp->gid = GarpGid_AddPort(garp->gid, portGid);
            garp->addNewPortFn(node, garp, portNumber);
            return TRUE;
        }
    }
    return FALSE;
}


// NAME:        GarpGid_RemovePort
//
// PURPOSE:     Remove connected port from port ring.
//
// PARAMETERS:  Port gid.
//
// RETURN:      Modified pointer to all ports.

static
GarpGid* GarpGid_RemovePort(
    GarpGid* portGid)
{
    GarpGid* prior = portGid;
    GarpGid* next = prior->nextInPortRing;

    while (next != portGid)
    {
        prior = next;
        next = prior->nextInPortRing;
    }

    prior->nextInPortRing = portGid->nextInPortRing;
    if (prior == portGid)
    {
        return NULL;
    }
    else
    {
        return prior;
    }
}


// NAME:        GarpGid_DestroyGid
//
// PURPOSE:     Destroys the instance of GID, releasing previously
//              allocated space. Send leave indications to the
//              application for previously registered attributes.
//
// PARAMETERS:  Node data.
//              Port gid.
//
// RETURN:      None.

static
void GarpGid_DestroyGid(
    Node* node,
    GarpGid* gid)
{
    unsigned gidIndex = 0;

    for (gidIndex = 0; gidIndex < gid->garp->lastGidIndex;
            gidIndex++)
    {
        if (GarpGid_RegisteredHere(gid, gidIndex))
        {
            gid->garp->leaveIndicationFn(
                node, gid->garp, gid, gidIndex);
        }
    }

    MEM_free(gid->machines);
    MEM_free(gid);
}


// NAME:        GarpGid_DestroyPort
//
// PURPOSE:     Destroys the instance of GID, disconnecting the port
//              if it is still connected (causing leaves to propagate
//              as required), then cause leave indications for this
//              port as required, finally releasing all allocated space
//              signalling the application that the port has been removed.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number
//
// RETURN:      None.

void GarpGid_DestroyPort(
    Node* node,
    Garp* garp,
    unsigned short portNumber)
{
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(garp->gid, portNumber, &portGid))
    {
        GarpGip_DisconnectPort(node, garp, portNumber);
        garp->gid = GarpGid_RemovePort(portGid);
        GarpGid_DestroyGid(node, portGid);
        garp->removePortFn(node, garp, portNumber);
    }
}

// -------------------------------------------------------------------------
