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
// PACKAGE     :: CELLULAR_ABSTRACT_APP
// DESCRIPTION :: Defines the data structures used
//                in the Cellular Abstract APP
//                Most of the time the data structures and functions start
//                with CellularAbstractApp**
// **/
#ifndef _CELLULAR_ABSTRACT_APP_H_
#define _CELLULAR_ABSTRACT_APP_H_
#include "cellular_abstract.h"
// /**
// CONSTANT    :: CELLULAR_ABSTRACT_APPLICATION_MAXIMUM_RETRY  :   5
// DESCRIPTION :: APPLICAITON MAXIMUM RETRY IF REJCTED OR FAILED
// **/
#define CELLULAR_ABSTRACT_APPLICATION_MAXIMUM_RETRY 5

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_DEFAULT_POWER_ON_TIME : (5 * SECOND)
//DESCRIPTION  :: Default power on duration after simulation start
// **/
#define  CELLULAR_ABSTRACT_DEFAULT_POWER_ON_TIME    (5 * SECOND)

// /**
// ENUM        :: Cellular_Abstract_App_Status
// DESCRIPTION :: Status of applicaiton
// **/
typedef enum
{
    CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED = 0,
    CELLULAR_ABSTRACT_APP_STATUS_ONGOING = 1,
    CELLULAR_ABSTRACT_APP_STATUS_REJECTED = 2,
    CELLULAR_ABSTRACT_APP_STATUS_FAILED = 3,
    CELLULAR_ABSTRACT_APP_STATUS_INITIATING = 4,
    CELLULAR_ABSTRACT_APP_STATUS_RETRYING = 5,
    CELLULAR_ABSTRACT_APP_STATUS_NEEDRETRY = 6,
    CELLULAR_ABSTRACT_APP_STATUS_PENDING = -1
}Cellular_Abstract_App_Status;

// /**
// ENUM        :: Cellular_Abstract_App_Timer_Type
// DESCRIPTION :: Type of Timer
// **/
typedef enum
{
    CELLULAR_ABSTRACT_APP_CALL_START_TIMER = 0,
    CELLULAR_ABSTRACT_APP_CALL_END_TIMER = 1,
    CELLULAR_ABSTRACT_APP_CALL_RETRY_TIMER = 2,
}Cellular_Abstract_App_Timer_Type;

// /**
// STRUCT      :: CellularAbstractApplicationInfo
// DESCRIPTION :: Definition of the data structure
//                for the abstract cellular App
// **/
typedef struct struct_cellular_abstract_application_info_str
{
    int appId;
    CellularAbstractApplicationType appType;
    NodeAddress appScrNodeId;
    NodeAddress appDestNodeId;
    clocktype appStartTime;
    clocktype appDuration;
    clocktype appSetupTime;
    clocktype appEndTime;
    short appNumChannelReq;
    double appBandwidthReq;
    BOOL appRetryAllowed;
    int appNumRetry;
    int appMaxRetry;
    Cellular_Abstract_App_Status  appStatus;
    Message *startCallTimer;
    Message *endCallTimer;
    Message *retryCallTimer;
    struct_cellular_abstract_application_info_str *next;
    // in the future implementation, more applications could
    // be possible including FTP,CBR,VBR and so on
}CellularAbstractApplicationInfo;

// /**
// STRUCT      :: CellularAbstractAppTimerInfo
// DESCRIPTION :: Info struncture for app timer
// **/
typedef struct struct_cellular_abstract_application_timer_info_str
{
    int appId;
    Cellular_Abstract_App_Timer_Type timerType;
}CellularAbstractAppTimerInfo;

// /**
// STRUCT      :: CellularAbstractApplicationStats
// DESCRIPTION :: stats for app
// **/
typedef struct struct_cellular_abstract_application_stat_str

{
    int numAppRequestSent; //only for src
    int numAppRequestRcvd;//only for dest

    //only for src MS
    int numAppAcceptedByNetwork;
    int numAppRejectedByNetwork;
    int numAppRejectedCauseSystemBusy;
    int numAppRejectedCauseUserBusy;
    int numAppRejectedCauseTooManyActiveApp;
    int numAppRejectedCauseUserUnreachable;
    int numAppRejectedCauseUserPowerOff;
    int numAppRejectedCauseNetworkNotFound;
    int numAppRejectedCauseUnsupportService;
    int numAppRejectedCauseUnknowUser;
    int numAppRetry;

    //for both
    int numAppSuccessEnd;
    int numOriginAppSuccessEnd;
    int numTerminateAppSuccessEnd;

    //for both
    int numAppDropped;
    int numOriginAppDropped;
    int numOriginAppDroppedCauseHandoverFailure;
    int numOriginAppDroppedCauseSelfPowerOff;
    int numOriginAppDroppedCauseRemotePowerOff;
    int numTerminateAppDropped;
    int numTerminateAppDroppedCauseHandoverFailure;
    int numTerminateAppDroppedCauseSelfPowerOff;
    int numTerminateAppDroppedCauseRemotePowerOff;
}CellularAbstractApplicationStats;

// /**
// STRUCT      :: CellularAbstractApplicationLayerData
// DESCRIPTION :: data structure for abstract cellular app layer
// **/
typedef struct struct_cellular_abstract_application_layer_str
{
    int numOriginApp;
    int numTerminateApp;

    //application list

    //app originating from this node
    CellularAbstractApplicationInfo *appInfoOriginList;

    //app terminating to this node
    CellularAbstractApplicationInfo *appInfoTerminateList;

    //ms active status
    CellularAbstractMsActiveStatus msActiveStatus;

    //statistics
    CellularAbstractApplicationStats    stats;
    BOOL collectStatistics;    // whether collect satistics
}CellularAbstractApplicationLayerData;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: CellularAbstractAppInit
// LAYER      :: APP
// PURPOSE    :: Init an application.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void CellularAbstractAppInit(Node *node,
                             const NodeInput *nodeInput);

// /**
// FUNCTION   :: CellularAbstractAppFinalize
// LAYER      :: APP
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
void CellularAbstractAppFinalize(Node *node);

// /**
// FUNCTION   :: CellularLayer3Layer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void CellularAbstractAppLayer(Node *node, Message *msg);

// /**
// FUNCTION   :: CellularAbstractAppInsertList
// LAYER      :: APP
// PURPOSE    :: Insert the application info to the specified app List.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appList : CellularAbstractApplicationInfo ** :
//                 Originating app List or terminating app List
// + appId : int : id of the application.
// + appSrcNodeId : NodeAddress : Source address of the app
// + appDestNodeId : NodeAddress : Destination address of the app
// + appStartTime : clocktype : Start time of the app
// + appDuration : clocktype : Duration of the app
// + appType : CellularAbstractApplicationType : Type of application
// + appNumChannelReq : short :
//                         Number of channel required for the application
// + appBandwidthReq : double : Bandwidth requirement for the application
// + isAppSrc : BOOL : Originaitng or terminating application
// RETURN     :: void : NULL
// **/
//************************************************************************
void
CellularAbstractAppInsertList(Node *node,
                              CellularAbstractApplicationInfo **appList,
                              int appId,
                              NodeAddress appScrNodeId,
                              NodeAddress appDestNodeId,
                              clocktype appStartTime,
                              clocktype appDuration,
                              CellularAbstractApplicationType appType,
                              short appNumChannelReq,
                              double appBandwidthReq,
                              BOOL isAppSrc);
#endif /* _CELLULAR_ABSTRACT_APP_H_ */
