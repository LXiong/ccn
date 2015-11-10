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

#ifndef _VOIP_APP_H_
#define _VOIP_APP_H_

#include "types.h"
#include "stats_app.h"


#define VOIP_MEAN_SPURT_GAP_RATIO  0.4
#define VOIP_PAYLOAD_TYPE  0
#define VOIP_CALL_TERMINATING_DELAY  5 * MILLI_SECOND

#define CALL_TERMINATING_DELAY  500 * MILLI_SECOND

#define VOIP_MAX_DATA_UNIT  160
#define MAX_ALIAS_ADDR_SIZE 32
#define VOIP_MOS_MAX 5.0
#define VOIP_MIN_ONEWAY_DELAY 1 * MINUTE

#define DEFAULT_TOTAL_LOSS_PROBABILITY 5.07

// /**
// CONSTANT    :: VOIP_DEFAULT_CALL_TIMEOUT  :   60
// DESCRIPTION :: Time duration for which the caller waits before
//                getting timed out.
// **/
#define     VOIP_DEFAULT_CALL_TIMEOUT   (60 * SECOND)

// /**
// CONSTANT    :: VOIP_DEFAULT_CONNECTION_DELAY  :   8
// DESCRIPTION :: Duration after which the called party answers
// **/
#define     VOIP_DEFAULT_CONNECTION_DELAY   (8 * SECOND)

#define MINIMUM_TALKING_TIME    (1 * SECOND)

typedef struct
{
    char codecScheme[20];
    unsigned int bitRatePerSecond;
    char packetizationIntervalInMilliSec[6];
} VoipEncodingScheme;

//VOIP Application Parameter
typedef enum
{
    VOIP_CALL_STATUS = 1,
    VOIP_ENCODING_SCHEME = 2,
    VOIP_PACKETIZATION_INTERVAL = 3,
    VOIP_TOS = 4,
    VOIP_DSCP = 5,
    VOIP_PRECEDENCE = 6,
} VoipParameterType;


typedef enum
{
    VOIP_SESSION_NOT_ESTABLISHED,
    VOIP_SESSION_ESTABLISHED,
    VOIP_SESSION_CLOSED
} VoipSessionStatus;

typedef enum
{
    VOIP_INITIATE_CALL,
    VOIP_SEND_DATA,
    VOIP_TERMINATE_CALL
} VoipTimerType;

typedef struct
{
    VoipTimerType type;
    unsigned short initiatorPort;

    NodeAddress srcAddr;
    int remainingPkt;   // used when timer type VOIP_SEND_DATA
    BOOL endSession;    // used when timer type VOIP_SEND_DATA
} VoipTimerData;

typedef struct
{
    unsigned short type;
    unsigned short initiatorPort;
    NodeAddress srcAddr;
    clocktype txTime;

} VoipData;


// Structure containing voip call initiator information.
typedef struct
{
    NodeAddress localAddr;
    NodeAddress remoteAddr;
    unsigned short initiatorPort;
    NodeAddress    callSignalAddr;
    unsigned short callSignalPort;
    NodeAddress nextDeliverAddr;

    int connectionId;
    unsigned int bitRate;
    int proxyConnectionId;

    TosType tos;

    char sourceAliasAddr[MAX_ALIAS_ADDR_SIZE];
    char destAliasAddr[MAX_ALIAS_ADDR_SIZE];
    BOOL initiator;

    clocktype interval;
    clocktype packetizationInterval;
    clocktype sessionInitiate;
    clocktype sessionEstablished;
    clocktype sessionFinish;
    clocktype sessionLastSent;
    clocktype sessionLastReceived;
    clocktype endTime;

    BOOL sessionIsClosed;

    clocktype totalTalkingTime;
    clocktype lastInterArrivalInterval;
    clocktype lastPacketReceptionTime;

    RandomSeed seed;

    clocktype       totalOneWayDelay;
    clocktype       maxOneWayDelay;
    clocktype       minOneWayDelay;
    clocktype       averageOneWayDelay;

    double          scoreMOS;
    double          maxMOS;
    double          minMOS;
    double          averageMOS;

    VoipSessionStatus status;
    void* rtpSessionPtr;

    std::string* applicationName;
    Int32 sessionId;
    Int32 uniqueId;
#ifdef ADDON_DB
    Int32 receiverId;
    Int32 sessionInitiator;
#endif // ADDON_DB
    STAT_AppStatistics* stats;
    // Dynamic Address
    NodeId destNodeId; // destination node id for this app session 
    Int16 clientInterfaceIndex; // client interface index for this app 
                                // session
    Int16 destInterfaceIndex; // destination interface index for this app
                              // session
    NodeId proxyNodeId; // destination node id for this SIP session 
    Int16 proxyInterfaceIndex; // proxy destination interface index for this app
                              // session
} VoipHostData;

