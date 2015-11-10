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


#ifndef MAC_GARP_H
#define MAC_GARP_H

// This GARP (Generic Attribute Registration Protocol) implementation
// follows the sample code of IEEE 802.1D, 1998 edition.


// /**
// CONSTANT    :: GARP_PROTOCOL_ID : 0x0001
// DESCRIPTION :: GARP PDU protocol ID.
// **/
#define GARP_PROTOCOL_ID                    0x0001

// /**
// CONSTANT    :: GARP_PDU_LENGTH_MAX : 1500
// DESCRIPTION :: Maximum length of a GARP PDU.
// **/
#define GARP_PDU_LENGTH_MAX                 1500

// /**
// CONSTANT    :: GARP_END_OF_PDU : 0x00
// DESCRIPTION :: End of PDU mark.
// **/
#define GARP_END_OF_PDU                     0x00

// /**
// CONSTANT    :: GARP_END_OF_ATTRIBUTE : 0x00
// DESCRIPTION :: End of attribute mark.
// **/
#define GARP_END_OF_ATTRIBUTE               0x00

// /**
// CONSTANT    :: GARP_ATTRIBUTE_ID_MIN : 0x01
// DESCRIPTION :: Minimum value of attribute ID.
// **/
#define GARP_ATTRIBUTE_ID_MIN               0x01

// /**
// CONSTANT    :: GARP_ATTRIBUTE_ID_MAX : 0xFF
// DESCRIPTION :: Maximum value of attribute ID.
// **/
#define GARP_ATTRIBUTE_ID_MAX               0xFF

// /**
// CONSTANT    :: GARP_ATTRIBUTE_LENGTH_MIN : 0x02
// DESCRIPTION :: Minimum length of an attribute.
// **/
#define GARP_ATTRIBUTE_LENGTH_MIN           0x02

// /**
// CONSTANT    :: GARP_ATTRIBUTE_LENGTH_MAX : 0xFF
// DESCRIPTION :: Maximum length of an attribute.
// **/
#define GARP_ATTRIBUTE_LENGTH_MAX           0xFF

// /**
// CONSTANT    :: GARP_GID_HOLD_TIME_DEFAULT : 100 * MILLI_SECOND
// DESCRIPTION :: Default value of an hold timer duration.
// **/
#define GARP_GID_HOLD_TIME_DEFAULT          (100 * MILLI_SECOND)

// /**
// CONSTANT    :: GARP_GID_JOIN_TIME_DEFAULT : 200 * MILLI_SECOND
// DESCRIPTION :: Default value of an join timer duration.
// **/
#define GARP_GID_JOIN_TIME_DEFAULT          (200 * MILLI_SECOND)

// /**
// CONSTANT    :: GARP_GID_LEAVE_TIME_DEFAULT : 600 * MILLI_SECOND
// DESCRIPTION :: Default value of an leave timer duration.
// **/
#define GARP_GID_LEAVE_TIME_DEFAULT         (600 * MILLI_SECOND)

// /**
// CONSTANT    :: GARP_GID_LEAVEALL_TIME_DEFAULT : 10000 * MILLI_SECOND
// DESCRIPTION :: Default value of an leave all timer duration.
// **/
#define GARP_GID_LEAVEALL_TIME_DEFAULT      (10000 * MILLI_SECOND)

// /**
// CONSTANT    :: GARP_GID_LEAVEALL_COUNT : 4
// DESCRIPTION :: Leave all count value.
// **/
#define GARP_GID_LEAVEALL_COUNT             4

// -------------------------------------------------------------------------
// Generic Attribute Registration Protocol : Common application elements
// -------------------------------------------------------------------------

