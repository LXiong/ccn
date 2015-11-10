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


#ifndef H323_H
#define H323_H
#include <vector>
#include "app_voip.h"
#include "circularbuffer.h"




#define INVALID_ID -1
#define IDENTIFIER_SIZE 14

// Signalling protocol type definition for H323
#define SIG_TYPE_H323     1

// Q.931 - Protocol discriminator
#define Q931_PROTOCOL_DISCRIMINATOR     0x08

// Q.931 - Call reference
#define Q931_DEFAULT_MAX_LEN_CALL_REFERENCE 0x03

#define H225_RAS_SETUP_DELAY 200 * MILLI_SECOND


// Q.931 message types.
#define Q931_ALERTING           0x01
#define Q931_CALL_PROCEEDING    0x02
#define Q931_CONNECT            0x07
#define Q931_CONNECT_ACK        0x0F
#define Q931_PROGRESS           0x03
#define Q931_SETUP              0x05
#define Q931_SETUP_ACK          0x0D

#define Q931_RESUME             0x26
#define Q931_RESUME_ACK         0x2E
#define Q931_RESUME_REJECT      0x22
#define Q931_SUSPEND            0x25
#define Q931_SUSPEND_ACK        0x2D
#define Q931_SUSPEND_REJECT     0x21
#define Q931_USER_INFORMATION   0x20

#define Q931_DISCONNECT         0x45
#define Q931_RELEASE            0x4D
#define Q931_RELEASE_COMPLETE   0x5A
#define Q931_RESTART            0x46
#define Q931_RESTART_ACK        0x4E

#define Q931_SEGMENT            0x60
#define Q931_CONGESTION_CONTROL 0x79
#define Q931_INFORMATION        0x7B
#define Q931_NOTIFY             0x6E
#define Q931_STATUS             0x7D
#define Q931_STATUS_ENQUIRY     0x75

#define H225_ProtocolID "0.0.8.2250.0.4"

#define SIZE_OF_H225_SETUP              80
#define SIZE_OF_H225_ALERTING           38
#define SIZE_OF_H225_CALL_PROCEEDING    38
#define SIZE_OF_H225_CONNECT            46
#define SIZE_OF_H225_RELEASE_COMPLETE   26

#define H323_BUFFER_SIZE  2*(SIZE_OF_H225_SETUP + \
    SIZE_OF_H225_ALERTING + SIZE_OF_H225_CALL_PROCEEDING + \
    SIZE_OF_H225_CONNECT + SIZE_OF_H225_RELEASE_COMPLETE)

typedef std::vector <CircularBuffer*> CircularBufferVector;
typedef enum
{
    H323_IDLE,
    H323_SETUP,
    H323_CALL_PROCEEDING,
    H323_ALERTING,
    H323_CONNECT
} H323CallSignalState;


typedef enum
{
    H323_INITIATOR,
    H323_RECEIVER,
    H323_NORMAL
} H323HostState;

// enum used in call signalling.
typedef enum
{
    Direct = 0,
    GatekeeperRouted,
} H323CallModel;


typedef enum
{
    NonStandardData = 0,
    Vendor,
    Gatekeeper,
    Gateway,
    Mcu,
    Terminal,
    Mc,
    UndefinedNode,
}H323EndpointType;

typedef enum
{
    none,
    create,
    join,
    invite,
    capability_negotiation,
    callIndependentSupplementaryService,
}H225ConferenceGoal;

typedef enum
{
    pointToPoint,
    oneToN,
    nToOne,
    nToN,
}H225CallType;


typedef struct struct_h225_setup_delay_msg
{
    int connectionId;
    unsigned char* data;
}H225SetupDelayMsg;


typedef struct struct_h225_transport_address
{
    NodeAddress                 ipAddr;
    unsigned short              port;
} H225TransportAddress;


// Structure of q931 General message.
typedef struct struct_q931_message
{
    unsigned char  protocolDiscriminator;   // Protocol discriminator
    unsigned char  callRefResvlen;
                   //callRefResvd:4,          // Call reference reserved
                   //callRefLength:4;         // Call reference Length
    unsigned char  callRefFlgVal;
                   //callRefFlag:1,           // call reference flag
                   //callRefValue:7;          // call reference value
    unsigned char  msgFlgType;//messageFlag:1,           // message flag
                   //messageType:7;           // Message type
} Q931Message;