typedef struct voip_call_acceptance_list_entry_str
{
    NodeAddress srcAddr;
    unsigned short initiatorPort;
    clocktype interval;
    clocktype startTime;
    clocktype endTime;
    clocktype packetizationInterval;

    struct voip_call_acceptance_list_entry_str* next;
} VoipCallAcceptanceListEntry;

typedef struct
{
    VoipCallAcceptanceListEntry* head;
    VoipCallAcceptanceListEntry* rear;
    int count;
} VoipCallAcceptanceList;


//////////////////////////// Prototype Declaration /////////////////////////

unsigned short VoipCallInitiatorInit(
    Node* node,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    unsigned short callSignalPort,
    unsigned short srcPort,             // initiatorPort
    char* sourceAliasAddr,
    char* destAliasAddr,
    char* appName,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    unsigned tos);

void VoipCallReceiverInit(
    Node* node,
    NodeAddress localAddr,
    NodeAddress remoteAddr,
    unsigned short remotePort,
    char* sourceAliasAddr,
    char* appName,
    unsigned short callSignalPort,
    int connectionId,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    unsigned tos);

VoipHostData* VoipGetInitiatorDataForCallSignal(
    Node* node,
    unsigned short callSignalPort);

VoipHostData* VoipGetInitiatorDataForCallSignal(
    Node* node,
    NodeAddress sourceAddr,
    unsigned short callSignalSourcePort);

VoipHostData* VoipGetReceiverDataForCallSignal(
    Node* node,
    NodeAddress sourceAddr,
    unsigned short callSignalSourcePort);

void VoipCallInitiatorHandleEvent(Node* node, Message* msg);

void VoipCallReceiverHandleEvent(Node* node, Message* msg);

void AppVoipInitiatorFinalize(Node* node, AppInfo* appInfo);

void AppVoipReceiverFinalize(Node* node, AppInfo* appInfo);

void VoipAddNewCallToList(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    clocktype packetizationInterval);

VoipHostData* VoipGetCallInitiatorData(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort);

VoipHostData* VoipGetCallReceiverData(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort);

VoipHostData* VoipGetCallReceiverDataForSessionStatus(Node* node);

VoipHostData* VoipCallInitiatorGetDataForConnectionId(
    Node *node,
    int connectionId);

VoipHostData* VoipCallReceiverGetDataForConnectionId(
    Node *node,
    int connectionId);

VoipHostData* VoipCallInitiatorGetDataForProxyConnectionId(
    Node *node,
    int connectionId);

VoipHostData* VoipCallReceiverGetDataForProxyConnectionId(
    Node *node,
    int connectionId);

VoipHostData* VoipGetReceiverDataForTargetAlias(Node* node, char* target);

void VoipAskForCallAlerting(
    Node* node,
    int connectionId,
    clocktype callProceedingDelay);

void VoipAskForConnectionEstablished(
    Node* node,
    int connectionId);

void VoipStartTransmission(Node* node, VoipHostData* voip);

void VoipReceiveDataFromRtp(
    Node* node,
    Message* msg,
    Address srcAddr,
    int payloadSize,
    BOOL initiator);

//-------------------------------------------------------------------------
// NAME         : VOIPGetMeanOpinionSquareScore()
// PURPOSE      : Get Mean Opinion Square (MOS) Score
// RETURNS      : void
// PARAMETERS   :
//         node : the Node
//        voip  : A Pointer of VoipHostData
// oneWayDelay  : One Way delay
//-------------------------------------------------------------------------
void VOIPGetMeanOpinionSquareScore(Node *node,
                                  VoipHostData* voip,
                                  clocktype oneWayDelay);

void VoipCloseSession(Node* node, VoipHostData* voip);

void VoipConnectionEstablished(Node* node, VoipHostData* voip);

//-----------------------------------------------------------
// NAME         : VoipReadConfiguration()
// PURPOSE      : Read VOIP related data
// ASSUMPTION   : None.
// RETURN VALUE : None.
//-----------------------------------------------------------

