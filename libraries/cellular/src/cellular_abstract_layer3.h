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
// PACKAGE     :: CELLULAR_ABSTRACT_LAYER3
// DESCRIPTION :: Defines the data structures and API functions
//                defined in the Abstract Cellular Implementation
//                Most of the time the data structure
//                starting with CellularAbstract**
// **/

#ifndef _CELLULAR_ABSTRACT_LAYER3_H_
#define _CELLULAR_ABSTRACT_LAYER3_H_

#include "coordinates.h"
#include "cellular.h"
#include "cellular_abstract.h"
#include "mac_cellular_abstract.h"

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_BANDWIDTH_UNIT : 16 (Kbps)
// DESCRIPTION :: The basic unit used to count the bandwidth resources
// **/
#define CELLULAR_ABSTRACT_BANDWIDTH_UNIT 16 // in Kbps

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_BANDWIDTH_FOR_LOCATION_UPDATE : 16 (Kbps)
// DESCRIPTION :: The basic unit used to count the bandwidth resources
// **/
#define CELLULAR_ABSTRACT_BANDWIDTH_FOR_LOCATION_UPDATE 16 // in Kbps


// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_MS : 10
// DESCRIPTION :: Maximuma number of ctive applications
//                a MS can have concurrently
// **/
#define CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_MS 50

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_CHANNEL_PER_SECTOR : 10
// DESCRIPTION :: Maximum number of channels can be allocated to each sectors
//                It is not used currently
// **/
#define CELLULAR_ABSTRACT_MAX_CHANNEL_PER_SECTOR 10

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SECTOR_PER_BS : 6
// DESCRIPTION :: Maximum number of channels can be allocated to each sectors
//                It is not used currently
// **/
#define CELLULAR_ABSTRACT_MAX_SECTOR_PER_BS 6

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SERVICE_TYPE_PER_BS : 10
// DESCRIPTION :: Max # of types of services supported by BS ( data/voice)
// **/
#define CELLULAR_ABSTRACT_MAX_SERVICE_TYPE_PER_BS 10 // 2 types defined

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_CHANNEL_PER_BS : 60
// DESCRIPTION :: Maximum number of channels allocated to each  BS
// **/
#define CELLULAR_ABSTRACT_MAX_CHANNEL_PER_BS 60

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS : 1000
// DESCRIPTION :: Maximum number of active applications at each  BS
// **/
#define CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS 1000

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_AGGREGATEDNODE : 10000
// DESCRIPTION :: Maximum number of active applications at aggregated node
// **/
#define CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_AGGREGATEDNODE 10000

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_BS_PER_SC : 20
// DESCRIPTION :: Maximum number of bs controlled by each switch center
// **/
#define CELLULAR_ABSTRACT_MAX_BS_PER_SC 40

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_VIRTUAL_CIRCUIT_PER_SC : 10000
// DESCRIPTION :: Maximum number of circuit allocated to each switch center
// **/
#define CELLULAR_ABSTRACT_MAX_VIRTUAL_CIRCUIT_PER_SC 10

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_SC : 1000
// DESCRIPTION :: Maximum number of active applications at each switch center
//                CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS
//                    *CELLULAR_ABSTRACT_MAX_BS_PER_SC
// **/
#define CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_SC 10000

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_MS_PER_SC : 10000
// DESCRIPTION :: Maximum number of MS under controlled by each switch center
// **/
#define CELLULAR_ABSTRACT_MAX_MS_PER_SC 10000

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_LAC_PER_SC : 20
// DESCRIPTION :: Maximum number of Location area code
//                under controlled by each switch center
// **/
#define CELLULAR_ABSTRACT_MAX_LAC_PER_SC 40

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SC_PER_GATEWAY : 6
// DESCRIPTION :: Maximum number of switch center connected with gateway
// **/
#define CELLULAR_ABSTRACT_MAX_SC_PER_GATEWAY 6 //assume a lager value

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_MS_PER_HLR : 60000
// DESCRIPTION :: Maximum number of record can be kept at HLR
// **/
#define CELLULAR_ABSTRACT_MAX_MS_PER_HLR 10000

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_USER_DEFAULT_AGE : 20
// DESCRIPTION :: Default user age
//                Currently it is not used
// **/
#define CELLULAR_ABSTRACT_USER_DEFAULT_AGE 20
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_CHANNEL_BANDWIDTH : 1000
// DESCRIPTION :: Default channel bandwidth
//                Currently it is not used
// **/
#define CELLULAR_ABSTRACT_DEFAULT_CHANNEL_BANDWIDTH 1000
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_CHANNEL_FREQUENCY : 1800000000
// DESCRIPTION :: Default channel carrier frequency
//                Currently it is not used
// **/
#define CELLULAR_ABSTRACT_DEFAULT_CHANNEL_FREQUENCY 180000000
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_CHANNEL_DATE_RATE : 2000000
// DESCRIPTION :: Default channel data rate
//                Currently it is not used
// **/
#define CELLULAR_ABSTRACT_DEFAULT_CHANNEL_DATE_RATE 2000000
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_BS_CONTROL_INFORMATION_INTERVAL :
//                                                       (20 * MILLI_SECOND)
// DESCRIPTION :: Default control information interval
// **/
#define CELLULAR_ABSTRACT_DEFAULT_BS_CONTROL_INFORMATION_INTERVAL \
                                                         (20 * MILLI_SECOND)

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_BS_SYSTEM_INFORMATION_DELAY :
//                                                        (10 * NANO_SECOND)
// DESCRIPTION :: Default max first control information delay
// **/
#define CELLULAR_ABSTRACT_DEFAULT_BS_SYSTEM_INFORMATION_DELAY  \
                                                (10 * NANO_SECOND)

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_BS_CONTROL_TX_POWER : 30
// DESCRIPTION :: Default control channel transmission power
// **/
#define CELLULAR_ABSTRACT_DEFAULT_BS_CONTROL_TX_POWER 30.0

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_THRESHOLD_BASED_CAC_PARAM_N  4
// DESCRIPTION :: Default threshold based CAC param N
// **/
#define CELLULAR_ABSTRACT_DEFAULT_THRESHOLD_BASED_CAC_PARAM_N  4

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_THRESHOLD_BASED_CAC_PARAM_M  1
// DESCRIPTION :: Default threshold based CAC param M , M < N
// **/
#define CELLULAR_ABSTRACT_DEFAULT_THRESHOLD_BASED_CAC_PARAM_M  1

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_DURATION
//                                                    (200 * MILLI_SECOND)
// DESCRIPTION :: Default round robin congestion control duration
// **/
#define CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_DURATION  \
                                                       (200 * MILLI_SECOND)

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_ACCESS_CLASS 10
// DESCRIPTION :: Default round robin congestion control, the number of access class
// **/
#define CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_ACCESS_CLASS 10

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_REFRAIN_PROBABILITY 0.25
// DESCRIPTION :: Default round robin congestion control,
//                the probability refrain from transmission channel request
// **/
#define CELLULAR_ABSTRACT_DEFAULT_ROUND_ROBIN_CONGESTION_CONTROL_REFRAIN_PROBABILITY 0.25

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_LOCATION_UPDATE_ATTEMPT : 4
// DESCRIPTION :: Maximum retry for location update if fail
// **
#define CELLULAR_ABSTRACT_MAX_LOCATION_UPDATE_ATTEMPT 4

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_PAGE_REQUEST_ATTEMPT : 4
// DESCRIPTION :: Maximum retry for page request if fail
// **/
#define CELLULAR_ABSTRACT_MAX_PAGE_REQUEST_ATTEMPT 4

// /**
// CONSTANT    :: CELLULAR_RESOURCE_USAGE_SAMPLE_INTERVAL:500 * MILLI_SECOND
// DESCRIPTION :: The sampling interval for the resource usage
static
const clocktype CELLULAR_RESOURCE_USAGE_SAMPLE_INTERVAL=(500 * MILLI_SECOND);
// **/
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_WIRED_DELAY : 50 * MICRO_SECOND
// DESCRIPTION :: Delay incurred on wired link transmission
// **/
static
const clocktype CELLULAR_ABSTRACT_WIRED_DELAY = (50 * MICRO_SECOND);
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_HANDOVER_END_POLLING_INTERVAL :
//                    100 * MILLI_SECOND
// DESCRIPTION :: The polling interval to check if handover is finished
// **/
static
const clocktype CELLULAR_ABSTRACT_HANDOVER_END_POLLING_INTERVAL
                    = (100 * MILLI_SECOND);
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_NO_COOMUNICATION_POLLING_INTERVAL :
//                    100 * MILLI_SECOND
// DESCRIPTION :: The polling interval to check if handover is finished
// **/
static
const clocktype CELLULAR_ABSTRACT_NO_COMUNICATION_POLLING_INTERVAL
                    = (100 * MILLI_SECOND);
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_CALL_CONFIRM_WAITING_TIME :
//                       10 * MICRO_SECOND
// **/
static
const clocktype CELLULAR_ABSTRACT_CALL_CONFIRM_WAITING_TIME
                    = (10 * MICRO_SECOND );

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_CALL_ALERTING_WAITING_TIME :
//                       20 * MICRO_SECOND
// **/
static
const clocktype CELLULAR_ABSTRACT_CALL_ALERTING_WAITING_TIME
                    = (20 * MICRO_SECOND );

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_CALL_CONNECT_WAITING_TIME :
//                       30 * MICRO_SECOND
// **/
static
const clocktype CELLULAR_ABSTRACT_CALL_CONNECT_WAITING_TIME
                    = (30 * MICRO_SECOND );

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_PAGING_TIME : 30 * SECOND
// DESCRIPTION :: The wating time before a reject msg is sent to
//                source after sending the paging msg to BSs
// **/
static const clocktype CELLULAR_ABSTRACT_PAGING_TIME = (30 * SECOND);

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_VLR_LIFETIME : 600 * MINUTE
// DESCRIPTION :: The lifetime of a VLR record
//                before it is deleted from the VLR
// **/
static const clocktype CELLULAR_ABSTRACT_VLR_LIFETIME = (600 * MINUTE);

// /**
// CONSTANT    ::
//             DefaultCellularAbstractPeriodicLocationUpdateTimer_T3212Time:
//                 (30 * MINUTE)
// DESCRIPTION :: Defines the default Periodic Location Update Timer duration
// **/
static const clocktype
    DefaultCellularAbstractPeriodicLocationUpdateTimer_T3212Time
        = (30 * MINUTE);

