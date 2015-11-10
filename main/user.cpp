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

/*
 * This file contains initialization function, message processing
 * function, and finalize function used by application layer.
 */

#include "types.h"
#include "api.h"
#include "memory.h"
#include "node.h"
#include "partition.h"
#include "user.h"
#include "app_cbr.h"

#ifdef CELLULAR_LIB
#include "user_profile_parser.h"
#include "user_trafficpattern_parser.h"
#include "app_cellular_abstract.h"
#endif // CELLULAR_LIB

#ifdef UMTS_LIB // not fully implemented to support UMTS CALL
#include "app_umtscall.h"
#endif
#ifdef MUOS_LIB
#include "app_phonecall.h"
#endif 

#define DEBUG_USER 0
#define DEBUG_USER_APP 0
#define DEFAULT_ON 1
#define OUTPUT_USER_SEX_AGE 0
#define OUTPUT_USER_DISSATISFACTION 0


#ifdef CELLULAR_LIB
/*
 * NAME:        UserCurrentAppListPrint.
 * PURPOSE:     Print list of current ongoing applications at node.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
static
void UserCurrentAppListPrint(Node *node)
{
    UserAppInfo *userAppInfo;

    userAppInfo = node->userData->currentApps;
    while (userAppInfo)
    {
        printf("  APP %d | %s ",
               userAppInfo->appId,
               userAppInfo->appData->appName);

        /*if (strcmp(userAppInfo->appData->appName,
                   "CELLULAR-ABSTRACT-APP")==0)
        {
            CellularAppData* appData = (CellularAppData*) userAppInfo->
                                       appData;
            printf("type %d ",
                   (int) appData->appTypeDistribution.getRandomNumber());
            printf("to node %d\n", (int)
                   appData->destinationNodeDistribution.getRandomNumber());
        }*/

        userAppInfo = userAppInfo->next;
    }
}


/*
 * NAME:        UserCurrentAppListInsert.
 * PURPOSE:     Insert item into list of ongoing applications at node.
 * PARAMETERS:  node - pointer to the node,
 *              appId - application ID,
 *              trafficPattern - traffic pattern data of application.
 * RETURN:      none.
 */
static
void UserCurrentAppListInsert(Node *node,
                              int appId,
                              TrafficPatternBeh *trafficPattern,
                              BOOL retry)
{
    UserAppInfo *userAppInfo;
    UserAppInfo *tmpAppInfo;

    userAppInfo = (UserAppInfo*) MEM_malloc(sizeof(UserAppInfo));
    userAppInfo->appId = appId;
    userAppInfo->numRetries = 0;
    userAppInfo->retry = retry;
    userAppInfo->next = NULL;
    //point to the traffic pattern beh data in our list
    userAppInfo->patternBeh = trafficPattern;

    //copy over the actual app data,
    //since we don't want to change the parameters
    if (!strcmp(trafficPattern->appData->appName, "CELLULAR-ABSTRACT-APP")) {
        CellularAppData* cellularData =
            (CellularAppData*) MEM_malloc(sizeof(CellularAppData));
        userAppInfo->appData = cellularData;

        memcpy(userAppInfo->appData, trafficPattern->appData,
               sizeof(CellularAppData));
        // Set seeds
        cellularData->destinationNodeDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                0);
        cellularData->durationDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                1);
        cellularData->appTypeDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                2);
        cellularData->bandwidthDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                3);
    }

    if (node->userData->currentApps == NULL)
    {
        node->userData->currentApps = userAppInfo;
    }
    else
    {
        tmpAppInfo = node->userData->currentApps;
        while (tmpAppInfo->next)
        {
            tmpAppInfo = tmpAppInfo->next;
        }
        tmpAppInfo->next = userAppInfo;
    }
    if (DEBUG_USER)
    {
        printf("Inserted app %d into node %d's list:\n",
               appId, node->nodeId);
        UserCurrentAppListPrint(node);
    }
}


/*
 * NAME:        UserCurrentAppListDelete.
 * PURPOSE:     Remove item from list of ongoing applications at node.
 * PARAMETERS:  node - pointer to the node,
 *              appId - application ID.
 * RETURN:      none.
 */
static
void UserCurrentAppListDelete(Node *node, int appId)
{
    UserAppInfo *userAppInfo;
    UserAppInfo *tmpAppInfo;

    tmpAppInfo = NULL;
    userAppInfo = node->userData->currentApps;

    while (userAppInfo)
    {
        //hopefully the appId will be unique
        if (userAppInfo->appId == appId)
        {
            if (tmpAppInfo == NULL)
            {
                //we are at the first entry
                node->userData->currentApps = userAppInfo->next;
            }
            else
            {
                tmpAppInfo->next = userAppInfo->next;
            }
            MEM_free(userAppInfo->appData);
            MEM_free(userAppInfo);
            if (DEBUG_USER)
            {
                printf("Deleted app %d from list\n", appId);
            }
            return;
        }
        tmpAppInfo = userAppInfo;
        userAppInfo = userAppInfo->next;
    }
    if (DEBUG_USER)
    {
        printf("Couldn't delete app %d (not found)\n", appId);
    }
}


/*
 * NAME:        UserCurrentAppListFind.
 * PURPOSE:     Remove item from list of ongoing applications at node.
 * PARAMETERS:  node - pointer to the node,
 *              appId - application ID.
 * RETURN:      Application Information.
 */
static
UserAppInfo* UserCurrentAppListFind(Node *node, int appId)
{
    UserAppInfo *userAppInfo;

    userAppInfo = node->userData->currentApps;
    while (userAppInfo)
    {
        if (userAppInfo->appId == appId)
        {
            return userAppInfo;
        }
        userAppInfo = userAppInfo->next;
    }
    printf("APP ID = %d\n", appId);
    ERROR_ReportError("Couldn't find app %d in list\n");
    return NULL;
}


/*
 * NAME:        UserCurrentAppListDelete.
 * PURPOSE:     Free memory of list of ongoing applications at node.
 * PARAMETERS:  appList - pointer to the list of applications.
 * RETURN:      none.
 */