// Each GARP, i.e., each instance of an application that uses the GARP
// protocol, is represented as a struct or control block with common
// initial fields. These comprise pointers to application-specific
// functions that are used by the GID and GIP components to signal
// protocol events to the application, and other controls common to all
// applications. The pointers include a pointer to the instances of GID
// (one per port) for the application, and to GIP (one per application).
// The signaling functions include the addition and removal of ports,
// which the application should use to initialize port attributes with
// any management state required.
//
// /**
// STRUCT      :: Garp
// DESCRIPTION :: Elements of switch GARP data.
// **/
typedef struct
{
    struct garp_gid* gid;
    unsigned maxGidIndex;
    unsigned lastGidIndex;

    RandomSeed randomTimerSeed;
    RandomSeed leaveAllTimerSeed;

    unsigned* gip;

    void (*joinIndicationFn)(Node*, void*, void* gid,
                             unsigned joiningGidIndex);
    void (*leaveIndicationFn)(Node*, void*, void* gid,
                             unsigned leavingGidIndex);

    void (*joinPropagatedFn)(Node*, void*, void* gid,
                             unsigned joiningGidIndex);
    void (*leavePropagatedFn)(Node*, void*, void* gid,
                             unsigned leavingGidIndex);

    void (*transmitFn)(Node*, void*, void* gid);
    void (*receiveFn)(Node*, void*, void* gid, Message*);

    void (*addNewPortFn)(Node*, void*, unsigned short portNumber);
    void (*removePortFn)(Node*, void*, unsigned short portNumber);
} Garp;


// -------------------------------------------------------------------------
// GID : GARP INFORMATION DISTRIBUTION PROTOCOL : GID MACHINES
// -------------------------------------------------------------------------

// The GID state of each attribute on each port is held in a GID 'machine'
// which comprises the Applicant and Registrar states for the port,
// including control modifiers for the states.
//
// The GID machine and its internal representation of GID states is not
// accessed directly : this struct is defined here to allow the GID
// Control Block (which is accessed externally), to be defined below.
//
// /**
// STRUCT      :: GarpGidMachine
// DESCRIPTION :: Elements of a GARP GID mahine.
// **/
typedef struct
{
    unsigned applicant : 5;
    unsigned registrar : 5;
} GarpGidMachine;


// -------------------------------------------------------------------------
// GID : GARP INFORMATION DISTRIBUTION PROTOCOL : MANAGEMENT STATES
// -------------------------------------------------------------------------
//
// Implementation independent representations of the GID states of a single
// attribute including management controls, following the standard state
// machine specification as follows:
//
//  Applicant :
//      Major state :           GARP_GID_VeryAnxious, GARP_GID_Anxious,
//                              GARP_GID_Quiet, GARP_GID_Leaving
//      Current participation : Active, Passive
//      Management controls :   GARP_GID_Normal, GARP_GID_NoProtocol
//
//  Registrar :
//      Major state :           GARP_GID_In, GARP_GID_Leave, GARP_GID_Empty
//      Management controls :   GARP_GID_NormalRegistration,
//                              GARP_GID_RegistrationFixed,
//                              GARP_GID_RegistrationForbidden
//
// with a struct defined to return the current state (caller supplies by
// reference).
//

// /**
// ENUM        :: GarpGid_ApplicantState
// DESCRIPTION :: Enumerated values of an applicant's states.
// **/
typedef enum
{
    GARP_GID_VeryAnxious,
    GARP_GID_Anxious,
    GARP_GID_Quiet,
    GARP_GID_Leaving
} GarpGid_ApplicantState;

// /**
// ENUM        :: GarpGid_ApplicantMgt
// DESCRIPTION :: Enumerated values of an applicant's management states.
// **/
typedef enum
{
    GARP_GID_Normal,
    GARP_GID_NoProtocol
} GarpGid_ApplicantMgt;

// /**
// ENUM        :: GarpGid_RegistrarState
// DESCRIPTION :: Enumerated values of a registrar's states.
// **/
typedef enum
{
    GARP_GID_In,
    GARP_GID_Leave,
    GARP_GID_Empty
} GarpGid_RegistrarState;

// /**
// ENUM        :: GarpGid_RegistrarMgt
// DESCRIPTION :: Enumerated values of a registrar's management states.
// **/
typedef enum
{
    GARP_GID_NormalRegistration,
    GARP_GID_RegistrationFixed,
    GARP_GID_RegistrationForbidden
} GarpGid_RegistrarMgt;