// /**
// CONSTANT    ::
//              DefaultCellularAbstractLocationUpdateRequestTimer_T3210Time:
//                (10 * SECOND)
// DESCRIPTION :: Defines the default waiting
//                for Location Update Result Timer duration
// **/
static const clocktype
    DefaultCellularAbstractLocationUpdateRequestTimer_T3210Time
        = (10 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractLocationUpdateRequestTimer_T3211Time
//                   : (10 * SECOND)
// DESCRIPTION :: Defines the default waiting time before restarting
//                a Location Update request
// **/
static const clocktype
    DefaultCellularAbstractLocationUpdateRequestTimer_T3211Time
        = (10 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCMServiceRequestTimer_T3230Time
//                   : (15 * SECOND)
// DESCRIPTION :: Defines the default waiting time for
//                the response of CM service request
// **/
static const clocktype
    DefaultCellularAbstractCMServiceRequestTimer_T3230Time
        = (15 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractMsChannelReleaseTimer_T3240Time
//                : (10 * SECOND)
// DESCRIPTION :: Defines the default waiting time for
//                channel release msg from network
// **/
static const clocktype
    DefaultCellularAbstractMsChannelReleaseTimer_T3240Time
        = (10 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractImmediateAssignmentTimer_T3101Time
//                   : 500 * MILLI_SECOND
// DESCRIPTION :: Defines the default waiting time for MS
//                to access the assigned resources
// **/
static const clocktype
    DefaultCellularAbstractImmediateAssignmentTimer_T3101Time
        = (500 * MILLI_SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractPageRequest_T3113Time
//                    : 600 * MILLI_SECOND
// DESCRIPTION :: Defines the default waiting time for the page response
// **/
static const clocktype
    DefaultCellularAbstractPageRequest_T3113Time = (600 * MILLI_SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractSlotNumberPerFrame 8
// DESCRIPTION :: Defines the default number of slots in a frame
// **/
static const int DefaultCellularAbstractSlotNumberPerFrame = 8;

// /**
// CONSTANT    :: DefaultCellularAbstractMaxRetryTimer_T3126Time :
//                    MAX(((CELLULAR_TX_INTEGER+2*CELLULAR_RANDACCESS_S)
//                        *DefaultCellularSlotDuration),(5 * SECOND))
// DESCRIPTION :: Defines the default waiting time for the response
//                to the channel request after maximum retries
// **/
static const clocktype
    DefaultCellularAbstractMaxRetryTimer_T3126Time =
        MAX(((CELLULAR_TX_INTEGER + 2 * CELLULAR_RANDACCESS_S)
        * DefaultCellularSlotDuration), (5 * SECOND));

// /**
// CONSTANT    :: DefaultCellularAbstractMsChannelReleaseTimer_T3110Time :
//                   (200 * MILLI_SECOND)
// DESCRIPTION :: Defines the Default CellularAbstract MS Channel Release
//                Timer before the resource available to other application
// **/
static const clocktype
    DefaultCellularAbstractMsChannelReleaseTimer_T3110Time
        = (200 * MILLI_SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractBsChannelReleaseTimer_T3111Time :
//                   (200 * MILLI_SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
// **/
static const clocktype
    DefaultCellularAbstractBsChannelReleaseTimer_T3111Time
        = (200 * MILLI_SECOND);

//***********************//
//call control timer     //
//***********************//

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T301Time :
//                   (180 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T301 start when aleart recieved,stop when conn rcvd,
//                expire clear the call
// **/

static const clocktype
    DefaultCellularAbstractCallControl_T301Time = (180 * SECOND);
// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T303Time : (30 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T303 start when Cm Service request sent,
//                stop when Call proceedign or
//                rel complete rcvd, expire clear the call
// **/

static const clocktype
    DefaultCellularAbstractCallControl_T303Time = (30 * SECOND);
// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T305Time : (30 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T305 start when disconnect sent,
//                stop when release or disconnect rcvd,
//                expire release the call
// **/

static const clocktype
    DefaultCellularAbstractCallControl_T305Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T308Time : (30 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T308 start when release request sent,
//                stop when rel comp or release rcvd,
//                expire first time,retrans relesae,
//                expire second time, send release
// **/
static const clocktype
    DefaultCellularAbstractCallControl_T308Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T308MaxRetry : 2
// DESCRIPTION :: Defines the DefaultCellularAbstract number of retry
// **/
static const int
DefaultCellularAbstractCallControl_T308MaxRetry = 2;

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T310Time : (30 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T310 start when call preoceeding rcvd,
//                stop when alert or connect, disconnect or proceeding rcvd
// **/
static const clocktype
    DefaultCellularAbstractCallControl_T310Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T313Time : (30 * SECOND)
// DESCRIPTION :: Defines the DefaultCellularAbstract BS
//                Channel Release Timer duration
//                T313 start when connect sent, stop when connect ack ecvd,
//                expire send disc
// **/
static const clocktype
    DefaultCellularAbstractCallControl_T313Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T323Time : (30 * SECOND)
//                Currently unused
// **/

static const clocktype
    DefaultCellularAbstractCallControl_T323Time = (30 * SECOND);
// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T331Time : (30 * SECOND)
//                Currently unused
// **/

static const clocktype
    DefaultCellularAbstractCallControl_T331Time = (30 * SECOND);
// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T332Time : (30 * SECOND)
//                Currently unused
// **/
static const clocktype
DefaultCellularAbstractCallControl_T332Time  = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T333Time : (30 * SECOND)
//                Currently unused
// **/
static const clocktype
    DefaultCellularAbstractCallControl_T333Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T334Time : (30 * SECOND)
//                Currently unused
// **/
static const clocktype
DefaultCellularAbstractCallControl_T334Time = (30 * SECOND);

// /**
// CONSTANT    :: DefaultCellularAbstractCallControl_T335Time : (30 * SECOND)
//                Currently unused
// **/
static const clocktype
DefaultCellularAbstractCallControl_T335Time = (30 * SECOND);

// /**
// ENUM        :: CellularAbstractLocationUpdatingType
// DESCRIPTION :: Definition of the types of location update
// **/
typedef enum
{
    CELLULAR_ABSTRACT_NORMAL_LOCATION_UPDATING = 0,
    CELLULAR_ABSTRACT_PERIODIC_LOCATION_UPDATING = 1,
    CELLULAR_ABSTRACT_IMSI_ATTACH = 2,
    CELLULAR_ABSTRACT_IMSI_DETACH = 3,
    CELLULAR_ABSTRACT_LOCATION_UPTATE_TYPE_INVALID,
}CellularAbstractLocationUpdatingType;

// /**
// ENUM        :: CellularAbstractChannelStatus
// DESCRIPTION :: Definition of the status of channels
//                Only used when channel id options is enabled
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CHANNEL_STATUS_IDLE = 0,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_INUSE_GENERAL =  1,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_INUSE_DATA = 2,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_INUSE_VOICE = 3,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_INUSE_CONTROL = 4,

    CELLULAR_ABSTRACT_CHANNEL_STATUS_RESERVED_GENERAL = 5,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_RESERVED_DATA = 6,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_RESERVED_VOICE = 7,
    CELLULAR_ABSTRACT_CHANNEL_STATUS_RESERVED_HANDOVER = 8,

    //use for channel assignemnt beforethe ms really use it
    CELLULAR_ABSTRACT_CHANNEL_STATUS_TENTATIVE = 9,
}CellularAbstractChannelStatus;

// /**
// ENUM        :: CellularAbstractCommStates
// DESCRIPTION :: Definition of the status of MS'scommunicating
// **/
typedef enum
{
    CELLULAR_ABSTRACT_MS_COMM_STATE_IDLE  = 0,
    CELLULAR_ABSTRACT_MS_COMM_STATE_COMMUNICATING = 1

}CellularAbstractCommStates;

// /**
// ENUM        :: CellularAbstractUserGender
// DESCRIPTION :: Definition of the gender of users
// **/
typedef enum
{
    CELLULAR_ABSTRACT_USER_MALE  = 0,
    CELLULAR_ABSTRACT_USER_FEMALE = 1
}CellularAbstractUserGender;


// /**
// ENUM        :: CellularAbstractNetworkTimerType
// DESCRIPTION :: Definition of the timers defined at layer 3
// **/
typedef enum
{
    MSG_NETWORK_CELLULAR_SystemInfoTimer = 0,

    //random access timer
    MSG_NETWORK_CELLULAR_WaitForChannelRequestResponseTimer,
    MSG_NETWORK_CELLULAR_ImmediateAssignmentTimer_T3101,

    //after M+1 retransmission of the channel request
    MSG_NETWORK_CELLULAR_T3126Timer,

    //location update timer

    //activate when send out the Loc update Req,
    //stop when receive Loc Update Suc or Rej
    MSG_NETWORK_CELLULAR_T3210Timer,
    MSG_NETWORK_CELLULAR_T3211Timer,
    MSG_NETWORK_CELLULAR_T3212Timer,//periodical location update

    MSG_NETWORK_CELLULAR_T3230Timer,//cm service

    //channel release
    MSG_NETWORK_CELLULAR_T3110Timer,
    MSG_NETWORK_CELLULAR_T3111Timer,
    MSG_NETWORK_CELLULAR_T3240Timer,

    //call control
    MSG_NETWORK_CELLULAR_T301Timer,
    MSG_NETWORK_CELLULAR_T303Timer,
    MSG_NETWORK_CELLULAR_T305Timer,
    MSG_NETWORK_CELLULAR_T308Timer,
    MSG_NETWORK_CELLULAR_T310Timer,
    MSG_NETWORK_CELLULAR_T313Timer,
    MSG_NETWORK_CELLULAR_T323Timer,
    MSG_NETWORK_CELLULAR_T331Timer,
    MSG_NETWORK_CELLULAR_T332Timer,
    MSG_NETWORK_CELLULAR_T333Timer,
    MSG_NETWORK_CELLULAR_T334Timer,
    MSG_NETWORK_CELLULAR_T335Timer,
    MSG_NETWORK_CELLULAR_T338Timer,
    MSG_NETWORK_CELLULAR_T3113Timer,

    //PDP service end timer at SC and aggregated node
    MSG_NETWORK_CELLULAR_PDP_DEACTIVATION_Timer,

    //pageing waiting timer for the page response
    MSG_NETWORK_CELLULAR_PAGING_Timer,

    //resource utilization sample tiemr
    MSG_NETWORK_CELLULAR_BW_SAMPLE_Timer,
}CellularAbstractNetworkTimerType;

// /**
// ENUM        :: CellularAbstractRRCause
// DESCRIPTION :: Definition of cause for the channel release
// **/
typedef enum
{
    CELLULAR_ABSTRACT_NORMAL_EVENT = 0,
    CELLULAR_ABSTRACT_ABNORMAL_RELEASE = 1,
    CELLULAR_ABSTRACT_PREEMPTIVE_RELEASE,

}CellularAbstractRRCause;

// /**
// ENUM        :: CellularAbstractCMServiceType
// DESCRIPTION :: Definition of types of CM service
//                CELLULAR_ABSTRACT_MOBILE_ORIGINATING_CALL_ESTABLISHMENT
//                is used now
// **/
typedef enum
{
    CELLULAR_ABSTRACT_MOBILE_ORIGINATING_CALL_ESTABLISHMENT = 0,
    CELLULAR_ABSTRACT_PACKET_MODE_CONNECTION_ESTABLISHMENT = 1,
    CELLULAR_ABSTRACT_EMERGENCY_CALL_ESTABLISHMENT,
    CELLULAR_ABSTRACT_SHORT_MESSAGE_SERVICE,
    CELLULAR_ABSTRACT_SUPPLEMENTARY_SERVICE_ACTIVATION,
    CELLULAR_ABSTRACT_LOCATION_SERVICES,
    CELLULAR_ABSTRACT_CM_SERVICE_TYPE_INVALID,
}CellularAbstractCMServiceType;

// /**
// ENUM        :: CellularAbstractCallDisconnectType
// DESCRIPTION :: Definition of types of call disconnection
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_DISCONNECT_BY_MS = 0,
    CELLULAR_ABSTRACT_CALL_DISCONNECT_BY_NETWORK = 1,
}CellularAbstractCallDisconnectType;

// /**
// ENUM        :: CellularAbstractCallReleaseType
// DESCRIPTION :: Definition of types of call release
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_RELEASE_BY_MS = 0,
    CELLULAR_ABSTRACT_CALL_RELEASE_BY_NETWORK = 1,
}CellularAbstractCallReleaseType;

// /**
// ENUM        :: CellularAbstractCallReleaseCompleteType
// DESCRIPTION :: Definition of types of call release complete
// **/
typedef enum
{
    CELLULAR_ABSTRACT_CALL_RELEASE_COMPLETE_BY_MS = 0,
    CELLULAR_ABSTRACT_CALL_RELEASE_COMPLETE_BY_NETWORK =  1,
}CellularAbstractCallReleaseCompleteType;

// /**
// ENUM        :: CellularAbstractHandoverState
// DESCRIPTION :: Definition of state of handover
// **/
typedef enum
{
    CELLULAR_ABSTRACT_HANDOVER_STATE_IDLE = 0,
    CELLULAR_ABSTRACT_HANDOVER_STATE_PENDING,
    CELLULAR_ABSTRACT_HANDOVER_STATE_PROCEEDING,
}CellularAbstractHandoverState;

// /**
// ENUM        :: CellularAbstractHandoverState
// DESCRIPTION :: Definition of types of handover
// **/
typedef enum
{
    CELLULAR_ABSTRACT_HANDOVER_TYPE_INTRA_CELL = 0,
    CELLULAR_ABSTRACT_HANDOVER_TYPE_INTER_CELL_INTRA_SC,//inter CELL
    CELLULAR_ABSTRACT_HANDOVER_TYPE_INTER_SC,
}CellularAbstractHandoverType;

// /**
// ENUM        :: CellularAbstractOptimizationLevel
// DESCRIPTION :: Definition of Optimization Level
// **/
typedef enum
{
    CELLULAR_ABSTRACT_OPTIMIZATION_LOW = 0,
    CELLULAR_ABSTRACT_OPTIMIZATION_MEDIUM,
    CELLULAR_ABSTRACT_OPTIMIZATION_HIGH,
}CellularAbstractOptimizationLevel;

///*************************************************************
//   Block: data definition for MS
//**************************************************************
// /**
// STRUCT      :: CellularAbstractChannelRequestInfo
// DESCRIPTION :: Mac data of the cellular abstract MAC
//                for each new request a CellularAbstractChannelRequestInfo
//                will be generated
//                and will be freed when failuare or assignemnt
// **/
typedef struct struct_cellular_abstract_channel_request_info_str
{

    int numChannelRequestAttempts;
    BOOL isRequestingChannel;
    Message* channelRequestMsg;
    Message* channelRequestTimer;
    int pageRequestId; // used when page response

    // wait for response after M+1 retry,after expire stop the request
    Message* timerT3126Msg;
    Message* timerT3240Msg; // wait for channel release

    clocktype reqInitTime;
}CellularAbstractChannelRequestInfo;

// /**
// STRUCT      :: CellularAbstractApplicationInfoAtMs
// DESCRIPTION :: Call(application, service) information at MS
// **/
typedef struct struct_cellular_abstract_application_info_at_ms_str
{

    BOOL inUse;

    //general info
    int transactionId;
    int appId;
    CellularAbstractApplicationType applicationType;
    clocktype callStartTime;
    clocktype callEndTime;
    clocktype callDuration;
    double bandwidthRequired;
    int numChannelRequired;
    double dataRateRequiremnt;
    double delayBound;
    NodeAddress srcMsNodeId;
    NodeAddress srcMsNodeAddr;
    NodeAddress srcBsNodeId;
    NodeAddress srcBsNodeAddr;

    NodeAddress destMsNodeId;
    NodeAddress destMsNodeAdd;
    NodeAddress destBsNodeId;
    NodeAddress destBsNodeAddr;

    NodeAddress msNodeId; // this appInfo is created for the src or dest

    CellularAbstractCallSrcDestType callSrcDestType;
    unsigned char msRrState;
    unsigned char msMmState;
    CellulaAbstractMsCcState msCcState;

    //resource alloated
    BOOL   isDedicatedChannelAssigned;
    double bandwidthAllocated;
    int sectorId;
   int assignedULChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
   int assignedDLChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
    CellularAbstractChannelRequestInfo *channelRequestForApplication;

    BOOL isHandoverInProgress;

    //call control timer
    Message *timerT3230;
    Message *timerT303;
    Message *timerT305;
    Message *timerT308;
    Message *timerT310;
    Message *timerT313;
    Message *timerT323;
    Message *timerT332;
    Message *timerT335;
}CellularAbstractApplicationInfoAtMs;

// /**
// STRUCT      :: CellularAbstractAssociatedBsSectorInfo
// DESCRIPTION :: Associated BS and sector info
// **/
typedef struct struct_cellular_abstract_associated_bs_sector_info_str
{
    NodeAddress associatedBSNodeId;
    NodeAddress associatedBSNodeAddress;
    int lac;
    int cellIdentity;

    // uplink control channel,Values received from BS via beacon msgs
    int controlDLChannelIndex;
    int controlULChannelIndex; //downlink control channel
    int associatedSectorId; // sector

        // congestion control
    CellularAbstractCallCongestionControlPolicy congestionControlPolicy;

    //params for round-robin congestion ocntrol
    double refrainProb;
    short numAccessClass;
    clocktype oneControlDuration;
}CellularAbstractAssociatedBsSectorInfo;

// /**
// STRUCT      :: CellularAbstractHandoverInfo
// DESCRIPTION :: Handover target BS and sector info
// **/
typedef struct struct_cellular_abstract_handover_info_str
{
    NodeAddress targetBSNodeId;
    NodeAddress targetBSNodeAddress;
    int lac;
    int cellIdentity;

    // uplink control channel,Values received from BS via beacon msgs
    int controlDLChannelIndex;
    int controlULChannelIndex; // downlink control channel
    int targetSectorId; // sector
    clocktype handoverReference;
}CellularAbstractHandoverInfo;

// /**
// STRUCT      :: CellularAbstractBsSectorCandidate
// DESCRIPTION :: data struture of BSs that MS can attach to
// **/
typedef struct struct_cellular_abstract_bs_sector_candidate_info_str
{
    CellularAbstractBsSystemInfo sysInfo;
    clocktype lastReceived;
    struct struct_cellular_abstract_bs_sector_candidate_info_str*
        nextCandidate;
}CellularAbstractBsSectorCandidate;
// /**
// STRUCT      :: CellularAbstractLayer3MsStats
// DESCRIPTION :: statistics of MS at Layer 3
// **/
typedef struct struct_cellular_abstract_layer3_ms_stats_str
{
    int numSystemInfoRecvd;
    int numMeasurementReportRecvdFromMac;

    int numCellSelectionPerformed;
    int numCellReSelectionPerformed;

    //location update
    int numLocationUpdateAttempt;
    int numLocationUpdateSent;
    double numLocationUpdateSentperAttempt;
    int numLocationUpdateSuccess;//positve response
    int numLocationUpdateRejected;//negative response reject by VLR
    int numLocationUpdateFailaure;//no response, does get the channel

    //channel request
    int numChannelRequestSent;
    int numImmediateAssignmentRcvd;
    int numImmediateAssignmentRejectRcvd;
    clocktype totalAccessDelay;
    double avgAccessDelay;

    int numChannelRequestAttempt;
    double numChnnaelRequestSentPerAttempt;
    int numChannelRequestAttemptForLocUpdate;
    int numChannelRequestAttemptForPageResponse;
    int numChannelRequestAttemptForCallIntiating;

    int numChannelRequestSuccess;//positve response
    int numChannelRequestSuccessForLocUpdate;
    int numChannelRequestSuccessForPageResponse;
    int numChannelRequestSuccessForCallIntiating;

    int numChannelRequestFailure;//no response
    int numChannelRequestFailureForLocUpdate;
    int numChannelRequestFailureForPageResponse;
    int numChannelRequestFailureForCallIntiating;

    //channel release
    int numChannelReleaseRcvd;

    //call related
    int numCmServiceRequestSent;
    int numCmServiceRejectRdvd;
    int numcmServiceAcceptRcvd;

    int numMOCallSetupSent;
    int numMOCallProceedingRcvd;
    int numMOCallAlertingRcvd;
    int numMOCallConnectRcvd;
    int numMOCallConnectAckSent;

    int numMOCallDisconnectByMsSent;
    int numMOCallDisconnectByNwRcvd;
    int numMTCallDisconnectByMsSent;
    int numMTCallDisconnectByNwRcvd;

    int numMOCallReleaseByMsSent;
    int numMOCallReleaseByNwRcvd;
    int numMTCallReleaseByMsSent;
    int numMTCallReleaseByNwRcvd;

    int numCallReleaseCompleteSent;
    int numCallReleaseCompleteRcvd;

    int numPageRequestRcvd;
    int numPageResponseSent;

    int numMTCallSetupRcvd;
    int numMTCallCallConfirmSent;
    int numMTCallCallAlertingSent;
    int numMTCallConnectSent;
    int numMTCallConnectAckRcvd;

    int numVoiceCallSetupInit;
    int numVoiceCallSetupRcvd;
    int numDataCallActivationSent;
    int numDataCallActivationRcvd;

    //handover
    int numHandoverRequiredSent;
    int numRIHandoverCommandRcvd;
    int numRIHandoverCompleteSent;
    int numHandoverRequiredRejectRcvd;

    //application
    int numCallStartRcvd;
    int numCallEndRcvd;
    int numCallRejectSent;
    int numCallDroppedSent;
}CellularAbstractLayer3MsStats;

// /**
// STRUCT      :: CellularAbstractLayer3BsMinInfo
// DESCRIPTION :: Info structure of BS for optimization
//                This is the minimal data structure for BS
// **/
typedef struct struct_cellular_abstract_layer3_bs__min_info_str
{
    // Node Id of the BS
    NodeId bsNodeId;

    // number of sectors this BS has
    short numSectors;

    // Position of the BS for calculating signal strength level
    Coordinates bsPosition;

    // system information
    Message *systemInfoType2;

    struct struct_cellular_abstract_layer3_bs__min_info_str* next;
}CellularAbstractLayer3BsMinInfo;

// /**
// STRUCT      :: CellularAbstractLayer3MsInfo
// DESCRIPTION :: Info structure of MS
//                This is the main data structure for MS
// **/
typedef struct struct_cellular_abstract_layer3_ms_info_str
{

    NodeAddress msNodeId;
    NodeAddress msNodeAddress;

    //user info
    short userAge;
    CellularAbstractUserGender userGender;

    //access class
    int msAccessClass;

    //attach/detach
    BOOL isPowerOn;
    BOOL powerOffInProgress;

    //BLOCK:for cell selection and location update
    BOOL isBsSelected;
    BOOL isLocationUpdateInProgress;
    Message *timerT3212Msg; // Periodic Update Timer

    // Location Update information
    CellularAbstractLocationUpdatingType   locationUpdatingType;
    clocktype   t3212TimeoutValue;
    CellularAbstractChannelRequestInfo *channelReqForLocationUpdate;
    int numLocationUpdateAttempt;
    Message *timerT3210Msg;//wait for response after send out loc update req
    Message *timerT3211Msg;//activate when loc failure to retry loc update

    //timer
    //activate when receive a assignment rejection
    //it will block all other new CH req before expire
    Message *timerT3122Msg;

    // associated BS & sector info
    //assume all the sector in the same cell share the same
    //uplink and downlink control channels
    CellularAbstractAssociatedBsSectorInfo *associatedBsSectorInfo;

    //measurement related
    //record the location information of the base stations,
    //and signal strength from multiple sectors
    CellularAbstractBsSectorCandidate *bsSectorCandidate;
    CellularAbstractMeasurementReportMsgInfo *reportInfo;

    //channel request related
    short txInteger;
    short maxReTrans;

    //Transactions: applicaiton plus other location updating,
    //security,authorization activities
    BOOL isTransactionInProcess;
    int numTransactions;

    //Application (services) info
    int numActiveApplicaions;
    CellularAbstractApplicationInfoAtMs
        msAppInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_MS+1];

    //user communication status
    CellularAbstractCommStates userCommState;

    //Handover
    BOOL isHandoverInProgress;
    CellularAbstractHandoverState handoverState;
    double  handoverMargin;
    CellularAbstractHandoverInfo *handoverInfo;

    //statistics
    CellularAbstractLayer3MsStats    stats;

    //optimization
    CellularAbstractLayer3BsMinInfo* bsMinInfo;
    Coordinates lastUpdatePosition;
}CellularAbstractLayer3MsInfo;

///*************************************************************
//   Block: data definition for BS
//**************************************************************//
// /**
// STRUCT      :: CellularAbstractSectorInfo
// DESCRIPTION :: data structursector info
// **/
typedef struct struct_cellular_abstract_sector_info_str
{
    int sectorId;
    int channelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_PER_SECTOR];
    int numChannelAssigned; //channels assigned to this sector
    int numChannelInUse;
    double sectorBandwidthAllocated;
    double bandwidthInUse;
}CellularAbstractSectorInfo;

// /**
// STRUCT      :: CellularAbstractChannelDescription
// DESCRIPTION :: data structure for the channel inforamtion
// **/
typedef struct struct_sbstract_cellular_channel_description
{
    short channelIndex;
    double channelFreq;
    //int timeslot; //TDD
   //int codeSequence; //CDMA
    BOOL isControlChannel;
    double channelBandwidth;
    double channelBasicDateRate;
    double channelBusyTime;
    double channelIdelTime;
    BOOL isChannelInUse;
    CellularAbstractChannelStatus channelStatus;
}CellularAbstractChannelDescription;
// /**
// STRUCT      :: CellularAbstractApplicationInfoAtBs
// DESCRIPTION :: Data structure used to keep application Information at BS
// **/
typedef struct struct_cellular_abstract_application_info_at_bs_str
{
    BOOL inUse;

    //general info
    int transactionId;
    int appId;
    CellularAbstractApplicationType applicationType;
    //clocktype callStartTime;
    //clocktype callEndTime;
    double bandwidthRequired;
    int numChannelRequired;
    double dataRateRequiremnt;
    double delayBound;
    NodeAddress srcMsNodeId;
    NodeAddress srcMsNodeAddr;
    NodeAddress srcBsNodeId;
    NodeAddress srcBsNodeAddr;

    NodeAddress destMsNodeId;
    NodeAddress destMsNodeAdd;
    NodeAddress destBsNodeId;
    NodeAddress destBsNodeAddr;

    //this appInfo is create for this msNodeId,it could be src or dest
    NodeAddress msNodeId;
    int sectorId;
    CellularAbstractCallSrcDestType callSrcDestType;

    BOOL isDedicatedChannelAssigned;
    double bandwidthAllocated;
   int assignedULChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
   int assignedDLChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];

    //virtual circuit id from trunk or interface id
    //when other network is used
    BOOL isBs2ScConnected;
    int  bs2ScConnectionId;

    //info for handover after intra handover successful,
    //this information is meaningless
    BOOL isHandoverInProgress;
    double hoBandwidthAllocated;
int hoAssignedULChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
int hoAssignedDLChannelIndex[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
    int hoSectorId;

    //all kinds of timer associate with this transaction
    Message *T3101TimerMsg;
}CellularAbstractApplicationInfoAtBs;

// /**
// STRUCT      :: CellularAbstractLayer3BsStats
// DESCRIPTION :: Data structure stattstics at bs
// **/
typedef struct struct_cellular_abstract_layer3_bs_stats_str
{

    //Beacon control info
    int numSysInfoSent;

    //channel request
    int numChannelRequestRcvd;
    int numImmediateAssignmentSent;
    int numImmediateAssignmentRejectSent;

    //channel release
    int numClearCommandRcvd;
    int numChannelReleaseSent;

    //cm service
    int numCmServiceAcceptRcvd;
    int numCmServiceAcceptFwd;
    int numCmServiceRequestRcvd;
    int numCmServiceRequestFwd;
    int numCmServiceRejectByBsSent;

    //call conctrol
    int numMOCallSetupRcvd;
    int numMOCallSetupFwd;
    int numMTCallSetupRcvd;
    int numMTCallSetupFwd;

    int numMTCallConfirmRcvd;
    int numMTCallConfirmFwd;

    int numMTCallAlertingRcvd;
    int numMTCallAlertingFwd;
    int numMTCallConnectRcvd;
    int numMTCallConnectFwd;
    int numMTCallConnectAckRcvd;
    int numMTCallConnectAckFwd;

    int numMOCallAlertingRcvd;
    int numMOCallAlertingFwd;
    int numMOCallConnectRcvd;
    int numMOCallConnectFwd;
    int numMOCallConnectAckRcvd;
    int numMOCallConnectAckFwd;

    int numCallProceedingRcvd;
    int numCallProceedingFwd;

    int numMOCallDisconnectByMsRcvd;
    int numMOCallDisconnectByMsFwd;
    int numMOCallDisconnectByNwRcvd;
    int numMOCallDisconnectByNwFwd;

    int numMTCallDisconnectByMsRcvd;
    int numMTCallDisconnectByMsFwd;
    int numMTCallDisconnectByNwRcvd;
    int numMTCallDisconnectByNwFwd;

    int numMOCallReleaseByMsRcvd;
    int numMOCallReleaseByMsFwd;
    int numMOCallReleaseByNwRcvd;
    int numMOCallReleaseByNwFwd;

    int numMTCallReleaseByMsRcvd;
    int numMTCallReleaseByMsFwd;
    int numMTCallReleaseByNwRcvd;
    int numMTCallReleaseByNwFwd;

    //page
    int numPagingRcvd;
    int numPageRequestSent;
    int numPageResponseRcvd;
    int numPageResponseFwd;

    //handover
    int numMSInitHandoverRequiredRcvd;
    int numMSInitHandoverRequiredFwd;
    int numNWInitHandoverRequriedSent;
    int numHandoverRequestRcvd;
    int numHandoverRequestAckSent;
    int numHandoverFailureSent;
    int numMSInitHandoverRequiredRejectRcvd;
    int numMSInitHandoverRequiredRejectFwd;
    int numNWInitHandoverRequiredRejectRcvd;
    int numHandoverCommandRcvd;
    int numRIHandoverCommandSent;
    int numRIHandoverCompleteRcvd;
    int numHandoverCompleteSent;

}CellularAbstractLayer3BsStats;

// /**
// STRUCT      :: CellularAbstractPageRequestInfo
// DESCRIPTION :: Data structure for pagerequest
// **/
typedef struct struct_cellular_abstract_page_request_info_str
{
    BOOL inUse;
    int pageRequestId;
    int numPage;
    Message *pageRequestMsg;
    Message *pageRequestTimer3113;
}CellularAbstractPageRequestInfo;

// /**
// STRUCT      :: CellularAbstractChannelAssignmentInfo
// DESCRIPTION :: Data structure for channel assignment
// **/
typedef struct struct_cellular_abstract_channel_assignment_info_str
{
    BOOL inUse;
    int assignmentId;
    Message *assignmentAccessTimer_T3101;
}CellularAbstractChannelAssignmentInfo;

// /**
// STRUCT      :: CellularAbstractLayer3BsInfo
// DESCRIPTION :: Data structure for bs infomation
//                It is the main data structure of BS
// **/
typedef struct struct_cellular_abstract_layer3_bs_info_str
{
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddress;

   // bsid, lac, ...
    int   cellId;
    int   lac;// Location Area Code

    // associated switch center id
    NodeAddress associatedScNodeId;
    NodeAddress associatedScNodeAddress;

    // location informaiton
    Coordinates bsLocation;

    // channel and channel allocation
    int numAdmissionRequest;
    int numChannelAssigned; // channels assigned to this BS
    int numChannelInUse;   // channel in use
    double bandwidthAllocated;
    double bandwidthInUse;
    CellularAbstractChannelDescription
        bsChannelBank[CELLULAR_ABSTRACT_MAX_CHANNEL_PER_BS];
    int   controlULChannelIndex; // downlink control channel index
    int   controlDLChannelIndex; // downlink control channel index


    // sector info
    short numSectorInCell;
    CellularAbstractSectorInfo
        bsSectorInfo[CELLULAR_ABSTRACT_MAX_SECTOR_PER_BS];

    // service supported
    BOOL  bsServiceSupport[CELLULAR_ABSTRACT_MAX_SERVICE_TYPE_PER_BS];

    // TODO:neignring cell downlink control channel information

    // page info
    int pageRequestId;
    CellularAbstractPageRequestInfo
        bsPageRequestInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS];
    int assignmentId;
    CellularAbstractChannelAssignmentInfo
        bsChannelAssignmentInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS];

    // call information
    CellularAbstractApplicationInfoAtBs
        bsAppInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_BS];

    // pageing timer

    // call admission control
    CellularAbstractCallAdmissionControlPolicy callAdmissionPolicy;

    //params for threshold-based CAC
    int cacThresholdParamN;
    int cacThresholdParamM;

    // congestion control
    CellularAbstractCallCongestionControlPolicy congestionControlPolicy;

    //params for round-robin congestion ocntrol
    double refrainProb;
    short numAccessClass;
    clocktype oneControlDuration;

    // handover
    double  handoverMargin;

    // TX power
    double bsTxPower;

    // channel requests transmission retry
    short txInteger;
    short maxReTrans;

    clocktype  systemInfoBroadcastInterval;

    // system information
    Message *systemInfoType2;

    // statistics
    CellularAbstractLayer3BsStats stats;
}CellularAbstractLayer3BsInfo;

//*************************************************************
//   Block: data structure definition for SC
//*************************************************************
// /**
// STRUCT      :: CellularAbstractScControlBsInfo
// DESCRIPTION :: Information of BS controled by the switch center
//                It is the main data structure of BS
// **/
typedef struct struct_cellular_abstract_sc_control_bs_str
{
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddress;
    int bsCellId;
    int lac;
}CellularAbstractScControlBsInfo;

// /**
// STRUCT      :: CellularAbstractCircuitDescription
// DESCRIPTION :: data structure for the circuit
// **/
typedef struct struct_cellular_abstract_circuit_description
{
    //short circuitIndex;
    //int timeslot; //TDD
    //double circuitBandwidth;
    //double circuitBasicDateRate;
    //double circuitBusyTime;
    //double circuitIdelTime;
    //BOOL isCircuitInUse;
}CellularAbstractCircuitDescription;

// /**
// STRUCT      :: CellularAbstractApplicationInfoAtSc
// DESCRIPTION :: data structure for application at switch center
// **/
typedef struct struct_cellular_abstract_application_info_at_sc_str
{
    BOOL inUse;

    //general info
    int transactionId;
    int appId;
    CellularAbstractApplicationType applicationType;
    // clocktype callStartTime;
    // clocktype callEndTime;
    double bandwidthRequired;
    int numChannelRequired;
    double dataRateRequiremnt;
    double delayBound;
    NodeAddress srcMsNodeId;
    NodeAddress srcMsNodeAddr;
    NodeAddress srcBsNodeId;
    NodeAddress srcBsNodeAddr;

    NodeAddress destMsNodeId;
    NodeAddress destMsNodeAdd;
    NodeAddress destBsNodeId;
    NodeAddress destBsNodeAddr;

    NodeAddress msNodeId; // this info is created for this ms.
    NodeAddress msNodeAddr;
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddr;

    CellularAbstractCallSrcDestType callSrcDestType;
    BOOL isBs2ScConnected;

    // virtual circuit id from trunk or interface id
    // when other network is used
    int Bs2ConnectionId;

    unsigned char nwRrState;
    unsigned char nwMmState;
    CellularAbstractNwCcState nwCcState;

    BOOL isHandoverInProgress;

    NodeAddress nextHopNodeId;//currently not used
    NodeAddress nextHopNodeAddr;//currently not used

    // all kinds of timer associate with this transaction
    Message *timerT301;
    Message *timerT303;
    Message *timerT305;
    Message *timerT306;
    Message *timerT308;
    Message *timerT310;
    Message *timerT313;
    Message *timerT323;
    Message *timerT331;
    Message *timerT333;
    Message *timerT334;
    Message *timerT338;

    Message *deactivatePDPTimer;
}CellularAbstractApplicationInfoAtSc;

// /**
// STRUCT      :: CellularAbstractScPagingInfo
// DESCRIPTION :: data structure for paging
// **/
typedef struct struct_cellular_abstract_sc_paging_info_str
{
    BOOL inUse;

    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractApplicationType appType;
    Message *pagingTimer;
    int numPagingAttempt;
    int numBsPaged;

    // how many Bs reponde to this paging, eithernegative or positive
    int numBsResponsed;

    BOOL msResponsed;
}CellularAbstractScPagingInfo;

// /**
// STRUCT      :: CellularAbstractVlrEntry
// DESCRIPTION :: data structure for a vlr entry in the VLR
// **/
typedef struct struct_cellular_abstract_vlr_entry_str
{
    BOOL    inUse;

    NodeAddress msNodeId;
    NodeAddress msNodeAddress;

    // location information of the MS
    int   lac;

    // Time of creation of this entry/record in the VLR
    clocktype lastUpdated;
}CellularAbstractVlrEntry;
// /**
// STRUCT      :: CellularAbstractLayer3ScStats
// DESCRIPTION :: data structure for statistics at switch center
// **/
typedef struct struct_cellular_abstract_layer3_sc_stats_str
{
    //locaiton information
    int numLocationUpdateRcvd;
    int numLocationUpdateAcceptSent;
    int numLocationUpdateRejectSent;
    int numMapdUpdateLocationSent;
    int numMapdCancelLocationRcvd;

    //CM service
    int numCMServiceRequestRcvd;
    int numCMServiceAcceptSent;
    int numCMServiceRejectByScSent;

    //call control
    int numMOCallSetupRcvd;
    int numMTCallsetupSent;
    int numMTCallConfirmRcvd;
    int numMTCallAlertingRcvd;
    int numMTCallConnectRcvd;
    int numMTCallConnectAckSent;
    int numMTCallDisconnectByNwSent;
    int numMTCallDisconnectByMsRcvd;
    int numMTCallReleaseByMsRcvd;
    int numMTCallReleaseByMwSent;

    int numMOCallAlertingSent;
    int numMOCallConnectSent;
    int numMOCallConnectAckRcvd;
    int numMOCallDisconnectByNwSent;
    int numMOCallDisconnectByMsRcvd;
    int numMOCallReleaseByNwSent;
    int numMOCallReleaseByMsRcvd;

    int numCallReleaseCompleteSent;
    int numCallreleaseCompleteRcvd;

    int numCallProceedingSent;

    int numPagingSent;
    int numPageResponseRcvd;

    //BS-SC command
    int numClearCommandSent;

    //handover
    int numHandoverRequiredRcvd;
    int numIntraCellHandoverRequiredRcvd;
    int numInterCellIntraScHandoverRequiredRcvd;
    int numInterScHandoverRequiredRcvd;

    int numHandoverRequestSent;
    int numHandoverRequestAckRcvd;
    int numHandoverFailureRcvd;
    int numHandoverRequiredRejectSent;
    int numHandoverCommandSent;
    int numHandoverCompleteRcvd;

}CellularAbstractLayer3ScStats;
// /**
// STRUCT      :: CellularAbstractLayer3ScInfo
// DESCRIPTION :: data structure for switch center
//                This is the main data structure at a switch center
// **/
typedef struct struct_cellular_abstract_layer3_sc_info_str
{
    NodeAddress scNodeId;
    NodeAddress scNodeAddress;

    //BSs linked to this MSC
    int numScControlBs;
    CellularAbstractScControlBsInfo
        scControlBsInfo[CELLULAR_ABSTRACT_MAX_BS_PER_SC];

    //virtual circuits info,
    //this circuit is used to assign to the BS when needed
    int numCircuitAssigned;//channels assigned to this BS
    int numCircuitInUse;   //channel in use
    CellularAbstractCircuitDescription
        scCircuitBank[CELLULAR_ABSTRACT_MAX_VIRTUAL_CIRCUIT_PER_SC];

    //gatewya link to this SC
    //we assume only one gateway is connected to this SC
    NodeAddress gatewayNodeId;
    NodeAddress gatewayNodeAddress;

    //application info at SC
    CellularAbstractApplicationInfoAtSc
        scAppInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_SC];

    //paging info
    CellularAbstractScPagingInfo
        scpagingInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_SC];

    //TODO: VLR could be a independent node,but here,
    //we assume VLR is with SC, the same to AuC and HLR
    // VLR data base associate with this SC
    CellularAbstractVlrEntry scVlr[CELLULAR_ABSTRACT_MAX_MS_PER_SC];

    //HLR data base associate with this SC,
    //it is necessary for inter SC handover and application
    //HLR node id
    NodeAddress associatedHLRNodeId;

    //TODO: AuC dat base associated with this SC

    //statistics
    CellularAbstractLayer3ScStats stats;
}CellularAbstractLayer3ScInfo;

