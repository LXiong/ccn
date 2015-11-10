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

// /**
// PACKAGE     :: CELLULAR_ABSTRACT
// DESCRIPTION :: Defines the data structures used in all layers of the abstract cellualr model
//                Most of the time the data structure start with Cellular***. For those
//                used for abstract implementation, they are started with CellularAbstract**
// **/

#ifndef _CELLULAR_ABSTRACT_H_
#define _CELLULAR_ABSTRACT_H_
#include "cellular.h"
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_SIGNAL_INDEX : -1
// DESCRIPTION :: Define the invalid sinal measurement index
// **/
#define CELLULAR_ABSTRACT_INVALID_SIGNAL_INDEX  -1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_BS_ID : 0
// DESCRIPTION :: Define the invalid BS ID
// **/
#define CELLULAR_ABSTRACT_INVALID_BS_ID 0

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_SC_ID : 0
// DESCRIPTION :: Define the invalid Switch Center ID
// **/
#define CELLULAR_ABSTRACT_INVALID_SC_ID 0

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_MS_ID : 0
// DESCRIPTION :: Define the invalid MS ID
// **/
#define CELLULAR_ABSTRACT_INVALID_MS_ID 0

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_LAC_ID : -1
// DESCRIPTION :: Define the invalid Location area code
// **/
#define CELLULAR_ABSTRACT_INVALID_LAC_ID -1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_LAC_ID : -1
// DESCRIPTION :: Define the invalid sector id
// **/
#define CELLULAR_ABSTRACT_INVALID_SECTOR_ID -1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_CHANNEL_ID : -1
// DESCRIPTION :: Define the invalid channel id
// **/
#define CELLULAR_ABSTRACT_INVALID_CHANNEL_ID -1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_INVALID_APP_ID : -1
// DESCRIPTION :: Define the invalid application Id
// **/
#define CELLULAR_ABSTRACT_INVALID_APP_ID -1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP : 4
// DESCRIPTION :: Define the maximum channel requirement for an applicaiton
//                It is unused at this time
// **/
#define CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP 4
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE : 6
// DESCRIPTION :: Define the maximum number of BS-SECTOR
//                measurement kept at one MS
// **/
#define CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE 6

// /**
// CONSTANT    :: CELLULAR_TX_INTEGER: 8
// DESCRIPTION :: Defines the CELLULAR Transmission integer
// REFERENCE   :: GSM 04.08
// **/
#define CELLULAR_TX_INTEGER 8

// /**
// CONSTANT    :: CELLULAR_MAX_RETRANS : 16
// DESCRIPTION :: Defines the CELLULAR maximum retransmission value
// REFERENCE   :: GSM 04.08
// **/
#define CELLULAR_MAX_RETRANS 16

// /**
// CONSTANT    :: CELLULAR_RANDACCESS_S : 55
// DESCRIPTION :: Defines the 'S' parameter from Table 3.1
// REFERENCE   :: GSM 04.08
// **/
#define CELLULAR_RANDACCESS_S 55

// /**
// CONSTANT    :: DefaultCellularSlotDuration : (577 * MICRO_SECOND)
// DESCRIPTION :: Specifies the default CellularSlotDuration
// **/
static const clocktype  DefaultCellularSlotDuration = (577 * MICRO_SECOND);

// /**
// ENUM        :: CellularEstCause
// DESCRIPTION :: Establishment cause for a RR connection
// **/
typedef enum
{
    // /**
    // CONSTANT    :: CELLULAR_EST_CAUSE_NORMAL_CALL : 0xe0
    // DESCRIPTION :: Defines CELLULAR SPEECH CALL or data application
    //                channel request cause
    // **/
    // Reference:   GSM 04.08, Section 9.1.8, Table 9.9
    // Comment: Lower 4 bits for rand# should be masked by function creating
    // the channel request msg.
    CELLULAR_EST_CAUSE_NORMAL_CALL = 0xe0,

    // /**
    // CONSTANT    :: CELLULAR_EST_CAUSE_LOCATION_UPDATING : 0x00
    // DESCRIPTION :: Defines CELLULAR location updating cause
    // **/
    CELLULAR_EST_CAUSE_LOCATION_UPDATING = 0x00,

    // /**
    // CONSTANT    :: CELLULAR_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL : 0x40
    // DESCRIPTION :: Defines establishment cause to paging channel request
    // **/
    // Reference:  GSM 04.08, Section 9.1.8, Table 9.9a
    // Comment  :  Lower 4 bits for rand# should be masked by function
    // creating the channel request msg.
    CELLULAR_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL = 0x40,
}CellularEstCause;