//
// NAME:          Q931MessageSetCallRefReserved()
//
// PURPOSE:       Set the value of reserved for Q931Message
//
// PARAMETERS:    callRefResvlen, The variable containing the value of
//                                  callRefResvd and callRefLength.
//                          resv, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetCallRefReserved(unsigned char *callRefResvlen,
                                          unsigned char resv)
{
    //masks resv within boundry range
    resv = resv & maskChar(5, 8);

    //clears first four bits
    *callRefResvlen = *callRefResvlen & maskChar(5, 8);

    //setting the value of reserved in callRefResvlen
    *callRefResvlen = *callRefResvlen | LshiftChar(resv, 4);
}


//
// NAME:          Q931MessageSetCallRefLength()
//
// PURPOSE:       Set the value of length for Q931Message
//
// PARAMETERS:    callRefResvlen, The variable containing the value of
//                                callRefResvd and callRefLength.
//                          len, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetCallRefLength(unsigned char *callRefResvlen,
                                        unsigned char len)
{
    //masks len within boundry range
    len = len & maskChar(5, 8);

    //clears last four bits
    *callRefResvlen = *callRefResvlen & maskChar(1, 4);

    //setting the value of len in callRefResvlen
    *callRefResvlen = *callRefResvlen | len;
}


//
// NAME:          Q931MessageSetCallRefFlag()
//
// PURPOSE:       Set the value of callRefFlag for Q931Message
//
// PARAMETERS:    callRefFlgVal, The variable containing the value of
//                               callRefFlag and callRefValue.
//                         flag, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetCallRefFlag(unsigned char *callRefFlgVal,
                                      BOOL flag)
{
    unsigned char flag_char = (unsigned char) flag;

    //masks flag within boundry range
    flag_char = flag_char & maskChar(8, 8);

        //clears first bit
        *callRefFlgVal = *callRefFlgVal & maskChar(2, 8);

        //setting the value of flag in callRefFlgVal
        *callRefFlgVal = *callRefFlgVal | LshiftChar(flag_char, 1);
}


//
// NAME:          Q931MessageSetCallRefValue()
//
// PURPOSE:       Set the value of callRefValue for Q931Message
//
// PARAMETERS:    callRefFlgVal, The variable containing the value of
//                               callRefFlag and callRefValue.
//                       value, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetCallRefValue(unsigned char *callRefFlgVal,
                                       unsigned char value)
{
    //masks value within boundry range
    value = value & maskChar(2, 8);

    //clears last seven bits
    *callRefFlgVal = *callRefFlgVal & maskChar(1, 1);

    //setting the value of value in callRefFlgVal
    *callRefFlgVal = *callRefFlgVal | value;
}


//
// NAME:          Q931MessageSetMsgType()
//
// PURPOSE:       Set the value of messageType for Q931Message
//
// PARAMETERS:     msgFlgType, The variable containing the value of
//                              messageFlag and messageType.
//                     Type, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetMsgType(unsigned char *msgFlgType,
                                  unsigned int Type)
{
    unsigned char Type_char = (unsigned char)Type;

    //masks Type within boundry range
    Type_char = Type_char & maskChar(2, 8);

    //clears last seven bits
    *msgFlgType = *msgFlgType & maskChar(1, 1);

    //setting the value of value in msgFlgType
    *msgFlgType = *msgFlgType | Type_char;
}


//
// NAME:          Q931MessageSetMsgFlag()
//
// PURPOSE:       Set the value of messageFlag for Q931Message
//
// PARAMETERS:     msgFlgType, The variable containing the value of
//                              messageFlag and messageType.
//                       flag, Input value for set operation
//
// RETURN:        void
//
// ASSUMPTION:    None
//
static void Q931MessageSetMsgFlag(unsigned char *msgFlgType,
                                  BOOL flag)
{
    unsigned char flag_char = (unsigned char) flag;

    //masks flag within boundry range
    flag_char = flag_char & maskChar(8, 8);

    //clears first bit
    *msgFlgType = *msgFlgType & maskChar(2, 8);

    //setting the value of flag in msgFlgType
    *msgFlgType = *msgFlgType |LshiftChar(flag_char, 1);
}


//
// NAME:          Q931MessageGetLen()
//
// PURPOSE:       Returns the value of length for RsvpCommonHeader
//
// PARAMETERS:    callRefResvlen, The variable containing the value of
//                                callRefResvd and callRefLength
//
// RETURN:        UInt8
//
// ASSUMPTION:    None
//
static UInt8 Q931MessageGetLen(unsigned char callRefResvlen)
{
    UInt8 len = callRefResvlen;

    //clears first 4 bits
    len = len & maskChar(5, 8);

    return len;
}