//*************************************************************
//   Block: data definition for Aggregated Node
//*************************************************************

// /**
// STRUCT      :: CellularAbstractLayer3ScInfo
// DESCRIPTION :: data structure for switch center
//                This is the main data structure at a switch center
// **/
typedef
struct struct_cellular_abstract_application_info_at_aggregated_node_str
{

    BOOL inUse;

    //general info
    int transactionId;
    int appId;
    CellularAbstractApplicationType applicationType;
    clocktype callStartTime;
    clocktype callEndTime;
    double bandwidthRequired;
    int numChannelRequired;
    double dataRateRequiremnt;
    double delayBound;
    NodeAddress srcMsNodeId;//we dod not distigush ms and aggregated
    NodeAddress destMsNodeId;

    NodeAddress msNodeId;//this App Info is created for the src or dest
    CellularAbstractCallSrcDestType callSrcDestType;
    unsigned char    msRrState;
    unsigned char    msMmState;
    CellulaAbstractMsCcState    msCcState;

    //resource alloated
    BOOL   isDedicatedChannelAssigned;
    double bandwidthAllocated;
}CellularAbstractApplicationInfoAtAggregatedNode;

// /**
// STRUCT      :: CellularAbstractLayer3AggregatedNodeStats
// DESCRIPTION :: statistics for aggregated node
// **/
typedef struct struct_cellular_abstract_layer3_aggregated_node_stats_str
{
    int numFTCallSetupRcvd;
    int numFTCallConfirmRcvd;
    int numFTCallAlertingSent;
    int numFTCallConnectSent;
    int numFTCallDisconnectByMsSent;
    int numFTCallDisconnectByNwRcvd;

    int numFOCallSetupSent;
    int numFOCallProceedingRcvd;
    int numFOCallAlertingRcvd;
    int numFOCallConnectRcvd;
    int numFOCallDisconnectByMsSent;
    int numFOCallDisconnectByNwRcvd;

    //application
    int numVoiceCallSetupInit;
    int numVoiceCallSetupRcvd;
    int numDataCallActivationSent;
    int numDataCallActivationRcvd;

    int numCallStartRcvd;
    int numCallEndRcvd;
    int numCallRejectSent;
    int numCallDroppedSent;
}CellularAbstractLayer3AggregatedNodeStats;