// /**
// ENUM        :: CellularAbstractApplicationType
// DESCRIPTION :: Define the types of applications supported by the MS
// **/
typedef enum
{
    CELLULAR_ABSTRACT_VOICE_PHONE = 0,
    CELLULAR_ABSTRACT_VIDEO_PHONE = 1,
    CELLULAR_ABSTRACT_TEXT_MAIL = 2,
    CELLULAR_ABSTRACT_PICTURE_MAIL = 3,
    CELLULAR_ABSTRACT_ANIMATION_MAIL = 4,
    CELLULAR_ABSTRACT_WEB = 5
}CellularAbstractApplicationType;

// /**
// ENUM        :: CellularAbstractServiceType
// DESCRIPTION :: Define the types of service supported by the BS
// **/
typedef enum
{
    CELLULAR_ABSTRACT_VOICE = 0,
    CELLULAR_ABSTRACT_DATA = 1
}CellularAbstractServiceType;

// /**
// ENUM        :: CellularAbstractMessageType
// DESCRIPTION :: Define the types of cellular protocol message
// **/
typedef enum
{
	CELLULAR_ABSTRACT_MESSAGE_TYPE_INVALID = 0,

    // CALL CONTROL
    //+ Call Establishment Messages: 0x0_
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_ALERTING = 0x01,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_CALL_CONFIRMED,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_CALL_PROCEEDING,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_CONNECT,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_SETUP,

    // Call Release Phase Messages: 0x1_

    //+ Call Clearing Messages: 0x2_
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_DISCONNECT,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_RELEASE_COMPLETE,
    CELLULAR_ABSTRACT_CC_MESSAGE_TYPE_RELEASE,

    //message for packet switch mode or session management
    CELLULAR_ABSTRACT_SM_ACTIVATE_PDP_CONTEXT_REQUEST,
    CELLULAR_ABSTRACT_SM_ACTIVATE_PDP_CONTEXT_ACCEPT,
    CELLULAR_ABSTRACT_SM_REQUEST_PDP_CONTEXT_ACTIVATION,
    CELLULAR_ABSTRACT_SM_DEACTIVATE_PDP_CONTEXT_REQUEST,
    CELLULAR_ABSTRACT_SM_DEACTIVATE_PDP_CONTEXT_ACCEPT,

    // MOBILITY MANAGEMENT
    //+ Registration Messages
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_IMSI_DETACH_INDICATION,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_LOCATION_UPDATE_ACCEPT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_LOCATION_UPDATE_REJECT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_LOCATION_UPDATE_REQUEST,

    //+ Msgs between VLR and HLR
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_MAPD_UPDATE_LOCATION,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_MAPD_UPDATE_LOCATION_RESULT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_MAPD_CANCEL_LOCATION,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_MAPD_CANCEL_LOCATION_RESULT,

    // Security Messages
    //TODO

    //+ Connection Management Messages
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_SERVICE_REJECT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_SERVICE_ABORT,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_RE_ESTABLISHMENT_REQUEST,
    CELLULAR_ABSTRACT_MM_MESSAGE_TYPE_CM_ABORT,

    //+ Miscellaneous Messages
    CELLULAR_ABSTRACT_MESSAGE_TYPE_MM_STATUS,

    // Radio Resource management
    //+ Paging messages (0x2-)
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_PAGING_REQUEST_TYPE1,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_PAGING_RESPONSE,

    //+ Channel establishment messages (0x3-)
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_IMMEDIATE_ASSIGNMENT,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_IMMEDIATE_ASSIGNMENT_REJECT,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_CHANNEL_REQUEST,

    //+ System information messages (0x1-)
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_MEASUREMENT_REPORT,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE2,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE3,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE5,

    //+ Handover (0x2-). RI - Radio Interface
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_RI_HANDOVER_COMMAND,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_RI_HANDOVER_ACCESS,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_RI_HANDOVER_COMPLETE,
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_RI_HANDOVER_REQUIRED_REJECT,

    //+ Channel Release (0x0-)
    CELLULAR_ABSTRACT_RR_MESSAGE_TYPE_CHANNEL_RELEASE,

    // the following messages are only transmitted between BS and SC, A interface

    //+ Assignment Messages
    CELLULAR_ABSTRACT_ASSIGNMENT_REQUEST,
    CELLULAR_ABSTRACT_ASSIGNMENT_COMPLETE,
    CELLULAR_ABSTRACT_ASSIGNMENT_FAILURE,

    //+ Handover Messages catergorized in RR
    CELLULAR_ABSTRACT_HANDOVER_REQUEST,
    CELLULAR_ABSTRACT_HANDOVER_REQUIRED,
    CELLULAR_ABSTRACT_HANDOVER_REQUEST_ACK,
    CELLULAR_ABSTRACT_HANDOVER_COMMAND,
    CELLULAR_ABSTRACT_HANDOVER_COMPLETE,
    CELLULAR_ABSTRACT_HANDOVER_FAILURE,
    CELLULAR_ABSTRACT_HANDOVER_PERFORMED,
    CELLULAR_ABSTRACT_HANDOVER_CANDIDATE_ENQUIRE,
    CELLULAR_ABSTRACT_HANDOVER_CANDIDATE_RESPONSE,
    CELLULAR_ABSTRACT_HANDOVER_REQUIRED_REJECT,
    CELLULAR_ABSTRACT_HANDOVER_DETECT,
    CELLULAR_ABSTRACT_HANDOVER_CLEAR_COMMAND,
    CELLULAR_ABSTRACT_HANDOVER_CLEAR_COMPLETE,

    //+ Release Messages (we catergorize them into RR
    CELLULAR_ABSTRACT_CLEAR_COMMAND,
    CELLULAR_ABSTRACT_CLEAR_COMPLETE,
    CELLULAR_ABSTRACT_CLEAR_REQUEST,

    //+ Radio Resource Messages
    CELLULAR_ABSTRACT_PAGING,
    CELLULAR_ABSTRACT_PAGING_RESPONSE,

    CELLULAR_ABSTRACT_COMPLETE_LAYER3_INFORMATION,

    //message when gateway invlove
    CELLULAR_ABSTRACT_CC_CALL_SETUP_INDICATION,
    CELLULAR_ABSTRACT_CC_CALL_CONNECT_INDICATION,
    CELLULAR_ABSTRACT_CC_CALL_DISCONNECT_INDICATION,
    CELLULAR_ABSTRACT_CC_CALL_ALERTING_INDICATION,
    CELLULAR_ABSTRACT_CC_CALL_REJECT_INDICATION,
    CELLULAR_ABSTRACT_SM_PDP_CONTEXT_INDICATION,

    CELLULAR_ABSTRACT_LAST_PACKET_OF_SESSION,
    CELLULAR_ABSTRACT_FIRST_PACKET_OF_SESSION,
} CellularAbstractMessageType;

