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

#include "app_umtscall.h"
#include "cellular.h"
#include "user.h"
#include "random.h"

#define DEBUG_APP   0
//***********************************************************************
// /**
// FUNCTION   :: AppUmtsCallGetApp
// LAYER      :: APP
// PURPOSE    :: Retrive the application info from the app List.
// PARAMETERS ::
// + node       : Node* : Pointer to node.
// + appId      : int : id of the application.
// + appSrcNodeId  : NodeAddress : src node id, who generate this app
// + appList    : AppUmtsCallInfo** :
//                  Originating app List or terminating app List
// + appInfo    : AppUmtsCallInfo** :
//                  Pointer to hold the retrived infomation
// RETURN     :: BOOL : success or fail when looking for the app in the list
// **/
//************************************************************************
static BOOL
AppUmtsCallGetApp(Node* node,
                   int appId,
                   NodeAddress appSrcNodeId,
                   AppUmtsCallInfo** appList,
                   AppUmtsCallInfo** appInfo)

{
    BOOL found = FALSE;
    AppUmtsCallInfo* currentApp;

    currentApp = *appList;
    if (DEBUG_APP && 0)
    {
        printf(
            "node %d: in Get APP Info\n",
            node->nodeId);
    }
    while (currentApp != NULL)
    {
        if (DEBUG_APP && 0)
        {
            printf(
                "node %d: Get APP Info, appId %d,"
                "src %d, dest %d \n",
                node->nodeId, currentApp->appId,
                currentApp->appSrcNodeId, currentApp->appDestNodeId);
        }
        if (currentApp->appId == appId &&
            currentApp->appSrcNodeId == appSrcNodeId)
        {
            found = TRUE;
            *appInfo = currentApp;
            break;
        }
            currentApp = currentApp->next;
    }
    if (DEBUG_APP && 0)
    {
        printf(
            "node %d: Get APP Info, success/fail= %d\n",
            node->nodeId,found);
    }
    return found;
}
//************************************************************************
// /**
// FUNCTION   :: AppUmtsCallInsertList
// LAYER      :: APP
// PURPOSE    :: Insert the application info to the specified app List.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appList : AppUmtsCallInfo ** :
//                 Originating app List or terminating app List
// + appId : int : id of the application.
// + appSrcNodeId : NodeAddress : Source address of the app
// + appDestNodeId : NodeAddress : Destination address of the app
// + avgTalkTime   : clockType : average talk timeo f the umts call
// + appStartTime : clocktype : Start time of the app
// + appDuration : clocktype : Duration of the app
// + isAppSrc : BOOL : Originaitng or terminating application
// RETURN     :: void : NULL
// **/
//************************************************************************
void
AppUmtsCallInsertList(Node* node,
                       AppUmtsCallInfo** appList,
                       int appId,
                       NodeAddress appSrcNodeId,
                       NodeAddress appDestNodeId,
                       clocktype avgTalkTime,
                       clocktype appStartTime,
                       clocktype appDuration,
                       BOOL isAppSrc)
{
    AppUmtsCallInfo* newApp;
    AppUmtsCallInfo* currentApp;
    AppUmtsCallLayerData* appUmtsCallLayerData;
    Message* callStartTimerMsg;
    AppUmtsCallTimerInfo* info;

    appUmtsCallLayerData = (AppUmtsCallLayerData*)
        node->appData.umtscallVar;
    if (isAppSrc == TRUE)
    {
        (appUmtsCallLayerData->numOriginApp)++;
    }
    else
    {
        (appUmtsCallLayerData->numTerminateApp)++;
    }

    newApp = (AppUmtsCallInfo*)
        MEM_malloc(sizeof(AppUmtsCallInfo));
    memset(newApp,
           0,
           sizeof(AppUmtsCallInfo));

    if (isAppSrc == TRUE)
    {
        newApp->appId = appId + 1; // src app use local one
    }
    else
    {
        newApp->appId = appId; // dest app, use app src's
    }

    newApp->appSrcNodeId = appSrcNodeId;
    newApp->appDestNodeId = appDestNodeId;
    newApp->appStartTime = appStartTime;
    newApp->appDuration = appDuration;
    newApp->avgTalkTime = avgTalkTime;
    assert(appDuration >= 0);
    newApp->appEndTime = newApp->appStartTime + newApp->appDuration;
    newApp->appMaxRetry = APP_UMTS_CALL_MAXIMUM_RETRY;
    newApp->appStatus = APP_UMTS_CALL_STATUS_PENDING;
    newApp->next = NULL;

    newApp->startCallTimer = NULL;
    newApp->endCallTimer = NULL;
    newApp->retryCallTimer = NULL;

    newApp->pktItvl = APP_UMTS_CALL_DEF_PKTITVL;
    newApp->dataRate = APP_UMTS_CALL_DEF_DATARATE;
    newApp->pktSize = (Int32)((newApp->dataRate * newApp->pktItvl)/(8 * SECOND));

    // Setup distribution for avgTalk time generation at the SRC and dest
    newApp->avgTalkDurDis = new RandomDistribution<clocktype>();
    if (isAppSrc == TRUE)
    {
        newApp->avgTalkDurDis->setSeed(node->globalSeed,
                                       newApp->appSrcNodeId,
                                       newApp->appDestNodeId,
                                       (UInt32)newApp->appStartTime);
    }
    else
    {
        newApp->avgTalkDurDis->setSeed(node->globalSeed,
                                       newApp->appDestNodeId,
                                       newApp->appSrcNodeId,
                                       (UInt32)newApp->appEndTime);
    }
    newApp->avgTalkDurDis->setDistributionExponential(
                                    newApp->avgTalkTime);

    if (*appList == NULL)
    {
       *appList = newApp;
    }
    else
    {
        currentApp = *appList;
        while (currentApp->next != NULL)
            currentApp = currentApp->next;
        currentApp->next = newApp;
    }

    // schedule a timer for start app event
    // Setup Call Start Timer with info on dest & call end time
    if (isAppSrc == TRUE)
    {
        callStartTimerMsg = MESSAGE_Alloc(node,
                                          APP_LAYER,
                                          APP_UMTS_CALL,
                                          MSG_APP_TimerExpired);
        MESSAGE_InfoAlloc(node,
                          callStartTimerMsg,
                          sizeof(AppUmtsCallTimerInfo));

        info = (AppUmtsCallTimerInfo*)
            MESSAGE_ReturnInfo(callStartTimerMsg);

        info->appSrcNodeId = newApp->appSrcNodeId;
        info->appId = newApp->appId;
        info->timerType = APP_UMTS_CALL_START_TIMER;

        MESSAGE_Send(node, callStartTimerMsg, newApp->appStartTime);

        if (DEBUG_APP)
        {
            char buf1[MAX_STRING_LENGTH];
            char buf2[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(appStartTime, buf1);
            TIME_PrintClockInSecond(appDuration, buf2);
            printf(
                "node %d: UMTS CALL APP Add one new %d th app"
                "src %d dest %d app start %s duration %s\n",
                node->nodeId, appUmtsCallLayerData->numOriginApp,
                appSrcNodeId, appDestNodeId, buf1, buf2);
        }
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallSchedEndTimer
// LAYER      :: APP
// PURPOSE    :: Schedule a call end timer
// PARAMETERS ::
// + node    : Node* : Pointer to node.
// + callApp : AppUmtsCallInfo* : Pointer to the umts call app
// + delay   : clocktype : delay for the timer
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppUmtsCallSchedEndTimer(
        Node* node,
        AppUmtsCallInfo* callApp,
        clocktype delay)
{
    Message* callEndTimerMsg = MESSAGE_Alloc(
                                    node,
                                    APP_LAYER,
                                    APP_UMTS_CALL,
                                    MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(
        node,
        callEndTimerMsg,
        sizeof(AppUmtsCallTimerInfo));

    AppUmtsCallTimerInfo* info = (AppUmtsCallTimerInfo*)
                MESSAGE_ReturnInfo(callEndTimerMsg);
    info->appSrcNodeId = callApp->appSrcNodeId;
    info->appId = callApp->appId;
    info->timerType = APP_UMTS_CALL_END_TIMER;

    MESSAGE_Send(node, callEndTimerMsg, delay);
    if (callApp->endCallTimer)
    {
        MESSAGE_CancelSelfMsg(node, callApp->endCallTimer);
    }
    callApp->endCallTimer = callEndTimerMsg;

    if (DEBUG_APP)
    {
        printf("node %d: scchedule a call end timer \n", node->nodeId);
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallSchedTalkStartTimer
// LAYER      :: APP
// PURPOSE    :: Schedule a call end timer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + callApp : AppUmtsCallInfo* : Pointer to the umts call app
// + delay   : clocktype : delay for the timer
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppUmtsCallSchedTalkStartTimer(
        Node* node,
        AppUmtsCallInfo* callApp,
        clocktype delay)
{
    callApp->talkStartTimer = MESSAGE_Alloc(
                                   node,
                                   APP_LAYER,
                                   APP_UMTS_CALL,
                                   MSG_APP_TimerExpired);
    MESSAGE_InfoAlloc(node,
                      callApp->talkStartTimer,
                      sizeof(AppUmtsCallTimerInfo));

    AppUmtsCallTimerInfo* timerInfo = (AppUmtsCallTimerInfo*)
                        MESSAGE_ReturnInfo(callApp->talkStartTimer);

    timerInfo->appSrcNodeId = callApp->appSrcNodeId;
    timerInfo->appId = callApp->appId;
    timerInfo->timerType = APP_UMTS_CALL_TALK_START_TIMER;

    MESSAGE_Send(node, callApp->talkStartTimer, delay);
}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallStartTalking
// LAYER      :: APP
// PURPOSE    :: Start to generate voice packet and send them to lower layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + appUmtsCallLayerData : AppUmtsCallLayerData *:Pointer to the app data
// + callApp : AppUmtsCallInfo* : Pointer to the umts call app
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppUmtsCallStartTalking(
        Node* node,
        AppUmtsCallLayerData* appUmtsCallLayerData,
        AppUmtsCallInfo* callApp)
{
    BOOL endSession;
    clocktype ts_now = node->getNodeTime();
    if (ts_now + APP_UMTS_CALL_MIN_TALK_TIME > callApp->appEndTime)
    {
        endSession = TRUE;
    }
    else
    {
        int  numPkts;
        clocktype talkDur = callApp->avgTalkDurDis->getRandomNumber();
        if (talkDur < APP_UMTS_CALL_MIN_TALK_TIME)
        {
            talkDur = APP_UMTS_CALL_MIN_TALK_TIME;
        }

        if (ts_now + talkDur + APP_UMTS_CALL_MIN_TALK_TIME
                    <= callApp->appEndTime)
        {
            endSession = FALSE;
            numPkts = (Int32)(talkDur / callApp->pktItvl);
        }
        else if (ts_now + talkDur >= callApp->appEndTime)
        {
            numPkts = (int)((callApp->appEndTime - ts_now)
                            / callApp->pktItvl);
            endSession = TRUE;
        }
        else
        {
            endSession = TRUE;
            numPkts = (int)(talkDur / callApp->pktItvl);
        }

        // Generating packets and send them to the network layer
        for (int i = 0; i < numPkts; i++)
        {
            Message* voicePkt = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    MSG_NETWORK_CELLULAR_FromAppPktArrival);
            MESSAGE_PacketAlloc(node, voicePkt, 0, TRACE_UMTS_LAYER3);
            MESSAGE_AddVirtualPayload(node, voicePkt, callApp->pktSize);

            MESSAGE_AddInfo(node,
                            voicePkt,
                            sizeof(AppUmtsCallData),
                            INFO_TYPE_AppUmtsCallData);
            AppUmtsCallData* voicePktData =
                (AppUmtsCallData*)MESSAGE_ReturnInfo(
                                        voicePkt,
                                        INFO_TYPE_AppUmtsCallData);

            voicePktData->numPkts = numPkts;
            voicePktData->TalkDurLeft = talkDur - i * callApp->pktItvl;
            voicePktData->ts_send = ts_now + (i + 1) * callApp->pktItvl;

            MESSAGE_InfoAlloc(node,
                              voicePkt,
                              sizeof(AppUmtsCallPktArrivalInfo));
            AppUmtsCallPktArrivalInfo* callInfo =
                (AppUmtsCallPktArrivalInfo*)MESSAGE_ReturnInfo(voicePkt);
            callInfo->appId = callApp->appId;
            callInfo->appSrcNodeId = callApp->appSrcNodeId;

            MESSAGE_Send(node, voicePkt, (i + 1) * callApp->pktItvl);
        }
        if (numPkts > 0)
        {
            callApp->curTalkEndTime = numPkts * callApp->pktItvl;
            callApp->appStatus = APP_UMTS_CALL_STATUS_TALKING;
            appUmtsCallLayerData->stats.numPktsSent += numPkts;
        }
    }
    if (endSession)
    {
        clocktype delayToEnd = (callApp->appEndTime > ts_now) ?
                        callApp->appEndTime - ts_now : 0;
        AppUmtsCallSchedEndTimer(
                node,
                callApp,
                delayToEnd);
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandleCallStartTimer
// LAYER      :: APP
// PURPOSE    :: Handle call start timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the Callstart Msg
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppUmtsCallHandleCallStartTimer(Node* node, Message* msg)
{
    Message* callStartMsgToNetwork;
    AppUmtsCallTimerInfo* infoIn;
    int appId;
    App_Umts_Call_Timer_Type timerType;
    AppUmtsCallStartMessageInfo* infoOut;
    AppUmtsCallLayerData* appUmtsCallLayerData;
    AppUmtsCallInfo* appList;
    AppUmtsCallInfo* retAppInfo;


    // get the appId associated with this timer event
    infoIn = (AppUmtsCallTimerInfo*)MESSAGE_ReturnInfo(msg);
    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf(
            "node %d: handle appStartTimer start for the appId is %d\n",
            node->nodeId, appId);
    }

    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;
    appList = appUmtsCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallStartTimer\n",
            node->nodeId);
    }

    if (AppUmtsCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        retAppInfo->startCallTimer = NULL;
        callStartMsgToNetwork  = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_CELLULAR,
                                    MSG_NETWORK_CELLULAR_FromAppStartCall);

        MESSAGE_InfoAlloc(
            node,
            callStartMsgToNetwork,
            sizeof(AppUmtsCallStartMessageInfo));

        infoOut = (AppUmtsCallStartMessageInfo*)
            MESSAGE_ReturnInfo(callStartMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appSrcNodeId = retAppInfo->appSrcNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        infoOut->callEndTime = retAppInfo->appEndTime;
        infoOut->avgTalkTime = retAppInfo->avgTalkTime;

        callStartMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;

        MESSAGE_Send(node, callStartMsgToNetwork, 0);

        // update the stats
        appUmtsCallLayerData->stats.numAppRequestSent++;
        if (DEBUG_APP)
        {
            printf("node %d: Get app info appid %d"
                "src %d dest %d \n",
                node->nodeId, infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId);
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
// FUNCTION   :: AppUmtsCallHandleCallEndTimer
// LAYER      :: APP
// PURPOSE    :: Handle Call end timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the CallEnd Msg
// RETURN     :: void : NULL
// **/
//****************************************************************
static
void AppUmtsCallHandleCallEndTimer(Node* node, Message* msg)
{
    Message* callEndMsgToNetwork;
    AppUmtsCallTimerInfo* infoIn;
    int appId;
    App_Umts_Call_Timer_Type timerType;
    AppUmtsCallEndMessageInfo* infoOut;
    AppUmtsCallLayerData* appUmtsCallLayerData;
    AppUmtsCallInfo* appList;
    AppUmtsCallInfo* retAppInfo;


    //get the appId associated with this timer event
    infoIn = (AppUmtsCallTimerInfo*)MESSAGE_ReturnInfo(msg);

    appId = infoIn->appId;
    timerType = infoIn->timerType;

    if (DEBUG_APP)
    {
        printf("node %d: handle appEndTimer start for the appId is %d\n",
            node->nodeId, appId);
    }

    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;
    if (node->nodeId == infoIn->appSrcNodeId)
    {
        appList = appUmtsCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appUmtsCallLayerData->appInfoTerminateList;
    }

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallEndTimer\n",
            node->nodeId);
    }

    if (AppUmtsCallGetApp(
        node, appId, infoIn->appSrcNodeId, &appList, &retAppInfo) == TRUE)
    {

        callEndMsgToNetwork = MESSAGE_Alloc(
                                node,
                                NETWORK_LAYER,
                                NETWORK_PROTOCOL_CELLULAR,
                                MSG_NETWORK_CELLULAR_FromAppEndCall);

        MESSAGE_InfoAlloc(
            node,
            callEndMsgToNetwork,
            sizeof(AppUmtsCallEndMessageInfo));

        infoOut = (AppUmtsCallEndMessageInfo*)
                        MESSAGE_ReturnInfo(callEndMsgToNetwork);

        infoOut->appId = retAppInfo->appId;
        infoOut->appSrcNodeId = retAppInfo->appSrcNodeId;
        infoOut->appDestNodeId = retAppInfo->appDestNodeId;
        assert(retAppInfo->appDuration >= 0);

        callEndMsgToNetwork->protocolType = NETWORK_PROTOCOL_CELLULAR;
        MESSAGE_Send(node, callEndMsgToNetwork, 0);

        // cancel timer
        if (retAppInfo->retryCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->retryCallTimer);

            retAppInfo->retryCallTimer = NULL;
        }
        if (retAppInfo->startCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->startCallTimer);

            retAppInfo->startCallTimer = NULL;
        }
        if (retAppInfo->talkStartTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->talkStartTimer);

            retAppInfo->talkStartTimer = NULL;
        }

        retAppInfo->endCallTimer = NULL;

        // update the stats
        appUmtsCallLayerData->stats.numAppSuccessEnd++;

        if (node->nodeId == retAppInfo->appSrcNodeId)
        {
            appUmtsCallLayerData->stats.numOriginAppSuccessEnd++;
        }
        else
        {
            appUmtsCallLayerData->stats.numTerminateAppSuccessEnd++;
        }

        retAppInfo->appStatus = APP_UMTS_CALL_STATUS_SUCCESSED;
        if (DEBUG_APP)
        {
            printf(
                "node %d: Get app info appid %d"
                " src %d dest %d \n",
                node->nodeId, infoOut->appId,
                infoOut->appSrcNodeId, infoOut->appDestNodeId);
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
            node->nodeId, appId);
        ERROR_Assert(FALSE, errorBuf);
    }

}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandleTalkStartTimer
// LAYER      :: APP
// PURPOSE    :: Handle talk start timer expiration event
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the Callstart Msg
// RETURN     :: void : NULL
// **/
//*************************************************************
static
void AppUmtsCallHandleTalkStartTimer(Node* node, Message* msg)
{
    AppUmtsCallLayerData* appUmtsCallLayerData;
    AppUmtsCallInfo* umtsCallApp;
    AppUmtsCallInfo* appList;

    AppUmtsCallTimerInfo* timerInfo =
        (AppUmtsCallTimerInfo*)MESSAGE_ReturnInfo(msg);
    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;

    if (node->nodeId == timerInfo->appSrcNodeId)
    {
        appList = appUmtsCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appUmtsCallLayerData->appInfoTerminateList;
    }
    if (AppUmtsCallGetApp(
            node,
            timerInfo->appId,
            timerInfo->appSrcNodeId,
            &(appList),
            &umtsCallApp) == TRUE)
    {
        umtsCallApp->talkStartTimer = NULL;
        clocktype ts_now = node->getNodeTime();
        if (ts_now < umtsCallApp->nextTalkStartTime)
        {
            AppUmtsCallSchedTalkStartTimer(
                node,
                umtsCallApp,
                umtsCallApp->nextTalkStartTime - ts_now);
        }
        else
        {
            AppUmtsCallStartTalking(node,
                                     appUmtsCallLayerData,
                                     umtsCallApp);
        }
    }
}

//**************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandleCallAcceptMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Accept msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//***************************************************************
static void
AppUmtsCallHandleCallAcceptMsg(Node* node, Message* msg)
{
    int appId;
    AppUmtsCallLayerData* appUmtsCallLayerData;
    AppUmtsCallInfo* appList;
    AppUmtsCallInfo* retAppInfo;

    AppUmtsCallAcceptMessageInfo* callAcceptInfo;

    callAcceptInfo =
        (AppUmtsCallAcceptMessageInfo*)MESSAGE_ReturnInfo(msg);

    appId = callAcceptInfo->appId;
    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;
    appList = appUmtsCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallAcceptMsg\n",
            node->nodeId);
    }
    if (AppUmtsCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        AppUmtsCallStartTalking(node, appUmtsCallLayerData, retAppInfo);
    }
    appUmtsCallLayerData->stats.numAppAcceptedByNetwork++;
}