// /**
// STRUCT      :: CellularAbstractLayer3AggregatedNodeInfo
// DESCRIPTION :: Data structure for aggregated node information
//                This is the main data structure for aggregated node
// **/
typedef struct struct_cellular_abstract_layer3_aggregated_node_info_str
{
    NodeAddress aggregatedNodeId;
    NodeAddress aggregatedNodeAddress;
    //gateway information
    NodeAddress gatewayNodeId;
    NodeAddress gatewayNodeAddress;
     //Transactions
    BOOL isTransactionInProcess;
    int numTransactions;
    //Application (services)info
    int numActiveApplicaions;
CellularAbstractApplicationInfoAtAggregatedNode
 aggregatedNodeAppInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_AGGREGATEDNODE];
    //statistics
    CellularAbstractLayer3AggregatedNodeStats stats;
}CellularAbstractLayer3AggregatedNodeInfo;

///*************************************************************
//   Block: data definition for Gateway
//**************************************************************
// /**
// STRUCT      :: CellularAbstractScInfo
// DESCRIPTION :: Data structure gateway to keep
//                the information of switch centers
// **/
typedef struct struct_cellular_abstract_sc_info_str
{
    NodeAddress scNodeId;
    NodeAddress scNodeAddress;
    int lac[CELLULAR_ABSTRACT_MAX_LAC_PER_SC];
}CellularAbstractScInfo;

