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


#ifndef H225_RAS_H
#define H225_RAS_H

//224.0.1.41
#define GATEKEEPER_DISCOVERY_MULTICAST_ADDRESS 0xE0000129

#define MULTICAST_GROUP_JOINING_TIME  1 * SECOND

#define GATEKEEPER_REQUEST_TIMER    (5 * SECOND)

#define GRQ_MAX_RETRY_COUNT     2

#define REGISTRATION_REQUEST_TIMER    (8 * SECOND)

#define RRQ_MAX_RETRY_COUNT     5

#define MAX_CALL_REF_VALUE     127

#define SIZE_OF_H225_RAS_GRQ    32

#define SIZE_OF_H225_RAS_GCF    28

#define SIZE_OF_H225_RAS_RRQ    86

#define SIZE_OF_H225_RAS_RCF    44

#define SIZE_OF_H225_RAS_ARQ    83

#define SIZE_OF_H225_RAS_ACF    36

#define H225_RAS_GRQ_PERIODIC_REFRESH (60 * SECOND)

#define SIZE_OF_H225_RAS_LRQ    50

#define SIZE_OF_H225_RAS_LCF    20

#define SIZE_OF_TRANS_ADDR      6

typedef enum
{
    GatekeeperRequest = 21,
    GatekeeperConfirm,
    GatekeeperReject,
    RegistrationRequest,
    RegistrationConfirm,
    RegistrationReject,
    UnregistrationRequest,
    UnregistrationConfirm,
    UnregistrationReject,
    AdmissionRequest,
    AdmissionConfirm,  // 31
    AdmissionReject,
    BandwidthRequest,
    BandwidthConfirm,
    BandwidthReject,
    DisengageRequest,
    DisengageConfirm,
    DisengageReject,
    LocationRequest,
    LocationConfirm,
    LocationReject,  // 41
    InfoRequest,
    InfoRequestResponse,
    NonStandardMessage,
    UnknownMessageResponse,
    RequestInProgress,
    ResourcesAvailableIndicate,
    ResourcesAvailableConfirm,
    InfoRequestAck,
    InfoRequestNak,
    ServiceControlIndication,
    ServiceControlResponse
} H225RasMessage;


typedef enum
{
    PREVIOUS,
    CURRENT
} H225RasSearchType;

typedef enum
{
    INGRES_TO_EGRES,
    EGRES_TO_INGRES
} H225ConectionIdType;

typedef struct struct_h225_source_dest_trans_addr_info
{
    H225TransportAddress        sourceTransAddr;
    H225TransportAddress        destTransAddr;
} H225SourceDestTransAddrInfo;


typedef struct struct_h225_gatekeeper_request
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
    H225TransportAddress        rasAddress;
    H323EndpointType            endpointType;
} H225GatekeeperRequest;


typedef struct struct_h225_gatekeeper_confirm
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
    H225TransportAddress        rasAddress;
} H225GatekeeperConfirm;


typedef struct struct_h225_gatekeeper_reject
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
//  H225GatekeeperRejectReason  rejectReason;
} H225GatekeeperReject;


typedef struct struct_h225_registration_request
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
    BOOL                        discoveryComplete;
    H225TransportAddress        callSignalAddress;
    H225TransportAddress        rasAddress;
    H323EndpointType            endpointType;
    char                        terminalAlias[MAX_ALIAS_ADDR_SIZE];
    BOOL                        keepAlive;
    BOOL                        willSupplyUUIEs;
    BOOL                        maintainConnection;
} H225RegistrationRequest;


typedef struct struct_h225_registration_confirm
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
    H225TransportAddress        callSignalAddress;
    H323EndpointType            endpointType;
    BOOL                        willRespondToIRR;
    BOOL                        preGrantedARQ;
    BOOL                        maintainConnection;
} H225RegistrationConfirm;


typedef struct struct_h225_registration_Reject
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    char                        protocolIdentifier[IDENTIFIER_SIZE];
//    rejectReason              RegistrationRejectReason,
} H225RegistrationReject;


typedef struct struct_h225_location_request
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    H323EndpointType            endpointType;
    //alias address of destination
    char                        destinationInfo[MAX_ALIAS_ADDR_SIZE];
    H225TransportAddress        replyAddress;
} H225LocationRequest;


typedef struct struct_h225_location_confirm
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    H225TransportAddress        callSignalAddress;
    H225TransportAddress        rasAddress;
} H225LocationConfirm;


typedef struct struct_h225_location_reject
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
//    rejectReason              locationRejectReason,
} H225LocationReject;


typedef struct struct_h225_admission_request
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    H225CallType                callType;
    H323EndpointType            endpointIdentifier;
    char                        aliasAddress[MAX_ALIAS_ADDR_SIZE];
    H225TransportAddress        sourceTransAddr;
    unsigned int                bandWidth;
    unsigned char               callReferenceValue;
    unsigned int                conferenceID;
    BOOL                        activeMC;
    BOOL                        answerCall;
    BOOL                        canMapAlias;
    unsigned int                callIdentifier;
    BOOL                        willSupplyUUIEs;
} H225AdmissionRequest;


