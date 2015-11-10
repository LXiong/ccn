
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "phy.h"
#include "network.h"
#include "network_ip.h"

#include "ipv6.h"

#include "mac_dot16.h"
#include "mac_dot16e.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_phy.h"
#include "mac_dot16_sch.h"
#include "phy_dot16.h"

#define DEBUG                    0
#define DEBUG_NETWORK_ENTRY      0

#define DEBUG_TIMER              0
#define DEBUG_PARAMETER          0
#define DEBUG_CHANNEL            0
#define DEBUG_PACKET             0
#define DEBUG_PACKET_DETAIL      0

#define DEBUG_RANGING            0
#define DEBUG_FLOW               0
#define DEBUG_BWREQ              0

#define DEBUG_NBR_SCAN           (0 && node->nodeId == DEBUG_SPECIFIC_NODE)
#define DEBUG_NBR_SCAN_DETAIL    (0 && node->nodeId == DEBUG_SPECIFIC_NODE)
#define DEBUG_HO                 (0 && node->nodeId == DEBUG_SPECIFIC_NODE)
#define DEBUG_HO_DETAIL          (0 && node->nodeId == DEBUG_SPECIFIC_NODE)
#define DEBUG_SPECIFIC_NODE      0

#define DEBUG_PACKING_FRAGMENTATION    0
#define DEBUG_CRC                      0
#define DEBUG_CDMA_RANGING             0
#define DEBUG_SLEEP                    0// && (node->nodeId == 3)
#define DEBUG_SLEEP_FRAME_NUMBER       0// && (node->nodeId == 2)
#define DEBUG_SLEEP_PACKET             0// && (node->nodeId == 3)
#define DEBUG_SLEEP_PS_CLASS_3         0// && (node->nodeId == 2)

#define DEBUG_ARQ                      0// && (node->nodeId == 0)
#define DEBUG_ARQ_INIT                 0
#define DEBUG_ARQ_WINDOW               0
#define DEBUG_ARQ_TIMER                0// && (node->nodeId == 0)
#define DEBUG_IDLE                     0// && (node->nodeId == 26)

#define PRINT_WARNING            0
#define PIGGYBACK_ENABLED        0


// Define prototype
static
void MacDot16SsRestartOperation(Node*, MacDataDot16*);
static
void MacDot16SsFreeServiceFlowList(Node* node, MacDataDot16* dot16);
static
void MacDot16SsFreeQueuedMsgInMgmtScheduler(Node* node,
                                            MacDataDot16* dot16);
static
void MacDot16SsSendRangeRequest(Node* node,
                                MacDataDot16* dot16,
                                Dot16CIDType cid,
                                MacDot16UlBurst* burst,
                                BOOL flag);

static
void MacDot16SsSendBandwidthRequest(Node* node,
                                    MacDataDot16* dot16,
                                    MacDot16UlBurst* burst);

// /**
// FUNCTION   :: MacDot16SsPrintParameter
// LAYER      :: MAC
// PURPOSE    :: Print system parameters
// PARAMETERS ::
// + node : Node* : Poointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16SsPrintParameter(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    char clockStr[MAX_STRING_LENGTH];

    printf("Node%u interface %d MAC 802.16 SS parameters:\n",
           node->nodeId, dot16->myMacData->interfaceIndex);

    if (dot16Ss->para.managed == TRUE)
    {
        printf("    is a managed station\n");
    }
    else
    {
        printf("    is not a managed station\n");
    }

    // print wait for DCD timeout value
    ctoa(dot16Ss->para.t1Interval, clockStr);
    printf("    T1 interval = %s\n", clockStr);

    // print wait for UCD timeout value
    ctoa(dot16Ss->para.t12Interval, clockStr);
    printf("    T12 interval = %s\n", clockStr);

    // print out flow timeout value
    ctoa(dot16Ss->para.flowTimeout, clockStr);
    printf("    Service flow timeout = %s\n", clockStr);

    // print out 802.16e parameters
    if (dot16->dot16eEnabled)
    {
        printf("    handover RSS trigger = %f\n",
               dot16Ss->para.hoRssTrigger);
        printf("    handover RSS margin = %f\n", dot16Ss->para.hoRssMargin);
    }
    if (dot16Ss->isARQEnabled)
    {
        MacDot16PrintARQParameter(dot16Ss->arqParam);
    }
}

// /**
// FUNCTION   :: MacDot16eSsPrintNbrBsList
// LAYER      :: MAC
// PURPOSE    :: Print current neighbor BS list learned through scanning
// PARAMETERS ::
// + node : Node* : Poointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsPrintNbrBsList(Node* node,
                               MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;

    printf("MAC 802.16: Node%u interface %d (SS)'s neighbor BS list:\n",
           node->nodeId, dot16->myMacData->interfaceIndex);

    bsInfo = dot16Ss->nbrBsList;

    if (bsInfo == NULL)
    {
        printf("    empty neighbor BS list\n");

        return;
    }

    while (bsInfo != NULL)
    {
        printf("    macAddress = %d:%d:%d:%d:%d:%d, DL = %d, rss = %f, "
               "cinr = %f\n", bsInfo->bsId[0], bsInfo->bsId[1],
               bsInfo->bsId[2], bsInfo->bsId[3], bsInfo->bsId[4],
               bsInfo->bsId[5], bsInfo->dlChannel, bsInfo->rssMean,
               bsInfo->cinrMean);

        bsInfo = bsInfo->next;
    }
}

static
void MacDot16eSsInitSleepModePSClassParameter(Node* node,
                                              MacDot16Ss* dot16Ss)
{
    MacDot16ePSClasses *psClassParameter = dot16Ss->psClassParameter;
    dot16Ss->isPowerDown = FALSE;
    if (dot16Ss->mobTrfIndPdu != NULL)
    {
        MESSAGE_Free(node, dot16Ss->mobTrfIndPdu);
        dot16Ss->mobTrfIndPdu = NULL;
    }
    if (dot16Ss->timers.timerT43 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT43);
        dot16Ss->timers.timerT43 = NULL;
    }
    // init PS class 1 parameter
    psClassParameter[0].classType = POWER_SAVING_CLASS_1;
    psClassParameter[0].psClassStatus = POWER_SAVING_CLASS_SUPPORTED;
    MacDot16eSetPSClassStatus((&psClassParameter[0]),
        POWER_SAVING_CLASS_STATUS_NONE);
    psClassParameter[0].finalSleepWindowBase =
        DOT16e_SS_DEFAULT_PS1_FINAL_SLEEP_WINDOW_BASE;
    psClassParameter[0].finalSleepWindowExponent =
        DOT16e_SS_DEFAULT_PS1_FINAL_SLEEP_WINDOW_EXPONENT;
    psClassParameter[0].initialSleepWindow =
        DOT16e_SS_DEFAULT_PS1_INITIAL_SLEEP_WINDOW;
    psClassParameter[0].listeningWindow =
        DOT16e_SS_DEFAULT_PS1_LISTENING_INTERVAL;
    psClassParameter[0].trafficTriggeredFlag =
        DOT16e_SS_PS1_TRAFFIC_TRIGGERED_FLAG;

    // init PS class 2 parameter
    psClassParameter[1].classType = POWER_SAVING_CLASS_2;
    psClassParameter[1].psClassStatus = POWER_SAVING_CLASS_SUPPORTED;
    MacDot16eSetPSClassStatus((&psClassParameter[1]),
        POWER_SAVING_CLASS_STATUS_NONE);
    psClassParameter[1].initialSleepWindow =
        DOT16e_SS_DEFAULT_PS2_INITIAL_SLEEP_WINDOW;
    psClassParameter[1].listeningWindow =
        DOT16e_SS_DEFAULT_PS2_LISTENING_INTERVAL;

    // init PS class 3 parameter
    psClassParameter[2].classType = POWER_SAVING_CLASS_3;
    psClassParameter[2].psClassStatus = POWER_SAVING_CLASS_SUPPORTED;
    MacDot16eSetPSClassStatus((&psClassParameter[2]),
        POWER_SAVING_CLASS_STATUS_NONE);
    psClassParameter[2].finalSleepWindowExponent =
        DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_EXPONENT;
    psClassParameter[2].finalSleepWindowBase =
        DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_BASE;
    return;
}// end of MacDot16eSsInitSleepModePSClassParameter

// /**
// FUNCTION   :: MacDot16SsInitParameter
// LAYER      :: MAC
// PURPOSE    :: Initialize system parameters
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput : const NodeInput* : Pointer to node input
// + interfaceIndex : int : Interface Index
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16SsInitParameter(
         Node* node,
         const NodeInput* nodeInput,
         int interfaceIndex,
         MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    BOOL wasFound;
    clocktype timeVal = 0;
    double doubleVal = 0.0 ;
    int intVal = 0 ;
    char buff[MAX_STRING_LENGTH];
    char stringVal[MAX_STRING_LENGTH];

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-SS-WAIT-DCD-TIMEOUT-INTERVAL",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal > DOT16_SS_MAX_T1_INTERVAL || timeVal <= 0)
        {
            char clockStr[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(DOT16_SS_MAX_T1_INTERVAL, clockStr);
            sprintf(buff,
                    "MAC802.16: MAC-802.16-SS-WAIT-DCD-TIMEOUT-INTERVAL "
                    "must be shorter than %s seconds and greater than 0\n",
                    clockStr);
            ERROR_ReportWarning(buff);
            dot16Ss->para.t1Interval = DOT16_SS_DEFAULT_T1_INTERVAL;
        }
        else
        {
            dot16Ss->para.t1Interval = timeVal;
        }
    }
    else
    {
        dot16Ss->para.t1Interval = DOT16_SS_DEFAULT_T1_INTERVAL;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-SS-WAIT-UCD-TIMEOUT-INTERVAL",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal > DOT16_SS_MAX_T12_INTERVAL || timeVal <=0)
        {
            char clockStr[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(DOT16_SS_MAX_T12_INTERVAL, clockStr);
            sprintf(buff,
                    "MAC802.16: MAC-802.16-SS-WAIT-UCD-TIMEOUT-INTERVAL "
                    "must be shorter than %s seconds and greater than 0\n",
                    clockStr);
            ERROR_ReportWarning(buff);
            dot16Ss->para.t12Interval = DOT16_SS_DEFAULT_T12_INTERVAL;
        }
        else
        {
            dot16Ss->para.t12Interval = timeVal;
        }
    }
    else
    {
        dot16Ss->para.t12Interval = DOT16_SS_DEFAULT_T12_INTERVAL;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-SERVICE-FLOW-TIMEOUT-INTERVAL",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= 0)
        {
            sprintf(buff,
                    "MAC802.16: MAC-802.16-SERVICE-FLOW-TIMEOUT-INTERVAL "
                    "must be greater than 0\n");
            ERROR_ReportWarning(buff);
            dot16Ss->para.flowTimeout =
                    DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL;
        }
        else
        {
            dot16Ss->para.flowTimeout = timeVal;
        }
    }
    else
    {
        dot16Ss->para.flowTimeout =
            DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL;
    }

    dot16Ss->para.managed = FALSE;  // not managed
    dot16Ss->para.authPolicySupport = FALSE; // not support
    dot16Ss->para.minTxPower = 10;

    dot16Ss->para.rangeBOStart = DOT16_RANGE_BACKOFF_START;
    dot16Ss->para.rangeBOEnd = DOT16_RANGE_BACKOFF_END;
    dot16Ss->para.requestBOStart = DOT16_REQUEST_BACKOFF_START;
    dot16Ss->para.requestBOEnd = DOT16_REQUEST_BACKOFF_END;

    dot16Ss->para.initialRangeRetries = DOT16_MAX_INITIAL_RANGE_RETRIES;
    dot16Ss->para.periodicRangeRetries = DOT16_MAX_PERIODIC_RANGE_RETRIES;
    dot16Ss->para.requestRetries = DOT16_MAX_REQUEST_RETRIES;
    dot16Ss->para.registrationRetries = DOT16_MAX_REGISTRATION_RETRIES;
    dot16Ss->para.dsxReqRetries = DOT16_DEFAULT_DSX_REQ_RETRIES;
    dot16Ss->para.dsxRspRetries = DOT16_DEFAULT_DSX_RSP_RETRIES;

    dot16Ss->para.t2Interval = DOT16_SS_MAX_T2_INTERVAL;
    dot16Ss->para.t3Interval = DOT16_SS_DEFAULT_T3_INTERVAL;
    dot16Ss->para.t4Interval = DOT16_SS_MIN_T4_INTERVAL;
    dot16Ss->para.t6Interval = DOT16_SS_DEFAULT_T6_INTERVAL;
    dot16Ss->para.t7Interval = DOT16_DEFAULT_T7_INTERVAL;
    dot16Ss->para.t8Interval = DOT16_DEFAULT_T8_INTERVAL;
    dot16Ss->para.t10Interval = DOT16_DEFAULT_T10_INTERVAL;
    dot16Ss->para.t14Interval = DOT16_SS_DEFAULT_T14_INTERVAL;
    dot16Ss->para.t16Interval = DOT16_SS_DEFAULT_T16_INTERVAL;
    dot16Ss->para.t18Interval = DOT16_SS_DEFAULT_T18_INTERVAL;
    dot16Ss->para.t20Interval = DOT16_SS_DEFAULT_T20_INTERVAL;
    dot16Ss->para.t21Interval = DOT16_SS_DEFAULT_T21_INTERVAL;

    dot16Ss->para.t43Interval = DOT16e_SS_DEFAULT_T43_INTERVAL;
    dot16Ss->para.lostDlInterval = DOT16_SS_DEFAULT_LOST_DL_MAP_INTERVAL;
    dot16Ss->para.lostUlInterval = DOT16_SS_DEFAULT_LOST_UL_MAP_INTERVAL;


    // 802.16e (mobility) related parameters
    dot16Ss->para.nbrScanDuration = DOT16_SS_DEFAULT_NBR_SCAN_DURATION;
    dot16Ss->para.nbrScanInterval = DOT16_SS_DEFAULT_NBR_SCAN_INTERVAL;
    dot16Ss->para.nbrScanIteration = DOT16_SS_DEFAULT_NBR_SCAN_ITERATION;
    dot16Ss->para.nbrScanMinGap = DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP;
    dot16Ss->para.nbrMeaLifetime = DOT16_SS_DEFAULT_NBR_MEA_LIFETIME;
    dot16Ss->para.t41Interval = DOT16e_SS_DEFAULT_T41_INTERVAL;
    dot16Ss->para.t42Interval = DOT16e_SS_DEFAULT_T42_INTERVAL;
    dot16Ss->para.t44Interval = DOT16e_SS_DEFAULT_T44_INTERVAL;


    IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-802.16e-HANDOVER-RSS-TRIGGER",
        &wasFound,
        &doubleVal);

    if (wasFound)
    {

        dot16Ss->para.hoRssTrigger = doubleVal;
    }
    else
    {
        dot16Ss->para.hoRssTrigger = DOT16e_DEFAULT_HO_RSS_TRIGGER;
    }

    IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-802.16e-HANDOVER-RSS-MARGIN",
        &wasFound,
        &doubleVal);

    if (wasFound)
    {

        dot16Ss->para.hoRssMargin = doubleVal;
    }
    else
    {
        dot16Ss->para.hoRssMargin = DOT16e_DEFAULT_HO_RSS_MARGIN;
    }
    dot16Ss->isCRCEnabled = DOT16_CRC_STATUS;
    dot16Ss->isFragEnabled = DOT16_FRAGMENTATION_STATUS;

    dot16Ss->isPackingEnabled = FALSE;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-PACKING-ENABLED",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "YES") == 0)
        {
            dot16Ss->isPackingEnabled = TRUE;
        }
        else if (strcmp(stringVal, "NO") != 0)
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-PACKING-ENABLED "
                    "shoudle be YES or NO. Use default value as NO.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
    }

    dot16Ss->bwReqType = DOT16_BWReq_NORMAL;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-CONTENTION-BASED-BWREQ-TYPE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "NORMAL") == 0)
        {
        }
        else if (strcmp(stringVal, "CDMA") == 0)
        {
            dot16Ss->bwReqType = DOT16_BWReq_CDMA;
        }
        else
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-CONTENTION-BASED-BWREQ-TYPE "
                    "shoudle be NORMAL or CDMA. Use default value as Normal.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
    }
    dot16Ss->rngType = DOT16_NORMAL;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-RANGING-TYPE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "NORMAL") == 0)
        {
        }
        else if (strcmp(stringVal, "CDMA") == 0)
        {
            dot16Ss->rngType = DOT16_CDMA;
        }
        else
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-RANGING-TYPE "
                    "shoudle be NORMAL or CDMA. Use default value as Normal.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
    }

    // SS init sleep mode parameter
    dot16Ss->isSleepModeEnabled = FALSE;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16e-SS-SUPPORT-SLEEP-MODE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "NO") == 0)
        {
        }
        else if (strcmp(stringVal, "YES") == 0)
        {
            dot16Ss->isSleepModeEnabled = TRUE;
        }
        else
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16e-SS-SUPPORT-SLEEP-MODE "
                    "shoudle be YES or NO. Use default value NO.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
        // init defult value
        MacDot16eSsInitSleepModePSClassParameter(node, dot16Ss);

        // read mapping configuration procedure
        // if user defined then store the parameter
    }

    // ARQ support
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-ARQ-ENABLED",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "YES") == 0)
        {
            // ARQ enabled
            dot16Ss->isARQEnabled = TRUE;
        }
        else if (strcmp(stringVal, "NO") == 0)
        {
            // ARQ not enabled
            dot16Ss->isARQEnabled = FALSE;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-ARQ-ENABLE, should"
                " be YES or NO. Use default value as NO.");

            // by default, ARQ not enabled
             dot16Ss->isARQEnabled = FALSE;
        }
    }
    else
    {
            // not configured. Assume ARQ not enabled by default
            dot16Ss->isARQEnabled = FALSE;
    }

    if (dot16Ss->isARQEnabled == TRUE )
    {
        dot16Ss->arqParam =
            (MacDot16ARQParam*) MEM_malloc(sizeof(MacDot16ARQParam));
        memset((char*)dot16Ss->arqParam, 0, sizeof(MacDot16ARQParam));
        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-WINDOW-SIZE",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 1 || intVal > (DOT16_ARQ_BSN_MODULUS/2))
            {
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16-ARQ-WINDOW-SIZE range must be "
                    " between 1 and DOT16_ARQ_BSN_MODULUS/2 !\n"
                    "           Using default value for the parameter." );

                dot16Ss->arqParam->arqWindowSize =
                        DOT16_ARQ_DEFAULT_WINDOW_SIZE ;

            }
            else
            {
                dot16Ss->arqParam->arqWindowSize = intVal;
            }
        }
        else
        {
            //take default value
            dot16Ss->arqParam->arqWindowSize =
                                           DOT16_ARQ_DEFAULT_WINDOW_SIZE ;
        }

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-RETRY-TIMEOUT",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 0)
            {
                sprintf(buff,
                        "MAC802.16:"
                        " MAC-802.16-ARQ-RETRY-TIMEOUT "
                        "must not be lesser than 0 seconds.\n"
                        "           Using default value for the parameter.");

                ERROR_ReportWarning(buff);
                dot16Ss->arqParam->arqRetryTimeoutTxDelay =
                                    DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
                dot16Ss->arqParam->arqRetryTimeoutRxDelay =
                                    DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
            }
            else
            {
                dot16Ss->arqParam->arqRetryTimeoutTxDelay = intVal/2;
                dot16Ss->arqParam->arqRetryTimeoutRxDelay = intVal/2 ;
            }
        }
        else
        {
            dot16Ss->arqParam->arqRetryTimeoutTxDelay =
                                DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
            dot16Ss->arqParam->arqRetryTimeoutRxDelay =
                                DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
        }

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-RETRY-COUNT",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 0)
            {
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-RETRY-COUNT "
                        "must not be lesser than 0 \n"
                        "           Using default value for the parameter.");

                ERROR_ReportWarning(buff);
                dot16Ss->arqParam->arqBlockLifeTime =
                                    DOT16_ARQ_DEFAULT_RETRY_COUNT ;
            }
            else
            {
                dot16Ss->arqParam->arqBlockLifeTime = intVal ;
            }
        }
        else
        {
            dot16Ss->arqParam->arqBlockLifeTime =
                                    DOT16_ARQ_DEFAULT_RETRY_COUNT ;
        }

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-SYNC-LOSS-INTERVAL",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 0)
            {
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-SYNC-LOSS-INTERVAL "
                        "must not be lesser than 0 \n"
                        "           Using default value for the parameter.");
            dot16Ss->arqParam->arqSyncLossTimeout =
                                DOT16_ARQ_DEFAULT_SYNC_LOSS_INTERVAL ;
            }
            else
            {
                dot16Ss->arqParam->arqSyncLossTimeout = intVal ;
            }
        }
        else
        {
            dot16Ss->arqParam->arqSyncLossTimeout =
                                    DOT16_ARQ_DEFAULT_SYNC_LOSS_INTERVAL;
        }

            dot16Ss->arqParam->arqDeliverInOrder =
                                    DOT16_ARQ_DEFAULT_DELIVER_IN_ORDER ;


        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-RX-PURGE-TIMEOUT",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 0)
            {
                sprintf(buff,
                        "MAC802.16: MAC-802.16-ARQ-RX-PURGE-TIMEOUT "
                        "must not be lesser than 0 \n"
                        "           Using default value for the parameter.");
                dot16Ss->arqParam->arqRxPurgeTimeout =
                                DOT16_ARQ_DEFAULT_RX_PURGE_TIMEOUT ;
            }
            else
            {
                dot16Ss->arqParam->arqRxPurgeTimeout = intVal ;
            }
        }
        else
        {
            dot16Ss->arqParam->arqRxPurgeTimeout =
                                    DOT16_ARQ_DEFAULT_RX_PURGE_TIMEOUT;
        }

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16-ARQ-BLOCK-SIZE",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
        if (intVal < 1 || intVal > (DOT16_ARQ_MAX_BLOCK_SIZE))
            {
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16-ARQ-BLOCK-SIZE range must be "
                    " between 1 and DOT16_ARQ_MAX_BLOCK_SIZE !\n"
                    "           Using default value for the parameter.");

                dot16Ss->arqParam->arqBlockSize =
                                DOT16_ARQ_DEFAULT_BLOCK_SIZE;
            }
            else
            {
                dot16Ss->arqParam->arqBlockSize = intVal;
            }
        }
        else
        {
            //take default value
            dot16Ss->arqParam->arqBlockSize = DOT16_ARQ_DEFAULT_BLOCK_SIZE;
        }
    }// end of if (dot16Ss->isARQEnabled == TRUE)

    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16e-SS-SUPPORT-IDLE-MODE",
                  &wasFound,
                  buff);

    if (wasFound)
    {
        if (strcmp(buff, "YES") == 0)
        {
            dot16Ss->isIdleEnabled = TRUE;
        }
        else
        {
            dot16Ss->isIdleEnabled = FALSE;
        }
    }
    else
    {
        // not configured. Assume PMP mode by default
        dot16Ss->isIdleEnabled = FALSE;
    }
    if (dot16Ss->isIdleEnabled == TRUE)
    {
        dot16Ss->para.t45Interval = DOT16e_SS_DEFAULT_T45_INTERVAL;
        dot16Ss->para.tIdleModeInterval = DOT16e_SS_IDLE_MODE_TIMEOUT;
        dot16Ss->para.tLocUpdInterval = DOT16e_SS_LOC_UPD_TIMEOUT;
    }// end of idle
} // End of MacDot16SsInitParameter

// /**
// FUNCTION   :: MacDot16SsInitDynamicStats
// LAYER      :: MAC
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIndex : int      : Interface where the MAC is running
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16SsInitDynamicStats(Node* node,
                                int interfaceIndex,
                                MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (!node->guiOption)
    {
        return;
    }

    dot16Ss->stats.dlCinrGuiId =
        GUI_DefineMetric("802.16 MAC (SS): Last Measured Downlink CINR (dB)",
                         node->nodeId,
                         GUI_PHY_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_AVERAGE_METRIC);

    dot16Ss->stats.dlRssiGuiId =
        GUI_DefineMetric("802.16 MAC (SS): Last Measured Downlink RSS (dBm)",
                         node->nodeId,
                         GUI_PHY_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_AVERAGE_METRIC);

    dot16Ss->stats.numPktToPhyGuiId =
        GUI_DefineMetric("802.16 MAC (SS): Number of Data Packets Transmitted",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

    dot16Ss->stats.numPktFromPhyGuiId =
        GUI_DefineMetric("802.16 MAC (SS): Number of Data Packets Received",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

    dot16Ss->stats.numNbrBsGuiId =
        GUI_DefineMetric("802.16 MAC (SS): Number of Active Neighbor BSs Discovered",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_AVERAGE_METRIC);
}

// /**
// FUNCTION   :: MacDot16SsPrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + interfaceIndex : int      : Interface index
// RETURN     :: void : NULL
// **/
static
void MacDot16SsPrintStats(Node* node,
                          MacDataDot16* dot16,
                          int interfaceIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    char buf[MAX_STRING_LENGTH];

    // print out # of data packets from upper layer
    sprintf(buf, "Number of data packets from upper layer = %d",
            dot16Ss->stats.numPktsFromUpper);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets txed in MAC frames
    sprintf(buf, "Number of data packets sent in MAC frames = %d",
            dot16Ss->stats.numPktsToLower);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets received in MAC frames
    sprintf(buf, "Number of data packets rcvd in MAC frames = %d",
            dot16Ss->stats.numPktsFromLower);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    if (dot16Ss->isFragEnabled == TRUE)
    {
        // print out # of fragments sent in MAC frames
        sprintf(buf, "Number of fragments sent in MAC frames = %d",
                dot16Ss->stats.numFragmentsSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of fragments received in MAC frames
        sprintf(buf, "Number of fragments received in MAC frames = %d",
                dot16Ss->stats.numFragmentsRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }
    // print out # of DL-MAP messages received
    sprintf(buf, "Number of DL-MAP messages rcvd = %d",
            dot16Ss->stats.numDlMapRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of UL-MAP messages received
    sprintf(buf, "Number of UL-MAP messages rcvd = %d",
            dot16Ss->stats.numUlMapRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DCD messages received
    sprintf(buf, "Number of DCD messages rcvd = %d",
            dot16Ss->stats.numDcdRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of UCD messages received
    sprintf(buf, "Number of UCD messages rcvd = %d",
            dot16Ss->stats.numUcdRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of network entry performed
    sprintf(buf, "Number of network entry performed = %d",
            dot16Ss->stats.numNetworkEntryPerformed);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of RNG-REQ messages sent
    sprintf(buf, "Number of RNG-REQ messages sent = %d",
            dot16Ss->stats.numRngReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of RNG-RSP messages received
    sprintf(buf, "Number of RNG-RSP messages rcvd = %d",
            dot16Ss->stats.numRngRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of SBC-REQ messages sent
    sprintf(buf, "Number of SBC-REQ messages sent = %d",
            dot16Ss->stats.numSbcReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of SBC-RSP messages sent
    sprintf(buf, "Number of SBC-RSP messages rcvd = %d",
            dot16Ss->stats.numSbcRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of PKM-REQ messages sent
    sprintf(buf, "Number of PKM-REQ messages sent = %d",
            dot16Ss->stats.numPkmReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of PKM-RSP messages sent
    sprintf(buf, "Number of PKM-RSP messages rcvd = %d",
            dot16Ss->stats.numPkmRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REG-REQ messages sent
    sprintf(buf, "Number of REG-REQ messages sent = %d",
            dot16Ss->stats.numRegReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REG-RSP messages sent
    sprintf(buf, "Number of REG-RSP messages rcvd = %d",
            dot16Ss->stats.numRegRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REP-REQ messages received
    sprintf(buf, "Number of REP-REQ messages rcvd = %d",
            dot16Ss->stats.numRepReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REP-RSP messages sent
    sprintf(buf, "Number of REP-RSP messages sent = %d",
            dot16Ss->stats.numRepRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-REQ messages sent
    sprintf(buf, "Number of DSA-REQ messages sent = %d",
            dot16Ss->stats.numDsaReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSX-RVD messages received
    sprintf(buf, "Number of DSX-RVD messages rcvd = %d",
            dot16Ss->stats.numDsxRvdRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-RSP messages received
    sprintf(buf, "Number of DSA-RSP messages rcvd = %d",
            dot16Ss->stats.numDsaRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     // print out # of DSA-ACK messages sent
    sprintf(buf, "Number of DSA-ACK messages sent = %d",
            dot16Ss->stats.numDsaAckSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-REQ messages received
    sprintf(buf, "Number of DSA-REQ messages rcvd = %d",
            dot16Ss->stats.numDsaReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     // print out # of DSA-RSP messages sent
    sprintf(buf, "Number of DSA-RSP messages sent = %d",
            dot16Ss->stats.numDsaRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-ACK messages received
    sprintf(buf, "Number of DSA-ACK messages rcvd = %d",
            dot16Ss->stats.numDsaAckRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    ////
    // print out # of DSC-REQ messages sent
    sprintf(buf, "Number of DSC-REQ messages sent = %d",
            dot16Ss->stats.numDscReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-RSP messages received
    sprintf(buf, "Number of DSC-RSP messages rcvd = %d",
            dot16Ss->stats.numDscRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     // print out # of DSC-ACK messages sent
    sprintf(buf, "Number of DSC-ACK messages sent = %d",
            dot16Ss->stats.numDscAckSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-REQ messages received
    sprintf(buf, "Number of DSC-REQ messages rcvd = %d",
            dot16Ss->stats.numDscReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     // print out # of DSC-RSP messages sent
    sprintf(buf, "Number of DSC-RSP messages sent = %d",
            dot16Ss->stats.numDscRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-ACK messages received
    sprintf(buf, "Number of DSC-ACK messages rcvd = %d",
            dot16Ss->stats.numDscAckRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-REQ messages sent
    sprintf(buf, "Number of DSD-REQ messages sent = %d",
            dot16Ss->stats.numDsdReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-RSP messages received
    sprintf(buf, "Number of DSD-RSP messages rcvd = %d",
            dot16Ss->stats.numDsdRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-REQ messages received
    sprintf(buf, "Number of DSD-REQ messages rcvd = %d",
            dot16Ss->stats.numDsdReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-RSP messages sent
    sprintf(buf, "Number of DSD-RSP messages sent = %d",
            dot16Ss->stats.numDsdRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
    if (dot16Ss->rngType == DOT16_CDMA)
    {
        sprintf(buf, "Number of CDMA ranging code sent = %d",
                dot16Ss->stats.numCdmaRngCodeSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }
    if (dot16Ss->bwReqType == DOT16_BWReq_CDMA)
    {
        sprintf(buf, "Number of CDMA bandwidth ranging code sent = %d",
                dot16Ss->stats.numCdmaBwRngCodeSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }

    //ARQ related stats.
    if (dot16Ss->isARQEnabled)
    {
        sprintf(buf, "Number of ARQ blocks received = %d",
                    dot16Ss->stats.numARQBlockRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

        sprintf(buf, "Number of ARQ blocks sent = %d",
                    dot16Ss->stats.numARQBlockSent);


            IO_PrintStat(node,
                         "MAC",
                         "802.16",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

        sprintf(buf, "Number of ARQ blocks discarded = %d",
                       dot16Ss->stats.numARQBlockDiscard);

            IO_PrintStat(node,
                         "MAC",
                         "802.16",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
    }

    // 802.16e related statistics
    if (dot16->dot16eEnabled)
    {
        // print out # of neighbor BS scan performed
        sprintf(buf, "Number of neighbor BS scanning performed = %d",
                dot16Ss->stats.numNbrScan);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of handovers performed
        sprintf(buf, "Number of handovers performed = %d",
                dot16Ss->stats.numHandover);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of NBR-ADV message received
        sprintf(buf, "Number of MOB-NBR-ADV messages rcvd = %d",
                dot16Ss->stats.numNbrAdvRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of NBR-SCN-REQ message received
        sprintf(buf, "Number of MOB-SCN-REQ messages sent = %d",
                dot16Ss->stats.numNbrScanReqSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-SCN-RSP message received
        sprintf(buf, "Number of MOB-SCN-RSP messages rcvd = %d",
                dot16Ss->stats.numNbrScanRspRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-SCN-REP message sent
        sprintf(buf, "Number of MOB-SCN-REP messages sent = %d",
                dot16Ss->stats.numNbrScanRepSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-MSHO-REQ message received
        sprintf(buf, "Number of MOB-MSHO-REQ messages sent = %d",
                dot16Ss->stats.numMsHoReqSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-BSHO-RSP message received
        sprintf(buf, "Number of MOB-BSHO-RSP messages rcvd = %d",
                dot16Ss->stats.numBsHoRspRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-BSHO-REQ message received
        sprintf(buf, "Number of MOB-BSHO-REQ messages rcvd = %d",
                dot16Ss->stats.numBsHoReqRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-HO-IND message sent
        sprintf(buf, "Number of MOB-HO-IND messages sent = %d",
                dot16Ss->stats.numHoIndSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        if (dot16Ss->isSleepModeEnabled)
        {
// Sleep mode statistics parameter
             // print out # Number of MOB-SLP-REQ messages sent
            sprintf(buf, "Number of MOB-SLP-REQ messages sent = %d",
                    dot16Ss->stats.numMobSlpReqSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

             // print out # Number of MOB-SLP-RSP messages received
            sprintf(buf, "Number of MOB-SLP-RSP messages received = %d",
                    dot16Ss->stats.numMobSlpRspRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

             // print out # Number of MOB-TRF-IND messages received
            sprintf(buf, "Number of MOB-TRF-IND messages received = %d",
                    dot16Ss->stats.numMobTrfIndRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
        }
        if (dot16Ss->isIdleEnabled == TRUE)
        {
            // print out # of DREG-REQ message sent
            sprintf(buf, "Number of DREG-REQ messages sent = %d",
                    dot16Ss->stats.numDregReqSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            // print out # of DREG-CMD message rcvd
            sprintf(buf, "Number of DREG-CMD messages rcvd = %d",
                    dot16Ss->stats.numDregCmdRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            // print out # of location updates performed
            sprintf(buf, "Number of location updates performed = %d",
                    dot16Ss->stats.numLocUpd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            // print out # of MOB-PAG-ADV message rcvd
            sprintf(buf, "Number of MOB-PAG-ADV messages rcvd = %d",
                    dot16Ss->stats.numMobPagAdvRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
        }// end of idle
    }
    // end of 802.16e related statistics
}

// /**
// FUNCTION   :: MacDot16SsIsDataGrantBurst
// LAYER      :: MAC
// PURPOSE    :: Tell whether an UL burst is data grant burst based UIUC
// PARAMETERS ::
// + node      : Node*        : Pointer to node
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + uiuc      : unsignec char: UIUC of the UL burst
// RETURN     :: BOOL         : TRUE for YES, FALSE otherwise
// **/
static
BOOL MacDot16SsIsDataGrantBurst(Node* node,
                                MacDataDot16* dot16,
                                unsigned char uiuc)
{

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        // for OFDMA PHY, burst profiles have UIUC from 1 to 10
// Note as per the standdard MAC802.16e uuiuc 1-10 should used as
//        a (Data Grant Burst Type), but in QualNet to support
//        NORMAL ranging procedure QualNet used 1-7 for this purpose
//        if (uiuc >= 1 && uiuc <= 10)
        if (uiuc >= 1 && uiuc <= 7)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//--------------------------------------------------------------------------
// Utility functions for DSX transaction
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsGetServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by
//               transaction ID of the flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + transId   : unsinged int : Trans. Id assoctated w/ the service flow
// RETURN     :: MacDot16ServiceFlow* : Point to the sflow if found
// **/
MacDot16ServiceFlow* MacDot16SsGetServiceFlowByTransId(
                         Node* node,
                         MacDataDot16* dot16,
                         unsigned int transId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* serviceFlow;
    int i;

    // for the ul service flow, the transId is in the range 0x0000 - 0x7FFF
    // for the dl service flow, the transId is in the range 0x8000 - 0xFFFF

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->ulFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->dlFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    // not found
    return NULL;
}

// /**
// FUNCTION   :: MacDot16SsDeleteServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow & its SS pointer by one
//               transaction Id of the flow
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + transId   : unsinged int  : Trans. Id of the service flow
// RETURN     :: void : TRUE = deletion success; FALSE = deletion failed
// **/
static
BOOL MacDot16SsDeleteServiceFlowByTransId(Node* node,
                                          MacDataDot16* dot16,
                                          unsigned int transId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16ServiceFlow* lastServiceFlow = NULL;
    BOOL found = FALSE;
    int serviceType = 0;
    BOOL isUlFlow = FALSE;
    int i;

    // for the ul service flow, the transId is in the range 0x0000 - 0x7FFF,
    // for the dl service flow, the transId is in the range 0x8000 - 0xFFFF

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            if (sFlow->dsaInfo.dsxTransId == transId ||
                sFlow->dscInfo.dsxTransId == transId ||
                sFlow->dsdInfo.dsxTransId == transId)
            {

                found = TRUE;
                serviceType = i;
                isUlFlow = TRUE;

                break;
            }

            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->dlFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            if (sFlow->dsaInfo.dsxTransId == transId ||
                sFlow->dscInfo.dsxTransId == transId ||
                sFlow->dsdInfo.dsxTransId == transId)
            {
                found = TRUE;
                serviceType = i;

                break;
            }

            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    if (found == TRUE)
    {
        if (sFlow == dot16Ss->ulFlowList[serviceType].flowHead)
        {
            dot16Ss->ulFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->ulFlowList[serviceType].numFlows --;
        }
        else if (sFlow == dot16Ss->dlFlowList[serviceType].flowHead)
        {
            dot16Ss->dlFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->dlFlowList[serviceType].numFlows --;
        }
        else
        {
             lastServiceFlow->next =  sFlow->next;
        }

        // free memory in dsxInfo
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsaInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dscInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsdInfo));

        if (isUlFlow)
        {
            // remove the corresponding queue
            dot16Ss->outScheduler[serviceType]->removeQueue(
                sFlow->queuePriority);
            dot16Ss->outScheduler[serviceType]->normalizeWeight();
        }


        // free temp queue
        MESSAGE_FreeList(node, sFlow->tmpMsgQueueHead);

        if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
        {
            MacDot16ePSClasses* psClass = NULL;
            psClass = &dot16Ss->psClassParameter[sFlow->psClassType - 1];
            if (psClass->numOfServiceFlowExists > 0)
            {
                psClass->numOfServiceFlowExists--;
            }
        }
        if (sFlow->isARQEnabled)
        {
            MacDot16ARQResetWindow(node, dot16, sFlow);
            //Deleting the ARQ Block Array
            MEM_free(sFlow->arqControlBlock->arqBlockArray);
            sFlow->arqControlBlock->arqBlockArray = NULL;
            //Deleting the ARQ Control Block
            MEM_free(sFlow->arqControlBlock);
            sFlow->arqControlBlock = NULL;
            //Deleting the ARQ Parameters
            MEM_free(sFlow->arqParam);
            sFlow->arqParam = NULL;
       }
        // free service lfow itself
        MEM_free(sFlow);
    }

    return found;
}

// /**
// FUNCTION   :: MacDot16SsGetServiceFlowByCid
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by CID of the flow
// PARAMETERS ::
// + node      : Node*                : Pointer to node
// + dot16     : MacDataDot16*        : Pointer to DOT16 structure
// + cid       : unsinged int         : CID of the service flow
// RETURN     :: MacDot16ServiceFlow* : Point to the sflow if found
// **/
MacDot16ServiceFlow* MacDot16SsGetServiceFlowByCid(
                         Node* node,
                         MacDataDot16* dot16,
                         unsigned int cid)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* serviceFlow;
    int i;

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->ulFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->cid== cid)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->dlFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->cid == cid)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    // not found
    return NULL;
}

// /**
// FUNCTION   :: MacDot16SsDeleteServiceFlowByCid
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow & its SS pointer by CID of the flow
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + cid       : unsinged int  : CID assoctated with the service flow
// RETURN     :: BOOL : TRUE = deletion success; FALSE = deletion failed
// **/
#if 0
static
BOOL MacDot16SsDeleteServiceFlowByCid(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned int cid)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    MacDot16ServiceFlow* lastServiceFlow;
    BOOL found = FALSE;
    int i;
    int serviceType;
    BOOL isUlFlow = FALSE;

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            if (sFlow->cid == cid)
            {
                serviceType = i;
                found = TRUE;
                isUlFlow = TRUE;
                break;
            }

            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->dlFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            if (sFlow->cid == cid)
            {
                serviceType = i;
                found = TRUE;
                break;
            }

            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    if (found == TRUE)
    {
        if (sFlow == dot16Ss->ulFlowList[serviceType].flowHead)
        {
            dot16Ss->ulFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->ulFlowList[serviceType].numFlows --;
        }
        else if (sFlow == dot16Ss->dlFlowList[serviceType].flowHead)
        {
            dot16Ss->dlFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->dlFlowList[serviceType].numFlows --;
        }
        else
        {
             lastServiceFlow->next =  sFlow->next;
        }

        // free mem in dsxInfo
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsaInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dscInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsdInfo));

        if (isUlFlow)
        {
            // remove the corresponding queue from the scheduler
            dot16Ss->outScheduler[serviceType]->removeQueue(
                sFlow->queuePriority);
            dot16Ss->outScheduler[serviceType]->normalizeWeight();
        }

        // free temp queue
        MESSAGE_FreeList(node, sFlow->tmpMsgQueueHead);
        // free service flow itself
        MEM_free(sFlow);
    }

    return found;
}
#endif
// /**
// FUNCTION   :: MacDot16SsGetServiceFlowByServiceFlowId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by flow id
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + sfId      : unsinged int          : flow Id of the service flow
// + sflowPtr  : MacDot16ServiceFlow** : Pointer to the Service flow
// RETURN     :: MacDot16ServiceFlow* : Point to the sflow if found
// **/
static
MacDot16ServiceFlow* MacDot16SsGetServiceFlowByServiceFlowId(
                         Node* node,
                         MacDataDot16* dot16,
                         unsigned int sfId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* serviceFlow;
    int i;

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->ulFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->sfid == sfId)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = dot16Ss->dlFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->sfid == sfId)
            {
                return serviceFlow;
            }

            serviceFlow = serviceFlow->next;
        }
    }

    // not found
    return NULL;
}
#if 0
// /**
// FUNCTION   :: MacDot16SsDeleteServiceFlowByServiceFlowId
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow & its SS pointer by flow id
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sfId      : unsinged int  : flow Id assoctated with the service flow
// RETURN     :: BOOL : TRUE = deletion success; FALSE = deletion failed
// **/
static
BOOL MacDot16SsDeleteServiceFlowByServiceFlowId(Node* node,
                                                MacDataDot16* dot16,
                                                unsigned int sfId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    MacDot16ServiceFlow* lastServiceFlow;
    BOOL found = FALSE;
    int serviceType;
    int i;
    BOOL isUlFlow = FALSE;

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            if (sFlow->sfid == sfId)
            {
                serviceType = i;
                found = TRUE;
                isUlFlow = TRUE;
                break;
            }
            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->dlFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            if (sFlow->sfid == sfId)
            {
                serviceType = i;
                found = TRUE;
                break;
            }
            lastServiceFlow = sFlow;
            sFlow = sFlow->next;
        }
    }

    if (found == TRUE)
    {
        if (sFlow == dot16Ss->ulFlowList[serviceType].flowHead)
        {
            dot16Ss->ulFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->ulFlowList[serviceType].numFlows --;
        }
        else if (sFlow == dot16Ss->dlFlowList[serviceType].flowHead)
        {
            dot16Ss->dlFlowList[serviceType].flowHead = sFlow->next;
            dot16Ss->dlFlowList[serviceType].numFlows --;
        }
        else
        {
             lastServiceFlow->next =  sFlow->next;
        }

        // free mem in dsxInfo
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsaInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dscInfo));
        MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsdInfo));

        if (isUlFlow)
        {
            // remove the corresponding queue from the scheduler
            dot16Ss->outScheduler[serviceType]->removeQueue(
                sFlow->queuePriority);
            dot16Ss->outScheduler[serviceType]->normalizeWeight();
        }

        // free temp queue
        MESSAGE_FreeList(node, sFlow->tmpMsgQueueHead);
        // free service lfow itself
        MEM_free(sFlow);
    }

    return found;
}
#endif
//--------------------------------------------------------------------------
// UL burst list operations
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16SsGetUlBurst
// LAYER      :: MAC
// PURPOSE    :: Get the UL burst allocation from the burst list
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + index     : int           : index of the burst
// RETURN     :: MacDot16UlBurst* : Pointer to burst, NULL if not found
// **/
static
MacDot16UlBurst* MacDot16SsGetUlBurst(Node* node,
                                      MacDataDot16* dot16,
                                      int index)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16UlBurst* burst;

    burst = dot16Ss->ulBurstHead;
    while (burst != NULL)
    {
        if (burst->index == index)
        {
            return burst;
        }
        burst = burst->next;
    }

    return NULL;
}

// /**
// FUNCTION   :: MacDot16SsBuildBurstInfo
// LAYER      :: MAC
// PURPOSE    :: Build the burst information to be passed to PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node
// + dot16     : MacDataDot16*    : Pointer to DOT16 structure
// + burst     : MacDot16UlBurst* : Pointer to allocated burst
// + sizeInBytes : int            : Size in bytes of PDUs to be txed
// + burstInfo : Dot16BurstInfo*  : For returning burst info
// RETURN     :: int : Return delay in PS that the signal will last
//                     Return -1 if the message is bigger than burst
// **/
static
int MacDot16SsBuildBurstInfo(Node* node,
                             MacDataDot16* dot16,
                             MacDot16UlBurst* burst,
                             int sizeInBytes,
                             Dot16BurstInfo* burstInfo,
                             MacDot16RngCdmaInfo* cdmaInfo = NULL)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int startIndex;
    int duration;
    int endIndex;
    int profileIndex;
    int burstDuration;

    if (MacDot16SsIsDataGrantBurst(node, dot16, burst->uiuc))
    {
        // it is a data grant burst
        profileIndex = burst->uiuc - 1;
    }
    else
    {
        // use most reliable one
        profileIndex = DOT16_UIUC_MOST_RELIABLE - 1;
    }

    if (burst->uiuc == DOT16_UIUC_CDMA_RANGE)
    {
        profileIndex = PHY802_16_BPSK;
    }

    burstInfo->burstIndex = 0;  // UL tx only contains one burst

    if (burst->uiuc == DOT16_UIUC_CDMA_RANGE)
    {
        ERROR_Assert(cdmaInfo != NULL,
                     "MAC802.16: cdma info can not br NULL");

        // set this value here
        burst->durationOffset =
            ((dot16Ss->ulDurationInPs * (cdmaInfo->ofdmaSubchannel - 1))
            + cdmaInfo->ofdmaSymbol);

        burstInfo->cdmaCodeIndex = cdmaInfo->rangingCode;
        burstInfo->signalType = (UInt8)CDMA; // FOR CDMA
        burstInfo->cdmaSpreadingFactor =
            (UInt8)PHY802_16_DEFAULT_CDMA_SPREADING_FACTOR;
    }
    else
    {
        burstInfo->cdmaCodeIndex = 0;
        burstInfo->signalType = (UInt8)NORMAL; // FOR Normal
        burstInfo->cdmaSpreadingFactor = 1;
    }
    duration = MacDot16PhyBytesToPs(
                   node,
                   dot16,
                   sizeInBytes,
                   &(dot16Ss->servingBs->ulBurstProfile[profileIndex]),
                   DOT16_UL);

    burstDuration = burst->duration - dot16Ss->para.sstgInPs;

    if (duration > burstDuration)
    {
        ERROR_ReportWarning("MAC802.16: Frame size too big to fit into the burst!");
        return -1;
    }

    startIndex = burst->durationOffset / dot16Ss->ulDurationInPs;
    endIndex = (burst->durationOffset + burstDuration - 1) /
               dot16Ss->ulDurationInPs;
    burstInfo->subchannelOffset = startIndex;
    burstInfo->numSubchannels = endIndex - startIndex + 1;

    startIndex = burst->durationOffset % dot16Ss->ulDurationInPs;
    burstInfo->symbolOffset =
            startIndex * MacDot16PhySymbolsPerPs(DOT16_UL);
    burstInfo->numSymbols =
            burstDuration * MacDot16PhySymbolsPerPs(DOT16_UL);

    burstInfo->modCodeType =
        dot16Ss->servingBs->ulBurstProfile[profileIndex].
        ofdma.fecCodeModuType;

    if (DEBUG || node->nodeId  == DEBUG_SPECIFIC_NODE)
    {
        printf("Node%d build burst(duration offset = %d) info, index = %d, "
               "subchannelOffset = %d, numSubchannels = %d, symboleOffset ="
               " %d, numSymbols = %d, modulation coding type = %d,uiuc%d\n",
               node->nodeId, burst->durationOffset, burstInfo->burstIndex,
               burstInfo->subchannelOffset, burstInfo->numSubchannels,
               burstInfo->symbolOffset, burstInfo->numSymbols,
               burstInfo->modCodeType, burst->uiuc);
    }

    if (burstInfo->numSubchannels > 1)
    {
        // if multiple subchannels used, then signal delay is whole UL part
        return dot16Ss->ulDurationInPs;
    }
    else
    {
        if (MacDot16SsIsDataGrantBurst(node,
                                       dot16,
                                       burst->uiuc))
        {
            return dot16Ss->ulDurationInPs - startIndex;
        }
        else
        {
            return burstDuration;
        }
    }
}
// /**
// FUNCTION   :: MacDot16SsRecomputeStartTime
// LAYER      :: MAC
// PURPOSE    :: Re-compute the actual start time for my transmission
//               inside the burst. This is for contention based tx.
// PARAMETERS ::
// + node      : Node*            : Pointer to node
// + dot16     : MacDataDot16*    : Pointer to DOT16 structure
// + burst     : MacDot16UlBurst* : Pointer to allocated burst
// + offset    : int              : Offset to my transmission opp in PS
// + newDuration : int            : Duration of the transmission opp size
// RETURN     :: void : NULL
// **/
static
void MacDot16SsRecomputeStartTime(Node* node,
                                  MacDataDot16* dot16,
                                  MacDot16UlBurst* burst,
                                  int offset,
                                  int newDuration)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    // adjust the burst info for later use
    burst->durationOffset += offset;
    burst->duration = newDuration - dot16Ss->para.sstgInPs;

    // calculate the start time on time-axis of this burst
    if ((burst->durationOffset % dot16Ss->ulDurationInPs + burst->duration)
        > dot16Ss->ulDurationInPs)
    {
        // part of this burst is rounded to the beginning of
        // next subchannel
        burst->startTime = node->getNodeTime();
    }
    else
    {
        // this burst starts in the middle of UL
        burst->startTime = dot16Ss->ulStartTime + (burst->durationOffset %
                           dot16Ss->ulDurationInPs) *
                           dot16->ulPsDuration;
    }

    burst->duration = newDuration;
    ERROR_Assert(burst->startTime >= node->getNodeTime() &&
           burst->startTime <= (dot16Ss->ulStartTime +
                                dot16Ss->ulDurationInPs *
                                dot16->ulPsDuration),
           "MAC802.16: Wrong burst info!");
}

//--------------------------------------------------------------------------
// Network entry
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16SsPerformPMPModeNetworkEntry
// LAYER      :: MAC
// PURPOSE    :: Perform PMP mode network entry procedures of the SS.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16SsPerformPMPModeNetworkEntry(Node* node,
                                          MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    dot16Ss->initInfo.initStatus = DOT16_SS_INIT_ScanDlCh;

    // scan channel to listen to DL-MAP messages
    MacDot16StartListening(node, dot16, dot16Ss->servingBs->dlChannel);

    // set a timeout timer for searching channel
    if (dot16Ss->timers.timerT20 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT20);
        dot16Ss->timers.timerT20 = NULL;
    }

    dot16Ss->timers.timerT20 = MacDot16SetTimer(node,
                                                dot16,
                                                DOT16_TIMER_T20,
                                                dot16Ss->para.t20Interval,
                                                NULL,
                                                0);

    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("scan physical channel index %d "
               "for DL synchronization and start T20\n",
               dot16Ss->chScanIndex);
    }
}

// /**
// FUNCTION   :: MacDot16SsPerformNetworkEntry
// LAYER      :: MAC
// PURPOSE    :: Perform network entry procedures of the SS.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16SsPerformNetworkEntry(Node* node,
                                   MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    dot16Ss->operational = FALSE;
    dot16Ss->initInfo.initStatus = DOT16_SS_INIT_PreEntry;

    dot16Ss->initInfo.dlSynchronized = FALSE;
    dot16Ss->initInfo.ulParamAccquired = FALSE;
    dot16Ss->initInfo.basicCapabilityNegotiated = FALSE;
    dot16Ss->initInfo.authorized =FALSE;
    dot16Ss->initInfo.initRangingCompleted = FALSE;
    dot16Ss->initInfo.rngState = DOT16_SS_RNG_Null;
    dot16Ss->initInfo.registered = FALSE;
    dot16Ss->initInfo.ipConnectCompleted = FALSE;
    dot16Ss->initInfo.timeOfDayEstablished = FALSE;
    dot16Ss->initInfo.paramTransferCompleted = FALSE;

    if (dot16->mode == DOT16_PMP)
    {
        //PMP mode network entry
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("start PMP mode network entry proc\n");
        }
        MacDot16SsPerformPMPModeNetworkEntry(node, dot16);
    }
    else if (dot16->mode == DOT16_MESH)
    {
        ERROR_ReportError("MAC 802.16: Mesh mode is not supported\n");
    }

    //increase stat
    if (dot16Ss->idleSubStatus != DOT16e_SS_IDLE_MSListen)
    {
        dot16Ss->stats.numNetworkEntryPerformed ++;
    }
    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        MacDot16eSsInitSleepModePSClassParameter(node, dot16Ss);
    }
}

// /**
// FUNCTION   :: MacDot16eSsPerformLocationUpdate
// LAYER      :: MAC
// PURPOSE    :: Perform Location update of the SS.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16eSsPerformLocationUpdate(Node* node,
                                    MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (dot16Ss->idleSubStatus == DOT16e_SS_IDLE_None)
    {
        return;
    }
    dot16Ss->performLocationUpdate = TRUE;

    dot16Ss->stats.numLocUpd++;
    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Performing Location Update.\n");
    }
}

#if 0

// /**
// FUNCTION   :: MacDot16eSsGetNbrBsByDl
// LAYER      :: MAC
// PURPOSE    :: Get a neighboring BS from the neighbor list by dl channel
//               Add it if not found
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + dlChannel : int            : Downlink channel of the BS
// RETURN     :: MacDot16SsBsInfo* : Pointer to the BS info structure
// **/
static
MacDot16SsBsInfo* MacDot16eSsGetNbrBsByDl(Node* node,
                                          MacDataDot16* dot16,
                                          int dlChannel)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;

    if (DEBUG_NBR_SCAN && 0) // disable it
    {
        printf("Node%d is get BS heard on dl channel %d\n",
               node->nodeId, dlChannel);
    }

    // check if the neighbor already in the list
    bsInfo = dot16Ss->nbrBsList;
    while (bsInfo != NULL)
    {
        if (bsInfo->dlChannel == dlChannel)
        {
            break;
        }

        bsInfo = bsInfo->next;
    }

    if (bsInfo == NULL)
    {
        // not in neighbor list, create it
        bsInfo = (MacDot16SsBsInfo*) MEM_malloc(sizeof(MacDot16SsBsInfo));
        ERROR_Assert(bsInfo != NULL, "MAC 802.16: Out of memory!");
        memset((char*) bsInfo, 0, sizeof(MacDot16SsBsInfo));

        bsInfo->dlChannel = dlChannel;
        if (dot16->duplexMode == DOT16_TDD)
        {
            bsInfo->ulChannel = dlChannel;
        }

        // add into the neighbor BS list
        bsInfo->next = dot16Ss->nbrBsList;
        dot16Ss->nbrBsList = bsInfo;
    }

    return bsInfo;
}

// /**
// FUNCTION   :: MacDot16eSsRemoveNbrBsByDl
// LAYER      :: MAC
// PURPOSE    :: Remove a BS from the neighbor BS list
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + dlChannel : int            : Downlink channel of the BS
// RETURN     :: MacDot16SsBsInfo* : The pointer to the removed BS
// **/
static
MacDot16SsBsInfo* MacDot16eSsRemoveNbrBsByDl(Node* node,
                                             MacDataDot16* dot16,
                                             int dlChannel)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;
    MacDot16SsBsInfo* prevBs;

    // check if the neighbor already in the list
    bsInfo = dot16Ss->nbrBsList;
    prevBs = NULL;
    while (bsInfo != NULL)
    {
        if (bsInfo->dlChannel == dlChannel)
        {
            break;
        }

        prevBs = bsInfo;
        bsInfo = bsInfo->next;
    }

    if (bsInfo != NULL)
    {
        // found, remove it from the list
        if (prevBs == NULL)
        {
            // at the beginning of the list
            dot16Ss->nbrBsList = bsInfo->next;
        }
        else
        {
            prevBs->next = bsInfo->next;
        }
    }

    return bsInfo;
}
#endif
// /**
// FUNCTION   :: MacDot16eSsGetNbrBsByBsId
// LAYER      :: MAC
// PURPOSE    :: Get a neighboring BS from the neighbor list by Bs Id
//               Add it if not found
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + bsId      :unsigned char*  : Pointer to the BS Id
// RETURN     :: MacDot16SsBsInfo* : Pointer to the BS info structure
// **/
static
MacDot16SsBsInfo* MacDot16eSsGetNbrBsByBsId(Node* node,
                                            MacDataDot16* dot16,
                                            unsigned char* bsId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;

    if (DEBUG_NBR_SCAN)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Node%d is adding or updating BS with Bs Id"
               "%d:%d:%d:%d:%d:%d\n",
               node->nodeId, bsId[0], bsId[1],
               bsId[2], bsId[3], bsId[4], bsId[5]);
    }

    // check if the neighbor already in the list
    bsInfo = dot16Ss->nbrBsList;
    while (bsInfo != NULL)
    {
        if (memcmp(bsInfo->bsId, bsId, DOT16_BASE_STATION_ID_LENGTH) == 0)
        {
            break;
        }

        bsInfo = bsInfo->next;
    }

    if (bsInfo == NULL)
    {
        // not in neighbor list, create it
        bsInfo = (MacDot16SsBsInfo*) MEM_malloc(sizeof(MacDot16SsBsInfo));
        ERROR_Assert(bsInfo != NULL, "MAC 802.16: Out of memory!");
        memset((char*) bsInfo, 0, sizeof(MacDot16SsBsInfo));

        memcpy(bsInfo->bsId, bsId, DOT16_BASE_STATION_ID_LENGTH);

        // add into the neighbor BS list
        bsInfo->next = dot16Ss->nbrBsList;
        dot16Ss->nbrBsList = bsInfo;

        if (node->guiOption)
        {
            dot16Ss->stats.numNbrBsScanned ++;
            GUI_SendIntegerData(node->nodeId,
                                dot16Ss->stats.numNbrBsGuiId,
                                dot16Ss->stats.numNbrBsScanned,
                                node->getNodeTime());
        }
    }

    return bsInfo;
}

// /**
// FUNCTION   :: MacDot16eSsRemoveNbrBsByBsId
// LAYER      :: MAC
// PURPOSE    :: Remove a BS from the neighbor BS list
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + bsId      : unsigned char* : Pointer ti the nbr Bs's Id
// RETURN     :: MacDot16SsBsInfo* : The pointer to the removed BS
// **/
static
MacDot16SsBsInfo* MacDot16eSsRemoveNbrBsByBsId(Node* node,
                                             MacDataDot16* dot16,
                                             unsigned char* bsId)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;
    MacDot16SsBsInfo* prevBs;

    // check if the neighbor already in the list
    bsInfo = dot16Ss->nbrBsList;
    prevBs = NULL;
    while (bsInfo != NULL)
    {
        if (memcmp(bsInfo->bsId, bsId, DOT16_BASE_STATION_ID_LENGTH) == 0)
        {
            break;
        }

        prevBs = bsInfo;
        bsInfo = bsInfo->next;
    }

    if (bsInfo != NULL)
    {
        // found, remove it from the list
        if (prevBs == NULL)
        {
            // at the beginning of the list
            dot16Ss->nbrBsList = bsInfo->next;
        }
        else
        {
            prevBs->next = bsInfo->next;
        }

        if (node->guiOption)
        {
            dot16Ss->stats.numNbrBsScanned -= 1;
            GUI_SendIntegerData(node->nodeId,
                                dot16Ss->stats.numNbrBsGuiId,
                                dot16Ss->stats.numNbrBsScanned,
                                node->getNodeTime());
        }
    }

    return bsInfo;
}

// /**
// FUNCTION   :: MacDot16eSsSelectBestNbrBs
// LAYER      :: MAC
// PURPOSE    :: Select the best BS in the nbr BS list for handover
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: MacDot16SsBsInfo* : Pointer to the BS info structure
// **/
static
MacDot16SsBsInfo* MacDot16eSsSelectBestNbrBs(Node* node,
                                             MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBsInfo* bsInfo;
    MacDot16SsBsInfo* bestBs;
    clocktype currentTime;

    if (DEBUG_HO_DETAIL)
    {
        printf("Node%d is selecting the best BS for handover\n",
                node->nodeId);
        MacDot16eSsPrintNbrBsList(node, dot16);
    }

    currentTime = node->getNodeTime();

    // check if the neighbor already in the list
    bsInfo = dot16Ss->nbrBsList;
    bestBs = NULL;
    while (bsInfo != NULL)
    {
        if (bsInfo->lastMeaTime > 0 &&
            bsInfo->lastMeaTime + dot16Ss->para.nbrMeaLifetime >=
            currentTime)
        {
            // only count fresh neighbor BSs

            if (bestBs == NULL)
            {
                bestBs = bsInfo;
            }
            else
            {
                if (bestBs->rssMean < bsInfo->rssMean)
                {
                    bestBs = bsInfo;
                }
            }
        }

        bsInfo = bsInfo->next;
    }

    return bestBs;
}


// /**
// FUNCTION   :: MacDot16SsEnqueueBandwidthRequest
// LAYER      :: MAC
// PURPOSE    :: Create a bandwidth request record to be sent at proper time
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + cid       : Dot16CIDType : CID that BW is requested for
// + bandwidth : int          : Requested bandwidth
// + serviceType : MacDot16ServiceType : Service type of the flow
//                                       for which the BW req is sent
// + isInc     : BOOL         : Whether the BW is for incremental or agg.
// RETURN     :: void : NULL
// **/
static
void MacDot16SsEnqueueBandwidthRequest(Node* node,
                                       MacDataDot16* dot16,
                                       Dot16CIDType cid,
                                       int bandwidth,
                                       MacDot16ServiceType serviceType,
                                       BOOL isInc,
                                       unsigned char serviceFlowIndex,
                                       int priority)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    MacDot16MacHeader* macHeader;

    if (dot16Ss->bwReqType == DOT16_BWReq_CDMA && dot16Ss->bwReqHead != NULL)
    {
        bwReqRecord = dot16Ss->bwReqHead;

        while (bwReqRecord != NULL)
        {
            // update the bw req
            if (bwReqRecord->serviceFlowIndex == serviceFlowIndex
                && bwReqRecord->queuePriority == priority)
            {
                if (MacDot16MacHeaderGetLEN(&bwReqRecord->macHeader) ==
                  (unsigned int)bandwidth)
                {
                    return;
                }
                else
                {
                    // set new b/w requirment
                    MacDot16MacHeaderSetLEN(&bwReqRecord->macHeader,
                                                                bandwidth);
                    return;
                }
            }
            bwReqRecord = bwReqRecord->next;
        }
    }

    if (DEBUG_BWREQ)
    {
        printf("Node%d need to schedule a bw request with size %d\n",
               node->nodeId, bandwidth);
    }

    // make sure the bandwidth size does not overflow
    if ((unsigned int)bandwidth > 0x0007FFFF)
    {
        bandwidth = 0x0007FFFF;
        if (DEBUG_BWREQ)
        {
            printf("    req for BW size %d due to BW field limit %d\n",
                   bandwidth, 0x0007FFFF);
        }
    }

    //create a new record
    bwReqRecord = (MacDot16SsBWReqRecord*)
                      MEM_malloc(sizeof(MacDot16SsBWReqRecord));
    ERROR_Assert(bwReqRecord != NULL, "MAC 802.16: Out of memory!");
    memset((char*) bwReqRecord, 0, sizeof(MacDot16SsBWReqRecord));

    bwReqRecord->serviceType = serviceType;
    bwReqRecord->serviceFlowIndex = serviceFlowIndex;
    bwReqRecord->queuePriority = priority;
    macHeader = &(bwReqRecord->macHeader);
    MacDot16MacHeaderSetHT(macHeader, 1);  // BW request header type
    MacDot16MacHeaderSetCID(macHeader, cid);
    MacDot16MacHeaderSetBR(macHeader, bandwidth);

    if (isInc == FALSE)
    {
        MacDot16MacHeaderSetBandwidthType(macHeader, DOT16_BANDWIDTH_AGG);
    }

    // put it into the BW request list for tx
    if (dot16Ss->bwReqHead == NULL)
    {
        dot16Ss->bwReqHead = bwReqRecord;
        dot16Ss->bwReqTail = bwReqRecord;
    }
    else
    {
        dot16Ss->bwReqTail->next = bwReqRecord;
        dot16Ss->bwReqTail = bwReqRecord;
    }
}

// /**
// FUNCTION   :: MacDot16SsEnqueueMgmtMsg
// LAYER      :: MAC
// PURPOSE    :: Queue a management message to be txed
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + cidType   : MacDot16CidType: Type of the cid for txing the msg
// + mgmtMsg   : Message* : Pointer to the mgmt msg to be enqueued
// RETURN     :: void : NULL
// **/
//static
void MacDot16SsEnqueueMgmtMsg(Node* node,
                              MacDataDot16* dot16,
                              MacDot16CidType cidType,
                              Message* mgmtMsg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int priority;
    BOOL queueIsFull = FALSE;

    if (cidType == DOT16_CID_BASIC)
    {
        priority = DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY;
    }
    else if (cidType == DOT16_CID_PRIMARY)
    {
        priority = DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY;
    }
    else
    {
        priority = DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY;
    }

    // enqueue the packet
    dot16Ss->mgmtScheduler->insert(mgmtMsg,
                                   &queueIsFull,
                                   priority,
                                   NULL,
                                   node->getNodeTime());

    if (queueIsFull)
    {
        MESSAGE_Free(node, mgmtMsg);
    }
}

// /**
// FUNCTION   :: MacDot16SsReallocateBandwidth
// LAYER      :: MAC
// PURPOSE    :: Reallocate the asigned bandwidth among multiple flows
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + bwGranted : int            : Number of byte  can be transmitted
// RETURN     :: NULL
// **/
static
void MacDot16SsReallocateBandwidth(Node* node,
                                   MacDataDot16* dot16,
                                   int bwGranted)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    int i;
    int j;

    if (bwGranted > 0 && dot16Ss->basicMgmtBwRequested > 0)
    {
        // basic mgmt queue
        if (bwGranted > dot16Ss->basicMgmtBwRequested)
        {
            dot16Ss->basicMgmtBwGranted = dot16Ss->basicMgmtBwRequested;
        }
        else
        {
            dot16Ss->basicMgmtBwGranted = bwGranted;
        }
        bwGranted -= dot16Ss->basicMgmtBwGranted;
    }

    if (bwGranted > 0 && dot16Ss->priMgmtBwRequested > 0)
    {
         // primary mgmt queue
        if (bwGranted > dot16Ss->priMgmtBwRequested)
        {
            dot16Ss->priMgmtBwGranted = dot16Ss->priMgmtBwRequested;
        }
        else
        {
            dot16Ss->priMgmtBwGranted = bwGranted;
        }
        bwGranted -= dot16Ss->priMgmtBwGranted;
    }

    if (bwGranted > 0 && dot16Ss->secMgmtBwRequested > 0)
    {
         // // secondary mgmt queue
        if (bwGranted > dot16Ss->secMgmtBwRequested)
        {
            dot16Ss->secMgmtBwGranted = dot16Ss->secMgmtBwRequested;
        }
        else
        {
            dot16Ss->secMgmtBwGranted = bwGranted;
        }
        bwGranted -= dot16Ss->secMgmtBwRequested;
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES && bwGranted > 0; i++)
    {
        // UGS first  then rtPS, then nrtPS, then BE
        for (j = 0; j < DOT16_NUM_SERVICE_TYPES && bwGranted > 0; j ++)
        {
            sFlow = dot16Ss->ulFlowList[j].flowHead;

            while (sFlow != NULL && bwGranted > 0)
            {
                if (sFlow->activated &&
                    sFlow->bwRequested > 0 &&
                    sFlow->serviceType == (MacDot16ServiceType)i)
                {
                     // //
                    if (bwGranted > sFlow->bwRequested)
                    {
                        sFlow->bwGranted = sFlow->bwRequested;
                    }
                    else
                    {
                        sFlow->bwGranted = bwGranted;
                    }
                    bwGranted -= sFlow->bwGranted;
                    if (sFlow->maxBwGranted < sFlow->bwGranted)
                    {
                        sFlow->maxBwGranted = sFlow->bwGranted;
                    }
                    sFlow->lastAllocTime = node->getNodeTime();
                }
                sFlow = sFlow->next;
            } // while
        } // for i
    } // for servType
}

// /**
// FUNCTION   :: MacDot16SsRefreshBandwidthReq
// LAYER      :: MAC
// PURPOSE    :: Refresh the current need of bandwidth for each flow
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: NULL
// **/
static
void MacDot16SsRefreshBandwidthReq(Node* node,
                             MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    int i;
    unsigned int tempBwReq;
    UInt8 crcSize = 0;

    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled &&
        dot16Ss->isPowerDown == TRUE)
    {
        return;
    }

    if (dot16Ss->rngType == DOT16_CDMA &&
        dot16Ss->initInfo.initRangingCompleted == FALSE &&
        dot16Ss->initInfo.rngState == DOT16_SS_RNG_WaitRngRsp)
    {
        return;
    }

    if (dot16Ss->bwReqType == DOT16_BWReq_CDMA && dot16Ss->bwReqHead != NULL)
    {
        MacDot16SsBWReqRecord* bwRec = dot16Ss->bwReqHead;
        MacDot16SsBWReqRecord* prevBwRec = NULL;

        while (bwRec != NULL)
        {
                // update the bw req
                if (bwRec->serviceFlowIndex == 0)
                {
                    // mgmt b/w req
                    tempBwReq = dot16Ss->mgmtScheduler->bytesInQueue(
                                    bwRec->queuePriority);
                }
                else
                {
                    // service flow b/w req
                    tempBwReq = dot16Ss->outScheduler[
                        bwRec->serviceFlowIndex - 1]
                        ->bytesInQueue(bwRec->queuePriority);
                }
            if (tempBwReq == 0)
            {
                // remove the b/w request
                if (bwRec == dot16Ss->bwReqHead)
                {
                    dot16Ss->bwReqHead = dot16Ss->bwReqHead->next;
                    MEM_free(bwRec);
                    bwRec = dot16Ss->bwReqHead;
                    if (dot16Ss->bwReqHead == NULL)
                    {
                        dot16Ss->bwReqTail = NULL;
                        break;
                    }
                    continue;
                }
                else if (bwRec == dot16Ss->bwReqTail)
                {
                    dot16Ss->bwReqTail = prevBwRec;
                    prevBwRec->next = NULL;
                    MEM_free(bwRec);
                    bwRec = NULL;
                    break;
                }
                else
                {
                    prevBwRec->next = bwRec->next;
                    MEM_free(bwRec);
                    bwRec = prevBwRec->next;
                    continue;
                }
            }
            prevBwRec = bwRec;
            bwRec = bwRec->next;
        }
    }


    if (dot16Ss->isCRCEnabled)
    {
        crcSize = DOT16_CRC_SIZE;
    }

    if (DEBUG_BWREQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("refresh the bw  request for this round UL frame\n");
    }

    // currently only aggrregated bandwidth management is fully supported

    // basic mgmt queue
    tempBwReq = dot16Ss->mgmtScheduler->bytesInQueue(
                    DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY);
    if (DEBUG_BWREQ)
    {
         printf("    recalculate the basic bw req bwGrant %d,"
                "last req %d, this time need %d\n",
                dot16Ss->basicMgmtBwGranted,
                dot16Ss->basicMgmtBwRequested, tempBwReq);
    }
    if (dot16Ss->basicMgmtBwRequested > 0 &&
        dot16Ss->basicMgmtBwGranted > 0)
    {
        dot16Ss->lastBasicMgmtBwRequest = dot16Ss->basicMgmtBwRequested;
        dot16Ss->basicMgmtBwRequested = 0;
    }
    if (tempBwReq > 0)
    {
        dot16Ss->basicMgmtBwRequested = tempBwReq;

        // insert a bandwidht request to bw-req list
        //treat mgmt msg as nrtPS
        MacDot16SsEnqueueBandwidthRequest(node,
                                          dot16,
                                          dot16Ss->servingBs->basicCid,
                                          dot16Ss->basicMgmtBwRequested,
                                          DOT16_SERVICE_nrtPS,
                                          FALSE,
                                          0,
                                          DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY);
    }

    tempBwReq = dot16Ss->mgmtScheduler->bytesInQueue(
                    DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY);
    if (DEBUG_BWREQ)
    {
         printf("    recalculate the pri bw req bwGrant %d,"
                "last req%d, this time need %d\n",
                dot16Ss->priMgmtBwGranted,
                dot16Ss->priMgmtBwRequested, tempBwReq);
    }
    if (dot16Ss->priMgmtBwRequested > 0 && dot16Ss->priMgmtBwGranted > 0)
    {
        dot16Ss->lastPriMgmtBwRequest = dot16Ss->priMgmtBwRequested;
        dot16Ss->priMgmtBwRequested = 0;
    }
    if (tempBwReq > 0)
    {
        dot16Ss->priMgmtBwRequested = tempBwReq;

        // insert a bandwidht request to bw-req list, treat as nrtPS
        MacDot16SsEnqueueBandwidthRequest(node,
                                          dot16,
                                          dot16Ss->servingBs->primaryCid,
                                          dot16Ss->priMgmtBwRequested,
                                          DOT16_SERVICE_nrtPS,
                                          FALSE,
                                          0,
                                          DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY);
    }

    if (dot16Ss->para.managed)
    {
        // secondary mgmt queue
        tempBwReq = dot16Ss->mgmtScheduler->bytesInQueue(
                        DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY);

        if (dot16Ss->secMgmtBwRequested > 0)
        {
            dot16Ss->lastSecMgmtBwRequest = dot16Ss->secMgmtBwRequested;
            dot16Ss->secMgmtBwRequested = 0;
        }
        if (tempBwReq > 0)
        {
            dot16Ss->secMgmtBwRequested = tempBwReq;

            // insert a bandwidht request to bw-req list, treat as nrtPS
            MacDot16SsEnqueueBandwidthRequest(
                node,
                dot16,
                dot16Ss->servingBs->secondaryCid,
                dot16Ss->secMgmtBwRequested,
                DOT16_SERVICE_nrtPS,
                FALSE,
                0,
                DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY);
        }
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            UInt32 sFlowBwReq = 0;
            if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
            {
                MacDot16ePSClasses* psClass = NULL;
                psClass = &dot16Ss->psClassParameter[sFlow->psClassType - 1];
                if (psClass->currentPSClassStatus ==
                    POWER_SAVING_CLASS_ACTIVATE &&
                    psClass->isSleepDuration == TRUE)
                {
                    break;
                }
            }

            if (sFlow->isARQEnabled && (sFlow->arqControlBlock != NULL))
            {
                sFlowBwReq = MacDot16ARQCalculateBwReq(sFlow, crcSize);
            }

//            if (sFlowBwReq == 0
//                && dot16Ss->outScheduler[i]->isEmpty(sFlow->queuePriority)
//                == TRUE)
//            {
//                sFlow = sFlow->next;
//                continue;
//            }
            // consider the QoS as well as queue length
            if (i == DOT16_SERVICE_UGS) // UGS
            {
                if (sFlow->activated &&
                    (node->getNodeTime() - sFlow->lastAllocTime) >=
                    sFlow->qosInfo.maxLatency)
                {
                        // for UGS, no BW request, just allocate
                        clocktype timeDiff =
                            node->getNodeTime() - sFlow->lastAllocTime;
                        tempBwReq = (int) (sFlow->qosInfo.maxSustainedRate *
                                         timeDiff / SECOND / 8);

                        if ((int)tempBwReq >= sFlow->qosInfo.minPktSize ||
                            timeDiff >= sFlow->qosInfo.maxLatency)
                        {
                            // now needs to count overhead due to transprot,
                            // IP and MAC headers

                            int actualPktSize = sFlow->qosInfo.minPktSize +
                                sizeof(MacDot16MacHeader) + crcSize;
                            tempBwReq =
                                (tempBwReq / sFlow->qosInfo.minPktSize + 1) *
                                 actualPktSize;

                            // need to allocate BW in this frame
                            if ((int)tempBwReq < actualPktSize)
                            {
                                tempBwReq = actualPktSize;
                            }
                        }
                    // If arq enable then requested b/w size can not be
                    // more than DOT16_ARQ_MAX_BANDWIDTH
                    tempBwReq += sFlowBwReq;
                    if (sFlow->isARQEnabled &&
                        tempBwReq > DOT16_ARQ_MAX_BANDWIDTH)
                    {
                        tempBwReq = DOT16_ARQ_MAX_BANDWIDTH ;
                    }
                    sFlow->bwRequested = tempBwReq ;
                }
                else
                {
                    // do not need BW this round
                    sFlow->bwRequested = 0;
                }
            }
            else if (i == DOT16_SERVICE_ertPS) // extended rtPS
            {
                if (sFlow->activated &&
                    (node->getNodeTime() - sFlow->lastAllocTime) >=
                    sFlow->qosInfo.maxLatency)
                {
                    tempBwReq = dot16Ss->outScheduler[i]->bytesInQueue(
                                sFlow->queuePriority);

                    if (sFlow->isFragStart)
                    {
                        tempBwReq -= sFlow->bytesSent;
                    }

                    tempBwReq = tempBwReq + ((crcSize +
                        sizeof(MacDot16MacHeader))
                        * dot16Ss->outScheduler[i]->numberInQueue(
                        sFlow->queuePriority))+
                        sFlowBwReq;

                    // If arq enable then requested b/w size can not be
                    //  more than DOT16_ARQ_MAX_BANDWIDTH
                    if (sFlow->isARQEnabled &&
                        tempBwReq > DOT16_ARQ_MAX_BANDWIDTH)
                    {
                        tempBwReq = DOT16_ARQ_MAX_BANDWIDTH;
                    }

                    sFlow->bwRequested = tempBwReq;

                    if (DEBUG_BWREQ)
                    {
                        printf("    recalculate the flow w req bwGrant %d,"
                               "last req %d, this time need %d flow cid"
                               " %d\n", sFlow->bwGranted,
                               sFlow->lastBwRequested, sFlow->bwRequested,
                               sFlow->cid);
                    }

                    if (abs((int)tempBwReq - sFlow->lastBwRequested) >
                        DOT16_ertPS_BW_REQ_ADJUST_THRESHOLD)
                    {
                        // if the current negotiated allocation is not
                        // enough or overallocaed SS will send bandwidth
                        //  reuest to adjust the allocation

                        if (sFlow->bwGranted > 6 && PIGGYBACK_ENABLED)
                        {
                            sFlow->needPiggyBackBwReq = TRUE;
                            if (DEBUG_BWREQ)
                            {
                                 printf("   bwGrant %d need piggy bw req,"
                                       "req %d, this time need %d flow cid"
                                       " %d\n", sFlow->bwGranted,
                                       sFlow->bwRequested, tempBwReq,
                                       sFlow->cid);
                            }
                        }
                        else
                        {
                            sFlow->lastBwRequested = sFlow->bwRequested;
                            // insert a bandwidht request to bw-req list
                            MacDot16SsEnqueueBandwidthRequest(
                                                    node,
                                                    dot16,
                                                    sFlow->cid,
                                                    sFlow->bwRequested,
                                                    sFlow->serviceType,
                                                    FALSE,
                                                    (UInt8)(i + 1),
                                                    sFlow->queuePriority);
                        }
                    }
                }
                else
                {
                    sFlow->bwRequested = 0;
                }
            }
            else if (sFlow->activated)
            {
                tempBwReq = dot16Ss->outScheduler[i]->bytesInQueue(
                                sFlow->queuePriority) + sFlowBwReq;

                if (sFlow->isFragStart)
                {
                    tempBwReq -= sFlow->bytesSent;
                }

                if (DEBUG_BWREQ)
                {
                    printf("    recalculate the flow w req bwGrant %d,"
                           "last req %d, this time need %d flow cid %d\n",
                           sFlow->bwGranted, sFlow->lastBwRequested,
                           tempBwReq, sFlow->cid);
                }
                if (sFlow->bwRequested > 0 && sFlow->bwGranted > 0)
                {
                    sFlow->lastBwRequested = sFlow->bwRequested;
                    sFlow->bwRequested = 0;
                }
                if (tempBwReq > 0)
                {
                    tempBwReq = tempBwReq +
                        ((crcSize + sizeof(MacDot16MacHeader)) *
                        dot16Ss->outScheduler[i]->numberInQueue(
                        sFlow->queuePriority));

                // If arq is enabled then requested b/w size can not be more
                // than DOT16_ARQ_MAX_BANDWIDTH

                    if (sFlow->isARQEnabled &&
                        tempBwReq > DOT16_ARQ_MAX_BANDWIDTH)
                    {
                        tempBwReq = DOT16_ARQ_MAX_BANDWIDTH ;
                    }

                    sFlow->bwRequested = tempBwReq ;

                    if (sFlow->bwGranted > 6 && PIGGYBACK_ENABLED)
                    {
                        sFlow->needPiggyBackBwReq = TRUE;
                        if (DEBUG_BWREQ)
                        {
                             printf("   bwGrant %d need piggy bw req,"
                                   "req %d, this time need %d flow cid"
                                   " %d\n", sFlow->bwGranted,
                                   sFlow->bwRequested, tempBwReq,
                                   sFlow->cid);
                        }
                    }
                    else
                    {
                        // insert a bandwidht request to bw-req list
                        MacDot16SsEnqueueBandwidthRequest(
                                                    node,
                                                    dot16,
                                                    sFlow->cid,
                                                    sFlow->bwRequested,
                                                    sFlow->serviceType,
                                                    FALSE,
                                                    (UInt8)(i + 1),
                                                    sFlow->queuePriority);
                    }
                }
            }
            sFlow = sFlow->next;
        } // while
    } // for i
}// end of MacDot16SsRefreshBandwidthReq

// /**
// FUNCTION   :: MacDot16SsScheduleBandwidthRequest
// LAYER      :: MAC
// PURPOSE    :: Schedule a list of bandwidth requests to be transmittd
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + numFreeBytes : int         : Size of the available space
// + isUnicastRequestOpp : BOOL : Is this an unicast request opp?
// + bwMsgSize : int*           : Return total size in bytes of bw requests
// RETURN     :: Message* : Point to the first bw request msg
//                          NULL if no any scheduled.
// **/
static
Message* MacDot16SsScheduleBandwidthRequest(Node* node,
                                            MacDataDot16* dot16,
                                            int numFreeBytes,
                                            BOOL isUnicastRequestOpp,
                                            int* bwMsgSize)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    MacDot16MacHeader* payload;
    Message* firstMsg = NULL;
    Message* lastMsg = NULL;
    Message* msg;

    int bwReqSize = sizeof(MacDot16MacHeader);
    int msgSize = 0;

    if (numFreeBytes < bwReqSize || dot16Ss->bwReqType == DOT16_BWReq_CDMA)
    {
        *bwMsgSize = 0;
        return NULL;
    }

    while ((numFreeBytes - msgSize) >= bwReqSize &&
           (dot16Ss->bwReqHead != NULL || dot16Ss->bwReqInContOpp!=NULL))
        {
        if (dot16Ss->bwReqHead)
        {
            bwReqRecord = dot16Ss->bwReqHead;
        }
        else
        {
            bwReqRecord = dot16Ss->bwReqInContOpp;
        }
        // allocate message
        msg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(msg != NULL, "MAC802.16: Out of memory!");
        MESSAGE_PacketAlloc(node, msg, bwReqSize, TRACE_DOT16);
        payload = (MacDot16MacHeader*) MESSAGE_ReturnPacket(msg);

        // copy over the content of the request
        memcpy(payload, (char*) &(bwReqRecord->macHeader), bwReqSize);

        // put into the buffer
        msg->next = NULL;
        if (firstMsg == NULL)
        {
            firstMsg = msg;
            lastMsg = msg;
        }
        else
        {
            lastMsg->next = msg;
            lastMsg = msg;
        }

        msgSize += bwReqSize;

        // Free the record
        if (dot16Ss->bwReqHead)
        {
            dot16Ss->bwReqHead = bwReqRecord->next;
            if (dot16Ss->bwReqHead == NULL)
            {
                dot16Ss->bwReqTail = NULL;
            }
        }
        else
        {
            dot16Ss->bwReqInContOpp = NULL;
        }
        MEM_free(bwReqRecord);

        if (DEBUG_BWREQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            if (!isUnicastRequestOpp)
            {
                printf("send unicast BW req as data grant with BW %d\n",
                       MacDot16MacHeaderGetBR(payload));
            }
            else
            {
                printf("send unicast BW req in polled Req Opp with BW %d\n",
                       MacDot16MacHeaderGetBR(payload));
            }
        }
    }

    *bwMsgSize = msgSize;
    return firstMsg;
}

// /**
// FUNCTION   :: MacDot16eSsCalcUnavlblInt
// LAYER      :: MAC
// PURPOSE    :: Calculate MS paging unavailable interval
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: clocktype  : MS Paging unavailable interval
// **/
static
clocktype MacDot16eSsCalcUnavlblInt(Node* node,
                            MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    clocktype unavlblInt;
    int unavlbIntFrames;    //in frames

    unavlbIntFrames = dot16Ss->servingBs->pagingOffset -
                        (dot16Ss->servingBs->frameNumber %
                            dot16Ss->servingBs->pagingCycle);

    if (dot16Ss->servingBs->pagingOffset <
        (dot16Ss->servingBs->frameNumber %
        dot16Ss->servingBs->pagingCycle))
    {
        unavlbIntFrames += dot16Ss->servingBs->pagingCycle;
    }
    else
    {
        unavlbIntFrames = dot16Ss->servingBs->pagingCycle -
            dot16Ss->servingBs->pagingOffset + unavlbIntFrames;
    }
    unavlblInt = unavlbIntFrames -1;
    unavlblInt = (unavlblInt) *
                    (clocktype)(dot16Ss->servingBs->frameDuration);

    //add duration of current frame
        unavlblInt += node->getNodeTime() - dot16Ss->servingBs->frameStartTime;
    return unavlblInt;
}

static
void MacDot16eStopTimersBeforeIdle(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    //determine time till MS Paging listening interval
    //switch off the node for MS paging unavailable interval
    if (dot16Ss->timers.timerMSPagingUnavlbl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerMSPagingUnavlbl);
        dot16Ss->timers.timerMSPagingUnavlbl = NULL;
    }
    dot16Ss->timers.timerMSPagingUnavlbl = MacDot16SetTimer(
            node,
            dot16,
            DOT16e_TIMER_MSPagingUnavlbl,
            MacDot16eSsCalcUnavlblInt(node, dot16),
                                      NULL,
                                      0);
    dot16Ss->idleSubStatus = DOT16e_SS_IDLE_Idle;
    // clean service flows
    while (dot16Ss->bwReqHead != NULL)
    {
        bwReqRecord = dot16Ss->bwReqHead->next;
        MEM_free(dot16Ss->bwReqHead);
        dot16Ss->bwReqHead = bwReqRecord;
    }
    dot16Ss->bwReqHead = NULL;
    dot16Ss->bwReqTail = NULL;

    // clean scheduling variables
    if (dot16Ss->ulBurstHead != NULL)
    {
        MacDot16UlBurst* ulBurst;
        MacDot16UlBurst* tmpUlBurst;

        ulBurst = dot16Ss->ulBurstHead;
        while (ulBurst != NULL)
        {
            tmpUlBurst = ulBurst->next;
            MEM_free(ulBurst);
            ulBurst = tmpUlBurst;
        }

        dot16Ss->ulBurstHead = NULL;
        dot16Ss->ulBurstTail = NULL;
    }

    MacDot16SsFreeServiceFlowList(node, dot16);

    // clean the mgmt scheduler
    MacDot16SsFreeQueuedMsgInMgmtScheduler(node, dot16);

    // clean the contents in ourBuffer
    MESSAGE_FreeList(node, dot16Ss->outBuffHead);
    dot16Ss->outBuffHead = NULL;
    dot16Ss->outBuffTail = NULL;

    // clean cached timers
    if (dot16Ss->timers.timerT1 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT1);
    }
    if (dot16Ss->timers.timerT2 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT2);
    }
    if (dot16Ss->timers.timerT3 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
    }
    if (dot16Ss->timers.timerT4 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT4);
    }

    if (dot16Ss->timers.timerT6 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT6);
    }
    if (dot16Ss->timers.timerT12 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
    }
    if (dot16Ss->timers.timerT16 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT16);
    }

    if (dot16Ss->timers.timerT18 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT18);
    }
    if (dot16Ss->timers.timerT20 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT20);
    }
    if (dot16Ss->timers.timerT21 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT21);
    }

    if (dot16Ss->timers.timerLostDl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostDl);
    }
    if (dot16Ss->timers.timerLostUl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostUl);
    }

    if (dot16Ss->timers.frameEnd != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.frameEnd);
    }

    if (dot16Ss->timers.timerT41 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT41);
    }
    if (dot16Ss->timers.timerT42 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT42);
    }
    if (dot16Ss->timers.timerT44 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT44);
    }

    if (dot16Ss->timers.timerT29 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT29);
    }
    memset((char*) &(dot16Ss->timers), 0, sizeof(dot16Ss->timers));

    MacDot16StopListening(node,
                          dot16,
                          dot16Ss->servingBs->dlChannel);

}

// /**
// FUNCTION   :: MacDot16eSsSwitchToIdle
// LAYER      :: MAC
// PURPOSE    :: Start switching to Idle mode1
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: None
// **/
static
void MacDot16eSsSwitchToIdle(Node* node,
                            MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Switching to Idle mode\n");
    }

    if (dot16Ss->timers.timerLocUpd != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLocUpd);
    }
    dot16Ss->timers.timerLocUpd = MacDot16SetTimer(
                                            node,
                                            dot16,
                                            DOT16e_TIMER_LocUpd,
                                            dot16Ss->para.tLocUpdInterval,
                                            NULL,
                                            0);
    MacDot16eStopTimersBeforeIdle(node, dot16);
}

// /**
// FUNCTION   :: MacDot16SsHandleDlMapPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DL-MAP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// + rxBeginTime: clocktype     : Signal arrival time of the received frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDlMapPdu(Node* node,
                             MacDataDot16* dot16,
                             unsigned char* macFrame,
                             int startIndex,
                             clocktype rxBeginTime)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16DlMapMsg* dlMap;
    int index;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Node%d handle DL-MAP message\n", node->nodeId);
    }

    // skip the MAC header
    index = startIndex;
    index += sizeof(MacDot16MacHeader);

    dlMap = (MacDot16DlMapMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DlMapMsg);

    // get frame number
    dot16Ss->servingBs->frameNumber = MacDot16PhyGetFrameNumber(
                                            node,
                                            dot16,
                                            &(macFrame[index]));

    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled
        && DEBUG_SLEEP_FRAME_NUMBER)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("SS received Frame Number: %d\n",
            dot16Ss->servingBs->frameNumber);
    }

    dot16Ss->servingBs->frameStartTime = rxBeginTime;

    if (dot16Ss->initInfo.dlSynchronized == FALSE)
    {
         // keep base station Id
        MacDot16CopyStationId(dot16Ss->servingBs->bsId,
                              dlMap->baseStationId);

        // get frame duration
        dot16Ss->servingBs->frameDuration = MacDot16PhyGetFrameDuration(
                                                node,
                                                dot16,
                                                &(macFrame[index]));

        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd first DL-MAP on the give channel "
                   "index %d, keep the BS Id [%d:%d:%d:%d:%d:%d]\n",
                    dot16Ss->chScanIndex,
                    dlMap->baseStationId[0], dlMap->baseStationId[1],
                    dlMap->baseStationId[2], dlMap->baseStationId[3],
                    dlMap->baseStationId[4], dlMap->baseStationId[5]);
        }

        // stop T21
        if (dot16Ss->timers.timerT21 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT21);
            dot16Ss->timers.timerT21 = NULL;
        }

        if (dot16Ss->timers.timerLostDl != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostDl);
            dot16Ss->timers.timerLostDl = NULL;
        }

        //start a Lost DL-MAP timer
        dot16Ss->timers.timerLostDl = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_LostDlSync,
                                          dot16Ss->para.lostDlInterval,
                                          NULL,
                                          0);

        // start timeout timer T1 for DCD
        if (dot16Ss->timers.timerT1 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT1);
            dot16Ss->timers.timerT1 = NULL;
        }
        dot16Ss->timers.timerT1 = MacDot16SetTimer(
                                      node,
                                      dot16,
                                      DOT16_TIMER_T1,
                                      dot16Ss->para.t1Interval,
                                      NULL,
                                      0);

        // start a timeout timer T12 for UCD
        if (dot16Ss->timers.timerT12 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
            dot16Ss->timers.timerT12 = NULL;
        }
        dot16Ss->timers.timerT12 = MacDot16SetTimer(
                                       node,
                                       dot16,
                                       DOT16_TIMER_T12,
                                       dot16Ss->para.t12Interval,
                                       NULL,
                                       0);

        dot16Ss->initInfo.dlSynchronized = TRUE;

        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd first DL-MAP,"
                   "Cancel T21 and start Lost DL-MAP timer,"
                   "T1 for DCD and T12 to wait for UCD \n");
            printf("    achieve MAC Syncronization"
                   " to downlink index %d\n",
                    dot16Ss->chScanIndex);
        }
    }
    else if (dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
    {
        // rcvd dl-map after sycn
        if (dot16Ss->timers.timerLostDl != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostDl);
            dot16Ss->timers.timerLostDl = NULL;
        }

        // reset the timer Lost DL-MAP
        dot16Ss->timers.timerLostDl = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_LostDlSync,
                                          dot16Ss->para.lostDlInterval,
                                          NULL,
                                          0);
        if (DEBUG_NETWORK_ENTRY && DEBUG)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Rcvd DL-MAP from index %d, reset Lost DL-MAP Timer\n",
                    dot16Ss->chScanIndex );
        }

        if (dot16Ss->initInfo.initStatus == DOT16_SS_INIT_ObtainUlParam &&
            dot16Ss->initInfo.ulParamAccquired == TRUE)
        {
            dot16Ss->initInfo.initStatus = DOT16_SS_INIT_RangingAutoAdjust;
            if (DEBUG_NETWORK_ENTRY)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("rcvd the 1st DL-MAP after UL param "
                        "accquired, so move the init staus to Ranging\n");
            }
        }
    }

    // update DCD count and dl burst profile if needed
    // this is used when DCD count changed at BS
    // when DL map uses new generation dcd count, it is time for SS to use
    // new generation burst profile, before that SS still uses
    // old burst profile though DCD count in DCD msg has been changed
    if (dlMap->dcdCount != dot16Ss->servingBs->dcdCount &&
        dlMap->dcdCount == dot16Ss->servingBs->nextGenDcdCount &&
        dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
    {
        dot16Ss->servingBs->dcdCount = dlMap->dcdCount;
        memcpy((char*) &(dot16Ss->servingBs->dlBurstProfile[0]),
               (char*) &(dot16Ss->servingBs->nextGenDlBurstProf[0]),
               sizeof(MacDot16DlBurstProfile) *
               dot16Ss->numOfDLBurstProfileInUsed);
    }

    // handle DL-MAP_IEs

    // if 802.16e supported
    if (dot16->dot16eEnabled)
    {
        MacDot16SsBsInfo* nbrInfo;

        if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_InScan)
        {
            // in neighbor scan period, update nbr BS list
            nbrInfo =
                MacDot16eSsGetNbrBsByBsId(
                node,
                dot16,
                dlMap->baseStationId);
            if (nbrInfo != NULL)
            {
                nbrInfo->dlChannel =
                    dot16->channelList[dot16Ss->nbrChScanIndex];
                if (dot16->duplexMode == DOT16_TDD)
                {
                    nbrInfo->ulChannel = nbrInfo->dlChannel;
                }

                // get frame duration
                nbrInfo->frameDuration = MacDot16PhyGetFrameDuration(
                                            node,
                                            dot16,
                                            &(macFrame[index]));
                // piont to this neighbor as the traget BS for scan or ho
                dot16Ss->targetBs = nbrInfo;
            }
        }
    }
    // end if 802.16e support

    // increase the statistics
    dot16Ss->stats.numDlMapRcvd ++;

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleUlMapPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing UL-MAP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// + rxBeginTime: clocktype     : Signal arrival time of the received frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleUlMapPdu(Node* node,
                             MacDataDot16* dot16,
                             unsigned char* macFrame,
                             int startIndex,
                             clocktype rxBeginTime)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16UlMapMsg* ulMap;
    MacDot16UlBurst* burst;
    int index;
    int length;
    int burstId;

    Dot16CIDType cid;
    unsigned char uiuc;
    clocktype duration;
    clocktype timePassed;
    int allocStartTime;

    int totalDuration;
    int bwGranted;

    if (DEBUG_BWREQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Handle UL MAP\n");
    }
    // skip the MAC header
    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    ulMap = (MacDot16UlMapMsg*) &(macFrame[index]);
    index += sizeof(MacDot16UlMapMsg);

    allocStartTime = MacDot16FourByteToInt(ulMap->allocStartTime);
    timePassed = node->getNodeTime() - rxBeginTime;
    dot16Ss->ulDurationInPs = (int)((dot16Ss->servingBs->frameDuration -
                               allocStartTime - dot16Ss->para.rtg) /
                               dot16->ulPsDuration);
    dot16Ss->ulStartTime = node->getNodeTime() + allocStartTime -
                           timePassed;
    if (dot16Ss->ulStartTime < node->getNodeTime())
    {
        dot16Ss->ulStartTime = node->getNodeTime();
    }

    // reset request opp variable
    dot16Ss->contBwReqOpp = FALSE;
    dot16Ss->ucastBwReqOpp = FALSE;
    bwGranted = 0;
    dot16Ss->isCdmaAllocationIERcvd = FALSE;

    if (dot16Ss->initInfo.initStatus < DOT16_SS_INIT_RangingAutoAdjust)
    {
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd a UL map before SS "
                   "is ready to handle it from ch %d, discard it\n",
                   dot16Ss->chScanIndex);
        }
        // increase statistics
        dot16Ss->stats.numUlMapRcvd ++;

        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update UCD count and ul burst prfile if needed
    // this is used when UCD count changed at BS
    // when UL map uses new generation ucd count, it is time for SS to use
    // new generation burst profile, before that SS still uses
    // old burst profile though UCD count in UCD msg has been changed
    if (ulMap->ucdCount != dot16Ss->servingBs->ucdCount &&
        ulMap->ucdCount == dot16Ss->servingBs->nextGenUcdCount)
    {
        dot16Ss->servingBs->ucdCount = ulMap->ucdCount;
        memcpy((char*) &(dot16Ss->servingBs->ulBurstProfile[0]),
               (char*) &(dot16Ss->servingBs->nextGenUlBurstProf[0]),
               sizeof(MacDot16UlBurstProfile) *
               dot16Ss->numOfULBurstProfileInUsed);
    }

    // now SS is ready to handle UL map
    // reset UL MAP timer
    if (dot16Ss->timers.timerLostUl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostUl);
        dot16Ss->timers.timerLostUl = NULL;
    }

    dot16Ss->timers.timerLostUl = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_LostUlSync,
                                          dot16Ss->para.lostUlInterval,
                                          NULL,
                                          0);
    if (DEBUG && DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd UL Map from ch "
               "%d and reset Lost UL Map Timer\n",
               dot16Ss->chScanIndex);
    }

    if (//dot16->dot16eEnabled == FALSE ||
        dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
    {
        // set a timer for the beginning of the UL part
        MacDot16SetTimer(
            node,
            dot16,
            DOT16_TIMER_FrameUlBegin,
            dot16Ss->ulStartTime - node->getNodeTime(),
            NULL,
            0);

        // handle the UL-MAP_IEs
        length = MacDot16MacHeaderGetLEN(macHeader) + startIndex;
        totalDuration = 0;
        burstId = 0;
        while (index < length)
        {
            BOOL CdmaAllocationIEFlag = FALSE;
            MacDot16GenericPhyOfdmaUlMapIE recCDMAInfo;
// note: read the CDMA corresponding confifuration parameter
            index += MacDot16PhyGetUlMapIE(node,
                                           dot16,
                                           (unsigned char*) macFrame,
                                           index,
                                           &cid,
                                           &uiuc,
                                           &duration,
                                           &recCDMAInfo);

            if (MacDot16IsMyCid(node, dot16, cid) &&
                uiuc != DOT16_UIUC_END_OF_MAP)
            {
                // this burst is applicable to me
                burst = (MacDot16UlBurst*)
                        MEM_malloc(sizeof(MacDot16UlBurst));
                burst->index = burstId++;
                burst->cid = cid;
                burst->uiuc = uiuc;
                burst->durationOffset = totalDuration;
                burst->duration = (int) duration;
                burst->next = NULL;
                burst->isCdmaAllocationIEReceived = FALSE;
                burst->needSendPaddingInDataBurst = FALSE;
                if (uiuc == DOT16_UIUC_CDMA_RANGE)
                {
                    memcpy(&burst->ulCdmaInfo.ulMapIE12,
                        &recCDMAInfo.ulMapIE12,
                        sizeof(MacDot16PhyOfdmaUlMapIEuiuc12));
                    if (DEBUG_CDMA_RANGING)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("Received CDMA ranging channel information"
                            " in UL MAP pdu\n");
                    }
                    burst->startTime = dot16Ss->ulStartTime;
                }
                else
                {
                    if (uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE)
                    {
                        memcpy(&burst->ulCdmaInfo.ulMapIE14,
                            &recCDMAInfo.ulMapIE14,
                            sizeof(MacDot16PhyOfdmaUlMapIEuiuc14));

                        if (isInitialRangingCode(recCDMAInfo.ulMapIE14.\
                            rangingCode))
                        {
                            if ((dot16Ss->sendCDMAInfo.rangingCode ==
                                recCDMAInfo.ulMapIE14.rangingCode)
                                && (dot16Ss->sendCDMAInfo.ofdmaFrame ==
                                recCDMAInfo.ulMapIE14.frameNumber)
                                && (dot16Ss->sendCDMAInfo.ofdmaSubchannel ==
                                recCDMAInfo.ulMapIE14.rangingSubchannel)
                                && ((dot16Ss->sendCDMAInfo.ofdmaSymbol ==
                                recCDMAInfo.ulMapIE14.rangingSymbol) ||
                                (dot16Ss->sendCDMAInfo.ofdmaSymbol ==
                                (recCDMAInfo.ulMapIE14.rangingSymbol + 1))))
                            {
                                if (DEBUG_CDMA_RANGING)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("Received CDMA allocation IE "
                                        "information in UL MAP pdu packet\n");
                                }
                                burst->isCdmaAllocationIEReceived = TRUE;
                                dot16Ss->isCdmaAllocationIERcvd = TRUE;
                                CdmaAllocationIEFlag = TRUE;
                            }
                        }
                        else if (isBWReqRangingCode(recCDMAInfo.ulMapIE14.\
                            rangingCode))
                        {
                            MacDot16RngCdmaInfo* cdmaInfo;
                            MacDot16SsBWReqRecord* bwReqRecord =
                                dot16Ss->bwReqHead;
                            while (bwReqRecord != NULL)
                            {
                                cdmaInfo = &(bwReqRecord->cdmaInfo);
                                if ((recCDMAInfo.ulMapIE14.rangingCode ==
                                    cdmaInfo->rangingCode)
                                    && (recCDMAInfo.ulMapIE14.\
                                    rangingSubchannel
                                    == cdmaInfo->ofdmaSubchannel)
                                    &&(recCDMAInfo.ulMapIE14.rangingSymbol
                                    == cdmaInfo->ofdmaSymbol)
                                    &&(recCDMAInfo.ulMapIE14.frameNumber
                                    == cdmaInfo->ofdmaFrame))
                                {
                                    burst->isCdmaAllocationIEReceived = TRUE;
                                    bwReqRecord->isCDMAAllocationIERecvd =
                                        TRUE;
                                    CdmaAllocationIEFlag = TRUE;
                                    break;
                                }
                                bwReqRecord = bwReqRecord->next;
                            } // END OF WHILE
                        }
                    }
                    else
                    {
                        memset(&burst->ulCdmaInfo,
                            0,
                            sizeof(MacDot16GenericPhyOfdmaUlMapIE));
                    }
                    // calculate the start time on time-axis of this burst
                    if ((totalDuration % dot16Ss->ulDurationInPs + duration -
                        dot16Ss->para.sstgInPs) > dot16Ss->ulDurationInPs)
                    {
                        // part of this burst is rounded to the beginning of
                        // next subchannel
                        burst->startTime = dot16Ss->ulStartTime;
                    }
                    else
                    {
                        // this burst starts in the middle of UL, which is end
                        // of the previous bursts
                        burst->startTime = dot16Ss->ulStartTime +
                                           (totalDuration %
                                           dot16Ss->ulDurationInPs) *
                                           dot16->ulPsDuration;
                    }
                }
                if (dot16Ss->ulBurstHead == NULL)
                {
                    dot16Ss->ulBurstHead = burst;
                    dot16Ss->ulBurstTail = burst;
                }
                else
                {
                    dot16Ss->ulBurstTail->next = burst;
                    dot16Ss->ulBurstTail = burst;
                }

                // count the BW allocated
                if (MacDot16SsIsDataGrantBurst(node, dot16, uiuc)
                    || CdmaAllocationIEFlag)
                {
                    int bitsPerPs;
                    unsigned char profileIndex = burst->uiuc - 1;
                    if (burst->uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE)
                    {
                        profileIndex = 0;
                    }

                    bitsPerPs = MacDot16PhyBitsPerPs(
                                    node,
                                    dot16,
                                    &(dot16Ss->servingBs->ulBurstProfile
                                        [profileIndex]),
                                    DOT16_UL);
                    bwGranted += (burst->duration - dot16Ss->para.sstgInPs)
                                 * bitsPerPs / 8;
                    if (CdmaAllocationIEFlag)
                    {
                        dot16Ss->basicMgmtBwRequested += (burst->duration
                            - dot16Ss->para.sstgInPs) * bitsPerPs / 8;
                    }
                    if (DEBUG_BWREQ)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("get alloc BW %d PS %d UIUC %d\n",
                               (burst->duration - dot16Ss->para.sstgInPs) *
                               bitsPerPs / 8,
                               burst->duration - dot16Ss->para.sstgInPs,
                               burst->uiuc);

                        printf("\t get total BW %d granted till now \n",
                               bwGranted);
                    }
                }

                //for initial ranging in contention burst
                if (cid == DOT16_BROADCAST_CID &&
                    (uiuc == DOT16_UIUC_RANGE ||
                    uiuc == DOT16_UIUC_CDMA_RANGE) &&
                    ((dot16Ss->initInfo.rngState ==
                        DOT16_SS_RNG_WaitBcstRngOpp) ||
                        (dot16Ss->initInfo.rngState ==
                        DOT16_SS_RNG_WaitBcstInitRngOpp)))
                {
                    //move the rng status
                    if (dot16Ss->initInfo.rngState ==
                        DOT16_SS_RNG_WaitBcstInitRngOpp)
                    {
                        dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitInvitedRngRspForInitOpp;
                    }
                    else
                    {
                        dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_BcstRngOppAlloc;
                    }
                    if (dot16Ss->operational == FALSE)
                    {
                        //since SS get a UL-Map with ranging opp,
                        // cancel the T2
                        if (dot16Ss->timers.timerT2 != NULL)
                        {
                            MESSAGE_CancelSelfMsg(
                                    node, dot16Ss->timers.timerT2);
                            dot16Ss->timers.timerT2 = NULL;
                        }
                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("rcvd a UL map with contention ranging"
                                   " opp from ch %d, change rng staut from"
                                   " waitBcstRngOpp to BcstRngOppAlloc and"
                                   " cancel Timer T2 if running\n",
                                   dot16Ss->chScanIndex);
                        }
                    }
                }
                // for initial ranging in invited unicast burst
                else if (cid >= DOT16_BASIC_CID_START &&
                         cid <= DOT16_BASIC_CID_END &&
                         (uiuc == DOT16_UIUC_RANGE ||
                         uiuc == DOT16_UIUC_CDMA_RANGE) &&
                         dot16Ss->initInfo.rngState ==
                             DOT16_SS_RNG_WaitInvitedRngOpp)
                {
                    dot16Ss->initInfo.rngState = DOT16_SS_RNG_RngInvited;
                    if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("rcvd a UL map with unicast rannge opp "
                               "from ch %d , change stauts from "
                               "waitInvitedRngOpp ro RngInvited\n",
                                dot16Ss->chScanIndex);
                    }
                } //end initial ranging
                //for periodic ranging
                else if (dot16Ss->initInfo.initRangingCompleted == TRUE &&
                        (MacDot16SsIsDataGrantBurst(node, dot16, uiuc) ||
                        ((uiuc == DOT16_UIUC_RANGE ||
                        uiuc == DOT16_UIUC_CDMA_RANGE) && cid !=
                        DOT16_BROADCAST_CID)))
                {
                    // as long as there is transmission opportunities
                    if (DEBUG_RANGING)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("rcvd a data grant/period range"
                                " opp, reset T4\n");
                    }

                    // restart T4
                    if (dot16Ss->timers.timerT4 != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node,
                                    dot16Ss->timers.timerT4);
                        dot16Ss->timers.timerT4 = NULL;
                    }
                    dot16Ss->timers.timerT4 =
                                MacDot16SetTimer(
                                            node,
                                            dot16,
                                            DOT16_TIMER_T4,
                                            dot16Ss->para.t4Interval,
                                            NULL,
                                            0);
                    // schedule a RNG-REQ if needed
                    if ((dot16Ss->periodicRngInfo.lastAnomaly ==
                        ANOMALY_ERROR ||
                        dot16Ss->periodicRngInfo.lastStatus ==
                        RANGE_CONTINUE) &&
                        dot16Ss->periodicRngInfo.toSendRngReq == FALSE &&
                        dot16Ss->periodicRngInfo.rngReqScheduled == FALSE
                        && (dot16Ss->rngType != DOT16_CDMA))
                    {
                        // only the first data grant set toSendRngReq = TRUE
                        // to avoid multiple RNG-REQ
                        // schedule a RNG-REQ with anomaly
                        dot16Ss->periodicRngInfo.toSendRngReq = TRUE;
                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("periodic range need to send "
                                   "RNG-REQ since lastStatus is continue "
                                   "or lastanomaly is error \n");
                        }
                    }
                } // end periodic ranging

                //request opps
                else if ((uiuc == DOT16_UIUC_REQUEST)
                    || (uiuc == DOT16_UIUC_CDMA_RANGE))
                {
                    if ((cid >= DOT16_MULTICAST_POLLING_CID_START &&
                        cid <= DOT16_MULTICAST_POLLING_CID_END) ||
                        cid == DOT16_BROADCAST_CID)
                    {
                        // if the request opp is a braodcast or multicast
                        // (both are contention based) opp
                        dot16Ss->contBwReqOpp = TRUE;
                    }
                    else if ((cid >= DOT16_BASIC_CID_START &&
                             cid <= DOT16_BASIC_CID_END) ||
                             (cid >= DOT16_TRANSPORT_CID_START &&
                              cid <= DOT16_TRANSPORT_CID_END))
                    {
                        // if the request opp is unicast poll
                        dot16Ss->ucastBwReqOpp = TRUE;
                    }
                } // end request opp
            }
            totalDuration += (int) duration;
        }
    }

    dot16Ss->aggBWGranted = bwGranted; // BW granted this round

    if (bwGranted > 0 || dot16Ss->ucastBwReqOpp)
    {
        // if in the last period a contention BW req is sent,
        // cancel the timer and free the request
        // it could also in the defer state, since get a ucast opp or
        // data grant,
        // do not use contention one
        if (DEBUG_BWREQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("get bw grant/unicast bw req assigned to this ss\n");
        }
        if (dot16Ss->bwReqType != DOT16_BWReq_CDMA
            && dot16Ss->bwReqInContOpp != NULL)
        {
            // bandwidth granted, free the request msg and clear timer
            MEM_free(dot16Ss->bwReqInContOpp);
            dot16Ss->bwReqInContOpp = NULL;

            if (dot16Ss->timers.timerT16 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT16);
                dot16Ss->timers.timerT16 = NULL;
            }

            dot16Ss->isWaitBwGrant = FALSE;

            dot16Ss->servingBs->requestRetries = 0;
            dot16Ss->servingBs->requestBOCount =
                            dot16Ss->para.requestBOStart;
            dot16Ss->servingBs->requestBOValue = 0;
        }
    }

    // redistribute the assigned BW to each prioirty queue
    if (bwGranted)
    {
        if (bwGranted > dot16Ss->bwRequestedForOutBuffer)
        {
            bwGranted -= dot16Ss->bwRequestedForOutBuffer;
        }

        if (dot16Ss->bwRequestedForOutBuffer && DEBUG_BWREQ)
        {
            printf(" ***** %d allocated BW is used"
                   "by the outbuffered packets\n",
                   dot16Ss->bwRequestedForOutBuffer);
        }

        if (bwGranted > 0)
        {
            MacDot16SsReallocateBandwidth(node, dot16, bwGranted);
            MacDot16UlBurst* burst = dot16Ss->ulBurstHead;
            while (burst != NULL)
            {
                if (MacDot16SsIsDataGrantBurst(node, dot16, burst->uiuc))
                {
                    int bitsPerPs;
                    int localBwGranted;
                    bitsPerPs = MacDot16PhyBitsPerPs(
                                    node,
                                    dot16,
                                    &(dot16Ss->servingBs->ulBurstProfile
                                        [burst->uiuc - 1]),
                                    DOT16_UL);
                    localBwGranted = (burst->duration -
                        dot16Ss->para.sstgInPs)
                                 * bitsPerPs / 8;
                    if (MacDot16ScheduleUlBurst(node, dot16, localBwGranted)
                        == localBwGranted &&
                        dot16Ss->bwRequestedForOutBuffer == 0)
                    {
                        // if no data packet need to be sent, then
                        // padding data need to send
                        burst->needSendPaddingInDataBurst = TRUE;
                        if (DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("need to send padding data in data"
                                " burst\n");
                        }
                    }
                }
                burst = burst->next;
            }
        }
        else
        {
            if (DEBUG_BWREQ)
            {
                printf(" *****all the allocated BW is used"
                       "by the outbuffered packets\n");
            }
        }
    }

    // set a timer for the end of the whole frame
    if (dot16Ss->timers.frameEnd != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.frameEnd);
    }
    dot16Ss->timers.frameEnd =
        MacDot16SetTimer(
            node,
            dot16,
            DOT16_TIMER_FrameEnd,
            dot16Ss->servingBs->frameDuration - timePassed -
            dot16Ss->para.rtg,
            NULL,
            0);

    // fire timers for contention bursts or my bursts
    // This timers brings the burst index for reference
    burst = dot16Ss->ulBurstHead;
    MacDot16UlBurst* temp1 = burst;
    while (burst != NULL)
    {
        if (burst->uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE
            && burst->isCdmaAllocationIEReceived == FALSE)
        {
            MacDot16UlBurst* temp = burst;
            if (temp1 != burst)
            {
                temp1->next = burst->next;
            }
            else
            {
                // you are deleting first packet
                if (temp == dot16Ss->ulBurstHead)
                {
                    dot16Ss->ulBurstHead = dot16Ss->ulBurstHead->next;
                    temp1 = dot16Ss->ulBurstHead;
                }
                if (temp == dot16Ss->ulBurstTail)
                {
                    dot16Ss->ulBurstTail = temp1;
                    temp1->next = NULL;
                }
            }
            burst = burst->next;
            MEM_free(temp);
        }
        else
        {
            // has some bursts assigned to me, set the timer
            MacDot16SetTimer(
                node,
                dot16,
                DOT16_TIMER_UlBurstStart,
                burst->startTime - node->getNodeTime(),
                NULL,
                burst->index);
            temp1 = burst;
            burst = burst->next;
        }
    }

    // increase statistics
    dot16Ss->stats.numUlMapRcvd ++;

    return MacDot16MacHeaderGetLEN(macHeader);
}

// /**
// FUNCTION   :: MacDot16SsHandleDcdPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DCD message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDcdPdu(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DcdMsg* dcd;
    int index;
    int length;
    unsigned char tlvType;
    unsigned char tlvLength;
    BOOL firstDCD = FALSE;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    dcd = (MacDot16DcdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DcdMsg);
    length = MacDot16MacHeaderGetLEN(macHeader);

    // increase statistics
    dot16Ss->stats.numDcdRcvd ++;

    if (dot16Ss->initInfo.dlSynchronized == FALSE)
    {
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd a DCD on channel %d before MAC sync",
                    dot16Ss->chScanIndex);
        }

        return MacDot16MacHeaderGetLEN(macHeader);
    }

    if (dot16Ss->timers.timerT1 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT1);
        dot16Ss->timers.timerT1 = NULL;
    }

    // reset the timer Lost DL-MAP
    dot16Ss->timers.timerT1 = MacDot16SetTimer(
                                  node,
                                  dot16,
                                  DOT16_TIMER_T1,
                                  dot16Ss->para.t1Interval,
                                  NULL,
                                  0);
    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Rcvd DCD, reset T1 Timer\n");
    }
    if (dot16Ss->initInfo.initStatus == DOT16_SS_INIT_ScanDlCh)
    {
        //first time rcvd a DCD
        firstDCD = TRUE;
        dot16Ss->servingBs->nextGenDcdCount = dcd->changeCount;
        dot16Ss->initInfo.initStatus = DOT16_SS_INIT_ObtainUlParam;

        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd first DCD  on channel %d after sync, obtain the "
                   "dl param and move the init status to obtain ul param\n",
                   dot16Ss->chScanIndex);
        }
    }
    else
    {
         if (dot16Ss->servingBs->dcdCount == dcd->changeCount)
         {
            // increase statistics
            if (DEBUG || DEBUG_NETWORK_ENTRY)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("rcvd a DCD with the same"
                       "changeCount on ch %d as local, do nothing\n",
                       dot16Ss->chScanIndex);
            }

            return MacDot16MacHeaderGetLEN(macHeader);

         }
         else
         {
            dot16Ss->servingBs->nextGenDcdCount = dcd->changeCount;
         }
    }


    // process TLVs
    // the following should be moved to specail PHY process UL burst
    // profile function
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        while ((index - startIndex) < length)
        {
            tlvType = macFrame[index ++];
            tlvLength = macFrame[index ++];

            switch (tlvType)
            {
                // common channel encoding
                case TLV_DCD_BsEirp:
                {
                    MacDot16TwoByteToShort(
                            dot16Ss->para.bs_EIRP,
                            macFrame[index],
                            macFrame[index + 1]);
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_RrsInitRngMax:
                {
                    MacDot16TwoByteToShort(
                            dot16Ss->para.rssIRmax,
                            macFrame[index],
                            macFrame[index + 1]);
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_PagingIntervalLength:
                {
                    dot16Ss->servingBs->pagingInterval = macFrame[index];
                    index += tlvLength;
                    break;
                }
                case TLV_DCD_PagingGroupId:
                {
                    MacDot16TwoByteToShort(dot16Ss->servingBs->pagingGroup,
                                          macFrame[index],
                                          macFrame[index + 1]);
                    if (dot16Ss->idleSubStatus != DOT16e_SS_IDLE_None &&
                        dot16Ss->currentPagingGroup !=
                            dot16Ss->servingBs->pagingGroup)
                    {
                        MacDot16eSsPerformLocationUpdate(node, dot16);
                    }
                    dot16Ss->currentPagingGroup =
                                        dot16Ss->servingBs->pagingGroup;
                    index += tlvLength;
                    break;
                }
                case TLV_DCD_Trigger:
                {
                    int triggerIndex;
                    unsigned char triggerTlvType;
                    unsigned char triggerTlvLength;

                    if (!dot16->dot16eEnabled)
                    {
                        index += tlvLength;

                        break;
                    }

                    triggerIndex = index;
                    while (triggerIndex < index + tlvLength)
                    {
                        triggerTlvType = macFrame[triggerIndex ++];
                        triggerTlvLength = macFrame[triggerIndex ++];
                        switch (triggerTlvType)
                        {
                            case TLV_DCD_TriggerTypeFuncAction:
                            {
                                dot16Ss->servingBs->trigger.triggerAction =
                                    (macFrame[triggerIndex] & 0x07);

                                dot16Ss->servingBs->trigger.triggerFunc =
                                    (macFrame[triggerIndex] & 0x38) >> 3;

                                dot16Ss->servingBs->trigger.triggerType =
                                    (macFrame[triggerIndex] & 0xc0) >> 6;

                                triggerIndex ++; // one byte in length

                                break;
                            }
                            case TLV_DCD_TriggerValue:
                            {
                                dot16Ss->servingBs->trigger.triggerValue =
                                    macFrame[triggerIndex ++];

                                break;
                            }
                            case TLV_DCD_TriggerAvgDuration:
                            {
                                dot16Ss->servingBs->trigger.
                                    triggerAvgDuration =
                                    macFrame[triggerIndex ++];

                                break;
                            }
                            default:
                            {
                                // TLVs not supported right now, skip it
                                triggerIndex += triggerTlvLength;

                                if (DEBUG_PACKET_DETAIL)
                                {
                                    printf("    DCD trigger Unknown "
                                           "TLV, type = %d, length = %d\n",
                                           triggerTlvType,
                                           triggerTlvLength);
                                }

                                break;
                            }

                        }

                    }

                    index += tlvLength;

                    break;
                }

                // frequency

                // mac version

                // OFDMA specific channel encoding
                case TLV_DCD_ChNumber:
                {
                    // not used right now
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_TTG:
                {
                    dot16Ss->para.ttg =
                        PhyDot16ConvertTTGRTGinclocktype
                                        (node,
                                         dot16->myMacData->phyNumber,
                                         macFrame[index ++]);

                    break;
                }
                case TLV_DCD_RTG:
                {
                    dot16Ss->para.rtg =
                        PhyDot16ConvertTTGRTGinclocktype
                                        (node,
                                         dot16->myMacData->phyNumber,
                                         macFrame[index ++]);

                    break;
                }
                case TLV_DCD_BsId:
                {
                    // not used right now
                    // the serving BS id is kept when handle dl map
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_DlBurstProfile:
                {
                    unsigned char diuc;
                    int burstIndex;
                    unsigned char burstTlvType;
                    unsigned char burstTlvLength;
                    MacDot16DlBurstProfile* burstProfile;

                    // get DIUC
                    diuc = macFrame[index];

                    // for DL burst profile, DIUC is equal to array index
                    burstProfile = (MacDot16DlBurstProfile*)
                                   &(dot16Ss->servingBs->
                                   nextGenDlBurstProf[diuc]);

                    // uplink burst profile
                    burstIndex = index + 1;
                    while (burstIndex < index + tlvLength)
                    {
                        burstTlvType = macFrame[burstIndex ++];
                        burstTlvLength = macFrame[burstIndex ++];
                        switch (burstTlvType)
                        {
                            case TLV_DCD_PROFILE_OfdmaFecCodeModuType:
                            {
                                burstProfile->ofdma.fecCodeModuType =
                                    macFrame[burstIndex ++];

                                break;
                            }
                            case TLV_DCD_PROFILE_OfdmaExitThreshold:
                            {
                                burstProfile->ofdma.exitThreshold =
                                    macFrame[burstIndex ++];

                                break;
                            }
                            case TLV_DCD_PROFILE_OfdmaEntryThreshold:
                            {
                                burstProfile->ofdma.entryThreshold =
                                    macFrame[burstIndex ++];

                                break;
                            }
                            default:
                            {
                                // TLVs not supported right now, skip it
                                burstIndex += burstTlvLength;

                                if (DEBUG_PACKET_DETAIL ||
                                    DEBUG_NETWORK_ENTRY)
                                {
                                    printf("    DL burst profile Unknown "
                                           "TLV, type = %d, length = %d\n",
                                           burstTlvType, burstTlvLength);
                                }

                                break;
                            }

                        }
                    }

                    index += tlvLength;

                    break;
                }
                default:
                {
                    // TLVs not supported right now, skip it
                    index += tlvLength;

                    if (DEBUG_PACKET_DETAIL || DEBUG_NETWORK_ENTRY)
                    {
                        printf("MAC802.16: Unknown TLV in DCD, type = %d, "
                               "length = %d\n",
                               tlvType, tlvLength);
                    }

                    break;
                }
            }
        }
    }

    if (firstDCD)
    {
        dot16Ss->servingBs->dcdCount = dcd->changeCount;
        memcpy((char*) &(dot16Ss->servingBs->dlBurstProfile[0]),
               (char*) &(dot16Ss->servingBs->nextGenDlBurstProf[0]),
               sizeof(MacDot16DlBurstProfile) *
               dot16Ss->numOfDLBurstProfileInUsed);
    }

    return MacDot16MacHeaderGetLEN(macHeader);
}

// /**
// FUNCTION   :: MacDot16SsUlChannelUseable
// LAYER      :: MAC
// PURPOSE    :: Determne whether SS can use this ulplink  channel
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + ucd       : MacDot16UcdMsg* : Pointer to the uplink channel descriptor
// RETURN     :: BOOL           : Usable or not
// **/
static
BOOL MacDot16SsUlChannelUseable(Node* node,
                              MacDataDot16* dot16,
                              MacDot16UcdMsg* ucd)
{

    // determine the usable or no by the modulation, coding scheme used
    // on the channel and SS's itself capabiltiy
    // currently, always return TRUE
    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Ul channel usable is OK!\n");
    }
    return TRUE;
}

// /**
// FUNCTION   :: MacDot16SsHandleUcdPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing UCD message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleUcdPdu(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16UcdMsg* ucd;
    int index;
    unsigned char tlvType;
    unsigned char tlvLength;
    int length;
    BOOL firstUcd = FALSE;

    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Rcvd a UCD from channel %d\n",
                dot16Ss->servingBs->dlChannel);
    }

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    length = MacDot16MacHeaderGetLEN(macHeader);

    ucd = (MacDot16UcdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16UcdMsg);

    // increase statistics
    dot16Ss->stats.numUcdRcvd ++;

    if (dot16Ss->initInfo.initStatus < DOT16_SS_INIT_ObtainUlParam)
    {
        //rcvd a UCD before downlink param is rcvd
        if (DEBUG_NETWORK_ENTRY)
        {
            printf("    state not ready to handle UCD\n");
        }

        return MacDot16MacHeaderGetLEN(macHeader);
    }
    else if (dot16Ss->initInfo.initStatus == DOT16_SS_INIT_ObtainUlParam &&
        dot16Ss->initInfo.ulParamAccquired == FALSE)
    {
        if (DEBUG_NETWORK_ENTRY)
        {
            printf("    after state ready to handle UCD\n");
        }

        // rcvd the first UCD afte SS ready
        if (MacDot16SsUlChannelUseable(node, dot16, ucd) == TRUE)
        {
            if (DEBUG_NETWORK_ENTRY)
            {
                printf("    channel is usable, UL papram acquired, "
                       "then reset T12, start T2 for broadcast ranging "
                       "and start Lost UL-Map timer\n");
            }

            dot16Ss->initInfo.ulParamAccquired = TRUE;
            firstUcd = TRUE;

            if (dot16->duplexMode == DOT16_TDD)
            {
                // if TDD, use the same channel
                dot16Ss->servingBs->ulChannel =
                            dot16Ss->servingBs->dlChannel;
            }
            else if (dot16->duplexMode == DOT16_FDD_HALF ||
                dot16->duplexMode == DOT16_FDD_FULL)
            {
                // FDD goes here
            }

            dot16Ss->servingBs->nextGenUcdCount = ucd->changeCount;
            dot16Ss->para.rangeBOStart = ucd->rangeBOStart;
            dot16Ss->para.rangeBOEnd = ucd->rangeBOEnd;
            dot16Ss->para.requestBOStart = ucd->requestBOStart;
            dot16Ss->para.requestBOEnd = ucd->requestBOEnd;

            // reset T12
            if (dot16Ss->timers.timerT12 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
                dot16Ss->timers.timerT12 = NULL;
            }
            dot16Ss->timers.timerT12 =  MacDot16SetTimer(
                                                node,
                                                dot16,
                                                DOT16_TIMER_T12,
                                                dot16Ss->para.t12Interval,
                                                NULL,
                                                0);

            // fot the first Initial Range, always contention-based
            if (dot16Ss->rngType == DOT16_CDMA)
            {
                dot16Ss->initInfo.rngState = DOT16_SS_RNG_WaitBcstInitRngOpp;
            }
            else
            {
                dot16Ss->initInfo.rngState = DOT16_SS_RNG_WaitBcstRngOpp;
            }
            dot16Ss->servingBs->rangeRetries = 0;

            if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
            {
                printf("    wait for contention rng opp for init rnging\n");
            }

            //start T2 for init ranging
            if (dot16Ss->timers.timerT2 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT2);
                dot16Ss->timers.timerT2 = NULL;
            }
            dot16Ss->timers.timerT2 = MacDot16SetTimer(
                                        node,
                                        dot16,
                                        DOT16_TIMER_T2,
                                        dot16Ss->para.t2Interval,
                                        NULL,
                                        0);

            //start Lost UL MAP interval
            if (dot16Ss->timers.timerLostUl != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostUl);
                dot16Ss->timers.timerLostUl = NULL;
            }
            dot16Ss->timers.timerLostUl = MacDot16SetTimer(
                                              node,
                                              dot16,
                                              DOT16_TIMER_LostUlSync,
                                              dot16Ss->para.lostUlInterval,
                                              NULL,
                                              0);
        }
        else
        {
            if (DEBUG_NETWORK_ENTRY)
            {
                printf("    channel is unusable! scan the next dl ch\n");
            }

            //restart the scan process for next downlink channel
            MacDot16StopListening(node,
                                  dot16,
                                  dot16Ss->servingBs->dlChannel);

            // switch to the next channel
            dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                   dot16->numChannels;

            dot16Ss->servingBs->dlChannel =
                dot16->channelList[dot16Ss->chScanIndex];

            //MacDot16SsPerformNetworkEntry(node, dot16);
            MacDot16SsRestartOperation(node, dot16);

            return MacDot16MacHeaderGetLEN(macHeader);

        }
    }
    else //SS has accquired ul param aldready
    {
        //reset T12
        if (dot16Ss->timers.timerT12 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
            dot16Ss->timers.timerT12 = NULL;
        }
        dot16Ss->timers.timerT12 =  MacDot16SetTimer(
                                            node,
                                            dot16,
                                            DOT16_TIMER_T12,
                                            dot16Ss->para.t12Interval,
                                            NULL,
                                            0);
        //when SS accquired the ul params
        if (ucd->changeCount == dot16Ss->servingBs->ucdCount)
        {
            if (DEBUG)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("rcvd UCD from channel %d,"
                       "reset T1, no dcd updated\n",
                       dot16Ss->chScanIndex);
            }

            // if no change in this UCD message, do nothing

            return MacDot16MacHeaderGetLEN(macHeader);
        }
        else
        {
            if (DEBUG)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("rcvd UCD from "
                       "channel %d, reset T1, ucd updated \n",
                       dot16Ss->chScanIndex);
            }
            dot16Ss->servingBs->nextGenUcdCount = ucd->changeCount;

            // has some change in the UCD message, update my record
            dot16Ss->para.rangeBOStart = ucd->rangeBOStart;
            dot16Ss->para.rangeBOEnd = ucd->rangeBOEnd;
            dot16Ss->para.requestBOStart = ucd->requestBOStart;
            dot16Ss->para.requestBOEnd = ucd->requestBOEnd;
        }

    }

    // next to decode channel TLVs
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        while ((index - startIndex) < length)
        {
            tlvType = macFrame[index ++];
            tlvLength = macFrame[index ++];

            switch (tlvType)
            {
                // common channel encoding
                case TLV_UCD_ContentionRsvTimout:
                {
                    dot16Ss->para.contRsvTimeout = macFrame[index ++];
                    break;
                }
                case TLV_UCD_BwReqOppSize:
                {
                    MacDot16TwoByteToShort(dot16Ss->para.requestOppSize,
                                           macFrame[index],
                                           macFrame[index + 1]);
                    index += tlvLength;
                    break;
                }
                case TLV_UCD_RngReqOppSize:
                {
                    MacDot16TwoByteToShort(dot16Ss->para.rangeOppSize,
                                           macFrame[index],
                                           macFrame[index + 1]);
                    index += tlvLength;
                    break;
                }
                case TLV_UCD_OfdmaPeriodicRngBackoffStart:
                {
                    index += tlvLength;
                    break;
                }
                case TLV_UCD_OfdmaPeriodicRngbackoffEnd:
                {
                    index += tlvLength;
                    break;
                }
                case TLV_UCD_SSTG:
                // canned for the purpose of implementation
                {
                    dot16Ss->para.sstgInPs = macFrame[index++];
                    break;
                }
                case TLV_UCD_UlBurstProfile:
                {
                    unsigned char uiuc;
                    int burstIndex;
                    unsigned char burstTlvType;
                    unsigned char burstTlvLength;
                    MacDot16UlBurstProfile* burstProfile;

                    // get the UIUC
                    uiuc = macFrame[index];

                    // (uiuc -1) is the array index
                    burstProfile = (MacDot16UlBurstProfile*)
                        &(dot16Ss->servingBs->nextGenUlBurstProf[uiuc - 1]);

                    // uplink burst profile
                    burstIndex = index + 1;
                    while (burstIndex < index + tlvLength)
                    {
                        burstTlvType = macFrame[burstIndex ++];
                        burstTlvLength = macFrame[burstIndex ++];

                        switch (burstTlvType)
                        {
                            case TLV_UCD_PROFILE_OfdmaFecCodeModuType:
                            {
                                burstProfile->ofdma.fecCodeModuType =
                                    macFrame[burstIndex ++];
                                break;
                            }
                            case TLV_UCD_PROFILE_OfdmaRngDataRatio:
                            {
                                burstProfile->ofdma.exitThreshold =
                                    macFrame[burstIndex ++];
                                break;
                            }
                            case TLV_UCD_PROFILE_OfdmaCnOverride:
                            {
                                burstProfile->ofdma.entryThreshold =
                                    macFrame[burstIndex];
                                burstIndex += 5;
                                break;
                            }
                            default:
                            {
                                // TLVs not supported right now, skip it
                                burstIndex += burstTlvLength;

                                if (DEBUG_PACKET_DETAIL ||
                                    DEBUG_NETWORK_ENTRY)
                                {
                                    printf("UL burst profile Unknown TLV, "
                                           "type = %d, length = %d\n",
                                           burstTlvType, burstTlvLength);
                                }
                                break;
                            }

                        }

                    }

                    index += tlvLength;
                    break;
                }
                default:
                {
                    // TLVs not supported right now, skip it
                    index += tlvLength;

                    if (DEBUG_PACKET_DETAIL || DEBUG_NETWORK_ENTRY)
                    {
                        printf("UCD Unknown TLV, type = %d, length = %d\n",
                               tlvType, tlvLength);
                    }
                    break;
                }
            }
        }
    }

    if (firstUcd)
    {
        dot16Ss->servingBs->ucdCount = ucd->changeCount;
        memcpy((char*) &(dot16Ss->servingBs->ulBurstProfile[0]),
               (char*) &(dot16Ss->servingBs->nextGenUlBurstProf[0]),
               sizeof(MacDot16UlBurstProfile) *
               dot16Ss->numOfULBurstProfileInUsed);
    }

    return MacDot16MacHeaderGetLEN(macHeader);
}

// /**
// FUNCTION   :: MacDot16SsAdjustLocalParam
// LAYER      :: MAC
// PURPOSE    :: Adjust/correct local params, includes power level,
//               timimg and freqency offset
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + timingAdjust : int         : in PHY specific unit
// + powLevelAdjust : char      : could be +/-, in uint of 0.25dB(m)?
// + freqOffsetAdjust : int     : in Hz
// RETURN     :: BOOL           : Correction successful or not
// **/
static
BOOL MacDot16SsAdjustLocalParam(Node* node,
                              MacDataDot16* dot16,
                              int timingAdjust,
                              char powLevelAdjust,
                              int freqOffsetAdjust)
{
    // adjust the power, refer to pg. 178 for detail
    if (powLevelAdjust != 0)
    {
        PhyData* thisPhy = node->phyData[dot16->myMacData->phyNumber];
        PhyDataDot16* phyDot16 = (PhyDataDot16 *)thisPhy->phyVar;
        D_Float32 temp;
        temp = ((float)((powLevelAdjust * DOT16_SS_DEFAULT_POWER_UNIT_LEVEL) + 3.0));
        phyDot16->txPower_dBm = phyDot16->txPower_dBm + temp;
        if ((phyDot16->txPower_dBm) > phyDot16->txMaxPower_dBm)
        {
            phyDot16->txPower_dBm = phyDot16->txMaxPower_dBm;
        }
    }
    return TRUE;
}


// /**
// FUNCTION   :: MacDot16SsBuildSbcRequest
// LAYER      :: MAC
// PURPOSE    :: Create a SBC-REQ msg to be sent at proper time
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Point to the message containing SBC-REQ PDU
// **/
static
Message* MacDot16SsBuildSbcRequest(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16SbcReqMsg* sbcRequest;
    Message* mgmtMsg;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];
    int index;

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a SBC-REQ msg\n");
    }

    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);
    index = 0;

    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    sbcRequest = (MacDot16SbcReqMsg*) &(payload[index]);
    index += sizeof(MacDot16SbcReqMsg);

    sbcRequest->type = DOT16_SBC_REQ;


    payload[index ++] = TLV_SBC_REQ_RSP_BwAllocSupport;  // type
    payload[index ++] = 1;  // length
    payload[index ++] = 2;

    payload[index ++] = TLV_SBC_REQ_RSP_TranstionGaps;  // type
    payload[index ++] = 2;  // length
    payload[index ++] = 10;
    payload[index ++] = 10;

    payload[index ++] = TLV_SBC_REQ_MaxTransmitPower;  // type
    payload[index ++] = 4;  // length
    payload[index ++] = 10;
    payload[index ++] = 10;
    payload[index ++] = 10;
    payload[index ++] = 10;

    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        payload[index ++] = TLV_SBC_REQ_RSP_PowerSaveClassTypesCapability;
        payload[index ++] = 1;  // length
        payload[index] = 0;
        for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
        {
            if (dot16Ss->psClassParameter[i].psClassStatus ==
                POWER_SAVING_CLASS_SUPPORTED)
            {
                payload[index] |= (0x01 << i);
            }
        }
        index++;
    }
    payload[index ++] = TLV_COMMON_CurrTransPow;  // type
    payload[index ++] = 1;  // length
    payload[index ++] = 10;

    // more PHY specific TLV value

    // set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16SsBuildPkmRequest
// LAYER      :: MAC
// PURPOSE    :: Build a PKM request message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + pkmCode   : MacDot16PKMCode : PKM code
// RETURN     :: Message* : Point to the message containing the PKM-REQ PDU
// **/
static
Message* MacDot16SsBuildPkmRequest(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16PKMCode pkmCode)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16PkmReqMsg* pkmRequest;
    Message* mgmtMsg;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];
    int index;

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a PKM-REQ msg\n");
    }

    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);
    index = 0;

    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->primaryCid);

    pkmRequest = (MacDot16PkmReqMsg*) &(payload[index]);
    index += sizeof(MacDot16PkmReqMsg);

    pkmRequest->type = DOT16_PKM_REQ;
    pkmRequest->code = (UInt8)pkmCode;
    dot16Ss->servingBs->authKeyInfo.lastMsgCode = pkmCode;
    dot16Ss->servingBs->authKeyInfo.pkmId ++;
    pkmRequest->pkmId = dot16Ss->servingBs->authKeyInfo.pkmId;

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16SsBuildRegRequest
// LAYER      :: MAC
// PURPOSE    :: Create a REG-REQ msg to be sent at proper time
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Point to the message containing REG-REQ PDU
// **/
static
Message* MacDot16SsBuildRegRequest(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RegReqMsg* regRequest;
    Message* mgmtMsg;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    int i;

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a REG-REQ msg\n");
    }

    index = 0;
    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);

    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->primaryCid);

    regRequest = (MacDot16RegReqMsg*) &(payload[index]);
    index += sizeof(MacDot16RegReqMsg);

    regRequest->type = DOT16_REG_REQ;

    if (dot16->mode == DOT16_PMP)
    {//PMP mode
        payload[index ++] = TLV_REG_REQ_RSP_NumUlCidSupport;  // type
        payload[index ++] = 2;  // length
        MacDot16ShortToTwoByte(payload[index], payload[index + 1],
            dot16Ss->servingBs->numCidSupport);
        index += 2;
        payload[index ++] = TLV_REG_REQ_RSP_MgmtSupport;  // type
        payload[index ++] = 1;  // length
        if (dot16Ss->para.managed == FALSE)
        {
            payload[index ++] = 0; // not support secondary connection
        }
        else
        {
            payload[index ++] = 1; // support secondary connection
        }

        payload[index ++] = TLV_REG_REQ_RSP_IpMgmtMode;  // type
        payload[index ++] = 1; // length
        payload[index ++] = 0; // unmanaged mode

        if (dot16->dot16eEnabled)
        {
            payload[index ++] = TLV_REG_REQ_RSP_MobilitySupportedParameters;
            payload[index ++] = 1; // length
            payload[index] = MOBILITY_SUPPORTED;
            if (dot16Ss->isSleepModeEnabled == TRUE)
            {
                payload[index] |= SLEEP_MODE_SUPPORTED;
            }
            // similary add for idle mode
            index++;
        }
    }


    // add more TLV if needed

    if (dot16Ss->para.authPolicySupport)
    {
        // Be sure to put HMAC tuple as the last TLV
        payload[index ++] = TLV_COMMON_HMacTuple;  // type
        payload[index ++] = 21; // length
        payload[index ++] = dot16Ss->servingBs->authKeyInfo.hmacKeySeq;
        for (i = 0; i < 20; i ++)
        {
            // message digest

            payload[index ++] = 0; // be sure to give the right value
        }
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16SsBuildRepRspMsg
// LAYER      :: MAC
// PURPOSE    :: Create a REP-RSP msg to be sent at proper time
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + reportType : unsigned char : report type
// RETURN     :: Message* : Point to the message containing REP-RSP PDU
// **/
static
Message* MacDot16SsBuildRepRspMsg(Node* node,
                                  MacDataDot16* dot16,
                                  unsigned char reportType)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RepRspMsg* repRsp;
    Message* mgmtMsg;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];
    int index;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build a REP-RSP msg\n");
    }

    index = 0;
    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);

    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    repRsp = (MacDot16RepRspMsg*) &(payload[index]);
    index += sizeof(MacDot16RepRspMsg);

    repRsp->type = DOT16_REP_RSP;

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // include other TLVs
    // currently CINR and RSSI are the only ones included
    if (reportType & 0x02)
    {
        payload[index ++] = TLV_REP_RSP_CinrReport;
        payload[index ++] = 2;

        if (dot16Ss->servingBs->cinrMean < -10)
        {
            payload[index ++] = 0;
        }
        else if (dot16Ss->servingBs->cinrMean > 53)
        {
            payload[index ++] = 53;
        }
        else
        {
            payload[index ++] =
                (char)(dot16Ss->servingBs->cinrMean - (-10));
        }
        payload[index ++] = 0;
    }

    if (reportType & 0x04)
    {
        payload[index ++] = TLV_REP_RSP_RssiReport;
        payload[index ++] = 2;

        if (dot16Ss->servingBs->rssMean < -123)
        {
            payload[index ++] = 0;
        }
        else if (dot16Ss->servingBs->rssMean > -40)
        {
            payload[index ++] = 0x53;
        }
        else
        {
            payload[index ++] =
                    (char)(dot16Ss->servingBs->rssMean - (-123));
        }
        payload[index ++] = 0;
    }

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

    return mgmtMsg;

}

// /**
// FUNCTION   :: MacDot16eSsBuildDregReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DREG-REQ mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Pointer to the MOB-SCN-REQ msg
// **/
static
Message* MacDot16eSsBuildDregReqMsg(
        Node* node,
        MacDataDot16* dot16,
        MacDot16eDregReqCode reqCode)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16eDregReqMsg* dregReqMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Build a DREG-REQ msg\n");
    }

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    dregReqMsg = (MacDot16eDregReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eDregReqMsg);

    dregReqMsg->type = (UInt8)DOT16_DREG_REQ;
    dregReqMsg->reqCode = (UInt8)reqCode;

    // if req code is MS Initiated Idle mode
    if (dregReqMsg->reqCode == DOT16e_DREGREQ_MSReq)
    {
        UInt8 idleModeRetainInfo;

        //set required idle mode retain info
        idleModeRetainInfo = DOT16e_RETAIN_Sbc | DOT16e_RETAIN_Reg |
                                DOT16e_RETAIN_Nw;

        macFrame[index ++] = TLV_DREG_PagingCycleReq;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
            dot16Ss->servingBs->pagingCycle);
        index += 2;
        macFrame[index ++] = TLV_DREG_IdleModeRetainInfo;
        macFrame[index ++] = 1;
        macFrame[index ++] = idleModeRetainInfo;

        macFrame[index ++] = TLV_DREG_MacHashSkipThshld;
        macFrame[index ++] = 2;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
            dot16->macHashSkipThshld);
        index += 2;

    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    return mgmtMsg;
}

// ------------------------------------------------------------------------
// Begin Build dot16 msg
//------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16eSsBuildMobScnReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the MOB-SCN-REQ mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + scanDuration : unsigned char* : Poitert to scan duration
//                                   if NULL, use default
//                                   0 when SS wish to cancel scan
// RETURN     :: Message* : Pointer to the MOB-SCN-REQ msg
// **/
static
Message* MacDot16eSsBuildMobScnReqMsg(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned char* scanDuration)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16eMobScnReqMsg* scnReq;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    MacDot16SsBsInfo* bsInfo;

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    scnReq = (MacDot16eMobScnReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobScnReqMsg);

    scnReq->type = DOT16e_MOB_SCN_REQ;
    if (scanDuration)
    {
        scnReq->duration = *scanDuration;
    }
    else
    {
        scnReq->duration = (UInt8)dot16Ss->para.nbrScanDuration;
    }
    scnReq->interval = (UInt8)dot16Ss->para.nbrScanInterval;
    scnReq->iteration = (UInt8)dot16Ss->para.nbrScanIteration;

    scnReq->numBsIndex = 0;
    scnReq->numBsId = 0;

    // Only full Bs Id is supported, Bs index will be supported in future
    bsInfo = dot16Ss->nbrBsList;
    while (bsInfo != NULL)
    {
        // copy the bs Id
        memcpy((unsigned char*) &macFrame[index],
                bsInfo->bsId,
                DOT16_BASE_STATION_ID_LENGTH);

        index += DOT16_BASE_STATION_ID_LENGTH;

        // scan type
        // use one byte rather than 3 bits to avoid
        // the bit operations and paddind
        // currently only scan with no association is supported.
        macFrame[index ++] = DOT16e_SCAN_SacnNoAssoc;
        scnReq->numBsId ++;
        bsInfo = bsInfo->next;
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16eSsBuildMobMshoReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the MOB-MSHO_REQ mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Pointer to the MOB_MSHO_REQ msg
// **/
static
Message* MacDot16eSsBuildMobMshoReqMsg(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16eMobMshoReqMsg* msHoReq;
    char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    MacDot16SsBsInfo* bsInfo;

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    msHoReq = (MacDot16eMobMshoReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobMshoReqMsg);

    msHoReq->type = DOT16e_MOB_MSHO_REQ;
    msHoReq->reportMetric = DOT16e_MSHO_ReportMetricRSSI;
    msHoReq->numNewBsId = 0;
    msHoReq->numNewBsIndex = 0;

    // Only full Bs Id is supported, Bs index will be supported in future

    bsInfo = dot16Ss->nbrBsList;
    while (bsInfo != NULL)
    {
        // copy the bs Id
        memcpy((unsigned char*) &macFrame[index],
                bsInfo->bsId,
                DOT16_BASE_STATION_ID_LENGTH);

        index += DOT16_BASE_STATION_ID_LENGTH;

        msHoReq->numNewBsId ++;

        // preamble (skip now)

        if (msHoReq->reportMetric & DOT16e_MSHO_ReportMetricCINR)
        {
            macFrame[index ++] = (char)bsInfo->cinrMean * 2;
        }

        if (msHoReq->reportMetric & DOT16e_MSHO_ReportMetricRSSI)
        {
            macFrame[index ++] = (char)((bsInfo->rssMean - DOT16e_RSSI_BASE)
                                 / DOT16_SS_DEFAULT_POWER_UNIT_LEVEL);
        }

        if (msHoReq->reportMetric & DOT16e_MSHO_ReportMetricRelativeDelay)
        {
            macFrame[index ++] = 0;
        }

        // service differentiation (use one byte instead 3bits)
        macFrame[index ++] = 3; // no predication available
        bsInfo = bsInfo->next;
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    return mgmtMsg;

}

// /**
// FUNCTION   :: MacDot16eSsBuildMobHoIndMsg
// LAYER      :: MAC
// PURPOSE    :: Build the MOB-HO-IND mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Pointer to the MOB-SCN-REQ msg
// **/
static
Message* MacDot16eSsBuildMobHoIndMsg(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16eMobHoIndMsg* hoInd;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    hoInd = (MacDot16eMobHoIndMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobHoIndMsg);

    hoInd->type = DOT16e_MOB_HO_IND;
    hoInd->mode = DOT16e_HOIND_Ho;  // 0b00: HO

    // if mode is 0b00, add the HO Mode
    if (hoInd->mode == DOT16e_HOIND_Ho)
    {
        MacDot16eMobHoIndHoMod* hoIndHoMod;

        hoIndHoMod = (MacDot16eMobHoIndHoMod*) &(macFrame[index]);
        index += sizeof(MacDot16eMobHoIndHoMod);

        hoIndHoMod->hoIndType = DOT16e_HOIND_ReleaseServBs;
        // 0b00: serving BS release

        ERROR_Assert(dot16Ss->targetBs != NULL,
                     "target BS is not specified for handover");

        // if HO indication mode is 0b00, add the target BS ID
        if (hoIndHoMod->hoIndType == DOT16e_HOIND_ReleaseServBs)
        {
            MacDot16CopyStationId(&(macFrame[index]),
                                    dot16Ss->targetBs->bsId);

            index += DOT16_BASE_STATION_ID_LENGTH;
        }
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    return mgmtMsg;
}

// /**
// FUNCTION   :: MacDot16eSsBuildMobScnRepMsg
// LAYER      :: MAC
// PURPOSE    :: Build the MOB-SCN-REP mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + reportMode : unsigned char : report mode
// + reportMetric : unsigned char : report metric
// RETURN     :: Message* : Pointer to the MOB-SCN-REQ msg
// **/
static
Message* MacDot16eSsBuildMobScnRepMsg(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned char reportMode,
                                      unsigned char reportMetric)
{

    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    MacDot16eMobScnRepMsg* scnRep;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    MacDot16SsBsInfo* bsInfo;
    int index;

    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->primaryCid);

    scnRep = (MacDot16eMobScnRepMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobScnRepMsg);

    // set teh vlaues
    scnRep->type = DOT16e_MOB_SCN_REP;

    // DOT16e_SCAN_NoReport will not come here
    if (reportMode == DOT16e_SCAN_EventTrigReport)
    {
        scnRep->reportMode = 0;
    }
    else if (reportMode == DOT16e_SCAN_PeriodicReport)
    {
        scnRep->reportMode = 1;
    }

    scnRep->reportMetric = reportMetric;

    scnRep->numBsIndex = 0;
    scnRep->numBsId = 0;

    // Only full Bs Id is supported, Bs index will be supported in future
    bsInfo = dot16Ss->nbrBsList;
    while (bsInfo != NULL &&
           bsInfo->lastMeaTime + DOT16_MEASUREMENT_VALID_TIME >
           node->getNodeTime())
    {
        // copy the bs Id
        memcpy((unsigned char*) &macFrame[index],
                bsInfo->bsId,
                DOT16_BASE_STATION_ID_LENGTH);

        index += DOT16_BASE_STATION_ID_LENGTH;

        if (reportMetric & DOT16e_MSHO_ReportMetricCINR)
        {
            macFrame[index ++] = (char) bsInfo->cinrMean * 2;
        }
        if (reportMetric & DOT16e_MSHO_ReportMetricRSSI)
        {
            macFrame[index ++] = (char)((bsInfo->rssMean - DOT16e_RSSI_BASE)
                                   / DOT16_SS_DEFAULT_POWER_UNIT_LEVEL);
        }
        if (reportMetric & DOT16e_MSHO_ReportMetricRelativeDelay)
        {
            macFrame[index ++] = 0;
        }
        scnRep->numBsId ++;
        bsInfo = bsInfo->next;
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // build the mgmt msg
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);

    return mgmtMsg;
}

static
void MacDot16eSsEnableDisableServiceFlowQueues(
    Node* node,
    MacDataDot16* dot16,
    int index,
    QueueBehavior queueBehavior)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    //BOOL returnVal = FALSE;
    Scheduler* scheduler;
    //MacDot16ServiceFlow* sFlow;

    if (index == -1)// for management packet
    {
        scheduler = dot16Ss->mgmtScheduler;
    }
    else
    {
        scheduler = dot16Ss->outScheduler[index];
    }
    scheduler->setQueueBehavior(ALL_PRIORITIES, queueBehavior);
    return;
}// end of MacDot16eSsEnableServiceFlowQueue

static
BOOL MacDot16eSsNeedToDeactivaePSClassType(Node* node, MacDataDot16* dot16,
                                  MacDot16ePSClassType classType)
{
    int i, j;
    BOOL retVal = FALSE;
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16ePSClasses* psClass = NULL;
    Scheduler* scheduler;
    int bytesNeedToSend = 0;
    int bwGranted = 0;
    for (i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        psClass = &dot16Ss->psClassParameter[i];
        if ((psClass->classType == classType) &&
            (psClass->isWaitForSlpRsp == FALSE)
            && (psClass->statusNeedToChange == FALSE))
        {
            for (j = 0; j < DOT16_NUM_SERVICE_TYPES; j++)
            {
                sFlow = dot16Ss->ulFlowList[j].flowHead;
                scheduler = dot16Ss->outScheduler[j];
                while (sFlow != NULL && sFlow->psClassType == classType)
                {
                    if (sFlow->activated && sFlow->admitted)
                    {
                        bytesNeedToSend += scheduler->bytesInQueue(
                            sFlow->queuePriority);
                        if (sFlow->isFragStart)
                        {
                            bytesNeedToSend -= sFlow->bytesSent;
                        }
                        bwGranted += sFlow->maxBwGranted;
                    }
                    sFlow = sFlow->next;
                }
            }// end of for DOT16_NUM_SERVICE_TYPES
            break;
        }// end of if psClass->classType == classType
    }// end of for DOT16e_MAX_POWER_SAVING_CLASS

    if (bytesNeedToSend > 0)
    {
        int bytesCanSend = bwGranted * psClass->listeningWindow;
        if (bytesNeedToSend > bytesCanSend)
        {
            retVal = TRUE;
            if (DEBUG_SLEEP)
            {
                printf("Present data is much more to tx in next Listen period");
                printf(" SS needs to deactivate PS class: %d\n", classType);
            }
        }
    }
    return retVal;
}// end of MacDot16eSsNeedToDeactivaePSClass

static
BOOL MacDot16eSsDeactivatePSClass(Node* node, MacDataDot16* dot16,
                                  MacDot16ePSClasses* psClass)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow = NULL;
    // need to deactivate the class
    // deactivate the class 1 & 2
    MacDot16eSetPSClassStatus(psClass, POWER_SAVING_CLASS_DEACTIVATE);
    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf(" SS Deactivate the class: %d\n", psClass->classType);
    }
    psClass->isSleepDuration = FALSE;
    psClass->isMobTrfIndReceived = FALSE;
    psClass->statusNeedToChange = FALSE;
    psClass->psClassIdleLastNoOfFrames = 0;
    for (int i =0; i < DOT16_NUM_SERVICE_TYPES; i++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        if (sFlow == NULL)
        {
            sFlow = dot16Ss->dlFlowList[i].flowHead;
        }
        if (sFlow && (psClass->classType ==
            sFlow->psClassType))
        {
            if (DEBUG_SLEEP)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("SS RESUME class: %d for "
                    "service type: %d\n",
                    psClass->classType,
                    sFlow->serviceType);
            }
            // disable/enable the serviceFlowQueues
            MacDot16eSsEnableDisableServiceFlowQueues(
                node,
                dot16,
                i,
                RESUME);
        }
    }// end of for
    return TRUE;
}// end of MacDot16eSsDeactivatePSClass

static
BOOL MacDot16eIsPSClassInfoNeedToAdd(
    MacDot16Ss* dot16Ss,
    MacDot16ePSClasses* psClass,
    MacDot16ePSClassStatus &newPSClassSatus)
{
     //MacDot16ePSClassStatus retVal = POWER_SAVING_CLASS_STATUS_NONE;
    newPSClassSatus = POWER_SAVING_CLASS_STATUS_NONE;
    BOOL retVal = FALSE;

    if ((psClass->numOfServiceFlowExists > 0) &&
        (psClass->isWaitForSlpRsp == FALSE))
    {
        if ((psClass->currentPSClassStatus == POWER_SAVING_CLASS_STATUS_NONE ||
            psClass->currentPSClassStatus == POWER_SAVING_CLASS_DEACTIVATE))
        {
            if ((psClass->classType == POWER_SAVING_CLASS_1 &&
                psClass->psClassIdleLastNoOfFrames >
                DOT16e_DEFAULT_PS1_IDLE_TIME) ||
                (psClass->classType == POWER_SAVING_CLASS_2 &&
                psClass->psClassIdleLastNoOfFrames >
                DOT16e_DEFAULT_PS2_IDLE_TIME))
            {
                // for these PS ss send activation request
                newPSClassSatus = POWER_SAVING_CLASS_ACTIVATE;
                retVal = TRUE;
            }
        }
        else if (psClass->currentPSClassStatus ==
            POWER_SAVING_CLASS_ACTIVATE && psClass->statusNeedToChange)
        {
            // for these PS ss send deactivation request
            newPSClassSatus = POWER_SAVING_CLASS_DEACTIVATE;
            retVal = TRUE;
        }
    }
    return retVal;
}// end of MacDot16eIsPSClassInfoNeedToAdd

static
int MacDot16ePSAddMobSlpReqParamtere(Node* node,
    MacDataDot16* dot16,
    UInt8* macFrame,
    MacDot16ePSClasses* psClass,
    MacDot16ePSClassStatus newPSClassStatus)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    BOOL operation = FALSE;
    BOOL defination = TRUE;
    int index = 0;

    //set Definition(1 bit), Operation(1 bit), Power_Saving_Class_ID(6 bit)
    if (defination)
    {
        macFrame[index] = 0x01; // defination of PS class is present
    }
    else
    {
        macFrame[index] = 0x00; // set 1st bit bit 0
        // set PS class ID
        macFrame[index] |= (psClass->psClassId < 2);
    }

    if (newPSClassStatus == POWER_SAVING_CLASS_ACTIVATE)
    {
        if (DEBUG_SLEEP)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Add PS class info %d Activate in MOB-SLP-REQ message\n",
                psClass->classType);
        }
        operation = TRUE;
        macFrame[index] |= 0x02;//Activation for PS class 1 & 2
    }
    else
    {
        if (DEBUG_SLEEP)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Add PS class info %d Deactivate in MOB-SLP-REQ message\n",
                psClass->classType);
        }
    }
    index++;

    if (operation == TRUE)
    {
        // Set 6 bit start frame no
        // 2 bits are for RFU
        macFrame[index] = (0x3F & (UInt8)(dot16Ss->servingBs->frameNumber +
            DOT16e_WAIT_FOR_SLP_RSP));
        psClass->startFrameNum = dot16Ss->servingBs->frameNumber
            + DOT16e_WAIT_FOR_SLP_RSP;
        index++;
    }
    if (defination == TRUE)
    {
        // Set power saving class type
        macFrame[index] = (UInt8)psClass->classType;
        // Set direction 00 mean Unspecified
        // Each CID has its own direction assign in its connection
        if (DOT16e_SS_PS1_TRAFFIC_TRIGGERED_FLAG &&
            psClass->classType == POWER_SAVING_CLASS_1)
        {
            // set 5th bit
            macFrame[index] |= 0x10;
        }
        // next 3bits are RFU
        index++;
        macFrame[index++] = psClass->initialSleepWindow;
        macFrame[index++] = psClass->listeningWindow;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
            psClass->finalSleepWindowBase);
        // Sleep window exponent in 3 bits
        macFrame[index + 1] &=
            ((psClass->finalSleepWindowExponent & 0x07) << 2);
        // no of cids in 3 bits
        //If Number_of_CIDs = 0, it means that all unicast CIDs
        // associated with the MS are requested for addition to the class
        index += 2;
    }
    return index;
}// end of MacDot16ePSAddMobSlpReqParamtere

static
BOOL McDot16eSsCheckIsPowerDownNeeded(Node* node,
                                      MacDataDot16* dot16,
                                      MacDot16ePSClasses* psClass3,
                                      int &noOfSleepFrames)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    // check need to go to power down
    // if class 1 & 2 is in power activate state
    // and SS does not have any mgmt pkt
    // then ss goes to power down for following period
    // powerDownDuration = min(clss3 sleep duration,
    //((class1 startframe + sleep duration) - current frame),
    //((class2 startframe + sleep duration) - current frame))
    MacDot16ePSClasses* psClass = NULL;
    int minPowerDownDuration = psClass3->initialSleepWindow;
    BOOL canGoesInToPowerDown = TRUE;
    int remSleepPeriod = 0;
    for (int i = 0; canGoesInToPowerDown &&
        (i < DOT16e_MAX_POWER_SAVING_CLASS); i++)
    {
        psClass = &dot16Ss->psClassParameter[i];
        if (psClass->classType != POWER_SAVING_CLASS_3)
        {
            if ((psClass->currentPSClassStatus
                == POWER_SAVING_CLASS_ACTIVATE)
                && psClass->isSleepDuration == TRUE)
            {
                remSleepPeriod = (psClass->startFrameNum +
                    psClass->sleepDuration - 1) - psClass3->startFrameNum;
                if (remSleepPeriod < minPowerDownDuration)
                {
                    minPowerDownDuration = remSleepPeriod;
                }
            }
            else
            {
                if (psClass->numOfServiceFlowExists > 0)
                {
                    canGoesInToPowerDown = FALSE;
                }
            }
        }
    }// end of for
    noOfSleepFrames = minPowerDownDuration;
    if (noOfSleepFrames <= 0)
    {
        canGoesInToPowerDown = FALSE;
    }
    return canGoesInToPowerDown;
}// end of McDot16eSsCheckIsPowerDownNeeded


// /**
// FUNCTION   :: MacDot16eSsBuildMobSlpReqMsgIfNeeded
// LAYER      :: MAC
// PURPOSE    :: Build the MOB-SLP-REQ mgmt msg
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: Message* : Pointer to the MOB-SLP-REQ msg
// **/
static
Message* MacDot16eSsBuildMobSlpReqMsgIfNeeded(Node* node, MacDataDot16* dot16)
{
//  Dot16e-2005, section 6.3.2.3.44 Sleep Request message (MOB_SLP-REQ)
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg = NULL;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index;
    UInt8 totalPSClassInfoNeedToSend = 0;
    MacDot16ePSClasses* psClass = NULL;
    int totalPSClassInfoindex;
    MacDot16ePSClassStatus newPSClassStatus;


    // build and send MobSlpReq msg
    memset(macFrame, 0, DOT16_MAX_MGMT_MSG_SIZE);
    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);

    macFrame[index++] = DOT16e_MOB_SLP_REQ;
    // maximum number of defined power saving class
    totalPSClassInfoindex = index;
    index++;


    for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        psClass = &dot16Ss->psClassParameter[i];

        if (MacDot16eIsPSClassInfoNeedToAdd(dot16Ss, psClass,
            newPSClassStatus))
        {
            // PS class status need to change
            totalPSClassInfoNeedToSend++;
            index += MacDot16ePSAddMobSlpReqParamtere(node,
                dot16,
                &(macFrame[index]),
                psClass,
                newPSClassStatus);
            psClass->isWaitForSlpRsp = TRUE;
        }
    }// end of for

    macFrame[totalPSClassInfoindex] = totalPSClassInfoNeedToSend;

    if (totalPSClassInfoNeedToSend > 0)
    {
        // Set the length field of the MAC header
        MacDot16MacHeaderSetLEN(macHeader, index);

        // build the mgmt msg
        mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(mgmtMsg != NULL, "MAC 802.16: Out of memory!");

        MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
        memcpy(MESSAGE_ReturnPacket(mgmtMsg), macFrame, index);
        if (DEBUG_SLEEP)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("build MOB-SLP-REQ message\n");
        }

        MacDot16SsEnqueueMgmtMsg(
            node,
            dot16,
            DOT16_CID_BASIC,
            mgmtMsg);
    }
    return mgmtMsg;
} // end of MacDot16eSsBuildMobSlpReqMsg

// /**
// FUNCTION   :: MacDot16eSsHandleMobTrfIndPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-TRF-IND message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobTrfIndPdu(Node* node,
                             MacDataDot16* dot16,
                             unsigned char* macFrame,
                             int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int index;
    UInt8 fmt;
    UInt16 slipId;
    UInt32 slipIdGrpIndication;
    int bitNo = 0;
    BOOL isPositiveTrfInd;

    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle MOB-TRF-IND message\n");
    }
    dot16Ss->stats.numMobTrfIndRcvd++;
    // skip the MAC header
    index = startIndex;
    if (dot16Ss->isSleepModeEnabled == FALSE)
    {
        MacDot16MacHeader* macHeader = (MacDot16MacHeader*)macFrame;
        index += MacDot16MacHeaderGetLEN(macHeader);
        return index - startIndex;
    }
    index += sizeof(MacDot16MacHeader);
    // skip message type
    index++;
    fmt = macFrame[index++];
    slipIdGrpIndication = MacDot16FourByteToInt((macFrame + index));
    index += 4;
    if ((fmt & 0x01) == 0x00)
    {
        // PS mode node check that packet is present for me or not
        for (int i = 0; (i < DOT16e_MAX_POWER_SAVING_CLASS)
            && (dot16Ss->psClassParameter[i].classType
            == POWER_SAVING_CLASS_1); i++)
        {
            MacDot16ePSClasses* psClass = &dot16Ss->psClassParameter[i];
            if (psClass->isSleepDuration == TRUE ||
                psClass->currentPSClassStatus ==
                POWER_SAVING_CLASS_DEACTIVATE)
            {
                continue;
            }
            psClass->isMobTrfIndReceived = TRUE;
            slipId = psClass->slipId;
            bitNo = slipId / 32;
            isPositiveTrfInd = (((slipIdGrpIndication >> bitNo) & 0x00000001)
                == 0x00000001);
            if (isPositiveTrfInd == TRUE)
            {
                UInt8 noOfOne = 0;
                UInt32 tempVal;
                int tempIndex;
                UInt8 bitNoInByte = (UInt8)(slipId % 32);

                for (int j = 0; j < bitNo; j++)
                {
                    // Count no of ones
                    if (((slipIdGrpIndication >> j) & 0x00000001)
                        == 0x00000001)
                    {
                        noOfOne++;
                    }
                }
                tempIndex = index + (noOfOne * 4);
                tempVal = MacDot16FourByteToInt(&macFrame[tempIndex]);
                isPositiveTrfInd = (((tempVal >>
                    bitNoInByte) & 0x01) == 0x01);

                if (isPositiveTrfInd == TRUE)
                {
                    // come in to awake state, BS has some packet
                    if (DEBUG_SLEEP)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("found positive traffic indication\n");
                    }
                    if (psClass->trafficTriggeredFlag)
                    {
                        // deactivate the PS class
                        MacDot16eSsDeactivatePSClass(node, dot16, psClass);
                    }
                    else
                    {
                        // sleep continue with initial sleep duration
                        psClass->sleepDuration = psClass->initialSleepWindow;
                        psClass->startFrameNum += psClass->listeningWindow;
                        if (DEBUG_SLEEP)
                        {
                            printf(" SS Set the next sleep duration %d is"
                                " initial sleep duration\n",
                                psClass->sleepDuration);
                        }
                    }
                    psClass->psClassIdleLastNoOfFrames = 0;
                    break;
                }
            }// end of if
        }// end of for
    }// end of if
    else
    {
        // when fmt == 1
    }
    return index - startIndex;
}// end of MacDot16eSsHandleMobTrfIndPdu

// /**
// FUNCTION   :: MacDot16eSsHandleMobSlpRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-SLP-RSP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobSlpRspPdu(Node* node,
                             MacDataDot16* dot16,
                             unsigned char* macFrame,
                             int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int index;
    int noOfSleepClass;
    MacDot16ePSClasses* psClass = NULL;

    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle MOB-SLP-RSP message\n");
    }
    dot16Ss->stats.numMobSlpRspRcvd++;
    if (dot16Ss->timers.timerT43 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT43);
        dot16Ss->timers.timerT43 = NULL;
    }

    // skip the MAC header
    index = startIndex;
    if (dot16Ss->isSleepModeEnabled == FALSE)
    {
        MacDot16MacHeader* macHeader = (MacDot16MacHeader*)macFrame;
        index += MacDot16MacHeaderGetLEN(macHeader);
        return index - startIndex;
    }
    index += sizeof(MacDot16MacHeader);

    // skip message type
    index++;
    noOfSleepClass = macFrame[index++];
    for (int i = 0; i < noOfSleepClass; i++)
    {
        UInt8 startFrameNo = 0;
        int lengthOfdata = macFrame[index++];
        BOOL sleepApproved = ((lengthOfdata & 0x80) == 0x80);
        lengthOfdata &= 0x7F;
        BOOL definition =  ((macFrame[index] & 0x01) == 0x01);// 1 bit
        BOOL operation = ((macFrame[index] & 0x02) == 0x02);// 1 bit
        //UInt8 psClassId = (macFrame[index] >> 2);// 6 bits

        index++;
        if (sleepApproved == TRUE)
        {
            if (operation == TRUE)
            {
                startFrameNo = (macFrame[index++] & 0x3F);
            }
            if (definition == TRUE)
            {
                for (int j = 0; j < DOT16e_MAX_POWER_SAVING_CLASS; j++)
                {
                    psClass = &dot16Ss->psClassParameter[j];
                    if (((macFrame[index] & 0x03) ==
                        psClass->classType))
                    {
                        UInt16 twoByteVal;
                        UInt8 noOfCIDs;
                        psClass->isWaitForSlpRsp = FALSE;
                        psClass->psDirection =
                            ((macFrame[index] >> 2) & 0x0C);
                        index++;
                        psClass->initialSleepWindow = macFrame[index++];
                        psClass->listeningWindow = macFrame[index++];
                        MacDot16TwoByteToShort(twoByteVal,
                            macFrame[index],
                            macFrame[index + 1]);
                        index += 2;
                        psClass->finalSleepWindowBase = (twoByteVal & 0x03FF);
                        psClass->finalSleepWindowExponent =
                            (UInt8)((twoByteVal >> 10) & 0x0007);
                        psClass->trafficIndReq =
                            ((twoByteVal & 0x2000) == 0x2000);
                        psClass->trafficTriggeredFlag =
                            (UInt8)((twoByteVal & 0x4000) == 0x4000);
                        if (psClass->trafficIndReq == TRUE)
                        {
                            MacDot16TwoByteToShort(twoByteVal,
                                macFrame[index],
                                macFrame[index + 1]);
                            index += 2;
                            psClass->slipId = twoByteVal & 0x03FF;
                            noOfCIDs = (UInt8)(twoByteVal >> 12);
                        }
                        else
                        {
                            noOfCIDs = macFrame[index++] & 0x0F;
                        }
                        if (noOfCIDs != 0)
                        {
                            index += (noOfCIDs * 2);
                        }
                        if (operation == TRUE)
                        {
                            UInt8 tempFrameNumInfo =
                                (UInt8)(dot16Ss->servingBs->frameNumber
                                & 0x0000003F);
                            if (tempFrameNumInfo <= startFrameNo)
                            {
                                psClass->startFrameNum =
                                    dot16Ss->servingBs->frameNumber +
                                    (startFrameNo - tempFrameNumInfo);
                            }
                            else
                            {
                                psClass->startFrameNum =
                                    dot16Ss->servingBs->frameNumber;
                            }

                            // activate class 1 & 2
                            MacDot16eSetPSClassStatus(psClass,
                                POWER_SAVING_CLASS_ACTIVATE);
                            if (psClass->classType == POWER_SAVING_CLASS_3)
                            {
                                if (DEBUG_SLEEP_PS_CLASS_3)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("handle MOB-SLP-RSP message for "
                                        "PS class 3\n");
                                }
                            }
                            psClass->statusNeedToChange = FALSE;
                            psClass->isSleepDuration = FALSE;
                            psClass->isMobTrfIndReceived = FALSE;
                            psClass->sleepDuration =
                                psClass->initialSleepWindow;
                        }
                        else
                        {
                            MacDot16eSsDeactivatePSClass(node,
                                dot16,
                                psClass);
                        }
                        break;
                    }
                }// end of for
            }
            else
            {
                if (operation == 0)
                {
                    // deactivate the class 1 & 2
                    MacDot16eSsDeactivatePSClass(node, dot16, psClass);
                }
            }
        }
        else
        {
            // not implemented yet
            index += lengthOfdata;
        }
    }// end of for

    if (dot16Ss->mobTrfIndPdu != NULL)
    {
        // get the type of the management type
        MacDot16eSsHandleMobTrfIndPdu(node,
            dot16,
            (unsigned char*)MESSAGE_ReturnPacket(dot16Ss->mobTrfIndPdu),
            0);

        MESSAGE_Free(node, dot16Ss->mobTrfIndPdu);
        dot16Ss->mobTrfIndPdu = NULL;
                }
    return index - startIndex;
}// end of MacDot16eSsHandleMobSlpRsp

// ------------------------------------------------------------------------
// End Build dot16e msg
//------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsHandleRngRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing RNG-RSP
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleRngRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RngRspMsg* rngRsp;
    Dot16CIDType cid;
    int length;
    int index;

    unsigned char tlvType;
    unsigned char tlvLength;
    BOOL isMyMsg = FALSE;
    unsigned char rngStatus = RANGE_CONTINUE;
    Dot16CIDType basicCid = 0;
    Dot16CIDType primaryCid = 0;

    int timingAdjust = 0;      // in PHY specific unit
    char powLevelAdjust = 0;   // could be +/-, in uint of 0.25dBm
    int freqOffsetAdjust = 0;  // in Hz
    BOOL correctionNeeded = FALSE;
    int dlFreqOverride = 0;    // in KHz, must used with rng Status abort

    unsigned char ulChIdOverride;
    unsigned char leastRobustDIUC;
    unsigned char dcdChangeCount;
    BOOL diucSpecified = FALSE;

    // The following two used when BS cannot decode the RNG-REQ
    unsigned char frameNum[3]; //
    unsigned char initRngOppNum; // (1 - 255)
    MacDot16RngCdmaInfo ofdmaRngCodeAttr;
    BOOL isCDMARngRSP = FALSE;
    BOOL localParamAdjustSucc = TRUE;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    rngRsp = (MacDot16RngRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RngRspMsg);

    length = MacDot16MacHeaderGetLEN(macHeader);
    cid = MacDot16MacHeaderGetCID(macHeader);
    if (dot16Ss->rngType != DOT16_CDMA && cid != DOT16_INITIAL_RANGING_CID)
    {
        isMyMsg = TRUE;
    }

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY ||
        DEBUG_RANGING)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling RNG-RSP message\n");
        printf("    Total packet length = %d\n", length);
        printf("    cid = %d\n", cid);
    }

    if (DEBUG_PACKET)
    {
        printf("Now, handling TLVs in the RNG-RSP message\n");
    }

    // get all TLVs in the RNG-RSP message
    while ((index - startIndex) < length)
    {
        tlvType = macFrame[index ++];

        switch (tlvType)
        {
            case TLV_RNG_RSP_TimingAdjust:
            {
                tlvLength = macFrame[index ++];  // 4 Byte
                memcpy((char*) &timingAdjust, &(macFrame[index]), 4);
                index += tlvLength;
                correctionNeeded= TRUE;
                break;
            }
            case TLV_RNG_RSP_PowerAdjust:
            {
                tlvLength = macFrame[index ++];
                powLevelAdjust = macFrame[index ++];
                correctionNeeded = TRUE;
                break;
            }
            case TLV_RNG_RSP_FreqAdjust:
            {
                tlvLength = macFrame[index ++];
                memcpy((char*) &freqOffsetAdjust, &(macFrame[index]), 4);
                correctionNeeded = TRUE;
                index += tlvLength;

                break;
            }
            case TLV_RNG_RSP_RngStatus:
            {
                tlvLength = macFrame[index ++];
                rngStatus = macFrame[index ++];

                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    RngStatus TLV, length = %d, status = %d,"
                           " index = %d\n", tlvLength, rngStatus, index);
                }

                break;
            }
            case TLV_RNG_RSP_DlFreqOver:
            {
                tlvLength = macFrame[index ++];
                memcpy((char*) &dlFreqOverride, &(macFrame[index]), 4);
                index += tlvLength;

                break;

            }
            case TLV_RNG_RSP_UlChIdOver:
            {
                tlvLength = macFrame[index ++];
                ulChIdOverride = macFrame[index ++];

                break;
            }
            case TLV_RNG_RSP_DlOpBurst:
            {
                tlvLength = macFrame[index ++];
                leastRobustDIUC = macFrame[index ++];
                dcdChangeCount = macFrame[index ++];
                diucSpecified = TRUE;

                break;
            }
            case TLV_RNG_RSP_MacAddr:
            {
                tlvLength = macFrame[index ++];

                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    MacAddress TLV, length = %d, MAC addr is "
                           "%d:%d:%d:%d:%d:%d, index = %d\n",
                           tlvLength, macFrame[index],
                           macFrame[index + 1], macFrame[index + 2],
                           macFrame[index + 3], macFrame[index + 4],
                           macFrame[index + 5], index);
                }

                if (MacDot16SameMacAddress(&(macFrame[index]),
                                           dot16->macAddress))
                {
                    isMyMsg = TRUE;
                }

                index += tlvLength;

                break;
            }

            case TLV_RNG_RSP_BasicCid:
            {
                tlvLength = macFrame[index ++];
                basicCid = ((Dot16CIDType)macFrame[index]) * 256 +
                           ((Dot16CIDType)macFrame[index + 1]);
                index += tlvLength;

                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    BasicCid TLV, length = %d, cid = %d,"
                           " index = %d\n", tlvLength, basicCid, index);
                }

                break;
            }
            case TLV_RNG_RSP_PrimaryCid:
            {
                tlvLength = macFrame[index ++];
                primaryCid = macFrame[index] * 256 + macFrame[index + 1];
                index += tlvLength;

                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    PrimaryCid TLV, length = %d, cid = %d,"
                           " index = %d\n", tlvLength, primaryCid, index);
                }

                break;
            }
            case TLV_RNG_RSP_AasBcastPerm:
            {
                tlvLength = macFrame[index ++];

                index += tlvLength;
                break;
            }
            case TLV_RNG_RSP_FrameNumber:
            {
                tlvLength = macFrame[index ++]; // must be 3 byte
                memcpy(frameNum, &(macFrame[index]), 3);
                index += tlvLength;

                break;
            }
            case TLV_RNG_RSP_InitRngOppNumber:
            {
                tlvLength = macFrame[index ++];
                initRngOppNum = macFrame[index ++];

                break;
            }
            case TLV_RNG_RSP_OfdmaRngCodeAttr:
            {
                tlvLength = macFrame[index ++]; //must be 4 byte
                memcpy(&ofdmaRngCodeAttr, (macFrame + index), 4);
                if (isInitialRangingCode(ofdmaRngCodeAttr.rangingCode)
                    || isPeriodicRangingCode(ofdmaRngCodeAttr.rangingCode))
                {
                    if ((ofdmaRngCodeAttr.rangingCode ==
                        dot16Ss->sendCDMAInfo.rangingCode)
                        && (ofdmaRngCodeAttr.ofdmaSubchannel ==
                        dot16Ss->sendCDMAInfo.ofdmaSubchannel)
                        && (ofdmaRngCodeAttr.ofdmaFrame ==
                        dot16Ss->sendCDMAInfo.ofdmaFrame)
                        &&((ofdmaRngCodeAttr.ofdmaSymbol ==
                        dot16Ss->sendCDMAInfo.ofdmaSymbol)
                        ||(ofdmaRngCodeAttr.ofdmaSymbol ==
                        dot16Ss->sendCDMAInfo.ofdmaSymbol + 1)))
                    {
                        isMyMsg = TRUE;
                        isCDMARngRSP = TRUE;
                        if (DEBUG_CDMA_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS handling CDMA RNG-RSP message\n");
                        }
                    }
                }

                index += tlvLength;
                break;
            }
            case TLV_RNG_RSP_PagingInfo:
            {
                tlvLength = macFrame[index ++];
                MacDot16TwoByteToShort(dot16Ss->servingBs->pagingCycle,
                                       macFrame[index], macFrame[index + 1]);
                dot16Ss->servingBs->pagingOffset =
                            (UInt8) macFrame[index + 2];
                MacDot16TwoByteToShort(dot16Ss->servingBs->pagingGroup,
                                       macFrame[index + 3],
                                       macFrame[index + 4]);
                index += tlvLength;
                break;
            }

            case TLV_RNG_RSP_LocUpdRsp:
            {
                tlvLength = macFrame[index ++];

                //assumed always successful
                index += tlvLength;
                break;
            }
            default:
            {
                // TLVs not supported right now
                tlvLength = macFrame[index ++];
                index += tlvLength;
                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    Unknown TLV, type = %d, length = %d\n",
                           tlvType, tlvLength);
                }

                break;
            }
        }
    }

    if (isMyMsg)
    {
        // increase the statistics
        dot16Ss->stats.numRngRspRcvd ++;

        // if the rng status is abort
        if (rngStatus == RANGE_ABORT)
        {//abort

            // This implements the abort case (both pg. 179 and pg. 204)
           if (dlFreqOverride != 0)
            {

               // redo initial ranging
                if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("    rcvd a abort status with freq override\n");
                }
            }
            else
            {//move to next dl channel and restart init

                // Switch to another channel and search
                // stop listening to the previous channel and restart dl
                // search
                if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("rcvd a abort RNG-RSP, move to next dl ch %d\n",
                           (dot16Ss->chScanIndex + 1) % dot16->numChannels);
                }
                MacDot16StopListening(node,
                                      dot16,
                                      dot16Ss->servingBs->dlChannel);

                // switch to the next channel
                dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                       dot16->numChannels;

                dot16Ss->servingBs->dlChannel =
                    dot16->channelList[dot16Ss->chScanIndex];
                //MacDot16SsPerformNetworkEntry(node, dot16);
                MacDot16SsRestartOperation(node, dot16);
            }

        }
        else
        {//continue or success

            // This implement pg179 and pg 204, continune and success case

            if (dot16Ss->operational == TRUE)
            {// This RNG-RSP is for periodic ranging
                // only continue status
                dot16Ss->periodicRngInfo.lastStatus =
                            (MacDot16RangeStatus)rngStatus;
                // Clear timer T3
                if ((dot16Ss->timers.timerT3 != NULL)
                    && dot16Ss->rngType == DOT16_CDMA
                    && isPeriodicRangingCode(
                        dot16Ss->sendCDMAInfo.rangingCode))
                {
                    MESSAGE_CancelSelfMsg(node,
                        dot16Ss->timers.timerT3);
                    dot16Ss->timers.timerT3 = NULL;
                }
                if (rngStatus == RANGE_CONTINUE)
                {
                    // reset the bool to make sure it can get the range opp
                    // either as unicast range opp or as data grant
                    dot16Ss->periodicRngInfo.rngReqScheduled = FALSE;
                    dot16Ss->periodicRngInfo.toSendRngReq = FALSE;
                    if (dot16Ss->rngType == DOT16_CDMA
                        && isPeriodicRangingCode(
                        dot16Ss->sendCDMAInfo.rangingCode))
                    {
                        dot16Ss->periodicRngInfo.toSendRngReq = TRUE;
                    }
                }
                if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("    rcvd a cont/succ status for"
                           " periodic rng\n");
                }

            }

            //adjustment/correction
            if (correctionNeeded)
            {
                localParamAdjustSucc = MacDot16SsAdjustLocalParam(
                                           node,
                                           dot16,
                                           timingAdjust,
                                           powLevelAdjust,
                                           freqOffsetAdjust);
            }

            //when adjust is made, status is always continue
            if (localParamAdjustSucc == FALSE && correctionNeeded == TRUE)
            {// correction falied

                if (dot16Ss->operational == FALSE)
                {// initial ranging case

                    // change the anomalies state variable
                    dot16Ss->initInfo.adjustAnmolies = TRUE;
                }
                else
                {// periodic raning case

                    //set error as lastAnomaly
                    dot16Ss->periodicRngInfo.lastAnomaly = ANOMALY_ERROR;
                }
            }
            else
            {// correction success
                if (dot16Ss->operational == FALSE)
                {// initial ranging case

                    // change the initanomalies state variable
                    dot16Ss->initInfo.adjustAnmolies = FALSE;
                }
                else
                {// periodic raning case

                    //set error as lastAnomaly
                    dot16Ss->periodicRngInfo.lastAnomaly = ANOMALY_NO_ERROR;
                }
                if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("    no corr is needed/corr suc\n");
                }
            }

            if (cid == DOT16_INITIAL_RANGING_CID && !isCDMARngRSP)
            {//must be initial ranging and rcvd the RNG-RSP

                // basicCid could be changed due to various reason,
                // update to latest one?
                dot16Ss->servingBs->basicCid = basicCid;
                dot16Ss->servingBs->primaryCid = primaryCid;

                if (DEBUG_RANGING)
                {
                    printf("    Node %d has performed initial ranging. "
                           "My basicCid = %d, primaryCid = %d, DL = %d\n",
                           node->nodeId,
                           dot16Ss->servingBs->basicCid,
                           dot16Ss->servingBs->primaryCid,
                           dot16Ss->servingBs->dlChannel);
                }
            }

            if (dot16Ss->operational == FALSE &&
                dot16Ss->initInfo.initRangingCompleted == FALSE)
            {//RNG-RSP for initial ranging, the cid could be initial
             // ranging cid or basic cid

                if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("    rcvd a RNG-RSP for init rng, cancel T3\n");
                }

                // cancel the ranging timeout timer (T3)
                if (dot16Ss->timers.timerT3 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
                    dot16Ss->timers.timerT3 = NULL;
                }

                if (rngStatus == RANGE_SUCC)
                {//status successful

                    // print the modulation and coding scheme
                    if (DEBUG_RANGING ||
                        node->nodeId == DEBUG_SPECIFIC_NODE)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        MacDot16PrintModulationCodingInfo(node, dot16);
                    }

                    dot16Ss->servingBs->rangeRetries = 0;
                    dot16Ss->servingBs->rangeBOCount =
                                dot16Ss->para.rangeBOStart;
                    if (dot16Ss->performNetworkReentry == TRUE)
                    {
                        if (DEBUG)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("Switching out of Idle mode\n");
                        }

                        dot16Ss->idleSubStatus = DOT16e_SS_IDLE_None;
                        dot16Ss->performNetworkReentry = FALSE;
                    }
                    if (dot16Ss->initInfo.rngState ==
                        DOT16_SS_RNG_WaitRngRspForInitOpp)
                    {
                        if (dot16Ss->isCdmaAllocationIERcvd)
                        {
                            dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_BcstRngOppAlloc;
                        }
                        else
                        {
                            dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitBcstRngOpp;
                                // tx the initial RNG-REQ
                                MacDot16SsSendRangeRequest(
                                    node,
                                    dot16,
                                    DOT16_INITIAL_RANGING_CID,
                                    NULL,
                                    FALSE);
                                dot16Ss->initInfo.rngState =
                                    DOT16_SS_RNG_WaitRngRsp;
                        }
                        if (DEBUG_CDMA_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS receive Range response PDU"
                                " with status RANGE_SUCC\n");
                        }
                    }
                    else if (dot16Ss->initInfo.initStatus ==
                       DOT16_SS_INIT_RangingAutoAdjust)
                    {
                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("    rcvd a success status for init rng,"
                                   "start basic negotiation, start T4 for "
                                   "periodic rng\n");
                        }

                        Message* sbcReqMgmtMsg;

                        // Move the init state to next one, ie.,
                        // BegotiateBasicCapability
                        dot16Ss->initInfo.initRangingCompleted = TRUE;
                        dot16Ss->initInfo.initStatus =
                                DOT16_SS_INIT_NegotiateBasicCapability;

                        // start T4 for periodic ranging
                        if (dot16Ss->timers.timerT4 != NULL)
                        {
                            MESSAGE_CancelSelfMsg(
                                node,
                                dot16Ss->timers.timerT4);

                            dot16Ss->timers.timerT4 = NULL;

                        }
                        dot16Ss->timers.timerT4 =
                            MacDot16SetTimer(node,
                                             dot16,
                                             DOT16_TIMER_T4,
                                             dot16Ss->para.t4Interval,
                                             NULL,
                                             0);

                        dot16Ss->periodicRngInfo.lastAnomaly =
                                ANOMALY_NO_ERROR;

                        //  scedule SBC-REQ
                        dot16Ss->servingBs->sbcRequestRetries = 0;

                        //build a SBC REQ  msg
                        sbcReqMgmtMsg = MacDot16SsBuildSbcRequest(
                                            node,
                                            dot16);
                        //enqueue the msg
                        MacDot16SsEnqueueMgmtMsg(
                            node,
                            dot16,
                            DOT16_CID_BASIC,
                            sbcReqMgmtMsg);
                    }
                    if (dot16Ss->performLocationUpdate == TRUE)
                    {
                        dot16Ss->performLocationUpdate = FALSE;
                        if (dot16Ss->timers.timerMSPaging != NULL)
                        {
                            MESSAGE_CancelSelfMsg(node,
                                dot16Ss->timers.timerMSPaging);
                            dot16Ss->timers.timerMSPaging = NULL;
                        }
                        dot16Ss->timers.timerMSPaging =
                            MacDot16SetTimer(node,
                                         dot16,
                                         DOT16e_TIMER_MSPaging,
                                         0,
                                         NULL,
                                         0);
                    }
                }
                else
                {// stauts continue

                    // here wait for invited ranging opp, since already
                    // got RSP

                    if (dot16Ss->initInfo.rngState ==
                        DOT16_SS_RNG_WaitRngRspForInitOpp)
                    {
                    // Wait for UL MAP
                        if (rngStatus == RANGE_CONTINUE)
                        {
                            dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitBcstInitRngOpp;
                            //MacDot16SsPerformNetworkEntry(node, dot16);
                            if (DEBUG_CDMA_RANGING)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS receive Range response PDU"
                                    " with status RANGE_CONTINUE\n");
                            }
                        }
                    }
                    else
                    {

                        dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitInvitedRngOpp;

                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("    rcvd a continue status for init rng,"
                                   "need to wait initial rng poll"
                                   "to complete init rng\n");
                        }
                    }
                }
            }// operational
            // for periodic ranging, do nothing more here
        }//rngStatus
    }//isMyMsg

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsPerformAuthKeyExchange
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing SBC-REQ
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: NULL
static
void MacDot16SsPerformAuthKeyExchange(Node* node,
                                      MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    Message* pkmReqMgntMsg;

    //scheduel the PKM-REQ.
    pkmReqMgntMsg = MacDot16SsBuildPkmRequest(
                        node,
                        dot16,
                        PKM_CODE_AuthRequest);
    MacDot16SsEnqueueMgmtMsg(
        node,
        dot16,
        DOT16_CID_PRIMARY,
        pkmReqMgntMsg);

    // increase stats
    dot16Ss->stats.numPkmReqSent ++;

    if (DEBUG_NETWORK_ENTRY || DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("schedule PKM-REQ try\n");
    }
}

// /**
// FUNCTION   :: MacDot16SsBasicCapbilityIsOK
// LAYER      :: MAC
// PURPOSE    :: Determine the SBC negotiation is OK or not
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: BOOL           : TRUE if OK, FALSE, if not OK
static
BOOL MacDot16SsBasicCapbilityIsOK(Node* node,
                                  MacDataDot16* dot16)
{

     return TRUE;
}

// /**
// FUNCTION   :: MacDot16SsPerformRegistration
// LAYER      :: MAC
// PURPOSE    :: Perform registration with the BS
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: void : NULL
static
void MacDot16SsPerformRegistration(Node* node,
                                   MacDataDot16* dot16)
{
    Message* regMgntMsg;

    //scheduel the REG-REQ.
    regMgntMsg = MacDot16SsBuildRegRequest(
                        node,
                        dot16);
    MacDot16SsEnqueueMgmtMsg(
        node,
        dot16,
        DOT16_CID_PRIMARY,
        regMgntMsg);

    if (DEBUG_NETWORK_ENTRY || DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("schedule REG-REQ\n");
    }
}

// /**
// FUNCTION   :: MacDot16SsHandleSbcRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing SBC-REQ
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleSbcRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16SbcRspMsg* sbcRsp;
    int index;
    int length;

    unsigned char tlvType;
    unsigned char tlvLength;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    sbcRsp = (MacDot16SbcRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16SbcRspMsg);

    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_NETWORK_ENTRY || DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling SBC-RSP message with length %d\n", length);
    }

    // increase the statistics
    dot16Ss->stats.numSbcRspRcvd ++;

    // cancel t18 if running
    if (dot16Ss->timers.timerT18 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT18);
        dot16Ss->timers.timerT18 = NULL;
    }

    // get all TLVs in the SBC-RSP message
    while ((index - startIndex) < length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];
        index += tlvLength;
    }

    // determine the rcvd SBC-RSP is OK?
    if (MacDot16SsBasicCapbilityIsOK(node, dot16))
    {

        dot16Ss->initInfo.basicCapabilityNegotiated = TRUE;
        if (dot16Ss->para.authPolicySupport)
        {
            dot16Ss->initInfo.initStatus =
                    DOT16_SS_INIT_AuthorizationKeyExchange;
            MacDot16SsPerformAuthKeyExchange(node, dot16);
        }
        else
        {
            // Move the init state to next step registration
            dot16Ss->initInfo.initStatus = DOT16_SS_INIT_RegisterWithBs;
            dot16Ss->servingBs->regRetries = 0;
            MacDot16SsPerformRegistration(node, dot16);
        }
    }
    else
    {
        if (dot16Ss->servingBs->sbcRequestRetries <= DOT16_MAX_SBC_RETRIES)
        {
            Message* sbcReqMgntMsg;
            sbcReqMgntMsg = MacDot16SsBuildSbcRequest(
                                node,
                                dot16);
            MacDot16SsEnqueueMgmtMsg(
                node,
                dot16,
                DOT16_CID_BASIC,
                sbcReqMgntMsg);

            if (DEBUG_NETWORK_ENTRY || DEBUG)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    sbc rsp not OK retry for the %dth time,"
                       "start a new SBC-REQ try\n",
                       dot16Ss->servingBs->sbcRequestRetries);
            }
        }
        else
        {
            // stop listening to the channel
            MacDot16StopListening(node,
                          dot16,
                          dot16Ss->servingBs->dlChannel);

            // re-initilize MAC
            MacDot16SsPerformNetworkEntry(node, dot16);

            if (DEBUG_NETWORK_ENTRY || DEBUG)
            {
                printf("    T18 timeout for the %dth time,"
                       "restart MAC init\n",
                       dot16Ss->servingBs->sbcRequestRetries);
            }
        }
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandlePkmRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing PKM-RSP
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandlePkmRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16PkmRspMsg* pkmRsp;
    int index;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    pkmRsp = (MacDot16PkmRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16PkmRspMsg);

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling PKM-RSP message\n");
    }

    // increase stats
    dot16Ss->stats.numPkmRspRcvd ++;

    if (pkmRsp->pkmId == dot16Ss->servingBs->authKeyInfo.pkmId)
    {
        if (pkmRsp->code ==  PKM_CODE_AuthReply &&
            dot16Ss->servingBs->authKeyInfo.lastMsgCode ==
            PKM_CODE_AuthRequest)
        {
            // handle TLVs

            // process authentication

            if (dot16Ss->operational == FALSE)
            {
                // Move the init state to next step registration
                dot16Ss->initInfo.initStatus = DOT16_SS_INIT_RegisterWithBs;

                dot16Ss->initInfo.authorized = TRUE;
                dot16Ss->servingBs->regRetries = 0;

                MacDot16SsPerformRegistration(node, dot16);

                if (DEBUG_NETWORK_ENTRY)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("    Finish authenticaiton and key exchange,"
                           "start registration with the network\n");
                }

            }
        }
        else if (pkmRsp->code ==  PKM_CODE_AuthReject &&
                 dot16Ss->servingBs->authKeyInfo.lastMsgCode ==
                 PKM_CODE_AuthRequest)
        {
            // handle TLVs

            // process authentication
        }
        else if (pkmRsp->code ==  PKM_CODE_KeyReply &&
                 dot16Ss->servingBs->authKeyInfo.lastMsgCode ==
                 PKM_CODE_KeyRequest)
        {
            // handle TLVs
            // process authentication
        }
        else if (pkmRsp->code ==  PKM_CODE_KeyReject &&
                 dot16Ss->servingBs->authKeyInfo.lastMsgCode ==
                 PKM_CODE_KeyRequest)
        {
            // handle TLVs
            // process authentication
        }
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleRegRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing REG-RSP
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleRegRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RegRspMsg* regRsp;
    int index;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    regRsp = (MacDot16RegRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RegRspMsg);

    if (DEBUG_PACKET || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling REG-RSP message\n");
    }

    //increase stats
    dot16Ss->stats.numRegRspRcvd ++;

    // cancel T6 if running
    if (dot16Ss->timers.timerT6 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT6);
        dot16Ss->timers.timerT6 = NULL;
    }

    // is the response OK
    if (regRsp->response == DOT16_REG_RSP_OK)
    {
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("    REG RSP is  OK, cancel T6\n");
        }
        if (dot16Ss->para.managed == TRUE)
        {
            // Establish secondary connecction

            // establish IP connectivity
        }
        else
        {
            // Now initialization is over and ready to
            // set up provisioned connections

            // move the init states
            dot16Ss->initInfo.registered = TRUE;

            // set others since nonmanaged SS is done with initialization
            dot16Ss->initInfo.ipConnectCompleted = TRUE;
            dot16Ss->initInfo.paramTransferCompleted = TRUE;
            dot16Ss->initInfo.timeOfDayEstablished = TRUE;
            dot16Ss->initInfo.initStatus =
                    DOT16_SS_INIT_EstProvisionedConnections;

            //... wait for provisioned connection estatblsih from BS
            dot16Ss->initInfo.initStatus =  DOT16_SS_INIT_Oprational;

            // ... wait for DSA-REQ from BS to establish provisioned
            // connections
            dot16Ss->operational = TRUE;

            if (dot16->dot16eEnabled &&
                dot16Ss->hoStatus != DOT16e_SS_HO_None)
            {
                dot16Ss->hoStatus = DOT16e_SS_HO_None;
                if (DEBUG_HO)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("---finish actual handover over---\n");
                }
            }

            // check if network layer has packet to send
            MacDot16NetworkLayerHasPacketToSend(node, dot16);
            if (DEBUG_NETWORK_ENTRY)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    *****unmanaged SS finish network entry*****\n");
            }
        }
    }
    else
    {
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("    REG RSP is failed, cncel T6\n");
        }
        if (dot16Ss->servingBs->regRetries > DOT16_MAX_REGISTRATION_RETRIES)
        {
            if (DEBUG_NETWORK_ENTRY)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    After max %d try of registration,"
                       "SS restart MAC intialization\n",
                       dot16Ss->servingBs->regRetries);

            }
            // Make sure whether: Switch to another channel and search?
            // stop listening to the previous channel
            MacDot16StopListening(node,
                                  dot16,
                                  dot16Ss->servingBs->dlChannel);

            // switch to the next channel
            dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                   dot16->numChannels;

            dot16Ss->servingBs->dlChannel =
                dot16->channelList[dot16Ss->chScanIndex];
            //MacDot16SsPerformNetworkEntry(node, dot16);
            MacDot16SsRestartOperation(node, dot16);

        }
        else
        {
            if (DEBUG_NETWORK_ENTRY)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    It is %d th try of registration,"
                       "retry registration\n",
                       dot16Ss->servingBs->regRetries);
            }
            MacDot16SsPerformRegistration(node, dot16);
        }
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleRepReqPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing REP-REQ
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleRepReqPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RepReqMsg* repReq;
    int index;
    unsigned char tlvType;
    int tlvLength;
    unsigned char reportType = 0;
    int length;
    Message* repRspMsg;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    repReq = (MacDot16RepReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RepReqMsg);
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling REP-REQ message\n");
    }

    //increase stats
    dot16Ss->stats.numRepReqRcvd ++;

    // get all TLVs in the REP-REQ message
    while ((index - startIndex) < length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];
        switch (tlvType)
        {
            case TLV_REP_REQ_ReportType:
            {
                reportType = macFrame[index];

                break;
            }

            default:
            {
                // TLVs not supported right now
                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    Unknown TLV, type = %d, length = %d\n",
                           tlvType, tlvLength);
                }

                break;
            }
        }
        index += tlvLength;
    }

    // build the REP-RSP
    if (dot16Ss->servingBs->lastMeaTime
        + DOT16_MEASUREMENT_VALID_TIME
        > node->getNodeTime())
    {
        repRspMsg = MacDot16SsBuildRepRspMsg(node, dot16, reportType);
        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_BASIC,
                                 repRspMsg);

        // update stats
        dot16Ss->stats.numRepRspSent ++;

        dot16Ss->servingBs->lastReportTime = node->getNodeTime();

        //reset the emergency report count
        dot16Ss->servingBs->numEmergecyMeasRep = 0;
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleDsaReqPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSX-RVD message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsaReqPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex,
                              TraceProtocolType appType)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaReqMsg* dsaReq;

    MacDot16ServiceType serviceType = DOT16_SERVICE_BE;
    MacDot16CsClassifier pktInfo;
    MacDot16QoSParameter qosInfo;
    unsigned int transId = 0;

    MacDot16ServiceFlowDirection sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
    MacDot16ServiceFlowInitial sfInitial;
    unsigned char confirmCode;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];

    unsigned int sfId = 0; // service flow id
    Dot16CIDType transportCid = 0; // transport Cid
    MacDot16ServiceFlow* sFlow = NULL;
    Message* mgmtMsg = NULL;

    int index = 0;
    int length = 0;

    unsigned char tlvType;
    int tlvLength;

    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    int csSpecIndex;
    unsigned char csSpecTlvType;
    unsigned char csSpecTlvLen;

    int csRuleIndex;
    unsigned char csRuleTlvType;
    unsigned char csRuleTlvLen;
    BOOL isPackingEnabled = FALSE;
    BOOL isFixedLengthSDU = FALSE;
    BOOL TrafficIndicationPreference = FALSE;
    unsigned int fixedLengthSDUSize = DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE;
    BOOL isARQEnabled = FALSE ;
    MacDot16ARQParam arqParam ;
    unsigned int arqPar = 0;
    memset(&arqParam,0,sizeof(MacDot16ARQParam));

    // increase statistics
    dot16Ss->stats.numDsaReqRcvd ++;
    dot16Ss->lastCSPacketRcvd = node->getNodeTime();
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSA-REQ message
    dsaReq = (MacDot16DsaReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaReqMsg);

    MacDot16TwoByteToShort(transId, dsaReq->transId[0], dsaReq->transId[1]);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
         printf("rcvd DSA-REQ for transId %d,"
                "build and schedule DSA RSP\n",
                transId);
    }
/* bug fix start 5377*/
    if (dot16Ss->operational != TRUE)
    {
        if (DEBUG_FLOW)
        {
            char time[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), time);
            printf("At time %s node %d rcvd DSA-REQ for "
                   "transId %d,in non-operational state."
                   "Hence ignoring the msg\n",
                   time, node->nodeId,transId);
        }
        return length;
    }
/* bug fix start 5377*/


    length = MacDot16MacHeaderGetLEN(macHeader);
    memset((char*) &qosInfo, 0, sizeof(MacDot16QoSParameter));

    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow != NULL)
    {
        if (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_REMOTE_DsaAckPending)
        {
            if (sFlow->dsaInfo.dsxRetry > 0)
            {// retry available
                 // make a copy of the dsa-ack msg copy
                mgmtMsg =
                    MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxRspCopy);

                //schedule the transmission of DSA-RSP
                MacDot16SsEnqueueMgmtMsg(node,
                                         dot16,
                                         DOT16_CID_PRIMARY,
                                         mgmtMsg);

            }
        }
        else if (sFlow->dsaInfo.dsxTransStatus ==
                 DOT16_FLOW_DSA_REMOTE_DeleteFlow)
        {
            // stay in current status
        }
        return length;
    }

    sfInitial = DOT16_SERVICE_FLOW_REMOTELY_INITIATED;

    // thirdly, TLV encoded information
    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        // handle TLVs
        switch (tlvType)
        {
            case TLV_COMMON_UlServiceFlow:
            {

            }
            case TLV_COMMON_DlServiceFlow:
            {
                if (tlvType == TLV_COMMON_UlServiceFlow)
                {
                    sfDirection = DOT16_UPLINK_SERVICE_FLOW;
                }
                else
                {
                    sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
                }

                // skip the value for this tlv
                sfParamIndex = index + 1;

                //process service flow TLVs
                while (sfParamIndex < index + tlvLength)
                {
                    sfParamTlvType = macFrame[sfParamIndex ++];
                    sfParamTlvLen = macFrame[sfParamIndex ++];
                    switch(sfParamTlvType)
                    {
                        case TLV_DSX_ServiceFlowId:
                        // Service Flow Identifier, 4 bytes
                        {
                            memcpy((char*) &(sfId),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_Cid:
                        // CID assigned to the service flow, 2 bytes
                        {
                            MacDot16TwoByteToShort(
                                    transportCid,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_QoSParamSetType:
                        {
                            // assume admitted and active QoS set are
                            // the same and 0x6 is used
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxSustainedRate:
                        {
                            memcpy((char*) &(qosInfo.maxSustainedRate),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxTrafficBurst:
                        {
                            memcpy((char*) &(qosInfo.maxPktSize),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MinReservedRate:
                        {
                            memcpy((char*) &(qosInfo.minReservedRate),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MinTolerableRate:
                        {
                            memcpy((char*) &(qosInfo.minPktSize),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_ServiceType:
                        {
                            serviceType =
                                (MacDot16ServiceType) macFrame
                                [sfParamIndex];

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_ToleratedJitter:
                        {
                            int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                (unsigned char*) &(qosInfo.toleratedJitter));

                            qosInfo.toleratedJitter =
                                (clocktype) (delayJitter) * MILLI_SECOND;

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxLatency:
                        {
                            int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                    (unsigned char*) &(qosInfo.maxLatency));

                            qosInfo.maxLatency =
                                (clocktype) (delayJitter) * MILLI_SECOND;

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_FixLenVarLanSDU:
                        {
                            isPackingEnabled = TRUE;
                            if (macFrame[sfParamIndex] ==
                                DOT16_FIXED_LENGTH_SDU)
                            {
                                isFixedLengthSDU = TRUE;
                            }
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_SduSize:
                        {
                            fixedLengthSDUSize = macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_Enable:
                        {
                            // ARQ enable
                            isARQEnabled = macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_WindowSize:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);
                            arqParam.arqWindowSize = arqPar ;
                            sfParamIndex += sfParamTlvLen ;
                            break;
                        }
                        case TLV_DSX_ARQ_RetryTimeoutTxDelay:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);

                            arqParam.arqRetryTimeoutTxDelay =
                                clocktype ( arqPar * MICRO_SECOND * 10 );

                                // granularity = 10
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_RetryTimeoutRxDelay:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);
                            arqParam.arqRetryTimeoutRxDelay =
                                clocktype ( arqPar * MICRO_SECOND * 10 );
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_BlockLifetime:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);

                            arqParam.arqBlockLifeTime =
                                clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_SyncLossTimeout:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);

                            arqParam.arqSyncLossTimeout =
                                clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_DeliverInOrder:
                        {
                            arqParam.arqDeliverInOrder = macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_RxPurgeTimeout:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);

                            arqParam.arqRxPurgeTimeout =
                                clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_Blocksize:
                        {
                            MacDot16TwoByteToShort(
                                arqPar,
                            macFrame[sfParamIndex],
                            macFrame[sfParamIndex + 1]);
                            arqParam.arqBlockSize = arqPar;

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }

                        case TLV_DSX_CSSpec:
                        {
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_TrafficIndicationPreference:
                        {
                            TrafficIndicationPreference =
                                macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_CS_TYPE_IPv4:
                        {
                            csSpecIndex = sfParamIndex + 1;
                            while (csSpecIndex <
                                   sfParamIndex + sfParamTlvLen)
                            {
                                csSpecTlvType = macFrame[csSpecIndex ++];
                                csSpecTlvLen = macFrame[csSpecIndex ++];
                                switch (csSpecTlvType)
                                {
                                    case TLV_CS_PACKET_ClassifierRule:
                                    {
                                        csRuleIndex = csSpecIndex + 1;
                                        while (csRuleIndex <
                                                csSpecIndex + csSpecTlvLen)
                                        {
                                            csRuleTlvType =
                                                macFrame[csRuleIndex ++];
                                            csRuleTlvLen =
                                                macFrame[csRuleIndex ++];
                                            switch (csRuleTlvType)
                                            {
                                                case TLV_CS_PACKET_Protocol:
                                                {
                                                    pktInfo.ipProtocol =
                                                        macFrame
                                                        [csRuleIndex];

                                                    csRuleIndex +=
                                                            csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_IpSrcAddr:
                                                {
                                                    NodeAddress addr;
                                                    memcpy(
                                                        (char*) &addr,
                                                        &(macFrame
                                                        [csRuleIndex]),
                                                        4);

                                                    SetIPv4AddressInfo
                                                        (&(pktInfo.srcAddr),
                                                        addr);

                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_IpDestAddr:
                                                {
                                                    NodeAddress addr;
                                                    memcpy(
                                                        (char*) &addr,
                                                        &(macFrame
                                                        [csRuleIndex]),
                                                        4);

                                                    SetIPv4AddressInfo
                                                        (&(pktInfo.dstAddr),
                                                        addr);

                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_SrcPortRange:
                                                {
                                                    MacDot16TwoByteToShort(
                                                        pktInfo.srcPort,
                                                        macFrame
                                                        [csRuleIndex],
                                                        macFrame
                                                        [csRuleIndex + 1]);

                                                    csRuleIndex +=
                                                            csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_DestPortRange:
                                                {
                                                    MacDot16TwoByteToShort(
                                                        pktInfo.dstPort,
                                                        macFrame
                                                        [csRuleIndex],
                                                        macFrame
                                                        [csRuleIndex + 1]);

                                                    csRuleIndex +=
                                                            csRuleTlvLen;

                                                    break;
                                                }
                                                default:
                                                {
                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    if (DEBUG_PACKET_DETAIL
                                                        || DEBUG_FLOW)
                                                    {
                                                        printf("    DSA-REQ"
                                                               " CSRule: "
                                                               " Unknown "
                                                               "TLV, type "
                                                               "= %d,"
                                                            "length = %d\n",
                                                             csRuleTlvType,
                                                             csRuleTlvLen);
                                                    }

                                                    break;
                                                }

                                            } // switch csRuleTlvType
                                        } // while csRuleIndex

                                        csSpecIndex += csSpecTlvLen;

                                        break;
                                    } // case csRule
                                    default:
                                    {
                                        csSpecIndex += csSpecTlvLen;
                                        if (DEBUG_FLOW)
                                        {
                                            printf("    DSA-REQ CSSpec:"
                                                   " Unknown TLV, type ="
                                                   " %d, length = %d\n",
                                                   csSpecTlvType,
                                                   csSpecTlvLen);
                                        }

                                        break;
                                    }
                                } // switch csSpecTlvType
                            } // while csSpecIndex

                            sfParamIndex += sfParamTlvLen;

                            break;
                        } // case TLV_DSX_CS_TYPE_IPv4
                        case TLV_DSX_CS_TYPE_IPv6:
                        {
                            csSpecIndex = sfParamIndex + 1;
                            while (csSpecIndex <
                                   sfParamIndex + sfParamTlvLen)
                            {
                                csSpecTlvType = macFrame[csSpecIndex ++];
                                csSpecTlvLen = macFrame[csSpecIndex ++];
                                switch (csSpecTlvType)
                                {
                                    case TLV_CS_PACKET_ClassifierRule:
                                    {
                                        csRuleIndex = csSpecIndex + 1;
                                        while (csRuleIndex <
                                                csSpecIndex + csSpecTlvLen)
                                        {
                                            csRuleTlvType =
                                                macFrame[csRuleIndex ++];
                                            csRuleTlvLen =
                                                macFrame[csRuleIndex ++];
                                            switch (csRuleTlvType)
                                            {
                                                case TLV_CS_PACKET_Protocol:
                                                {
                                                    pktInfo.ipProtocol =
                                                    macFrame[csRuleIndex];

                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_IpSrcAddr:
                                                {
                                                    in6_addr addr;

                                                    memcpy(
                                                        (char*) &addr,
                                                        &(macFrame
                                                        [csRuleIndex]),
                                                        16);

                                                    SetIPv6AddressInfo(
                                                        &(pktInfo.srcAddr),
                                                        addr);

                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_IpDestAddr:
                                                {
                                                    in6_addr addr;

                                                    memcpy(
                                                        (char*) &addr,
                                                        &(macFrame
                                                        [csRuleIndex]),
                                                        16);

                                                    SetIPv6AddressInfo(
                                                        &(pktInfo.dstAddr),
                                                        addr);

                                                    csRuleIndex +=
                                                            csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_SrcPortRange:
                                                {
                                                    MacDot16TwoByteToShort(
                                                        pktInfo.srcPort,
                                                        macFrame
                                                        [csRuleIndex],
                                                        macFrame
                                                        [csRuleIndex + 1]);
                                                    csRuleIndex +=
                                                        csRuleTlvLen;

                                                    break;
                                                }
                                                case
                                                TLV_CS_PACKET_DestPortRange:
                                                {
                                                    MacDot16TwoByteToShort(
                                                        pktInfo.dstPort,
                                                        macFrame
                                                        [csRuleIndex],
                                                        macFrame
                                                        [csRuleIndex + 1]);
                                                    csRuleIndex +=
                                                            csRuleTlvLen;

                                                    break;
                                                }
                                                default:
                                                {
                                                    csRuleIndex +=
                                                            csRuleTlvLen;
                                                    if (DEBUG_PACKET_DETAIL
                                                        || DEBUG_FLOW)
                                                    {
                                                        printf("    DSA-REQ"
                                                            " CSRule: "
                                                            " Unknown "
                                                            "TLV, type"
                                                            " = %d,"
                                                            " length = "
                                                            "%d\n",
                                                            csRuleTlvType,
                                                            csRuleTlvLen);
                                                    }

                                                    break;
                                                }

                                            } // switch csRuleTlvType
                                        } // while csRuleIndex

                                        csSpecIndex += csSpecTlvLen;

                                        break;
                                    } // case csRule
                                    default:
                                    {
                                        csSpecIndex += csSpecTlvLen;
                                        if (DEBUG_PACKET_DETAIL ||
                                            DEBUG_FLOW)
                                        {
                                            printf("    DSA-REQ CSSpec:"
                                                   " Unknown TLV, type = "
                                                   " %d, length = %d\n",
                                                   csSpecTlvType,
                                                   csSpecTlvLen);
                                        }

                                        break;
                                    }
                                } // switch csSpecTlvType
                            } // while csSpecIndex

                            sfParamIndex += sfParamTlvLen;

                            break;
                        } // case TLV_DSX_CS_TYPE_IPv6
                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf("    DSA-REQ flow info:   Unknown"
                                       " TLV, type = %d, length = %d\n",
                                       sfParamTlvType, sfParamTlvLen);
                            }

                            break;
                        }
                    } // switch sfParamTlvType
                } // while sfParamIndex

                index += tlvLength;

                break;
            } // case TLV_COMMON_UlServiceFlow/TLV_COMMON_DlServiceFlow
            case TLV_COMMON_HMacTuple:
            {

                index += tlvLength;

                break;
            }
            default:
            {
                // TLVs not supported right now
                index += tlvLength;
                if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                {
                    printf("    DSA-REQ:   Unknown TLV, type = %d,"
                           " length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    confirmCode = DOT16_FLOW_DSX_CC_OKSUCC;

    // create a service flow
    if (sfDirection == DOT16_DOWNLINK_SERVICE_FLOW)
    {
        memcpy(&macAddress, &dot16Ss->servingBs->bsId,
            DOT16_MAC_ADDRESS_LENGTH);
    }
    else
    {
        memcpy(&macAddress, &dot16Ss->servingBs->bsId,
            DOT16_MAC_ADDRESS_LENGTH);
    }
//We need to negotiate the ARQ parameters sent by the SS.
//arqParam variable is already assigned the ARQ parameters of the Service
//Flow received from BS above.
//Negotiating ARQ Enable.
//Only when BS and SS both have ARQ Enabled ,the parameter set as
//TRUE for the service flow.

    if (!(isARQEnabled && dot16Ss->isARQEnabled))
    {
       isARQEnabled = FALSE ;
    }

    if (isARQEnabled)
    {
        MacDot16ARQParam dot16SsARQParam;
        if (isPackingEnabled && isFixedLengthSDU)
        {
            isFixedLengthSDU = FALSE;
        }

        memcpy(&dot16SsARQParam,dot16Ss->arqParam,sizeof(MacDot16ARQParam));
        MacDot16ARQConvertParam(&dot16SsARQParam,
                                    dot16Ss->servingBs->frameDuration);
        //Negotiating ARQ_WINDOW_SIZE.
        //Smaller of the two will be used.

        if (arqParam.arqWindowSize > dot16SsARQParam.arqWindowSize )
        {
          arqParam.arqWindowSize = dot16SsARQParam.arqWindowSize ;
        }

        //Negotiating ARQ_RETRY_TIMEOUT_TRANSMITTER_DELAY.
        //Greater of the two will be used.

        if (arqParam.arqRetryTimeoutTxDelay <
                                dot16SsARQParam.arqRetryTimeoutTxDelay)
        {
          arqParam.arqRetryTimeoutTxDelay =
                                dot16SsARQParam.arqRetryTimeoutTxDelay ;
        }

        //Negotiating ARQ_RETRY_TIMEOUT_RECEIVER_DELAY.
        //Greater of the two will be used.

        if (arqParam.arqRetryTimeoutRxDelay <
                                dot16SsARQParam.arqRetryTimeoutRxDelay)
        {
          arqParam.arqRetryTimeoutRxDelay =
                                    dot16SsARQParam.arqRetryTimeoutRxDelay ;
        }

        //Negotiating ARQ_BLOCK_LIFETIME.
        //The DSA-REQ message shall contain the value of this parameter as
        //defined by the parent service flow.

        //Negotiating ARQ_SYNC_LOSS_TIMEOUT.
        //According to the standard the parameter is decided by the BS.


        //Negotiating ARQ_DELIVER_IN_ORDER.
        //According to the standard the value is decided by the SS.

        arqParam.arqDeliverInOrder = dot16SsARQParam.arqDeliverInOrder ;

        //Negotiating ARQ_RX_PURGE_TIMEOUT.
        //The DSA-REQ message shall contain the value of this parameter as
        //defined by the parent service flow..

        //Negotiating ARQ_BLOCK_SIZE.
        //According to the standard
        //Absence of the parameter during a DSA dialog indicates
        //the originator of the message desires the maximum value.

        if (!arqParam.arqBlockSize)
        {
          arqParam.arqBlockSize = DOT16_ARQ_MAX_BLOCK_SIZE ;
        }
        //Else Smaller of the two will be used.
        else if (arqParam.arqBlockSize > dot16SsARQParam.arqBlockSize)
        {
          arqParam.arqBlockSize = dot16SsARQParam.arqBlockSize ;
        }
    }

    MacDot16SsAddServiceFlow(node,
                             dot16,
                             macAddress,
                             serviceType,
                             (unsigned int)-1, // invalid csfId, no use for
                                               // remote initiated servic
                                               //flow
                             &qosInfo,
                             &pktInfo,
                             transId,
                             sfDirection,
                             sfInitial,
                             isPackingEnabled,
                             isFixedLengthSDU,
                             fixedLengthSDUSize,
                             appType,
                             isARQEnabled,
                             &arqParam);




    // get the newly added service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    sFlow->cid = transportCid;
    sFlow->sfid = sfId;

    // set dsxCC
    sFlow->dsaInfo.dsxCC = (MacDot16FlowTransConfirmCode)confirmCode;
    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        sFlow->TrafficIndicationPreference = TrafficIndicationPreference;
    }

    //build the DSA-RSP
    mgmtMsg = MacDot16BuildDsaRspMsg(node,
                                     dot16,
                                     sFlow,
                                     transId,
                                     FALSE, // assume no qos params
                                            //change has been made
                                     dot16Ss->servingBs->primaryCid);

    // save a copy of DSA-RSP
    sFlow->dsaInfo.dsxRspCopy = MESSAGE_Duplicate(node, mgmtMsg);

    //schedule the transmission of DSA-RSP
    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_PRIMARY,
                             mgmtMsg);


    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd DSA-REQ, process it,"
               "build and schedule DSA-RSP, status REMOTE_DsaAckPending\n");
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16SsHandleDsxRvdPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSX-RVD message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsxRvdPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsxRvdMsg* dsxRvd;
    MacDot16ServiceFlow *sFlow;
    unsigned int transId;
    int index;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    dsxRvd = (MacDot16DsxRvdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsxRvdMsg);

    //increase stats
    dot16Ss->stats.numDsxRvdRcvd++;
    transId = dsxRvd->transId[0] * 256 + dsxRvd->transId[1];

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("DSX-RVD with unrecognized transId");
        }
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    if (transId == sFlow->dsaInfo.dsxTransId)
    {
        if (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_DsaRspPending)

        {
            // stop T14
            if (sFlow->dsaInfo.timerT14 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT14);
                sFlow->dsaInfo.timerT14 = NULL;
            }

            // stay at current status
        }
    }
    else if (transId == sFlow->dscInfo.dsxTransId)
    {
        if (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_DscRspPending)
        {
            // stop T14
            if (sFlow->dscInfo.timerT14 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT14);
                sFlow->dscInfo.timerT14 = NULL;
            }

            // stay at current status
        }
    }

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling DSX-RVD message for transId %d\n", transId);
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleDsaRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSA-RSP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsaRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaRspMsg* dsaRsp;
    MacDot16ServiceFlow *sFlow;
    MacDot16ServiceFlowDirection sfDirection;
    MacDot16FlowTransConfirmCode confirmCode;

    unsigned int transId;
    int index;
    unsigned char tlvType;
    int length;
    int tlvLength;

    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    Message* mgmtMsg;

    BOOL isARQEnabled = FALSE ;
    unsigned int arqPar = 0;
    MacDot16ARQParam arqParam ;
    memset(&arqParam,0,sizeof(MacDot16ARQParam));

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    dsaRsp = (MacDot16DsaRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaRspMsg);

    //increase stats
    dot16Ss->stats.numDsaRspRcvd ++;

    // confirm code
    confirmCode = (MacDot16FlowTransConfirmCode)dsaRsp->confirmCode;

    // transId
    transId = dsaRsp->transId[0] * 256 + dsaRsp->transId[1];

    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling DSA-RSP message for transId %d\n", transId);
    }

    if (DEBUG_PACKET)
    {
        printf("    Now, handling TLVs in the DSX-RSP msg\n");
    }

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "Node%d recv DSA-RSP with unrecognized trans ID %d!",
                    node->nodeId, transId);
            ERROR_ReportWarning(errStr);
        }
        return length;
    }
    sFlow->lastAllocTime = node->getNodeTime();
    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        sFlow->TrafficIndicationPreference =
            DOT16e_TRAFFIC_INDICATION_PREFERENCE;
            sFlow->psClassType =
            MacDot16eGetPSClassTypebyServiceType(sFlow->serviceType);
        dot16Ss->psClassParameter[sFlow->psClassType - 1].\
            numOfServiceFlowExists++;
    }// end of if
    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        switch (tlvType)
        {
            case TLV_COMMON_UlServiceFlow:
            case TLV_COMMON_DlServiceFlow:
            {
                if (tlvType == TLV_COMMON_UlServiceFlow)
                {
                    sfDirection = DOT16_UPLINK_SERVICE_FLOW;
                }
                else
                {
                    sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
                }

                // skip the value for this tlv
                sfParamIndex = index + 1;

                //process service flow TLVs
                while (sfParamIndex < index + tlvLength)
                {
                    sfParamTlvType = macFrame[sfParamIndex ++];
                    sfParamTlvLen = macFrame[sfParamIndex ++];
                    switch(sfParamTlvType)
                    {
                        case TLV_DSX_ServiceFlowId:
                        // Service Flow Identifier, 4 bytes
                        {
                            memcpy((char*) &(sFlow->sfid),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_Cid:
                        {
                            // CID assigned to the service flow, 2 bytes
                            MacDot16TwoByteToShort(
                                sFlow->cid,
                                macFrame[sfParamIndex],
                                macFrame[sfParamIndex + 1]);
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_Enable:
                        {
                        // ARQ enable
                            isARQEnabled = macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_WindowSize:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);
                            arqParam.arqWindowSize = arqPar ;
                            sfParamIndex += sfParamTlvLen ;
                            break;
                        }
                        case TLV_DSX_ARQ_RetryTimeoutTxDelay:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);

                            arqParam.arqRetryTimeoutTxDelay =
                                    clocktype ( arqPar * MICRO_SECOND * 10 );

                            // granularity = 10
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_RetryTimeoutRxDelay:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);
                            arqParam.arqRetryTimeoutRxDelay =
                                    clocktype ( arqPar * MICRO_SECOND * 10 );
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_BlockLifetime:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);

                            arqParam.arqBlockLifeTime =
                                    clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_SyncLossTimeout:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);

                            arqParam.arqSyncLossTimeout =
                                    clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_DeliverInOrder:
                        {
                            arqParam.arqDeliverInOrder = macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_RxPurgeTimeout:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);

                            arqParam.arqRxPurgeTimeout =
                                    clocktype ( arqPar * MICRO_SECOND * 10 );

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        case TLV_DSX_ARQ_Blocksize:
                        {
                            MacDot16TwoByteToShort(
                                    arqPar,
                                    macFrame[sfParamIndex],
                                    macFrame[sfParamIndex + 1]);
                            arqParam.arqBlockSize = arqPar;

                            sfParamIndex += sfParamTlvLen;
                            break;
                        }

                        case TLV_DSX_TrafficIndicationPreference:
                        {
                            sFlow->TrafficIndicationPreference =
                                macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;
                            break;
                        }
                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf(" DSA-REQ flow info:   Unknown TLV, "
                                       "type = %d, length = %d\n",
                                       sfParamTlvType, sfParamTlvLen);
                            }
                            break;
                        }
                    } // switch sfParamTlvType
                } // while sfParamIndex

                index += tlvLength;

                break;
            } // case TLV_COMMON_UlServiceFlow/TLV_COMMON_DlServiceFlow
            case TLV_COMMON_HMacTuple:
            {

                index += tlvLength;

                break;
            }
            default:
            {
                // TLVs not supported right now
                index += tlvLength;
                if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                {
                    printf(" DSA-RSP:   Unknown TLV, type = %d, "
                           "length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    if (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_DsaRspPending ||
        sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_RetryExhausted)
    {
        // stop T7 if in DsaRepPending state
        if (sFlow->dsaInfo.timerT7 != NULL &&
            sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_LOCAL_DsaRspPending)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT7);
            sFlow->dsaInfo.timerT7 = NULL;
        }

        // stop T10 if in DOT16_FLOW_DSA_LOCAL_RetryExhausted status
        if (sFlow->dsaInfo.timerT10 != NULL &&
            sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_LOCAL_RetryExhausted)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT10);
            sFlow->dsaInfo.timerT10 = NULL;
        }

        // confirm code is OK?
        if (confirmCode == DOT16_FLOW_DSX_CC_OKSUCC)
        {
            if (sFlow->activated == FALSE)
            {
                // create the corresponding queue in scheduler
                MacDot16SchAddQueueForServiceFlow(node, dot16, sFlow);
            }

            // enable SF
            sFlow->admitted = TRUE; // assume admitted and activated
            sFlow->activated = TRUE;
            sFlow->isARQEnabled = isARQEnabled;
            if (isARQEnabled)
            {
                memcpy(sFlow->arqParam, &arqParam, sizeof(MacDot16ARQParam));
                MacDot16ARQCbInit(node, dot16, sFlow);
            }

            if (DEBUG_FLOW)
             {
                 printf("    ARQ Enabled = %d\n",sFlow->isARQEnabled);

                 if (isARQEnabled)
                     {
                         MacDot16PrintARQParameter(sFlow->arqParam);
                     }
            }
            // set CC to OK
            sFlow->dsaInfo.dsxCC = confirmCode;

            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    DSA-RSP message with OK/success code\n");
            }
        }
        else
        {
            // set CC to reject
            sFlow->dsaInfo.dsxCC = confirmCode;
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    DSA-RSP message with rejet code\n");
            }
        }

        // build DSA-ACK
        // schedule transmission
        mgmtMsg = MacDot16BuildDsaAckMsg(node,
                                         dot16,
                                         sFlow,
                                         dot16Ss->servingBs->primaryCid,
                                         transId);

        // make a copy of the dsa-ack msg for future usage
        sFlow->dsaInfo.dsxAckCopy = MESSAGE_Duplicate(node, mgmtMsg);

        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_PRIMARY,
                                 mgmtMsg);
        // Start T10
        if (sFlow->dsaInfo.timerT10 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT10);
            sFlow->dsaInfo.timerT10 = NULL;
        }
        sFlow->dsaInfo.timerT10 = MacDot16SetTimer(
                                      node,
                                      dot16,
                                      DOT16_TIMER_T10,
                                      dot16Ss->para.t10Interval,
                                      NULL,
                                      transId,
                                      DOT16_FLOW_XACT_Add);

        // Move the status to holding down
        sFlow->dsaInfo.dsxTransStatus = DOT16_FLOW_DSA_LOCAL_HoldingDown;
    }
    else if (sFlow->dsaInfo.dsxTransStatus ==
             DOT16_FLOW_DSA_LOCAL_HoldingDown)
    {
        // implementing pg 241 figure 108
        // make a copy of the dsa-ack msg  and schedule the transmission
        mgmtMsg = MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxAckCopy);

        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_PRIMARY,
                                 mgmtMsg);

        // stay in the current status

        if (DEBUG_PACKET || DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("    this is duplicated DSA-RSP in HoldingDown,"
                    "so DSA-ACK message was lost \n");
        }
    }
    else if (sFlow->dsaInfo.dsxTransStatus ==
             DOT16_FLOW_DSA_LOCAL_DeleteFlow)
    {
        // stay in current status
        if (DEBUG_PACKET || DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("    this is duplicated DSA-RSP in DeleteFlow,"
                   "so DSA-ACK message was lost \n");
        }
    }

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling DSA-RSP message for transId %d sfId %d\n",
               transId, sFlow->sfid);
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16SsHandleDsaAckPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSA-ACK message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsaAckPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaAckMsg* dsaAck;
    int index;
    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    int length;

    // increase stat
    dot16Ss->stats.numDsaAckRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSA-ACK message
    dsaAck = (MacDot16DsaAckMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsaAckMsg);

    MacDot16TwoByteToShort(transId, dsaAck->transId[0], dsaAck->transId[1]);
    confirmCode = (MacDot16FlowTransConfirmCode)dsaAck->confirmCode;

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSA-ACK with transId %d\n",
               transId);
    }

    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            ERROR_ReportWarning("no sflow associates with the DSA-ACK msg");
        }
        return length;
    }
    if (sFlow->isARQEnabled == TRUE)
    {
        MacDot16ARQCbInit(node, dot16, sFlow);
    }
    sFlow->lastAllocTime = node->getNodeTime();
    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        sFlow->TrafficIndicationPreference =
            DOT16e_TRAFFIC_INDICATION_PREFERENCE;
            sFlow->psClassType =
            MacDot16eGetPSClassTypebyServiceType(sFlow->serviceType);
        dot16Ss->psClassParameter[sFlow->psClassType - 1].\
            numOfServiceFlowExists++;
    }// end of if
    if (sFlow->dsaInfo.dsxTransStatus ==
        DOT16_FLOW_DSA_REMOTE_DsaAckPending)
    {
        // stop T8
        if (sFlow->dsaInfo.timerT8 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT8);
            sFlow->dsaInfo.timerT8 = NULL;
        }

        // confirm code is OK?
        if (confirmCode == DOT16_FLOW_DSX_CC_OKSUCC)
        {
            sFlow->admitted = TRUE;
            sFlow->activated = TRUE;
        }

        // start T8 again
        sFlow->dsaInfo.timerT8 = MacDot16SetTimer(
                                     node,
                                     dot16,
                                     DOT16_TIMER_T8,
                                     dot16Ss->para.t8Interval,
                                     NULL,
                                     transId,
                                     DOT16_FLOW_XACT_Add);

        // move the status to HoldingDown
        sFlow->dsaInfo.dsxTransStatus =
             DOT16_FLOW_DSA_REMOTE_HoldingDown;
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSA-ACK, process it,"
                   "status move to REMOTE_Holdingdown\n");
        }
    }
    else if (sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_REMOTE_HoldingDown ||
             sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_REMOTE_DeleteFlow)
    {
        // stay in current status
    }

    return length;
}

//**
// FUNCTION   :: MacDot16SsHandleDscReqPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSC-REQ message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDscReqPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DscReqMsg* dscReq;
    MacDot16ServiceFlow *sFlow;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlowDirection sfDirection;
    MacDot16QoSParameter qosInfo;
    unsigned int transId;
    int index;
    unsigned char tlvType;
    int length;
    int tlvLength;

    unsigned int sfId = 0;
    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    Message* mgmtMsg;
    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    dscReq = (MacDot16DscReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscReqMsg);

    //increase stats
    dot16Ss->stats.numDscReqRcvd ++;

    // transId
    transId = dscReq->transId[0] * 256 + dscReq->transId[1];

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
         printf("handling DSC-REQ message for transId %d\n",
                transId);
    }

    if (DEBUG_PACKET)
    {
        printf("Now, handling TLVs in the DSC-REQ message\n");
    }
    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);
    memset((char*) &qosInfo, 0, sizeof(MacDot16QoSParameter));

    // thirdly, TLV encoded information
    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        switch (tlvType)
        {
            case TLV_COMMON_UlServiceFlow:
            {

            }
            case TLV_COMMON_DlServiceFlow:
            {
                if (tlvType == TLV_COMMON_UlServiceFlow)
                {
                    sfDirection = DOT16_UPLINK_SERVICE_FLOW;
                }
                else
                {
                    sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
                }

                // skip the value for this tlv
                sfParamIndex = index + 1;

                //process service flow TLVs
                while (sfParamIndex < index + tlvLength)
                {
                    sfParamTlvType = macFrame[sfParamIndex ++];
                    sfParamTlvLen = macFrame[sfParamIndex ++];
                    switch(sfParamTlvType)
                    {

                        case TLV_DSX_ServiceFlowId:
                        // Service Flow Identifier, 4 bytes
                        {
                            memcpy((char*) &(sfId),
                                    &(macFrame[sfParamIndex]), 4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_QoSParamSetType:
                        {
                            // assume admitted and active QoS set are
                            // the same and 0x6 is used
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxSustainedRate:
                        {
                            memcpy((char*) &(qosInfo.maxSustainedRate),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxTrafficBurst:
                        {
                            memcpy((char*) &(qosInfo.maxPktSize),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MinReservedRate:
                        {
                            memcpy((char*) &(qosInfo.minReservedRate),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MinTolerableRate:
                        {
                            memcpy((char*) &(qosInfo.minPktSize),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_ToleratedJitter:
                        {
                            int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                (unsigned char*) &(qosInfo.toleratedJitter));

                            qosInfo.maxLatency =
                                (clocktype) (delayJitter) * MILLI_SECOND;

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxLatency:
                        {
                            int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                    (unsigned char*) &(qosInfo.maxLatency));

                            qosInfo.maxLatency =
                                (clocktype) (delayJitter) * MILLI_SECOND;

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }

                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf(" DSC-REQ flow info:   Unknown TLV,"
                                       " type = %d, length = %d\n",
                                       sfParamTlvType, sfParamTlvLen);
                            }

                            break;
                        }
                    } // switch sfParamTlvType
                } // while sfParamIndex

                index += tlvLength;

                break;
            } // case TLV_COMMON_UlServiceFlow/TLV_COMMON_DlServiceFlow
            case TLV_COMMON_HMacTuple:
            {

                index += tlvLength;

                break;
            }
            default:
            {
                // TLVs not supported right now

                index += tlvLength;
                if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                {
                    printf(" DSC-REQ:   Unknown TLV, type = %d, "
                           " length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByServiceFlowId(node, dot16, sfId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no sflow associates with the DSC-REQ msg");
        }
        return length;
    }

    if (sFlow != NULL)
    {
        if (sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_REMOTE_DscAckPending)
        {
            if (sFlow->dscInfo.dsxRetry > 0)
            {// retry available
                // make a copy of the dsc-ack msg  and schedule
                // the transmission
                mgmtMsg = MESSAGE_Duplicate(
                            node,
                            sFlow->dscInfo.dsxRspCopy);

                MacDot16SsEnqueueMgmtMsg(node,
                                         dot16,
                                         DOT16_CID_PRIMARY,
                                         mgmtMsg);

                // statistics
                dot16Ss->stats.numDscRspSent ++;

                // set retry
                sFlow->dscInfo.dsxRetry --;
            }

            // stay in current status
        }
        else if (sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_REMOTE_DeleteFlow)
        {
            // stay in current status
        }
        else if (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSX_NULL)
        {
            // determine whether BS can support the change
            // Now assume it always can support
            sFlow->status = DOT16_FLOW_ChangingRemote;
            sFlow->numXact ++;
            confirmCode = DOT16_FLOW_DSX_CC_OKSUCC;
            sFlow->dscInfo.dsxTransStatus = DOT16_FLOW_DSC_REMOTE_Begin;
            sFlow->dscInfo.dsxTransId = transId;

            // set dsxCC
            sFlow->dscInfo.dsxCC =
                    (MacDot16FlowTransConfirmCode)confirmCode;

            // save old and new QoS info
            // save the QoS state
            sFlow->dscInfo.dscOldQosInfo =
                (MacDot16QoSParameter*)
                MEM_malloc(sizeof(MacDot16QoSParameter));

            memcpy(sFlow->dscInfo.dscOldQosInfo,
                   &(sFlow->qosInfo),
                   sizeof(MacDot16QoSParameter));

            sFlow->dscInfo.dscNewQosInfo =
                (MacDot16QoSParameter*)
                MEM_malloc(sizeof(MacDot16QoSParameter));

            memcpy(sFlow->dscInfo.dscNewQosInfo,
                   &qosInfo,
                   sizeof(MacDot16QoSParameter));

            // change the bandwidth if it is decrease BW
            // if new QoS parameters require less bandwidth
            if (qosInfo.minReservedRate < sFlow->qosInfo.minReservedRate)
            {
                // modify policing and scheduling params

                // decrease bandwidth
                memcpy((char*) &(sFlow->qosInfo),
                        &qosInfo,
                        sizeof(MacDot16QoSParameter));
            }

            // build the DSC-RSP
            mgmtMsg = MacDot16BuildDscRspMsg(
                                node,
                                dot16,
                                sFlow,
                                transId,
                                dot16Ss->servingBs->primaryCid);

            // save a copy of DSC-RSP
            sFlow->dscInfo.dsxRspCopy = MESSAGE_Duplicate(node, mgmtMsg);

            //schedule the transmission
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);

            //increase stats
            dot16Ss->stats.numDscRspSent ++;

            // start T8
            if (sFlow->dscInfo.timerT8 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT8);
                sFlow->dscInfo.timerT8 = NULL;
            }
            sFlow->dscInfo.timerT8 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T8,
                    dot16Ss->para.t8Interval,
                    NULL,
                    transId,
                    DOT16_FLOW_XACT_Change);

            // set dsx retry
            sFlow->dscInfo.dsxRetry --;

            // move the dsx status
            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_REMOTE_DscAckPending;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("    rcvd DSC-REQ, process it, build and schedule"
                       " DSC-RSP, start T8, status REMOTE_DscAckPending"
                       " for sfId %d\n", sFlow->sfid);
            }
        }
    }
    else
    {
        // not in the list
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSC-REQ, but cannot "
                   "find the service with transId %d sfId %d\n",
                   transId, sfId);
        }
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16SsHandleDscRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSC-RSP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDscRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DscRspMsg* dscRsp;
    MacDot16ServiceFlow *sFlow;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlowDirection sfDirection;

    unsigned int transId;
    int index;
    unsigned char tlvType;
    int length;
    int tlvLength;

    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    Message* mgmtMsg;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    dscRsp = (MacDot16DscRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscRspMsg);

    //increase stats
    dot16Ss->stats.numDscRspRcvd ++;

    // confirm code
    confirmCode = (MacDot16FlowTransConfirmCode)dscRsp->confirmCode;

    // transId
    transId = dscRsp->transId[0] * 256 + dscRsp->transId[1];

    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling DSC-RSP message for transId %d\n",
               transId);
    }

    if (DEBUG_PACKET)
    {
        printf("    Now, handling TLVs in the DSC-RSP message\n");
    }

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("DSC-RSP with unrecognized transId");
        }
        return length;
    }

    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        switch (tlvType)
        {
            case TLV_COMMON_UlServiceFlow:
            {

            }
            case TLV_COMMON_DlServiceFlow:
            {
                if (tlvType == TLV_COMMON_UlServiceFlow)
                {
                    sfDirection = DOT16_UPLINK_SERVICE_FLOW;
                }
                else
                {
                    sfDirection = DOT16_DOWNLINK_SERVICE_FLOW;
                }

                // skip the value for this tlv
                sfParamIndex = index + 1;

                //process service flow TLVs
                while (sfParamIndex < index + tlvLength)
                {
                    sfParamTlvType = macFrame[sfParamIndex ++];
                    sfParamTlvLen = macFrame[sfParamIndex ++];
                    switch(sfParamTlvType)
                    {
                        case TLV_DSX_ServiceFlowId:
                        // Service Flow Identifier, 4 bytes
                        {
                            memcpy((char*) &(sFlow->sfid),
                                   &(macFrame[sfParamIndex]),
                                   4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_Cid:
                        // CID assigned to the service flow, 2 bytes
                        {
                            MacDot16TwoByteToShort(
                                sFlow->cid,
                                macFrame[sfParamIndex],
                                macFrame[sfParamIndex + 1]);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf(" DSC-RSP flow info:   Unknown TLV,"
                                       " type = %d, length = %d\n",
                                       sfParamTlvType, sfParamTlvLen);
                            }

                            break;
                        }
                    } // switch sfParamTlvType
                } // while sfParamIndex

                index += tlvLength;

                break;
            } // case TLV_COMMON_UlServiceFlow/TLV_COMMON_DlServiceFlow
            case TLV_COMMON_HMacTuple:
            {

                index += tlvLength;

                break;
            }
            default:
            {
                // TLVs not supported right now
                index += tlvLength;
                if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                {
                    printf(" DSC-RSP:   Unknown TLV, type = %d, length"
                           " = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    if (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_DscRspPending ||
        sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_RetryExhausted)

    {
        // stop T7 if in DsaRepPending state
        if (sFlow->dscInfo.timerT7 != NULL &&
            sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_LOCAL_DscRspPending)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT7);
            sFlow->dscInfo.timerT7 = NULL;
        }

        // stop T10 if in DOT16_FLOW_DSA_LOCAL_RetryExhausted status
        if (sFlow->dscInfo.timerT10 != NULL &&
            sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_LOCAL_RetryExhausted)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT10);
            sFlow->dscInfo.timerT10 = NULL;
        }

        // confirm code is OK?
        if (confirmCode == DOT16_FLOW_DSX_CC_OKSUCC)
        {
            // start using new SF params
            if (sFlow->dscInfo.dscOldQosInfo->minReservedRate <
                sFlow->dscInfo.dscNewQosInfo->minReservedRate)
            {
                // when UL increasing BW, do it now
                // ul decrease BW is done when sending dsc-req msg
                memcpy((char*) &(sFlow->qosInfo),
                        sFlow->dscInfo.dscNewQosInfo,
                        sizeof(MacDot16QoSParameter));
            }

            // set CC to OK
            sFlow->dscInfo.dsxCC = confirmCode;

            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                char clockStr[MAX_STRING_LENGTH];
                ctoa(node->getNodeTime(), clockStr);
                printf("    DSC-RSP message with OK/success code,"
                       "change policy and schedule if needed\n");
            }
        }
        else
        {
            // set CC to reject
            sFlow->dscInfo.dsxCC = confirmCode;

            // restore old QoS parameters
            // only for decrease case
            if (sFlow->dscInfo.dscOldQosInfo->minReservedRate >
                sFlow->dscInfo.dscNewQosInfo->minReservedRate)
            {
                // ul decrease BW is done when sending dsc-req msg
                memcpy((char*) &(sFlow->qosInfo),
                        sFlow->dscInfo.dscOldQosInfo,
                        sizeof(MacDot16QoSParameter));
            }
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                printf("    DSC-RSP message with reject code,"
                       "restore old QoS if needed\n");
            }
        }

        // build DSC-ACK
        // schedule transmission
        // make a copy of the DSC-ACK msg
         mgmtMsg = MacDot16BuildDscAckMsg(node,
                                          dot16,
                                          sFlow,
                                          dot16Ss->servingBs->primaryCid,
                                          transId);

         // make a copy of the dsc-ack msg for future usage
        sFlow->dscInfo.dsxAckCopy = MESSAGE_Duplicate(node, mgmtMsg);

        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_PRIMARY,
                                 mgmtMsg);

        // Start T10
        if (sFlow->dscInfo.timerT10 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT10);
            sFlow->dscInfo.timerT10 = NULL;
        }
        sFlow->dscInfo.timerT10 = MacDot16SetTimer(
                                      node,
                                      dot16,
                                      DOT16_TIMER_T10,
                                      dot16Ss->para.t10Interval,
                                      NULL,
                                      transId,
                                      DOT16_FLOW_XACT_Change);

        // Move the status to holding down
        sFlow->dscInfo.dsxTransStatus = DOT16_FLOW_DSC_LOCAL_HoldingDown;
    }
    else if (sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_LOCAL_HoldingDown)
    {
        // implementing pg 252 figure 117
        // make a copy of the dsc-ack msg  and schedule the transmission
        mgmtMsg = MESSAGE_Duplicate(node, sFlow->dscInfo.dsxAckCopy);
        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_PRIMARY,
                                 mgmtMsg);

        // stay in the current status

        if (DEBUG_PACKET || DEBUG_FLOW)
        {
            printf("    this is duplicated DSC-RSP in HoldingDown,"
                   "so DSC-ACK message was lost \n");
        }
    }
    else if (sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_LOCAL_DeleteFlow)
    {
        // stay in current status
        if (DEBUG_PACKET || DEBUG_FLOW)
        {
            printf("    this is duplicated DSC-RSP in DeleteFlow,"
                   "so DSA-ACK message was lost \n");
        }
    }

    return length; // index - startIndex;
}

// /**
// FUNCTION   :: MacDot16SsHandleDscAckPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSC-ACK message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDscAckPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DscAckMsg* dscAck;
    int index;
    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    int length;

    // increase stat
    dot16Ss->stats.numDscAckRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSC-ACK message
    dscAck = (MacDot16DscAckMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscAckMsg);

    MacDot16TwoByteToShort(transId, dscAck->transId[0], dscAck->transId[1]);
    confirmCode = (MacDot16FlowTransConfirmCode)dscAck->confirmCode;

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSC-ACK with transId %d\n", transId);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no sflow associates with the DSC-ACK msg");
        }

        return length;
    }

    if (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_REMOTE_DscAckPending)
    {
        // stop T8
        if (sFlow->dscInfo.timerT8 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT8);
            sFlow->dscInfo.timerT8 = NULL;
        }

        // confirm code is OK?
        if (confirmCode == DOT16_FLOW_DSX_CC_OKSUCC)
        {
            if (sFlow->dscInfo.dscNewQosInfo > sFlow->dscInfo.dscOldQosInfo)
            {
                // ul increase BW is done when sending dsc-req msg
                memcpy((char*) &(sFlow->qosInfo),
                        sFlow->dscInfo.dscNewQosInfo,
                        sizeof(MacDot16QoSParameter));
            }
        }

        // start T8 again
        sFlow->dscInfo.timerT8 = MacDot16SetTimer(
                                     node,
                                     dot16,
                                     DOT16_TIMER_T8,
                                     dot16Ss->para.t8Interval,
                                     NULL,
                                     transId,
                                     DOT16_FLOW_XACT_Change);
        // move the status to HoldingDown
        sFlow->dscInfo.dsxTransStatus =
             DOT16_FLOW_DSC_REMOTE_HoldingDown;
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSC-ACK, process it,"
                   "status move to REMOTE_Holdingdown\n");
        }
    }
    else if (sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_REMOTE_HoldingDown ||
             sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_REMOTE_DeleteFlow)
    {
        // stay in current status
    }

    return length;

}

// /**
// FUNCTION   :: MacDot16SsHandleDsdReqPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSD-REQ message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsdReqPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsdReqMsg* dsdReq;
    int index;

    unsigned int transId;
    unsigned int sfId;
    MacDot16ServiceFlow* sFlow;
    MacDot16FlowTransConfirmCode confirmCode;
    int length;

    // increase stat
    dot16Ss->stats.numDsdReqRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSD-REQ message
    dsdReq = (MacDot16DsdReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdReqMsg);

    MacDot16TwoByteToShort(transId, dsdReq->transId[0], dsdReq->transId[1]);
    sfId = MacDot16FourByteToInt(dsdReq->sfid);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSD-REQ with transId %d sfId %d\n",
               transId, sfId);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByServiceFlowId(node, dot16, sfId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no sflow associates with the DSD-REQ msg");
        }
        return length;
    }

    if (sFlow != NULL)
    {
        if (sFlow->dsdInfo.dsxTransStatus == DOT16_FLOW_DSX_NULL)
        {
            Message* mgmtMsg;

            // verify the SS is the owner of the service flow
            // Here assume it is passed the exam
            sFlow->status = DOT16_FLOW_Deleting;
            sFlow->numXact ++;
            confirmCode = DOT16_FLOW_DSX_CC_OKSUCC;
            sFlow->dsdInfo.dsxTransId = transId;
            sFlow->dsdInfo.dsxCC = confirmCode;

            // disable the service flow
            sFlow->admitted = FALSE;
            sFlow->activated = FALSE;
            sFlow->dsdInfo.dsxTransStatus = DOT16_FLOW_DSD_REMOTE_Begin;

             // make a copy of the DSD-RSP msg
             mgmtMsg = MacDot16BuildDsdRspMsg(
                            node,
                            dot16,
                            sFlow,
                            dot16Ss->servingBs->primaryCid,
                            transId);

            // make a copy of the dsd-rsp msg for future usage
            sFlow->dsdInfo.dsxRspCopy = MESSAGE_Duplicate(node, mgmtMsg);

            //schedule the transmission of DSD-RSP
            MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_PRIMARY,
                             mgmtMsg);

            // increase stats
            dot16Ss->stats.numDsdRspSent ++;

            // Start T10
            if (sFlow->dsdInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT10);
                sFlow->dsdInfo.timerT10 = NULL;
            }
            sFlow->dsdInfo.timerT10 = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_T10,
                                          dot16Ss->para.t10Interval,
                                          NULL,
                                          transId,
                                          DOT16_FLOW_XACT_Delete);

            // Move the status to holding down
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_REMOTE_HoldingDown;
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                printf("    rcvd a DSD-REQ for transId %d, "
                       "Sent DSD-RSP and start T10, Move to HoldingDown\n",
                       transId);
            }
        }
        else if (sFlow->dsdInfo.dsxTransStatus ==
                     DOT16_FLOW_DSD_REMOTE_HoldingDown)
        {
            Message* mgmtMsg;

            // make a copy of the dsa-ack msg  and schedule the transmission
            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dsdInfo.dsxRspCopy);
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);
            // increase stats
            dot16Ss->stats.numDsdRspSent ++;

            // stay in current status
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
               printf("    this is a duplicated DSD-REQ in HoldingDown,"

                      "so DSD-REQ message was lost \n");
            }
        }
    }
    else
    {
        // not in the list
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSD-REQ, but cannot "
                   "find the service with transId %d sfId %d\n",
                   transId, sfId);
        }
    }

    return length;

}

// /**
// FUNCTION   :: MacDot16SsHandleDsdRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DSD-RSP message
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16SsHandleDsdRspPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16DsdRspMsg* dsdRsp;
    int index;
    unsigned int transId;
    int sfId;
    MacDot16ServiceFlow* sFlow;
    int length;

    // increase stat
    dot16Ss->stats.numDsdRspRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSD-REQ message
    dsdRsp = (MacDot16DsdRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdRspMsg);

    MacDot16TwoByteToShort(transId, dsdRsp->transId[0], dsdRsp->transId[1]);
    sfId = MacDot16FourByteToInt(dsdRsp->sfid);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSD-RSP with transId %d\n", transId);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node, dot16, transId);

    if (sFlow != NULL)
    {
        if (sFlow->dsdInfo.dsxTransStatus ==
            DOT16_FLOW_DSD_LOCAL_DsdRspPending)
        {
            if (DEBUG_FLOW)
            {
                printf("    rcvd DSD-RSP, stop T7, move status to holding "
                       "for service with transId %d sfId %d\n",
                       transId, sfId);
            }
            // stop T7
            if (sFlow->dsdInfo.timerT7 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT7);
                sFlow->dsdInfo.timerT7 = NULL;
            }

            // starT T10
            if (sFlow->dsdInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT10);
                sFlow->dsdInfo.timerT10 = NULL;
            }
            sFlow->dsdInfo.timerT10 = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_T10,
                                          dot16Ss->para.t10Interval,
                                          NULL,
                                          transId,
                                          DOT16_FLOW_XACT_Delete);

            // move status to holding down
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_LOCAL_HoldingDown;
        }
        else if (sFlow->dsdInfo.dsxTransStatus ==
                 DOT16_FLOW_DSD_LOCAL_HoldingDown)
        {
            // Notify CS or high layer the DSD succeeded?
            // stay in current status
        }
    }
    else
    {
        // not in the list
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSD-RSP, but cannot "
                   "find the service with transId %d sfId %d\n",
                   transId, sfId);
        }
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no sflow associates with the DSD-RSP msg");
        }
    }

    return length;
}
//--------------------------------------------------------------------------
// Handle dot16e msg (start)
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16eSsHandleMobNbrAdvPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-NBR-ADV
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobNbrAdvPdu(Node* node,
                                  MacDataDot16* dot16,
                                  unsigned char* macFrame,
                                  int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobNbrAdvMsg* nbrAdv;
    int index;
    unsigned char opFieldBitMap;
    unsigned char fragmentation;
    int numNbrBs;
    int i;

    if (DEBUG_PACKET || DEBUG_HO) // disable it
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("is handling MOB-NBR-ADV msg\n");
    }

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    nbrAdv = (MacDot16eMobNbrAdvMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobNbrAdvMsg);

    if (!(dot16->dot16eEnabled && dot16Ss->operational))
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // only when SS supoorting dot16e & in operational mode should be here!

    if (dot16Ss->servingBs->configChangeCount == nbrAdv->configChangeCount)
    {
        // if the config count is the same as the record, ignore it
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update  stats
    dot16Ss->stats.numNbrAdvRcvd ++;

    // get the common info
    opFieldBitMap = nbrAdv->opFiledBitMap;
    fragmentation = nbrAdv->fragmentaion;
    numNbrBs = nbrAdv->numNeighbors;

    for (i = 0; i < numNbrBs; i++)
    {
        // get the value for each BS
        int nbrIndex =0;
        int nbrLength = 0;;
        unsigned char phyProfileId;
        unsigned lsbDcdUcd;


        int dlChannel = 0;
        unsigned char bsId[DOT16_BASE_STATION_ID_LENGTH];

        nbrLength = macFrame[index ++];
        nbrIndex = index;
        phyProfileId = macFrame[nbrIndex ++];
        lsbDcdUcd = macFrame[nbrIndex ++];

        // TLV
        while (nbrIndex < index + nbrLength)
        {
            unsigned char nbrTlvType;
            unsigned char nbrTlvLength = 0;

            nbrTlvType = macFrame[nbrIndex ++];
            nbrTlvLength = macFrame[nbrIndex ++];
            switch (nbrTlvType)
            {
                case TLV_MOB_NBR_ADV_DcdSettings:
                {
                    int dcdIndex;

                    dcdIndex = nbrIndex;
                    while (dcdIndex < nbrIndex + nbrTlvLength)
                    {

                        unsigned char dcdTlvType;
                        unsigned char dcdTlvLength = 0;

                        dcdTlvType = macFrame[dcdIndex ++];
                        dcdTlvLength = macFrame[dcdIndex ++];
                        switch (dcdTlvType)
                        {
                            case TLV_DCD_ChNumber:
                            {
                                dlChannel = macFrame[dcdIndex ++];

                                break;
                            }
                            case TLV_DCD_BsId:
                            {
                                MacDot16CopyStationId(
                                    bsId,
                                    &(macFrame[dcdIndex]));

                                dcdIndex += dcdTlvLength;

                                break;
                            }
                            default:
                            {
                                dcdIndex += dcdTlvLength;
                                break;
                            }
                        }
                    }
                    nbrIndex += nbrTlvLength;

                    break;
                }
                case TLV_MOB_NBR_ADV_UcdSettings:
                {
                    nbrIndex += nbrTlvLength;

                    break;
                }
                default:
                {
                    nbrIndex += nbrTlvLength;

                    break;
                }

            } // switch (nbrTlvType)
        } // while (nbrIndex)

        MacDot16SsBsInfo* nbrBsInfo = NULL;

        nbrBsInfo = MacDot16eSsGetNbrBsByBsId(node, dot16, bsId);

        if (nbrBsInfo != NULL)
        {
            nbrBsInfo->nbrBsIndex = i; // the index in the NBR-ADV msg
            nbrBsInfo->dlChannel = dlChannel;

            if (dot16->duplexMode == DOT16_TDD)
            {
                nbrBsInfo->ulChannel = dlChannel;
            }

            // since we assume all the others are the same as serving BS
            memcpy((void*)&(nbrBsInfo->trigger),
                   (void*)&(dot16Ss->servingBs->trigger),
                   sizeof(MacDot16eTriggerInfo));

        }
        index += nbrLength;
    } // for (i)

    return MacDot16MacHeaderGetLEN(macHeader); // index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eSsHandleMobScnRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-SCN-RSP
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobScnRspPdu(Node* node,
                                  MacDataDot16* dot16,
                                  unsigned char* macFrame,
                                  int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobScnRspMsg* scanRsp;
    int index;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    scanRsp = (MacDot16eMobScnRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobScnRspMsg);

    if (DEBUG_PACKET || DEBUG_NBR_SCAN)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("is handling MOB-SCN-RSP message:\n"
               "    duration = %d\n", scanRsp->duration);
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update stats
    dot16Ss->stats.numNbrScanRspRcvd ++;

    if (dot16Ss->timers.timerT44)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT44);
        dot16Ss->timers.timerT44 = NULL;
    }
    if (scanRsp->duration == 0)
    {
        // the MOB-SCN-REQ is rejected
        if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_WaitScnRsp)
        {
            dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;
        }
        else if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_Interleave ||
                 dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_Started)
        {
            // get MOB-SCN-RSP with 0 duration while in
            // 1. counting down start frame
            // 2. inerleave state
            // needs cancel scan
            dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;
            dot16Ss->numInterFrames = 0;
            dot16Ss->numScanFrames = 0;
            dot16Ss->numIterations = 0;
            if (dot16Ss->needReportScanResult)
            {
                dot16Ss->needReportScanResult = FALSE;
            }
        }
    }
    else if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_WaitScnRsp)
    {// ss is waiting for a reply of scn-rsp
        MacDot16eMobScnRspInterval* scanInterval;

        scanInterval = (MacDot16eMobScnRspInterval*) &(macFrame[index]);
        index += sizeof(MacDot16eMobScnRspInterval);

        // MOB-SCN-REQ is accepted
        dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_Started;

        dot16Ss->scanDuration = scanRsp->duration;
        dot16Ss->scanInterval = scanInterval->interval;
        dot16Ss->numIterations = scanInterval->iteration;
        dot16Ss->startFrames = scanInterval->startFrame;
        dot16Ss->numScanFrames = 0;
        dot16Ss->numInterFrames = 0;
        dot16Ss->targetBs = NULL;

        dot16Ss->scanReportMetric = scanRsp->reportMetric;
        dot16Ss->scanReportMode = scanRsp->reportMode;
        dot16Ss->scanReportInterval = scanRsp->reportPeriod;

        if (scanRsp->reportMode == DOT16e_SCAN_EventTrigReport)
        {
            dot16Ss->needReportScanResult = TRUE;
        }
        else if (scanRsp->reportMode == DOT16e_SCAN_PeriodicReport)
        {
            dot16Ss->needReportScanResult = TRUE;
        }
        else
        {
            dot16Ss->needReportScanResult = FALSE;
        }

        // increase the statistics
        dot16Ss->stats.numNbrScan ++;

        if (DEBUG_NBR_SCAN)
        {
            printf("Node%d start neighbor BS scan with:\n", node->nodeId);
            printf("    start frame = %d\n", dot16Ss->startFrames);
            printf("    duration of each period = %d\n",
                   dot16Ss->scanDuration);
            printf("    interleave interval period = %d\n",
                   dot16Ss->scanInterval);
            printf("    scan iterations = %d\n", dot16Ss->numIterations);
        }
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eSsHandleMobBshoRspPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-BSHO-RSP
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobBshoRspPdu(Node* node,
                                  MacDataDot16* dot16,
                                  unsigned char* macFrame,
                                  int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobBshoRspMsg* bsHoRsp;
    Message* hoIndMsg;
    int index;

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    bsHoRsp = (MacDot16eMobBshoRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobBshoRspMsg);

    if (DEBUG_PACKET || DEBUG_HO)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("is handling MOB-BSHO-RSP message\n");
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update stats
    dot16Ss->stats.numBsHoRspRcvd ++;

    // stop timer T41
    if (dot16Ss->timers.timerT41)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT41);
        dot16Ss->timers.timerT41 = NULL;
    }

    // build HO-IND and start the actual HO
     // send an MOB-HO-IND to release serving BS
    hoIndMsg = MacDot16eSsBuildMobHoIndMsg(
                   node,
                   dot16);

    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_BASIC,
                             hoIndMsg);

    dot16Ss->hoStatus = DOT16e_SS_HO_WaitHoInd;

    if (DEBUG_HO)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("    schedule MOB_HO_IND to current BS\n");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eSsHandleMobBshoReqPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-BSHO-REQ
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleMobBshoReqPdu(Node* node,
                                  MacDataDot16* dot16,
                                  unsigned char* macFrame,
                                  int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobBshoReqMsg* bsHoReq;
    Message* hoIndMsg;


    MacDot16SsBsInfo* targetBs;
    int index;

    if (DEBUG_PACKET || DEBUG_HO)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("is handling MOB-BSHO-REQ message\n");
    }

    index = startIndex;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    bsHoReq = (MacDot16eMobBshoReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobBshoReqMsg);

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update stats
    dot16Ss->stats.numBsHoReqRcvd ++;

    // if SS is already in HO, discard it.
    if (dot16Ss->hoStatus != DOT16e_SS_HO_None ||
        dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_None)
    {
        if (DEBUG_HO)
        {
            printf("    already in HO %d or scan %d, "
                   "discard the MOB-BSHO-REQ msg\n",
                   dot16Ss->hoStatus,
                   dot16Ss->nbrScanStatus);
        }

        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // stop timer T41
    if (dot16Ss->timers.timerT41)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT41);
        dot16Ss->timers.timerT41 = NULL;
    }


    // select a nbr BS to perform handover
    targetBs = MacDot16eSsSelectBestNbrBs(node, dot16);

    if (!targetBs)
    {
        // no target BS availble for handover
        // stay with current BS
        if (DEBUG_HO)
        {
            printf("    No Target Bs available for handover\n");
        }
        return index - startIndex;
    }
    else
    {
        dot16Ss->targetBs = targetBs;
    }
    // build HO-IND and start the actual HO
     // send an MOB-HO-IND to release serving BS
    hoIndMsg = MacDot16eSsBuildMobHoIndMsg(
                   node,
                   dot16);

    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_BASIC,
                             hoIndMsg);

    dot16Ss->hoStatus = DOT16e_SS_HO_WaitHoInd;

    if (DEBUG_HO)
    {
        printf("    schedule a MOB_HO_IND to current BS\n");
    }

    return index - startIndex;
}

//--------------------------------------------------------------------------
// handle dot16e msg (end)
// /**
// FUNCTION   :: MacDot16eSsHandleDregCmdPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing DREG-CMD
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSsHandleDregCmdPdu(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16eDregCmdMsg* dregCmd;
    int index;
    unsigned char tlvType;
    int tlvLength;
    int length;
    index = startIndex;
    if (!dot16Ss->isIdleEnabled)
    {
        return index - startIndex;
    }
    //increase stats
    dot16Ss->stats.numDregCmdRcvd++;
    dot16Ss->isSSSendDregPDU = FALSE;
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    dregCmd = (MacDot16eDregCmdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eDregCmdMsg);
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling DREG-CMD message\n");
    }

    // get all TLVs in the DREG-CMD message
    while ((index - startIndex) < length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];
        switch (tlvType)
        {
            case TLV_DREG_PagingInfo:
            {
                MacDot16TwoByteToShort(dot16Ss->servingBs->pagingCycle,
                                       macFrame[index], macFrame[index + 1]);
                dot16Ss->servingBs->pagingOffset =
                            (UInt8) macFrame[index + 2];
                MacDot16TwoByteToShort(dot16Ss->servingBs->pagingGroup,
                                        macFrame[index + 3],
                                        macFrame[index + 4]);
                break;
            }
            case TLV_DREG_ReqDuration:
            {
                dot16Ss->servingBs->dregDuration = (UInt8) macFrame[index];
                break;
            }
            case TLV_DREG_PagingCtrlId:
            {
                memcpy(&dot16Ss->pgCtrlId, &(macFrame[index]),
                    sizeof(Address));
                break;
            }
            case TLV_DREG_IdleModeRetainInfo:
            {
                //UInt8 idleModeRtnInfo = macFrame[index];
                break;
            }
            case TLV_DREG_MacHashSkipThshld:
            {
                MacDot16TwoByteToShort(dot16Ss->servingBs->macHashSkipThshld,
                                       macFrame[index], macFrame[index + 1]);
                break;
            }

            default:
            {
                // TLVs not supported right now
                if (DEBUG_PACKET_DETAIL)
                {
                    printf("    Unknown TLV, type = %d, length = %d\n",
                           tlvType, tlvLength);
                }

                break;
            }
        }
        index += tlvLength;
    }

    switch (dregCmd->actCode)
    {
        case DOT16e_DREGCMD_Code0:
        {
            break;
        }
        case DOT16e_DREGCMD_Code1:
        {
            break;
        }
        case DOT16e_DREGCMD_Code2:
        {
            break;
        }
        case DOT16e_DREGCMD_Code3:
        {
            break;
        }
        case DOT16e_DREGCMD_Code4:
        {
            break;
        }
        case DOT16e_DREGCMD_Code5:
        {
            if (dot16Ss->timers.timerT45 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT45);
                dot16Ss->timers.timerT45 = NULL;
            }
            if (dot16Ss->idleSubStatus == DOT16e_SS_IDLE_WaitDregCmd)
            {
                MacDot16eSsSwitchToIdle(node, dot16);
            }
            else if (dot16Ss->idleSubStatus == DOT16e_SS_IDLE_None)
            {

            }
            break;
        }
        case DOT16e_DREGCMD_Code6:
        {
            if (dot16Ss->timers.timerT45 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT45);
                dot16Ss->timers.timerT45 = NULL;
            }
            dot16Ss->idleSubStatus = DOT16e_SS_IDLE_None;
            MacDot16SetTimer(node,
                             dot16,
                             DOT16e_TIMER_DregReqDuration,
                             dot16Ss->servingBs->dregDuration,
                             NULL,
                             0);
            break;
        }
        case DOT16e_DREGCMD_Code7:
        {
            if (dot16Ss->timers.timerT45 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT45);
                dot16Ss->timers.timerT45 = NULL;
            }
            dot16Ss->idleSubStatus = DOT16e_SS_IDLE_WaitDregCmd;
            break;
        }
    }
    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eSSHandleMobPagAdvPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing MOB-PAG-ADV
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + macFrame  : unsigned char* : Pointer to the start of the MAC frame
// + startIndex: int            : Start of the PDU in the frame
// RETURN     :: int : Length of this PDU
// **/
static
int MacDot16eSSHandleMobPagAdvPdu(Node* node,
                                MacDataDot16* dot16,
                                unsigned char* macFrame,
                                int startIndex)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    int index;
    int length;
    int i;
    MacDot16eMobPagAdvActCode actCode;

    index = startIndex;
    actCode = DOT16e_MOB_PAG_ADV_NoAct;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    length = MacDot16MacHeaderGetLEN(macHeader);
    index++; // msg type
    index += 2; // paging id
    int numMac = macFrame[index++];

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handling MOB_PAG-ADV message\n");
    }

    //increase stats
    dot16Ss->stats.numMobPagAdvRcvd++;

    MacDot16TwoByteToShort(dot16Ss->servingBs->pagingGroup,
                            macFrame[index],
                            macFrame[index + 1]);
    if (dot16Ss->idleSubStatus != DOT16e_SS_IDLE_None &&
        dot16Ss->currentPagingGroup != dot16Ss->servingBs->pagingGroup)
    {
        MacDot16eSsPerformLocationUpdate(node, dot16);
    }
    dot16Ss->currentPagingGroup = dot16Ss->servingBs->pagingGroup;
    index += 2; //increment paging group id size

    //for (i = 0; i < mobPagAdv->numMac; i++)
    for (i = 0; i < numMac; i++)
    {
        unsigned char macHash[DOT16e_MAC_HASH_LENGTH];

        MacDot16eUInt64ToMacHash(macHash,
            MacDot16eCalculateMacHash(dot16->macAddress));

        if (MacDot16eSameMacHash(macFrame + index, macHash) == TRUE)
        {
            actCode = (MacDot16eMobPagAdvActCode)macFrame[index + 3];
            switch (actCode)
            {
                case DOT16e_MOB_PAG_ADV_NoAct:
                {
                    //do nothing
                    break;
                }
                case DOT16e_MOB_PAG_ADV_PerRng:
                {
                    MacDot16eSsPerformLocationUpdate(node, dot16);
                    break;
                }
                case DOT16e_MOB_PAG_ADV_EntNw:
                {
                    if (dot16Ss->performNetworkReentry == FALSE)
                    {
                        dot16Ss->performNetworkReentry = TRUE;

                        if (DEBUG_IDLE)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("Received Action code for Network Entry"
                                   " in MOB-PAG-ADV msg.\n");
                        }
                    }
                    break;
                }
            }

            break;
        }
        index += 4;
    }

    return index - startIndex;
}

//--------------------------------------------------------------------------
// handle dot16e msg (end)
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsHandleDataPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a received MAC PDU containing data
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Pointer to the message containing the PDU
// RETURN     :: int : Length of this PDU
// **/
static
Message* MacDot16SsHandleDataPdu(Node* node,
                                 MacDataDot16* dot16,
                                 Message* msg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int pktLen = 0;
    MacDot16MacHeader* macHeader;
    MacDot16ServiceFlow* sFlow;
    Message* returnPtr = NULL;
    Dot16CIDType cid;
    UInt16 newLen = 0;

    if (!dot16Ss->operational)
    {
        returnPtr = msg->next;
        MESSAGE_Free(node, msg);
        if (DEBUG_PACKET)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("free a data packet before node is registered \n");
        }

        return returnPtr;
    }

    pktLen = MESSAGE_ReturnPacketSize(msg);

    // Copy the generic mac header
    macHeader = (MacDot16MacHeader*)MEM_malloc(sizeof(MacDot16MacHeader));
    memcpy(macHeader, MESSAGE_ReturnPacket(msg),
        sizeof(MacDot16MacHeader));
    // Remove CRC information
    if (MacDot16MacHeaderGetCI(macHeader))
    {
        if (DEBUG_CRC)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Remove %d bytes virtual CRC information\n",
                DOT16_CRC_SIZE);
        }
        MESSAGE_RemoveVirtualPayload(node, msg, DOT16_CRC_SIZE);
        newLen =
            (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
            - DOT16_CRC_SIZE);
        MacDot16MacHeaderSetLEN(macHeader,
            newLen);
    }
    // remove MAC header
    MESSAGE_RemoveHeader(node, msg, sizeof(MacDot16MacHeader), TRACE_DOT16);
    newLen = (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
        - sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetLEN(macHeader,
        newLen);

    cid = MacDot16MacHeaderGetCID(macHeader);
    sFlow = MacDot16SsGetServiceFlowByCid(
                node,
                dot16,
                cid);

    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
    {
        if (sFlow != NULL)
        {
            MacDot16ePSClasses* psClass = &dot16Ss->psClassParameter[
                sFlow->psClassType - 1];
            if (DEBUG_SLEEP_PACKET)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("PS class: %d Received data packet service type: %d\n",
                    psClass->classType, sFlow->serviceType);
            }
            psClass->psClassIdleLastNoOfFrames = 0;
        }
    }

    if (sFlow != NULL && sFlow->isARQEnabled)
    {
        int pduLength = MacDot16MacHeaderGetLEN(macHeader);
        returnPtr =
            MacDot16ARQHandleDataPdu(
            node,
            dot16,
            sFlow,
            NULL,
            msg,
            pduLength,
            (BOOL)(MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_PACKING_SUBHEADER));
        MEM_free(macHeader);
        return  returnPtr;
    }

    if (MacDot16IsTransportCid(node, dot16,
        MacDot16MacHeaderGetCID(macHeader)))
    {
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("no sFlow match the CID in data PDU");
            }

            // Drop the packet
            returnPtr = msg->next;
            MESSAGE_Free(node, msg);
            MEM_free(macHeader);
            return returnPtr;
        }

        if ((MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_FRAGMENTATION_SUBHEADER))
        {
            UInt8 fragMsgFC;
            UInt16 fragMsgFSN;
            Message* nextMsg = msg->next;
            msg->next = NULL;
            // Remove fragmentation subheader
            if (DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("SS Remove Fragmentation Subheader\n");
            }
            dot16Ss->stats.numFragmentsRcvd++;
            MacDot16NotExtendedARQDisableFragSubHeader* fragSubHeader =
                (MacDot16NotExtendedARQDisableFragSubHeader*)
                MESSAGE_ReturnPacket(msg);
            fragMsgFC = MacDot16FragSubHeaderGetFC(fragSubHeader);
            fragMsgFSN = MacDot16FragSubHeaderGet3bitFSN(fragSubHeader);
            // remove Packing header
            MESSAGE_RemoveHeader(
                node,
                msg,
                sizeof(MacDot16NotExtendedARQDisableFragSubHeader),
                TRACE_DOT16);

            switch(fragMsgFC)
            {
                case DOT16_NO_FRAGMENTATION:
                {
                    ERROR_ReportWarning("MAC 802.16: Fragmentation "
                        "subheader could not "
                        "contain unfragmented packet");
                    break;
                }
                case DOT16_FIRST_FRAGMENT:
                {
                    MacDot16HandleFirstFragDataPdu(
                        node,
                        dot16,
                        msg,
                        sFlow,
                        fragMsgFSN);

                    if (DEBUG_PACKING_FRAGMENTATION)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("SS handle first fragmented packet\n");
                    }
                    break;
                }
                case DOT16_MIDDLE_FRAGMENT:
                {
                    if ((sFlow->isFragStart == FALSE)
                        || (sFlow->fragMsg == NULL))
                    {
                        // Drop the packet
                        sFlow->bytesSent = 0;
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS Drop the middle fragmented"
                                " packet\n");
                        }
                        MESSAGE_Free(node, msg);
                    }
                    else
                    {
                        Message* tempMsg = sFlow->fragMsg;
                        sFlow->fragFSNNoReceived\
                            [sFlow->noOfFragPacketReceived] =
                            fragMsgFSN;
                        sFlow->noOfFragPacketReceived++;
                        while (tempMsg->next != NULL)
                        {
                            tempMsg = tempMsg->next;
                        }
                        tempMsg->next = msg;
                        msg->next = NULL;
                    }
                    if (DEBUG_PACKING_FRAGMENTATION)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("SS handle middle fragmented packet\n");
                    }
                    break;
                }
                case DOT16_LAST_FRAGMENT:
                {
                    MacDot16HandleLastFragDataPdu(
                        node,
                        dot16,
                        msg,
                        sFlow,
                        fragMsgFSN,
                        dot16Ss->servingBs->bsId);
                    if (DEBUG_PACKING_FRAGMENTATION)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("SS handle last fragmented packet\n");
                    }
                    break;
                }

                default:
                    break;
            }// end of switch

            MEM_free(macHeader);
            returnPtr = nextMsg;
            return returnPtr;
        }

        if ((MacDot16MacHeaderGetGeneralType(macHeader) &
                DOT16_PACKING_SUBHEADER) || (sFlow->isPackingEnabled
            && sFlow->isFixedLengthSDU))
        {
            int pduLength = MacDot16MacHeaderGetLEN(macHeader);
            Message* packMsg = msg;
            UInt16 noOfPackPacketRecvd = 0;
            UInt8 packetDataFC = 0;
            UInt8 packetDataFSN = 0;

            while (pduLength > 0 && (msg != NULL))
            {
                if (DEBUG_PACKING_FRAGMENTATION &&
                    sFlow->isFixedLengthSDU == FALSE)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("SS Remove Packing Subheader\n");
                }
                packMsg = msg;
                msg = msg->next;
                packMsg->next = NULL;
                if ((MacDot16MacHeaderGetGeneralType(macHeader) &
                    DOT16_EXTENDED_TYPE))
                {
                    // Check for ARQ also
                }
                else
                {
                    UInt16 dataLen = 0;
                    MacDot16NotExtendedARQDisablePackSubHeader* packHeader;
                    UInt8 packingHeaderSize = 0;

                    if (sFlow->isFixedLengthSDU == FALSE)
                    {
                        packHeader
                            = (MacDot16NotExtendedARQDisablePackSubHeader*)
                            MESSAGE_ReturnPacket(packMsg);
                        dataLen =
                            MacDot16NotExtendedARQDisablePackSubHeaderGet11bitLength(
                            packHeader);

                        packetDataFSN =
                            MacDot16PackSubHeaderGet3bitFSN(packHeader);
                        // remove Packing header
                        MESSAGE_RemoveHeader(
                            node,
                            packMsg,
                            sizeof(
                            MacDot16NotExtendedARQDisablePackSubHeader),
                            TRACE_DOT16);
                        packetDataFC =
                            MacDot16PackSubHeaderGetFC(packHeader);
                        packingHeaderSize =
                                sizeof(
                                MacDot16NotExtendedARQDisablePackSubHeader);
                    }
                    else
                    {
                        packingHeaderSize = 0;
                        dataLen = (UInt16)sFlow->fixedLengthSDUSize;
                        if ((dataLen - packingHeaderSize) > pduLength)
                        {
                            dataLen = (UInt16)(pduLength - packingHeaderSize);
                        }
                        packetDataFC = DOT16_NO_FRAGMENTATION;
                    }
                    pduLength = pduLength - dataLen - packingHeaderSize;
                    switch(packetDataFC)
                    {
                    case DOT16_NO_FRAGMENTATION:
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS Received packed packet\n");
                            }
                        }
                        break;
                    case DOT16_LAST_FRAGMENT:
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS Received last fragmented packet\n");
                            }
                        }
                        break;
                    case DOT16_FIRST_FRAGMENT:
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS Received first fragmented"
                                    " packet\n");
                            }
                        }
                        break;
                    case DOT16_MIDDLE_FRAGMENT:
                        {
                            ERROR_ReportWarning("MAC 802.16: Packing"
                                " subheader could not contain middle "
                                "fragmented packet");
                        }
                        break;
                    default:
                        break;
                    }
                }
                noOfPackPacketRecvd++;
                if (packetDataFC == DOT16_NO_FRAGMENTATION)
                {
                    // pass to upper layer
                    MacDot16CsPacketFromLower(
                        node,
                        dot16,
                        packMsg,
                        dot16Ss->servingBs->bsId);
                    // increase statistics
                    dot16Ss->stats.numPktsFromLower ++;

                    // update dynamic stats
                    if (node->guiOption)
                    {
                        GUI_SendIntegerData(node->nodeId,
                                            dot16Ss->stats.numPktFromPhyGuiId,
                                            dot16Ss->stats.numPktsFromLower,
                                            node->getNodeTime());
                    }
                }// end of if packetDataFC == DOT16_NO_FRAGMENTATION
                else
                {
                    if (packetDataFC == DOT16_LAST_FRAGMENT)
                    {
                        MacDot16HandleLastFragDataPdu(
                            node,
                            dot16,
                            packMsg,
                            sFlow,
                            packetDataFSN,
                            dot16Ss->servingBs->bsId);
                        dot16Ss->stats.numFragmentsRcvd++;
                    }// end of if packetDataFC[i] == DOT16_LAST_FRAGMENT
                    else
                    {
                        if (packetDataFC == DOT16_FIRST_FRAGMENT)
                        {
                            MacDot16HandleFirstFragDataPdu(
                                node,
                                dot16,
                                packMsg,
                                sFlow,
                                packetDataFSN);
                            dot16Ss->stats.numFragmentsRcvd++;
                        }// end of if packetDataFC[i] == DOT16_FIRST_FRAGMENT
                        else
                        {
                            // Ignore
                        }
                    }
                }
            }// end of while
            MEM_free(macHeader);
            returnPtr = msg;
            return returnPtr;
        }
    }

    returnPtr = msg->next;
    // pass to upper layer
    MacDot16CsPacketFromLower(
        node,
        dot16,
        msg,
        dot16Ss->servingBs->bsId);

    // increase statistics
    dot16Ss->stats.numPktsFromLower++;
    // update dynamic stats
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            dot16Ss->stats.numPktFromPhyGuiId,
                            dot16Ss->stats.numPktsFromLower,
                            node->getNodeTime());
    }

    MEM_free(macHeader);
    return returnPtr;
}

// /**
// FUNCTION   :: MacDot16eSsPerformSleepModeOpoeration
// LAYER      :: MAC
// PURPOSE    :: SUSPEND PS class service flow
//              Set End time and next sleep duration for PS class
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// **/
static
void MacDot16eSsPerformSleepModeOpoeration(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    MacDot16ePSClasses* psClass = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    int currentFrameNumber = dot16Ss->servingBs->frameNumber + 1;
    int i, index;
    if (dot16Ss->isPowerDown == TRUE)
    {
        return;
    }
    for (index = 0; index < DOT16e_MAX_POWER_SAVING_CLASS; index++)
    {
        psClass = &dot16Ss->psClassParameter[index];
        if (((psClass->currentPSClassStatus == POWER_SAVING_CLASS_ACTIVATE) &&
            (psClass->startFrameNum == currentFrameNumber)) ||
            (((psClass->startFrameNum + psClass->sleepDuration) ==
                currentFrameNumber) && psClass->isWaitForSlpRsp == FALSE))
        {
            if ((psClass->isSleepDuration == FALSE) &&
                (psClass->startFrameNum == currentFrameNumber) &&
                (psClass->currentPSClassStatus ==
                POWER_SAVING_CLASS_ACTIVATE))
            {
                if (psClass->classType == POWER_SAVING_CLASS_3)
                {
                    int noOfSleepFrames = 0;
                    if (McDot16eSsCheckIsPowerDownNeeded(node, dot16, psClass,
                        noOfSleepFrames))
                    {
                        clocktype powerDownDuration;
                        psClass->isSleepDuration = TRUE;
                        dot16Ss->isPowerDown = TRUE;
                        powerDownDuration = (noOfSleepFrames *
                            (dot16Ss->servingBs->frameDuration -
                             dot16Ss->para.rtg));
                        MacDot16SetTimer(node,
                            dot16,
                            DOT16e_TIMER_SlpPowerUp,
                            powerDownDuration,
                            NULL,
                            0);
                        // Stop listening channel
                        MacDot16StopListening(
                            node,
                            dot16,
                            dot16Ss->servingBs->dlChannel);
                        dot16Ss->initInfo.dlSynchronized = FALSE;
                        dot16Ss->initInfo.ulParamAccquired = FALSE;
                        if (DEBUG_SLEEP_PS_CLASS_3)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS goes in power down state for %d "
                                "frames\n", noOfSleepFrames);
                        }
                    }
                }
                else
                {
                    for (i =0; i < DOT16_NUM_SERVICE_TYPES; i++)
                    {
                        sFlow = dot16Ss->ulFlowList[i].flowHead;
                        if (sFlow == NULL)
                        {
                            sFlow = dot16Ss->dlFlowList[i].flowHead;
                        }
                        if (sFlow && (psClass->classType ==
                            sFlow->psClassType))
                        {
                            MacDot16eSsEnableDisableServiceFlowQueues(
                                node,
                                dot16,
                                i,
                                SUSPEND);
                            if (DEBUG_SLEEP)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS SUSPEND class: %d for service "
                                    "type: %d Sleep Duration: %d frames\n",
                                    psClass->classType,
                                    sFlow->serviceType,
                                    psClass->sleepDuration);
                            }
                        }
                    }// end of for
                    psClass->isSleepDuration = TRUE;
                    if (psClass->classType == POWER_SAVING_CLASS_1
                        && psClass->trafficIndReq &&
                        psClass->isMobTrfIndReceived == FALSE)
                    {
                        // Set variable to Deactivate the PS class 1
                        psClass->statusNeedToChange = TRUE;
                    }
                }
            }
            else
            {
                for (i =0; i < DOT16_NUM_SERVICE_TYPES; i++)
                {
                    sFlow = dot16Ss->ulFlowList[i].flowHead;
                    if (sFlow == NULL)
                    {
                        sFlow = dot16Ss->dlFlowList[i].flowHead;
                    }
                    if (sFlow && (psClass->classType == sFlow->psClassType))
                    {
                        MacDot16eSsEnableDisableServiceFlowQueues(
                            node,
                            dot16,
                            i,
                            RESUME);
                        if (DEBUG_SLEEP)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS RESUME class: %d for service type: %d"
                                "\n", psClass->classType,
                                sFlow->serviceType);
                        }
                    }// end of if
                }// end of for
                psClass->isSleepDuration = FALSE;
                psClass->isMobTrfIndReceived = FALSE;
                psClass->startFrameNum = currentFrameNumber;
                psClass->psClassIdleLastNoOfFrames = 0;
                if (psClass->classType == POWER_SAVING_CLASS_1)
                {
                    int previousSleepDuration =
                        psClass->sleepDuration;
                    // incr sleep duration
                    psClass->sleepDuration = MIN(2 *
                        previousSleepDuration,
                        (int)(psClass->finalSleepWindowBase *
                        (UInt32)pow(2.0, (double)
                        psClass->finalSleepWindowExponent)));
                }
                psClass->startFrameNum += psClass->listeningWindow;
                if (psClass->classType == POWER_SAVING_CLASS_2)
                {
                    if (MacDot16eSsNeedToDeactivaePSClassType(node, dot16,
                        POWER_SAVING_CLASS_2))
                    {
                        psClass->statusNeedToChange = TRUE;
                    }
                }
            }
        }// end of if
    }// end of if
} // end of MacDot16eSsPerformSleepModeOpoeration

// /**
// FUNCTION   :: MacDot16eSSNeedToBufferMobTrfIndPdu
// LAYER      :: MAC
// PURPOSE    :: Buffer this received MAC PDU,
//              if classI is waiting for SLP RSP
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Message contains the PDU
// RETURN     :: BOOL : TRUE/FALSE
// **/
BOOL MacDot16eSSNeedToBufferMobTrfIndPdu(Node* node,
                                         MacDataDot16* dot16,
                                         Message* msg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ePSClasses* psClass = NULL;
    for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        psClass = &dot16Ss->psClassParameter[i];
        if ((psClass->classType == POWER_SAVING_CLASS_1) &&
            (psClass->currentPSClassStatus == POWER_SAVING_CLASS_DEACTIVATE
            || psClass->currentPSClassStatus ==
            POWER_SAVING_CLASS_STATUS_NONE)
            && (psClass->isWaitForSlpRsp == TRUE))
        {
            dot16Ss->mobTrfIndPdu = msg;
            return TRUE;
        }
    }
    return FALSE;
}// end of MacDot16eSSNeedToBufferMobTrfIndPdu


// /**
// FUNCTION   :: MacDot16SsHandlePdu
// LAYER      :: MAC
// PURPOSE    :: Handle a receive MAC PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Message contains the PDU
// + rxBeginTime: clocktype    : Signal arrival time of the receive frame
// RETURN     :: int : Length of this PDU
// **/
static
Message* MacDot16SsHandlePdu(Node* node,
                        MacDataDot16* dot16,
                        Message* msg,
                        clocktype rxBeginTime)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    unsigned char* macFrame;
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    unsigned char msgType;
    int index;
    BOOL needFree = TRUE;
    Message* returnPtr = NULL;

    macFrame = (unsigned char*) MESSAGE_ReturnPacket(msg);
    index = 0;

    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    cid = MacDot16MacHeaderGetCID(macHeader);

    if (MacDot16IsManagementCid(node, dot16, cid))
    {
        // this is a management message

        // get the type of the management type
        msgType = macFrame[index + sizeof(MacDot16MacHeader)];

        // If in nbr scan mode, only handle DL-MAP
        if (dot16->dot16eEnabled &&
            dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_InScan)
        {
            if (msgType != DOT16_DL_MAP)
            {
                index += MacDot16MacHeaderGetLEN(macHeader);
                returnPtr = msg->next;
                MESSAGE_Free(node, msg);

                return returnPtr;
            }
        }

        switch (msgType)
        {
            case DOT16_DL_MAP:
            { // this is a DL-MAP message
                index += MacDot16SsHandleDlMapPdu(node,
                                                  dot16,
                                                  macFrame,
                                                  index,
                                                  rxBeginTime);
                break;
            }

            case DOT16_UL_MAP:
            { // this is a UL-MAP message
                index += MacDot16SsHandleUlMapPdu(node,
                                                  dot16,
                                                  macFrame,
                                                  index,
                                                  rxBeginTime);
                break;
            }

            case DOT16_DCD:
            { // this is a DCD message
                index += MacDot16SsHandleDcdPdu(node,
                                                dot16,
                                                macFrame,
                                                index);
                break;
            }

            case DOT16_UCD:
            { // this is a UCD message
                index += MacDot16SsHandleUcdPdu(node,
                                                dot16,
                                                macFrame,
                                                index);
                break;
            }

            case DOT16_RNG_RSP:
            { // this is a RNG-RSP message
                index += MacDot16SsHandleRngRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break;
            }
            case DOT16_SBC_RSP:
            {
                // this is a SBC-RSP message
                index += MacDot16SsHandleSbcRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_PKM_RSP:
            {
                // this is a PKM-RSP message
                index += MacDot16SsHandlePkmRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break;
            }
            case DOT16_REG_RSP:
            {
                // this is a REG-RSP message
                index += MacDot16SsHandleRegRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break;
            }
            case DOT16_REP_REQ:
            {
                // this is a REP-REQ message
                index += MacDot16SsHandleRepReqPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSA_REQ:
            {
                // this is a REG-RSP message
                index += MacDot16SsHandleDsaReqPdu(
                    node,
                    dot16,
                    macFrame,
                    index,
                    (TraceProtocolType)msg->originatingProtocol);

                break;
            }
            case DOT16_DSX_RVD:
            {
                // this is a DSX-RVD message
                index += MacDot16SsHandleDsxRvdPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSA_RSP:
            {
                // this is a DSX-RSP message
                index += MacDot16SsHandleDsaRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSA_ACK:
            {
                // this is a DSA-ACK message
                index += MacDot16SsHandleDsaAckPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSC_REQ:
            {
                // this is a DSC-REQ message
                index += MacDot16SsHandleDscReqPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSC_RSP:
            {
                // this is a DSC-RSP message
                index += MacDot16SsHandleDscRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSC_ACK:
            {
                // this is a DSC-ACK message
                index += MacDot16SsHandleDscAckPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);

                break;
            }
            case DOT16_DSD_REQ:
            {
                // this is a DSD-REQ message
                index += MacDot16SsHandleDsdReqPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break;
            }
            case DOT16_DSD_RSP:
            {
                // this is a DSD-RSP message
                index += MacDot16SsHandleDsdRspPdu(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break;
            }
            case DOT16e_MOB_NBR_ADV:
            {
                // this is a MOB-NBR-ADV message
                index += MacDot16eSsHandleMobNbrAdvPdu(node,
                                                       dot16,
                                                       macFrame,
                                                       index);
                break;
            }
            case DOT16e_MOB_SCN_RSP:
            {
                // this is a MOB-SCN-RSP message
                index += MacDot16eSsHandleMobScnRspPdu(node,
                                                       dot16,
                                                       macFrame,
                                                       index);
                break;
            }
            case DOT16e_MOB_BSHO_RSP:
            {
                // this is a MOB_BSHO_RSP message
                index += MacDot16eSsHandleMobBshoRspPdu(node,
                                                        dot16,
                                                        macFrame,
                                                        index);
                break;
            }
            case DOT16e_MOB_BSHO_REQ:
            {
                // this is a MOB_BSHO_REQ message
                index += MacDot16eSsHandleMobBshoReqPdu(node,
                                                        dot16,
                                                        macFrame,
                                                        index);
                break;
            }
            case DOT16e_MOB_SLP_RSP:
            {
                // this is a MOB_BSHO_REQ message
                index += MacDot16eSsHandleMobSlpRspPdu(node,
                                                        dot16,
                                                        macFrame,
                                                        index);
                break;
            }
            case DOT16e_MOB_TRF_IND:
            {
                if (dot16Ss->mobTrfIndPdu != NULL)
                {
                    MESSAGE_Free(node, dot16Ss->mobTrfIndPdu);
                    dot16Ss->mobTrfIndPdu = NULL;
                }

                // this is a MOB_BSHO_REQ message
                if (MacDot16eSSNeedToBufferMobTrfIndPdu(node, dot16, msg)
                    == FALSE)
                {
                    index += MacDot16eSsHandleMobTrfIndPdu(node,
                                                            dot16,
                                                            macFrame,
                                                            index);
                }
                else
                {
                    needFree = FALSE;
                }
                break;
            }
            case DOT16_ARQ_FEEDBACK:
            {
                //this a an ARQ feedback message.
                // this is a MOB_BSHO_REQ message
                index += MacDot16ARQHandleFeedback(node,
                                                   dot16,
                                                   macFrame,
                                                   index);
                break ;
            }
            case DOT16_ARQ_DISCARD:
            {
                index += MacDot16ARQHandleDiscardMsg(node,
                                            dot16,
                                            macFrame,
                                            0);
                break ;
            }
            case DOT16_ARQ_RESET:
            {
                index += MacDot16ARQHandleResetMsg(node,
                                            dot16,
                                            macFrame,
                                            0);

                break;
            }
            case DOT16_DREG_CMD:
            {
                index += MacDot16eSsHandleDregCmdPdu(node,
                                                     dot16,
                                                     macFrame,
                                                     index);
                break;
            }
            case DOT16e_MOB_PAG_ADV:
            {
                index += MacDot16eSSHandleMobPagAdvPdu(node,
                                                       dot16,
                                                       macFrame,
                                                       index);
                break;
            }
            default:
            {
                ERROR_ReportError("MAC 802.16: Unknown mgmt msg type!");
                break;
            }
        }

        returnPtr = msg->next;
        if (needFree)
        {
            MESSAGE_Free(node, msg);
        }
        return returnPtr;
    }
    else
    {
        // this is a data PDU
        returnPtr = MacDot16SsHandleDataPdu(node, dot16, msg);
        return returnPtr;
    }
}

// /**
// FUNCTION   :: MacDot16SsSendRangeRequest
// LAYER      :: MAC
// PURPOSE    :: Send a RNG-REQ message to BS
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + cid       : Dot16CIDType : CID of the ranging
// + burst     : MacDot16UlBurst* : The burst for tx opp
// RETURN     :: void : NULL
// **/
static
void MacDot16SsSendRangeRequest(Node* node,
                                MacDataDot16* dot16,
                                Dot16CIDType cid,
                                MacDot16UlBurst* burst,
                                BOOL flag)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    MacDot16RngReqMsg* rngRequest;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];

    Dot16BurstInfo burstInfo;
    int durationInPs = 0;
    int index = 0;
    int i;

    Message* msg = NULL;

    if (dot16Ss->rngType == DOT16_CDMA
        && burst != NULL && burst->uiuc == DOT16_UIUC_CDMA_RANGE)
    {
        MacDot16RngCdmaInfo cdmaInfo;
        if (flag == FALSE)
        {
            cdmaInfo.ofdmaFrame = 0;
            if (cid == DOT16_INITIAL_RANGING_CID)
            {
                cdmaInfo.rangingCode = DOT16_CDMA_START_INITIAL_RANGING_CODE
                    + (UInt8)(RANDOM_nrand(dot16->seed)
                    % (DOT16_CDMA_END_INITIAL_RANGING_CODE
                    - DOT16_CDMA_START_INITIAL_RANGING_CODE));
                if (DEBUG_CDMA_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("SS Send the Initial Ranging code on first slot\n");
                }
            }
            else
            {
                cdmaInfo.rangingCode = DOT16_CDMA_START_PERIODIC_RANGING_CODE
                    + (UInt8)(RANDOM_nrand(dot16->seed)
                    % (DOT16_CDMA_END_PERIODIC_RANGING_CODE
                    - DOT16_CDMA_START_PERIODIC_RANGING_CODE));
                if (DEBUG_CDMA_RANGING)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("Send Periodic CDMA ranging code in UL MAP pdu\n");
                }
                // Restart T3
                if (dot16Ss->timers.timerT3 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
                    dot16Ss->timers.timerT3 = NULL;
                }
                // set the timeout timer for waitting RNG-RSP
                dot16Ss->timers.timerT3 = MacDot16SetTimer(
                                              node,
                                              dot16,
                                              DOT16_TIMER_T3,
                                              dot16Ss->para.t3Interval,
                                              NULL,
                                              0);
                dot16Ss->servingBs->rangeRetries = 0;
                dot16Ss->servingBs->rangeBOCount = 0;
            }
            cdmaInfo.ofdmaSubchannel = (UInt8)(burst->ulCdmaInfo.ulMapIE12.\
                subchannelOffset + (RANDOM_nrand(dot16->seed)
                % burst->ulCdmaInfo.ulMapIE12.noOfSubchannels));
            cdmaInfo.ofdmaSymbol = (UInt8)(burst->ulCdmaInfo.ulMapIE12.\
                ofmaSynbolOffset + (RANDOM_nrand(dot16->seed)
                % burst->ulCdmaInfo.ulMapIE12.noOfOfdmaSymbols));

            cdmaInfo.ofdmaSymbol = cdmaInfo.ofdmaSymbol /
                (UInt8) MacDot16PhySymbolsPerPs(DOT16_UL);

            memcpy(&(dot16Ss->sendCDMAInfo),
                &cdmaInfo,
                sizeof(MacDot16RngCdmaInfo));
        }
        else
        {
            memcpy(&cdmaInfo,
                &(dot16Ss->sendCDMAInfo),
                sizeof(MacDot16RngCdmaInfo));
            cdmaInfo.ofdmaSymbol += 1; // transmit in to next symbole / PS
            if (cdmaInfo.ofdmaSymbol > dot16Ss->ulDurationInPs)
            {
                // transmit in to previous symbol / PS
                cdmaInfo.ofdmaSymbol -= 2;
                memcpy(&(dot16Ss->sendCDMAInfo),
                    &cdmaInfo,
                    sizeof(MacDot16RngCdmaInfo));
            }
            if (DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Send the Ranging code on adjacent slot\n");
            }
        }

        memcpy((payload + index),
            &cdmaInfo,
            sizeof(MacDot16RngCdmaInfo));
        index += sizeof(MacDot16RngCdmaInfo);

        msg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

        MESSAGE_PacketAlloc(node, msg, index, TRACE_DOT16);
        memcpy(MESSAGE_ReturnPacket(msg), payload, index);

        // add the burst information for PHY
        //burst->duration = dot16Ss->para.rangeOppSize;
        burst->duration = MacDot16PhyBytesToPs(
            node,
            dot16,
            index,
            &(dot16Ss->servingBs->ulBurstProfile[PHY802_16_BPSK]),
            DOT16_UL) + dot16Ss->para.sstgInPs;
        durationInPs = MacDot16SsBuildBurstInfo(node,
                                                dot16,
                                                burst,
                                                index,
                                                &burstInfo,
                                                &cdmaInfo);
        MacDot16AddBurstInfo(node, msg, &burstInfo);
        dot16Ss->stats.numCdmaRngCodeSent++;
    }
    else
    {
        if (DEBUG_PACKET)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("transmits a RNG-REQ msg\n");
        }

        macHeader = (MacDot16MacHeader*) &(payload[index]);
        index += sizeof(MacDot16MacHeader);

        memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
        MacDot16MacHeaderSetCID(macHeader, cid);

        rngRequest = (MacDot16RngReqMsg*) &(payload[index]);
        index += sizeof(MacDot16RngReqMsg);

        rngRequest->type = (UInt8)DOT16_RNG_REQ;
        rngRequest->dlChannelId = (UInt8)dot16Ss->servingBs->dlChannel;

        if (cid == DOT16_INITIAL_RANGING_CID)
        {
            //contention ranging opp
            // initial ranging in attempts to join the network
            // add requested downlink burst profile, type = 1, length = 1
            payload[index ++] = TLV_RNG_REQ_DlBurst;  // type
            payload[index ++] = 1;  // length
            payload[index ++] = dot16Ss->servingBs->leastRobustDiuc;
            // dl burst profile

            // add SS MAC address TLV
            payload[index ++] = TLV_RNG_REQ_MacAddr;  // type
            payload[index ++] = DOT16_MAC_ADDRESS_LENGTH;  // length
            for (i = 0; i < DOT16_MAC_ADDRESS_LENGTH; i ++)
            {
                payload[index + i] = dot16->macAddress[i];
            }
            index += DOT16_MAC_ADDRESS_LENGTH;

            // add range purpose & old sering BS Id
            if (dot16Ss->hoStatus == DOT16e_SS_HO_HoOngoing)
            {
                payload[index ++] = TLV_RNG_REQ_RngPurpose;
                payload[index ++] = 1;
                payload[index ++] = DOT16_RNG_REQ_HO; // currently in HO

                payload[index ++] = TLV_RNG_REQ_ServBsId;
                payload[index ++] = DOT16_MAC_ADDRESS_LENGTH;
                MacDot16CopyStationId((unsigned char*)&(payload[index]),
                                       dot16Ss->prevServBsId);

                index += DOT16_MAC_ADDRESS_LENGTH;
            }

        // add the following when attempting to re-enter the network
        if (dot16Ss->performNetworkReentry == TRUE)
        {
            payload[index ++] = TLV_RNG_REQ_RngPurpose;
            payload[index ++] = 1;
            payload[index ++] = DOT16_RNG_REQ_HO; //reentry from idle

            payload[index ++] = TLV_RNG_REQ_PagingCtrlId;
            payload[index ++] = sizeof(Address);
            memcpy(&(payload[index]), &dot16Ss->pgCtrlId, sizeof(Address));
            index += sizeof(Address);
        }
        else if (dot16Ss->performLocationUpdate == TRUE)
        {
            payload[index ++] = TLV_RNG_REQ_RngPurpose;
            payload[index ++] = 1;
            payload[index ++] = DOT16_RNG_REQ_LocUpd; //loc upd from idle
        }
        }
        else
        {
            // allocated ranging opp
            if (dot16Ss->operational == FALSE)
            {
                // for init ranging on basic CID
                //add mac version, DOT16_MAC_VER_2004 in this implementation
                payload[index ++] = TLV_COMMON_MacVersion;  // type
                payload[index ++] = 1;  // length
                payload[index ++] = DOT16_MAC_VER_2004;  // mac vesion

                if (dot16Ss->initInfo.adjustAnmolies == TRUE)
                {
                    //this is only for initial range
                    //add ranging anomalies
                    payload[index ++] = TLV_RNG_REQ_RngAnomal;  // type
                    payload[index ++] = 1;  // length
                    payload[index ++] = dot16Ss->initInfo.rngAnomalies;
                    // anomaly

                    //reset the adjustAnmolies
                    dot16Ss->initInfo.adjustAnmolies = FALSE;
                }
            }
            else if (dot16Ss->operational == TRUE &&
                dot16Ss->periodicRngInfo.toSendRngReq == TRUE &&
                dot16Ss->periodicRngInfo.rngReqScheduled == TRUE)
            {
                // this is only for perioidic ranging,
                // refer to  standard IEEE802.16-2004 pg. 204
                // add ranging anomalies
                payload[index ++] = TLV_RNG_REQ_RngAnomal;  // type
                payload[index ++] = 1;  // length
                payload[index ++] = (UInt8)
                    dot16Ss->periodicRngInfo.lastAnomaly;
                // anomaly
                dot16Ss->periodicRngInfo.toSendRngReq = FALSE;
            }
            else if (dot16Ss->operational == TRUE)
            {
                // This is to implement pg 199, when no data grant is issued
                // to SS while SS needs to change downlink profile
                // add requested downlink burst profile, type = 1, length = 1
                payload[index ++] = TLV_RNG_REQ_DlBurst;  // type
                payload[index ++] = 1;  // length
                payload[index ++] = dot16Ss->servingBs->leastRobustDiuc;
                // dl burst profile
            }
        }

        //set the Mac header length field
        MacDot16MacHeaderSetLEN(macHeader, index);

        msg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

        MESSAGE_PacketAlloc(node, msg, index, TRACE_DOT16);
        memcpy(MESSAGE_ReturnPacket(msg), payload, index);

        if (!(dot16Ss->rngType == DOT16_CDMA &&
            (dot16Ss->initInfo.rngState == DOT16_SS_RNG_BcstRngOppAlloc
            || (dot16Ss->initInfo.rngState == DOT16_SS_RNG_WaitBcstRngOpp))))
        {
            // add the burst information for PHY
            burst->duration = dot16Ss->para.rangeOppSize;
            durationInPs = MacDot16SsBuildBurstInfo(node,
                                                    dot16,
                                                    burst,
                                                    index,
                                                    &burstInfo);
            MacDot16AddBurstInfo(node, msg, &burstInfo);
        }
        // increase statistics
        dot16Ss->stats.numRngReqSent ++;
    }// end of if-else burst->uiuc == DOT16_UIUC_CDMA_RANGE

    if (dot16Ss->rngType == DOT16_CDMA &&
        (dot16Ss->initInfo.rngState == DOT16_SS_RNG_BcstRngOppAlloc
        || dot16Ss->initInfo.rngState == DOT16_SS_RNG_WaitBcstRngOpp))
    {
        if (dot16Ss->cdmaAllocationMsgList != NULL)
        {
            MESSAGE_FreeList(node, dot16Ss->cdmaAllocationMsgList);
            dot16Ss->cdmaAllocationMsgList = NULL;
        }
        dot16Ss->cdmaAllocationMsgList = msg;
    }
    else
    {

        MacDot16PhyTransmitUlBurst(node,
                                   dot16,
                                   msg,
                                   dot16Ss->servingBs->ulChannel,
                                   durationInPs);
    }
    if (dot16Ss->operational == FALSE &&
        (dot16Ss->initInfo.rngState == DOT16_SS_RNG_BcstRngOppAlloc ||
         dot16Ss->initInfo.rngState == DOT16_SS_RNG_Backoff ||
         dot16Ss->initInfo.rngState == DOT16_SS_RNG_RngInvited ||
         dot16Ss->initInfo.rngState == DOT16_SS_RNG_WaitBcstInitRngOpp ||
         dot16Ss->initInfo.rngState ==
            DOT16_SS_RNG_WaitInvitedRngRspForInitOpp ||
         dot16Ss->initInfo.rngState ==
            DOT16_SS_RNG_WaitRngRspForInitOpp ||
         dot16Ss->initInfo.rngState == DOT16_SS_RNG_WaitBcstRngOpp) &&
        dot16Ss->initInfo.initRangingCompleted == FALSE)
    {
        // T3 ony used for initial range
        if (dot16Ss->timers.timerT3 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
            dot16Ss->timers.timerT3 = NULL;
        }
        // set the timeout timer for waitting RNG-RSP
        dot16Ss->timers.timerT3 = MacDot16SetTimer(
                                      node,
                                      dot16,
                                      DOT16_TIMER_T3,
                                      dot16Ss->para.t3Interval,
                                      NULL, // dot16Ss->timers.timerT3,
                                      0);
        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING )
        {
            printf("    Sent contention RNG-REQ for initial range with "
                   "cid %d and start T3 on ch %d, request diuc %d\n", cid,
                   dot16Ss->servingBs->ulChannel,
                   dot16Ss->servingBs->leastRobustDiuc);
        }
    }
    else
    {
        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING )
        {
            if (dot16Ss->operational)
            {
                printf("    Sent RNG-REQ for periodic range "
                       "with cid %d \n", cid);
            }
            else
            {
                printf("    Sent init RNG-REQ for in invited range "
                       "with cid %d \n", cid);
            }
        }
    }
}



static
void MacDot16SsSendCDMABandwidthRangingCode(Node* node,
                                    MacDataDot16* dot16,
                                    MacDot16UlBurst* burst,
                                    MacDot16SsBWReqRecord* bwReqRecord)
{
    MacDot16RngCdmaInfo cdmaInfo;
    Message* msg;
    Dot16BurstInfo burstInfo;
    int durationInPs;

    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    cdmaInfo.ofdmaFrame = 0;

    cdmaInfo.rangingCode = DOT16_CDMA_START_BWREQUEST_RANGING_CODE
        + (UInt8)(RANDOM_nrand(dot16->seed)
        % (DOT16_CDMA_END_BWREQUEST_RANGING_CODE
        - DOT16_CDMA_START_BWREQUEST_RANGING_CODE));

    cdmaInfo.ofdmaSubchannel = (UInt8)(burst->ulCdmaInfo.\
        ulMapIE12.subchannelOffset + (RANDOM_nrand(dot16->seed)
        % burst->ulCdmaInfo.ulMapIE12.noOfSubchannels));
    cdmaInfo.ofdmaSymbol = (UInt8)(burst->ulCdmaInfo.ulMapIE12.\
        ofmaSynbolOffset + (RANDOM_nrand(dot16->seed)
        % burst->ulCdmaInfo.ulMapIE12.noOfOfdmaSymbols));
    cdmaInfo.ofdmaSymbol = cdmaInfo.ofdmaSymbol /
        (UInt8) MacDot16PhySymbolsPerPs(DOT16_UL);

    memcpy(&(bwReqRecord->cdmaInfo),
        &cdmaInfo,
        sizeof(MacDot16RngCdmaInfo));
    msg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, msg, sizeof(MacDot16RngCdmaInfo), TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(msg),
        &(bwReqRecord->cdmaInfo),
        sizeof(MacDot16RngCdmaInfo));

    // add the burst information for PHY
    burst->duration = MacDot16PhyBytesToPs(
        node,
        dot16,
        sizeof(MacDot16RngCdmaInfo),
        &(dot16Ss->servingBs->ulBurstProfile[PHY802_16_BPSK]),
        DOT16_UL) + dot16Ss->para.sstgInPs;
    durationInPs = MacDot16SsBuildBurstInfo(node,
                                            dot16,
                                            burst,
                                            sizeof(MacDot16RngCdmaInfo),
                                            &burstInfo,
                                            &cdmaInfo);
    MacDot16AddBurstInfo(node, msg, &burstInfo);
    MacDot16PhyTransmitUlBurst(node,
                               dot16,
                               msg,
                               dot16Ss->servingBs->ulChannel,
                               durationInPs);
    if (DEBUG_CDMA_RANGING)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("SS Send the Ranging code for bandwidth request\n");
    }
    bwReqRecord->rangingCodeSent = TRUE;
    dot16Ss->stats.numCdmaBwRngCodeSent++;
}// end of MacDot16SsSendCDMABandwidthRangingCode

// /**
// FUNCTION   :: MacDot16SsSendBandwidthRequest
// LAYER      :: MAC
// PURPOSE    :: Send a bandwidth request to BS
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + burst     : MacDot16UlBurst* : The burst for tx opp
// RETURN     :: void : NULL
// **/
static
void MacDot16SsSendBandwidthRequest(Node* node,
                                    MacDataDot16* dot16,
                                    MacDot16UlBurst* burst)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    Dot16BurstInfo burstInfo;
    int durationInPs;
    char* payload;

    Message* msg;
    int msgSize;

    if (dot16Ss->bwReqType == DOT16_BWReq_CDMA)
    {
        Message* tempMsg = NULL;
        MacDot16SsBWReqRecord* prevBwReqRecord = NULL;
        if (dot16Ss->bwReqHead == NULL)
        {
            return;
        }

        msgSize = sizeof(MacDot16MacHeader);

        // get the first pre-created BW request from list
        bwReqRecord = dot16Ss->bwReqHead;
        prevBwReqRecord = bwReqRecord;
        while (bwReqRecord != NULL)
        {
            if (bwReqRecord->isCDMAAllocationIERecvd == TRUE)
            {
        // allocate message
        msg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

        MESSAGE_PacketAlloc(node, msg, msgSize, TRACE_DOT16);
        payload = (char*) MESSAGE_ReturnPacket(msg);

                // copy over the content of the request
                memcpy(payload, (char*) &(bwReqRecord->macHeader), msgSize);
                tempMsg = dot16Ss->cdmaAllocationMsgList;
                if (tempMsg == NULL)
                {
                    dot16Ss->cdmaAllocationMsgList = msg;
                }
                else
                {
                    while (tempMsg->next != NULL)
                    {
                        tempMsg = tempMsg->next;
                    }
                    tempMsg->next = msg;
                }
                msg->next = NULL;
                if (bwReqRecord == dot16Ss->bwReqHead)
                {
                    dot16Ss->bwReqHead = bwReqRecord->next;
                    MEM_free(bwReqRecord);
                    bwReqRecord = dot16Ss->bwReqHead;
                    prevBwReqRecord = bwReqRecord;
                }
                else if (bwReqRecord == dot16Ss->bwReqTail)
                {
                    dot16Ss->bwReqTail = prevBwReqRecord;
                    prevBwReqRecord->next = NULL;
                    MEM_free(bwReqRecord);
                    bwReqRecord = NULL;
                }
                else
                {
                    prevBwReqRecord->next = bwReqRecord->next;
                    MEM_free(bwReqRecord);
                    bwReqRecord = prevBwReqRecord->next;
                }
            }
            else
            {
                prevBwReqRecord = bwReqRecord;
                bwReqRecord = bwReqRecord->next;
            }
        }
        return;
    }

    // if no bandwidth request is scheduled, just return
    if (dot16Ss->bwReqInContOpp == NULL)
    {
        return;
    }

    if (DEBUG_PACKET || DEBUG_BWREQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("transmit a contention BW request UIUC %d,"
               "my basic cid is %d\n",
               burst->uiuc,
               dot16Ss->servingBs->basicCid);
    }

    msgSize = sizeof(MacDot16MacHeader);

    // allocate message
    msg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

    MESSAGE_PacketAlloc(node, msg, msgSize, TRACE_DOT16);
    payload = (char*) MESSAGE_ReturnPacket(msg);

    // get the first pre-created BW request from list
    bwReqRecord = dot16Ss->bwReqInContOpp;

    // copy over the content of the request
    memcpy(payload, (char*) &(bwReqRecord->macHeader), msgSize);

    // add the burst information for PHY
    burst->duration = dot16Ss->para.requestOppSize;
    durationInPs = MacDot16SsBuildBurstInfo(node,
                                            dot16,
                                            burst,
                                            msgSize,
                                            &burstInfo);
    MacDot16AddBurstInfo(node, msg, &burstInfo);

    // pass to PHY for transmission
    MacDot16PhyTransmitUlBurst(node,
                               dot16,
                               msg,
                               dot16Ss->servingBs->ulChannel,
                               durationInPs);

    //set timeout timer for the bandwidth request
    dot16Ss->isWaitBwGrant = TRUE;
    if (dot16Ss->timers.timerT16 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT16);
        dot16Ss->timers.timerT16 = NULL;
    }
    dot16Ss->timers.timerT16 = MacDot16SetTimer(
                                   node,
                                   dot16,
                                   DOT16_TIMER_T16,
                                   dot16Ss->para.t16Interval,
                                   NULL,
                                   0);

    if (DEBUG_BWREQ)
    {
        printf("    send contention BW req with %d and start T16\n",
                MacDot16MacHeaderGetBR(&(bwReqRecord->macHeader)));
    }
}

// /**
// FUNCTION   :: MacDot16SsSendPaddedData
// LAYER      :: MAC
// PURPOSE    :: Send a padded data to BS in case no pDU to send in the
//               allocated tx Opps
// PARAMETERS ::
// + node      : Node*            : Pointer to node
// + dot16     : MacDataDot16*    : Pointer to DOT16 structure
// + cid       : unsinged int     : SS's basic CID
// + burst     : MacDot16UlBurst* : The burst for tx opp
// RETURN     :: void : NULL
static
void MacDot16SsSendPaddedData(Node* node,
                              MacDataDot16* dot16,
                              unsigned int cid,
                              MacDot16UlBurst* burst)
{
    MacDot16MacHeader* macHeader;
    MacDot16PaddedMsg* paddedMsg;
    Dot16BurstInfo burstInfo;
    int durationInPs;
    unsigned char* payload;
    int index;

    Message* msg;
    int msgSize;

    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    msg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(msg != NULL, "MAC 802.16: Out of memory!");

    msgSize = sizeof(MacDot16MacHeader);
    msgSize += sizeof(MacDot16PaddedMsg);

    MESSAGE_PacketAlloc(node, msg, msgSize, TRACE_DOT16);
    payload = (unsigned char*) MESSAGE_ReturnPacket(msg);

    index = 0;

    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, cid);

    paddedMsg = (MacDot16PaddedMsg*) &(payload[index]);
    index += sizeof(MacDot16PaddedMsg);

    paddedMsg->type = DOT16_PADDED_DATA;

    //set the Mac header length field
    MacDot16MacHeaderSetLEN(macHeader, index);

    // add the burst information for PHY
    durationInPs = MacDot16SsBuildBurstInfo(node,
                                            dot16,
                                            burst,
                                            index,
                                            &burstInfo);
    if (durationInPs == -1)
    {
        // unable to hold the padding message
        MESSAGE_Free(node, msg);
        return;
    }

    MacDot16AddBurstInfo(node, msg, &burstInfo);

    MacDot16PhyTransmitUlBurst(node,
                               dot16,
                               msg,
                               dot16Ss->servingBs->ulChannel,
                               durationInPs);
    if (DEBUG_RANGING)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("send padded data\n");
    }
}

// /**
// FUNCTION   :: MacDot16SsCheckOutgoingMsg
// LAYER      :: MAC
// PURPOSE    :: Go through the list of outgoing messages before
//               being passed to the PHY. This gives a chance to
//               update proper state variables, timers etc.
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + firstMsg  : Message*     : Point to the first msg of the list of PDUs
// RETURN     :: void : NULL
// **/
static
void MacDot16SsCheckOutgoingMsg(Node* node,
                                MacDataDot16* dot16,
                                Message* firstMsg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    unsigned char* payload;
    unsigned char mgmtMsgType;
    int macHeaderLen;
    Message* msg;

    macHeaderLen = sizeof(MacDot16MacHeader);

    // go through the PDU list
    msg = firstMsg;
    while (msg != NULL)
    {
        Message* nextPdu;
        // get the MAC header
        payload = (unsigned char*) MESSAGE_ReturnPacket(msg);
        macHeader = (MacDot16MacHeader*) payload;
        nextPdu = MacDot16SkipPdu(msg);

        cid = MacDot16MacHeaderGetCID(macHeader);

        // check if it is management message & not BW request
        if (MacDot16MacHeaderGetHT(macHeader) == 0 &&
            MacDot16IsManagementCid(node, dot16, cid))
        {
            // it is a management message, get its type
            mgmtMsgType = payload[macHeaderLen];

            if (dot16->dot16eEnabled)
            {
                if (mgmtMsgType == DOT16e_MOB_SCN_REQ)
                {
                    if (dot16Ss->nbrScanStatus < DOT16e_SS_NBR_SCAN_Started)
                    {
                        dot16Ss->scanReqRetry --;

                        // start T44
                        if (dot16Ss->timers.timerT44 != NULL)
                        {
                            MESSAGE_CancelSelfMsg(
                                        node,
                                        dot16Ss->timers.timerT44);
                            dot16Ss->timers.timerT44 = NULL;
                        }
                        dot16Ss->timers.timerT44 =
                            MacDot16SetTimer(node,
                                             dot16,
                                             DOT16e_TIMER_T44,
                                             dot16Ss->para.t44Interval,
                                             NULL,
                                             0);
                        dot16Ss->nbrScanStatus =
                                DOT16e_SS_NBR_SCAN_WaitScnRsp;

                        // update stats
                        dot16Ss->stats.numNbrScanReqSent ++;
                    }
                }

                else if (mgmtMsgType == DOT16e_MOB_HO_IND &&
                         dot16Ss->hoStatus == DOT16e_SS_HO_WaitHoInd)
                {
                    dot16Ss->hoStatus = DOT16e_SS_HO_HoIndSent;
                    // update stats
                    dot16Ss->stats.numHoIndSent ++;

                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("sent the HO IND to BS\n");
                    }
                }
                else if (mgmtMsgType == DOT16e_MOB_MSHO_REQ)
                {
                    dot16Ss->hoNumRetry --;
                    // strat retramsmissio timer T41?
                    if (dot16Ss->timers.timerT41 != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node,
                                    dot16Ss->timers.timerT41);
                        dot16Ss->timers.timerT41 = NULL;
                    }
                    dot16Ss->timers.timerT41 =
                        MacDot16SetTimer(node,
                                         dot16,
                                         DOT16e_TIMER_T41,
                                         dot16Ss->para.t41Interval,
                                         NULL,
                                         0);

                    dot16Ss->hoStatus = DOT16e_SS_HO_LOCAL_WaitHoRsp;

                    // update stats
                    dot16Ss->stats.numMsHoReqSent ++;

                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("send MSHO REQ to BS\n");
                    }
                }
                else if (mgmtMsgType == DOT16e_MOB_SLP_REQ)
                {
                    if (DEBUG_SLEEP)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("send MOB SLP REQ to BS\n");
                    }
                    dot16Ss->stats.numMobSlpReqSent++;
                    //set a new T43
                    if (dot16Ss->timers.timerT43 == NULL)
                    {
                        dot16Ss->timers.timerT43 = MacDot16SetTimer(
                                                       node,
                                                       dot16,
                                                       DOT16e_TIMER_T43,
                                                       dot16Ss->para.\
                                                       t43Interval,
                                                       NULL,
                                                       0);
                    }
                    else
                    {
                        MESSAGE_CancelSelfMsg(node,
                            dot16Ss->timers.timerT43);
                        dot16Ss->timers.timerT43 = MacDot16SetTimer(
                                                       node,
                                                       dot16,
                                                       DOT16e_TIMER_T43,
                                                       dot16Ss->para.\
                                                       t43Interval,
                                                       NULL,
                                                       0);
                    }
                }
                else if (mgmtMsgType == DOT16_DREG_REQ)
                {
                    dot16Ss->stats.numDregReqSent++;
                    dot16Ss->idleSubStatus = DOT16e_SS_IDLE_WaitDregCmd;
                    if (dot16Ss->timers.timerT45 != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT45);
                    }
                    dot16Ss->timers.timerT45 = MacDot16SetTimer(node,
                        dot16,
                        DOT16e_TIMER_T45,
                        dot16Ss->para.t45Interval,
                        NULL,
                        0);
                }
            }

            // for SBC-REQ start T18
            if (mgmtMsgType == DOT16_SBC_REQ)
            {
                // increase statistics
                dot16Ss->stats.numSbcReqSent ++;

                //increase the sbc req retry
                dot16Ss->servingBs->sbcRequestRetries ++;

                //start T18
                if (dot16Ss->timers.timerT18 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT18);
                    dot16Ss->timers.timerT18 = NULL;
                }
                dot16Ss->timers.timerT18 = MacDot16SetTimer(
                                               node,
                                               dot16,
                                               DOT16_TIMER_T18,
                                               dot16Ss->para.t18Interval,
                                               NULL,
                                               0);

                if (DEBUG_NETWORK_ENTRY)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send SBC REQ and Set T18\n");
                }
            }
            // for REG-REQ start T6
            else if (mgmtMsgType == DOT16_REG_REQ)
            {
                // increase statistics
                dot16Ss->stats.numRegReqSent ++;

                //increase the sbc req retry
                dot16Ss->servingBs->regRetries ++;

                //start T6
                if (dot16Ss->timers.timerT6 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT6);
                    dot16Ss->timers.timerT6 = NULL;

                }
                dot16Ss->timers.timerT6 = MacDot16SetTimer(
                                                   node,
                                                   dot16,
                                                   DOT16_TIMER_T6,
                                                   dot16Ss->para.t6Interval,
                                                   NULL,
                                                   0);
            }
            else if (mgmtMsgType == DOT16_DSA_REQ)
            {
                MacDot16DsaReqMsg* dsaReq;
                MacDot16ServiceFlow* sFlow;
                unsigned int transId;

                dsaReq = (MacDot16DsaReqMsg*) &(payload[macHeaderLen]);
                transId = dsaReq->transId[0] * 256 + dsaReq->transId[1];
                sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                                          dot16,
                                                          transId);

                if (sFlow == NULL)
                {
                    if (PRINT_WARNING)
                    {
                        ERROR_ReportWarning(
                                "NULL sflow w/the scheduled DSA-REQ");
                    }
                    msg = msg->next;
                    continue;
                }

                // incease the stat
                dot16Ss->stats.numDsaReqSent ++;

                if (sFlow->dsaInfo.dsxTransStatus ==
                    DOT16_FLOW_DSA_LOCAL_HoldingDown)
                {
                    msg = msg->next;
                    continue;
                }

                // Start T7, transId as reference
                if (sFlow->dsaInfo.timerT7 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT7);
                    sFlow->dsaInfo.timerT7 = NULL;
                }

                sFlow->dsaInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             sFlow->dsaInfo.dsxTransId,
                                             DOT16_FLOW_XACT_Add);

                // Start T14 if needed, transId as reference
                if (sFlow->dsaInfo.timerT14 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT14);
                    sFlow->dsaInfo.timerT14 = NULL;
                }

                sFlow->dsaInfo.timerT14 = MacDot16SetTimer(
                                              node,
                                              dot16,
                                              DOT16_TIMER_T14,
                                              dot16Ss->para.t14Interval,
                                              NULL,
                                              sFlow->dsaInfo.dsxTransId,
                                              DOT16_FLOW_XACT_Add);

                // Set retry
                sFlow->dsaInfo.dsxRetry --;

                // move the dsx status to local RSA-RSP pending
                sFlow->dsaInfo.dsxTransStatus =
                    DOT16_FLOW_DSA_LOCAL_DsaRspPending;


                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSA-REQ, start T7 and T14 "
                           "and status is local DSA-RSP pending\n");
                }
            }
            else if (mgmtMsgType == DOT16_DSC_REQ)
            {
                MacDot16DscReqMsg* dscReq;
                MacDot16ServiceFlow* sFlow;
                unsigned int transId;

                dscReq = (MacDot16DscReqMsg*) &(payload[macHeaderLen]);
                transId = dscReq->transId[0] * 256 + dscReq->transId[1];
                sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                                          dot16,
                                                          transId);
                if (sFlow == NULL)
                {
                    if (PRINT_WARNING)
                    {
                        ERROR_ReportWarning(
                            "NULL flow handle with the schedueld DSC_REQ");
                    }
                    msg = msg->next;
                    continue;
                }

                if (sFlow->dscInfo.dsxTransStatus ==
                    DOT16_FLOW_DSC_LOCAL_HoldingDown)
                {
                    msg = msg->next;
                    continue;
                }

                // Start T7, transId as reference
                if (sFlow->dscInfo.timerT7 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT7);
                    sFlow->dscInfo.timerT7 =  NULL;
                }
                sFlow->dscInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             sFlow->dscInfo.dsxTransId,
                                             DOT16_FLOW_XACT_Change);

                // Start T14 if needed, transId as reference
                if (sFlow->dscInfo.timerT14 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT14);
                    sFlow->dscInfo.timerT14 = NULL;
                }
                sFlow->dscInfo.timerT14 = MacDot16SetTimer(
                                              node,
                                              dot16,
                                              DOT16_TIMER_T14,
                                              dot16Ss->para.t14Interval,
                                              NULL,
                                              sFlow->dscInfo.dsxTransId,
                                              DOT16_FLOW_XACT_Change);

                // Set retry
                sFlow->dscInfo.dsxRetry --;

                // move the dsx status to local RSA-RSP pending
                sFlow->dscInfo.dsxTransStatus =
                    DOT16_FLOW_DSC_LOCAL_DscRspPending;

                // incease the stat
                dot16Ss->stats.numDscReqSent ++;

                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSC-REQ, start T7 and T14 "
                           "and status is local DSC-RSP pending\n");
                }
            }
            else if (mgmtMsgType == DOT16_DSD_REQ)
            {
                MacDot16DsdReqMsg* dsdReq;
                MacDot16ServiceFlow* sFlow = NULL;
                unsigned int transId;

                dsdReq = (MacDot16DsdReqMsg*) &(payload[macHeaderLen]);
                transId = dsdReq->transId[0] * 256 + dsdReq->transId[1];
                sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                                          dot16,
                                                          transId);

                if (sFlow == NULL)
                {
                    if (PRINT_WARNING)
                    {
                        ERROR_ReportWarning(
                            "NULL flow handle with scheduled DSD-REQ!");
                    }
                    msg = msg->next;
                    continue;
                }

                if (sFlow->dsdInfo.dsxTransStatus ==
                    DOT16_FLOW_DSD_LOCAL_HoldingDown)
                {
                    msg = msg->next;
                    continue;
                }

                // Start T7, transId as reference
                if (sFlow->dsdInfo.timerT7 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT7);
                    sFlow->dsdInfo.timerT7 = NULL;
                }
                sFlow->dsdInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             sFlow->dsdInfo.dsxTransId,
                                             DOT16_FLOW_XACT_Delete);

                // Set retry
                sFlow->dsdInfo.dsxRetry --;

                // move the dsx status to local RSD-RSP pending
                sFlow->dsdInfo.dsxTransStatus =
                    DOT16_FLOW_DSD_LOCAL_DsdRspPending;

                // incease the stat
                dot16Ss->stats.numDsdReqSent ++;
                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSD-REQ tansId %d,"
                           "start T7 and status is local DSD-RSP pending\n",
                           sFlow->dsdInfo.dsxTransId);
                }
            }
            else if (mgmtMsgType == DOT16_DSA_ACK)
            {
                MacDot16DsaAckMsg* dsaAck;
                MacDot16ServiceFlow* sFlow;
                unsigned int transId;

                dsaAck = (MacDot16DsaAckMsg*) &(payload[macHeaderLen]);
                transId = dsaAck->transId[0] * 256 + dsaAck->transId[1];
                sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                                          dot16,
                                                          transId);
                if (sFlow == NULL)
                {
                    if (PRINT_WARNING)
                    {
                        ERROR_ReportWarning(
                            "NULL flow handle with the scheduled DSA-ACK!");
                    }
                    msg = msg->next;
                    continue;
                }
                // incease the stat
                dot16Ss->stats.numDsaAckSent ++;

                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSA-ACK, status is OK\n");
                }
            }
            else if (mgmtMsgType == DOT16_DSC_ACK)
            {
                MacDot16DscAckMsg* dscAck;
                MacDot16ServiceFlow* sFlow;
                unsigned int transId;

                dscAck = (MacDot16DscAckMsg*) &(payload[macHeaderLen]);
                transId = dscAck->transId[0] * 256 + dscAck->transId[1];
                sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                                          dot16,
                                                          transId);
                if (sFlow == NULL)
                {
                    if (PRINT_WARNING)
                    {
                        ERROR_ReportWarning(
                            "NULL flow handle with the scheduled DSC-ACK!");
                    }
                    msg = msg->next;
                    continue;
                }

                // incease the stat
                dot16Ss->stats.numDscAckSent ++;

                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSC-ACK, status is op\n");
                }
            }
            else if (mgmtMsgType == DOT16_DSA_RSP)
            {
                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("send DSA-RSP, status is OK\n");
                }
            }
        }
        else if (MacDot16MacHeaderGetHT(macHeader) == 0)
        {
            MacDot16ServiceFlow* sFlow =
                MacDot16SsGetServiceFlowByCid(node,dot16,cid);
            if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled && sFlow)
            {
                MacDot16ePSClasses* psClass = &dot16Ss->psClassParameter[
                    sFlow->psClassType - 1];
                psClass->psClassIdleLastNoOfFrames = 0;
                if (DEBUG_SLEEP_PACKET)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("PS class: %d Send data packet to BS service"
                        " type: %d\n", psClass->classType,
                        sFlow->serviceType);
                }
            }
            if (sFlow != NULL && sFlow->isARQEnabled)
            {
                int pduSize = MacDot16MacHeaderGetLEN(macHeader)
                    - macHeaderLen;
                payload += macHeaderLen;
                int sduSize = macHeaderLen;
                int skipHedaerSize = macHeaderLen;

                if ((MacDot16MacHeaderGetGeneralType(macHeader))==
                                DOT16_FAST_FEEDBACK_ALLOCATION_SUBHEADER )
                {
                    payload += sizeof(MacDot16DataGrantSubHeader);
                    sduSize += sizeof(MacDot16DataGrantSubHeader);
                    skipHedaerSize += sizeof(MacDot16DataGrantSubHeader);
                    //removing the size of the MacDot16DataGrantSubHeader
                    pduSize -= sizeof(MacDot16DataGrantSubHeader);
                }
                if (dot16Ss->isCRCEnabled)
                {
                    pduSize -= DOT16_CRC_SIZE;
                    sduSize += DOT16_CRC_SIZE;
                }

                if (MacDot16MacHeaderGetGeneralType(macHeader)
                    & DOT16_PACKING_SUBHEADER)
                {
                    MacDot16ExtendedorARQEnablePackSubHeader* packSubheader
                        = NULL;
                    Message* startPdu = msg;
                    pduSize = MacDot16MacHeaderGetLEN(macHeader);

                    while (pduSize > 0)
                    {
                        UInt16 numARQBlocks = 0;
                        payload = (unsigned char*)(MESSAGE_ReturnPacket(msg)
                            + skipHedaerSize);
                        packSubheader =
                            (MacDot16ExtendedorARQEnablePackSubHeader*)payload;
                        sduSize +=
                            MacDot16ExtendedARQEnablePackSubHeaderGet11bitLength(
                            packSubheader) +
                            sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
                        pduSize -= sduSize;
                        startPdu = msg;

                        MacDot16ARQCalculateNumBlocks(numARQBlocks,
                            (unsigned int)
                            MacDot16ExtendedARQEnablePackSubHeaderGet11bitLength(
                            packSubheader));
                        MacDot16ARQSetRetryAndBlockLifetimeTimer(node,
                            dot16,
                            sFlow,
                            MESSAGE_ReturnInfo(msg),
                            numARQBlocks,
                            MacDot16PackSubHeaderGet11bitBSN(packSubheader));
                        while (sduSize > 0)
                        {
                            sduSize -= MESSAGE_ReturnPacketSize(msg);
                            msg = msg->next;
                        }// end of while
                        MESSAGE_RemoveInfo(node, startPdu,INFO_TYPE_DEFAULT);
                        sduSize = 0;
                        skipHedaerSize = 0;
                    }
                }
                else
                {
                    MacDot16ExtendedorARQEnableFragSubHeader*
                        arqFragSubHeader = NULL;
                    arqFragSubHeader =
                        (MacDot16ExtendedorARQEnableFragSubHeader*)
                        payload;
                    pduSize -= sizeof(
                        MacDot16ExtendedorARQEnableFragSubHeader);
                    UInt16 numARQBlocks = 0;
                    MacDot16ARQCalculateNumBlocks(numARQBlocks,
                        (unsigned int)pduSize);
                    MacDot16ARQSetRetryAndBlockLifetimeTimer
                                                (node,
                                                 dot16,
                                                 sFlow,
                                                 MESSAGE_ReturnInfo(msg),
                                                 numARQBlocks,
                                                 MacDot16Get11bit(
                                                 arqFragSubHeader));
                    MESSAGE_RemoveInfo(node, msg, INFO_TYPE_DEFAULT);
                }
            }
            // data packets, increase the statistics
        }//end of if (MacDot16MacHeaderGetHT(macHeader)
        msg = nextPdu;
    }
}

// /**
// FUNCTION   :: MacDot16SsTransmitUlBurst
// LAYER      :: MAC
// PURPOSE    :: Schedule and transmit during my UL burst
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + burst     : MacDot16UlBurst : Description of a UL burst
// RETURN     :: void : NULL
// **/
static
void MacDot16SsTransmitUlBurst(Node* node,
                               MacDataDot16* dot16,
                               MacDot16UlBurst* burst)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    Dot16BurstInfo burstInfo;
    int bitsPerPs;
    int burstSize;
    int msgSize;
    Message* firstMsg = NULL;
    Message* lastMsg = NULL;
    int i;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("start transmitting a UL burst with duration = %d\n",
               burst->duration);
    }

    if ((dot16Ss->rngType == DOT16_CDMA ||
        dot16Ss->bwReqType == DOT16_BWReq_CDMA) &&
        burst != NULL &&
        burst->uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE &&
        burst->isCdmaAllocationIEReceived == FALSE )
    {
        return;
    }
    // update the service flow empty time
    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            if (sFlow->isEmpty == FALSE &&
                dot16Ss->outScheduler[i]->isEmpty(sFlow->queuePriority))
            {
                sFlow->emptyBeginTime = node->getNodeTime();
                sFlow->isEmpty = TRUE;
            }
            sFlow = sFlow->next;
        }
    }

    bitsPerPs = MacDot16PhyBitsPerPs(
                    node,
                    dot16,
                    &(dot16Ss->servingBs->ulBurstProfile[burst->uiuc - 1]),
                    DOT16_UL);

    burstSize = (burst->duration - dot16Ss->para.sstgInPs) * bitsPerPs / 8;
    msgSize = 0;

    // get the list of scheduled messages
    if (dot16Ss->outBuffHead != NULL &&
        MESSAGE_ReturnPacketSize(dot16Ss->outBuffHead) <= burstSize)
    {
        firstMsg = dot16Ss->outBuffHead;
        lastMsg = firstMsg;
    }

    if (firstMsg != NULL)
    {
        if (burstSize <= dot16Ss->ulBytesAllocated)
        {
            MacDot16MacHeader* macHeader;
            int pduLength;
            Message* prevMsg = NULL;
            lastMsg = firstMsg;
            msgSize = 0;
            BOOL flag = FALSE;
            while ((burstSize > msgSize) && (lastMsg != NULL))
            {
                macHeader = (MacDot16MacHeader*)
                    MESSAGE_ReturnPacket(lastMsg);
                pduLength = MacDot16MacHeaderGetLEN(macHeader);
                if (flag && (burstSize < (msgSize + pduLength)))
                {
                    break;
                }
                msgSize += pduLength;
                flag = TRUE;
                while (pduLength > 0)
                {
                    prevMsg = lastMsg;
                    pduLength -= MESSAGE_ReturnPacketSize(lastMsg);
                    lastMsg = lastMsg->next;
                }
            }

            if (prevMsg != NULL)
            {
                dot16Ss->outBuffHead = lastMsg;
                lastMsg = prevMsg;
                if (dot16Ss->outBuffHead == NULL)
                {
                    dot16Ss->outBuffTail = NULL;
                }
                lastMsg->next = NULL;
                dot16Ss->ulBytesAllocated -= burstSize;
            }
            else
            {
                firstMsg = NULL;
            }
        }
        else
        {
            msgSize = dot16Ss->ulBytesAllocated;
            lastMsg = dot16Ss->outBuffTail;

            // reset scheduling variables
            dot16Ss->outBuffHead = NULL;
            dot16Ss->outBuffTail = NULL;
            dot16Ss->ulBytesAllocated = 0;
        }
    }

    // if some space left in the burst, use it to send bandwidth request
    if (burstSize > msgSize)
    {
        Message* bwMsgList;
        int bwMsgSize;

        bwMsgList = MacDot16SsScheduleBandwidthRequest(
                        node,
                        dot16,
                        burstSize - msgSize,
                        FALSE,
                        &bwMsgSize);

        if (firstMsg == NULL)
        {
            // no data scheduled before
            firstMsg = bwMsgList;
        }
        else
        {
            lastMsg->next = bwMsgList;
        }
        msgSize += bwMsgSize;
    }

    if (firstMsg != NULL)
    {
        int durationInPs;

        // check the list of outgoing msgs for updating some variables
        MacDot16SsCheckOutgoingMsg(node, dot16, firstMsg);

        // add the burst information for PHY
        durationInPs = MacDot16SsBuildBurstInfo(
                           node,
                           dot16,
                           burst,
                           msgSize,
                           &burstInfo);
        MacDot16AddBurstInfo(node, firstMsg, &burstInfo);

        // start transmitting the burst on the UL channel
        MacDot16PhyTransmitUlBurst(node,
                                   dot16,
                                   firstMsg,
                                   dot16Ss->servingBs->ulChannel,
                                   durationInPs);
    }
}

// /**
// FUNCTION   :: MacDot16SsFreeServiceFlowList
// LAYER      :: MAC
// PURPOSE    :: Free all service flows
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16SsFreeServiceFlowList(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    MacDot16ServiceFlow* tmpFlow;
    int i;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("free service flow list\n");
    }

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            // invalid the classifer at CS
            MacDot16CsInvalidClassifier(node, dot16, sFlow->csfId);

            // free mem in dsxInfo
            // dsa
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dsaInfo));
            // dsc
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dscInfo));
            // dsd
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dsdInfo));

            // remove the corresponding queue from scheduler

            dot16Ss->outScheduler[i]->removeQueue(sFlow->queuePriority);

            // free tmp queue
            MESSAGE_FreeList(node, sFlow->tmpMsgQueueHead);

            // free flow
            tmpFlow = sFlow;
            sFlow = sFlow->next;
            MEM_free(tmpFlow);
        }

        memset(&(dot16Ss->ulFlowList[i]),
               0,
               sizeof(MacDot16ServiceFlowList));

        sFlow = dot16Ss->dlFlowList[i].flowHead;
        while (sFlow != NULL)
        {
            // free mem in dsxInfo
            // dsa
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dsaInfo));
            // dsc
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dscInfo));
            // dsd
            MacDot16ResetDsxInfo(node,
                                 dot16,
                                 sFlow,
                                 &(sFlow->dsdInfo));

            // free tmp queue
            MESSAGE_FreeList(node, sFlow->tmpMsgQueueHead);

            // free flow
            tmpFlow = sFlow;
            sFlow = sFlow->next;
            MEM_free(tmpFlow);
        }

        memset(&(dot16Ss->dlFlowList[i]),
               0,
               sizeof(MacDot16ServiceFlowList));
    }
}

// /**
// FUNCTION   :: MacDot16SsTimeoutServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Go through the service flow list and delete old flows
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16SsTimeoutServiceFlow(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    clocktype currentTime;
    int i;

    currentTime = node->getNodeTime();

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = dot16Ss->ulFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            if (sFlow->isEmpty == TRUE &&
                dot16Ss->outScheduler[i]->isEmpty(sFlow->queuePriority) &&
                (currentTime - sFlow->emptyBeginTime) >
                    dot16Ss->para.flowTimeout &&
                ((sFlow->status == DOT16_FLOW_Nominal)))

            {
                if (!(sFlow->isARQEnabled && sFlow->arqControlBlock != NULL))
                {
                    MacDot16SsDeleteServiceFlow(node, dot16, sFlow);
                }
            }
            sFlow = sFlow->next;
        }
    }
}

// /**
// FUNCTION   :: MacDot16SsFreeQueuedMsgInMgmtScheduler
// LAYER      :: MAC
// PURPOSE    :: Free the queued control msg in mgmt scheudler
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
static
void MacDot16SsFreeQueuedMsgInMgmtScheduler(Node* node,
                                            MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    Scheduler* scheduler;
    Message* nextPkt = NULL;
    int priority;
    BOOL notEmpty = TRUE;
    clocktype currentTime;

    scheduler = dot16Ss->mgmtScheduler;
    currentTime = node->getNodeTime();

    while (notEmpty)
    {
        // peek next packet
        notEmpty = scheduler->retrieve(ALL_PRIORITIES,
                                       0,
                                       &nextPkt,
                                       &priority,
                                       PEEK_AT_NEXT_PACKET,
                                       currentTime);

        if (notEmpty)
        {
            scheduler->retrieve(ALL_PRIORITIES,
                                0,
                                &nextPkt,
                                &priority,
                                DEQUEUE_PACKET,
                                currentTime);
            ERROR_Assert(nextPkt != NULL,
                    "Free a mgmt queue msg is NULL\n");
            MESSAGE_Free(node, nextPkt);
        }
    }
}

// /**
// FUNCTION   :: MacDot16SsHandoverInNbrScan
// LAYER      :: MAC
// PURPOSE    :: Perform handover while SS is in scan mode if needed
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + targetBs  : MacDot16SsBsInfo* : Pointer to the target BS
// RETURN     :: void : NULL
// **/
static
void MacDot16SsHandoverInNbrScan(Node* node,
                                 MacDataDot16* dot16,
                                 MacDot16SsBsInfo* targetBs)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    Message* msHoReq;

    // reset the scan status

    dot16Ss->startFrames = 0;
    dot16Ss->numInterFrames = 0;
    dot16Ss->numIterations = 0;
    dot16Ss->numScanFrames = 0;
    dot16Ss->scanDuration = 0;
    dot16Ss->scanInterval = 0;

    if (dot16Ss->needReportScanResult)
    {
        dot16Ss->needReportScanResult = FALSE;
    }


    dot16Ss->targetBs = targetBs;

    msHoReq = MacDot16eSsBuildMobMshoReqMsg(node,
                                            dot16);
    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_BASIC,
                             msHoReq);

    dot16Ss->hoStatus = DOT16e_SS_HO_LOCAL_Begin;
    dot16Ss->hoNumRetry = DOT16e_MAX_HoReqRetry;

}

// /**
// FUNCTION   :: MacDot16SsUpdateBsMeasurement
// LAYER      :: MAC
// PURPOSE    :: Update measurement of BS signal quality
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + signalMea : PhySignalMeasurement* : singal meaurement info
// RETURN     :: void : NULL
// **/
static
void MacDot16SsUpdateBsMeasurement(Node* node,
                                   MacDataDot16* dot16,
                                   PhySignalMeasurement* signalMea)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SignalMeasurementInfo meanMeas;

    int moduCodeType = PHY802_16_QPSK_R_1_2;

    if (dot16Ss->rngType == DOT16_CDMA)
    {
        moduCodeType = PHY802_16_BPSK;
    }
    if (signalMea == NULL)
    {
        // no measurement data, do nothing
        return;
    }

     if (dot16->dot16eEnabled &&
        dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_InScan &&
        dot16Ss->operational)
    {
        // update signal measurement of neighbor BS
        if (dot16Ss->targetBs != NULL)
        {
            // update the measurement histroy and return the mean
            MacDot16UpdateMeanMeasurement(node,
                                          dot16,
                                          &(dot16Ss->targetBs->measInfo[0]),
                                          signalMea,
                                          &meanMeas);

            dot16Ss->targetBs->cinrMean = meanMeas.cinr;
            dot16Ss->targetBs->rssMean = meanMeas.rssi;
            dot16Ss->targetBs->lastMeaTime = node->getNodeTime();

            if (node->nodeId == DEBUG_SPECIFIC_NODE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("in scan mode, CINR to scanned BS %d:%d:%d:%d:%d:%d "
                       "is %f, RSSI to scanned BS %f, Rx_Power %f\n",
                        dot16Ss->targetBs->bsId[0],
                        dot16Ss->targetBs->bsId[1],
                        dot16Ss->targetBs->bsId[2],
                        dot16Ss->targetBs->bsId[3],
                        dot16Ss->targetBs->bsId[4],
                        dot16Ss->targetBs->bsId[5],
                        dot16Ss->targetBs->cinrMean,
                        dot16Ss->targetBs->rssMean, signalMea->rss);
            }

            if (dot16Ss->targetBs->rssMean > dot16Ss->servingBs->rssMean &&
                dot16Ss->servingBs->rssMean <
                IN_DB(PhyDot16GetRxSensitivity_mW(
                          node,
                          dot16->myMacData->phyNumber,
                          moduCodeType)))
            {
                // if during scan signal to servign BS is too weak
                // select a nbr BS to perform handover
                MacDot16SsBsInfo* targetBs;

                targetBs = MacDot16eSsSelectBestNbrBs(node, dot16);
                if (targetBs)
                {
                    if (DEBUG_HO)
                    {
                        printf("Node%d in scan mode, but current BS "
                               "has too weak signal and starts handover"
                               "to BS with DL channel = %d UL channel %d\n",
                               node->nodeId, targetBs->dlChannel,
                               targetBs->ulChannel);
                    }
                    MacDot16SsHandoverInNbrScan(node, dot16, targetBs);
                }

            }
        }
    }
    else
    {
        if (dot16Ss->initInfo.dlSynchronized)
        {
            MacDot16UpdateMeanMeasurement(
                                    node,
                                    dot16,
                                    &(dot16Ss->servingBs->measInfo[0]),
                                    signalMea,
                                    &meanMeas);

            dot16Ss->servingBs->cinrMean = meanMeas.cinr;
            dot16Ss->servingBs->rssMean = meanMeas.rssi;
            dot16Ss->servingBs->lastMeaTime = node->getNodeTime();

            // update the dynamic statistics
            if (node->guiOption)
            {
                GUI_SendRealData(node->nodeId,
                                 dot16Ss->stats.dlCinrGuiId,
                                 signalMea->cinr,
                                 node->getNodeTime());

                GUI_SendRealData(node->nodeId,
                                 dot16Ss->stats.dlRssiGuiId,
                                 NON_DB(signalMea->rss),
                                 node->getNodeTime());
            }

            if (node->nodeId == DEBUG_SPECIFIC_NODE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("CINR to serving BS %d:%d:%d:%d:%d:%d is %f,"
                       "RSSI to serving BS %f, Rx_Power %f\n",
                        dot16Ss->servingBs->bsId[0],
                        dot16Ss->servingBs->bsId[1],
                        dot16Ss->servingBs->bsId[2],
                        dot16Ss->servingBs->bsId[3],
                        dot16Ss->servingBs->bsId[4],
                        dot16Ss->servingBs->bsId[5],
                        dot16Ss->servingBs->cinrMean,
                        dot16Ss->servingBs->rssMean, signalMea->rss);
                MacDot16PrintModulationCodingInfo(node, dot16);
                if (0)
                {
                    Coordinates msLocation;
                    MOBILITY_ReturnCoordinates(node, &msLocation);
                    printf("SS loc %f %f\n",
                           msLocation.common.c1,
                           msLocation.common.c2);
                }
            }
        }
        // only when SS has already finish the intitialization with
        // serving BS could it start the scan and handover procedure
        if (dot16->dot16eEnabled && dot16Ss->operational &&
            dot16->numChannels > 1)
        {
            // check if need to trigger handover
            if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_None &&
                dot16Ss->hoStatus == DOT16e_SS_HO_None)
            {
                if (dot16Ss->servingBs->rssMean <
                    dot16Ss->para.hoRssTrigger)
                {
                    // need to init handover
                    if (dot16Ss->nbrBsList != NULL)
                    {
                        // select a nbr BS to perform handover
                        MacDot16SsBsInfo* targetBs;
                        targetBs = MacDot16eSsSelectBestNbrBs(node, dot16);
                        if (targetBs != NULL &&
                           (targetBs->rssMean - dot16Ss->servingBs->rssMean)
                            >= dot16Ss->para.hoRssMargin)
                        {
                            // find a suitable nbr BS, do handover
                            if (DEBUG_HO)
                            {
                                printf("Node%d starts handover to BS with "
                                       "DL channel = %d UL channel %d\n",
                                       node->nodeId, targetBs->dlChannel,
                                       targetBs->ulChannel);
                            }

                            dot16Ss->targetBs = targetBs;
                            Message* msHoReq;
                            msHoReq = MacDot16eSsBuildMobMshoReqMsg(node,
                                                                    dot16);
                            MacDot16SsEnqueueMgmtMsg(node,
                                                     dot16,
                                                     DOT16_CID_BASIC,
                                                     msHoReq);

                            dot16Ss->hoStatus = DOT16e_SS_HO_LOCAL_Begin;
                            dot16Ss->hoNumRetry = DOT16e_MAX_HoReqRetry;
                            // set timer and the retry counter when
                            // check outgoing
                        }
                        else if (targetBs != NULL &&
                                (targetBs->rssMean >
                                dot16Ss->servingBs->rssMean)
                                && dot16Ss->servingBs->rssMean <
                                IN_DB(PhyDot16GetRxSensitivity_mW(
                                          node,
                                          dot16->myMacData->phyNumber,
                                          moduCodeType)))
                        {
                            // if the serving BS signal is too weak and
                            // target BS has stronger signal, handover to
                            // target BS though the difference is not over
                            // the handover margin to avoid call dropping
                            if (DEBUG_HO)
                            {
                                printf("Node%d, current BS has too weak "
                                       "signal, starts handover to BS with "
                                       "DL channel = %d UL channel %d, "
                                       "though the signal diff. is "
                                       "relatively small\n",
                                       node->nodeId, targetBs->dlChannel,
                                       targetBs->ulChannel);
                            }

                            dot16Ss->targetBs = targetBs;
                            Message* msHoReq;
                            msHoReq = MacDot16eSsBuildMobMshoReqMsg(node,
                                                                    dot16);
                            MacDot16SsEnqueueMgmtMsg(node,
                                                     dot16,
                                                     DOT16_CID_BASIC,
                                                     msHoReq);
                            dot16Ss->hoStatus = DOT16e_SS_HO_LOCAL_Begin;
                            dot16Ss->hoNumRetry = DOT16e_MAX_HoReqRetry;
                            // set timer and the retry counter when check
                            // outgoing
                        }
                    }
                    else
                    {
                        // no nbr BS in the list, restart
                    }
                }
            }

            // if not in HO or neighbor scan mode, and if RSS is below
            // the trigger, needs to start neighbor BS scan
            if (dot16Ss->hoStatus == DOT16e_SS_HO_None &&
                dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_None &&
                dot16Ss->servingBs->trigger.triggerType ==
                DOT16e_TRIGEGER_METRIC_TYPE_RSSI)
            {

                // for RSSI case
                if (dot16Ss->servingBs->trigger.triggerFunc ==
                    DOT16e_TRIGEGER_METRIC_FUNC_SERV_LESS_ABSO &&
                    dot16Ss->servingBs->rssMean <
                    dot16Ss->servingBs->trigger.triggerValue &&
                    (dot16Ss->lastScanTime + dot16Ss->para.nbrScanMinGap) <=
                    node->getNodeTime())
                {// less than threshold
                    if (dot16Ss->servingBs->trigger.triggerAction ==
                        DOT16e_TRIGGER_ACTION_MOB_SCN_REQ)
                    {
                        // needs to scan NBR BSs
                        if (DEBUG_NBR_SCAN)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("inits neighbor BS scan\n");
                        }

                        // need to start scanning by tx a req to serving BS
                        Message* mgmtMsg;
                        mgmtMsg =
                            MacDot16eSsBuildMobScnReqMsg(node, dot16, NULL);
                        MacDot16SsEnqueueMgmtMsg(node,
                                                 dot16,
                                                 DOT16_CID_BASIC,
                                                 mgmtMsg);
                        dot16Ss->nbrScanStatus =
                            DOT16e_SS_NBR_SCAN_WaitScnReq;
                        dot16Ss->scanReqRetry = DOT16e_MAX_ScanReqRetry;
                    }
                }
            }
        }
    }
}

// /**
// FUNCTION   :: MacDot16eSsUpdateNbrScan
// LAYER      :: MAC
// PURPOSE    :: Update the neighbor scanning status
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsUpdateNbrScan(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (DEBUG_NBR_SCAN_DETAIL &&
        dot16Ss->nbrScanStatus >= DOT16e_SS_NBR_SCAN_Started)
    {
        printf("Node%d update the neighbor scan status.\n", node->nodeId);
        printf("    scan status = %d\n", dot16Ss->nbrScanStatus);
        printf("    scan channel index = %d\n",
               dot16->channelList[dot16Ss->nbrChScanIndex]);
        printf("    scan duration = %d\n", dot16Ss->scanDuration);
        printf("    interleave interval = %d\n", dot16Ss->scanInterval);
        printf("    # of start frames left = %d\n", dot16Ss->startFrames);
        printf("    # of scan frames left = %d\n", dot16Ss->numScanFrames);
        printf("    # of interleave frames left = %d\n",
               dot16Ss->numInterFrames);
        printf("    # of iterations left = %d\n", dot16Ss->numIterations);
    }

    if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_Started)
    {
        // just started, count down start frames first
        if (dot16Ss->startFrames == 0)
        {
            // switch to scan mode
            dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_InScan;
            dot16Ss->nbrChScanIndex = (dot16Ss->chScanIndex + 1) %
                                      dot16->numChannels;
            dot16Ss->numScanFrames = dot16Ss->scanDuration;
            dot16Ss->numInterFrames = 0;

            MacDot16StartListening(
                node,
                dot16,
                dot16->channelList[dot16Ss->nbrChScanIndex]);

            if (DEBUG_NBR_SCAN)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("start a new scan interval by listen to nbr dl"
                       " ch %d\n",
                       dot16->channelList[dot16Ss->nbrChScanIndex]);
            }

        }
        else
        {
            dot16Ss->startFrames --;
        }
    }
    else if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_InScan)
    {
        // in scan mode, count down scan frames
        dot16Ss->numScanFrames --;
        if (dot16Ss->numScanFrames > 0)
        {
            // move to scan next channel
            dot16Ss->nbrChScanIndex = (dot16Ss->nbrChScanIndex + 1) %
                                      dot16->numChannels;
            if (dot16Ss->nbrChScanIndex == dot16Ss->chScanIndex)
            {
                // skip DL channel of serving BS
                dot16Ss->nbrChScanIndex = (dot16Ss->nbrChScanIndex + 1) %
                                          dot16->numChannels;
            }

            MacDot16StartListening(
                node,
                dot16,
                dot16->channelList[dot16Ss->nbrChScanIndex]);
        }
        else
        {
            dot16Ss->numIterations --;
            if (dot16Ss->numIterations > 0)
            {
                // current scan period finished, switch to interleaved
                // frames
                dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_Interleave;
                dot16Ss->numInterFrames = dot16Ss->scanInterval;
                dot16Ss->numScanFrames = 0;
            }
            else
            {
                // finished the nbr scan intervals, quit scan mode
                dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;
                dot16Ss->lastScanTime = node->getNodeTime();

                // to see if scan report is needed
                if (dot16Ss->needReportScanResult &&
                    dot16Ss->scanReportMode == DOT16e_SCAN_EventTrigReport)
                {
                    Message* mgmtMsg;

                    // build the MOB-SCN-REP msg
                    // schedule the transmission
                    mgmtMsg = MacDot16eSsBuildMobScnRepMsg(
                                  node,
                                  dot16,
                                  dot16Ss->scanReportMode,
                                  dot16Ss->scanReportMetric);

                    MacDot16SsEnqueueMgmtMsg(node,
                                             dot16,
                                             DOT16_CID_PRIMARY,
                                             mgmtMsg);
                    // update stat
                    dot16Ss->stats.numNbrScanRepSent ++;

                    dot16Ss->needReportScanResult = FALSE;

                    if (DEBUG_NBR_SCAN)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("send scan report to BS\n");
                    }
                }
            }

            // listen to DL channel of serving BS
            MacDot16StartListening(node,
                                   dot16,
                                   dot16Ss->servingBs->dlChannel);

            if (DEBUG_NBR_SCAN)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("either one scan interval/overall scan finshed,"
                       "listen to serving BS dl ch %d\n",
                       dot16Ss->servingBs->dlChannel);
            }
        }
    }
    else if (dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_Interleave)
    {
        // in interleaved frames, listen to serving BS
        dot16Ss->numInterFrames --;
        if (dot16Ss->numInterFrames > 0)
        {
            // still in interleaving frames, listen to serving BS
            MacDot16StartListening(node,
                                   dot16,
                                   dot16Ss->servingBs->dlChannel);
        }
        else
        {
            // switch to scan  mode
            dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_InScan;
            dot16Ss->numScanFrames = dot16Ss->scanDuration;

            // scan next channel
            dot16Ss->nbrChScanIndex = (dot16Ss->nbrChScanIndex + 1) %
                                      dot16->numChannels;
            if (dot16Ss->nbrChScanIndex == dot16Ss->chScanIndex)
            {
                // skip DL channel of serving BS
                dot16Ss->nbrChScanIndex = (dot16Ss->nbrChScanIndex + 1) %
                                          dot16->numChannels;
            }

            // listen to the scanning channel
            MacDot16StartListening(
                node,
                dot16,
                dot16->channelList[dot16Ss->nbrChScanIndex]);

            if (DEBUG_NBR_SCAN)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("interval finshed, start a new scan interval"
                       "listen to nbr BS dl ch %d\n",
                       dot16->channelList[dot16Ss->nbrChScanIndex]);
            }
        }
    }
    else
    {
        // not in scan mode, listening to serving BS
        MacDot16StartListening(node, dot16, dot16Ss->servingBs->dlChannel);
    }
}

// /**
// FUNCTION   :: MacDot16SsRestartOperation
// LAYER      :: MAC
// PURPOSE    :: Re-start operation of the SS.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16SsRestartOperation(Node* node,
                                MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    int i;

    if (DEBUG || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("*****restart the operation****\n");
    }

    // stop listening to any channel at the begining until operation start
    for (i = 0; i < dot16->numChannels; i ++)
    {
        MacDot16StopListening(node, dot16, dot16->channelList[i]);
    }

    // clean existing stats, variables, buffers, queues, etc.

    // reset some variables of serving BS structure
    dot16Ss->servingBs->dcdCount = 0;
    dot16Ss->servingBs->ucdCount = 0;
    dot16Ss->servingBs->rangeRetries = 0;
    dot16Ss->servingBs->rangeBOCount = 0;
    dot16Ss->servingBs->rangeBOValue = 0;
    dot16Ss->servingBs->requestRetries = 0;
    dot16Ss->servingBs->requestBOCount = 0;
    dot16Ss->servingBs->requestBOValue = 0;
    dot16Ss->servingBs->regRetries = 0;
    dot16Ss->servingBs->cinrMean = 0.0;
    dot16Ss->servingBs->rssMean = 0.0;

    // clean bandwidth situation
    dot16Ss->isWaitBwGrant = FALSE;
    dot16Ss->mgmtBWRequested = 0;
    dot16Ss->mgmtBWGranted = 0;
    dot16Ss->aggBWRequested = 0;
    dot16Ss->aggBWGranted = 0;

    while (dot16Ss->bwReqHead != NULL)
    {
        bwReqRecord = dot16Ss->bwReqHead->next;
        MEM_free(dot16Ss->bwReqHead);
        dot16Ss->bwReqHead = bwReqRecord;
    }
    dot16Ss->bwReqHead = NULL;
    dot16Ss->bwReqTail = NULL;

    // clean scheduling variables
    if (dot16Ss->ulBurstHead != NULL)
    {
        MacDot16UlBurst* ulBurst;
        MacDot16UlBurst* tmpUlBurst;

        ulBurst = dot16Ss->ulBurstHead;
        while (ulBurst != NULL)
        {
            tmpUlBurst = ulBurst->next;
            MEM_free(ulBurst);
            ulBurst = tmpUlBurst;
        }

        dot16Ss->ulBurstHead = NULL;
        dot16Ss->ulBurstTail = NULL;
    }

    // clean neighbor scanning or handover status
    dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;
    dot16Ss->hoStatus = DOT16e_SS_HO_None;
    dot16Ss->targetBs = NULL;

    if (dot16Ss->nbrBsList != NULL)
    {
        MacDot16SsBsInfo* bsInfo;
        MacDot16SsBsInfo* tmpBsInfo;

        bsInfo = dot16Ss->nbrBsList;
        while (bsInfo != NULL)
        {
            tmpBsInfo = bsInfo;
            bsInfo = bsInfo->next;
            MEM_free(tmpBsInfo);
        }

        dot16Ss->nbrBsList = NULL;

        if (node->guiOption)
        {
            dot16Ss->stats.numNbrBsScanned = 0;
            GUI_SendIntegerData(node->nodeId,
                                dot16Ss->stats.numNbrBsGuiId,
                                dot16Ss->stats.numNbrBsScanned,
                                node->getNodeTime());
        }
    }

    // clean service flows
    MacDot16SsFreeServiceFlowList(node, dot16);

    // clean the mgmt scheduler
    MacDot16SsFreeQueuedMsgInMgmtScheduler(node, dot16);

    // clean the contents in ourBuffer
    MESSAGE_FreeList(node, dot16Ss->outBuffHead);
    dot16Ss->outBuffHead = NULL;
    dot16Ss->outBuffTail = NULL;

    // clean cached timers
    if (dot16Ss->timers.timerT1 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT1);
    }
    if (dot16Ss->timers.timerT2 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT2);
    }
    if (dot16Ss->timers.timerT3 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
    }
    if (dot16Ss->timers.timerT4 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT4);
    }

    if (dot16Ss->timers.timerT6 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT6);
    }
    if (dot16Ss->timers.timerT12 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
    }
    if (dot16Ss->timers.timerT16 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT16);
    }

    if (dot16Ss->timers.timerT18 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT18);
    }
    if (dot16Ss->timers.timerT20 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT20);
    }
    if (dot16Ss->timers.timerT21 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT21);
    }
    if (dot16Ss->timers.timerLostDl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostDl);
    }
    if (dot16Ss->timers.timerLostUl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostUl);
    }
    if (dot16Ss->timers.frameEnd != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.frameEnd);
    }

    if (dot16Ss->timers.timerT41 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT41);
    }
    if (dot16Ss->timers.timerT42 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT42);
    }
    if (dot16Ss->timers.timerT44 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT44);
    }

    if (dot16Ss->timers.timerT29 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT29);
    }
    memset((char*) &(dot16Ss->timers), 0, sizeof(dot16Ss->timers));

    // Now, restart the operation from beginning (network entry)
    dot16Ss->servingBs->dlChannel =
                    dot16->channelList[dot16Ss->chScanIndex];
    if (dot16->duplexMode == DOT16_TDD)
    {
        dot16Ss->servingBs->ulChannel = dot16Ss->servingBs->dlChannel;
    }

    MacDot16SsPerformNetworkEntry(node, dot16);
}

// /**
// FUNCTION   :: MacDot16SsStartOperation
// LAYER      :: MAC
// PURPOSE    :: Start operation of the SS.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16SsStartOperation(Node* node,
                              MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = NULL;

    dot16Ss = (MacDot16Ss*) dot16->ssData;
    ERROR_Assert(dot16Ss != NULL, "MAC 802.16: SS node was not inited\n");

    if (DEBUG || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("*****start operation!*****\n");
    }

    MacDot16SsPerformNetworkEntry(node, dot16);
}

// /**
// FUNCTION   :: MacDot16SsFastRestartInHandover
// LAYER      :: MAC
// PURPOSE    :: Re-start operation of the SS in handover.
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 SS data struct
// RETURN     :: void      : NULL
// **/
static
void MacDot16SsFastRestartInHandover(Node* node,
                                     MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16SsBWReqRecord* bwReqRecord;
    int i;

    if (DEBUG || DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("*****have a fast restart operation in handover****\n");
    }

    // stop listening to any channel at the begining until operation start
    for (i = 0; i < dot16->numChannels; i ++)
    {
        MacDot16StopListening(node, dot16, dot16->channelList[i]);
    }

    // clean existing stats, variables, buffers, queues, etc.

    if (dot16Ss->targetBs)
    {
        // reset some variables of target BS structure
        dot16Ss->targetBs->rangeRetries = dot16Ss->servingBs->rangeRetries;
        dot16Ss->targetBs->rangeBOCount = dot16Ss->servingBs->rangeBOCount;
        dot16Ss->targetBs->rangeBOValue = 0;
        dot16Ss->targetBs->requestRetries = 0;
        dot16Ss->targetBs->requestBOCount = 0;
        dot16Ss->targetBs->requestBOValue = 0;
        dot16Ss->targetBs->regRetries = 0;
    }

    // clean bandwidth situation
    dot16Ss->isWaitBwGrant = FALSE;
    dot16Ss->mgmtBWRequested = 0;
    dot16Ss->mgmtBWGranted = 0;
    dot16Ss->aggBWRequested = 0;
    dot16Ss->aggBWGranted = 0;

    dot16Ss->bwRequestedForOutBuffer = 0;

    while (dot16Ss->bwReqHead != NULL)
    {
        bwReqRecord = dot16Ss->bwReqHead->next;
        MEM_free(dot16Ss->bwReqHead);
        dot16Ss->bwReqHead = bwReqRecord;
    }
    dot16Ss->bwReqHead = NULL;
    dot16Ss->bwReqTail = NULL;

    // clean scheduling variables
    if (dot16Ss->ulBurstHead != NULL)
    {
        MacDot16UlBurst* ulBurst;
        MacDot16UlBurst* tmpUlBurst;

        ulBurst = dot16Ss->ulBurstHead;
        while (ulBurst != NULL)
        {
            tmpUlBurst = ulBurst->next;
            MEM_free(ulBurst);
            ulBurst = tmpUlBurst;
        }

        dot16Ss->ulBurstHead = NULL;
        dot16Ss->ulBurstTail = NULL;
    }

    // clean scanning or handover status
    dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;

    // clean service flows
    MacDot16SsFreeServiceFlowList(node, dot16);

    // clean the mgmt scheduler
    MacDot16SsFreeQueuedMsgInMgmtScheduler(node, dot16);

    // clean the contents in ourBuffer
    MESSAGE_FreeList(node, dot16Ss->outBuffHead);
    dot16Ss->outBuffHead = NULL;
    dot16Ss->outBuffTail = NULL;

    // clean cached timers
    if (dot16Ss->timers.timerT1 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT1);
    }
    if (dot16Ss->timers.timerT2 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT2);
    }
    if (dot16Ss->timers.timerT3 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT3);
    }
    if (dot16Ss->timers.timerT4 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT4);
    }

    if (dot16Ss->timers.timerT6 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT6);
    }
    if (dot16Ss->timers.timerT12 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
    }
    if (dot16Ss->timers.timerT16 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT16);
    }

    if (dot16Ss->timers.timerT18 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT18);
    }
    if (dot16Ss->timers.timerT20 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT20);
    }
    if (dot16Ss->timers.timerT21 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT21);
    }

    if (dot16Ss->timers.timerLostDl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostDl);
    }
    if (dot16Ss->timers.timerLostUl != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerLostUl);
    }
    if (dot16Ss->timers.frameEnd != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.frameEnd);
    }

    if (dot16Ss->timers.timerT41 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT41);
    }
    if (dot16Ss->timers.timerT42 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT42);
    }
    if (dot16Ss->timers.timerT44 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT44);
    }

    if (dot16Ss->timers.timerT29 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT29);
    }
    memset((char*) &(dot16Ss->timers), 0, sizeof(dot16Ss->timers));

    dot16Ss->operational = FALSE;
    memset((char*) &(dot16Ss->initInfo), 0, sizeof(MacDot16SsInitInfo));

    // assume  target BS has the same profile as serving BS
    if (dot16Ss->targetBs)
    {
        memcpy((char*) &(dot16Ss->targetBs->dlBurstProfile[0]),
                (char*) &(dot16Ss->servingBs->dlBurstProfile[0]),
                sizeof(MacDot16DlBurstProfile) *
               dot16Ss->numOfDLBurstProfileInUsed);

        memcpy((char*) &(dot16Ss->targetBs->ulBurstProfile[0]),
                (char*) &(dot16Ss->servingBs->ulBurstProfile[0]),
                sizeof(MacDot16UlBurstProfile) *
               dot16Ss->numOfULBurstProfileInUsed);

        dot16Ss->targetBs->frameDuration =
                        dot16Ss->servingBs->frameDuration;

        dot16Ss->initInfo.dlSynchronized = TRUE;
        dot16Ss->initInfo.ulParamAccquired = TRUE;
        dot16Ss->initInfo.initStatus = DOT16_SS_INIT_RangingAutoAdjust;
        if (dot16Ss->rngType == DOT16_CDMA)
        {
            dot16Ss->initInfo.rngState = DOT16_SS_RNG_WaitBcstInitRngOpp;
        }
        else
        {
            dot16Ss->initInfo.rngState = DOT16_SS_RNG_WaitBcstRngOpp;
        }

        // start a timeout timer T12 for UCD
        if (dot16Ss->timers.timerT12 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT12);
            dot16Ss->timers.timerT12 = NULL;
        }
        dot16Ss->timers.timerT12 = MacDot16SetTimer(
                                       node,
                                       dot16,
                                       DOT16_TIMER_T12,
                                       dot16Ss->para.t12Interval,
                                       NULL,
                                       0);
    }
}

// /**
// FUNCTION   :: MacDot16eSsFinishHandover
// LAYER      :: MAC
// PURPOSE    :: Finish the handover procedure. Basically switch target BS
//               as the serving BS.
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsFinishHandover(Node* node, MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    MacDot16SsFastRestartInHandover(node, dot16);
    MacDot16CopyStationId(dot16Ss->prevServBsId, dot16Ss->servingBs->bsId);

    // firstly, put current serving BS to nbr BS list
    dot16Ss->servingBs->next = dot16Ss->nbrBsList;
    dot16Ss->nbrBsList  = dot16Ss->servingBs;

    if (node->guiOption)
    {
        dot16Ss->stats.numNbrBsScanned ++;
        GUI_SendIntegerData(node->nodeId,
                            dot16Ss->stats.numNbrBsGuiId,
                            dot16Ss->stats.numNbrBsScanned,
                            node->getNodeTime());
    }

    // put target BS as the new serving Bs
    dot16Ss->servingBs = dot16Ss->targetBs;
    //dot16Ss->servingBs->uiuc = DOT16_UIUC_MOST_RELIABLE;

    dot16Ss->targetBs = NULL;

    dot16Ss->chScanIndex = MacDot16GetIndexInChannelList(
                               node,
                               dot16,
                               dot16Ss->servingBs->dlChannel);

    MacDot16eSsRemoveNbrBsByBsId(node,
                                 dot16,
                                 dot16Ss->servingBs->bsId);

    dot16Ss->hoStatus = DOT16e_SS_HO_HoOngoing;

    dot16Ss->stats.numHandover ++;
}
//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsAddServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Add a new service flow
// PARAMETERS ::
// + node        : Node*                        : Pointer to node
// + dot16       : MacDataDot16*                : Pointer to DOT16 structure
// + serviceType : MacDot16ServiceType          : Service type of the flow
// + csfId       : unsigned int                 : Classifier ID
// + qosInfo     : MacDot16QoSParameter*        : QoS parameter of the flow
// + pktInfo     : MacDot16CsIpv4Classifier*    : Classifier of the flow
// + transId     : unsigned int                 : Trans Id of flow
// + sfDirection : MacDot16ServiceFlowDirection : Direction of service flow
// + sfInitial   : MacDot16ServiceFlowInitial   : Local or remote initaited
// + isARQEnabled : BOOL : Indicates whether ARQ is to be enabled for the
//                         flow
// + arqParam : MacDot16ARQParam* : Pointer to the structure containing the
//                                  ARQ Parameters to be used in case ARQ is
//                                  enabled.
// RETURN     :: Dot16CIDType : Basic CID of SS that the flow intended for
// **/
Dot16CIDType MacDot16SsAddServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 unsigned char* macAddress,
                 MacDot16ServiceType serviceType,
                 unsigned int csfId,
                 MacDot16QoSParameter* qosInfo,
                 MacDot16CsClassifier* pktInfo,
                 unsigned int transId,
                 MacDot16ServiceFlowDirection sfDirection,
                 MacDot16ServiceFlowInitial sfInitial,
                 BOOL isPackingEnabled,
                 BOOL isFixedLengthSDU,
                 unsigned int fixedLengthSDUSize,
                 TraceProtocolType appType,
                 BOOL isARQEnabled,
                 MacDot16ARQParam* arqParam)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    Message* mgmtMsg;

    // we could verify the macAddress with BS's MAC address. If they are
    // different, no need to add. Not implemented right now.
    if (DEBUG_FLOW)
    {
        char clockStr[MAX_CLOCK_STRING_LENGTH];
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("adding a new flow dl/ul %d csfId %d\n",
               sfDirection, csfId);
        printf("    nextHop MAC address = %d:%d:%d:%d:%d:%d\n",
               macAddress[0], macAddress[1], macAddress[2],
               macAddress[3], macAddress[4], macAddress[5]);
        printf("    service type = %d\n", serviceType);
        printf("    minPktSize = %d\n", qosInfo->minPktSize);
        printf("    maxPktSize = %d\n", qosInfo->maxPktSize);
        printf("    maxSustainedRate = %d\n", qosInfo->maxSustainedRate);
        printf("    minReservedRate = %d\n", qosInfo->minReservedRate);
        ctoa(qosInfo->maxLatency, clockStr);
        printf("    maxLatency = %s\n", clockStr);
        ctoa(qosInfo->toleratedJitter, clockStr);
        printf("    toleratedJitter = %s\n", clockStr);
    }

    sFlow = (MacDot16ServiceFlow*) MEM_malloc(sizeof(MacDot16ServiceFlow));
    ERROR_Assert(sFlow != NULL, "MAC 802.16: Out of memory!");
    memset((char*)sFlow, 0, sizeof(MacDot16ServiceFlow));

    sFlow->next = NULL;
    if (sfInitial == DOT16_SERVICE_FLOW_LOCALLY_INITIATED)
    {
        sFlow->status = DOT16_FLOW_AddingLocal;
    }
    else
    {
        sFlow->status = DOT16_FLOW_AddingRemote;
    }
    sFlow->csfId = csfId;
    sFlow->serviceType = serviceType;
    sFlow->emptyBeginTime = node->getNodeTime();

    // copy over QoS parameters
    memcpy((char*) &(sFlow->qosInfo),
           (char*) qosInfo,
           sizeof(MacDot16QoSParameter));

    sFlow->sfDirection = sfDirection;
    sFlow->admitted = FALSE;

    // initialize the dsx transaction info
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsaInfo));
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dscInfo));
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsdInfo));

    //start DSA transaction
    sFlow->dsaInfo.sfInitial = sfInitial;
    sFlow->isPackingEnabled = isPackingEnabled;
    sFlow->isFixedLengthSDU = isFixedLengthSDU;
    sFlow->fixedLengthSDUSize = fixedLengthSDUSize;
    sFlow->isFragStart = FALSE;
    sFlow->bytesSent = 0;
    sFlow->fragMsg = NULL;
    sFlow->fragFSNno = 0;
    sFlow->appType = appType;

    // increase xAatNum;
    sFlow->numXact ++;

   // ARQ support
    sFlow->isARQEnabled = isARQEnabled;


    if (sFlow->isARQEnabled)
    {
        sFlow->arqParam = (MacDot16ARQParam*)
                                        MEM_malloc(sizeof(MacDot16ARQParam));
        memset(sFlow->arqParam, 0, sizeof(MacDot16ARQParam));
        memcpy(sFlow->arqParam, arqParam, sizeof(MacDot16ARQParam));

        if (DEBUG_FLOW)
        {
            printf("    ARQ Enabled = %d\n",sFlow->isARQEnabled);
             if (isARQEnabled)
            {
                MacDot16PrintARQParameter(sFlow->arqParam);
            }
        }
    }

    if (sfDirection == DOT16_UPLINK_SERVICE_FLOW)
    {
        sFlow->next = dot16Ss->ulFlowList[serviceType].flowHead;
        dot16Ss->ulFlowList[serviceType].flowHead = sFlow;
        dot16Ss->ulFlowList[serviceType].numFlows ++;

        // if ertPS
        if (serviceType == DOT16_SERVICE_ertPS)
        {
            // for ertPS by deafult, it will allocate BW based on the max
            // sustain rate
            sFlow->lastBwRequested = (int)(sFlow->qosInfo.maxLatency *
                                           sFlow->qosInfo.maxSustainedRate /
                                           SECOND / 8);
        }
        if (DEBUG_FLOW)
        {
            printf("    a uplink service flow\n");
        }
    }
    else
    {
        sFlow->next = dot16Ss->dlFlowList[serviceType].flowHead;
        dot16Ss->dlFlowList[serviceType].flowHead = sFlow;
        dot16Ss->dlFlowList[serviceType].numFlows ++;

        if (DEBUG_FLOW)
        {
            printf("    a downlink service flow\n");
        }
    }

    // for local initiated service flow, need to initate the DSA...
    if (sfInitial == DOT16_SERVICE_FLOW_LOCALLY_INITIATED)
    {
        sFlow->dsaInfo.dsxTransStatus = DOT16_FLOW_DSA_LOCAL_Begin;

        // set the dsx with a transactonId
        sFlow->dsaInfo.dsxTransId = dot16Ss->transId;

        // flow management, dsa-req
        mgmtMsg = MacDot16BuildDsaReqMsg(
                      node,
                      dot16,
                      sFlow,
                      pktInfo,
                      sfDirection,
                      dot16Ss->servingBs->primaryCid);

        // adjust the transactionId for future usage
        if (dot16Ss->transId == DOT16_SS_TRANSACTION_ID_END)
        {
            dot16Ss->transId = DOT16_SS_TRANSACTION_ID_START;
        }
        else
        {
            dot16Ss->transId ++;
        }

        // make a copy of the dsa-req msg
        sFlow->dsaInfo.dsxReqCopy = MESSAGE_Duplicate(node, mgmtMsg);

        MacDot16SsEnqueueMgmtMsg(node,
                                 dot16,
                                 DOT16_CID_PRIMARY,
                                 mgmtMsg);
    }
    else
    {
        // set the dsx with a transactonId
        sFlow->dsaInfo.dsxTransId = transId;
        sFlow->dsaInfo.dsxTransStatus = DOT16_FLOW_DSA_REMOTE_Begin;

        if (DEBUG_FLOW)
        {
            printf("    created flow & status is remote DSA begin\n");
        }
    }

    return dot16Ss->servingBs->basicCid;
}

// /**
// FUNCTION   :: MacDot16SsChangeServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Change a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16SsChangeServiceFlow(
         Node* node,
         MacDataDot16* dot16,
         MacDot16ServiceFlow* sFlow,
         MacDot16QoSParameter *newQoSInfo)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    Message* mgmtMsg;

    MacDot16QoSParameter qosInfoLast;
    MacDot16QoSParameter qosInfoNew;
    if (sFlow->dscInfo.dscNewQosInfo)
    {
        memcpy(&qosInfoLast, sFlow->dscInfo.dscNewQosInfo,
                sizeof(MacDot16QoSParameter));
    }
    memcpy(&qosInfoNew, newQoSInfo, sizeof(MacDot16QoSParameter));
    if (sFlow->dscInfo.dsxTransStatus == DOT16_FLOW_DSC_LOCAL_DscRspPending
        && sFlow->dscInfo.dscNewQosInfo && MacDot16ServiceFlowQoSChanged
        (qosInfoLast, qosInfoNew) == FALSE)

    {
         if (!sFlow->dscInfo.dscPendingQosInfo)
         {
             sFlow->dscInfo.dscPendingQosInfo =
            (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
         }
         memcpy(sFlow->dscInfo.dscPendingQosInfo,
                newQoSInfo,
                sizeof(MacDot16QoSParameter));
         return;
    }


    // start dsc transaction
    sFlow->status =  DOT16_FLOW_ChangingLocal;
    sFlow->numXact ++;
    sFlow->dscInfo.dsxTransStatus = DOT16_FLOW_DSC_LOCAL_Begin;

    // set the dsc with a transactonId
    sFlow->dscInfo.dsxTransId = dot16Ss->transId;

    // save the QoS state
    if (!sFlow->dscInfo.dscOldQosInfo)
    {
    sFlow->dscInfo.dscOldQosInfo =
        (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
    }
    memcpy(sFlow->dscInfo.dscOldQosInfo,
           &(sFlow->qosInfo),
           sizeof(MacDot16QoSParameter));

    if (!sFlow->dscInfo.dscNewQosInfo)
    {
    sFlow->dscInfo.dscNewQosInfo =
        (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
    }

    memcpy(sFlow->dscInfo.dscNewQosInfo,
           newQoSInfo,
           sizeof(MacDot16QoSParameter));

    // if new QoS parameters require less bandwidth
    if (newQoSInfo->minReservedRate < sFlow->qosInfo.minReservedRate)
    {
        // modify policing and scheduling params

        // decrease bandwidth
        memcpy((char*) &(sFlow->qosInfo),
                newQoSInfo,
                sizeof(MacDot16QoSParameter));
    }

    // flow management, dsc-req
    mgmtMsg = MacDot16BuildDscReqMsg(
                  node,
                  dot16,
                  sFlow,
                  newQoSInfo,
                  dot16Ss->servingBs->primaryCid);

    // adjust the transactionId for future usage
    if (dot16Ss->transId == DOT16_SS_TRANSACTION_ID_END)
    {
        dot16Ss->transId = DOT16_SS_TRANSACTION_ID_START;
    }
    else
    {
        dot16Ss->transId ++;
    }

    // make a copy of the dsc-req msg
    if (sFlow->dscInfo.dsxReqCopy)
    {
        MESSAGE_Free(node, sFlow->dscInfo.dsxReqCopy);
    }
    sFlow->dscInfo.dsxReqCopy = MESSAGE_Duplicate(node, mgmtMsg);

    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_PRIMARY,
                             mgmtMsg);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("req free bandwidth and "
               "scheudle DSC-REQ for "
               "sf %d with dsTransId %d\n",
               sFlow->sfid, sFlow->dscInfo.dsxTransId);
    }
}

// /**
// FUNCTION   :: MacDot16SsDeleteServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16SsDeleteServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16ServiceFlow* sFlow)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    Message* mgmtMsg;
    // The service flow is inactive for a long time
    // in either NUll or nominal status, needs to delete it

    // start dsd transaction
    sFlow->status = DOT16_FLOW_Deleting;
    sFlow->numXact ++;
    sFlow->dsdInfo.dsxTransStatus = DOT16_FLOW_DSD_LOCAL_Begin;

    // set the dsx with a transactonId
    sFlow->dsdInfo.dsxTransId = dot16Ss->transId;

    // flow management, dsd-req
    mgmtMsg = MacDot16BuildDsdReqMsg(
                  node,
                  dot16,
                  sFlow,
                  dot16Ss->servingBs->primaryCid);

    // adjust the transactionId for future usage
    if (dot16Ss->transId == DOT16_SS_TRANSACTION_ID_END)
    {
        dot16Ss->transId = DOT16_SS_TRANSACTION_ID_START;
    }
    else
    {
        dot16Ss->transId ++;
    }

    // make a copy of the dsa-req msg
    if (sFlow->dsdInfo.dsxReqCopy){
         MESSAGE_Free(node, sFlow->dsdInfo.dsxReqCopy);
    }
    sFlow->dsdInfo.dsxReqCopy = MESSAGE_Duplicate(node, mgmtMsg);

    MacDot16SsEnqueueMgmtMsg(node,
                             dot16,
                             DOT16_CID_PRIMARY,
                             mgmtMsg);

    // Notify CS to invalidate the classfier
    MacDot16CsInvalidClassifier(node, dot16, sFlow->csfId);

    // disable the service flow
    // Assume (de)admit and (de)activate happen at the same time
    sFlow->admitted = FALSE;
    sFlow->activated = FALSE;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("req free bandwidth and "
               "scheudle DSD-REQ for sf %d with dsTransId %d\n",
               sFlow->sfid, sFlow->dsdInfo.dsxTransId);
    }
}

// /**
// FUNCTION   :: MacDot16SsEnqueuePacket
// LAYER      :: MAC
// PURPOSE    :: Enqueue packet of a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress : unsigned char* : MAC address of next hop
// + csfId : unsigned int : ID of the classifier at CS sublayer
// + serviceType : MacDot16ServiceType : Type of the data service flow
// + msg   : Message* : The PDU from upper layer
// + flow  : MacDot16ServiceFlow** : Pointer to the service flow
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE: successfully enqueued, FALSE, dropped
// **/
BOOL MacDot16SsEnqueuePacket(
         Node* node,
         MacDataDot16* dot16,
         unsigned char* macAddress,
         unsigned int csfId,
         MacDot16ServiceType serviceType,
         Message* msg,
         MacDot16ServiceFlow** flow,
         BOOL* msgDropped)
{
    *msgDropped = FALSE;
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16ServiceFlow* sFlow;
    BOOL queueIsFull = FALSE;

    // found the service flow record
    sFlow = dot16Ss->ulFlowList[serviceType].flowHead;
    while (sFlow != NULL)
    {
        if (sFlow->csfId == csfId)
        {
            break;
        }

        sFlow = sFlow->next;
    }

    if (sFlow == NULL)
    {
        // no such service flow existing
        MESSAGE_Free(node, msg);

        // increase the statistics
        dot16Ss->stats.numPktsFromUpper ++;
        *msgDropped = TRUE;
        return TRUE;

    }

    // pass flow back up to the calling function
    *flow = sFlow;

    // enqueue the packet
    msg->next = NULL;

    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled &&
        sFlow->psClassType == POWER_SAVING_CLASS_1)
    {
        MacDot16ePSClasses* psClass = NULL;
        psClass = &dot16Ss->psClassParameter[sFlow->psClassType - 1];
        if (psClass->isWaitForSlpRsp == FALSE
            && psClass->currentPSClassStatus == POWER_SAVING_CLASS_ACTIVATE
            && psClass->statusNeedToChange == FALSE)
        {
            // set this variable to deactivate the class I
            psClass->statusNeedToChange = TRUE;
        }
    }

    if (sFlow->admitted == FALSE)
    {
        if (sFlow->numBytesInTmpMsgQueue + MESSAGE_ReturnPacketSize(msg) +
            sizeof(MacDot16MacHeader) <= DOT16_DEFAULT_DATA_QUEUE_SIZE)
        {
            // not addmitted yet, put into temp queue
            if (sFlow->tmpMsgQueueHead == NULL)
            {
                sFlow->tmpMsgQueueHead = msg;
                sFlow->tmpMsgQueueTail = msg;
            }
            else
            {
                sFlow->tmpMsgQueueTail->next = msg;
                sFlow->tmpMsgQueueTail = msg;
            }
            sFlow->numBytesInTmpMsgQueue += MESSAGE_ReturnPacketSize(msg) +
                                            sizeof(MacDot16MacHeader);

        }
        else
        {
            // tmporary buffer is full, return FALSE to notify
            // CS sublayer to stop getting packets from upper layer
            return FALSE;
        }
    }
    else
    {
        dot16Ss->outScheduler[serviceType]->insert(msg,
                                                   &queueIsFull,
                                                   sFlow->queuePriority,
                                                   NULL,
                                                   node->getNodeTime());

        if (queueIsFull)
        {
            // unable to enqueue, notify CS.
            return FALSE;
        }
    }

    // increase the statistics
    dot16Ss->stats.numPktsFromUpper ++;

    if (sFlow->isEmpty == TRUE)
    {
        sFlow->isEmpty = FALSE;
    }

    return TRUE;
}

// /**
// FUNCTION   :: MacDot16eSsInitiateIdleMode
// LAYER      :: MAC
// PURPOSE    :: SS initiates Idle mode
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
void MacDot16eSsInitiateIdleMode(Node * node,
                                MacDataDot16* dot16)
{
    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Initiating Idle Mode.\n");
    }
    Message* dregReqMsg;
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (dot16Ss->initInfo.registered != TRUE ||
        dot16Ss->isSSSendDregPDU == TRUE)
    {
        return;
    }

    if (dot16Ss->isSleepModeEnabled)
    {
        MacDot16eSsInitSleepModePSClassParameter(node, dot16Ss);
    }

    dregReqMsg = MacDot16eSsBuildDregReqMsg(node, dot16,
                                            DOT16e_DREGREQ_MSReq);
    MacDot16SsEnqueueMgmtMsg(node,
                            dot16,
                            DOT16_CID_BASIC,
                            dregReqMsg);
}

//--------------------------------------------------------------------------
// Handle Timers
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16SsHandleT7T14Timeout
// LAYER      :: MAC
// PURPOSE    :: SS handle T7/T14 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16SsHandleT7T14Timeout(Node * node,
                                  MacDataDot16* dot16,
                                  MacDot16Timer* timerInfo)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16TimerType timerType;
    MacDot16ServiceFlow* sFlow;
    unsigned int transId;
    MacDot16ServieFlowXactType xactType;

    timerType = timerInfo->timerType;
    transId = timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                              dot16,
                                              transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("T7 or T14 timeout with unrecognized transId");
        }

        return;
    }

    if (xactType == DOT16_FLOW_XACT_Add)
    {
        if (timerInfo->timerType == DOT16_TIMER_T7)
        {
            sFlow->dsaInfo.timerT7 = NULL;
        }
        else
        {
            sFlow->dsaInfo.timerT14 = NULL;
        }
    }
    else if (xactType == DOT16_FLOW_XACT_Delete)
    {
        if (timerInfo->timerType == DOT16_TIMER_T7)
        {
            sFlow->dsdInfo.timerT7 = NULL;
        }
    }
    else
    {
        if (timerInfo->timerType == DOT16_TIMER_T7)
        {
            sFlow->dscInfo.timerT7 = NULL;
        }
        else
        {
            sFlow->dscInfo.timerT14 = NULL;
        }
    }

    if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add)
    {// Service flow adding related

      if (sFlow->dsaInfo.dsxTransStatus ==
          DOT16_FLOW_DSA_LOCAL_DsaRspPending)
      {
        // determine retry availability
        if (sFlow->dsaInfo.dsxRetry > 0)
        {// still available to retry

            // make a copy of the dsa-req msg
            Message *mgmtMsg;

            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxReqCopy);
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);

            // reset T7
            if (timerInfo->timerType == DOT16_TIMER_T7)
            {
              sFlow->dsaInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             transId,
                                             DOT16_FLOW_XACT_Add);

                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("T7 Timeout for DSA-req transId %d\n",
                           timerInfo->info);
                }
            }
            else
            {// T14 timeout
                if (sFlow->dsaInfo.timerT7 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT7);
                    sFlow->dsaInfo.timerT7 = NULL;
                }
                sFlow->dsaInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             transId,
                                             DOT16_FLOW_XACT_Add);
                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("T14 Timeout for DSA-req transId %d\n",
                            timerInfo->info);
                }
            }

            // move the dsx status to local DSA-RSP pending
            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_LOCAL_DsaRspPending;

            if (DEBUG_FLOW)
            {
                 printf("   T7 or T14 Timeout "
                        "resend DSA-req transId %d, reset T7\n",
                        transId);
            }
        }
        else
        {// retry not available

            // start Timer t10 and move status to retry-exhausted
            if (sFlow->dsaInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT10);
                sFlow->dsaInfo.timerT10 = NULL;
            }
            sFlow->dsaInfo.timerT10 =
                    MacDot16SetTimer(node,
                                     dot16,
                                     DOT16_TIMER_T10,
                                     dot16Ss->para.t10Interval,
                                     NULL,
                                     transId,
                                     DOT16_FLOW_XACT_Add);
            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_LOCAL_RetryExhausted;

            // cancel T7 (if this is T14) or T14 (if this is T7)
            if (sFlow->dsaInfo.timerT7 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT7);
                sFlow->dsaInfo.timerT7 = NULL;
            }
            if (sFlow->dsaInfo.timerT14 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT14);
                sFlow->dsaInfo.timerT14 = NULL;
            }

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 or T14 Timeout opp DSA-req"
                       "transId %d is exhausted\n",
                       transId);
            }
        }
      }
    } // adding
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Delete &&
             sFlow->dsdInfo.dsxTransStatus ==
             DOT16_FLOW_DSD_LOCAL_DsdRspPending)
    {
        // determine retry availability
        if (sFlow->dsdInfo.dsxRetry > 0)
        {// still available to retry

            // make a copy of the dsd-req msg
            Message* mgmtMsg;

            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dsdInfo.dsxReqCopy);
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);
            // reset T7
            sFlow->dsdInfo.timerT7 = MacDot16SetTimer(
                                         node,
                                         dot16,
                                         DOT16_TIMER_T7,
                                         dot16Ss->para.t7Interval,
                                         NULL,
                                         transId,
                                         DOT16_FLOW_XACT_Delete);

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                 printf("T7 Timeout for DSD-REQ transId %d\n",
                        timerInfo->info);
            }
        }
        else
        {// retry mot available

            // Move the status
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_LOCAL_End;

            // reset the dsaInfo
            MacDot16ResetDsxInfo(
                node,
                dot16,
                sFlow,
                &(sFlow->dsdInfo));

            // decrease numXact
            sFlow->numXact --;

            // Move the status back to operational
            sFlow->status = DOT16_FLOW_Nominal;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 timeout, opp for DSD-req "
                       "transId %d is exhausted\n",
                       transId);
            }
        }
    } // deleting
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_LOCAL_DscRspPending)
    {

        if (sFlow->dscInfo.dsxRetry > 0)
        {// still available to retry

            // make a copy of the dsa-req msg
            Message* mgmtMsg;

            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dscInfo.dsxReqCopy);
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);
            // reset T7
            if (timerInfo->timerType == DOT16_TIMER_T7)
            {
                sFlow->dscInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             transId,
                                             DOT16_FLOW_XACT_Change);

                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("T7 Timeout for DSC-req transId %d\n",
                           timerInfo->info);
                }
            }
            else
            {// T14 timeout
                if (sFlow->dscInfo.timerT7 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT7);
                    sFlow->dscInfo.timerT7 = NULL;
                }
                sFlow->dscInfo.timerT7 = MacDot16SetTimer(
                                             node,
                                             dot16,
                                             DOT16_TIMER_T7,
                                             dot16Ss->para.t7Interval,
                                             NULL,
                                             transId,
                                             DOT16_FLOW_XACT_Change);
                if (DEBUG_FLOW)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("T14 Timeout for DSC-req transId %d\n",
                            timerInfo->info);
                }
            }

            // move the dsx status to local DSC-RSP pending
            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_LOCAL_DscRspPending;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 or T14 Timeout resend DSC-req transId %d,"
                       " reset T7\n", transId);
            }
        }
        else
        {// retry not available

            // start Timer t10 and move status to retry-exhausted
            if (sFlow->dscInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT10);
                sFlow->dscInfo.timerT10 = NULL;
            }
            sFlow->dscInfo.timerT10 =
                    MacDot16SetTimer(node,
                                     dot16,
                                     DOT16_TIMER_T10,
                                     dot16Ss->para.t10Interval,
                                     NULL,
                                     transId,
                                     DOT16_FLOW_XACT_Change);
            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_LOCAL_RetryExhausted;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 or T14 Timeout opp DSC-req transId %d is "
                       "exhausted\n", transId);
            }
        }
    }// change

}

// /**
// FUNCTION   :: MacDot16SsHandleT10Timeout
// LAYER      :: MAC
// PURPOSE    :: SS handle T10 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16SsHandleT10Timeout(Node * node,
                                MacDataDot16* dot16,
                                MacDot16Timer* timerInfo)
{
    MacDot16ServiceFlow* sFlow;
    unsigned int transId;
    MacDot16ServieFlowXactType xactType;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T10 Timeout for DSX-req transId %d, "
               "action type = %d\n",
               timerInfo->info, timerInfo->Info2);
    }

    transId = timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;

    // Get the service flow
    sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                              dot16,
                                              transId);

    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("T10 timeout with unrecognized transId");
        }

        return;
    }

    if (xactType == DOT16_FLOW_XACT_Add)
    {
        sFlow->dsaInfo.timerT10 = NULL;
    }
    else if (xactType == DOT16_FLOW_XACT_Delete)
    {
        sFlow->dsdInfo.timerT10 = NULL;
    }
    else
    {
        sFlow->dscInfo.timerT10 = NULL;
    }

    if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add &&
        (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_HoldingDown))
    {

        // Move the status
        sFlow->dsaInfo.dsxTransStatus =
            DOT16_FLOW_DSA_LOCAL_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));

        // decrease numXact
        sFlow->numXact --;

        // now service flow is operational
        sFlow->status = DOT16_FLOW_Nominal;
    } // add & holdingdown
    else if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add &&
        ((sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_RetryExhausted) ||
        (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_DeleteFlow)))
    {
        sFlow->dsaInfo.dsxTransStatus =
            DOT16_FLOW_DSA_LOCAL_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));

        // decrease numXact
        sFlow->numXact --;
        // now service flow is not operational
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }
    } // add & retryexhaust/delete flow (Local)
    else if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add &&
        sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_REMOTE_DeleteFlow)
    {
        // notify CS DSA end?

        // move status to end
        sFlow->dsaInfo.dsxTransStatus =
            DOT16_FLOW_DSA_REMOTE_End;
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));
        sFlow->numXact --;
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }
    } // add and deleteFlow(remote)
    else if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Delete &&
        (sFlow->dsdInfo.dsxTransStatus ==
            DOT16_FLOW_DSD_LOCAL_HoldingDown ||
         sFlow->dsdInfo.dsxTransStatus ==
            DOT16_FLOW_DSD_REMOTE_HoldingDown))
    {
        if (sFlow->dsdInfo.dsxTransStatus ==
            DOT16_FLOW_DSD_LOCAL_HoldingDown )
        {
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_LOCAL_End;
        }
        else
        {
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_REMOTE_End;
        }

        // do not need to reset because delete will do it
        sFlow->numXact --;
        sFlow->status = DOT16_FLOW_Deleted;
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }

        // remove Sflow from the list
        MacDot16SsDeleteServiceFlowByTransId(
            node,
            dot16,
            transId);
        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("DSD T10 timeout, flow is freed and DSD complete \n");
        }
    } // delete & holdingDown(remote or local)
    else if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Change &&
        ((sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_HoldingDown) ||
        (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_RetryExhausted) ||
        (sFlow->dscInfo.dsxTransStatus ==
            DOT16_FLOW_DSC_LOCAL_DeleteFlow)))
    {
        sFlow->dscInfo.dsxTransStatus =
            DOT16_FLOW_DSC_LOCAL_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dscInfo));

        // decrease numXact
        sFlow->numXact --;

        // now service flow is operational
        sFlow->status = DOT16_FLOW_Nominal;

        // Call any pending QOS change saved
        if (sFlow->dscInfo.dscPendingQosInfo)
        {
            MacDot16QoSParameter pendingQosInfo;
            memcpy(&pendingQosInfo, sFlow->dscInfo.dscPendingQosInfo,
                 sizeof(MacDot16QoSParameter));
            MEM_free(sFlow->dscInfo.dscPendingQosInfo);
            sFlow->dscInfo.dscPendingQosInfo = NULL;
            MacDot16SsChangeServiceFlow(node,
                                        dot16,
                                        sFlow,
                                        &pendingQosInfo);
        }
    } // change & Local HOldingDown, RetryExhausted, DeleteFlow
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_REMOTE_DeleteFlow)
    {
        sFlow->dscInfo.dsxTransStatus =
            DOT16_FLOW_DSC_REMOTE_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dscInfo));

        // decrease numXact
        sFlow->numXact --;

        // move bakc to nominal
        sFlow->status = DOT16_FLOW_Nominal;
    } // change & remote DeleteFlow

}

// /**
// FUNCTION   :: MacDot16SsHandleT8Timeout
// LAYER      :: MAC
// PURPOSE    :: SS handle T8 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16SsHandleT8Timeout(Node * node,
                               MacDataDot16* dot16,
                               MacDot16Timer* timerInfo)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    unsigned int transId;
    MacDot16ServiceFlow* sFlow;
    MacDot16ServieFlowXactType xactType;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T8 timeout\n");
    }
    transId = timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;
    sFlow = MacDot16SsGetServiceFlowByTransId(node,
                                              dot16,
                                              transId);
    if (sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no service flow associate with T8's transId");
        }

        return;
    }

    if (xactType == DOT16_FLOW_XACT_Add)
    {
        sFlow->dsaInfo.timerT8 = NULL;
    }
    else if (xactType == DOT16_FLOW_XACT_Delete)
    {
        sFlow->dsdInfo.timerT8 = NULL;
    }
    else
    {
        sFlow->dscInfo.timerT8 = NULL;
    }

    if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add &&
        sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_REMOTE_DsaAckPending)
    {
        if (sFlow->dsaInfo.dsxRetry > 0)
        {
            Message* mgmtMsg;

            // make a copy of the dsa-rsp msg copy
            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxRspCopy);

            //schedule the transmission of DSA-RSP
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);

            // stay in the current status
        }
        else
        {

            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_REMOTE_End;

            MacDot16ResetDsxInfo(
                node,
                dot16,
                sFlow,
                &(sFlow->dsaInfo));

            // decrease numXact
            sFlow->numXact --;

            // further development for the state transition
            if (sFlow->numXact == 0)
            {
                sFlow->status = DOT16_FLOW_NULL;
            }
        }
    } // add & ackpending
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Add &&
             sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_REMOTE_HoldingDown)
    {
        sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_REMOTE_End;

        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));

        // decrease numXact
        sFlow->numXact --;

        sFlow->status = DOT16_FLOW_Nominal;
    } // add && holding
    else  if (sFlow != NULL &&
              xactType == DOT16_FLOW_XACT_Change &&
              sFlow->dscInfo.dsxTransStatus ==
                  DOT16_FLOW_DSC_REMOTE_DscAckPending)
    {
        if (sFlow->dscInfo.dsxRetry > 0)
        {
            Message* mgmtMsg;

            // make a copy of the dsa-ack msg copy
            mgmtMsg = MESSAGE_Duplicate(node, sFlow->dscInfo.dsxRspCopy);

            //schedule the transmission of DSC-RSP
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_PRIMARY,
                                     mgmtMsg);
            // increase stats
            dot16Ss->stats.numDscRspSent ++;

            // decrease retry
            sFlow->dscInfo.dsxRetry --;
            sFlow->dscInfo.timerT8 = MacDot16SetTimer(
                                         node,
                                         dot16,
                                         DOT16_TIMER_T8,
                                         dot16Ss->para.t8Interval,
                                         NULL,
                                         transId,
                                         DOT16_FLOW_XACT_Change);

            // stay in the current status
        }
        else
        {

            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_REMOTE_End;

            MacDot16ResetDsxInfo(
                node,
                dot16,
                sFlow,
                &(sFlow->dscInfo));

            // decrease numXact
            sFlow->numXact --;

            sFlow->status = DOT16_FLOW_Nominal;

        }
    } // change & ackpending
    else if (sFlow != NULL &&
            xactType == DOT16_FLOW_XACT_Change &&
            sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_REMOTE_HoldingDown)
    {
        sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_REMOTE_End;
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dscInfo));

        // decrease numXact
        sFlow->numXact --;

        sFlow->status = DOT16_FLOW_Nominal;
    } // change & holdingdown

}
// /**
// FUNCTION   :: MacDot16SsCheckDlIntervalUsageCode
// LAYER      :: MAC
// PURPOSE    :: check the diuc.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Interface index
// + unicastBurst     : BOOL              : Unicast burst or not
// + codeModyType     : unsigned char     : coding modulation used
// RETURN     :: void : NULL
// **/
static
void MacDot16SsCheckDlIntervalUsageCode(Node * node,
                                        MacDataDot16* dot16,
                                        BOOL unicastBurst,
                                        unsigned char codeModuType)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    unsigned  char diucInUse = 0;
    unsigned char leastRobustDiuc;
    BOOL needReportMea = FALSE;

    // determine the current least robust DIUC
    if (MacDot16GetLeastRobustBurst(node,
                                    dot16,
                                    DOT16_DL,
                                    dot16Ss->servingBs->cinrMean,
                                    &leastRobustDiuc))
    {
        // whether needs to change the local copy of least dl
        // burst and perform DBCP

        // DBCP is not appliable for OFDMA PHY
        // to see if need to send REP-RSP to report measurement

        if (dot16Ss->servingBs->leastRobustDiuc != leastRobustDiuc)
        {

            dot16Ss->servingBs->leastRobustDiuc = leastRobustDiuc;

            needReportMea = TRUE;
        }
    }

    if (unicastBurst)
    {
        // if this is a unicast burst then it has the diuc
        // get the diuc in use
        int profileIndex;

        MacDot16GetProfIndexFromCodeModType(node,
                                            dot16,
                                            DOT16_DL,
                                            codeModuType,
                                            &profileIndex);


        diucInUse = (UInt8)profileIndex;

        if (diucInUse !=
            dot16Ss->servingBs->leastRobustDiuc)
        {
            // if the current diuc in use is
            // different from leastRobust
            needReportMea = TRUE;
        }
        else if (diucInUse ==
                 dot16Ss->servingBs->leastRobustDiuc &&
                 dot16Ss->servingBs->numEmergecyMeasRep != 0)
        {
            // if BS changed the diuc to latest one
            dot16Ss->servingBs->numEmergecyMeasRep = 0;
        }

    }
    else
    {
        //broadcast
        unicastBurst = 0;

    }

    if (needReportMea && dot16Ss->operational)
    {
        if ((dot16Ss->servingBs->lastReportTime +
             DOT16_MEASUREMENT_VALID_TIME * 2) <
             node->getNodeTime() &&
             dot16Ss->servingBs->numEmergecyMeasRep ==
             DOT16_MAX_EMERGENCY_MEAS_REPORT)
        {
            // too long not to report
            dot16Ss->servingBs->numEmergecyMeasRep = 0;
        }

        if (unicastBurst &&
            diucInUse <
            dot16Ss->servingBs->leastRobustDiuc &&
            (dot16Ss->servingBs->numEmergecyMeasRep <
             DOT16_MAX_EMERGENCY_MEAS_REPORT) &&
            (dot16Ss->servingBs->lastReportTime +
             DOT16_EMERGENCY_MEAS_REPORT_INTERVAL) <
             node->getNodeTime())
        {
            dot16Ss->servingBs->numEmergecyMeasRep ++;
        }
        else if (unicastBurst &&
                 diucInUse >
                 dot16Ss->servingBs->leastRobustDiuc)
        {
            // must report
        }
        else
        {
            // if already send many times in short period
            needReportMea = FALSE;
        }

        if (needReportMea)
        {
            Message* repRspMsg;

            // report the mean of cinr & rssi to BS
            repRspMsg = MacDot16SsBuildRepRspMsg(node,
                                                 dot16,
                                                 0x06);
            MacDot16SsEnqueueMgmtMsg(node,
                                     dot16,
                                     DOT16_CID_BASIC,
                                     repRspMsg);
            // update stats
            dot16Ss->stats.numRepRspSent ++;

            dot16Ss->servingBs->lastReportTime =
                node->getNodeTime();

            if (DEBUG || DEBUG_RANGING ||
                node->nodeId == DEBUG_SPECIFIC_NODE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Report signal measurement and request "
                       "change DIUC from %d in use to %d\n",
                       diucInUse,
                       dot16Ss->servingBs->leastRobustDiuc);

            }
        }
    }

    if (dot16Ss->initInfo.initRangingCompleted)
    {
        if (DEBUG_RANGING)
        {
            MacDot16PrintModulationCodingInfo(node, dot16);
            printf("Node %d cid %d least diuc %d, cinr Mean %f rng status %d\n",
                    node->nodeId, dot16Ss->servingBs->basicCid,
                    dot16Ss->servingBs->leastRobustDiuc,
                    dot16Ss->servingBs->cinrMean,dot16Ss->initInfo.rngState);
        }
    }
}

// /**
// FUNCTION   :: MacDot16SsHandleT45Timeout
// LAYER      :: MAC
// PURPOSE    :: SS handle T45 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsHandleT45Timeout(Node * node,
                               MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    dot16Ss->idleSubStatus = DOT16e_SS_IDLE_None;
    dot16Ss->dregReqRetCount++;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T45 timeout\n");
    }
    dot16Ss->isSSSendDregPDU = FALSE;
    if (dot16Ss->dregReqRetCount < DOT16e_SS_DREG_REQ_RETRY_COUNT)
    {
        MacDot16eSsInitiateIdleMode(node, dot16);
    }
    else
    {
        //re-initialize mac
        MacDot16SsRestartOperation(node, dot16);
    }
}

// /**
// FUNCTION   :: MacDot16SsHandleDregReqDurationTimeout
// LAYER      :: MAC
// PURPOSE    :: SS handle Dreg Req Duration timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsHandleDregReqDurationTimeout(Node * node,
                                            MacDataDot16* dot16)
{
    MacDot16eSsInitiateIdleMode(node, dot16);
}

// /**
// FUNCTION   :: MacDot16SsLocUpdTimeout
// LAYER      :: MAC
// PURPOSE    :: SS handle Location update timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsLocUpdTimeout(Node * node,
                    MacDataDot16* dot16, Message* msg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    //perform location update
    MacDot16eSsPerformLocationUpdate(node, dot16);
    //renew timers
/*
    if (dot16Ss->timers.timerIdleMode != NULL)
    {
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerIdleMode);
    }
    dot16Ss->timers.timerIdleMode = MacDot16SetTimer(
                                        node,
                                        dot16,
                                        DOT16e_TIMER_IdleMode,
                                        dot16Ss->para.tIdleModeInterval,
                                        NULL,
                                        0);
*/
    dot16Ss->timers.timerLocUpd = MacDot16SetTimer(
                                        node,
                                        dot16,
                                        DOT16e_TIMER_LocUpd,
                                        dot16Ss->para.tLocUpdInterval,
                                        NULL,
                                        0);
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: MacDot16SsIdleTimeout
// LAYER      :: MAC
// PURPOSE    :: SS handle Idle mode timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
/*
static
void MacDot16eSsIdleTimeout(Node * node,
                    MacDataDot16* dot16)
{
}
*/
// /**
// FUNCTION   :: MacDot16SsMsPagingUnavlblIntervalTimeout
// LAYER      :: MAC
// PURPOSE    :: To wake up the node and listen to BS Paging Adv
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsMsPagingUnavlblIntervalTimeout(Node * node,
                    MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (dot16Ss->performNetworkReentry == FALSE)
    {
        dot16Ss->idleSubStatus = DOT16e_SS_IDLE_MSListen;
        dot16Ss->timers.timerMSPaging = MacDot16SetTimer(
                node,
                dot16,
                DOT16e_TIMER_MSPaging,
                dot16Ss->servingBs->pagingInterval *
                    (dot16Ss->servingBs->frameDuration - dot16Ss->para.rtg),
                NULL,
                0);
    }
    else
    {
        dot16Ss->idleSubStatus = DOT16e_SS_IDLE_None;
    }

    //scan the network once again.
    //here we use PerformNetworkEntry function to scan and synch, but we
    //would stop before RNG-REQ (if not required)
    MacDot16SsPerformNetworkEntry(node, dot16);
    if (dot16Ss->performNetworkReentry == FALSE)
    {
        dot16Ss->operational = TRUE;
    }
}

// /**
// FUNCTION   :: MacDot16SsMsPagingIntervalTimeout
// LAYER      :: MAC
// PURPOSE    :: To put node back to Idle mode
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eSsMsPagingIntervalTimeout(Node * node,
        MacDataDot16* dot16)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    if (!dot16Ss->performLocationUpdate)
    {
        MacDot16eStopTimersBeforeIdle(node, dot16);
    }
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 SS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16SsInit(Node* node,
                    int interfaceIndex,
                    const NodeInput* nodeInput)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    MacDot16Ss* dot16Ss;
    unsigned int phyNumber;
    int i;

    if (DEBUG)
    {
        printf("Node%u SS is initializing 802.16 MAC on interface %d\n",
               node->nodeId, interfaceIndex);
    }

    dot16Ss = (MacDot16Ss*) MEM_malloc(sizeof(MacDot16Ss));
    ERROR_Assert(dot16Ss != NULL, "802.16: Out of memory!");

    // using memset to initialize the whole DOT16 data strucutre
    memset((char*)dot16Ss, 0, sizeof(MacDot16Ss));

    dot16->ssData = (void*) dot16Ss;

    // init system parameters
    MacDot16SsInitParameter(
        node,
        nodeInput,
        interfaceIndex,
        dot16);

    if (DEBUG_PARAMETER)
    {
        // print out parameters
        MacDot16SsPrintParameter(node, dot16);
    }

    // init dynamic statistics
    if (node->guiOption)
    {
        MacDot16SsInitDynamicStats(node, interfaceIndex, dot16);
    }

    // start scanning channel to receive DL-MAP message
    // Start from my first listening channel
    phyNumber = dot16->myMacData->phyNumber;
    dot16Ss->chScanIndex = 0;
    for (i = 0; i < dot16->numChannels; i ++)
    {
        if (PHY_IsListeningToChannel(node,
                                     phyNumber,
                                     dot16->channelList[i]))
        {
            dot16Ss->chScanIndex = i;
            break;
        }
    }

    // stop scanning to any channel at the begining until operation start
    for (i = 0; i < dot16->numChannels; i ++)
    {
        MacDot16StopListening(node, dot16, dot16->channelList[i]);
    }

    // allocate structure to store the information of serving BS.
    dot16Ss->servingBs = (MacDot16SsBsInfo*)
                         MEM_malloc(sizeof(MacDot16SsBsInfo));
    ERROR_Assert(dot16Ss->servingBs != NULL, "MAC 802.16: Out of memory!");
    memset((char*) dot16Ss->servingBs, 0, sizeof(MacDot16SsBsInfo));

    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];
    dot16Ss->servingBs->ulChannel = DOT16_INVALID_CHANNEL;
    dot16Ss->servingBs->numCidSupport = DOT16_DEFAULT_MAX_NUM_CID_SUPPORT;

    dot16Ss->operational = FALSE;
    dot16Ss->transId = DOT16_SS_TRANSACTION_ID_START;

    // dot16e
    dot16Ss->servingBs->configChangeCount = 0; // init value
    if (dot16Ss->rngType == DOT16_CDMA
        || dot16Ss->bwReqType == DOT16_BWReq_CDMA)
    {
        dot16Ss->numOfDLBurstProfileInUsed = DOT16_NUM_DL_BURST_PROFILES;
        dot16Ss->numOfULBurstProfileInUsed = DOT16_NUM_UL_BURST_PROFILES;
    }
    else
    {
        dot16Ss->numOfDLBurstProfileInUsed = DOT16_NUM_DL_BURST_PROFILES - 1;
        dot16Ss->numOfULBurstProfileInUsed = DOT16_NUM_UL_BURST_PROFILES - 1;
    }
}

static
BOOL MacDot16eIsPSClassActive(MacDataDot16* dot16,
                              MacDot16ePSClassType classType)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    int i = 0;
    MacDot16ePSClasses* psClass = NULL;
    BOOL retVal = FALSE;

    for (i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        psClass = &dot16Ss->psClassParameter[i];
        if (psClass->classType == classType && psClass->currentPSClassStatus
            == POWER_SAVING_CLASS_ACTIVATE && psClass->isSleepDuration
            == TRUE)
        {
            retVal = TRUE;
            break;
        }
    }// end of for
    return retVal;
}// end of MacDot16eIsPSClassActive

// /**
// FUNCTION   :: MacDot16SsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : int      : Interface index
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16SsLayer(Node* node, int interfaceIndex, Message* msg)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    BOOL msgReused = FALSE;

    if (DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("MAC 802.16 intf%d get a msg\n",
               interfaceIndex);
    }

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            MacDot16TimerType timerType;
            MacDot16Timer* timerInfo = NULL;
            clocktype delay = 0;

            // get info from message
            timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(msg);
            timerType = timerInfo->timerType;

            if (DEBUG_TIMER)
            {
                printf("MAC 802.16: node%u intf%d get a timer msg %d\n",
                       node->nodeId, interfaceIndex, timerType);
            }

            switch (timerType)
            {
                case DOT16_TIMER_OperationStart:
                {
                    // power on of the SS
                    MacDot16SsStartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_T1:
                {
                    // Timeout without getting a DCD
                    // lost syncronization, need to reboot
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DCD Timer T1 timeout, lost sync, restart "
                               "network entry procedure from channel %d\n",
                                dot16Ss->chScanIndex);
                    }
                    dot16Ss->timers.timerT1 = NULL;
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_T2:
                {

                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T2 Timeout, cannot get UL MAP with ranging "
                               "opportunity on channel index %d, and switch"
                               " to next channel %d\n",
                                dot16Ss->chScanIndex,
                                (dot16Ss->chScanIndex + 1) %
                                dot16->numChannels);
                    }
                    dot16Ss->timers.timerT2 = NULL;

                    // Timeout of gettting UL MAP with ranging opportunity
                    // Switch to another channel and search
                    // stop listening to the previous channel and restart
                    // dl search
                    MacDot16StopListening(node,
                                          dot16,
                                          dot16Ss->servingBs->dlChannel);

                    // switch to the next channel
                    dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                           dot16->numChannels;

                    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];
                    //MacDot16SsPerformNetworkEntry(node, dot16);
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_T3:
                {
                    // initial Ranging request timeout without getting a
                    // RNG-RSP

                    // backoff and retry
                    dot16Ss->servingBs->rangeRetries ++;

                    dot16Ss->timers.timerT3 = NULL;
                    // If number of retries has reached maximum value, we
                    // should give up this DL channel, and switch to next
                    // one
                    if (dot16Ss->servingBs->rangeRetries >=
                                    DOT16_MAX_INITIAL_RANGE_RETRIES)
                    {
                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("init rng, T3 expires with max retries,"
                                    "Mrak the channel %d unusable,"
                                    "and start init again for next ch %d\n",
                                    dot16Ss->chScanIndex,
                                    (dot16Ss->chScanIndex + 1) %
                                    dot16->numChannels);

                        }

                        // Switch to another channel and search
                        // stop listening to the previous channel and
                        // restart dl search
                        MacDot16StopListening(
                                            node,
                                            dot16,
                                            dot16Ss->servingBs->dlChannel);

                        // switch to the next channel
                        dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                               dot16->numChannels;

                        dot16Ss->servingBs->dlChannel =
                            dot16->channelList[dot16Ss->chScanIndex];
                        //MacDot16SsPerformNetworkEntry(node, dot16);
                        MacDot16SsRestartOperation(node, dot16);
                    }
                    else
                    {
                        // T3 expire
                        // one case. initail range in contention-based burst
                        // lost/ rsp lost another case, initial range in
                        // polled raning burst is lost / rsp lost both case
                        //  will use ocntention-based burst ranging again

                        dot16Ss->servingBs->rangeBOCount ++;

                        if (dot16Ss->servingBs->rangeBOCount >
                            dot16Ss->para.rangeBOEnd)
                        {
                            dot16Ss->servingBs->rangeBOCount =
                                                dot16Ss->para.rangeBOEnd;
                        }

                        dot16Ss->servingBs->rangeBOValue =
                            RANDOM_nrand(dot16->seed) %
                            (int)pow(2.0, dot16Ss->servingBs->rangeBOCount);

                        dot16Ss->servingBs->curerntTxPower +=
                                DOT16_SS_DEFAULT_POWER_UNIT_LEVEL;

                        // if reaching maximum go to minimum
                        if (dot16Ss->servingBs->curerntTxPower >
                        dot16Ss->servingBs->maxTxPowerInitRng)
                        {
                            dot16Ss->servingBs->curerntTxPower =
                                    dot16Ss->para.minTxPower;
                        }

                        if (dot16Ss->rngType == DOT16_CDMA)
                        {
                            dot16Ss->initInfo.rngState =
                                        DOT16_SS_RNG_WaitBcstInitRngOpp;
                        }
                        else
                        {
                            dot16Ss->initInfo.rngState =
                                        DOT16_SS_RNG_WaitBcstRngOpp;
                        }
                        if (DEBUG_NETWORK_ENTRY || DEBUG_RANGING)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("Init ranging, T3 timer expires,"
                                   "perform backoff and  try again "
                                   "with backoff %d in max %d\n",
                                   dot16Ss->servingBs->rangeBOValue,
                                   (int)pow(2.0, dot16Ss->servingBs->
                                   rangeBOCount));
                        }
                    }

                    break;
                }
                case DOT16_TIMER_T4:
                {
                    // periodic ranging
                    // SS did not get data grant from BS in this interval.
                    // need to re start initilization
                    if (DEBUG_RANGING || DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T4 Timeout, cannot get unicast (perioidic) "
                               "ranging opportunity from channel index %d,"
                               "and swtich to next channel %d\n",
                                dot16Ss->chScanIndex,
                                (dot16Ss->chScanIndex + 1) % dot16->
                                numChannels);
                    }

                    dot16Ss->timers.timerT4 = NULL;
                    // restart T4
                    dot16Ss->timers.timerT4 =
                                MacDot16SetTimer(
                                            node,
                                            dot16,
                                            DOT16_TIMER_T4,
                                            dot16Ss->para.t4Interval,
                                            NULL,
                                            0);

                    if (dot16Ss->rngType == DOT16_CDMA)
                    {
                        // set parameter to send periodic ranging code
                        dot16Ss->periodicRngInfo.toSendRngReq = TRUE;
                    }
                    else
                    {
                        // Timeout of gettting UCD on this channel
                        // Switch to another channel and search
                        // stop listening to the previous channel
                        MacDot16StopListening(node,
                                              dot16,
                                              dot16Ss->servingBs->dlChannel);

                        // switch to the next channel
                        dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                               dot16->numChannels;

                        dot16Ss->servingBs->dlChannel =
                            dot16->channelList[dot16Ss->chScanIndex];
                        //MacDot16SsPerformNetworkEntry(node, dot16);
                        MacDot16SsRestartOperation(node, dot16);
                    }
                    break;
                }
                case DOT16_TIMER_T6:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T6 timeout\n");

                    }

                    dot16Ss->timers.timerT6 = NULL;

                    if (dot16Ss->servingBs->regRetries >
                        DOT16_MAX_REGISTRATION_RETRIES)
                    {
                        if (DEBUG_NETWORK_ENTRY)
                        {
                            printf("    After max %d try of registration,"
                                   "SS restart MAC intialization\n",
                                   dot16Ss->servingBs->regRetries);

                        }
                        // Make sure whether: Switch to another channel
                        // and search? stop listening to the previous
                        // channel
                        MacDot16StopListening(
                                        node,
                                        dot16,
                                        dot16Ss->servingBs->dlChannel);

                        // switch to the next channel
                        dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                               dot16->numChannels;

                        dot16Ss->servingBs->dlChannel =
                            dot16->channelList[dot16Ss->chScanIndex];

                        //MacDot16SsPerformNetworkEntry(node, dot16);
                        MacDot16SsRestartOperation(node, dot16);

                    }
                    else
                    {
                        if (DEBUG_NETWORK_ENTRY)
                        {
                            printf("    It is %d th try of registration,"
                                   "retry registration\n",
                                   dot16Ss->servingBs->regRetries);
                        }

                        MacDot16SsPerformRegistration(node, dot16);
                    }

                    break;
                }
                case DOT16_TIMER_T7:
                {
                    MacDot16SsHandleT7T14Timeout(node, dot16, timerInfo);

                    break;
                }
                case DOT16_TIMER_T14:
                {
                    MacDot16SsHandleT7T14Timeout(node, dot16, timerInfo);

                    break;
                }
                case DOT16_TIMER_T8:
                {
                    MacDot16SsHandleT8Timeout(node, dot16, timerInfo);

                    break;
                }
                case DOT16_TIMER_T10:
                {
                    MacDot16SsHandleT10Timeout(node, dot16, timerInfo);

                    break;
                }
                case DOT16_TIMER_T12:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T12 Timeout, cannot get UCD on"
                               "ch index %d, and switch to next channel"
                               " %d\n",
                               dot16Ss->chScanIndex,
                               (dot16Ss->chScanIndex + 1) %
                               dot16->numChannels);
                    }
                    dot16Ss->timers.timerT12 = NULL;

                    // Timeout of gettting UCD on this channel
                    // Switch to another channel and search
                    // stop listening to the previous channel
                    MacDot16StopListening(node,
                                          dot16,
                                          dot16Ss->servingBs->dlChannel);

                    // switch to the next channel
                    dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                           dot16->numChannels;

                    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];
                    //MacDot16SsPerformNetworkEntry(node, dot16);
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_T16:
                {
                    // Bandwidth request timeout without getting BW grant
                    dot16Ss->timers.timerT16 = NULL;
                    dot16Ss->isWaitBwGrant = FALSE;

                    // backoff and retry
                    dot16Ss->servingBs->requestRetries ++;
                    if (dot16Ss->servingBs->requestRetries >
                        dot16Ss->para.requestRetries)
                    {
                        if (dot16Ss->bwReqInContOpp != NULL)
                        {
                            MEM_free(dot16Ss->bwReqInContOpp);
                            dot16Ss->bwReqInContOpp = NULL;
                        }

                        dot16Ss->servingBs->requestRetries = 0;
                        dot16Ss->servingBs->requestBOCount =
                            dot16Ss->para.requestBOStart;
                        dot16Ss->servingBs->requestBOValue = 0;
                        if (DEBUG_BWREQ)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("T16 Timeout, reach bw req max retry\n");
                        }
                    }
                    else
                    {
                        dot16Ss->servingBs->requestBOCount ++;
                        if (dot16Ss->servingBs->requestBOCount >
                            dot16Ss->para.requestBOEnd)
                        {
                            dot16Ss->servingBs->requestBOCount =
                                dot16Ss->para.requestBOEnd;
                        }

                        dot16Ss->servingBs->requestBOValue =
                            RANDOM_nrand(dot16->seed) %
                            (int)pow(2.0, dot16Ss->servingBs->
                            requestBOCount);

                        if (DEBUG_BWREQ)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("T16 Timeout, select new backoff %d"
                                    " out of %d\n",
                                    dot16Ss->servingBs->requestBOValue,
                                    (int)pow(2.0, dot16Ss->servingBs->
                                    requestBOCount));
                        }
                    }

                    break;
                }
                case DOT16_TIMER_T18:
                {
                    dot16Ss->timers.timerT18 = NULL;
                    if (dot16Ss->servingBs->sbcRequestRetries <=
                        DOT16_MAX_SBC_RETRIES)
                    {
                        Message* sbcReqMgntMsg;
                        sbcReqMgntMsg = MacDot16SsBuildSbcRequest(
                                            node,
                                            dot16);
                        MacDot16SsEnqueueMgmtMsg(
                            node,
                            dot16,
                            DOT16_CID_BASIC,
                            sbcReqMgntMsg);

                        if (DEBUG_NETWORK_ENTRY || DEBUG)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("T18 timeout for the %d th time,"
                                   "start a new SBC-REQ try\n",
                                   dot16Ss->servingBs->sbcRequestRetries);
                        }

                    }
                    else
                    {
                        // stop listening to the channel
                        MacDot16StopListening(node,
                                      dot16,
                                      dot16Ss->servingBs->dlChannel);
                        //re-initilize MAC
                        MacDot16SsPerformNetworkEntry(node, dot16);
                        if (DEBUG_NETWORK_ENTRY || DEBUG)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("T18 timeout for the %d th time,"
                                   "restart MAC init\n",
                                   dot16Ss->servingBs->sbcRequestRetries);
                        }
                    }
                    break;
                }
                case DOT16_TIMER_T20:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T20 Timeout, cannot detect PHY preamble on"
                               "channel index %d, and switch to next "
                               "channel %d\n",
                               dot16Ss->chScanIndex,
                               (dot16Ss->chScanIndex + 1) %
                               dot16->numChannels);
                    }

                    dot16Ss->timers.timerT20 = NULL;

                    // Timeout of searching for  phy preamble on this
                    // channel Switch to another channel and search
                    // stop listening to the previous channel
                    MacDot16StopListening(node,
                                          dot16,
                                          dot16Ss->servingBs->dlChannel);

                    // switch to the next channel
                    dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                           dot16->numChannels;

                    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];

                    //MacDot16SsPerformNetworkEntry(node, dot16);
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_T21:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T21 Timeout, Though detcted PHY preambles,"
                               "cannot detect DL-MAP preamble on"
                               "channel index %d, and switch to "
                               "next channel %d\n",
                               dot16Ss->chScanIndex,
                               (dot16Ss->chScanIndex + 1) %
                               dot16->numChannels);
                    }
                    dot16Ss->timers.timerT21 = NULL;

                    // Timeout of searching DL-MAP on this channel
                    // Switch to another channel and search

                    // stop listening to the previous channel
                    MacDot16StopListening(node,
                                          dot16,
                                          dot16Ss->servingBs->dlChannel);

                    // switch to the next channel
                    dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                           dot16->numChannels;

                    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];

                    //MacDot16SsPerformNetworkEntry(node, dot16);
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }

                case DOT16e_TIMER_T41:
                {
                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf(" T41 timeout, to see if retry available\n");
                    }

                    dot16Ss->timers.timerT41 = NULL;

                    if (dot16Ss->hoNumRetry > 0 &&
                        dot16Ss->hoStatus == DOT16e_SS_HO_LOCAL_WaitHoRsp)
                    {
                        // build another MOB_MSHO_REQ
                        Message* msHoReq;

                        msHoReq = MacDot16eSsBuildMobMshoReqMsg(node,
                                                                dot16);
                        MacDot16SsEnqueueMgmtMsg(node,
                                                 dot16,
                                                 DOT16_CID_BASIC,
                                                 msHoReq);
                    }
                    else
                    {
                        dot16Ss->hoStatus =
                                DOT16e_SS_HO_LOCAL_RetryExhausted;

                        // start another timer T42 to waot for possible
                        // reply
                        if (dot16Ss->timers.timerT42 != NULL)
                        {
                            MESSAGE_CancelSelfMsg(
                                            node,
                                            dot16Ss->timers.timerT42);

                            dot16Ss->timers.timerT42 = NULL;
                        }
                        dot16Ss->timers.timerT42 =
                                MacDot16SetTimer(
                                                node,
                                                dot16,
                                                DOT16e_TIMER_T42,
                                                dot16Ss->para.t42Interval,
                                                NULL,
                                                0);
                    }

                    break;
                }

                case DOT16e_TIMER_T42:
                {
                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf(" T42 timeout, end the ho transaction\n");
                    }

                    dot16Ss->hoStatus = DOT16e_SS_HO_LOCAL_Done;
                    dot16Ss->hoStatus = DOT16e_SS_HO_None;

                    break;
                }
                case DOT16e_TIMER_T43:
                {
                    if (DEBUG_SLEEP)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf(" T43 timeout, MOB_SLP-REQ retransmit\n");
                    }
                    dot16Ss->timers.timerT43 = NULL;
                    for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
                    {
                        dot16Ss->psClassParameter[i].isWaitForSlpRsp = FALSE;
                    }

                    MacDot16eSsBuildMobSlpReqMsgIfNeeded(node, dot16);
                    break;
                }
                case DOT16e_TIMER_T44:
                {
                    if (DEBUG_NBR_SCAN)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T44 timeout, retransmit a new scan req\n");
                    }

                    dot16Ss->timers.timerT44 = NULL;
                    if (dot16Ss->scanReqRetry)
                    {
                        Message* mgmtMsg;

                        mgmtMsg = MacDot16eSsBuildMobScnReqMsg(
                                                    node,
                                                    dot16,
                                                    NULL);

                        MacDot16SsEnqueueMgmtMsg(node,
                                                 dot16,
                                                 DOT16_CID_BASIC,
                                                 mgmtMsg);

                        dot16Ss->nbrScanStatus =
                            DOT16e_SS_NBR_SCAN_WaitScnReq;
                    }
                    else
                    {
                        // cancel this scan req
                        dot16Ss->nbrScanStatus = DOT16e_SS_NBR_SCAN_None;
                    }

                    break;
                }
                case DOT16_TIMER_LostDlSync:
                {
                    // we lost downlink synchronization, need to reboot
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("Lost DL MAP timer timeout, restart "
                               "network entry procedure from channel %d\n",
                                dot16Ss->chScanIndex);
                    }
                    dot16Ss->timers.timerLostDl = NULL;
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_LostUlSync:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("Lost UL-MAP Timeout, cannot get ul map on"
                                "channel index %d, and switch to next "
                                "channel %d\n",
                                dot16Ss->chScanIndex,
                                (dot16Ss->chScanIndex + 1) %
                                dot16->numChannels);
                    }

                    dot16Ss->timers.timerLostUl = NULL;

                    // Timeout of gettting ul-map on this channel
                    // Switch to another channel and search
                    // stop listening to the previous channel

                    MacDot16StopListening(node,
                                          dot16,
                                          dot16Ss->servingBs->dlChannel);

                    // switch to the next channel
                    dot16Ss->chScanIndex = (dot16Ss->chScanIndex + 1) %
                                           dot16->numChannels;

                    dot16Ss->servingBs->dlChannel =
                        dot16->channelList[dot16Ss->chScanIndex];

                    //MacDot16SsPerformNetworkEntry(node, dot16);
                    MacDot16SsRestartOperation(node, dot16);

                    break;
                }
                case DOT16_TIMER_FrameUlBegin:
                {
                    // beginning of the UL part of a MAC frame
                    MacDot16StopListening(node,
                                          dot16,
                                          dot16->lastListeningChannel);

                    MacDot16SsTimeoutServiceFlow(node, dot16);

                    // determine the bw request for each queue whose
                    // request can be sent in this round ul burst

                    MacDot16SsRefreshBandwidthReq(node, dot16);

                    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled &&
                        dot16Ss->operational)
                    {
                        int i;
                        MacDot16ePSClasses* psClass = NULL;
                        //MacDot16eSsPerformSleepModeOpoeration(node, dot16);
                        MacDot16eSsBuildMobSlpReqMsgIfNeeded(node, dot16);
                        // incr the parameter last received value
                        for (i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
                        {
                            psClass = &dot16Ss->psClassParameter[i];
                            if ((psClass->numOfServiceFlowExists > 0) &&
                                (psClass->statusNeedToChange == FALSE) &&
                                (psClass->currentPSClassStatus ==
                                POWER_SAVING_CLASS_DEACTIVATE ||
                                psClass->currentPSClassStatus ==
                                POWER_SAVING_CLASS_STATUS_NONE))
                            {
                                psClass->psClassIdleLastNoOfFrames++;
                            }
                        }// end of for
                    }


                    if (dot16Ss->bwReqType != DOT16_BWReq_CDMA &&
                        dot16Ss->contBwReqOpp == TRUE &&
                        dot16Ss->bwReqInContOpp == NULL &&
                        dot16Ss->bwReqHead != NULL)
                    {
                        // only contention based vailable
                        // copy a first non UGS and non rtPs req to the
                        // bwReqInContOpp
                        MacDot16SsBWReqRecord* curRec = dot16Ss->bwReqHead;
                        MacDot16SsBWReqRecord* lastRec = dot16Ss->bwReqHead;
                        while (curRec != NULL)
                        {
                            if (curRec->serviceType != DOT16_SERVICE_rtPS)
                            {
                                dot16Ss->bwReqInContOpp = curRec;
                                dot16Ss->servingBs->requestRetries = 0;
                                dot16Ss->servingBs->requestBOCount =
                                            dot16Ss->para.requestBOStart;

                                dot16Ss->servingBs->requestBOValue =
                                    RANDOM_nrand(dot16->seed) %
                                    (int)pow(2.0, dot16Ss->servingBs->
                                    requestBOCount);

                                if (DEBUG_BWREQ)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("select init new backoff %d out"
                                           " of %d\n", dot16Ss->servingBs->
                                           requestBOValue, (int)pow(2.0,
                                            dot16Ss->servingBs->
                                            requestBOCount));
                                }
                                if (curRec == dot16Ss->bwReqHead)
                                {
                                    // if the first one matches
                                    dot16Ss->bwReqHead = curRec->next;
                                    if (dot16Ss->bwReqHead == NULL)
                                    {
                                        dot16Ss->bwReqTail = NULL;
                                    }
                                }
                                else if (curRec == dot16Ss->bwReqTail)
                                {
                                    // if the last one matches
                                    dot16Ss->bwReqTail = lastRec;
                                    dot16Ss->bwReqTail->next = NULL;
                                }
                                else
                                {
                                    // middle one matches
                                    lastRec->next= curRec->next;
                                }
                                break;
                            }
                            lastRec = curRec;
                            curRec = curRec->next;
                        } // end of while
                    }

                    break;
                }

                case DOT16_TIMER_FrameEnd:
                {
                    // end of the UL part of a MAC frame

                    MacDot16SsBWReqRecord* bwReqRecord;
                    int i;
                    MacDot16ServiceFlow* sFlow;
                    MacDot16UlBurst* burst;

                    // check if we need to start nbr scan
                    if (dot16->dot16eEnabled)
                    {

                        if (dot16Ss->isSleepModeEnabled)
                        {
                            if (dot16Ss->isSleepModeEnabled &&
                                dot16Ss->operational)
                            {
                                MacDot16eSsPerformSleepModeOpoeration(node,
                                    dot16);
                            }
                        }// end of if

                        if (dot16Ss->hoStatus == DOT16e_SS_HO_HoIndSent)
                        {
                            // switch from current serving BS to target HO
                            // BS
                            if (DEBUG_HO)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                 printf("---handover to target BS---\n");
                            }

                            MacDot16eSsFinishHandover(node, dot16);
                        }
                        if (!(dot16->dot16eEnabled &&
                            dot16Ss->isSleepModeEnabled &&
                            MacDot16eIsPSClassActive(dot16,
                            POWER_SAVING_CLASS_3)
                            == TRUE))
                        {
                            MacDot16eSsUpdateNbrScan(node, dot16);
                        }
                    }
                    else
                    {
                        MacDot16StartListening(
                            node,
                            dot16,
                            dot16Ss->servingBs->dlChannel);
                    }
                    dot16Ss->timers.frameEnd =
                        MacDot16SetTimer(node,
                                         dot16,
                                         DOT16_TIMER_FrameEnd,
                                         dot16Ss->servingBs->frameDuration -
                                         dot16Ss->para.rtg,
                                         NULL,
                                         0);

                    // free the UL burst info
                    burst = dot16Ss->ulBurstHead;
                    while (burst != NULL)
                    {
                        dot16Ss->ulBurstHead = burst->next;
                        MEM_free(burst);
                        burst = dot16Ss->ulBurstHead;
                    }
                    dot16Ss->ulBurstHead = NULL;
                    dot16Ss->ulBurstTail = NULL;

                    // free the bandiwdht request did not send out in
                    // this round of ul burst
                    if (dot16Ss->bwReqType != DOT16_BWReq_CDMA)
                    {
                        while (dot16Ss->bwReqHead != NULL)
                        {
                            bwReqRecord =  dot16Ss->bwReqHead;

                            // Free the record
                            dot16Ss->bwReqHead = bwReqRecord->next;
                            if (dot16Ss->bwReqHead == NULL)
                            {
                                dot16Ss->bwReqTail = NULL;
                            }
                            if (bwReqRecord != NULL)
                            {
                                MEM_free(bwReqRecord);
                            }
                        }
                    }
                    // reset the bandwidth granted in this round
                    dot16Ss->basicMgmtBwGranted = 0;
                    dot16Ss->priMgmtBwGranted = 0;
                    dot16Ss->secMgmtBwGranted = 0;
                    dot16Ss->aggBWGranted = 0;

                    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
                    {
                        sFlow = dot16Ss->ulFlowList[i].flowHead;

                        while (sFlow != NULL)
                        {
                            if (sFlow->bwGranted  && sFlow->activated)
                            {
                              sFlow->bwGranted = 0;
                            }
                            sFlow = sFlow->next;
                        } // while
                    } // for i

                    // to see if any packet left in the out buffer due to
                    // dynamic reason
                    Message* tmpMsg = dot16Ss->outBuffHead;
                    dot16Ss->bwRequestedForOutBuffer = 0;
                    while (tmpMsg)
                    {
                        dot16Ss->bwRequestedForOutBuffer +=
                            MESSAGE_ReturnPacketSize(tmpMsg);
                        tmpMsg = tmpMsg->next;
                    }
                    if (dot16Ss->bwRequestedForOutBuffer)
                    {
                        // insert a bandwidht request to bw-req list
                        // treat as basic mgmt bw req
                        MacDot16SsEnqueueBandwidthRequest(
                            node,
                            dot16,
                            dot16Ss->servingBs->basicCid,
                            dot16Ss->bwRequestedForOutBuffer,
                            DOT16_SERVICE_nrtPS,
                            FALSE,
                            0,
                            DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY);
                    }

                    if (dot16->dot16eEnabled && dot16Ss->isIdleEnabled)
                    {
                        if ((node->getNodeTime() - dot16Ss->lastCSPacketRcvd) >=
                            DOT16e_CS_INIT_IDLE_DELAY)
                        {
                            // SS build packet to enter in to idle mode
                            MacDot16eSsInitiateIdleMode(node, dot16);
                        }
                    }
                    if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled
                        && dot16Ss->isPowerDown == TRUE)
                    {
                        dot16Ss->servingBs->frameNumber++;
                    }
                    if (DEBUG_BWREQ)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("after UL frame end recalculate the bw req"
                               "for outbuff, this time need %d\n",
                               dot16Ss->bwRequestedForOutBuffer);
                    }

                    break;
                }

                case DOT16_TIMER_Ranging:
                {
                    MacDot16UlBurst* burst;

                    burst = MacDot16SsGetUlBurst(node,
                                                 dot16,
                                                 timerInfo->info);

                    if (burst != NULL)
                    {
                        // tx the RNG-REQ
                        if (DEBUG_RANGING || DEBUG_NETWORK_ENTRY)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("init rng, send RNG-REQ after "
                                   "backoff\n");
                        }

                            MacDot16SsSendRangeRequest(
                                node,
                                dot16,
                                DOT16_INITIAL_RANGING_CID,
                                burst,
                                FALSE);
                        if (burst->uiuc == DOT16_UIUC_CDMA_RANGE)
                        {
                            MacDot16SsSendRangeRequest(
                                node,
                                dot16,
                                DOT16_INITIAL_RANGING_CID,
                                burst,
                                TRUE);
                        }
                        if (dot16Ss->initInfo.rngState ==
                            DOT16_SS_RNG_WaitInvitedRngRspForInitOpp ||
                            dot16Ss->initInfo.rngState ==
                            DOT16_SS_RNG_WaitBcstInitRngOpp)
                        {

                            dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitRngRspForInitOpp;
                        }
                        else
                        {
                            dot16Ss->initInfo.rngState =
                                DOT16_SS_RNG_WaitRngRsp;
                        }
                    }
                    break;
                }

                case DOT16_TIMER_BandwidthRequest:
                {
                    MacDot16UlBurst* burst;

                    burst =
                        MacDot16SsGetUlBurst(node, dot16, timerInfo->info);

                        // tx the bandwidth request header
                        if (burst != NULL && dot16Ss->bwReqInContOpp != NULL)
                        {
                            MacDot16SsSendBandwidthRequest(node, dot16, burst);
                        }
                    break;
                }

                case DOT16_TIMER_UlBurstStart:
                {
                    MacDot16UlBurst* burst;

                    if (DEBUG_TIMER)
                    {
                        printf("      UlBurstStart timer\n");
                    }

                    burst = MacDot16SsGetUlBurst(node,
                                                 dot16,
                                                 timerInfo->info);

                    if (burst != NULL)
                    {
                        if ((burst->uiuc == DOT16_UIUC_RANGE)
                            ||(burst->uiuc == DOT16_UIUC_CDMA_RANGE))
                        {
                            if (dot16Ss->bwReqType == DOT16_BWReq_CDMA
                                && burst->uiuc == DOT16_UIUC_CDMA_RANGE)
                            {
                                // send all ranging code for bw req
                                MacDot16SsBWReqRecord* bwRec =
                                    dot16Ss->bwReqHead;
                                while (bwRec != NULL)
                                {
                                    if (bwRec->isCDMAAllocationIERecvd == FALSE)
                                    {
                                        MacDot16SsSendCDMABandwidthRangingCode(
                                            node,
                                            dot16,
                                            burst,
                                            bwRec);
                                    }
                                    bwRec = bwRec->next;
                                }// end of while
                            }

                            if (dot16Ss->operational == FALSE  &&
                                dot16Ss->initInfo.initRangingCompleted
                                == FALSE)
                            {
                                 // initial range
                                 if (dot16Ss->initInfo.rngState ==
                                        DOT16_SS_RNG_BcstRngOppAlloc
                                    || dot16Ss->initInfo.rngState ==
                                        DOT16_SS_RNG_Backoff
                                    || dot16Ss->initInfo.rngState ==
                                    DOT16_SS_RNG_WaitInvitedRngRspForInitOpp
                                    ||  dot16Ss->initInfo.rngState ==
                                    DOT16_SS_RNG_WaitBcstInitRngOpp)
                                {
                                    // initial range in contention ranging
                                    // burst two case, a new backoff or SS
                                    // still in backoff
                                    if (dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_BcstRngOppAlloc ||
                                        dot16Ss->initInfo.rngState ==
                                        DOT16_SS_RNG_WaitInvitedRngRspForInitOpp)
                                    {
                                        if (dot16Ss->servingBs->rangeRetries
                                            == 0)
                                        {
                                            // first time to send inital
                                            // range decide the transmission
                                            // power refer to standeard
                                            // Pg176

                                            //max tx power value
                                            dot16Ss->servingBs->
                                                maxTxPowerInitRng =
                                                dot16Ss->para.bs_EIRP +
                                                dot16Ss->para.rssIRmax -
                                                dot16Ss->servingBs->rssMean;

                                            // current Tx power value for
                                            // init range
                                            if (dot16Ss->para.bs_EIRP == 0
                                                ||
                                                dot16Ss->para.rssIRmax == 0)
                                            {
                                                dot16Ss->servingBs->
                                                    curerntTxPower =
                                                    dot16Ss->para.
                                                    minTxPower;
                                            }
                                            else
                                            {
                                                dot16Ss->servingBs->
                                                    curerntTxPower =
                                                    dot16Ss->para.minTxPower
                                                    + (dot16Ss->servingBs->
                                                    maxTxPowerInitRng -
                                                    dot16Ss->para.minTxPower)
                                                    / 2;
                                            }

                                            // decide the initial backoff
                                            // window
                                            dot16Ss->servingBs->rangeBOCount
                                                = dot16Ss->para.
                                                rangeBOStart;

                                            dot16Ss->servingBs->rangeBOValue
                                                = RANDOM_nrand(dot16->seed)
                                                % (int)pow(2.0, dot16Ss->
                                                servingBs->rangeBOCount);

                                            if (DEBUG_NETWORK_ENTRY ||
                                                DEBUG_RANGING)
                                            {
                                                MacDot16PrintRunTimeInfo(
                                                                    node,
                                                                    dot16);

                                                printf("first time in "
                                                       "contention rng opp,"
                                                       " choosing backoff "
                                                       "value %d in max %d "
                                                       "and set Inti "
                                                       "Txpower %f\n",
                                                       dot16Ss->servingBs->
                                                       rangeBOValue,
                                                       (int)pow(2.0, dot16Ss
                                                       ->servingBs->
                                                       rangeBOCount),
                                                       dot16Ss->servingBs->
                                                       curerntTxPower);
                                            }
                                        }

                                        if (dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_WaitInvitedRngRspForInitOpp)
                                        {
                                            dot16Ss->initInfo.rngState =
                                                DOT16_SS_RNG_WaitBcstInitRngOpp;
                                        }
                                        else if (dot16Ss->rngType != DOT16_CDMA)
                                        {
                                            dot16Ss->initInfo.rngState =
                                                DOT16_SS_RNG_Backoff;
                                        }
                                    }

                                    if (dot16Ss->rngType == DOT16_CDMA &&
                                        ((burst->uiuc ==
                                        DOT16_UIUC_CDMA_RANGE &&
                                        dot16Ss->initInfo.rngState ==
                                        DOT16_SS_RNG_WaitBcstInitRngOpp)
                                        || (burst->uiuc ==
                                        DOT16_UIUC_CDMA_ALLOCATION_IE &&
                                        dot16Ss->initInfo.rngState ==
                                        DOT16_SS_RNG_WaitBcstRngOpp)))
                                    {
                                        // tx the initial RNG-REQ
                                        MacDot16SsSendRangeRequest(
                                            node,
                                            dot16,
                                            DOT16_INITIAL_RANGING_CID,
                                            burst,
                                            FALSE);
                                        if (dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_WaitBcstInitRngOpp)
                                        {
                                            MacDot16SsSendRangeRequest(
                                                node,
                                                dot16,
                                                DOT16_INITIAL_RANGING_CID,
                                                burst,
                                                TRUE);
                                        }
                                        if (dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_WaitInvitedRngRspForInitOpp
                                            || dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_WaitBcstInitRngOpp)
                                        {

                                            dot16Ss->initInfo.rngState =
                                                DOT16_SS_RNG_WaitRngRspForInitOpp;
                                        }
                                        else
                                        {
                                            dot16Ss->initInfo.rngState =
                                                DOT16_SS_RNG_WaitRngRsp;
                                        }
                                    }
                                    else
                                    {
                                        // burst for initial ranging
                                        int numTxOpps =
                                                burst->duration /
                                                dot16Ss->para.rangeOppSize;

                                        if (numTxOpps <=
                                            dot16Ss->servingBs->rangeBOValue)
                                        {
                                            dot16Ss->servingBs->rangeBOValue
                                                -= numTxOpps;
                                        }
                                        else
                                        {
                                            // re-calculate the start time
                                            MacDot16SsRecomputeStartTime(
                                                node,
                                                dot16,
                                                burst,
                                                dot16Ss->servingBs->rangeBOValue
                                                * dot16Ss->para.rangeOppSize,
                                                dot16Ss->para.rangeOppSize);

                                            if (dot16Ss->idleSubStatus ==
                                               DOT16e_SS_IDLE_MSListen &&
                                               !dot16Ss->performLocationUpdate)
                                            {
                                                //do nothing here
                                                // need to through assert
                                            }
                                            else if (burst->startTime <=
                                                node->getNodeTime())
                                            {
                                                if (DEBUG_NETWORK_ENTRY ||
                                                    DEBUG_RANGING)
                                                {
                                                    MacDot16PrintRunTimeInfo(
                                                                node,
                                                                dot16);
                                                    printf("Init rng contention"
                                                            " opp, backoff "
                                                            "value reach %d in"
                                                            " opp %d,transmit"
                                                            " RNG-REQ\n",
                                                            dot16Ss->servingBs
                                                            ->rangeBOValue,
                                                            numTxOpps);
                                                }

                                                // tx the initial RNG-REQ
                                                MacDot16SsSendRangeRequest(
                                                    node,
                                                    dot16,
                                                    DOT16_INITIAL_RANGING_CID,
                                                    burst,
                                                    FALSE);
                                                dot16Ss->initInfo.rngState =
                                                    DOT16_SS_RNG_WaitRngRsp;
                                            }
                                            else
                                            {
                                                if (dot16Ss->idleSubStatus ==
                                                   DOT16e_SS_IDLE_MSListen &&
                                                !dot16Ss->performLocationUpdate)
                                                {
                                                    //do nothing here
                                                    // need to through assert
                                                }
                                                else
                                                {
                                                // set the timer for txing
                                                // RNG-REQ
                                                delay = burst->startTime -
                                                        node->getNodeTime();
                                                MacDot16SetTimer(
                                                    node,
                                                    dot16,
                                                    DOT16_TIMER_Ranging,
                                                    delay,
                                                    NULL,
                                                    burst->index);
                                            }
                                        }
                                    }
                                    }
                                } //DOT16_SS_RNG_BcstRngOppAlloc
                                else if (dot16Ss->initInfo.rngState ==
                                            DOT16_SS_RNG_RngInvited)
                                {
                                    if (dot16Ss->idleSubStatus ==
                                       DOT16e_SS_IDLE_MSListen &&
                                       !dot16Ss->performLocationUpdate)
                                    {
                                        //do nothing here
                                        // need to through assert
                                    }
                                    else
                                    {
                                    if (DEBUG_NETWORK_ENTRY ||
                                        DEBUG_RANGING)
                                    {
                                        MacDot16PrintRunTimeInfo(
                                                                node,
                                                                dot16);

                                        printf("polled/unicsat invited init"
                                               " rng opp\n");
                                    }

                                    // initial range with basic assigned
                                    MacDot16SsSendRangeRequest(
                                            node,
                                            dot16,
                                            dot16Ss->servingBs->basicCid,
                                            burst,
                                            FALSE);
                                   dot16Ss->initInfo.rngState =
                                        DOT16_SS_RNG_WaitRngRsp;

                                    dot16Ss->servingBs->rangeRetries = 0;
                                    dot16Ss->servingBs->rangeBOCount =
                                                dot16Ss->para.rangeBOStart;
                                    }
                                } // DOT16_SS_RNG_RngInvited
                            } // oprational == FALSE (initial range)
                            else if (burst->cid != DOT16_BROADCAST_CID)
                            {
                                // perioidic range
                                if (DEBUG_RANGING)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("periodic rng invited opp\n");
                                }

                                // determine whether to send a RNG-REQ or
                                // padding data Refer to pg. 202
                                if (dot16Ss->periodicRngInfo.toSendRngReq
                                    == TRUE &&
                                    dot16Ss->periodicRngInfo.rngReqScheduled
                                    == FALSE)
                                {
                                    if (dot16Ss->idleSubStatus ==
                                       DOT16e_SS_IDLE_MSListen &&
                                       !dot16Ss->performLocationUpdate)
                                    {
                                        //do nothing here
                                        // need to through assert
                                    }
                                    else
                                    {
                                    dot16Ss->periodicRngInfo.rngReqScheduled
                                                        = TRUE;
                                    if (dot16Ss->rngType != DOT16_CDMA)
                                    {
                                        // tx the perioidic RNG-REQ
                                        if (DEBUG_RANGING)
                                        {
                                            printf("Periodic invited rng, "
                                                   "needto send RNG-REQ\n");
                                        }
                                    }
                                    MacDot16SsSendRangeRequest(
                                        node,
                                        dot16,
                                        dot16Ss->servingBs->basicCid,
                                        burst,
                                        FALSE);
                                    dot16Ss->periodicRngInfo.lastAnomaly =
                                                        ANOMALY_NO_ERROR;
                                }
                                }
                                else
                                {
                                    if (DEBUG_RANGING)
                                    {
                                        printf("    periodic invited rng,"
                                               " last status is success, "
                                               "need to paddign data for "
                                               "this invited priodic rng "
                                               "opp\n");
                                    }

                                    if (burst->uiuc != DOT16_UIUC_CDMA_RANGE)
                                    {
                                        // padding
                                        MacDot16SsSendPaddedData(
                                                node,
                                                dot16,
                                                dot16Ss->servingBs->basicCid,
                                                burst);
                                    }
                                }
                            } // oprational == TRUE
                        } // Ranging UIUC
                        else if (burst->uiuc == DOT16_UIUC_REQUEST ||
                            (burst->uiuc == DOT16_UIUC_CDMA_RANGE &&
                            dot16Ss->bwReqType == DOT16_BWReq_CDMA))
                        {
                            if (dot16Ss->contBwReqOpp &&
                                dot16Ss->bwReqInContOpp != NULL &&
                                dot16Ss->isWaitBwGrant == FALSE)
                            {
                                // burst for bandwidth request
                                int numTxOpps = burst->duration /
                                            dot16Ss->para.requestOppSize;

                                if ((numTxOpps <=
                                    dot16Ss->servingBs->requestBOValue))
                                {
                                    dot16Ss->servingBs->requestBOValue -=
                                        numTxOpps;
                                }
                                else
                                {
                                    // re-calculate the start time
                                    MacDot16SsRecomputeStartTime(
                                        node,
                                        dot16,
                                        burst,
                                        dot16Ss->servingBs->requestBOValue
                                        * dot16Ss->para.requestOppSize,
                                        dot16Ss->para.requestOppSize);

                                    if (burst->startTime <=
                                        node->getNodeTime())
                                    {
                                        // tx the contention based BW
                                        // request
                                        MacDot16SsSendBandwidthRequest(
                                            node,
                                            dot16,
                                            burst);
                                    }
                                    else
                                    {
                                        // set the timer for txing BW
                                        // request
                                        delay = burst->startTime -
                                                node->getNodeTime();
                                        MacDot16SetTimer(
                                            node,
                                            dot16,
                                            DOT16_TIMER_BandwidthRequest,
                                            delay,
                                            NULL,
                                            burst->index);
                                    }
                                }
                            }
                            else if (burst->cid != DOT16_BROADCAST_CID
                                     && dot16Ss->ucastBwReqOpp
                                     && dot16Ss->bwReqHead != NULL
                                     && dot16Ss->bwReqType != DOT16_BWReq_CDMA)
                            {

                                Message* bwMsgList;
                                int bwMsgSize;
                                Dot16BurstInfo burstInfo;
                                int bitsPerPs;
                                int burstSize;
                                int durationInPs;

                                bitsPerPs =
                                    MacDot16PhyBitsPerPs(
                                        node,
                                        dot16,
                                        &(dot16Ss->servingBs->
                                          ulBurstProfile[
                                              DOT16_UIUC_MOST_RELIABLE - 1]),
                                        DOT16_UL);

                                burstSize = (burst->duration -
                                             dot16Ss->para.sstgInPs) *
                                             bitsPerPs / 8;

                                bwMsgList =
                                    MacDot16SsScheduleBandwidthRequest(
                                        node,
                                        dot16,
                                        burstSize,
                                        TRUE,
                                        &bwMsgSize);

                                if (bwMsgList != NULL)
                                {
                                    durationInPs =
                                        MacDot16SsBuildBurstInfo(
                                                                node,
                                                                dot16,
                                                                burst,
                                                                bwMsgSize,
                                                                &burstInfo);
                                    MacDot16AddBurstInfo(
                                                        node,
                                                        bwMsgList,
                                                        &burstInfo);

                                    // pass to PHY for transmission
                                    MacDot16PhyTransmitUlBurst(
                                            node,
                                            dot16,
                                            bwMsgList,
                                            dot16Ss->servingBs->ulChannel,
                                            durationInPs);
                                }
                            }
                        } // REQUEST UIUC
                        else
                        {
                            if (burst->uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE)
                            {
                                MacDot16SsSendBandwidthRequest(node,
                                    dot16, NULL);

                                Message* msg = dot16Ss->cdmaAllocationMsgList;
                                Dot16BurstInfo burstInfo;
                                int durationInPs;
                                if (msg != NULL)
                                {
                                    dot16Ss->cdmaAllocationMsgList =
                                        msg->next;
                                    msg->next = NULL;
                                    // add the burst information for PHY
                                    durationInPs = MacDot16SsBuildBurstInfo(
                                                       node,
                                                       dot16,
                                                       burst,
                                                       MESSAGE_ReturnPacketSize(msg),
                                                       &burstInfo);
                                    MacDot16AddBurstInfo(
                                        node,
                                        msg,
                                        &burstInfo);

                                    // start transmitting the burst on the UL channel
                                    MacDot16PhyTransmitUlBurst(
                                        node,
                                        dot16,
                                        msg,
                                        dot16Ss->servingBs->ulChannel,
                                        durationInPs);
                                }
                            }
                            else
                            {
                                //if (dot16Ss->needSendPaddingInDataBurst)
                                if (burst->needSendPaddingInDataBurst)
                                {
                                    //dot16Ss->needSendPaddingInDataBurst = FALSE;
                                    burst->needSendPaddingInDataBurst = FALSE;

                                    // padding
                                    MacDot16SsSendPaddedData(
                                            node,
                                            dot16,
                                            dot16Ss->servingBs->basicCid,
                                            burst);
                                }
                                else
                                {
                                    // burst for data transmission
                                    MacDot16SsTransmitUlBurst(
                                        node,
                                        dot16,
                                        burst);
                                }
                            }
                        } // DATA GRANT UIUC
                    }

                    break;
                }

                 case DOT16_ARQ_RETRY_TIMER:
                 case DOT16_ARQ_BLOCK_LIFETIME_TIMER:
                 case DOT16_ARQ_RX_PURGE_TIMER:
                 case DOT16_ARQ_DISCARD_RETRY_TIMER:
                 {
                     MacDot16ARQHandleTimeout(node,
                                             dot16,
                                             timerInfo);
                     break;
                 }
                 case DOT16_ARQ_SYNC_LOSS_TIMER:
                 {
                    MacDot16ServiceFlow* sFlow = NULL;
                    if (DEBUG_ARQ_TIMER)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DOT16_ARQ_SYNC_LOSS_TIMER expired. cid = %d\n",
                        (Dot16CIDType)timerInfo->info);
                    }
                    sFlow =MacDot16SsGetServiceFlowByCid(node,
                        dot16,
                        (Dot16CIDType)timerInfo->info);
                    if (sFlow == NULL)
                    {
                        if (PRINT_WARNING)
                        {
                            ERROR_ReportWarning("No service flow associated"
                                " with the timer expired \n");
                        }

                        //bug fix start #5373
                        MESSAGE_Free(node, msg);
                        //bug fix end #5373

                        return ;
                    }
                    sFlow->arqSyncLossTimer = NULL;
                    // to initiate reset message
                    MacDot16ARQBuildandSendResetMsg(node,
                        dot16,
                        (Dot16CIDType)timerInfo->info,
                        (UInt8)DOT16_ARQ_RESET_Initiator);
                    break;
                 }
                case DOT16_TIMER_T22:
                {
                    if (DEBUG_ARQ_TIMER)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DOT16_TIMER_T22 timer expired\n");
                    }
                    MacDot16ARQHandleResetRetryTimeOut(node, dot16,
                        (Dot16CIDType)timerInfo->info,
                        (UInt8)timerInfo->Info2);
                     break;
                }
                case DOT16e_TIMER_SlpPowerUp:
                {
                    if (DEBUG_SLEEP_PS_CLASS_3)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DOT16e_TIMER_SlpPowerUp "
                            "timer expired\n");
                    }
                    MacDot16ePSClasses* psClass;
                    for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
                    {
                        psClass = &dot16Ss->psClassParameter[i];
                        if (psClass->classType == POWER_SAVING_CLASS_3)
                        {
                            MacDot16eSetPSClassStatus(psClass,
                                POWER_SAVING_CLASS_DEACTIVATE);
                            psClass->isSleepDuration = FALSE;
                            psClass->isMobTrfIndReceived = FALSE;
                        }
                    }
                    dot16Ss->isPowerDown = FALSE;
                    MacDot16StartListening(
                        node,
                        dot16,
                        dot16Ss->servingBs->dlChannel);
                    break;
                }
                case DOT16e_TIMER_T45:
                {
                    dot16Ss->timers.timerT45 = NULL;
                    MacDot16eSsHandleT45Timeout(node, dot16);
                    break;
                }
                case DOT16e_TIMER_DregReqDuration:
                {
                    MacDot16eSsHandleDregReqDurationTimeout(node, dot16);
                    break;
                }
                case DOT16e_TIMER_LocUpd:
                {
                    dot16Ss->timers.timerLocUpd = NULL;
                    MacDot16eSsLocUpdTimeout(node, dot16, msg);
                    msgReused = TRUE;
                    break;
                }
                case DOT16e_TIMER_MSPagingUnavlbl:
                {
                    dot16Ss->timers.timerMSPagingUnavlbl = NULL;
                    MacDot16eSsMsPagingUnavlblIntervalTimeout(node, dot16);
                    break;
                }
                case DOT16e_TIMER_MSPaging:
                {
                    dot16Ss->timers.timerMSPaging = NULL;
                    MacDot16eSsMsPagingIntervalTimeout(node, dot16);
                    break;
                }
                default:
                {
                    char tmpString[MAX_STRING_LENGTH];
                    sprintf(tmpString,
                            "DOT16: Unknow timer type %d\n",
                            timerType);
                    ERROR_ReportError(tmpString);
                    break;
                }
            }
            break;
        }
        default:
        {
            char tmpString[MAX_STRING_LENGTH];
            sprintf(tmpString,
                    "DOT16: Unknow message event type %d\n",
                    msg->eventType);
            ERROR_ReportError(tmpString);
            break;
        }
    }

    if (!msgReused)
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: MacDot16SsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node: Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16SsFinalize(Node *node, int interfaceIndex)
{
    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    MacDot16SsPrintStats(node, dot16, interfaceIndex);
}

// /**
// FUNCTION   :: MacDot16SsReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16SsReceivePacketFromPhy(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
    MacDot16MacHeader* macHeader = NULL;
    unsigned char* payload = NULL;
    Message* tmpMsg = NULL;
    Message* nextMsg = NULL;
    clocktype rxBeginTime = node->getNodeTime();
    PhySignalMeasurement* signalMea = NULL;
    PhySignalMeasurement signalMeaVal = {0, 0};
    BOOL measValid = FALSE;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("recvd a packet with pktSize=%d\n",
               MESSAGE_ReturnPacketSize(msg));
    }

    if (dot16Ss->initInfo.initStatus == DOT16_SS_INIT_PreEntry)
    {
        // if SS have not start itself
        MESSAGE_FreeList(node, msg);
        return;
    }

    signalMea = (PhySignalMeasurement*) MESSAGE_ReturnInfo(msg);
    if (signalMea != NULL)
    {
        memcpy((char*)&signalMeaVal,
               (char*)signalMea,
               sizeof(PhySignalMeasurement));
        measValid = TRUE;
    }

    // update measurement if needed
    if (measValid)
    {
        rxBeginTime = signalMea->rxBeginTime;

        if (dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
        {
            // when not in scan mode, update now
            MacDot16SsUpdateBsMeasurement(node, dot16, &signalMeaVal);
        }

        if (dot16Ss->initInfo.dlSynchronized &&
            dot16Ss->nbrScanStatus != DOT16e_SS_NBR_SCAN_InScan)
        {
            // get the MAC header of the MAC PDU
            Dot16CIDType cid;
            unsigned char codeModuType;

            payload = (unsigned char*) MESSAGE_ReturnPacket(msg);
            macHeader = (MacDot16MacHeader*) payload;
            cid = MacDot16MacHeaderGetCID(macHeader);
            codeModuType = signalMeaVal.fecCodeModuType;

            if (MacDot16MacHeaderGetHT(macHeader) == 0 &&
                MacDot16IsMyCid(node, dot16, cid) &&
               (MacDot16IsBasicCid(cid) ||
                MacDot16IsPrimaryCid(cid) ||
                MacDot16IsTransportCid(node, dot16, cid)))
            {
                // ucast
                MacDot16SsCheckDlIntervalUsageCode(node,
                                                   dot16,
                                                   TRUE,
                                                   codeModuType);
            }
            else if (MacDot16MacHeaderGetHT(macHeader) == 0 &&
                     MacDot16IsMyCid(node, dot16, cid))
            {
                // bcast or mcast
                MacDot16SsCheckDlIntervalUsageCode(node,
                                                   dot16,
                                                   FALSE,
                                                   codeModuType);
            }
        }
    }

    // if SS is still detecting PHY preamble, then this is PHY indication
    if (dot16Ss->timers.timerT20 != NULL)
    {
        //cancel the T20
        MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT20);
        dot16Ss->timers.timerT20 = NULL;

        if (dot16Ss->timers.timerT21 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, dot16Ss->timers.timerT21);
            dot16Ss->timers.timerT21 = NULL;
        }
        //set a new T21
        dot16Ss->timers.timerT21 = MacDot16SetTimer(
                                       node,
                                       dot16,
                                       DOT16_TIMER_T21,
                                       dot16Ss->para.t21Interval,
                                       NULL,
                                       0);
        if (DEBUG_NETWORK_ENTRY)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("PHY must be sycned since rcvd PDU on CH index %d,"
                   "cancel T20 & search for DL-MAP and start T21\n",
                   dot16Ss->chScanIndex);
        }
    }

    tmpMsg = msg;
    while (tmpMsg != NULL)
    {
        // get the MAC header of the MAC PDU
        payload = (unsigned char*) MESSAGE_ReturnPacket(tmpMsg);
        macHeader = (MacDot16MacHeader*) payload;
        if (MacDot16MacHeaderGetHT(macHeader) == 0)
        {
            // this is a generic MAC header
            if (MacDot16IsMyCid(node,
                                dot16,
                                MacDot16MacHeaderGetCID(macHeader)))
            {
                // this is my PDU
                nextMsg = MacDot16SsHandlePdu(node, dot16, tmpMsg, rxBeginTime);
            }
            else
            {
                Dot16CIDType cid;
                cid = MacDot16MacHeaderGetCID(macHeader);

                if (MacDot16IsManagementCid(node, dot16, cid))
                {
                    nextMsg = tmpMsg->next;
                    tmpMsg->next = NULL;
                    // this is not a PDU for me, skip it
                    MESSAGE_Free(node, tmpMsg);
                }
                else
                {
                        // this is not a PDU for me, skip it
                        int pduLength = MacDot16MacHeaderGetLEN(macHeader);
                        while (pduLength > 0)
                        {
                            nextMsg = tmpMsg->next;
                            tmpMsg->next = NULL;
                            pduLength -= MESSAGE_ReturnPacketSize(tmpMsg);
                            MESSAGE_Free(node, tmpMsg);
                            tmpMsg = nextMsg;
                        }
                }
            }
        }
        else
        {
            nextMsg = tmpMsg->next;
            tmpMsg->next = NULL;
            // this is a bandwidth request message
            // SS cannot handle BW request, skip it
            ERROR_ReportWarning("MAC 802.16: SS got a BW request PDU!");
            MESSAGE_Free(node, tmpMsg);
        }

        tmpMsg = nextMsg;
    }

    // for scan, update after handle the msg to see which nbr BS it is
    if (measValid && dot16Ss->nbrScanStatus == DOT16e_SS_NBR_SCAN_InScan)
    {
        MacDot16SsUpdateBsMeasurement(node, dot16, &signalMeaVal);
    }

}

// /**
// FUNCTION   :: MacDot16SsReceivePhyStatusChangeNotification
// LAYER      :: MAC
// PURPOSE    :: React to change in physical status.
// PARAMETERS ::
// + node             : Node*         : Pointer to node.
// + dot16            : MacDataDot16* : Pointer to Dot16 structure
// + oldPhyStatus     : PhyStatusType : The previous physical state
// + newPhyStatus     : PhyStatusType : New physical state
// + receiveDuration  : clocktype     : The receiving duration
// RETURN     :: void : NULL
// **/
void MacDot16SsReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    // The PHY model may automatically start listening to the
    // the channel it just transmitted on after finishing the transmission.
    // This will cause a SS to detect and receive signals from other SSs.
    //

    // We explicitly stop this.
    if (oldPhyStatus == PHY_TRANSMITTING)
    {
        MacDot16StopListening(node, dot16, dot16Ss->servingBs->ulChannel);
    }
}