// /**
// STRUCT      :: GarpGidStates
// DESCRIPTION :: Elements to describe a GID's state.
// **/
typedef struct
{
    unsigned applicantState : 2;
    unsigned applicantMgt   : 1;

    unsigned registrarState : 2;
    unsigned registrarMgt   : 2;
} GarpGidStates;


// -------------------------------------------------------------------------
// GID : GARP INFORMATION DISTRIBUTION PROTOCOL : PROTOCOL EVENTS
// -------------------------------------------------------------------------
//
// A complete set of events is specified to allow events to be passed
// around without rewriting. The other end of the spectrum would have
// been one set of events for received messages, a set of receive
// indications, a set of user requests etc. and separate transition
// tables for each.
//
// Event definitions are ordered in an attempt not to waste space in
// transition tables and to pack switch cases together.
//
// /**
// ENUM        :: GarpGidEvent
// DESCRIPTION :: Enumerated values to all possible events of GID.
// **/
typedef enum
{
    GARP_GID_NULL,                  // 0
    GARP_GID_RX_LEAVE_EMPTY,        // 1
    GARP_GID_RX_LEAVE_IN,           // 2
    GARP_GID_RX_EMPTY,              // 3
    GARP_GID_RX_JOIN_EMPTY,         // 4
    GARP_GID_RX_JOIN_IN,            // 5
    GARP_GID_JOIN,                  // 6
    GARP_GID_LEAVE,                 // 7
    GARP_GID_NORMAL_OPERATION,      // 8
    GARP_GID_NO_PROTOCOL,           // 9
    GARP_GID_NORMAL_REGISTRATION,   // 10
    GARP_GID_FIX_REGISTRATION,      // 11
    GARP_GID_FORBID_REGISTRATION,   // 12
    GARP_GID_RX_LEAVE_ALL,          // 13
    GARP_GID_RX_LEAVE_ALL_RANGE,    // 14
    GARP_GID_TX_LEAVE_EMPTY,        // 15
    GARP_GID_TX_LEAVE_IN,           // 16
    GARP_GID_TX_EMPTY,              // 17
    GARP_GID_TX_JOIN_EMPTY,         // 18
    GARP_GID_TX_JOIN_IN,            // 19
    GARP_GID_TX_LEAVE_ALL,          // 20
    GARP_GID_TX_LEAVE_ALL_RANGE     // 21
} GarpGidEvent;


enum
{
    GARP_GID_RX_EVENT_COUNT  = (GARP_GID_RX_JOIN_IN + 1),
    GARP_GID_REQ_EVENTS      = 2,
    GARP_GID_AMGT_EVENTS     = 2,
    GARP_GID_RMGT_EVENTS     = 3,
    GARP_GID_TX_EVENT_COUNT  = 7
};