static
void FreeAppList(TrafficPatternBeh *appList)
{
    TrafficPatternBeh *iterator;
    TrafficPatternBeh *temp;
    iterator = appList;

    while (iterator)
    {
        MEM_free(iterator->distRetryInt);
        MEM_free(iterator->appData);

        temp = iterator;
        iterator = iterator->next;

        MEM_free(temp);
    }
}


/*
 * NAME:        USER_TurnOnPhone.
 * PURPOSE:     Handle notification of user and cellular layer
 *              when turning on phone.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
static
void USER_TurnOnPhone(Node *node)
{
    Message *notifyCellularLayerMsg;
    Message *startupDelayTimerMsg;

    //notify the cellular layer of phone on
    notifyCellularLayerMsg = MESSAGE_Alloc(node,
                                           NETWORK_LAYER,
                                           NETWORK_PROTOCOL_CELLULAR,
                                           MSG_CELLULAR_PowerOn);

    MESSAGE_Send(node, notifyCellularLayerMsg, 0);


    //the phone will take a little time to connect to the network, so
    //we should only turn our status to ON once the delay has passed
    startupDelayTimerMsg = MESSAGE_Alloc(node,
                                         USER_LAYER,
                                         0,
                                         MSG_USER_PhoneStartup);
    if (DEBUG_USER)
    {
        printf("Node %d turned phone ON.\n", node->nodeId);
    }

    MESSAGE_Send(node, startupDelayTimerMsg, USER_PHONE_STARTUP_DELAY);
}


/*
 * NAME:        USER_TurnOffPhone.
 * PURPOSE:     Handle notification of cellular layer
 *              when turning on phone.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
static
void USER_TurnOffPhone(Node *node)
{
    Message *notifyCellularLayerMsg;

    //notify the cellular layer of phone off
    notifyCellularLayerMsg = MESSAGE_Alloc(node,
                                           NETWORK_LAYER,
                                           NETWORK_PROTOCOL_CELLULAR,
                                           MSG_CELLULAR_PowerOff);

    MESSAGE_Send(node, notifyCellularLayerMsg, 0);

    if (DEBUG_USER)
    {
        printf("Node %d turned phone OFF.\n",node->nodeId);
    }
}

/*
 * NAME:        USER_ReadStatusInfo.
 * PURPOSE:     populate user's
 *              status list.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
static void USER_ReadStatusInfo(Node* node, const NodeInput* nodeInput)
{
    char errorString[MAX_STRING_LENGTH];
    char statusName[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    clocktype statusStartTime;
    int numStatus = 0;
    BOOL retVal;
    UserStatus* iterator = NULL;

    while (TRUE)
    {
        IO_ReadStringInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "USER-STATUS",
            numStatus,
            (numStatus == 0),
            &retVal,
            statusName);

        if (!retVal)
        {
            if (numStatus == 0)
            {
                strcpy(statusName, "default");
            }
            else
            {
                break;
            }
        }

        IO_ReadTimeInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "USER-STATUS-START-TIME",
            numStatus,
            (numStatus == 0),
            &retVal,
            &statusStartTime);

        if (!retVal)
        {
            statusStartTime = USER_DEFAULT_STATUS_START_TIME;
        }

        // now that we've read in status name and time, add entry to list
        UserStatus* statusPtr = (UserStatus*)MEM_malloc(sizeof(UserStatus));
        strncpy(statusPtr->name, statusName, MAX_STRING_LENGTH);
        statusPtr->startTime = statusStartTime;
        statusPtr->next = NULL;

        if (node->userData->statusList == NULL)
        {
            node->userData->statusList = statusPtr;
            iterator = node->userData->statusList;
        }
        else
        {
            iterator->next = statusPtr;
            iterator = iterator->next;
        }
        statusPtr = NULL;

        numStatus++;
    }

    if (DEBUG_USER)
    {
        printf("Node %d Read in the following status times:\n",
               node->nodeId);
        UserStatus* statusPtr = node->userData->statusList;
        while (statusPtr)
        {
            TIME_PrintClockInSecond(statusPtr->startTime, clockStr);
            printf("%s at %s seconds\n", statusPtr->name, clockStr);
            statusPtr = statusPtr->next;
        }
    }
}


/*
 * NAME:        USER_SendStatusMsg.
 * PURPOSE:     send status
 *              timer message.
 * PARAMETERS:  node - pointer to the node,
 * RETURN:      none.
 */
static
void USER_SendStatusMsg(Node *node)
{
    char clockStr[MAX_STRING_LENGTH];
    Message *statusTimerMsg;

    if (node->userData->statusList)
    {
        if (node->userData->statusList->startTime < node->getNodeTime())
        {
            ERROR_ReportError("Status start times out of order\n");
        }

        statusTimerMsg = MESSAGE_Alloc(node,
                                       USER_LAYER, //layer
                                       0,          //protocol
                                       MSG_USER_StatusChange);//type

        //we need to give it the time delay (startTime - now)
        MESSAGE_Send(node,
                     statusTimerMsg,
                     (node->userData->statusList->startTime -
                      node->getNodeTime()));

        TIME_PrintClockInSecond(node->userData->statusList->startTime,
                                clockStr);
        if (DEBUG_USER)
        {
            printf("Next status change %s to come at %s\n",
                   node->userData->statusList->name, clockStr);
        }
    }
    else
    {
        if (DEBUG_USER)
        {
            printf("Node %d: status list empty!\n", node->nodeId);
        }
    }
}
#endif // CELLULAR_LIB