// /**
// ENUM        :: CellularAbstractChannelRequestType
// DESCRIPTION :: Define the types of channel requested by the transaction
//                Only CELLULAR_ABSTRACT_CHANNEL_REQUEST_GENERAL is used
//                CELLULAR_ABSTRACT_CHANNEL_REQUEST_VOICE and
//                CELLULAR_ABSTRACT_CHANNEL_REQUEST_DATA
//                would be used when channels are reserved for
//                voice/data exclusively
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CHANNEL_REQUEST_GENERAL = 1,
    CELLULAR_ABSTRACT_CHANNEL_REQUEST_VOICE = 2,
    CELLULAR_ABSTRACT_CHANNEL_REQUEST_DATA = 3
}CellularAbstractChannelRequestType;

// /**
// ENUM        :: CellulaAbstractNwCcState
// DESCRIPTION :: Define the call control state at the network side
// **/
typedef enum
{
    CELLULAR_ABSTRACT_NW_CC_STATE_NULL = 0, // N0
    CELLULAR_ABSTRACT_NW_CC_MM_CONNECTION_PENDING = 1, // N0.1 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_CALL_INITIATED = 2, // N1 - MO
    CELLULAR_ABSTRACT_NW_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING = 3, // N3 - MO
    CELLULAR_ABSTRACT_NW_CC_STATE_CALL_DELIVERED = 4, // N4 - MO
    CELLULAR_ABSTRACT_NW_CC_STATE_CALL_PRESENT = 6, // N6 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_CALL_RECEIVED = 7, // N7 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_CONNECT_REQUEST = 8, // N8 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_MOBILE_TERM_CALL_CONFIRMED = 9, // N9 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_ACTIVE = 10, // N10 - MO & MT
    CELLULAR_ABSTRACT_NW_CC_STATE_DISCONNECT_REQUEST = 11,
    CELLULAR_ABSTRACT_NW_CC_STATE_DISCONNECT_INDICATION = 12, // N12 - From Nw
    CELLULAR_ABSTRACT_NW_CC_STATE_RELEASE_REQUEST = 19, // N19 - By MS
    CELLULAR_ABSTRACT_NW_CC_STATE_MOBILE_ORIG_MODIFY = 26, // N26 - MO
    CELLULAR_ABSTRACT_NW_CC_STATE_MOBILE_TERM_MODIFY = 27, // N27 - MT
    CELLULAR_ABSTRACT_NW_CC_STATE_CONNECT_INDICATION = 28 // N28 - MO
} CellularAbstractNwCcState;