//
// NAME:          Q931MessageGetCallRefReserved()
//
// PURPOSE:       Returns the value of reserved for Q931Message
//
// PARAMETERS:    callRefResvlen, The variable containing the value of
//                                callRefResvd and callRefLength
//
// RETURN:        unsigned char
//
// ASSUMPTION:    None
//
static unsigned char Q931MessageGetCallRefReserved
(unsigned char callRefResvlen)
{
    unsigned char resv = callRefResvlen;

    //clears last 4 bits
    resv = resv & maskChar(1, 4);

    //setting the value of resv in callRefResvlen
    resv = RshiftChar(resv, 4);

    return (unsigned char)resv;
}


//
// NAME:          Q931MessageGetCallRefFlag()
//
// PURPOSE:       Returns the value of callRefFlag for Q931Message
//
// PARAMETERS:    callRefFlgVal, The variable containing the value of
//                               callRefFlag and callRefValue
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//
static BOOL Q931MessageGetCallRefFlag(unsigned char callRefFlgVal)
{
    unsigned char flag = callRefFlgVal;

    //clears last 7 bits
    flag = flag & maskChar(1, 1);

    //setting the value of flag in callRefFlgVal
    flag = RshiftChar(flag, 1);

    return (BOOL)flag;
}


//
// NAME:          Q931MessageGetCallRefValue()
//
// PURPOSE:       Returns the value of callRefValue for Q931Message
//
// PARAMETERS:    callRefFlgVal, The variable containing the value of
//                               callRefFlag and callRefValue
//
// RETURN:        UInt8
//
// ASSUMPTION:    None
//
static UInt8 Q931MessageGetCallRefValue(unsigned char callRefFlgVal)
{
    UInt8 value = callRefFlgVal;

    //clears the first bit
    value = value & maskChar(2, 8);

    return value;
}


//
// NAME:          Q931MessageGetMsgFlag()
//
// PURPOSE:       Returns the value of messageFlag for Q931Message
//
// PARAMETERS:    msgFlgType, The variable containing the value of
//                              messageFlag and messageType.
//
// RETURN:        BOOL
//
// ASSUMPTION:    None
//
static BOOL Q931MessageGetMsgFlag(unsigned char msgFlgType)
{
    unsigned char flag = msgFlgType;

    //clears last 7 bits
    flag = flag & maskChar(1, 1);

    //setting the value of flag in msgFlgType
    flag = RshiftChar(flag, 1);

    return (BOOL)flag;
}


//
// NAME:          Q931MessageGetMsgType()
//
// PURPOSE:       Returns the value of messageType for Q931Message
//
// PARAMETERS:     msgFlgType, The variable containing the value of
//                              messageFlag and messageType.
//
// RETURN:        UInt8
//
// ASSUMPTION:    None
//
static UInt8 Q931MessageGetMsgType(unsigned char msgFlgType)
{
    UInt8 Type = msgFlgType;

    //clears the first bit
    Type = Type & maskChar(2, 8);

    return Type;
}



// Structure for Alerting.
typedef struct struct_h225_alerting
{
    Q931Message      q931Pdu;
    char             protocolIdentifier[IDENTIFIER_SIZE];
    H323EndpointType destinationInfo;
    clocktype        callIdentifier;
    BOOL             multipleCalls;
    BOOL             maintainConnection;
} H225Alerting;


// Structure for CallProceeding
typedef struct struct_h225_call_proceeding
{
    Q931Message      q931Pdu;
    char             protocolIdentifier[IDENTIFIER_SIZE];
    H323EndpointType destinationInfo;
    clocktype        callIdentifier;
    BOOL             multipleCalls;
    BOOL             maintainConnection;
} H225CallProceeding;


// Structure for Connect
typedef struct struct_h225_connect
{
    Q931Message      q931Pdu;
    char             protocolIdentifier[IDENTIFIER_SIZE];
    H323EndpointType destinationInfo;
    clocktype        conferenceId;
    clocktype        callIdentifier;
    BOOL             multipleCalls;
    BOOL             maintainConnection;
} H225Connect;