/*
 * NAME:        USER_Initialize.
 * PURPOSE:     user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void USER_Initialize(Node *node, const NodeInput *nodeInput)
{
#ifdef CELLULAR_LIB
    char profileNameStr[MAX_STRING_LENGTH];
    UserProfileDataMappingRecord *userProfile;
    BOOL retVal;
    TrafficBehData *tempPattern;
    TrafficBehData *iterator;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "USER-PROFILE",
        &retVal,
        profileNameStr);

    if (!retVal) //user profile not used for this node
    {
        if (DEBUG_USER)
        {
            printf("Node %d not using user profile\n", node->nodeId);
        }
        node->userData->enabled = FALSE;
        return;
    }

    node->userData->enabled = TRUE;

    //copy over node seed to user object
    RANDOM_SetSeed(node->userData->seed,
                   node->globalSeed,
                   node->nodeId,
                   USER_LAYER);

    //initialize all nodes for cellular app use
    CellularAbstractApplicationLayerData *appCellularVar;
    Node* nextNode  = NULL;
    nextNode = node->partitionData->firstNode;
    while (nextNode != NULL)
    {
        appCellularVar = (CellularAbstractApplicationLayerData *)
                         nextNode->appData.appCellularAbstractVar;
        if (appCellularVar == NULL)
        {
            CellularAbstractAppInit(nextNode, nodeInput);
        }
        nextNode = nextNode->nextNodeData;
    }

    node->userData->statusList = NULL;
    //read in user status information from config file
    //populate user status list
    USER_ReadStatusInfo(node, nodeInput);

    //on/off status of phone at start of simulation
    // assume cellular layer3 is used.
    // Because USER model is used in the simulation
    // when DEFAULT ON is 1 turn on phone
    if (DEFAULT_ON)
    {
        USER_TurnOnPhone(node);
    }
    else
    {
        node->userData->phoneOn = FALSE;
    }

    if (DEBUG_USER)
    {
        printf("Looking up data for user profile %s\n", profileNameStr);
    }

    //look up the user profile name to make sure it exists
    userProfile = UserProfileDataMappingLookup(node, profileNameStr);

    if (userProfile == NULL)
    {
        ERROR_ReportError(
            "User Profile not found in hash table\n"
            "  make sure that USER-PROFILE-FILE is specified and"
            " that the profile exists in the file\n");
    }

    node->userData->age = (int)userProfile->data.distRecAge->getRandomNumber();
    node->userData->sex = (int)userProfile->data.distRecSex->getRandomNumber();

    if (DEBUG_USER)
    {
        printf("node %d has age %d and sex %d\n",
               node->nodeId, node->userData->age, node->userData->sex);
    }

    node->userData->trafficBehaviorList = NULL;
    node->userData->arrivalInterval = NULL;
    node->userData->appList = NULL;
    node->userData->currentApps = NULL;

    //we have age and sex, now set dissatisfaction degree and
    //initialize statistics keeping variables
    node->userData->dissatisfactionDegree = 0;
    node->userData->avgDissatisfactionDegree = 0;
    node->userData->totalAppsGenerated = 0;
    node->userData->totalAppsSuccessfullyFinished = 0;
    node->userData->totalAppsRejected = 0;
    node->userData->totalAppsDropped = 0;
    node->userData->totalRetries = 0;
    node->userData->avgRetriesPerApp = 0;

    node->userData->numActiveApps = 0;

    //we need to copy over the traffic behavior list
    node->userData->trafficBehaviorList = userProfile->data.trafficBehData;

    if (node->userData->trafficBehaviorList == NULL)
    {
        printf("user profile = %s\n",userProfile->data.userProfileName);
        ERROR_ReportError(
            "No Traffic Behavior List for user profile\n");
    }

    iterator = NULL;

    if (DEBUG_USER)
    {
        tempPattern = node->userData->trafficBehaviorList;
        printf("Retrieved traffic behavior list:\n");
        while (tempPattern)
        {
            printf("%s  %s\n",
                   tempPattern->name,
                   tempPattern->statusName);
            tempPattern = tempPattern->next;
        }
    }

    USER_SendStatusMsg(node);
#endif // CELLULAR_LIB
}


#ifdef CELLULAR_LIB
// /**
// FUNCTION   :: USER_SetApplicationArrival
// LAYER      :: USER
// PURPOSE    :: Schedule an application arrival time.
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// RETURN     :: void : NULL
// **/
void USER_SetApplicationArrival(Node *node)
{
    clocktype appArrival;
    char clockStr[MAX_STRING_LENGTH];
    Message *arrivalIntervalTimerMsg;

    appArrival = node->userData->arrivalInterval->getRandomNumber();

    TIME_PrintClockInSecond(appArrival+node->getNodeTime(), clockStr);
    //make sure this next application arrives within this behavior duration
    //don't send applications that will arrive after this duration
    if (node->userData->behaviorDuration > (appArrival+node->getNodeTime()))
    {
        arrivalIntervalTimerMsg = MESSAGE_Alloc(node,
                                                USER_LAYER,
                                                0,
                                                MSG_USER_ApplicationArrival);

        if (DEBUG_USER)
        {
            printf("Next application to arrive at %s\n\n", clockStr);
        }
        MESSAGE_Send(node, arrivalIntervalTimerMsg, appArrival);
    }
    else
    {
        if (DEBUG_USER)
        {
            printf("Next app arrival at %s would be after the duration\n\n",
                   clockStr);
        }
    }
}