// /**
// STRUCT      :: CellularAbstractHlrEntry
// DESCRIPTION :: Data structure for an entry in HLR
// **/
typedef struct struct_cellular_abstract_hlr_entry_str
{
    BOOL    inUse;

    NodeAddress msNodeId;
    NodeAddress msNodeAddress;
    NodeAddress scNodeId;
    NodeAddress scNodeAddress;

    //location information of the MS
    //NodeAddress bsNodeId;
    //NodeAddress bsNodeAddress;
    int   lac;
    // int   cellId;

    // Time of creation of this entry/record in the VLR
    clocktype lastUpdated;
    CellularAbstractMsActiveStatus msActiveStatus;
}CellularAbstractHlrEntry;

// /**
// STRUCT      :: CellularAbstractLayer3GatewayStats
// DESCRIPTION :: Statistics of gateway
// **/
typedef struct struct_cellular_abstract_layer3_gateway_stats_str
{
    int numMapdUpdateLocationRcvd;
    int numMapdUpdateLocationResultSent;
    int numMapdCancelLocationSent;
    int numMapdCancelLocationResultRcvd;

    int numMOFTVoiceCall;
    int numInterScMOMTCall;
    int numFOMTVoiceCall;

    int numMOFTDataCall;
    int numInterScMOMTDataCall;
    int numFOMTDataCall;

}CellularAbstractLayer3GatewayStats;
// /**
// STRUCT      :: CellularAbstractLayer3GatewayInfo
// DESCRIPTION :: Data structure of gateway
//                This is the main structure of gateway
// **/
typedef struct struct_cellular_abstract_layer3_gateway_info_str
{
    NodeAddress gatewayNodeId;
    NodeAddress gatewayNodeAddress;

    //SC connect to this gateway
    int numSc;
    CellularAbstractScInfo
        gatewayConnectSc[CELLULAR_ABSTRACT_MAX_SC_PER_GATEWAY];
    //HLR
    CellularAbstractHlrEntry hlr[CELLULAR_ABSTRACT_MAX_MS_PER_HLR];

    //aggregated node info
    NodeAddress aggregatedNodeId;
    NodeAddress aggregatedNodeAddress;

    //statistics
    CellularAbstractLayer3GatewayStats stats;
}CellularAbstractLayer3GatewayInfo;