typedef struct struct_h225_admission_confirm
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
    unsigned int                bandWidth;
    H323CallModel               callModel;
    H225TransportAddress        destCallSignalAddress;
    BOOL                        willRespondToIRR;
    BOOL                        uuiesRequested;
} H225AdmissionConfirm;


typedef struct struct_h225_admission_reject
{
    Q931Message                 q931Pdu;
    unsigned int                requestSeqNum;
} H225AdmissionReject;


typedef struct struct_h225_ras_stat
{
    int gatekeeperRequested;
    int gatekeeperConfirmed;
    int gatekeeperRejected;

    int registrationRequested;
    int registrationConfirmed;
    int registrationRejected;

    int admissionRequested;
    int admissionConfirmed;
    int admissionRejected;

    int locationRequested;
    int locationConfirmed;
    int locationRejected;
} H225RasStat;


typedef struct struct_gate_keeper_ras_process
{
    H225TransportAddress terminalRasTransAddr;
    unsigned short       rasAssignedPort;
} H225RasProcessEntry;


// gatekeeper information structure
typedef struct struct_gate_keeper_ras_registration
{
    H225TransportAddress gatekeeperRasTransAddr;
    H225TransportAddress gatekeeperCallSignalTransAddr;

    H225TransportAddress terminalRasTransAddr;
    H225TransportAddress terminalCallSignalTransAddr;

    char                 terminalAliasAddr[MAX_ALIAS_ADDR_SIZE];
} H225RasGateKeeperRegnItem;

typedef struct struct_call_entry
{
    unsigned int         requestSeqNum;
    H225TransportAddress initiatorCallSignalTransAddr;
    H225TransportAddress localCallSignalTransAddr;
    H225TransportAddress receiverCallSignalTransAddr;
    H225TransportAddress prevCallSignalTransAddr;
    NodeAddress          nextCallSignalDeliverAddr;
    int                  ingresConnectionId;
    int                  egresConnectionId;
    BOOL                 isMarkedForDel;
} H225RasCallEntry;

typedef struct struct_h225_ras_gatekeeper_data
{
    LinkedList*               rasProcessList;
    // H225RasCallEntry
    LinkedList*               callEntryList;

    // gatekeeper structure H225RasGateKeeperRegnItem
    LinkedList*               gateKeeperInfoList;
} H225RasGatekeeperData;


typedef struct struct_h225_ras_terminal_data
{
    unsigned short       GRQRetryCount;
    BOOL                 h225RasGotGateKeeperResponse;
    BOOL                 isAlreadyRegistered;
    unsigned short       RRQRetryCount;
    BOOL                 h225RasGotRegistrationResponse;
    H225TransportAddress gatekeeperRasTransAddr;
    H225TransportAddress terminalRasTransAddr;
    BOOL                 gateKeeperUnicastAddAvailable;
    clocktype            minStartTime;
    // Dynamic Address
    NodeId destNodeId; // destination node id for this app session 
    Int16 clientInterfaceIndex; // client interface index for this app 
                                // session
    Int16 destInterfaceIndex; // destination interface index for this app
                              // session

    std::string*          gateKeeperURL;

} H225RasTerminalData;

// Structure to store timer infomation for H323
struct H323TimerData
{
    H225TransportAddress sourceTransAddr;
    H225TransportAddress destTransAddr;
};

typedef struct struct_h225_ras_data
{
    void                 *h225RasCommon;   // either it is GatekeeperData
                                              // or TerminalData
    unsigned int         requestSeqNum;
    H225RasStat          *h225RasStat;
} H225RasData;

//////////////////////////// Prototype Declaration ///////////////////////

void H225RasAdmissionRequest(Node* node,
                             VoipHostData* voip);

void H225RasCreateAndSendARQForReceiver(Node* node, VoipHostData* voip);

BOOL H323ReadEndpointList(PartitionData * partitionData,
                          NodeAddress thisNodeId,
                          const NodeInput* nodeInput,
                          char* terminalAliasAddr);

void H225RasGatekeeperInit(Node* node, H323Data* h323);

void H225RasTerminalInit(Node *node,
                         const NodeInput *nodeInput,
                         H323Data* h323,
                         Address homeUnicastAdr,
                         unsigned short initiatorPort);

void H225RasLayer(Node *node, Message *msg);

void H225RasFinalize(Node *node);

int H225RasSearchCallListForConnectionId(
    H225RasData* h225Ras,
    int connectionId,
    H225ConectionIdType connectionIdType);

H225RasCallEntry* H225RasSearchCallList(
    H225RasData* h225Ras,
    NodeAddress sourceAddr,
    unsigned short sourcePort,
    H225RasSearchType searchType);