// /**
// FUNCTION   :: USER_SetTrafficPattern
// LAYER      :: USER
// PURPOSE    :: Set a user's traffic pattern based on its profile.
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// RETURN     :: void : NULL
// **/
void USER_SetTrafficPattern(Node *node)
{
    char clockStr[MAX_STRING_LENGTH];
    TrafficBehData *trafficPattern;
    TrafficPatternDataMappingRecord *trafficPatternRecord;
    TrafficPatternBeh *tempAppData;
    TrafficPatternBeh *iterator;
    TrafficPatternBeh *appData;

    //pick a newtraffic pattern to use
    trafficPattern = node->userData->trafficBehaviorList;

    while (trafficPattern &&
            (strcmp(trafficPattern->statusName,
                    node->userData->statusList->name) != 0))
    {
        trafficPattern = trafficPattern->next;
    }

    //if we cannot find traffic pattern, look for a default traffic pattern
    if (trafficPattern == NULL)
    {
        trafficPattern = node->userData->trafficBehaviorList;

        while (trafficPattern &&
                (strcmp(trafficPattern->statusName, "default") != 0))
        {
            trafficPattern = trafficPattern->next;
        }

        //no default, so exit function without scheduling applications
        if (trafficPattern == NULL)
        {
            if (DEBUG_USER)
            {
                printf("Status %s not found in traffic "
                       "pattern list for node %d\n",
                       node->userData->statusList->name, node->nodeId);
            }
            return;
        }
        else if (DEBUG_USER)
        {
            printf("Status %s not found, using default...\n",
                   node->userData->statusList->name);
        }
    }

    //set duration to the start of the next status
    if (node->userData->statusList->next)
    {
        node->userData->behaviorDuration =
            node->userData->statusList->next->startTime;
    }
    else
    {
        node->userData->behaviorDuration = CLOCKTYPE_MAX;
    }

    if (DEBUG_USER)
    {
        TIME_PrintClockInSecond(node->userData->behaviorDuration, clockStr);
        printf("Status change %s, Using Traffic Pattern %s\n",
               node->userData->statusList->name, trafficPattern->name);
        printf("Pattern to last until %s\n", clockStr);
    }

    //now that we have figured out the duration, we need to figure out the
    //max num apps and copy over the arrival interval distribution
    trafficPatternRecord =
        TrafficPatternDataMappingLookup(node, trafficPattern->name);

    if (trafficPatternRecord == NULL)
    {
        ERROR_ReportError(
            "Traffic pattern not found in hash table\n");
    }

    if (DEBUG_USER)
    {
        printf("Found Traffic Pattern Record %s\n",
               trafficPatternRecord->data.trafficPatternName);
    }

    if (trafficPatternRecord->data.numAppTypes < 1)
    {
        if (DEBUG_USER)
        {
            printf("No apps for %s, so no applciations "
                   "will be generated for this pattern\n",
                   trafficPatternRecord->data.trafficPatternName);
        }
        return;
    }

    node->userData->maxNumApps = (int)trafficPatternRecord->data.distMaxNumApp->getRandomNumber();

    if (DEBUG_USER)
    {
        printf("%d max num apps for this duration\n",
               node->userData->maxNumApps);
    }

    //allocate memory if necessary
    if (node->userData->arrivalInterval == NULL)
    {
        node->userData->arrivalInterval =
            (RandomDistribution<clocktype>*) MEM_malloc(
                sizeof(RandomDistribution<clocktype>));
    }

    memcpy(node->userData->arrivalInterval,
           trafficPatternRecord->data.distArrivalInt,
           sizeof(RandomDistribution<clocktype>));

    node->userData->arrivalInterval->setSeed(node->globalSeed,
            node->nodeId,
            0,
            0);

    //we've copied over the arrival interval distribution
    //now copy over the list of application details
    appData = trafficPatternRecord->data.trafficBeh;
    if (appData == NULL)
    {
        ERROR_ReportError("No Application Information for pattern\n");
    }

    //need to free the old appList before copying in the new one
    FreeAppList(node->userData->appList);
    node->userData->appList = NULL;
    iterator = NULL;

    int appId = 0;
    while (appData)
    {
        tempAppData =
            (TrafficPatternBeh*) MEM_malloc(sizeof(TrafficPatternBeh));

        tempAppData->probability = appData->probability;
        tempAppData->retryProbability = appData->retryProbability;

        tempAppData->distRetryInt =
            (RandomDistribution<clocktype>*) MEM_malloc(
                sizeof(RandomDistribution<clocktype>));

        memcpy(tempAppData->distRetryInt, appData->distRetryInt,
               sizeof(RandomDistribution<clocktype>));

        tempAppData->distRetryInt->setSeed(node->globalSeed,
                                           node->nodeId,
                                           appId,
                                           4);

        tempAppData->maxNumRetry = appData->maxNumRetry;

        CellularAppData* cellularData =
            (CellularAppData*) MEM_malloc(sizeof(CellularAppData));
        tempAppData->appData = cellularData;
        memcpy(tempAppData->appData, appData->appData,
               sizeof(CellularAppData));
        // Set seeds
        cellularData->destinationNodeDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                0);
        cellularData->durationDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                1);
        cellularData->appTypeDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                2);
        cellularData->bandwidthDistribution.setSeed(node->globalSeed,
                node->nodeId,
                appId,
                3);

        tempAppData->next = NULL;

        if (node->userData->appList == NULL)
        {
            node->userData->appList = tempAppData;
            iterator = node->userData->appList;
        }
        else
        {
            iterator->next = tempAppData;
            iterator = iterator->next;
        }
        appData = appData->next;
        appId++;
    }

    iterator = NULL;
    tempAppData = node->userData->appList;

    if (DEBUG_USER)
    {
        printf("Retrieved Application Details:\n");

        //everything should be copied over by now, let's check
        while (tempAppData)
        {
            printf("%f  %f  %i\n",
                   tempAppData->probability,
                   tempAppData->retryProbability,
                   tempAppData->maxNumRetry);
            printf("%hi %s  ", tempAppData->appData->numAppParams,
                   tempAppData->appData->appName);
            printf("\n");
            tempAppData = tempAppData->next;
        }
    }

    USER_SetApplicationArrival(node);
}


/*
 * NAME:        USER_UpdateDissatisfaction.
 * PURPOSE:     update user's dissatisfaction degree.
 * PARAMETERS:  node - pointer to the node,
 *              dissatisfaction - amount to update dissatisfaction.
 * RETURN:      none.
 */
void USER_UpdateDissatisfaction(Node *node, double dissatisfaction)
{
    node->userData->dissatisfactionDegree += dissatisfaction;
    if (node->userData->dissatisfactionDegree < 0)
    {
        node->userData->dissatisfactionDegree = 0;
    }
    else if (node->userData->dissatisfactionDegree > 1)
    {
        node->userData->dissatisfactionDegree = 1;
    }

    if (DEBUG_USER)
    {
        if (dissatisfaction < 0)
        {
            printf("I'm getting more satisfied: %f\n",
                   node->userData->dissatisfactionDegree);
        }
        else
        {
            printf("I'm getting dissatisfied: %f\n",
                   node->userData->dissatisfactionDegree);
        }
    }
}