// /**
// ENUM        :: CellulaAbstractMsCcState
// DESCRIPTION :: Define the call control state at the MS side
// **/
typedef enum
{
     CELLULAR_ABSTRACT_MS_CC_STATE_NULL = 0, // U0
     CELLULAR_ABSTRACT_MS_CC_STATE_MM_CONNECTION_PENDING = 1, // U0.1 MO
     CELLULAR_ABSTRACT_MS_CC_STATE_CALL_INITIATED = 2, // U1   MO
     CELLULAR_ABSTRACT_MS_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING = 3, // U3 MO
     CELLULAR_ABSTRACT_MS_CC_STATE_CALL_DELIVERED = 4, // U4   MO
     CELLULAR_ABSTRACT_MS_CC_STATE_CALL_PRESENT = 6, // U6   MT
     CELLULAR_ABSTRACT_MS_CC_STATE_CALL_RECEIVED = 7, // U7   MT
     CELLULAR_ABSTRACT_MS_CC_STATE_CONNECT_REQUEST  = 8, // U8   MT
     CELLULAR_ABSTRACT_MS_CC_STATE_MOBILE_TERM_CALL_CONFIRMED = 9, // U9 MT
     CELLULAR_ABSTRACT_MS_CC_STATE_ACTIVE = 10, // U10  Both
     CELLULAR_ABSTRACT_MS_CC_STATE_DISCONNECT_REQUEST = 11, // U11  Both
     CELLULAR_ABSTRACT_MS_CC_STATE_DISCONNECT_INDICATION = 12, // U12 From Nw
     CELLULAR_ABSTRACT_MS_CC_STATE_RELEASE_REQUEST = 13, // U19  MO
     CELLULAR_ABSTRACT_MS_CC_STATE_MOBILE_ORIG_MODIFY = 14, // U26
     CELLULAR_ABSTRACT_MS_CC_STATE_MOBILE_TERM_MODIFY = 15 // U27
} CellulaAbstractMsCcState;

// /**
// ENUM        :: CellularAbstractInterlayerInfoType
// DESCRIPTION :: Define the type of info delivered between MAC and LAYER3
//                only CELLULAR_ABSTRACT_CONTROL_PROTOCOL_PACKET is used
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CONTROL_PROTOCOL_PACKET = 1,
    CELLULAR_ABSTRACT_TRAFFIC_PROTOCOL_PACKET = 2
}CellularAbstractInterlayerInfoType;

// /**
// ENUM        :: CellularAbstractCallSrcDestType
// DESCRIPTION :: Define the type of applicaitons' src and dest
// **/
typedef enum
{
    CELLULAR_ABSTRACT_MOBILE_ORIGINATING_CALL = 1,
    CELLULAR_ABSTRACT_MOBILE_TERMINATING_CALL = 2,
    CELLULAR_ABSTRACT_FIXED_ORIGINATING_CALL = 3,
    CELLULAR_ABSTRACT_FIXED_TERMINATING_CALL = 4,
}CellularAbstractCallSrcDestType;

// /**
// ENUM        :: CellularAbstractCallIndicationType
// DESCRIPTION :: Define the types of indication transmitted
//                between SC and GATEWAY
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_SETUP_INDICATION = 1,
    CELLULAR_ABSTRACT_CALL_ALERTING_INDICATION,
    CELLULAR_ABSTRACT_CALL_CONNECT_INDICATION,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_INDICATION,
    CELLULAR_ABSTRACT_PDP_CONTEXT_INDICATION,
}CellularAbstractCallIndicationType;

// /**
// ENUM        :: CellularAbstractReceiverSet
// DESCRIPTION :: Define the receiver set
//                CellularAbstractReceiverSet only used for
//                implementing the MAC
// **/
typedef enum
{
    CELLULAR_ABSTRACT_RECEIVER_ALL_BS_MS = 0xff,
    CELLULAR_ABSTRACT_RECEIVER_ALL_MS = 0xfe,
    CELLULAR_ABSTRACT_RECEIVER_ALL_BS = 0xfd,
    CELLULAR_ABSTRACT_RECEIVER_SINGLE = 0xfc
}CellularAbstractReceiverSet;