///*****************************************************************
//the main data structure for cellular abstract implementation
///****************************************************************//

// /**
// STRUCT      :: CellularAbstractLayer3Data
// DESCRIPTION :: Structure of the layer 3 data for a cellualr abstract node
//                This is the main data structure of abstract cellular model
// **/
typedef struct struct_cellular_abstract_layer3_str
{
    // Each node has only one of the following based on its type
    CellularAbstractLayer3MsInfo    *msLayer3Info;    // Abstract_MS
    CellularAbstractLayer3BsInfo     *bsLayer3Info;     // Abstract_BS
    CellularAbstractLayer3ScInfo      *scLayer3Info;     // Abstract_SC
    CellularAbstractLayer3GatewayInfo *gatewayLayer3Info; //Abstract gateway

    //Abstract aggregated node
    CellularAbstractLayer3AggregatedNodeInfo *aggregatedNodeLayer3Info;

    //optimization
    short optLevel;
    double movThreshold;
}CellularAbstractLayer3Data;

//**************************************************************************
//Definition of the Info associate with Timers
//**************************************************************************
// /**
// STRUCT      :: CellularAbstractGenericTimerInfo
// DESCRIPTION :: Structure of the info field for the timer send to
//                the network layer itself
//                be sure to put timerType as the first variable
// **/
typedef struct struct_cellular_abstract_generic_timer_info_str
{
    CellularAbstractNetworkTimerType timerType;

    //the node who generate this transaactionId,
    //for ms it isitself,for sc it is the MO(MT)'ms
    NodeAddress srcNodeId;

    //this is useful to distigush the channel request
    //timer for differnt purpose
   //for location update it using 0.
   //for others transaction id can be set to the application's tranaction id
    int transactionId;
}CellularAbstractGenericTimerInfo;

// /**
// STRUCT      :: CellularAbstractMsChannelRequestTimerInfo
// DESCRIPTION :: Structure of the info field for the
//                timer associated with channel request
// **/
typedef struct struct_cellular_abstract_ms_channel_request_timer_info_str
{

    CellularAbstractNetworkTimerType timerType;
    int transactionId;
    NodeAddress srcNodeId;
}CellularAbstractMsChannelRequestTimerInfo;

// /**
// STRUCT      :: CellularAbstractBsSystemTimerInfo
// DESCRIPTION :: Structure of the info field for the timer associated
//                with BS system infomation broadcast timer
// **/
typedef struct struct_cellular_abstract_bs_system_timer_info_str
{
    CellularAbstractNetworkTimerType timerType;
    int transactionId;
}CellularAbstractBsSystemTimerInfo;

// /**
// STRUCT      :: CellularAbstractBsSystemTimerInfo
// DESCRIPTION :: Structure of the info field for the
//                timer associated with immediate assignment timer
// **/
typedef
struct struct_cellular_abstract_bs_immediate_assignment_timer_info_str
{

    CellularAbstractNetworkTimerType timerType;

    int assignmentId;
    int transactionId;
    NodeAddress srcNodeId;

    int sectorId;
    CellularEstCause estCause;
    CellularAbstractChannelRequestType channelRequestType;
    int numChannelRequired;
    double bandwidthAllocated;

    int channelAllocated[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
}CellularAbstractBsImmediateAssignmentTimerInfo;

// /**
// STRUCT      :: CellularAbstractBsTimer3113Info
// DESCRIPTION :: Structure of the info field for the
//                timer associated with page request
// **/
typedef struct struct_cellular_abstract_bs_timer_3113_info_str
{
    CellularAbstractNetworkTimerType timerType;
    int pageRequestId;
}CellularAbstractBsTimer3113Info;

// /**
// STRUCT      :: CellularAbstractTimer308Info
// DESCRIPTION :: Structure of the info field for the timer T308
// **/
typedef struct struct_cellular_abstract_timer_308_info_str
{

    CellularAbstractNetworkTimerType timerType;

    //the node who generate this transaactionId,
    //for ms it isitself,for sc it is the MO(MT)'ms
    NodeAddress srcNodeId;
    int transactionId;
    int numExpiration;
}CellularAbstractTimer308Info;

// /**
// STRUCT      :: CellularAbstractPagingTimerInfo
// DESCRIPTION :: Structure of the info field for the timer associate
//                with paging msg at SC
// **/
typedef struct struct_cellular_abstract_paging_timer_info_str
{
    CellularAbstractNetworkTimerType timerType;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractApplicationType appType;
}CellularAbstractPagingTimerInfo;

// /**
// STRUCT      :: CellularAbstractDeactivationTimerInfo
// DESCRIPTION :: Structure of the info field for the deactivation timer
// **/
typedef struct struct_cellular_abstract_deactivation_timer_info_str
{
    CellularAbstractNetworkTimerType timerType;

    //the node who generate this transaactionId,
    //for ms it isitself,for sc it is the MO(MT)'ms
    NodeAddress srcNodeId;
    int transactionId;
    CellularAbstractCallDisconectCauseType callDiscCause;
}CellularAbstractDeactivationTimerInfo;
//*********************************************************************
//The protocol packet(message) format at layer 3
//Note: The BsSystemInfo packet has been move to cellular_abstract.h
//      padInfo is added to make 32bit and 64bit result compatible
//*********************************************************************

// /**
// STRUCT      :: CellularAbstractMapdUpdateLocationPkt
// DESCRIPTION :: Structure of the Update location packet between VLR and HLR
// **/
typedef struct struct_cellular_abstract_mapd_update_location_pkt_str
{
    NodeAddress                     msNodeId;
    NodeAddress                     msNodeAddr;
    NodeAddress                     scNodeId;
    CellularAbstractLocationUpdatingType   locationUpdatingType;
    int                           lac;
    int padinfo;
}CellularAbstractMapdUpdateLocationPkt;
// /**
// STRUCT      :: CellularAbstractMapdCancelLocationPkt
// DESCRIPTION :: Structure of the Cancel location packet between VLR and HLR
// **/
typedef struct struct_cellular_abstract_mapd_cancel_location_pkt_str
{
    NodeAddress                     msNodeId;
    NodeAddress                     msNodeAddr;
    NodeAddress                     scNodeId;
    int                             lac;
    int                             cancelCause; //currently unused
    int padInfo;
}CellularAbstractMapdCancelLocationPkt;
// /**
// STRUCT      :: CellularAbstractChannelRequestPkt
// DESCRIPTION :: Structure of the channel request packet
// **/
typedef struct struct_cellular_abstract_channel_request_packet_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddr;
    int sectorId;
    CellularEstCause estCause;
    CellularAbstractChannelRequestType channelRequestType;
    int numChannelRequired;
    double bandwidthRequired;
    int rand_disc;
    int transactionId; //the channel request is for transaction #
    int pageReqId; //only used when it is for answer to page
    int padInfo;
}CellularAbstractChannelRequestPkt;