/*
 * NAME:        USER_HandleCallRetry.
 * PURPOSE:     handle retry of dropped/rejected app.
 * PARAMETERS:  node - pointer to the node,
 *              appId - application ID.
 * RETURN:      none.
 */
void USER_HandleCellularRetry(Node *node, int appId)
{
    double random;
    char clockStr[MAX_STRING_LENGTH];
    char timeStr[MAX_STRING_LENGTH];
    UserAppInfo *appInfo;
    clocktype retryInterval;

    //attempt retry
    appInfo = UserCurrentAppListFind(node, appId);
    if (appInfo == NULL)
    {
        printf("APP ID: %d\n",appId);
        ERROR_ReportError("Cannot find application in"
                          "user's current app list\n");
    }

    if (!appInfo->retry)
    {
        //this application has a duration longer than the traffic
        //behavior duration, so we will not retry it
        UserCurrentAppListDelete(node, appId);
        node->userData->numActiveApps--;
        if (DEBUG_USER)
        {
            printf("NO RETRY\n");
            printf("num active apps: %d\n",
                   node->userData->numActiveApps);
        }
        return;
    }

    //get random number to see if we decide on a retry
    //check to see that we have not exceeded max retries
    //if both true, we can initiate the retry
    random = RANDOM_erand(node->userData->seed);

    if (DEBUG_USER)
    {
        printf("Picking random number to decide retry %f\n", random);
    }

    retryInterval = appInfo->patternBeh->distRetryInt->getRandomNumber();

    if ((random < appInfo->patternBeh->retryProbability) &&
            (appInfo->numRetries < appInfo->patternBeh->maxNumRetry) &&
            ((retryInterval + node->getNodeTime()) <
             node->userData->behaviorDuration) &&
            node->userData->phoneOn)
    {
        NodeAddress destinationNode;
        clocktype appDuration;
        CellularAbstractApplicationType appType;
        double bandwidth;

        //initiate retry
        //find out when we will retry
        //only do it if this occurs before the end of the duration
        CellularAppData* appData = (CellularAppData*) appInfo->appData;
        destinationNode = appData->destinationNodeDistribution.getRandomNumber();
        appDuration = appData->durationDistribution.getRandomNumber();
        appType = (CellularAbstractApplicationType)
                  appData->appTypeDistribution.getRandomNumber();
        bandwidth = appData->bandwidthDistribution.getRandomNumber();

        CellularAbstractApplicationLayerData *appCellularVar;
        appCellularVar =
            (CellularAbstractApplicationLayerData *)
            node->appData.appCellularAbstractVar;

        if (DEBUG_USER_APP)
        {
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            TIME_PrintClockInSecond(appDuration, timeStr);
            printf("CELLULAR-ABSTRACT-APP %d %d %s %s ",
                   node->nodeId, destinationNode, clockStr, timeStr);

            switch (appType)
            {
            case CELLULAR_ABSTRACT_VOICE_PHONE:
                printf("VOICE "); break;
            case CELLULAR_ABSTRACT_VIDEO_PHONE:
                printf("VIDEO-PHONE "); break;
            case CELLULAR_ABSTRACT_TEXT_MAIL:
                printf("TEXT-MAIL "); break;
            case CELLULAR_ABSTRACT_PICTURE_MAIL:
                printf("PICTURE-MAIL "); break;
            case CELLULAR_ABSTRACT_ANIMATION_MAIL:
                printf("ANIMATION-MAIL "); break;
            case CELLULAR_ABSTRACT_WEB:
                printf("WEB "); break;
            default:
                ERROR_ReportError(
                    "Unknown application type!\n");
                break;
            }
            printf("%f\n", bandwidth);
        }

        CellularAbstractAppInsertList(
            node,
            &(appCellularVar->appInfoOriginList),
            appCellularVar->numOriginApp,
            node->nodeId, destinationNode,
            retryInterval, appDuration,
            appType, 1,
            bandwidth, TRUE);
        appInfo->appId = appCellularVar->numOriginApp;
        appInfo->numRetries++;
        appInfo->retry =
            ((node->getNodeTime() + retryInterval + appDuration) <
             node->userData->behaviorDuration);

        node->userData->totalRetries++;

        TIME_PrintClockInSecond(retryInterval, clockStr);
        TIME_PrintClockInSecond(retryInterval + node->getNodeTime(),
                                timeStr);

        if (DEBUG_USER)
        {
            printf("RETRYING in %s seconds (at %s)"
                   " with new app ID %d\n\n",
                   clockStr, timeStr, appInfo->appId);
        }
    }
    else
    {
        //get rid of this entry
        UserCurrentAppListDelete(node, appId);
        node->userData->numActiveApps--;
        if (DEBUG_USER)
        {
            printf("NO RETRY\n");
            printf("num active apps: %d\n",
                   node->userData->numActiveApps);
        }
    }
}
#endif // CELLULAR_LIB


// /**
// FUNCTION   :: USER_HandleCallUpdate
// LAYER      :: USER
// PURPOSE    :: Reaction to the status change of an application session
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + appStatus : UserApplicationStatus : New status of the app session
// RETURN     :: void : NULL
// **/
void USER_HandleCallUpdate(Node *node, int appId,
                           UserApplicationStatus appStatus)
{
#ifdef CELLULAR_LIB
    switch (appStatus)
    {
    case USER_CALL_COMPLETED:
    {
        //call completed, update dissatisfaction
        USER_UpdateDissatisfaction(node, USER_DECREASE_DISSATISFACTION);
        node->userData->totalAppsSuccessfullyFinished++;
        node->userData->numActiveApps--;
        if (DEBUG_USER)
        {
            printf("Call Completed\n");
            printf("num active apps: %d\n",
                   node->userData->numActiveApps);
        }

        //remove this app info from our list
        UserCurrentAppListDelete(node, appId);
        break;
    }
    case USER_CALL_REJECTED:
    {
        if (DEBUG_USER)
        {
            printf("Call REJECTED: \n");
        }
        node->userData->totalAppsRejected++;
        //call rejected, update dissatisfaction
        USER_UpdateDissatisfaction(node, USER_INCREASE_DISSATISFACTION);

        //retry?
        USER_HandleCellularRetry(node, appId);
        break;
    }
    case USER_CALL_DROPPED:
    {
        if (DEBUG_USER)
        {
            printf("Call DROPPED: \n");
        }
        node->userData->totalAppsDropped++;
        //call rejected, update dissatisfaction
        USER_UpdateDissatisfaction(node, USER_INCREASE_DISSATISFACTION);

        //retry?
        USER_HandleCellularRetry(node, appId);
        break;
    }
    default:
    {
        ERROR_ReportError(
            "Unknown application status!\n");
        break;
    }
    }
#endif // CELLULAR_LIB
}