//**********************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandleCallDroppedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Dropped msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppUmtsCallHandleCallDroppedMsg(Node* node, Message* msg)
{
    AppUmtsCallDropMessageInfo* msgInfo;
    int appId;
    AppUmtsCallDropCauseType callDropCause;
    AppUmtsCallInfo* appList;
    AppUmtsCallInfo* retAppInfo;

    AppUmtsCallLayerData* appUmtsCallLayerData;

    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;

    msgInfo =
        (AppUmtsCallDropMessageInfo*)MESSAGE_ReturnInfo(msg);

    appId = msgInfo->appId;

    callDropCause = msgInfo->dropCause;
    
    // update the application status to dropped
    if (node->nodeId == msgInfo->appSrcNodeId)
    {
        appList = appUmtsCallLayerData->appInfoOriginList;
        if (AppUmtsCallGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
            if (retAppInfo->appStatus ==
                APP_UMTS_CALL_STATUS_SUCCESSED)
            {
                // if call end msg is  sent before rcv this call drop msg
                // do nothing
            }
            else
            {
                retAppInfo->appStatus = APP_UMTS_CALL_STATUS_FAILED;

                // cancel timer
                if (retAppInfo->retryCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->retryCallTimer);

                    retAppInfo->retryCallTimer = NULL;
                }
                if (retAppInfo->startCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->startCallTimer);

                    retAppInfo->startCallTimer = NULL;
                }
                if (retAppInfo->talkStartTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->talkStartTimer);

                    retAppInfo->talkStartTimer = NULL;
                }
                if (retAppInfo->endCallTimer != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        retAppInfo->endCallTimer);

                    retAppInfo->endCallTimer = NULL;
                }
                if (callDropCause ==
                    APP_UMTS_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    appUmtsCallLayerData->
                        stats.numOriginAppDroppedCauseHandoverFailure++;
                }
                else if (callDropCause ==
                        APP_UMTS_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    appUmtsCallLayerData->
                        stats.numOriginAppDroppedCauseSelfPowerOff++;
                }
                else if (callDropCause ==
                        APP_UMTS_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    appUmtsCallLayerData->
                        stats.numOriginAppDroppedCauseRemotePowerOff++;
                }

                // update stats
                appUmtsCallLayerData->stats.numAppDropped++;
                appUmtsCallLayerData->stats.numOriginAppDropped++;

                // TODO:schedule a retry timer if required
                // if the user layer is enabled, let it know of this event
                // if the user layer is enabled, let it know of this event
                if (node->userData->enabled)
                {
                    USER_HandleCallUpdate(node,
                                          appId,
                                          USER_CALL_DROPPED);
                }
            } // if (retAppInfo->appStatus)
        } // if (AppUmtsCallGetApp()), app exist
    } // if (node->nodeId == msgInfo->appSrcNodeId);src of umts call

    else if (node->nodeId == msgInfo->appDestNodeId)
    {
        appList = appUmtsCallLayerData->appInfoTerminateList;

        if (AppUmtsCallGetApp(
            node, appId, msgInfo->appSrcNodeId,
            &appList, &retAppInfo) == TRUE)
        {
           if (retAppInfo->appStatus == APP_UMTS_CALL_STATUS_SUCCESSED)
           {
               // do nothing
           }
            else
            {
               retAppInfo->appStatus = APP_UMTS_CALL_STATUS_FAILED;

               if (callDropCause ==
                    APP_UMTS_CALL_DROP_CAUSE_HANDOVER_FAILURE)
                {
                    appUmtsCallLayerData->
                        stats.numTerminateAppDroppedCauseHandoverFailure++;
                }
                else if (callDropCause ==
                        APP_UMTS_CALL_DROP_CAUSE_SELF_POWEROFF)
                {
                    appUmtsCallLayerData->
                        stats.numTerminateAppDroppedCauseSelfPowerOff++;
                }
                else if (callDropCause ==
                        APP_UMTS_CALL_DROP_CAUSE_REMOTEUSER_POWEROFF)
                {
                    appUmtsCallLayerData->
                        stats.numTerminateAppDroppedCauseRemotePowerOff++;
                }

                // update stats
                appUmtsCallLayerData->stats.numAppDropped++;
                appUmtsCallLayerData->stats.numTerminateAppDropped++;
            } // if (retAppInfo->appStatus)
        } // if (AppUmtsCallGetApp()); app exist
    } // if (node->nodeId == msgInfo->appDestNodeId), dest of umts call

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallDroppedMsg\n",
            node->nodeId);
    }
}
//**********************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandleCallRejectedMsg
// LAYER      :: APP
// PURPOSE    :: Handle Call Reject msg from netowrk
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppUmtsCallHandleCallRejectedMsg(Node* node, Message* msg)
{
    AppUmtsCallRejectMessageInfo* msgInfo;

    int appId;
    AppUmtsCallInfo* appList;
    AppUmtsCallInfo* retAppInfo;

    AppUmtsCallLayerData* appUmtsCallLayerData;

    appUmtsCallLayerData = (AppUmtsCallLayerData*)
                                node->appData.umtscallVar;

    msgInfo =
        (AppUmtsCallRejectMessageInfo*)MESSAGE_ReturnInfo(msg);

    // update the application status to rejected
    appList = appUmtsCallLayerData->appInfoOriginList;

    if (DEBUG_APP)
    {
        printf(
            "node %d: ready to Get APP Info, HandleCallRejectedMsg\n",
            node->nodeId);
    }

    appId = msgInfo->appId;

    if (AppUmtsCallGetApp(
        node, appId, node->nodeId, &appList, &retAppInfo) == TRUE)
    {
        // cancel timer
        if (retAppInfo->retryCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->retryCallTimer);

            retAppInfo->retryCallTimer = NULL;
        }
        if (retAppInfo->startCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->startCallTimer);

            retAppInfo->startCallTimer = NULL;
        }
        if (retAppInfo->talkStartTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->talkStartTimer);

            retAppInfo->talkStartTimer = NULL;
        }
        if (retAppInfo->endCallTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                retAppInfo->endCallTimer);

            retAppInfo->endCallTimer = NULL;
        }
        retAppInfo->appStatus = APP_UMTS_CALL_STATUS_REJECTED;
    }

    // TODO:schedule a retry timer if required

    appUmtsCallLayerData->stats.numAppRejectedByNetwork++;

    if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_SYSTEM_BUSY)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseSystemBusy++;
        if (DEBUG_APP)
        {
            printf("node %d: SYSTEM BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_USER_BUSY)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseUserBusy++;
        if (DEBUG_APP)
        {
            printf("node %d: USER BUSY\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseTooManyActiveApp++;
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_USER_UNREACHABLE)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseUserUnreachable++;
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_USER_POWEROFF)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseUserPowerOff++;
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_NETWORK_NOT_FOUND)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseNetworkNotFound++;
        if (DEBUG_APP)
        {
            printf("node %d: NETWORK NOT FOUND\n", node->nodeId);
        }
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_UNSUPPORTED_SERVICE)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseUnsupportService++;
    }
    else if (msgInfo->rejectCause ==
        APP_UMTS_CALL_REJECT_CAUSE_UNKNOWN_USER)
    {
        appUmtsCallLayerData->stats.numAppRejectedCauseUnknowUser++;
        if (DEBUG_APP)
        {
            printf("node %d: UNKNOWN USER\n", node->nodeId);
        }
    }
}