// /**
// STRUCT      :: CellularAbstractImmediateAssignmentPkt
// DESCRIPTION :: Structure of the channel immediate assignment packet
// **/
typedef struct struct_cellular_abstract_immediate_assignment_packet_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;

    unsigned int bsNodeId;
    int sectorId;
    CellularEstCause estCause;
    CellularAbstractChannelRequestType channelRequestType;
    int numChannelRequired;
    int rand_disc;
    int transactionId;//the channel request is for transaction #
    int assignmentId;
    double bandwidthAllocated;
    int channelAllocated[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
    // CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP needs to be even number to keep
    // platform consistency of alignment
}CellularAbstractImmediateAssignmentPkt;

// /**
// STRUCT      :: CellularAbstractImmediateAssignmentRejectPkt
// DESCRIPTION :: Structure of the channel
//                immediate assignment reject packet
// **/
typedef
struct struct_cellular_abstract_immediate_assignment_reject_packet_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    unsigned int bsNodeId;
    int sectorId;
    CellularEstCause estCause;
    CellularAbstractChannelRequestType channelRequestType;
    int numChannelRequired;
    int rand_disc;
    double bandwidthAllocated;
    int transactionId;//the reject is for transaction #
    CellularAbstractCallRejectCauseType rejectCause; //reject cause;
    int channelAllocated[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
    // CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP needs to be even number to keep
    // platform consistency of alignment
}CellularAbstractImmediateAssignmentRejectPkt;
// /**
// STRUCT      :: CellularAbstractLocationUpdateRequestPkt
// DESCRIPTION :: Structure of the location update request
//                packet between MS and SC
// **/
typedef struct struct_cellular_abstract_location_update_request_pkt_str
{
    CellularAbstractLocationUpdatingType   locationUpdatingType;
    int   lac;

    // mobile station classmark
    //unsigned char imsi[CELLUAR_MAX_IMSI_LENGTH];
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddr;
    int assignmentId;
    int padInfo;
}CellularAbstractLocationUpdateRequestPkt;

// /**
// STRUCT      :: CellularAbstractLocationUpdateAcceptPkt
// DESCRIPTION :: Structure of the location update accept
//                acket between MS and SC
// **/
typedef struct struct_cellular_abstract_location_update_accept_pkt_str
{
    CellularAbstractLocationUpdatingType   locationUpdatingType;
    int   lac;

    // mobile station classmark
    //unsigned char imsi[CELLUAR_MAX_IMSI_LENGTH];
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
}CellularAbstractLocationUpdateAcceptPkt;
// /**
// STRUCT      :: CellularAbstractLocationUpdateRejectPkt
// DESCRIPTION :: Structure of the location update reject
//                packet between MS and SC
// **/
typedef struct struct_cellular_abstract_location_update_reject_pkt_str
{
    CellularAbstractLocationUpdatingType   locationUpdatingType;
    int   lac;

    // mobile station classmark
    //unsigned char imsi[CELLUAR_MAX_IMSI_LENGTH];
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    //reject Cause
}CellularAbstractLocationUpdateRejectPkt;

// /**
// STRUCT      :: CellularAbstractClearCommandPkt
// DESCRIPTION :: Structure of the clear command packet from SC
// **/
typedef struct struct_cellular_abstract_clear_command_pkt_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    int transactionId;
    CellularAbstractRRCause rrCause;
}CellularAbstractClearCommandPkt;

// /**
// STRUCT      :: CellularAbstractChannelReleasePkt
// DESCRIPTION :: Structure of the channel release packet from BS
// **/
typedef struct struct_cellular_abstract_channel_release_pkt_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    int transactionId;
    CellularAbstractRRCause rrCause;
}CellularAbstractChannelReleasePkt;

// /**
// STRUCT      :: CellularAbstractCMServiceRequestPkt
// DESCRIPTION :: Structure of the CM service request packet from MS
// **/
typedef struct struct_cellular_abstract_CM_service_request_pkt_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    NodeAddress bsNodeId;
    NodeAddress bsNodeAddr;
    int transactionId;
    CellularAbstractCMServiceType cmServiceType;

    // resource assignemnt id from BS, indicating the access of the resource
    int assignmentId;
    int padInfo;
}CellularAbstractCMServiceRequestPkt;

// /**
// STRUCT      :: CellularAbstractCMServiceAcceptPkt
// DESCRIPTION :: Structure of the CM service accept packet from SC
// **/
typedef struct struct_cellular_abstract_CM_service_accept_pkt_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    int transactionId;
    CellularAbstractCMServiceType cmServiceType;
}CellularAbstractCMServiceAcceptPkt;

// /**
// STRUCT      :: CellularAbstractCMServiceRejectPkt
// DESCRIPTION :: Structure of the CM service reject packet from SC
// **/
typedef struct struct_cellular_abstract_CM_service_reject_pkt_str
{
    NodeAddress msNodeId;
    NodeAddress msNodeAddr;
    int transactionId;
    CellularAbstractCMServiceType cmServiceType;
    CellularAbstractCallRejectCauseType rejectCause;
    int padInfo;
}CellularAbstractCMServiceRejectPkt;

// /**
// STRUCT      :: CellularAbstractCallSetupPkt
// DESCRIPTION :: Structure of the Call setup packet from MS
// **/
typedef struct struct_cellular_abstract_call_setup_pkt_str
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    int numChannelRequired;
    NodeAddress appSrcScNodeId;
    double bandwidthRequired;
}CellularAbstractCallSetupPkt;

// /**
// STRUCT      :: CellularAbstractCallSetupIndicationPkt
// DESCRIPTION :: Structure of the Call setup Indication
//                packet between SC and Gateway
// **/
typedef CellularAbstractCallSetupPkt CellularAbstractCallSetupIndicationPkt;

// /**
// STRUCT      :: CellularAbstractCallProceedingPkt
// DESCRIPTION :: Structure of the Call Proceeding packet
//                between SC and Gateway
// **/
typedef struct struct_cellular_abstract_call_proceeding_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
}CellularAbstractCallProceedingPkt;

// /**
// STRUCT      :: CellularAbstractPagingPkt
// DESCRIPTION :: Structure of the paging packet between SC and BS
// **/
typedef struct struct_cellular_abstract_paging_pkt_str
{
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    double bandwidthRequired;
    clocktype appDuration;
}CellularAbstractPagingPkt;

// /**
// STRUCT      :: CellularAbstractPageRequestPkt
// DESCRIPTION :: Structure of the page request packet between BS and MS
// **/
typedef struct struct_cellular_abstract_page_request_pkt_str
{

    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    int pageRequestId;     //page id
    NodeAddress bsNodeId;
    double bandwidthRequired;
    clocktype appDuration;
}CellularAbstractPageRequestPkt;

// /**
// STRUCT      :: CellularAbstractPageResponsePkt
// DESCRIPTION :: Structure of the page response packet between BS and MS
// **/
typedef struct struct_cellular_abstract_page_response_pkt_str
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    int pageRequestId;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    int assignmentId;
    int padInfo;
    double bandwidthRequired;
    clocktype appDuration;

}CellularAbstractPageResponsePkt;

// /**
// STRUCT      :: CellularAbstractCallConfirmPkt
// DESCRIPTION :: Structure of the call confirm packet
// **/
typedef struct struct_cellular_abstract_call_confirm_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    int padIfno;
}CellularAbstractCallConfirmPkt;

// /**
// STRUCT      :: CellularAbstractCallAlertingPkt
// DESCRIPTION :: Structure of the call alerting packet
// **/
typedef struct struct_cellular_abstract_call_allerting_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    int padInfo;
}CellularAbstractCallAlertingPkt;

// /**
// STRUCT      :: CellularAbstractCallAlertingIndicationPkt
// DESCRIPTION :: Structure of the call alerting indication packet
// **/
typedef CellularAbstractCallAlertingPkt
    CellularAbstractCallAlertingIndicationPkt;

// /**
// STRUCT      :: CellularAbstractCallConnectPkt
// DESCRIPTION :: Structure of the call connect packet
// **/
typedef struct struct_cellular_abstract_call_connect_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    int padInfo;
}CellularAbstractCallConnectPkt;

// /**
// STRUCT      :: CellularAbstractCallConnectIndicationPkt
// DESCRIPTION :: Structure of the call connect indication packet
// **/
typedef CellularAbstractCallConnectPkt
    CellularAbstractCallConnectIndicationPkt;

// /**
// STRUCT      :: CellularAbstractCallConnectAckPkt
// DESCRIPTION :: Structure of the call connect indication packet
// **/
typedef struct struct_cellular_abstract_call_connect_ack_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    int padInfo;
}CellularAbstractCallConnectAckPkt;

// /**
// STRUCT      :: CellularAbstractCallDisconnectPkt
// DESCRIPTION :: Structure of the call disconnect packet
// **/
typedef struct struct_cellular_abstract_call_disconnect_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    CellularAbstractCallDisconnectType callDisconnectType;
    CellularAbstractCallDisconectCauseType callDiscCause;
    int padInfo;
}CellularAbstractCallDisconnectPkt;