#ifdef CELLULAR_LIB
/*
 * NAME:        USER_StartCellularApp.
 * PURPOSE:     generate new cellular application.
 * PARAMETERS:  node - pointer to the node,
 *              patternBeh - pointer to the traffic pattern behavior.
 * RETURN:      none.
 */
void USER_StartCellularApp(Node *node, TrafficPatternBeh *patternBeh)
{
    char errorString[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    char timeStr[MAX_STRING_LENGTH];
    NodeId destinationNode;
    clocktype appDuration;
    CellularAbstractApplicationType appType;
    double bandwidth;

    if (node->userData->phoneOn)
    {
        if (patternBeh->appData->numAppParams != 4)
        {
            ERROR_ReportError(
                "CELLULAR ABSTRACT APP needs 4 parameters\n");
        }

        //infinite loop if node's only destination is itself
        do
        {
            destinationNode = ((CellularAppData*)patternBeh->appData)->destinationNodeDistribution.getRandomNumber();

            if (!PARTITION_NodeExists(node->partitionData,
                                      destinationNode))
            {
                sprintf(
                    errorString,
                    "Node %d does not exist",
                    destinationNode);
                ERROR_ReportError(errorString);
            }
        } while (destinationNode == node->nodeId);

        appDuration = ((CellularAppData*)patternBeh->appData)->durationDistribution.getRandomNumber();
        appType = (CellularAbstractApplicationType)((CellularAppData*)patternBeh->appData)->appTypeDistribution.getRandomNumber();

        if ((appType != CELLULAR_ABSTRACT_VOICE_PHONE) &&
                (appType != CELLULAR_ABSTRACT_VIDEO_PHONE) &&
                (appType != CELLULAR_ABSTRACT_TEXT_MAIL) &&
                (appType != CELLULAR_ABSTRACT_PICTURE_MAIL) &&
                (appType != CELLULAR_ABSTRACT_ANIMATION_MAIL) &&
                (appType != CELLULAR_ABSTRACT_WEB))
        {
            ERROR_ReportError("Incorrect Application Type\n");
        }

        bandwidth = ((CellularAppData*)patternBeh->appData)->bandwidthDistribution.getRandomNumber();

        if (DEBUG_USER)
        {
            TIME_PrintClockInSecond(appDuration, clockStr);
            printf(" Node %d calling node %d with %s\n",
                   node->nodeId, destinationNode,
                   patternBeh->appData->appName);
            printf("   for %s seconds with app type %d"
                   " and bandwidth %f\n",
                   clockStr, appType, bandwidth);
        }

        //start the application
        CellularAbstractApplicationLayerData *appCellularVar;
        appCellularVar =
            (CellularAbstractApplicationLayerData *)
            node->appData.appCellularAbstractVar;

        if (DEBUG_USER_APP)
        {
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            TIME_PrintClockInSecond(appDuration, timeStr);
            printf("CELLULAR-ABSTRACT-APP %d %d %s %s ",
                   node->nodeId,
                   destinationNode,
                   clockStr,
                   timeStr);

            switch (appType)
            {
            case CELLULAR_ABSTRACT_VOICE_PHONE:
                printf("VOICE "); break;
            case CELLULAR_ABSTRACT_VIDEO_PHONE:
                printf("VIDEO-PHONE "); break;
            case CELLULAR_ABSTRACT_TEXT_MAIL:
                printf("TEXT-MAIL "); break;
            case CELLULAR_ABSTRACT_PICTURE_MAIL:
                printf("PICTURE-MAIL "); break;
            case CELLULAR_ABSTRACT_ANIMATION_MAIL:
                printf("ANIMATION-MAIL "); break;
            case CELLULAR_ABSTRACT_WEB:
                printf("WEB "); break;
            default:
                ERROR_ReportError(
                    "Unknown application type!\n");
                break;
            }
            printf("%f\n", bandwidth);
        }

        CellularAbstractAppInsertList(
            node,
            &(appCellularVar->appInfoOriginList),
            appCellularVar->numOriginApp,
            node->nodeId, destinationNode,
            (clocktype)0, appDuration,
            appType, 1,
            bandwidth, TRUE);

        //save this app's info into the list
        //Right now the code is pretty coupled with cellular app
        //we are using the cellular app's appId for ourselves
        //in order to make it more universal, we should probably
        //assign our own stuff, however it is convenient now
        //since appId is contained in the messages that cellular
        //app is sending to itself, so we just forward it

        //WARNING, here is a dependency on cellular_abstract
        //which sets appId to appCellularVar->numOriginApp + 1
        //if this were to change, there would be a side effect:
        //the user's appId would be off of the real one kept at
        //the application layer
        //since CellularAbstractAppInsertList has already
        //incremented numOriginApp, we can use it directly
        //w/o incrementing
        UserCurrentAppListInsert(node,
                                 appCellularVar->numOriginApp,
                                 patternBeh,
                                 ((node->getNodeTime() + appDuration) <
                                  node->userData->behaviorDuration));

        node->userData->numActiveApps++;
        node->userData->totalAppsGenerated++;

        if (DEBUG_USER)
        {
            printf("  num active apps: %d\n",
                   node->userData->numActiveApps);
        }
    }
    else
    {
        if (DEBUG_USER)
        {
            printf("Phone is off, app not generated\n");
        }
    }
}


/*
 * NAME:        USER_StartCBRApp.
 * PURPOSE:     generate new CBR application.
 * PARAMETERS:  node - pointer to the node,
 *              patternBeh - pointer to the traffic pattern behavior.
 * RETURN:      none.
 */
void USER_StartCBRApp(Node *node, TrafficPatternBeh *patternBeh)
{
    char errorStr[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    char intervalStr[MAX_STRING_LENGTH];
    NodeId destinationNode;
    int numItems;
    int itemSize;
    clocktype interval;
    clocktype appDuration;

    CbrAppData* cbrData = (CbrAppData*) patternBeh->appData;

    if (node->userData->phoneOn)
    {
        if (patternBeh->appData->numAppParams != 5)
        {
            ERROR_ReportError(
                "CBR APP needs 5 parameters\n");
        }

        //infinite loop if node's only destination is itself
        do
        {
            destinationNode = cbrData->destinationNodeDistribution.getRandomNumber();

            if (!PARTITION_NodeExists(node->partitionData,
                                      destinationNode))
            {
                sprintf(
                    errorStr,
                    "Node %d does not exist",
                    destinationNode);
                ERROR_ReportError(errorStr);
            }
        } while (destinationNode == node->nodeId);

        numItems = cbrData->numItemsDistribution.getRandomNumber();
        itemSize = cbrData->itemSizeDistribution.getRandomNumber();
        interval = cbrData->intervalDistribution.getRandomNumber();
        appDuration = cbrData->durationDistribution.getRandomNumber();

        if (DEBUG_USER)
        {
            TIME_PrintClockInSecond(appDuration, clockStr);
            TIME_PrintClockInSecond(interval, intervalStr);
            printf(" Node %d calling node %d with %s\n",
                   node->nodeId, destinationNode,
                   patternBeh->appData->appName);
            printf("   sending %d items of size %d with interval %s"
                   " and duration %s\n",
                   numItems, itemSize, intervalStr, clockStr);
        }

        //start the application

        if (DEBUG_USER_APP)
        {
            TIME_PrintClockInSecond(appDuration, clockStr);
            TIME_PrintClockInSecond(interval, intervalStr);
            printf("CBR %d %d %d %d %s %s\n",
                   node->nodeId,
                   destinationNode,
                   numItems,
                   itemSize,
                   intervalStr,
                   clockStr);
        }

        Node *destNode;
        IdToNodePtrMap* nodeHash;
        Address sourceAddr;
        Address destAddr;

        sourceAddr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                         node, (NodeAddress)(node->nodeId));

        nodeHash = node->partitionData->nodeIdHash;
        destNode = MAPPING_GetNodePtrFromHash(nodeHash, destinationNode);
        destAddr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                       destNode, (NodeAddress)destinationNode);

        stringstream srcString;
        stringstream destString;

        srcString << node->nodeId;
        destString << destinationNode;

        AppCbrClientInit(
            node,
            sourceAddr,
            destAddr,
            numItems,
            itemSize,
            interval,
            node->getNodeTime(),
            node->getNodeTime() + appDuration,
            APP_DEFAULT_TOS,
            srcString.str().c_str(),
            destString.str().c_str(),
            FALSE,
            NULL);


        AppCbrServerInit(destNode);

        //save this app's info into the list
        UserCurrentAppListInsert(node,
                                 node->userData->totalAppsGenerated,
                                 patternBeh,
                                 ((node->getNodeTime() + appDuration) <
                                  node->userData->behaviorDuration));

        if (DEBUG_USER)
        {
            printf("  num active apps: %d\n",
                   node->userData->numActiveApps);
        }

        //send message to self for when app finishes
        AppTimer *timer;
        Message *timerMsg;

        timerMsg = MESSAGE_Alloc(node,
                                 USER_LAYER,
                                 0,
                                 MSG_USER_TimerExpired);

        MESSAGE_InfoAlloc(node, timerMsg, sizeof(AppTimer));

        timer = (AppTimer *)MESSAGE_ReturnInfo(timerMsg);
        timer->type = USER_CALL_COMPLETED;
        //save the appId
        timer->connectionId = node->userData->totalAppsGenerated;

        MESSAGE_Send(node, timerMsg, appDuration);

        node->userData->numActiveApps++;
        node->userData->totalAppsGenerated++;
    }
    else
    {
        if (DEBUG_USER)
        {
            printf("Phone is off, app not generated\n");
        }
    }
}