//**********************************************************************
// /**
// FUNCTION   :: AppUmtsCallHandlePktArrival
// LAYER      :: APP
// PURPOSE    :: Handle Packet arrival from the lower layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg  : Message : Point to the call accept
// RETURN     :: void : NULL
// **/
//*************************************************************************
static
void AppUmtsCallHandlePktArrival(Node* node, Message* msg)
{
    AppUmtsCallPktArrivalInfo* callInfo =
        (AppUmtsCallPktArrivalInfo*)MESSAGE_ReturnInfo(msg);
    AppUmtsCallInfo* umtsCallApp;

    AppUmtsCallLayerData* appUmtsCallLayerData
                 = (AppUmtsCallLayerData*)node->appData.umtscallVar;

    AppUmtsCallInfo* appList;
    if (node->nodeId == callInfo->appSrcNodeId)
    {
        appList = appUmtsCallLayerData->appInfoOriginList;
    }
    else
    {
        appList = appUmtsCallLayerData->appInfoTerminateList;
    }
    if (AppUmtsCallGetApp(
            node,
            callInfo->appId,
            callInfo->appSrcNodeId,
            &appList,
            &umtsCallApp))
    {
        clocktype ts_now = node->getNodeTime();
        AppUmtsCallData* voicePktData =
            (AppUmtsCallData*)MESSAGE_ReturnInfo(
                                    msg,
                                    INFO_TYPE_AppUmtsCallData);

        appUmtsCallLayerData->stats.numPktsRcvd++;
        appUmtsCallLayerData->stats.totEndToEndDelay +=
                    (ts_now - voicePktData->ts_send);
        if (umtsCallApp->appStatus == APP_UMTS_CALL_STATUS_ONGOING ||
            (umtsCallApp->appStatus == APP_UMTS_CALL_STATUS_TALKING &&
             umtsCallApp->curTalkEndTime < ts_now))
        {
            umtsCallApp->appStatus = APP_UMTS_CALL_STATUS_LISTENING;

            // Start timer to start talking
            AppUmtsCallSchedTalkStartTimer(
                    node,
                    umtsCallApp,
                    voicePktData->TalkDurLeft);
            umtsCallApp->nextTalkStartTime =
                ts_now + voicePktData->TalkDurLeft
                       + APP_UMTS_CALL_TALK_TURNAROUND_TIME;
        }
        else if (umtsCallApp->appStatus == APP_UMTS_CALL_STATUS_LISTENING)
        {
            if (!umtsCallApp->talkStartTimer)
            {
                AppUmtsCallSchedTalkStartTimer(
                        node,
                        umtsCallApp,
                        voicePktData->TalkDurLeft);
                umtsCallApp->nextTalkStartTime =
                    ts_now + voicePktData->TalkDurLeft
                           + APP_UMTS_CALL_TALK_TURNAROUND_TIME;
            }
            else
            {
                if (ts_now + voicePktData->TalkDurLeft
                           + APP_UMTS_CALL_TALK_TURNAROUND_TIME
                           > umtsCallApp->nextTalkStartTime)
                {
                    umtsCallApp->nextTalkStartTime =
                        ts_now + voicePktData->TalkDurLeft
                               + APP_UMTS_CALL_TALK_TURNAROUND_TIME;
                }
            } // if (umtsCallApp->talkStartTimer); timer is active
        } // if (umtsCallApp->appStatus)
    } // if (AppUmtsCallGetApp()), app exists
}