// /**
// STRUCT      :: CellularAbstractCallDisconnectIndicationPkt
// DESCRIPTION :: Structure of the call disconnect indication
//                packet between SC and gateway
// **/
typedef CellularAbstractCallDisconnectPkt
    CellularAbstractCallDisconnectIndicationPkt;

// /**
// STRUCT      :: CellularAbstractCallReleasePkt
// DESCRIPTION :: Structure of the call release packet
// **/
typedef struct struct_cellular_abstract_call_release_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    CellularAbstractCallReleaseType callReleaseType;
    //release cause 1
    //release cause 2
}CellularAbstractCallReleasePkt;

// /**
// STRUCT      :: CellularAbstractCallReleaseCompletePkt
// DESCRIPTION :: Structure of the call release complete packet
// **/
typedef struct struct_cellular_abstract_call_release_complete_pkt_str
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress srcBsNodeId;
    NodeAddress destBsNodeId;
    CellularAbstractCallReleaseCompleteType callReleaseCompleteType;
}CellularAbstractCallReleaseCompletePkt;

// /**
// STRUCT      :: CellularAbstractActivatePDPContextRequestPkt
// DESCRIPTION :: Structure of the Activeate PDP context request packet
// **/
typedef struct struct_cellular_abstract_activate_pdp_context_request
{
    double bandwidthRequired;
    clocktype appDuration;
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    CellularAbstractApplicationType appType;
    CellularAbstractCallSrcDestType callSrcDestType;
    int numChannelRequired;
    int padInfo;

}CellularAbstractActivatePDPContextRequestPkt;

// /**
// STRUCT      :: CellularAbstractPDPContextIndicationPkt
// DESCRIPTION :: Structure of the activeate PDP context
//                request indication packet
// **/
typedef CellularAbstractActivatePDPContextRequestPkt
                            CellularAbstractPDPContextIndicationPkt;

// /**
// STRUCT      :: CellularAbstractActivatePDPContextAcceptPkt
// DESCRIPTION :: Structure of the activeate PDP context accept packet
// **/
typedef struct struct_cellular_abstract_activate_pdp_context_accept
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    int padInfo;
    double bandwidthRequired;
    clocktype appDuration;
}CellularAbstractActivatePDPContextAcceptPkt;

// /**
// STRUCT      :: CellularAbstractRequestPDPContextActivationPkt
// DESCRIPTION :: Structure of the request PDP context activation packet
// **/
typedef struct struct_cellular_abstract_request_pdp_context_activation
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    int padInfo;
    clocktype appDuration;
    double bandwidthRequired;
}CellularAbstractRequestPDPContextActivationPkt;

// /**
// STRUCT      :: CellularAbstractDeactivatePDPContextRequestPkt
// DESCRIPTION :: Structure of the deactivate PDP context reuest packet
// **/
typedef struct struct_cellular_abstract_deactivate_pdp_context_request
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    CellularAbstractCallSrcDestType callSrcDestType;
    CellularAbstractCallDisconectCauseType callDiscCause;
    clocktype appDuration;
    double bandwidthRequired;
}CellularAbstractDeactivatePDPContextRequestPkt;

// /**
// STRUCT      :: CellularAbstractDeactivatePDPContextAcceptPkt
// DESCRIPTION :: Structure of the deactivate PDP context accept packet
// **/
typedef struct struct_cellular_abstract_deactivate_pdp_context_accept
{
    int transactionId;
    int appId;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    CellularAbstractApplicationType appType;
    CellularAbstractCallSrcDestType callSrcDestType;
    CellularAbstractCallDisconectCauseType callDiscCause;
    double bandwidthRequired;
    clocktype appDuration;
 }CellularAbstractDeactivatePDPContextAcceptPkt;

// /**
// STRUCT      :: CellularAbstractPDPFirstPacket
// DESCRIPTION :: Structure of the PDP first packet
// **/
typedef struct struct_cellular_abstract_PDP_first_packet_str
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    int padInfo;
    double bandwidthRequired;
    clocktype appDuration;
}CellularAbstractPDPFirstPacket;

// /**
// STRUCT      :: CellularAbstractPDPLastPacket
// DESCRIPTION :: Structure of the PDP last packet
// **/
typedef struct struct_cellular_abstract_PDP_last_packet_str
{
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress appSrcNodeId;
    NodeAddress srcBsNodeId;
    NodeAddress appDestNodeId;
    NodeAddress destBsNodeId;
    int numChannelRequired;
    int padInfo;
    double bandwidthRequired;
    clocktype appDuration;
}CellularAbstractPDPLastPacket;
// /**
// STRUCT      :: CellularAbstractMTCallRejectIndication
// DESCRIPTION :: Structure of MT call rejection indication
// **/
typedef struct struct_cellular_abstract_mt_call_reject_indication
{
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    CellularAbstractCallSrcDestType callSrcDestType;
    CellularAbstractCallRejectCauseType rejectCause;
}CellularAbstractMTCallRejectIndication;

// /**
// STRUCT      :: CellularAbstractHandoverAppInfo
// DESCRIPTION :: Structure of app Info in handover
// **/
typedef struct struct_cellular_abstract_handover_app_info
{
    double bandwidthRequired;
    double bandwidthAllocated;
    int transactionId;
    int appId;
    CellularAbstractApplicationType appType;
    CellularAbstractCallSrcDestType callSrcDestType;
    NodeAddress appSrcNodeId;
    NodeAddress appDestNodeId;
    int numChannelRequired;
    BOOL isHandoverAllowed;
    int channelAllocated[CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP];
    // CELLULAR_ABSTRACT_MAX_CHANNEL_ALLOWED_PER_APP needs to be even number to keep
    // platform consistency of alignment
}CellularAbstractHandoverAppInfo;

//BEWARE: ALL THE HANDOVER ACKET ARE DEFINED as the same structure for simplicity
// /**
// STRUCT      :: CellularAbstractHandoverRequestPkt
// DESCRIPTION :: Structure of handover request packet
//                between MS and BS and BS and SC
// **/
typedef struct struct_cellular_abstract_handover_request_pkt
{
    NodeAddress msNodeId;
    NodeAddress currentBsNodeId;
    NodeAddress targetBsNodeId;
    int currentSectorId;
    int targetSectorId;
    int numApplication;
    CellularAbstractHandoverType handoverType;
    int padInfo;
    CellularAbstractHandoverAppInfo
        handoverAppInfo[CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_MS];
    // CELLULAR_ABSTRACT_MAX_ACTIVE_APP_PER_MS needs to be even number to keep
    // platform consistency of alignment
}CellularAbstractHandoverRequestPkt;

// /**
// STRUCT      :: CellularAbstractHandoverRequiredPkt
// DESCRIPTION :: Structure of handover required packet between SC and BC
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverRequiredPkt;

// /**
// STRUCT      :: CellularAbstractHandoverCommandPkt
// DESCRIPTION :: Structure of handover command packet between BS and MS
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverCommandPkt;

// /**
// STRUCT      :: CellularAbstractHandoverCompletePkt
// DESCRIPTION :: Structure of handover complete packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverCompletePkt;

// /**
// STRUCT      :: CellularAbstractHandoverRequestAckPkt
// DESCRIPTION :: Structure of handover request ack packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverRequestAckPkt;

// /**
// STRUCT      :: CellularAbstractHandoverFailurePkt
// DESCRIPTION :: Structure of handover failure packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverFailurePkt;

// /**
// STRUCT      :: CellularAbstractHandoverRequiredRejectPkt
// DESCRIPTION :: Structure of handover required reject packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverRequiredRejectPkt;

// /**
// STRUCT      :: CellularAbstractHandoverClearCommnadPkt
// DESCRIPTION :: Structure of handover clear command packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverClearCommnadPkt;
// /**
// STRUCT      :: CellularAbstractHandoverClearCompletePkt
// DESCRIPTION :: Structure of handover clear complete packet
// **/
typedef CellularAbstractHandoverRequestPkt
            CellularAbstractHandoverClearCompletePkt;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: CellularAbstractLayer3PreInit
// LAYER      :: Layer3
// PURPOSE    :: Preinitialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3PreInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3Init
// LAYER      :: Layer3
// PURPOSE    :: Initialize Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3Init(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3MsInit
// LAYER      :: Layer3
// PURPOSE    :: Initialize MS's Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3MsInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3BsInit
// LAYER      :: Layer3
// PURPOSE    :: Initialize BS's Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3BsInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3ScInit
// LAYER      :: Layer3
// PURPOSE    :: Initialize SC's Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3ScInit(Node *node, const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3GatewayInit
// LAYER      :: Layer3
// PURPOSE    :: Initialize Gateway's Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3GatewayInit(
                                       Node *node,
                                       const NodeInput *nodeInput);
// /**
// FUNCTION   :: CellularAbstractLayer3AggregatedNodeInit
// LAYER      :: Layer3
// PURPOSE    :: Initialize AggregatedNode's Cellular Layer protocol
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3AggregatedNodeInit(
                                              Node *node,
                                              const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractLayer3MsFinalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3MsFinalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3BsFinalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3BsFinalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3ScFinalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3ScFinalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3GatewayFinalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3GatewayFinalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3AggregatedNodeFinalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3AggregatedNodeFinalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3Finalize
// LAYER      :: Layer3
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3Finalize(Node *node);

// /**
// FUNCTION   :: CellularAbstractLayer3Layer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3Layer(Node *node, Message *msg);

// /**
// FUNCTION   :: CellularAbstractReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// + interfaceIndex   : int    : Interface from which the packet is received
// RETURN     :: void : NULL
// **/
void CellularAbstractReceivePacketFromMacLayer(Node *node,
                                               Message *msg,
                                               NodeAddress lastHopAddress,
                                               int interfaceIndex);

// /**
// FUNCTION   :: CellularAbstractLayer3ReceivePacketOverIp
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret
// + sourceAddress    : Address       : Message from node
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3ReceivePacketOverIp(
                                               Node *node,
                                               Message *msg,
                                               Address sourceAddress);

// /**
// FUNCTION   :: CellularAbstractLayer3Callback
// LAYER      :: Layer3
// PURPOSE    :: Callback function for mobility layer to notify of movement
//               Added for optimization
// PARAMETERS ::
// + node             : Node*                       : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractLayer3Callback(
         Node* node);
#endif /* _CELLULAR_ABSTRACT_LAYER3_H_ */