// /**
// FUNCTION   :: USER_HandleUserLayerEvent
// LAYER      :: USER
// PURPOSE    :: Handle messages and events for user layer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : The event
// RETURN     :: void : NULL
// **/
void USER_HandleUserLayerEvent(Node *node, Message *msg)
{
    double random;
    TrafficPatternBeh *patternBeh;
    UserStatus *status;

    switch (msg->eventType)
    {
    case MSG_USER_PhoneStartup:
    {
        if (DEBUG_USER)
        {
            printf("Turning on phone...\n");
        }
        node->userData->phoneOn = TRUE;
        break;
    }
    case MSG_USER_StatusChange:
    {
        if (DEBUG_USER)
        {
            printf("Status change...\n");
        }
        //set the traffic pattern based on the status
        USER_SetTrafficPattern(node);

        //since we have started the pattern based on this status
        //we can now get rid of the status entry
        status = node->userData->statusList;
        node->userData->statusList = node->userData->statusList->next;
        MEM_free(status);

        //send the message for the next status
        USER_SendStatusMsg(node);
        break;
    }
    case MSG_USER_ApplicationArrival:
    {
        if (DEBUG_USER)
        {
            printf("Generating new application:\n");
        }

        //only generate new app if we have not reached the max
        if (node->userData->numActiveApps < node->userData->maxNumApps)
        {
            //pick a random application from the list
            patternBeh = node->userData->appList;
            random = RANDOM_erand(node->userData->seed);

            if (DEBUG_USER)
            {
                printf("Picking Random Application %f\n", random);
            }

            if (patternBeh->probability == 0)
            {
                printf("probability should be greater than 0\n");
                break;
            }

            while (patternBeh &&
                    ((random - patternBeh->probability) > 0))
            {
                random -= patternBeh->probability;
                patternBeh = patternBeh->next;
            }

            if (patternBeh == NULL)
            {
                patternBeh = node->userData->appList;
                ERROR_ReportWarning(
                    "Traffic Pattern Probabilities do not add up to 1\n");
            }

            //we've picked the application
            //create the application
            if (strcmp(patternBeh->appData->appName,
                       "CELLULAR-ABSTRACT-APP") == 0)
            {
                USER_StartCellularApp(node, patternBeh);
            }
            else if (strcmp(patternBeh->appData->appName,
                            "CBR") == 0)
            {
                USER_StartCBRApp(node, patternBeh);
            }
            else
            {
                ERROR_ReportError(
                    "Can only process cellular app at this point\n");
            }
        }
        else
        {
            if (DEBUG_USER)
            {
                printf("Max Num Apps Reached!"
                       " Cannot initiate more until one finishes.\n");
            }
        }

        //pick new arrival time for next application
        USER_SetApplicationArrival(node);
        break;
    }
    case MSG_USER_TimerExpired:
    {
        AppTimer *timer;
        timer = (AppTimer *) MESSAGE_ReturnInfo(msg);

        switch (timer->type)
        {
        case USER_CALL_COMPLETED:
        {
            int appId = timer->connectionId;

            if (DEBUG_USER)
            {
                printf("CBR call Completed\n");
            }

            USER_HandleCallUpdate(node, appId, USER_CALL_COMPLETED);

            break;
        }
        default:
            ERROR_ReportWarning(
                "Unknown timer type recieved at user\n");
            break;
        }
        break;
    }
    default:
        ERROR_ReportWarning(
            "Unknown message eventType recieved at user\n");
        break;
    }

    MESSAGE_Free(node, msg);
}
#endif // CELLULAR_LIB