//***********************************************************************
// /**
// FUNCTION   :: AppUmtsCallInitStats
// LAYER      :: APP
// PURPOSE    :: Init the statistic of app layer
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + nodeInput  : NodeInput : Point to the input
// RETURN     :: void : NULL
// **/
//***********************************************************************
static
void AppUmtsCallInitStats(Node* node, const NodeInput* nodeInput)
{
    if (DEBUG_APP)
    {
        printf("node %d: UMTS-CALL APP Init Stat\n", node->nodeId);
    }
}

//***********************************************************************
// /**
// FUNCTION   :: AppUmtsCallInit
// LAYER      :: APP
// PURPOSE    :: Init an application.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void AppUmtsCallInit(Node* node, const NodeInput* nodeInput)
{
    AppUmtsCallLayerData* appUmtsCallLayerData;
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    if (DEBUG_APP)
    {
        printf("node %d: UMTS-CALL APP Init\n", node->nodeId);
    }

    appUmtsCallLayerData =
        (AppUmtsCallLayerData*)
            MEM_malloc(sizeof(AppUmtsCallLayerData));

    memset(appUmtsCallLayerData,
           0,
           sizeof(AppUmtsCallLayerData));
    appUmtsCallLayerData->appInfoOriginList = NULL;
    appUmtsCallLayerData->appInfoTerminateList = NULL;
    appUmtsCallLayerData->numOriginApp = 0;
    appUmtsCallLayerData->numTerminateApp = 0;


    node->appData.umtscallVar = (void*)appUmtsCallLayerData;

    // init stats
    AppUmtsCallInitStats(node, nodeInput);

    IO_ReadString(
           node->nodeId,
           ANY_DEST,
           nodeInput,
           "CELLULAR-STATISTICS",
           &retVal,
           buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        appUmtsCallLayerData->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        appUmtsCallLayerData->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! "
            "Default value YES is used.");
        appUmtsCallLayerData->collectStatistics = TRUE;
    }
}

