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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#include "app_cellular_abstract.h"
#include "cellular.h"
#include "cellular_abstract.h"

#include "user.h"

#define DEBUG_APP   0
//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractGetApp
// LAYER      :: APP
// PURPOSE    :: Retrive the application info from the app List.
// PARAMETERS ::
// + node       : Node* : Pointer to node.
// + appId      : int : id of the application.
// + appSrcNodeId  : NodeAddress : src node id, who generate this app
// + appList    : CellularAbstractApplicationInfo ** :
//                  Originating app List or terminating app List
// + appInfo    : CellularAbstractApplicationInfo ** :
//                  Pointer to hold the retrived infomation
// RETURN     :: BOOL : success or fail when looking for the app in the list
// **/
//************************************************************************
static BOOL
CellularAbstractGetApp(Node *node,
                       int appId,
                       NodeAddress appSrcNodeId,
                       CellularAbstractApplicationInfo **appList,
                       CellularAbstractApplicationInfo **appInfo)

{
    BOOL found=FALSE;
    CellularAbstractApplicationInfo *currentApp;

    currentApp = *appList;
    if (DEBUG_APP)
    {
        printf(
            "node %d: in Get APP Info\n",
            node->nodeId);
    }
    while (currentApp != NULL)
    {
        if (DEBUG_APP)
        {
            printf(
                "node %d: Get APP Info, appId %d,"
                "src %d,dest %d, channel %d type %d\n",
                node->nodeId, currentApp->appId,
                currentApp->appScrNodeId, currentApp->appDestNodeId,
                currentApp->appNumChannelReq, currentApp->appType);
        }
        if (currentApp->appId == appId &&
            currentApp->appScrNodeId == appSrcNodeId)
        {
            found = TRUE;
            *appInfo = currentApp;
            break;
        }
            currentApp = currentApp->next;
    }
    if (DEBUG_APP)
    {
        printf(
            "node %d: Get APP Info, success/fail= %d\n",
            node->nodeId,found);
    }
    return found;
}
//************************************************************************
// /**
// FUNCTION   :: CellularAbstractAppInsertList
// LAYER      :: APP
// PURPOSE    :: Insert the application info to the specified app List.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appList : CellularAbstractApplicationInfo ** :
//                  Originating app List or terminating app List
// + appId : int : id of the application.
// + appSrcNodeId : NodeAddress : Source address of the app
// + appDestNodeId : NodeAddress : Destination address of the app
// + appStartTime : clocktype : Start time of the app
// + appDuration : clocktype : Duration of the app
// + appType : CellularAbstractApplicationType : Type of application
// + appNumChannelReq : short : Number of channel required
//                                          for the application
// + appBandwidthReq : double : Bandwidth requirement for the application
// + isAppSrc : BOOL : Originaitng or terminating application
// RETURN     :: void : NULL
// **/
//************************************************************************
void
CellularAbstractAppInsertList(
                            Node *node,
                            CellularAbstractApplicationInfo **appList,
                            int appId,
                            NodeAddress appScrNodeId,
                            NodeAddress appDestNodeId,
                            clocktype appStartTime,
                            clocktype appDuration,
                            CellularAbstractApplicationType appType,
                            short appNumChannelReq,
                            double appBandwidthReq,
                            BOOL isAppSrc)
{
    CellularAbstractApplicationInfo *newApp;
    CellularAbstractApplicationInfo *currentApp;
    CellularAbstractApplicationLayerData *cellularAppLayerData;
    Message *callStartTimerMsg;
    CellularAbstractAppTimerInfo *info;

    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
        node->appData.appCellularAbstractVar;
    if (isAppSrc == TRUE)
    {
        (cellularAppLayerData->numOriginApp) ++;
    }
    else
    {
        (cellularAppLayerData->numTerminateApp) ++;
    }

    newApp = (CellularAbstractApplicationInfo *)
        MEM_malloc(sizeof(CellularAbstractApplicationInfo));
    memset(newApp,
           0,
           sizeof(CellularAbstractApplicationInfo));

    if (isAppSrc ==TRUE)
    {
        newApp->appId = appId + 1;//src app use local one
    }
    else
    {
        newApp->appId = appId;//dest app, use app src's
    }

    newApp->appScrNodeId = appScrNodeId;
    newApp->appDestNodeId = appDestNodeId;
    newApp->appStartTime = appStartTime;
    newApp->appDuration = appDuration;
    assert(appDuration >= 0);
    newApp->appEndTime = newApp->appStartTime + newApp->appDuration;
    newApp->appMaxRetry = CELLULAR_ABSTRACT_APPLICATION_MAXIMUM_RETRY;
    newApp->appNumChannelReq = appNumChannelReq;
    newApp->appBandwidthReq = appBandwidthReq;
    newApp->appType = appType;
    newApp->appStatus = CELLULAR_ABSTRACT_APP_STATUS_PENDING;
    newApp->next = NULL;

    newApp->startCallTimer = NULL;
    newApp->endCallTimer = NULL;
    newApp->retryCallTimer = NULL;


    if (*appList == NULL)
    {
       *appList = newApp;
    }
    else
    {   currentApp = *appList;
        while (currentApp->next != NULL)
            currentApp = currentApp->next;
        currentApp->next = newApp;
    }

    //schedule a timer for start app event
    //Setup Call Start Timer with info on dest & call end time
    if (isAppSrc == TRUE)
    {
        callStartTimerMsg = MESSAGE_Alloc(
                                          node,
                                          APP_LAYER,
                                          APP_CELLULAR_ABSTRACT,
                                          MSG_APP_TimerExpired);
        MESSAGE_InfoAlloc(
                          node,
                          callStartTimerMsg,
                          sizeof(CellularAbstractAppTimerInfo));

        info = (CellularAbstractAppTimerInfo*)
            MESSAGE_ReturnInfo(callStartTimerMsg);

        info->appId = newApp->appId;
        info->timerType = CELLULAR_ABSTRACT_APP_CALL_START_TIMER;

        MESSAGE_Send(node, callStartTimerMsg, newApp->appStartTime);

        if (DEBUG_APP)
        {
            char buf1[255];
            char buf2[255];
            TIME_PrintClockInSecond(appStartTime, buf1);
            TIME_PrintClockInSecond(appDuration, buf2);
            printf(
                "node %d: Cellular APP Add one new %d th app"
                "src %d dest %d app type  %d start %s duration %s"
                "#of channel %d bandwidth %f\n",
                node->nodeId, cellularAppLayerData->numOriginApp,
                appScrNodeId, appDestNodeId, appType, buf1,
                buf2, appNumChannelReq, appBandwidthReq);
        }
    }
}
//**************************************************************
// /**
// FUNCTION   :: CellularAbstractAppHandleCallStartTimer
// LAYER      :: APP
// PURPOSE    :: Handle call start timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the Callstart Msg
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void CellularAbstractAppHandleCallStartTimer(Node *node,Message *msg)
{
    Message *callStartMsgToNetwork;
    CellularAbstractAppTimerInfo *infoIn;
    int appId;
    Cellular_Abstract_App_Timer_Type timerType;
    CellularAbstractCallStartMessageInfo *infoOut;
    CellularAbstractApplicationLayerData *cellularAppLayerData;
    CellularAbstractApplicationInfo *appList;
    CellularAbstractApplicationInfo *retAppInfo;


    //get the appId associated with this timer event
    infoIn = (CellularAbstractAppTimerInfo*) MESSAGE_ReturnInfo(msg);
    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf(
            "node %d: handle appStartTimer start for the appId is %d\n",
            node->nodeId, appId);
    }

    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
                                node->appData.appCellularAbstractVar;
    appList = cellularAppLayerData->appInfoOriginList;

    if (CellularAbstractGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {

        callStartMsgToNetwork  = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    MSG_NETWORK_CELLULAR_FromAppStartCall);

        MESSAGE_InfoAlloc(
            node,
            callStartMsgToNetwork,
            sizeof(CellularAbstractCallStartMessageInfo));

        infoOut = (CellularAbstractCallStartMessageInfo *)
            MESSAGE_ReturnInfo(callStartMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appType = retAppInfo->appType;
        infoOut->appSrcNodeId = retAppInfo->appScrNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        infoOut->appNumChannelReq = retAppInfo->appNumChannelReq;
        infoOut->appBandwidthReq = retAppInfo->appBandwidthReq;
        infoOut->appDuration = retAppInfo->appDuration;
        assert(retAppInfo->appDuration >= 0);
        callStartMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;

        MESSAGE_Send(node, callStartMsgToNetwork, 0);

        //update the stats
        cellularAppLayerData->stats.numAppRequestSent ++;
        if (DEBUG_APP)
        {
            printf("node %d: Get app info appid %d"
                "src %d dest %d #CHs %d type %d\n",
                node->nodeId,infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId,
                infoOut->appNumChannelReq, infoOut->appType);
        }
        if (DEBUG_APP)
        {
            printf("node %d: appStartTimerProc end for the appId is %d\n",
                node->nodeId, appId);
        }

    }
    else
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(
            errorBuf,
            "node %d: cannot resovle the appId %d"
            "provide by the app layer\n",
            node->nodeId, appId);
        ERROR_Assert(FALSE, errorBuf);
    }
}
//*****************************************************************
// /**
// FUNCTION   :: CellularAbstractAppHandleCallEndTimer
// LAYER      :: APP
// PURPOSE    :: Handle Call end timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the CallEnd Msg
// RETURN     :: void : NULL
// **/
//****************************************************************
static
void CellularAbstractAppHandleCallEndTimer(Node *node,Message *msg)
{
    Message *callEndMsgToNetwork;
    CellularAbstractAppTimerInfo*infoIn;
    int appId;
    Cellular_Abstract_App_Timer_Type timerType;
    CellularAbstractCallEndMessageInfo *infoOut;
    CellularAbstractApplicationLayerData *cellularAppLayerData;
    CellularAbstractApplicationInfo *appList;
    CellularAbstractApplicationInfo *retAppInfo;


    //get the appId associated with this timer event
    infoIn = (CellularAbstractAppTimerInfo*) MESSAGE_ReturnInfo(msg);

    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf("node %d: handle appEndTimer start for the appId is %d\n",
            node->nodeId, appId);
    }

    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
                                node->appData.appCellularAbstractVar;
    appList = cellularAppLayerData->appInfoOriginList;

    if (CellularAbstractGetApp(
        node,appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {

        callEndMsgToNetwork = MESSAGE_Alloc(
                                node,
                                NETWORK_LAYER,
                                NETWORK_PROTOCOL_CELLULAR,
                                MSG_NETWORK_CELLULAR_FromAppEndCall);

        MESSAGE_InfoAlloc(
            node,
            callEndMsgToNetwork,
            sizeof(CellularAbstractCallEndMessageInfo));

        infoOut = (CellularAbstractCallEndMessageInfo *)
                        MESSAGE_ReturnInfo(callEndMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appType = retAppInfo->appType;
        infoOut->appSrcNodeId = retAppInfo->appScrNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        infoOut->appNumChannelReq = retAppInfo->appNumChannelReq;
        infoOut->appBandwidthReq = retAppInfo->appBandwidthReq;
        infoOut->appDuration = retAppInfo->appDuration;
        assert(retAppInfo->appDuration >= 0);

        callEndMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;
        MESSAGE_Send(node, callEndMsgToNetwork, 0);

        //update the stats
        cellularAppLayerData->stats.numAppSuccessEnd ++;
        cellularAppLayerData->stats.numOriginAppSuccessEnd ++;

        retAppInfo->appStatus = CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED;
        if (DEBUG_APP)
        {
            printf(
                "node %d: Get app info appid %d"
                " src %d dest %d #CHs %d type %d\n",
                node->nodeId,infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId,
                infoOut->appNumChannelReq, infoOut->appType);
        }
        if (DEBUG_APP)
        {
            printf(
                "node %d: appEndTimerProc end for the appId is %d\n",
                node->nodeId, appId);
        }

    }
    else
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(
            errorBuf,
            "node %d: cannot resovle the appId %d"
            " provide by the app layer\n",
            node->nodeId,appId);
        ERROR_Assert(FALSE, errorBuf);
    }

}
//**************************************************************
// /**
// FUNCTION   :: CellularAbstractAppHandleCallAcceptMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Accept msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//***************************************************************
static void
CellularAbstractAppHandleCallAcceptMsg(Node *node, Message *msg)
{
    int appId;
    CellularAbstractApplicationLayerData *cellularAppLayerData;
    CellularAbstractApplicationInfo *appList;
    CellularAbstractApplicationInfo *retAppInfo;
    Message *callEndTimerMsg;

    //clocktype callEndTime;
    CellularAbstractAppTimerInfo        *info;
    CellularAbstractCallAcceptMessageInfo *callAcceptInfo;

    callAcceptInfo =
        (CellularAbstractCallAcceptMessageInfo *) MESSAGE_ReturnInfo(msg);

    appId = callAcceptInfo->appId;
    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
                                node->appData.appCellularAbstractVar;
    appList = cellularAppLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: handle CallAcceptMsg\n",
            node->nodeId);
    }
    if (CellularAbstractGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        callEndTimerMsg = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_CELLULAR_ABSTRACT,
                            MSG_APP_TimerExpired);

        MESSAGE_InfoAlloc(
            node,
            callEndTimerMsg,
            sizeof(CellularAbstractAppTimerInfo));

        info = (CellularAbstractAppTimerInfo*)
                    MESSAGE_ReturnInfo(callEndTimerMsg);
        info->appId = appId;
        info->timerType = CELLULAR_ABSTRACT_APP_CALL_END_TIMER;

        retAppInfo->appEndTime =
            retAppInfo->appStartTime+retAppInfo->appDuration;
        assert(retAppInfo->appDuration >= 0);
        retAppInfo->appSetupTime =
            node->getNodeTime() - retAppInfo->appStartTime;
        retAppInfo->appStatus = CELLULAR_ABSTRACT_APP_STATUS_ONGOING;
        MESSAGE_Send(node, callEndTimerMsg, retAppInfo->appDuration);

        retAppInfo->endCallTimer = callEndTimerMsg;

        if (DEBUG_APP)
        {
            printf("node %d: scchedule a call end timer \n",node->nodeId);
        }
    }
    cellularAppLayerData->stats.numAppAcceptedByNetwork ++;
}
//**********************************************************************
// /**
// FUNCTION   :: CellularAbstractAppHandleCallDroppedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Dropped msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void CellularAbstractAppHandleCallDroppedMsg(Node *node, Message *msg)
{
    CellularAbstractCallDropMessageInfo *msgInfo;
    int appId;
    CellularAbstractCallDropCauseType callDropCause;
    CellularAbstractApplicationInfo *appList;
    CellularAbstractApplicationInfo *retAppInfo;

    CellularAbstractApplicationLayerData *cellularAppLayerData;

    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
                                node->appData.appCellularAbstractVar;

    msgInfo =
        (CellularAbstractCallDropMessageInfo *)MESSAGE_ReturnInfo(msg);

    appId = msgInfo->appId;

    callDropCause = msgInfo->dropCause;
    //update the application status to dropped
    if (node->nodeId == msgInfo->appSrcNodeId)
    {
        appList = cellularAppLayerData->appInfoOriginList;
        if (CellularAbstractGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
            if (retAppInfo->appStatus ==
                CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED)
            {
                //if call end msg is  sent before rcv this call drop msg
                //do nothing
            }
            else
            {
                retAppInfo->appStatus = CELLULAR_ABSTRACT_APP_STATUS_FAILED;
                if (retAppInfo->endCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->endCallTimer);

                    retAppInfo->endCallTimer = NULL;
                }
                if (callDropCause ==
                    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    cellularAppLayerData->
                        stats.numOriginAppDroppedCauseHandoverFailure ++;
                }
                else if (callDropCause ==
                    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    cellularAppLayerData->
                        stats.numOriginAppDroppedCauseSelfPowerOff ++;
                }
                else if (callDropCause ==
                    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    cellularAppLayerData->
                        stats.numOriginAppDroppedCauseRemotePowerOff ++;
                }

                //update stats
                cellularAppLayerData->stats.numAppDropped ++;
                cellularAppLayerData->stats.numOriginAppDropped ++;

                //TODO: schedule a retry timer if required
                //if the user layer is enabled, let it know of this event
                if (node->userData->enabled)
                {
                    USER_HandleCallUpdate(node,
                                          appId,
                                          USER_CALL_DROPPED);
                }
            }
        }
    }

    else if (node->nodeId == msgInfo->appDestNodeId)
    {
        appList = cellularAppLayerData->appInfoTerminateList;

        if (CellularAbstractGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
           if (retAppInfo->appStatus ==
                  CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED)
           {
               //do nothing
           }
            else
            {
               retAppInfo->appStatus = CELLULAR_ABSTRACT_APP_STATUS_FAILED;

               if (callDropCause ==
                CELLULAR_ABSTRACT_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    cellularAppLayerData->
                        stats.numTerminateAppDroppedCauseHandoverFailure ++;
                }
                else if (callDropCause ==
                    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    cellularAppLayerData->
                        stats.numTerminateAppDroppedCauseSelfPowerOff ++;
                }
                else if (callDropCause ==
                    CELLULAR_ABSTRACT_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    cellularAppLayerData->
                        stats.numTerminateAppDroppedCauseRemotePowerOff ++;
                }

                //update stats
                cellularAppLayerData->stats.numAppDropped ++;
                cellularAppLayerData->stats.numTerminateAppDropped ++;
            }
        }

    }

    if (DEBUG_APP)
    {
        printf(
            "node %d: Handle CallDropMsg\n",
            node->nodeId);
    }
}
//**********************************************************************
// /**
// FUNCTION   :: CellularAbstractAppHandleCallRejectedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Reject msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void CellularAbstractAppHandleCallRejectedMsg(Node *node, Message *msg)
{
    CellularAbstractCallRejectMessageInfo *msgInfo;

    int appId;
    CellularAbstractApplicationInfo *appList;
    CellularAbstractApplicationInfo *retAppInfo;

    CellularAbstractApplicationLayerData *cellularAppLayerData;

    cellularAppLayerData = (CellularAbstractApplicationLayerData *)
                                node->appData.appCellularAbstractVar;

    msgInfo =
        (CellularAbstractCallRejectMessageInfo *)MESSAGE_ReturnInfo(msg);

    //update the application status to rejected
    appList = cellularAppLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: Handle CallRejectMsg\n",
            node->nodeId);
    }

    appId = msgInfo->appId;

    if (CellularAbstractGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        retAppInfo->appStatus = CELLULAR_ABSTRACT_APP_STATUS_REJECTED;
    }

    //TODO:schedule a retry timer if required

    cellularAppLayerData->stats.numAppRejectedByNetwork ++;

    if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_SYSTEM_BUSY)
    {
        cellularAppLayerData->stats.numAppRejectedCauseSystemBusy ++;
        if (DEBUG_APP)
        {
            printf("node %d: SYSTEM BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_BUSY)
    {
        cellularAppLayerData->stats.numAppRejectedCauseUserBusy ++;
        if (DEBUG_APP)
        {
            printf("node %d: USER BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP)
    {
        cellularAppLayerData->stats.numAppRejectedCauseTooManyActiveApp ++;
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_UNREACHABLE)
    {
        cellularAppLayerData->stats.numAppRejectedCauseUserUnreachable ++;
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_USER_POWEROFF)
    {
        cellularAppLayerData->stats.numAppRejectedCauseUserPowerOff ++;
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_NETWORK_NOT_FOUND)
    {
        cellularAppLayerData->stats.numAppRejectedCauseNetworkNotFound ++;
        if (DEBUG_APP)
        {
            printf("node %d: NETWORK NOT FOUND\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_UNSUPPORTED_SERVICE)
    {
        cellularAppLayerData->stats.numAppRejectedCauseUnsupportService ++;
    }
    else if (msgInfo->rejectCause ==
        CELLULAR_ABSTRACT_CALL_REJECT_CAUSE_UNKNOWN_USER)
    {
        cellularAppLayerData->stats.numAppRejectedCauseUnknowUser ++;
        if (DEBUG_APP)
        {
            printf("node %d: UNKNOWN USER\n",node->nodeId);
        }
    }
}
//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractAppInitStats
// LAYER      :: APP
// PURPOSE    :: Init the statistic of app layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     :: void : NULL
// **/
//***********************************************************************
static
void CellularAbstractAppInitStats(Node *node)
{
    if (DEBUG_APP)
    {
        printf("node %d: Cellular APP Init Stat\n", node->nodeId);
    }
}
//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractNotifyNetworkPowerOn
// LAYER      :: APP
// PURPOSE    :: Send netowrk layer the power on notification
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     :: void : NULL
// **/
//***********************************************************************
static
void CellularAbstractNotifyNetworkPowerOn(Node *node)
{
    Message *powerOnMsg;
    CellularAbstractApplicationLayerData *cellularAppLayerData;

    cellularAppLayerData =
        (CellularAbstractApplicationLayerData *)
            node->appData.appCellularAbstractVar;

    powerOnMsg =
        MESSAGE_Alloc(
            node,
            NETWORK_LAYER,
            NETWORK_PROTOCOL_CELLULAR,
            MSG_CELLULAR_PowerOn);

    RandomSeed seed;  // This is needed only here.
    RANDOM_SetSeed(seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_CELLULAR_ABSTRACT);

    MESSAGE_Send(
        node,
        powerOnMsg,
        (clocktype) (RANDOM_erand(seed) *
                     CELLULAR_ABSTRACT_DEFAULT_POWER_ON_TIME));

    cellularAppLayerData->msActiveStatus = CELLULAR_ABSTRACT_MS_ACTIVE;


    if (DEBUG_APP)
    {
        printf("node %d: is power on \n", node->nodeId);
    }

    // for testing poweroff purpose only
    // FOr example, MS node 5 is powered off at a time instance between
    // 200 and 600 second
    /*if (node->nodeId  == 5){
        Message *powerOffMsg;

        powerOffMsg =
            MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                NETWORK_PROTOCOL_CELLULAR,
                MSG_CELLULAR_PowerOff);

        MESSAGE_Send(
            node,
            powerOffMsg,
            ((clocktype)(400 * RANDOM_erand(seed) )+ 200) * SECOND);
    }*/

}

//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractNotifyNetworkPowerOff
// LAYER      :: APP
// PURPOSE    :: Send netowrk layer the power on notification
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN     :: void : NULL
// **/
//***********************************************************************
static
void CellularAbstractNotifyNetworkPowerOff(Node *node)
{
    Message *powerOffMsg;

    CellularAbstractApplicationLayerData *cellularAppLayerData;

    cellularAppLayerData =
        (CellularAbstractApplicationLayerData *)
            node->appData.appCellularAbstractVar;


    powerOffMsg =
        MESSAGE_Alloc(
            node,
            NETWORK_LAYER,
            NETWORK_PROTOCOL_CELLULAR,
            MSG_CELLULAR_PowerOff);

    MESSAGE_Send(
        node,
        powerOffMsg,
        0);

    cellularAppLayerData->msActiveStatus = CELLULAR_ABSTRACT_MS_INACTIVE;

    if (DEBUG_APP)
    {
        printf("node %d: is power off \n", node->nodeId);
    }

}

//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractAppInit
// LAYER      :: APP
// PURPOSE    :: Init an application.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void CellularAbstractAppInit(Node *node, const NodeInput *nodeInput)
{
    CellularAbstractApplicationLayerData *cellularAppLayerData;
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    char profileNameStr[MAX_STRING_LENGTH];


    if (DEBUG_APP)
    {
        printf("node %d: Cellular APP Init\n",node->nodeId);
    }

    cellularAppLayerData =
        (CellularAbstractApplicationLayerData *)
            MEM_malloc(sizeof(CellularAbstractApplicationLayerData));

    memset(cellularAppLayerData,
           0,
           sizeof(CellularAbstractApplicationLayerData));

    node->appData.appCellularAbstractVar = (void *) cellularAppLayerData;

    cellularAppLayerData->msActiveStatus = CELLULAR_ABSTRACT_MS_INACTIVE;

    IO_ReadString(
           node->nodeId,
           ANY_DEST,
           nodeInput,
           "CELLULAR-STATISTICS",
           &retVal,
           buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        cellularAppLayerData->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        cellularAppLayerData->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! Default value"
            "YES is used.");
        cellularAppLayerData->collectStatistics = TRUE;
    }

    // User model is initialized after application,
    // so we can't look at userData
    IO_ReadString(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "USER-PROFILE",
            &retVal,
            profileNameStr);

    if (!retVal)
    {
        CellularAbstractNotifyNetworkPowerOn(node);
    }

    //init stats
    CellularAbstractAppInitStats(node);
}
//***********************************************************************
// /**
// FUNCTION   :: CellularAbstractAppFinalize
// LAYER      :: APP
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void CellularAbstractAppFinalize(Node *node)
{
    if (DEBUG_APP)
    {
        printf("node %d: APP Layer finalize the node \n", node->nodeId);
    }

    CellularAbstractApplicationLayerData *cellularAppLayerData;
    char buf[MAX_STRING_LENGTH];

    cellularAppLayerData =
        (CellularAbstractApplicationLayerData *)
            node->appData.appCellularAbstractVar;

    if (cellularAppLayerData->collectStatistics == TRUE ||
        node->appData.appStats ==  TRUE)
    {
        sprintf(
            buf,
            "Number of application requests sent to layer 3 = %d",
            cellularAppLayerData->stats.numAppRequestSent);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests received = %d",
            cellularAppLayerData->stats.numAppRequestRcvd);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests accepted = %d",
            cellularAppLayerData->stats.numAppAcceptedByNetwork);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected = %d",
            cellularAppLayerData->stats.numAppRejectedByNetwork);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
         "Number of application requests rejected (cause: System Busy) = %d",
            cellularAppLayerData->stats.numAppRejectedCauseSystemBusy);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: Network Not "
            "Found) = %d",
            cellularAppLayerData->stats.numAppRejectedCauseNetworkNotFound);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: Too Many Active "
            "App) = %d",
            cellularAppLayerData->stats.numAppRejectedCauseTooManyActiveApp);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: Unknown User) "
            "= %d",
            cellularAppLayerData->stats.numAppRejectedCauseUnknowUser);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: User Power Off) "
            "= %d",
            cellularAppLayerData->stats.numAppRejectedCauseUserPowerOff);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
           "Number of application requests rejected (cause: User Busy) = %d",
            cellularAppLayerData->stats.numAppRejectedCauseUserBusy);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: Unsupported "
            "Service) = %d",
           cellularAppLayerData->stats.numAppRejectedCauseUnsupportService);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of application requests rejected (cause: User "
            "Unreachable) = %d",
            cellularAppLayerData->stats.numAppRejectedCauseUserUnreachable);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of applications successfully end = %d",
            cellularAppLayerData->stats.numAppSuccessEnd);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
        sprintf(
            buf,
            "Number of origin applications successfully end = %d",
            cellularAppLayerData->stats.numOriginAppSuccessEnd);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
        sprintf(
            buf,
            "Number of terminating applications successfully end = %d",
            cellularAppLayerData->stats.numTerminateAppSuccessEnd);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Total number of applications dropped = %d",
            cellularAppLayerData->stats.numAppDropped);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of origin application dropped = %d",
            cellularAppLayerData->stats.numOriginAppDropped);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of origin applications dropped (cause: Handover "
            "Failure) = %d",
            cellularAppLayerData->
                stats.numOriginAppDroppedCauseHandoverFailure);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
         "Number of origin applications dropped (cause: Self PowerOff) = %d",
           cellularAppLayerData->stats.numOriginAppDroppedCauseSelfPowerOff);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
        sprintf(
            buf,
            "Number of origin applications dropped (cause: Remote "
            "PowerOff) = %d",
         cellularAppLayerData->stats.numOriginAppDroppedCauseRemotePowerOff);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
         sprintf(
            buf,
            "Number of terminating applications dropped  = %d",
            cellularAppLayerData->stats.numTerminateAppDropped);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of terminating applications dropped (cause: Handover "
            "Failure) = %d",
            cellularAppLayerData->
                stats.numTerminateAppDroppedCauseHandoverFailure);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);

        sprintf(
            buf,
            "Number of terminating applications dropped (cause: Self "
            "PowerOff) = %d",
            cellularAppLayerData->
                stats.numTerminateAppDroppedCauseSelfPowerOff);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
        sprintf(
            buf,
            "Number of terminating applications dropped (cause: Remote "
            "PowerOff) = %d",
            cellularAppLayerData->
                stats.numTerminateAppDroppedCauseRemotePowerOff);
        IO_PrintStat(
                    node,
                    "Application",
                    "CELLULAR",
                    ANY_DEST,
                    node->nodeId,
                    buf);
    }
}
//*********************************************************************
// /**
// FUNCTION   :: CellularLayer3Layer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
//***********************************************************************
void CellularAbstractAppLayer(Node *node, Message *msg)
{
    switch (msg->eventType)
    {
        //message from application layer itself,e.g., timer
        case MSG_APP_TimerExpired:
        {
            CellularAbstractAppTimerInfo *info;
            info = (CellularAbstractAppTimerInfo*) MESSAGE_ReturnInfo(msg);
            switch(info->timerType)
            {
                case CELLULAR_ABSTRACT_APP_CALL_START_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: Cellular APP handle appStartTimer \n",
                            node->nodeId);
                    }
                    CellularAbstractAppHandleCallStartTimer(node, msg);
                    break;
                }
                case CELLULAR_ABSTRACT_APP_CALL_END_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: Cellular APP handle appEndTimer\n",
                            node->nodeId);
                    }
                    //if the user layer is enabled,let it know of this event
                    if (node->userData->enabled)
                    {
                        USER_HandleCallUpdate(node,
                                              info->appId,
                                              USER_CALL_COMPLETED);
                    }
                    CellularAbstractAppHandleCallEndTimer(node, msg);
                    break;
                }
                case CELLULAR_ABSTRACT_APP_CALL_RETRY_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: Cellular APP handle appRetryTimer\n",
                            node->nodeId);
                    }

                    //update the application status to success
                    break;
                }
            }
            break;
        }


        //message from network layer
        case MSG_APP_CELLULAR_FromNetworkCallAccepted:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: Cellular APP handle"
                    "FromNetworkCallAccepted\n",
                    node->nodeId);
            }
            CellularAbstractAppHandleCallAcceptMsg(node, msg);

            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallRejected:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: Cellular APP handle"
                    "APP_FROM_NETWORK_REJECTED\n",
                    node->nodeId);
            }
            //if the user layer is enabled, let it know of this event
            if (node->userData->enabled)
            {
                CellularAbstractCallRejectMessageInfo *rejectedInfo;
                rejectedInfo = (CellularAbstractCallRejectMessageInfo*)
                                                    MESSAGE_ReturnInfo(msg);
                USER_HandleCallUpdate(node,
                                      rejectedInfo->appId,
                                      USER_CALL_REJECTED);
            }

            CellularAbstractAppHandleCallRejectedMsg(node, msg);
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallArrive:
        {

            if (DEBUG_APP)
            {
                printf(
                    "node %d: Cellular APP handle"
                    "APP_FROM_NETWORK_REMOTE_CALL_ARRIVE\n",
                    node->nodeId);
            }

            //accept the incoming call
            CellularAbstractApplicationLayerData *appCellularVar;
            CellularAbstractApplicationInfo *retAppInfo;

            Message *answerMsg;
            CellularAbstractCallArriveMessageInfo *callArriveInfo;
            CellularAbstractCallAnsweredMessageInfo *callAnswerInfo;

            appCellularVar =
                (CellularAbstractApplicationLayerData *)
                    node->appData.appCellularAbstractVar;

            answerMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_CELLULAR,
                            MSG_NETWORK_CELLULAR_FromAppCallAnswered);

            MESSAGE_InfoAlloc(
                node,
                answerMsg,
                sizeof(CellularAbstractCallAnsweredMessageInfo));

            callAnswerInfo =
                (CellularAbstractCallAnsweredMessageInfo*)
                    MESSAGE_ReturnInfo(answerMsg);

            callArriveInfo =
                (CellularAbstractCallArriveMessageInfo *)
                    MESSAGE_ReturnInfo(msg);

            callAnswerInfo->appId = callArriveInfo->appId;
            callAnswerInfo->transactionId = callArriveInfo->transactionId;
            callAnswerInfo->appType = callArriveInfo->appType;
            callAnswerInfo->appSrcNodeId = callArriveInfo->appSrcNodeId;
            callAnswerInfo->appDestNodeId = callArriveInfo->appDestNodeId;

            MESSAGE_Send(node, answerMsg, 0);

            //add the app to terminating app list
            CellularAbstractAppInsertList(
                node,
                &(appCellularVar->appInfoTerminateList),
                callAnswerInfo->appId,
                callAnswerInfo->appSrcNodeId,
                callAnswerInfo->appDestNodeId,
                0,
                0,
                callAnswerInfo->appType,
                callAnswerInfo->appNumChannelReq,
                callAnswerInfo->appBandwidthReq,
                FALSE);

            //TODO update app status to ongoing

            if (CellularAbstractGetApp(
                node,
                callArriveInfo->appId,
                callAnswerInfo->appSrcNodeId,
                &(appCellularVar->appInfoTerminateList),
                &retAppInfo) == TRUE)
            {
                retAppInfo->appStatus =
                    CELLULAR_ABSTRACT_APP_STATUS_ONGOING;
            }

            appCellularVar->stats.numAppRequestRcvd ++;
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallEndByRemote:
        {

            CellularAbstractCallEndByRemoteMessageInfo *msgInfo;
            int appId;
            NodeAddress srcNodeId;
            NodeAddress destNodeId;
            CellularAbstractApplicationLayerData *appCellularVar;
            CellularAbstractApplicationInfo *retAppInfo;

            msgInfo =
                (CellularAbstractCallEndByRemoteMessageInfo *)
                    MESSAGE_ReturnInfo(msg);
            appCellularVar =
                (CellularAbstractApplicationLayerData *)
                    node->appData.appCellularAbstractVar;

            appId = msgInfo->appId;
            srcNodeId = msgInfo->appSrcNodeId;
            destNodeId = msgInfo->appDestNodeId;

            if (node->nodeId == srcNodeId)
            {
                //update the application status to success
                if (CellularAbstractGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appCellularVar->appInfoOriginList),
                    &retAppInfo) == TRUE)
                {
                    retAppInfo->appStatus =
                        CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED;
                }
                appCellularVar->stats.numOriginAppSuccessEnd ++;
                if (DEBUG_APP)
                {
                    printf(
                        "%d: MO/FO call ended by remote user--APP DEST\n",
                        node->nodeId);
                }
            }
            else if (node->nodeId == destNodeId)
            {
                //update the application status to success
                if (CellularAbstractGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appCellularVar->appInfoTerminateList),
                    &retAppInfo) == TRUE)
                {
                    retAppInfo->appStatus =
                        CELLULAR_ABSTRACT_APP_STATUS_SUCCESSED;
                }

                appCellularVar->stats.numTerminateAppSuccessEnd ++;
                if (DEBUG_APP)
                {
                    printf(
                        "%d: MT/FT call ended by remote user--APP SRC\n",
                        node->nodeId);
                }
            }
            //update stats
            appCellularVar->stats.numAppSuccessEnd ++;
            if (DEBUG_APP)
            {
                printf(
                    "node %d: Cellular APP handle "
                    "CELLULAR_APP_FROM_NETWORK_REMOTE_CALL_END\n",
                    node->nodeId);
            }
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallDropped:
        {

            if (DEBUG_APP)
            {
                printf(
                    "node %d: Cellular APP handle"
                    "CELLULAR_APP_FROM_NETWORK_DROPPED\n",
                    node->nodeId);

            }

            CellularAbstractAppHandleCallDroppedMsg(node, msg);
            break;
        }
        default:
        {
            printf(
                "CELLULAR ABSTRACT APP: Protocol = %d\n",
                MESSAGE_GetProtocol(msg));
            assert(FALSE);
            abort();
            break;
        }
    }
    MESSAGE_Free(node, msg);
}