/*
 * NAME:        USER_ProcessEvent.
 * PURPOSE:     call proper protocol to process messages.
 * PARAMETERS:  node - pointer to the node,
 *              msg - pointer to the received message.
 * RETURN:      none.
 */
void USER_ProcessEvent(Node *node, Message *msg)
{
    clocktype now;
    char clockStr[MAX_STRING_LENGTH];
    now = node->getNodeTime();
    TIME_PrintClockInSecond(now, clockStr);

    switch (msg->layerType)
    {
    case USER_LAYER:
    {
        if (DEBUG_USER)
        {
            printf("Node %d got a message from USER_LAYER at %s\n",
                   node->nodeId, clockStr);
        }
#ifdef CELLULAR_LIB
        USER_HandleUserLayerEvent(node, msg);
#endif // CELLULAR_LIB
        break;
    }
    default:
        printf("Protocol = %d\n", MESSAGE_GetProtocol(msg));
        ERROR_ReportError("Unknown message layer type\n");
        break;
    }//switch//
}


/*
 * NAME:        USER_Finalize.
 * PURPOSE:     call protocols to print statistics.
 * PARAMETERS:  node - pointer to the node,
 * RETURN:      none.
 */
void
USER_Finalize(Node *node)
{
#ifdef CELLULAR_LIB
    //free memory for each node
    UserStatus *statusList;
    UserStatus *tempStatus;
    TrafficPatternBeh *patternList;
    TrafficPatternBeh *tempPattern;
    UserAppInfo *appInfo;
    UserAppInfo *tempInfo;

    if (node->userData->enabled == FALSE)
    {
        return;
    }

    statusList = node->userData->statusList;
    while (statusList)
    {
        tempStatus = statusList;
        statusList = statusList->next;
        MEM_free(tempStatus);
    }
    patternList = node->userData->appList;
    while (patternList)
    {
        tempPattern = patternList;
        patternList = patternList->next;
        MEM_free(tempPattern);
    }
    appInfo = node->userData->currentApps;
    while (appInfo)
    {
        tempInfo = appInfo;
        appInfo = appInfo->next;
        MEM_free(tempInfo);
    }
    MEM_free(node->userData->arrivalInterval);


    //for user profile analysis
    if (OUTPUT_USER_SEX_AGE)
    {
        printf("%d, %d, %d\n", node->nodeId, node->userData->sex,
               node->userData->age);
    }

    if (OUTPUT_USER_DISSATISFACTION)
    {
        printf("%d, %d\n", node->nodeId,
               (int)(node->userData->dissatisfactionDegree*100));
    }

    //output stats
    char buf[MAX_STRING_LENGTH];

    if (node->userData->totalAppsGenerated == 0)
    {
        node->userData->avgDissatisfactionDegree = 0;
    }
    else
    {
        node->userData->avgDissatisfactionDegree =
            node->userData->dissatisfactionDegree/node->
            userData->totalAppsGenerated;
    }
    sprintf(
        buf,
        "Average Dissatisfaction = %d%%",
        (int)(node->userData->avgDissatisfactionDegree * 100 ));

    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    sprintf(
        buf,
        "Applications Generated = %d",
        node->userData->totalAppsGenerated);
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    sprintf(
        buf,
        "Applications Successfully Finished = %d",
        node->userData->totalAppsSuccessfullyFinished);
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    sprintf(
        buf,
        "Applications Rejected = %d",
        node->userData->totalAppsRejected);
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    sprintf(
        buf,
        "Applications Dropped = %d",
        node->userData->totalAppsDropped);
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    sprintf(
        buf,
        "Retries = %d",
        node->userData->totalRetries);
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);

    if (node->userData->totalAppsGenerated == 0)
    {
        sprintf(
            buf,
            "Average Retries per App = 0");
    }
    else
    {
        sprintf(
            buf,
            "Average Retries per App = %f",
            (double)node->userData->totalRetries/node->
            userData->totalAppsGenerated);
    }
    IO_PrintStat(
        node,
        "USER",
        "CELLULAR",
        ANY_DEST,
        node->nodeId,
        buf);
#endif // CELLULAR_LIB
}