// Structure for Setup
typedef struct struct_h225_setup
{
    Q931Message        q931Pdu;
    char               protocolIdentifier[IDENTIFIER_SIZE];

    // Contains an EndpointType to allow the called party to determine
    // whether the call involves a gateway or not.
    H323EndpointType   sourceInfo;

    // Needed to inform the gatekeeper of the destination terminal's call
    // signalling transport address
    BOOL               activeMC;

    clocktype          conferenceId;
    H225ConferenceGoal conferenceGoal;

    H225CallType       callType;
    H225TransportAddress  sourceCallSignalAddr;
    clocktype          callIdentifier;

    BOOL               mediaWaitForConnect;
    BOOL               canOverlapSend;

    clocktype          endpointIdentifier;
    BOOL               multipleCalls;
    BOOL               maintainConnection;
} H225Setup;


// Structure for Setup
typedef struct struct_h225_release_complete
{
    Q931Message q931Pdu;
    char        protocolIdentifier[IDENTIFIER_SIZE];
    clocktype   callIdentifier;

} H225ReleaseComplete;


typedef struct struct_h323_stat
{
    int callInitiated;
    int callReceived;
    int connectionEstablished;
    int callRejected;
    int callForwarded;
} H323Stat;

// terminal and gatekeeper and call signal address
typedef struct struct_h225_terminal_data
{
    H225TransportAddress gatekeeperCallSignalTransAddr;
    H225TransportAddress terminalCallSignalTransAddr;
    char                 terminalAliasAddr[MAX_ALIAS_ADDR_SIZE];
    // Dynamic Address
    NodeId destNodeId; // destination node id for this app session 
    Int16 clientInterfaceIndex; // client interface index for this app 
                                // session
    Int16 destInterfaceIndex; // destination interface index for this app
                              // session
} H225TerminalData;

typedef struct struct_h323_data
{
    H323CallSignalState callSignalState;
    H323HostState       hostState;

    unsigned short      callSignalPort;
    BOOL                gatekeeperAvailable;
    H323EndpointType    endpointType;     // may terminal or gatekeeper
    H323CallModel       callModel;
    clocktype           callIdentifier;

    BOOL                isH323StatEnabled;
    H323Stat            h323Stat;
    int                 connectionId;

    // contains call signal info of terminal
    H225TerminalData    *h225Terminal;

    // contains RAS info
    void                *h225Ras;
    CircularBufferVector *msgBufferVector; //a vector of CircularBuffer
} H323Data;


BOOL H323IsHostReceiver(Node* node);
BOOL H323IsHostInitiator(Node* node);

void H323Init(Node* node,
              const NodeInput *nodeInput,
              BOOL gatekeeperFound,
              H323CallModel callModel,
              unsigned short callSignalPort,
              unsigned short srcPort,
              clocktype waitTime,
              NodeId destId);

BOOL H323GatekeeperInit(Node *node,
                        const NodeInput *nodeInput,
                        H323CallModel callModel);

void H323Layer(Node *node, Message *msg);

void H323InitiateConnection(Node* node, void* voip);
void H323TerminateConnection(Node* node, void* voip);

void H323Finalize(Node *node);

void Q931PduCreate(Q931Message* q931Pdu,
              char callRefFlag,
              unsigned char callRefValue,
              unsigned int msgType);

void H323SendH225SetupMessage(Node* node, int connectionId);
void H323InitiateAlertingMessage(Node* node, int connectionId);

void H323PutClocktype(unsigned char** ptr, clocktype value);
void H323PutInt(unsigned char** ptr, unsigned int value);
void H323PutShort(unsigned char** ptr, unsigned short value);
void H323PutChar(unsigned char** ptr, unsigned char value);
void H323PutString(unsigned char** ptr, const char* string, int maxSize);

void H323PutTransAddr(unsigned char** ptr,
                      const H225TransportAddress* transAddr);

void H323PutTransAddrItemwise(unsigned char** ptr,
                              NodeAddress ipAddr,
                              unsigned short port);

void H323CopyTransAddr(H225TransportAddress* destPtr,
                       const H225TransportAddress* srcPtr);

//---------------------------------------------------------------------------
// NAME:        H323TerminalInit.
// PURPOSE:     listen on H323 port.
// PARAMETERS:
// + node       : Node*     : pointer to the node.
// + serverAddr : Address   : server address
// RETURN:      NONE
//---------------------------------------------------------------------------
void
H323TerminalInit(Node* node, Address serverAddr);



#endif /* H323_H */