// /**
// ENUM        :: CellularAbstractChannelType
// DESCRIPTION :: Define the types of logical channel,
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH = 1,
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_RACCH,

    // AGCH + PCH togather are called as CCCH
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_PAGCH,
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_SACCH,
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_FACCH,

    // SDCCH + SACCH + FACCH are togather called as DCCH
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_SDCCH,
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_TCH, // Full-rate
    CELLULAR_ABSTRACT_CONTROL_CHANNEL_WIRED,
}CellularAbstractChannelType;

// /**
// ENUM        :: CellularAbstractCallRejectCauseType
// DESCRIPTION :: Define the casue of call rejection
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_SYSTEM_BUSY =1,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_BUSY,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_UNREACHABLE,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_POWEROFF,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_NETWORK_NOT_FOUND,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_UNSUPPORTED_SERVICE,
    CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_UNKNOWN_USER,
}CellularAbstractCallRejectCauseType;

// /**
// ENUM        :: CellularAbstractCallDropCauseType
// DESCRIPTION :: Define the casue of call rejection
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_SYSTEM_CALL_ADMISSION_CONTROL =1,
    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_SYSTEM_CONGESTION_CONTROL,
    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_HANDOVER_FAILURE,
    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_SELF_POWEROFF,
    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF,
}CellularAbstractCallDropCauseType;

// /**
// ENUM        :: CellularAbstractCallDisconectCauseType
// DESCRIPTION :: Define the casue of call rejection
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_DISCONNECT_NORMAL =1,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_TIMEOUT,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_MTReject,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_DROP_HANDOVER,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_DROP_POWEROFF,
}CellularAbstractCallDisconectCauseType;

// /**
// ENUM        :: CellularAbstractMsActiveStatus
// DESCRIPTION :: Definition of the MS's status: active or inactive
// **/
typedef enum
{
    CELLULAR_ABSTRACT_MS_INACTIVE = 0,
    CELLULAR_ABSTRACT_MS_ACTIVE = 1,
}CellularAbstractMsActiveStatus;

// /**
// ENUM        :: CellularAbstractCallAdmissionControlPolicy
// DESCRIPTION :: Definition of the call admission control
//                policy supported in the system
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_ADMISSION_CONTROL_NONE = 0,
    CELLULAR_ABSTRACT_CALL_ADMISSION_CONTROL_THRESHOLD_BASED = 1,
}CellularAbstractCallAdmissionControlPolicy;

// /**
// ENUM        :: CellularAbstractCallCongestionControlPolicy
// DESCRIPTION :: Definition of the congestion control
//                policy supported in the system
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CONGESTION_CONTROL_NONE = 0,
    CELLULAR_ABSTRACT_CONGESTION_CONTROL_ROUND_ROBIN = 1,
    CELLULAR_ABSTRACT_CONGESTION_CONTROL_PROBABILISTIC_POLICY = 2,
}CellularAbstractCallCongestionControlPolicy;

//info structure for the message between APP and Network layers
// /**
// STRUCT      :: CellularAbstractCallStartMessageInfo
// DESCRIPTION :: Definition of Info field structure for
//                the MSG_NETWORK_CELLULAR_FromAppStartCall
// **/
typedef struct struct_cellular_abstract_call_start_message_info
{

    CellularAbstractApplicationType appType;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    short appNumChannelReq;
    double appBandwidthReq;
    clocktype appDuration; // only used for packet switch mode
}CellularAbstractCallStartMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallEndMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_NETWORK_CELLULAR_FromAppEndCall
// **/
typedef CellularAbstractCallStartMessageInfo
            CellularAbstractCallEndMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallRejectedInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallRejected
// **/
typedef CellularAbstractCallStartMessageInfo
            CellularAbstractCallRejectedInfo;

// /**
// STRUCT      :: CellularAbstractCallArriveMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallArrive
// **/
typedef struct struct_cellular_abstract_call_arrive_message_info
{
    CellularAbstractApplicationType appType;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    short appNumChannelReq;
    double appBandwidthReq;
    int transactionId;
}CellularAbstractCallArriveMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallAcceptMessageInfo;
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallAccepted
// **/
typedef CellularAbstractCallArriveMessageInfo
            CellularAbstractCallAcceptMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallAnsweredMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_NETWORK_CELLULAR_FromAppCallAnswered