int
H225RasSearchAndMarkCallEntry(
    H225RasData* h225Ras,
    int connectionId,
    H225ConectionIdType connectionIdType);

int
H225RasSearchAndRemoveCallEntry(
    Node* node,
    H225RasData* h225Ras,
    int connectionid);

void H225RasInsertAcceptedCall(
    Node* node,
    NodeAddress initiatorAddr,
    unsigned short initiatorPort,
    NodeAddress receiverAddr);

void H225RasSearchAcceptedCall(
    Node* node,
    NodeAddress receiverAddr,
    unsigned short receiverPort,
    H225TransportAddress* initiatorTransAddr);

//---------------------------------------------------------------------------
// NAME:        H225RasHandleChangeAddressEvent.
// PURPOSE:     DHCP handler function to handle address change
// PARAMETERS:
// + node           : Node*     : Node which got the DHCP notification.
// + interfaceIndex : Int32     : interfaceIndex which has changed its 
//                                address
// + oldAddress     : Address*    : old IP address
// + subnetMask     : NodeAddress : subnet mask of the node's network
// + networkType    : NetworkType : network type of the topology in which
//                                  node is present
// RETURN : None.
//---------------------------------------------------------------------------
void
H225RasHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);

//---------------------------------------------------------------------------
// FUNCTION             :H323ClientGetSessionAddressState.
// PURPOSE              :get the current address sate of client and server 
// PARAMETERS:
// + node               : Node* : pointer to the node
// + clientPtr          : H225RasTerminalData* : pointer to H323 client data
// RETRUN:     
// addressState         : AppAddressState :
//                        ADDRESS_FOUND : if both client and server
//                                        are having valid address
//                        ADDRESS_INVALID : if either of them are in 
//                                          invalid address state
//---------------------------------------------------------------------------
AppAddressState
H323ClientGetSessionAddressState(Node* node,
                                 H225RasTerminalData* clientPtr);

//-------------------------------------------------------------------------
// NAME:        H323IsDHCPClient()
// PURPOSE:     find whether the node is DHCP client at the interface index
// PARAMETERS:  
// + node       : Node*         : Node which is to be checked for DHCP 
//                                client enabled
// + nodeInput  : NodeInput*    : The input file
// RETURN::
// BOOL         : TRUE if DHCP client enabled FALSE otherwise
//-------------------------------------------------------------------------
BOOL
H323IsDHCPClient(Node* node, const NodeInput* nodeInput);

//---------------------------------------------------------------------------
// NAME:        H323IsDHCPEnabled()
// PURPOSE:     find whether the node is DHCP enabled at the interface index
// PARAMETERS:  
// + node       : Node*         : Node which is to be checked for DHCP 
//                                enabled
// + nodeInput  : NodeInput*    : The input file
// RETURN::
// BOOL         : TRUE if DHCP enabled FALSE otherwise
//---------------------------------------------------------------------------
BOOL 
H323IsDHCPEnabled(Node* node, const NodeInput* nodeInput);

//--------------------------------------------------------------------------
// FUNCTION    :: H323UrlSessionStartCallBack
// PURPOSE     :: To update the client when URL is resolved for TCP based
//                H323.This is a dummy function for H323 as we do not need to
//                update any data structure after the URL is resolved in this
//                case. This function has been kept only for consistency.
// PARAMETERS   ::
// + node       : Node*          : pointer to the node
// + serverAddr : Address*       : server address
// + sourcePort : UInt16         : source port
// + uniqueId   : Int32          : connection id
// +interfaceId : Int16          : interface index,
// +serverUrl   : std::string    : server URL
// +packetSendingInfo : AppUdpPacketSendingInfo* : information required to 
//                                                send buffered application 
//                                                packets in case of UDP 
//                                                based applications; not
//                                                used by H323
// RETURN       :
// bool         : TRUE if application packet can be sent; FALSE otherwise
//                TCP based application will always return FALSE as 
//                this callback will initiate TCP Open request and not send 
//                packet
//---------------------------------------------------------------------------
bool
H323UrlSessionStartCallBack(
                     Node* node,
                     Address* serverAddr,
                     UInt16 sourcePort,
                     Int32 uniqueId,
                     Int16 interfaceId,
                     std::string serverUrl,
                     AppUdpPacketSendingInfo* packetSendingInfo);
//---------------------------------------------------------------------------
// FUNCTION             : H323TerminalAddAddressInformation.
// PURPOSE              : store client interface index, destination 
//                        interface index destination node id to get the 
//                        correct address when application starts
// PARAMETERS:
// + node               : Node*             : pointer to the node
// + h225TerminalData   : H225RasTerminalData* : pointer to client data
// + rmtAddr            : NodeAddress       : Remote Addr
// RETRUN               : NONE
//---------------------------------------------------------------------------
void
H323TerminalAddAddressInformation(Node* node,
                                H225RasTerminalData* h225TerminalData,
                                NodeAddress rmtAddr);

#endif // H225_RAS_H