//***********************************************************************
// /**
// FUNCTION   :: AppUmtsCallFinalize
// LAYER      :: APP
// PURPOSE    :: Print stats and clear protocol variables
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// RETURN     :: void : NULL
// **/
//**********************************************************************
void AppUmtsCallFinalize(Node* node)
{
    if (DEBUG_APP)
    {
        printf("node %d: APP Layer finalize the node \n", node->nodeId);
    }

    AppUmtsCallLayerData* appUmtsCallLayerData;
    char buf[MAX_STRING_LENGTH];

    appUmtsCallLayerData =
        (AppUmtsCallLayerData*)
            node->appData.umtscallVar;

    if (!appUmtsCallLayerData->collectStatistics)
    {
        return;
    }

    if (appUmtsCallLayerData->numOriginApp == 0 &&
        appUmtsCallLayerData->numTerminateApp == 0)
    {
        return;
    }
    sprintf(
        buf,
        "Number of app request sent to layer 3 = %d",
        appUmtsCallLayerData->stats.numAppRequestSent);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request rcvd = %d",
        appUmtsCallLayerData->stats.numAppRequestRcvd);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request accepted = %d",
        appUmtsCallLayerData->stats.numAppAcceptedByNetwork);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request rejected = %d",
        appUmtsCallLayerData->stats.numAppRejectedByNetwork);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (casue: System Busy) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseSystemBusy);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Network Not Found) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseNetworkNotFound);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Too Many Active App) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseTooManyActiveApp);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Unknown User) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseUnknowUser);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Power Off) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseUserPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Busy) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseUserBusy);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: Unsupported Service) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseUnsupportService);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app request reject (cause: User Unreachable) = %d",
        appUmtsCallLayerData->stats.numAppRejectedCauseUserUnreachable);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app successfully end = %d",
        appUmtsCallLayerData->stats.numAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of originating app successfully end = %d",
        appUmtsCallLayerData->stats.numOriginAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of terminating app successfully end = %d",
        appUmtsCallLayerData->stats.numTerminateAppSuccessEnd);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of app dropped  = %d",
        appUmtsCallLayerData->stats.numAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped  = %d",
        appUmtsCallLayerData->stats.numOriginAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped (Cause: Handover Failure)  = %d",
        appUmtsCallLayerData->
            stats.numOriginAppDroppedCauseHandoverFailure);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of originating app dropped (Cause: Self PowerOff)  = %d",
        appUmtsCallLayerData->
            stats.numOriginAppDroppedCauseSelfPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
    sprintf(
        buf,
        "Number of originating app dropped (Cause: Remote PowerOff)  = %d",
        appUmtsCallLayerData->
            stats.numOriginAppDroppedCauseRemotePowerOff);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
     sprintf(
        buf,
        "Number of terminating app dropped  = %d",
        appUmtsCallLayerData->stats.numTerminateAppDropped);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Handover Faliure)  = %d",
        appUmtsCallLayerData->
            stats.numTerminateAppDroppedCauseHandoverFailure);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Self PowerOff)  = %d",
        appUmtsCallLayerData->
            stats.numTerminateAppDroppedCauseSelfPowerOff);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of terminating app dropped (Cause: Remote PowerOff)  = %d",
        appUmtsCallLayerData->
            stats.numTerminateAppDroppedCauseRemotePowerOff);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of voice packets sent  = %d",
        appUmtsCallLayerData->stats.numPktsSent);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    sprintf(
        buf,
        "Number of voice packets received  = %d",
        appUmtsCallLayerData->stats.numPktsRcvd);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);

    char delayBuf[MAX_STRING_LENGTH];
    clocktype avgEndToEndDelay = 0;
    if (appUmtsCallLayerData->stats.numPktsRcvd > 0)
    {
        avgEndToEndDelay =
            appUmtsCallLayerData->stats.totEndToEndDelay /
            appUmtsCallLayerData->stats.numPktsRcvd;
    }
    TIME_PrintClockInSecond(
        avgEndToEndDelay,
        delayBuf);
    sprintf(
        buf,
        "Average end to end delay (seconds) = %s",
        delayBuf);
    IO_PrintStat(
                node,
                "Application",
                "UMTS-CALL",
                ANY_DEST,
                node->nodeId,
                buf);
}