// **/
typedef CellularAbstractCallArriveMessageInfo
            CellularAbstractCallAnsweredMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallEndByRemoteMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallEndByRemote
// **/
typedef CellularAbstractCallArriveMessageInfo
            CellularAbstractCallEndByRemoteMessageInfo;

// /**
// STRUCT      :: CCellularAbstractCallRejectMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallRejected
// **/
typedef struct struct_cellular_abstract_call_reject_message_info
{
    CellularAbstractApplicationType appType;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    short appNumChannelReq;
    double appBandwidthReq;
    int transactionId;
    CellularAbstractCallRejectCauseType rejectCause;
}CellularAbstractCallRejectMessageInfo;

// /**
// STRUCT      :: CellularAbstractCallDropMessageInfo
// DESCRIPTION :: Definition of Info field structure
//                for the MSG_APP_CELLULAR_FromNetworkCallDropped
// **/
typedef struct struct_cellular_abstract_call_dropped_message_info
{
    CellularAbstractApplicationType appType;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    short appNumChannelReq;
    double appBandwidthReq;
    int transactionId;
    CellularAbstractCallDropCauseType dropCause;
}CellularAbstractCallDropMessageInfo;

// /**
// STRUCT      :: CellularGenericLayer3MsgHdr
// DESCRIPTION :: Definition the structure of layer 3 header
//                for the message(actually protocol-related packet)
//                between Network and MAC layers
//                CellularGenericLayer3MsgHdr is added to
//                each message generated at the layers to help the
//                receiver properly handle the message.
// **/
typedef struct struct_cellular_generic_layer3_message_header_str
{
    CellularProtocolDiscriminator protocolDiscriminator;
    CellularAbstractMessageType messageType;
}CellularGenericLayer3MsgHdr;

// /**
// STRUCT      :: CellularAbstractBsSystemInfo
// DESCRIPTION :: Definition the structure of system information of BS
// **/
typedef struct struct_cellular_abstract_bs_system_information_str
{
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddress;
    int lac;
    int cellId;
    int txInteger;
    int maxReTrans;

    // uplink control channel,Values received from BS via beacon msgs
    int controlDLChannelIndex;

    // downlink control channel
    int controlULChannelIndex;

    // congestion control
    CellularAbstractCallCongestionControlPolicy congestionControlPolicy;

    int numSector;    // sector

    //params for round-robin congestion ocntrol or probabilistic
    int numAccessClass;

    int padding;

    double refrainProb;

    clocktype oneControlDuration;

    // neiboring cell's downlink channel index
    // service support
    // and others

    //Coordinates bsLocation;
    double bsLocationC1;
    double bsLocationC2;
    double bsLocationC3;
}CellularAbstractBsSystemInfo;

// /**
// STRUCT      :: CellularAbstractMeasurementReportMsgInfo
// DESCRIPTION :: Definition the structure of measurement report
//                generated at MAC layer
// **/
typedef struct struct_cellular_abstract_measurement_report_message_info_str
{
    NodeAddress monitoredBsId;
    int   monitoredSectorId;
    double receivedSignalStrength;
}CellularAbstractMeasurementReportMsgInfo;

// /**
// STRUCT      :: CellularAbstractSelectCellInfo
// DESCRIPTION :: Definition the structure of cell and sector
//                selection generated at layer 3
// **/
typedef struct struct_cellular_abstract_select_cell_info_str
{
    int lac;
    int cellId;
    int sectorId;
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddress;

    // uplink control channel,Values received from BS via beacon msgs
    int controlDLChannelIndex;

    //downlink control channel
    int controlULChannelIndex;
}CellularAbstractSelectCellInfo;

// /**
// STRUCT      :: CellularAbstractNetowrkToMacMsgInfo
// DESCRIPTION :: Definition the structure of info field
//                appended to each msg transmitted over radio interface
// **/
typedef struct struct_cellular_abstract_network_to_mac_message_info_str
{
    CellularAbstractInterlayerInfoType infoType;
    int channelIndex;
    CellularAbstractChannelType channelType;

    //int interfaceIndex;

    //the following two only use for the purpose of
    //implmentation the abstract MAC
    CellularAbstractReceiverSet recvSet;
    NodeAddress recvId; // for broadcast,using -1
}CellularAbstractNetowrkToMacMsgInfo;
#endif /* _CELLULAR_ABSTARCT_H_ */