void VoipReadConfiguration(Node* srcNode,
                      Node* destNode,
                      const NodeInput *nodeInput,
                      char* codecScheme,
                      clocktype& packetizationInterval,
                      unsigned int& bitRatePerSecond);

//-----------------------------------------------------------
// NAME         : VoipGetEncodingSchemeDetails()
// PURPOSE      : get Encoding Sheme Details
// ASSUMPTION   : None.
// RETURN VALUE : None.
//-----------------------------------------------------------
void VoipGetEncodingSchemeDetails(Node* node,
                           char* codecScheme,
                           clocktype& packetizationInterval,
                           unsigned int& bitRatePerSecond);

//--------------------------------------------------------------------------
// NAME         : VOIPgetOption()
// PURPOSE      : Get VOIP optional Parameter
// PARAMETERS   : token - String Parameter
// RETURN VALUE : Boolean
//--------------------------------------------------------------------------

int VOIPgetOption(char* token);

//--------------------------------------------------------------------------
// NAME         : VOIPsetCallStatus()
// PURPOSE      : set VOIP Call Status whether Accept or Reject
// PARAMETERS   : optionStr - String Parameter
// RETURN VALUE : Boolean
//--------------------------------------------------------------------------

BOOL VOIPsetCallStatus(char* optionStr);

// Each VOIP application uses two ports starting at this base counting
// downward.
#define VOIP_PORT_NUMBER_BASE     65000

void VOIPSendRtpTerminateSession(Node* node, NodeAddress remoteAddr,
                                 unsigned short initiatorPort);

void VOIPSendRtpInitiateNewSession(Node* node,
    NodeAddress localAddr,
    char* srcAliasAddress,
    NodeAddress remoteAddr,
    clocktype packetizationInterval,
    unsigned short localPort,
    BOOL           initiator,
    void*          RTPSendFunc);

//--------------------------------------------------------------------------
// NAME         : VoipUpdateInitiator()
// PURPOSE      : update voip initiator data
// PARAMETERS   : 
// + node       : Node*         : pointer to node
// + voip       : VoipHostData* : voip host data
// + openResult : TransportToAppOpenResult* : open result data
// + isDirect   : bool          : TRUE if direct call FALSE otherwise
// RETURN :: NONE
//--------------------------------------------------------------------------
void
VoipUpdateInitiator(Node *node,
                    VoipHostData* voip,
                    TransportToAppOpenResult* openResult,
                    bool isDirect = TRUE);

//---------------------------------------------------------------------------
// FUNCTION             :AppVoipClientGetSessionAddressState.
// PURPOSE              :get the current address sate of client and server 
// PARAMETERS:
// + node               : Node* : pointer to the node
// + clientPtr          : VoipHostData* : pointer to VOIP client data
// RETRUN:     
// addressState         : AppAddressState :
//                        ADDRESS_FOUND : if both client and server
//                                        are having valid address
//                        ADDRESS_INVALID : if either of them are in 
//                                          invalid address state
//---------------------------------------------------------------------------
AppAddressState
AppVoipClientGetSessionAddressState(Node* node,
                                   VoipHostData* clientPtr);

//---------------------------------------------------------------------------
// FUNCTION             : AppVoipClientAddAddressInformation.
// PURPOSE              : store client interface index, destination 
//                        interface index destination node id to get the 
//                        correct address when application starts
// PARAMETERS:
// + node               : Node*             : pointer to the node
// + clientPtr          : VoipHostData* : pointer to client data
// + remoteAddr         : NodeAddress   : Remote proxy/Destn addr
// + isProxy            : bool : flag to check if proxy d/b needs an update
// RETRUN               : NONE
//---------------------------------------------------------------------------
void
AppVoipClientAddAddressInformation(Node* node,
                                VoipHostData* clientPtr,
                                NodeAddress remoteAddr,
                                bool isProxy = FALSE);

//--------------------------------------------------------------------------
// NAME         : VoipGetInitiatorDataForCallSignal()
// PURPOSE      : search for a voip call initiator data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetInitiatorDataForCallSignal(
                                Node* node,
                                unsigned short callSignalPort);

//--------------------------------------------------------------------------
// NAME         : VoipGetReceiverDataForCallSignal()
// PURPOSE      : search for a voip call receiver data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetReceiverDataForCallSignal(
                                Node* node,
                                unsigned short callSignalSourcePort);

#endif