typedef void (*GARP_TimerExpiryEventFn)(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// ENUM        :: GarpGidTimerType
// DESCRIPTION :: Enumerated values for GARP self timers.
// **/
typedef enum
{
    GARP_JoinTimer,
    GARP_HoldTimer,
    GARP_LeaveTimer,
    GARP_LeaveAllTimer,
} GarpGidTimerType;

// /**
// STRUCT      :: GarpGidTimerInfo
// DESCRIPTION :: Elements of a GARP self timers.
// **/
typedef struct garp_timer_info
{
    GARP_TimerExpiryEventFn eventFn;
    Garp* garp;
    unsigned short portNumber;
} GarpGidTimerInfo;


// -------------------------------------------------------------------------
// GID : GARP INFORMATION DISTRIBUTION PROTOCOL : PROTOCOL INSTANCES
// -------------------------------------------------------------------------
//
// Each instance of GID is represented by one of these control blocks.
// There is a single instance of GID per port for each GARP application.
//
// /**
// STRUCT      :: GarpGid
// DESCRIPTION :: Elements of a GARP GID.
// **/
typedef struct garp_gid
{
    Garp* garp;
    int interfaceIndex;
    unsigned short portNumber;

    // Connect next port to GID ring that holds all
    // possible GIDs for an GARP application.
    struct garp_gid* nextInPortRing;

    // Connect next port in GIP ring where all ports are in forwarding
    // state. Only this ports can propagate Join/Leave requests.
    struct garp_gid* nextInConnectedRing;

    // Flag to indicate if a port can send/receive GARP PDUs.
    unsigned isEnabled          : 1;

    // Flag to indicate if a port is connected into GIP ring.
    unsigned isConnected        : 1;

    // Flag to indicate if attached to a point-to-point link.
    unsigned isPointToPoint     : 1;

    // Flag to indicate immediate transmission of a GARP PDU.
    unsigned canScheduleTxNow   : 1;

    // Flag to indicate if Join Timer is scheduled.
    unsigned canStartJoinTimer  : 1;

    // Flag to indicate if Join Timer is running.
    unsigned joinTimerRunning   : 1;

    // Flag to indicate if Leave Timer is scheduled.
    unsigned canStartLeaveTimer : 1;

    // Flag to indicate if Leave Timer is running.
    unsigned leaveTimerRunning  : 1;

    // Flag to control immediate transmission.
    unsigned txNowScheduled     : 1;

    // Flag to holds transmission until holddown timer expires.
    unsigned holdTx             : 1;

    // Flag to indicate pending transmission.
    unsigned txPending          : 1;

    // Different timer periods and leave all count.
    clocktype joinTimeout;
    clocktype leaveTimeout4;
    clocktype holdTimeout;
    int leaveallCountdown;
    clocktype leaveallTimeoutN;

    // Pointer to GID machines array.
    GarpGidMachine* machines;

    // Flags to control message transmission.
    unsigned lastTransmitted;
    unsigned lastToTransmit;
    unsigned untransmitMachine;
} GarpGid;


// -------------------------------------------------------------------------
// Interface functions.

// /**
// FUNCTION   :: Garp_Init
// LAYER      :: Mac
// PURPOSE    :: Initialize the main Garp data structure.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + garp           : Garp*          : Pointer to GARP data.
// RETURN     :: void : 
// **/
void Garp_Init(Node* node,
               Garp* garp);

// /**
// FUNCTION   :: GarpGid_CreatePort
// LAYER      :: Mac
// PURPOSE    :: Create a new instance of GID for the given port.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + garp           : Garp*          : Pointer to GARP data.
// + interfaceIndex : int            : Interface index.
// + portNumber     : unsigned short : Assigned number to the port.
// RETURN     :: BOOL                : TRUE, if instance can be created.
//                                     FALSE, otherwise.
// **/
BOOL GarpGid_CreatePort(
    Node* node,
    Garp* garp,
    int interfaceIndex,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_DestroyPort
// LAYER      :: Mac
// PURPOSE    :: Destroy instance of GID for the given port.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + garp           : Garp*          : Pointer to GARP data.
// + portNumber     : unsigned short : Assigned number to the port.
// RETURN     :: None.
// **/
void GarpGid_DestroyPort(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_FindPort
// LAYER      :: Mac
// PURPOSE    :: Find the GID instance for the given port.
// PARAMETERS ::
// + firstGid       : GarpGid*       : Pointer to GID list.
// + portNumber     : unsigned short : Assigned number to the port.
// + gid            : GarpGid**      : Pointer to collect GID instance.
// RETURN     :: BOOL                : TRUE, if port found,
//                                     FALSE, otherwire.
// **/
BOOL GarpGid_FindPort(
    GarpGid* firstGid,
    unsigned short portNumber,
    GarpGid** gid);

// /**
// FUNCTION   :: GarpGid_FindUnused
// LAYER      :: Mac
// PURPOSE    :: Find a machine not used by any Port.
// PARAMETERS ::
// + garp           : Garp*          : Pointer to GARP data.
// + fromIndex      : unsigned       : Index to start.
// + foundIndex     : unsigned*      : Pointer to collect index of machine.
// RETURN     :: BOOL                : TRUE, if port found,
//                                     FALSE, otherwire.
// **/
BOOL GarpGid_FindUnused(
    Garp* garp,
    unsigned fromIndex,
    unsigned* foundIndex);

// /**
// FUNCTION   :: GarpGid_ManageAttribute
// LAYER      :: Mac
// PURPOSE    :: Change attribute's management state on given port.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + portGid   : GarpGid*      : Pointer to GID data.
// + index     : unsigned      : Index of a GID mechine.
// + directive : GarpGidEvent  : Event to process on this machine.
// RETURN     :: None.
// **/
void GarpGid_ManageAttribute(
    Node* node,
    GarpGid* portGid,
    unsigned index,
    GarpGidEvent directive);

// /**
// FUNCTION   :: GarpGid_ReceiveMsg
// LAYER      :: Mac
// PURPOSE    :: Process state machines of an attribute, indicated by
//               gidIndex, with the received attribute event.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + portGid   : GarpGid*      : Pointer to GID data.
// + gidIndex  : unsigned      : Index of GID machine.
// + msg       : GarpGidEvent  : Event to process on this machine.
// RETURN     :: None.
// **/
void GarpGid_ReceiveMsg(
    Node* node,
    GarpGid* portGid,
    unsigned gidIndex,
    GarpGidEvent msg);

// /**
// FUNCTION   :: GarpGid_ReceiveLeaveAll
// LAYER      :: Mac
// PURPOSE    :: Process leave all event for all attributes present
//               at port gid.
// PARAMETERS ::
// + portGid   : GarpGid*      : Pointer to GID data.
// RETURN     :: None.
// **/
void GarpGid_ReceiveLeaveAll(
    GarpGid* portGid);

// /**
// FUNCTION   :: GarpGid_LeaveTimerExpired
// LAYER      :: Mac
// PURPOSE    :: Process after expiry of Leave timer.
// PARAMETERS ::
// + node         : Node*          : Pointer to node.
// + portGid      : GarpGid*       : Pointer to GID data.
// + portNumber   : unsigned short : Assign number to the port.
// RETURN     :: None.
// **/
void GarpGid_LeaveTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_JoinTimerExpired
// LAYER      :: Mac
// PURPOSE    :: Process after expiry of Join timer.
// PARAMETERS ::
// + node         : Node*          : Pointer to node.
// + garp         : Garp*          : Pointer to GID data.
// + portNumber   : unsigned short : Assign number to the port.
// RETURN     :: None.
// **/
void GarpGid_JoinTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_LeaveAllTimerExpired
// LAYER      :: Mac
// PURPOSE    :: Process after expiry of Leave All timer.
// PARAMETERS ::
// + node         : Node*          : Pointer to node.
// + garp         : Garp*          : Pointer to GID data.
// + portNumber   : unsigned short : Assign number to the port.
// RETURN     :: None.
// **/
void GarpGid_LeaveAllTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_HoldTimerExpired
// LAYER      :: Mac
// PURPOSE    :: Process after expiry of Hold timer.
// PARAMETERS ::
// + node         : Node*          : Pointer to node.
// + garp         : Garp*          : Pointer to GID data.
// + portNumber   : unsigned short : Assigned number to the port.
// RETURN     :: None.
// **/
void GarpGid_HoldTimerExpired(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGid_NextTx
// LAYER      :: Mac
// PURPOSE    :: Check to see either if a leave all event is
//               required to be sent or scan the GID machines
//               for messsages that require transmission.
// PARAMETERS ::
// + node      : Node*       : Pointer to node.
// + portGid   : GarpGid*    : Pointer to GID at this port.
// + gidIndex  : unsigned*   : Pointer to collect index of a machine.
// RETURN     :: GarpGidEvent.
// **/
GarpGidEvent GarpGid_NextTx(
    Node* node,
    GarpGid* portGid,
    unsigned* gidIndex);

// /**
// FUNCTION   :: GarpGid_Untransmit
// LAYER      :: Mac
// PURPOSE    :: Initiate necessary actions when all messages cannot be
//               send in a single PDU.
// PARAMETERS ::
// + portGid   : GarpGid*      : Pointer to GID data.
// RETURN     :: None.
// **/
void GarpGid_Untransmit(
    GarpGid* portGid);

// /**
// FUNCTION   :: GarpGidTt_States
// LAYER      :: Mac
// PURPOSE    :: Report given GID machine state.
// PARAMETERS ::
// + machine   : GarpGidMachine* : Pointer to particular machine.
// + state     : GarpGidStates*  : Pointer to collect state of the machine.
// RETURN     :: None.
// **/
void GarpGidTt_States(
    GarpGidMachine* machine,
    GarpGidStates* state);

// /**
// FUNCTION   :: GarpGip_CreateGip
// LAYER      :: Mac
// PURPOSE    :: Create a new instance of GIP.
// PARAMETERS ::
// + max_attributes : unsigned   : Maximum attributes to be handled.
// + gip            : unsigned** : Pointer to collect created GIP.
// RETURN     :: BOOL            : TRUE, if can be created,
//                                 FALSE, otherwise.
// **/
BOOL GarpGip_CreateGip(
    unsigned max_attributes,
    unsigned** gip);

// /**
// FUNCTION   :: GarpGip_DestroyGip
// LAYER      :: Mac
// PURPOSE    :: Destroy the instance of GIP.
// PARAMETERS ::
// + gip       : void*         : Pointer to GIP.
// RETURN  :: None.
// **/
void GarpGip_DestroyGip(
    void* gip);

// /**
// FUNCTION   :: GarpGip_ConnectPort
// LAYER      :: Mac
// PURPOSE    :: Connect the port into GIP propagation ring.
// PARAMETERS ::
// + node        : Node*          : Pointer to node.
// + garp        : Garp*          : Pointer to GARP data.
// + portNumber  : unsigned short : Assigned number to the port.
// RETURN     :: None.
// **/
void GarpGip_ConnectPort(
    Node* node,
    Garp* garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: GarpGip_DisconnectPort
// LAYER      :: Mac
// PURPOSE    :: Disconnect the port from GIP propagation ring.
// PARAMETERS ::
// + node        : Node*          : Pointer to node.
// + garp        : Garp*          : Pointer to GARP data.
// + portNumber  : unsigned short : Assigned number to the port.
// RETURN     :: None.
// **/
void GarpGip_DisconnectPort(
    Node* node,
    Garp *garp,
    unsigned short portNumber);

// /**
// FUNCTION   :: Garp_HandleReceivedFrameFromMac
// LAYER      :: Mac
// PURPOSE    :: Process a GARP application frame received from MAC.
// PARAMETERS ::
// + node       : Node*           : Pointer to node.
// + sw         : MacSwitch*      : Pointer to switch.
// + msg        : Message*        : Pointer to received message.
// + sourceAddr : NodeAddress     : Source address of the frame.
// + destAddr   : NodeAddress     : destination address of the frame.
// + incomingInterfaceIndex : int : Incoming interface of switch.
// + vlanId     : VlanId          : VLAN ID carried by the frame.
// + priority   : TosType         : Prioryty of this frame.
// RETURN     :: None.
// **/
void Garp_HandleReceivedFrameFromMac(
    Node* node,
    MacSwitch* sw,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    int incomingInterfaceIndex,
    VlanId vlanId,
    TosType priority);

// /**
// FUNCTION   :: Garp_DecodeReceivedEvent
// LAYER      :: Mac
// PURPOSE    :: Decode received event for an attribute.
// PARAMETERS ::
// + event        : GarpGidEvent  : Received event.
// + decodedEvent : GarpGidEvent* : Decoded event.
// RETURN  :: None.
// **/
void Garp_DecodeReceivedEvent(
    GarpGidEvent event,
    GarpGidEvent* decodedEvent);

// /**
// FUNCTION   :: Garp_Layer
// LAYER      :: Mac
// PURPOSE    :: GARP layer function.
// PARAMETERS ::
// + node           : Node*    : Pointer to node.
// + interfaceIndex : int      : Interface index to process the message.
// + msg            : Message* : Pointer to the message to process.
// RETURN  :: None.
// **/
void Garp_Layer(
    Node* node,
    int interfaceIndex,
    Message* msg);

#endif // MAC_GARP_H