//*********************************************************************
// /**
// FUNCTION   :: AppUmtsCallLayer
// LAYER      :: APPLICATION
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
//***********************************************************************
void AppUmtsCallLayer(Node* node, Message* msg)
{
    switch (msg->eventType)
    {
        // message from application layer itself,e.g., timer
        case MSG_APP_TimerExpired:
        {
            AppUmtsCallTimerInfo* info;
            info = (AppUmtsCallTimerInfo*)MESSAGE_ReturnInfo(msg);
            switch (info->timerType)
            {
                case APP_UMTS_CALL_START_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: UMTS-CALL APP handle appStartTimer\n",
                            node->nodeId);
                    }
                    AppUmtsCallHandleCallStartTimer(node, msg);
                    break;
                }
                case APP_UMTS_CALL_END_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: UMTS-CALL APP handle appEndTimer\n",
                            node->nodeId);
                    }
                    // if the user layer is enabled,let it know of this event
                    if (node->userData->enabled)
                    {
                        USER_HandleCallUpdate(node,
                                              info->appId,
                                              USER_CALL_COMPLETED);
                    }
                    AppUmtsCallHandleCallEndTimer(node, msg);
                    break;
                }
                case APP_UMTS_CALL_RETRY_TIMER:
                {
                    if (DEBUG_APP)
                    {
                        printf(
                            "node %d: UMTS-CALL APP handle appRetryTimer\n",
                            node->nodeId);
                    }

                    // update the application status to success
                    break;
                }
                case APP_UMTS_CALL_TALK_START_TIMER:
                {
                    AppUmtsCallHandleTalkStartTimer(node, msg);
                }
            }
            break;
        }

        // message from network layer
        case MSG_APP_CELLULAR_FromNetworkCallAccepted:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: UMTS-CALL APP handle"
                    "FromNetworkCallAccepted\n",
                    node->nodeId);
            }
            AppUmtsCallHandleCallAcceptMsg(node, msg);

            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallRejected:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: UMTS-CALL APP handle"
                    "APP_FROM_NETWORK_REJECTED\n",
                    node->nodeId);
            }
            // if the user layer is enabled, let it know of this event
            if (node->userData->enabled)
            {
                AppUmtsCallRejectMessageInfo *rejectedInfo;
                rejectedInfo = (AppUmtsCallRejectMessageInfo*)
                                    MESSAGE_ReturnInfo(msg);
                USER_HandleCallUpdate(node,
                                      rejectedInfo->appId,
                                      USER_CALL_REJECTED);
            }

            AppUmtsCallHandleCallRejectedMsg(node, msg);
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallArrive:
        {
            if (DEBUG_APP)
            {
                printf(
                    "node %d: UMTS-CALL APP handle"
                    "APP_FROM_NETWORK_REMOTE_CALL_ARRIVE\n",
                    node->nodeId);
            }

             // add the app to terminating app list
            AppUmtsCallLayerData* appUmtsCallLayerData;
            AppUmtsCallInfo* retAppInfo;

            appUmtsCallLayerData =
                (AppUmtsCallLayerData*)
                    node->appData.umtscallVar;
            AppUmtsCallArriveMessageInfo* callArriveInfo =
                                (AppUmtsCallArriveMessageInfo*)
                                MESSAGE_ReturnInfo(msg);

            clocktype ts_now = node->getNodeTime();
            clocktype duration = callArriveInfo->callEndTime > ts_now ?
                                callArriveInfo->callEndTime - ts_now : 0;

            AppUmtsCallInsertList(
                node,
                &(appUmtsCallLayerData->appInfoTerminateList),
                callArriveInfo->appId,
                callArriveInfo->appSrcNodeId,
                callArriveInfo->appDestNodeId,
                callArriveInfo->avgTalkTime,
                node->getNodeTime(),
                duration,
                FALSE);

            // TODO update app status to ongoing

            if (AppUmtsCallGetApp(
                node,
                callArriveInfo->appId,
                callArriveInfo->appSrcNodeId,
                &(appUmtsCallLayerData->appInfoTerminateList),
                &retAppInfo) == TRUE)
            {
                retAppInfo->appStatus = APP_UMTS_CALL_STATUS_ONGOING;
            }

            // accept the incoming call
            appUmtsCallLayerData->stats.numAppRequestRcvd++;

            Message* answerMsg;
            AppUmtsCallAnsweredMessageInfo* callAnswerInfo;

            answerMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_CELLULAR,
                            MSG_NETWORK_CELLULAR_FromAppCallAnswered);

            MESSAGE_InfoAlloc(
                node,
                answerMsg,
                sizeof(AppUmtsCallAnsweredMessageInfo));

            callAnswerInfo =
                (AppUmtsCallAnsweredMessageInfo*)
                    MESSAGE_ReturnInfo(answerMsg);

            callAnswerInfo->appId = callArriveInfo->appId;
            callAnswerInfo->transactionId = callArriveInfo->transactionId;
            callAnswerInfo->appSrcNodeId = callArriveInfo->appSrcNodeId;
            callAnswerInfo->appDestNodeId = callArriveInfo->appDestNodeId;

            // Usually it takes a user serveral seconds to answer a umts 
            MESSAGE_Send(node, answerMsg, 3 * SECOND);

            break;
        }

        case MSG_APP_CELLULAR_FromNetworkCallEndByRemote:
        {

            AppUmtsCallEndByRemoteMessageInfo* msgInfo;
            int appId;
            NodeAddress srcNodeId;
            NodeAddress destNodeId;
            AppUmtsCallLayerData* appUmtsCallLayerData;
            AppUmtsCallInfo* retAppInfo;

            msgInfo =
                (AppUmtsCallEndByRemoteMessageInfo*)
                    MESSAGE_ReturnInfo(msg);
            appUmtsCallLayerData =
                (AppUmtsCallLayerData*)
                    node->appData.umtscallVar;

            appId = msgInfo->appId;
            srcNodeId = msgInfo->appSrcNodeId;
            destNodeId = msgInfo->appDestNodeId;

            if (node->nodeId == srcNodeId)
            {
                // update the application status to success
                if (AppUmtsCallGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appUmtsCallLayerData->appInfoOriginList),
                    &retAppInfo) == TRUE)
                {
                    // cancel timer
                    if (retAppInfo->retryCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->retryCallTimer);

                        retAppInfo->retryCallTimer = NULL;
                    }
                    if (retAppInfo->startCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->startCallTimer);

                        retAppInfo->startCallTimer = NULL;
                    }
                    if (retAppInfo->talkStartTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->talkStartTimer);

                        retAppInfo->talkStartTimer = NULL;
                    }
                    if (retAppInfo->endCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->endCallTimer);

                        retAppInfo->endCallTimer = NULL;
                    }
                    retAppInfo->appStatus = APP_UMTS_CALL_STATUS_SUCCESSED;
                }

                appUmtsCallLayerData->stats.numOriginAppSuccessEnd++;
                if (DEBUG_APP)
                {
                    printf(
                        "node %d: MO/FO call ended by "
                        "remote user--APP DEST\n",
                        node->nodeId);
                }
            }
            else if (node->nodeId == destNodeId)
            {
                // update the application status to success
                if (AppUmtsCallGetApp(
                    node,
                    appId,
                    srcNodeId,
                    &(appUmtsCallLayerData->appInfoTerminateList),
                    &retAppInfo) == TRUE)
                {
                    // cancel timer
                    if (retAppInfo->retryCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->retryCallTimer);

                        retAppInfo->retryCallTimer = NULL;
                    }
                    if (retAppInfo->startCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->startCallTimer);

                        retAppInfo->startCallTimer = NULL;
                    }
                    if (retAppInfo->talkStartTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->talkStartTimer);

                        retAppInfo->talkStartTimer = NULL;
                    }
                    if (retAppInfo->endCallTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(
                            node,
                            retAppInfo->endCallTimer);

                        retAppInfo->endCallTimer = NULL;
                    }
                    retAppInfo->appStatus = APP_UMTS_CALL_STATUS_SUCCESSED;
                }

                if (node->nodeId == retAppInfo->appSrcNodeId)
                {
                    appUmtsCallLayerData->stats.numOriginAppSuccessEnd++;
                }
                else
                {
                    appUmtsCallLayerData->stats.numTerminateAppSuccessEnd++;
                }
                if (DEBUG_APP)
                {
                    printf(
                        "node %d: MT/FT call ended by "
                        "remote user--APP SRC\n",
                        node->nodeId);
                }
            }

            // update stats
            appUmtsCallLayerData->stats.numAppSuccessEnd++;
            if (DEBUG_APP)
            {
                printf(
                    "node %d: UMTS APP handle "
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
                    "node %d: UMTS-CALL APP handle"
                    "CELLULAR_APP_FROM_NETWORK_DROPPED\n",
                    node->nodeId);

            }

            AppUmtsCallHandleCallDroppedMsg(node, msg);
            break;
        }

        case MSG_APP_CELLULAR_FromNetworkPktArrive:
        {
            AppUmtsCallHandlePktArrival(node, msg);
            break;
        }
        default:
        {
            printf(
                "UMTS-CALL APP: Protocol = %d\n",
                MESSAGE_GetProtocol(msg));
            assert(FALSE);
            abort();
            break;
        }
    }

    MESSAGE_Free(node, msg);
}
