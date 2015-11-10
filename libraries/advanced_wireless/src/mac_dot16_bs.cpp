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
#include <string>

#include "api.h"
#include "network.h"
#include "network_ip.h"

#include "mac_dot16.h"
#include "mac_dot16e.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_phy.h"
#include "mac_dot16_sch.h"
#include "phy_dot16.h"
#include "dot16_backbone.h"

#define DEBUG              0
#define DEBUG_TIMER        0
#define DEBUG_PARAMETER    0
#define DEBUG_CHANNEL      0
#define DEBUG_PACKET       0
#define DEBUG_PACKET_DETAIL 0
#define DEBUG_FLOW         0
#define DEBUG_SCHEDULING   0
#define DEBUG_HO           0
#define DEBUG_HO_DETAIL    0
#define DEBUG_NBR_SCAN     0
#define DEBUG_NETWORK_ENTRY  0
#define DEBUG_INIT_RANGE   0
#define DEBUG_PERIODIC_RANGE 0
#define DEBUG_BWREQ        0

#define DEBUG_ADMISSION_CONTROL        0
#define DEBUG_PACKING_FRAGMENTATION    0
#define DEBUG_CRC                      0
#define DEBUG_CDMA_RANGING             0
#define DEBUG_SLEEP                    0// && (node->nodeId == 3)
#define DEBUG_SLEEP_PS_CLASS_3         0
#define DEBUG_ARQ                      0// && (node->nodeId == 0)
#define DEBUG_ARQ_INIT                 0
#define DEBUG_ARQ_WINDOW               0
#define DEBUG_ARQ_TIMER                0// && (node->nodeId == 0)
#define DEBUG_IDLE                     0

#define PRINT_WARNING      0

// /**
// FUNCTION   :: MacDot16BsPrintParameter
// LAYER      :: MAC
// PURPOSE    :: Print system parameters
// PARAMETERS ::
// + node : Node* : Poointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16BsPrintParameter(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    char clockStr[MAX_STRING_LENGTH];

    printf("Node%u interface %d MAC 802.16 BS parameters:\n",
           node->nodeId, dot16->myMacData->interfaceIndex);

    printf("    Downlink Channel = %d\n", dot16Bs->dlChannel);
    printf("    Uplink Channel = %d\n", dot16Bs->ulChannel);

    ctoa(dot16Bs->para.frameDuration, clockStr);
    printf("    Frame duration = %s\n", clockStr);

    if (dot16->duplexMode == DOT16_TDD)
    {
        ctoa(dot16Bs->para.tddDlDuration,clockStr);
        printf("    TDD DL duration = %s\n", clockStr);
    }

    ctoa(dot16Bs->para.ttg, clockStr);
    printf("    Transmit/receive transition gap = %s\n", clockStr);

    ctoa(dot16Bs->para.rtg, clockStr);
    printf("    Receive/transmit transition gap = %s\n", clockStr);

    ctoa(dot16Bs->para.sstg, clockStr);
    printf("    Subscriber station transition gap = %s\n", clockStr);
    printf("    Subscriber station transition gap in PS = %d\n",
           dot16Bs->para.sstgInPs);

    ctoa(dot16Bs->para.dcdInterval, clockStr);
    printf("    DCD interval = %s\n", clockStr);

    ctoa(dot16Bs->para.ucdInterval, clockStr);
    printf("    UCD interval = %s\n", clockStr);

    printf("    Ranging backoff start = %d\n",
           dot16Bs->para.rangeBOStart);
    printf("    Ranging backoff end = %d\n",
           dot16Bs->para.rangeBOEnd);
    printf("    BW request backoff start = %d\n",
           dot16Bs->para.requestBOStart);
    printf("    BW request backoff end = %d\n",
           dot16Bs->para.requestBOEnd);

    ctoa(dot16Bs->para.flowTimeout, clockStr);
    printf("    Service flow timeout = %s\n", clockStr);

    if (dot16Bs->isARQEnabled )
    {
        MacDot16PrintARQParameter(dot16Bs->arqParam);
    }
}


 /* Parse a paging group id list from a string formatted
 * as "[0 4 5]".
 */
/*
static
int MacDot16BsParsePagingGroupId(
    char* pagingGrpId,
    UInt16* groupIds)
{
    int     i   = 0;
    int     strLength;
    char    key[MAX_STRING_LENGTH];
    char    errorString[MAX_STRING_LENGTH];
    char*   next;


    strLength = ( int )strlen( pagingGrpId );

    if (pagingGrpId[i] == '[' && pagingGrpId[strLength - 1] == ']'){
        pagingGrpId++; // skip '['
        IO_Chop( pagingGrpId );  // skip ']'

        next = pagingGrpId;
        while (*next != '\0'){
            IO_GetToken( key,next,&next );
            if (IO_IsStringNonNegativeInteger(key)){
                groupIds[i++] = (UInt16) strtoul(key, NULL, 10);
            }
            else{
                sprintf( errorString,
                    "MAC-802.16e-BS-PAGING-GROUP-ID has an"
                        "invalid value, %s\n",
                    pagingGrpId );
                ERROR_ReportWarning( errorString );
            }
        }
        return i;
    }
    else{
        sprintf( errorString,
            "MAC-802.16e-BS-PAGING-GROUP-ID is incorrectly "
                "formatted, %s\n",
            pagingGrpId );
        ERROR_ReportWarning( errorString );
    }

    return 0;
}*/

// /**
// FUNCTION   :: MacDot16BsInitParameter
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
void MacDot16BsInitParameter(
         Node* node,
         const NodeInput* nodeInput,
         int interfaceIndex,
         MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    BOOL wasFound = FALSE;
    clocktype timeVal = 0;
    int intVal = 0;
    double doubleVal = 0;
    char buff[MAX_STRING_LENGTH] = {0, 0};
    char stringVal[MAX_STRING_LENGTH] = {0, 0};

    // Read configuration parameters
    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-FRAME-DURATION",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= 0)
        {
            ERROR_ReportWarning("MAC 802.16: MAC-802.16-BS-FRAME-DURATION "
                                "must be longer than 0!");
            dot16Bs->para.frameDuration = DOT16_BS_DEFAULT_FRAME_DURATION;
        }
        else
        {
            dot16Bs->para.frameDuration = timeVal;
        }
    }
    else
    {
        dot16Bs->para.frameDuration = DOT16_BS_DEFAULT_FRAME_DURATION;
    }

    // set and validate the frame duration
    MacDot16PhySetFrameDuration(node, dot16, dot16Bs->para.frameDuration);

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-TDD-DL-DURATION",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= 0)
        {
            ERROR_ReportWarning("MAC 802.16: MAC-802.16-BS-TDD-DL-DURATION "
                                "must be longer than 0!");
            dot16Bs->para.tddDlDuration = DOT16_BS_DEFAULT_TDD_DL_DURATION;
        }
        else
        {
            dot16Bs->para.tddDlDuration = timeVal;
        }
    }
    else
    {
        dot16Bs->para.tddDlDuration = DOT16_BS_DEFAULT_TDD_DL_DURATION;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-TTG",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= DOT16_PHY_RADIO_TURNAROUND_TIME)
        {
            ERROR_ReportWarning("MAC 802.16: MAC-802.16-BS-TTG must be "
                                "longer than radio turnaround time (2US)!");
            dot16Bs->para.ttg = DOT16_BS_DEFAULT_TTG;
        }
        else
        {
            dot16Bs->para.ttg = timeVal;
        }
    }
    else
    {
        dot16Bs->para.ttg = DOT16_BS_DEFAULT_TTG;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-RTG",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= DOT16_PHY_RADIO_TURNAROUND_TIME)
        {
            ERROR_ReportWarning("MAC 802.16: MAC-802.16-BS-RTG must be "
                                "longer than radio turnaround time (2US)!");
            dot16Bs->para.rtg = DOT16_BS_DEFAULT_RTG;
        }
        else
        {
            dot16Bs->para.rtg = timeVal;
        }
    }
    else
    {
        dot16Bs->para.rtg = DOT16_BS_DEFAULT_RTG;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-SSTG",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal <= DOT16_PHY_RADIO_TURNAROUND_TIME)
        {
            ERROR_ReportWarning("MAC 802.16: MAC-802.16-BS-SSTG must be "
                                "longer than radio turnaround time (2US)!");
            dot16Bs->para.sstg = DOT16_BS_DEFAULT_SSTG;
        }
        else
        {
            dot16Bs->para.sstg = timeVal;
        }
    }
    else
    {
        dot16Bs->para.sstg = DOT16_BS_DEFAULT_SSTG;
    }

    // translate the SSTG to in terms of number of physical slots
    dot16Bs->para.sstgInPs =
        (int)(dot16Bs->para.sstg / dot16->ulPsDuration);
    if (dot16Bs->para.sstgInPs < 1)
    {
        dot16Bs->para.sstgInPs = 1;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-DCD-BROADCAST-INTERVAL",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal < dot16Bs->para.frameDuration)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-DCD-BROADCAST-INTERVAL must be "
                "longer than the frame duration!");
            dot16Bs->para.dcdInterval = DOT16_BS_DEFAULT_DCD_INTERVAL;
        }
        else if (timeVal > DOT16_BS_MAX_DCD_INTERVAL)
        {
            sprintf(buff,
                    "MAC802.16: MAC-802.16-BS-DCD-BROADCAST-INTERVAL must"
                    "be shorter than %d seconds.\n",
                    (int)(DOT16_BS_MAX_DCD_INTERVAL / SECOND));
            ERROR_ReportWarning(buff);
            dot16Bs->para.dcdInterval = DOT16_BS_DEFAULT_DCD_INTERVAL;
        }
        else
        {
            dot16Bs->para.dcdInterval = timeVal;
        }
    }
    else
    {
        dot16Bs->para.dcdInterval = DOT16_BS_DEFAULT_DCD_INTERVAL;
    }

    IO_ReadTime(node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-802.16-BS-UCD-BROADCAST-INTERVAL",
                &wasFound,
                &timeVal);

    if (wasFound)
    {
        if (timeVal < dot16Bs->para.frameDuration)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-UCD-BROADCAST-INTERVAL must be "
                "longer than the frame duration!");
            dot16Bs->para.ucdInterval = DOT16_BS_DEFAULT_UCD_INTERVAL;
        }
        else if (timeVal > DOT16_BS_MAX_UCD_INTERVAL)
        {
            sprintf(buff,
                    "MAC802.16: MAC-802.16-BS-UCD-BROADCAST-INTERVAL must"
                    " be shorter than %d seconds.\n",
                    (int)(DOT16_BS_MAX_UCD_INTERVAL / SECOND));
            ERROR_ReportWarning(buff);
            dot16Bs->para.ucdInterval = DOT16_BS_DEFAULT_UCD_INTERVAL;
        }
        else
        {
            dot16Bs->para.ucdInterval = timeVal;
        }
    }
    else
    {
        dot16Bs->para.ucdInterval = DOT16_BS_DEFAULT_UCD_INTERVAL;
    }

    IO_ReadInt(node,
               node->nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-802.16-BS-RANGING-BACKOFF-MIN",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        if (intVal < 0)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-RANGING-BACKOFF-MIN must be "
                "larger than 0!");
            dot16Bs->para.rangeBOStart = DOT16_RANGE_BACKOFF_START;
        }
        else
        {
            dot16Bs->para.rangeBOStart = (UInt8)intVal;
        }
    }
    else
    {
        dot16Bs->para.rangeBOStart = DOT16_RANGE_BACKOFF_START;
    }

    IO_ReadInt(node,
               node->nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-802.16-BS-RANGING-BACKOFF-MAX",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        if (intVal < dot16Bs->para.rangeBOStart)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-RANGING-BACKOFF-MAX must be "
                "no smaller than MAC-802.16-BS-RANGING-BACKOFF-MIN!");
            dot16Bs->para.rangeBOEnd = DOT16_RANGE_BACKOFF_END;
        }
        else
        {
            dot16Bs->para.rangeBOEnd = (UInt8)intVal;
        }
    }
    else
    {
        dot16Bs->para.rangeBOEnd = DOT16_RANGE_BACKOFF_END;
    }

    IO_ReadInt(node,
               node->nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-802.16-BS-BANDWIDTH-REQUEST-BACKOFF-MIN",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        if (intVal < 0)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-BANDWIDTH-REQUEST-BACKOFF-MIN "
                "must be larger than 0!");
            dot16Bs->para.requestBOStart = DOT16_REQUEST_BACKOFF_START;
        }
        else
        {
            dot16Bs->para.requestBOStart = (UInt8)intVal;
        }
    }
    else
    {
        dot16Bs->para.requestBOStart = DOT16_REQUEST_BACKOFF_START;
    }

    IO_ReadInt(node,
               node->nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-802.16-BS-BANDWIDTH-REQUEST-BACKOFF-MAX",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        if (intVal < dot16Bs->para.requestBOStart)
        {
            ERROR_ReportWarning(
                "MAC802.16: MAC-802.16-BS-BANDWIDTH-REQUEST-BACKOFF-MAX "
                "must be no smaller than "
                "MAC-802.16-BS-BANDWIDTH-REQUEST-BACKOFF-MIN!");
            dot16Bs->para.requestBOEnd = DOT16_REQUEST_BACKOFF_END;
        }
        else
        {
            dot16Bs->para.requestBOEnd = (UInt8)intVal;
        }
    }
    else
    {
        dot16Bs->para.requestBOEnd = DOT16_REQUEST_BACKOFF_END;
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
        if (timeVal <=0)
        {
            sprintf(buff,
                    "MAC802.16: MAC-802.16-SERVICE-FLOW-TIMEOUT-INTERVAL "
                    "must be greater than 0\n");
            ERROR_ReportWarning(buff);
            dot16Bs->para.flowTimeout =
                DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL;
        }
        else
        {
            dot16Bs->para.flowTimeout = timeVal;
        }
    }
    else
    {
        dot16Bs->para.flowTimeout =
            DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL;
    }

    // parameters below could be user configurable. But now supported now.
    dot16Bs->para.rangeOppSize = (UInt16)MacDot16PhyBytesToPs(
                                     node,
                                     dot16,
                                     DOT16_RANGE_REQUEST_SIZE,
                                     &(dot16Bs->ulBurstProfile[0]),
                                     DOT16_UL) +
                                     (UInt16)dot16Bs->para.sstgInPs;
    dot16Bs->para.requestOppSize = (UInt16)MacDot16PhyBytesToPs(
                                       node,
                                       dot16,
                                       sizeof(MacDot16MacHeader),
                                       &(dot16Bs->ulBurstProfile[0]),
                                       DOT16_UL) +
                                       (UInt16)dot16Bs->para.sstgInPs;

    //timer related
    dot16Bs->para.t7Interval = DOT16_DEFAULT_T7_INTERVAL;
    dot16Bs->para.t8Interval = DOT16_DEFAULT_T8_INTERVAL;
    dot16Bs->para.t9Interval = DOT16_BS_DEFAULT_T9_INTERVAL;
    dot16Bs->para.t10Interval = DOT16_DEFAULT_T10_INTERVAL;
    dot16Bs->para.rngRspProcInterval =
        DOT16_BS_DEFAULT_RNG_RSP_PROC_INTERVAL;

    dot16Bs->para.t27ActiveInterval =
        DOT16_BS_DEFAULT_T27_ACTIVE_INTERVAL;
    dot16Bs->para.t27IdleInterval =
        DOT16_BS_DEFAULT_T27_IDLE_INTERVAL;
    dot16Bs->para.t17Interval = DOT16_BS_DEFAULT_T17_INTERVAL;
    dot16Bs->para.ucdTransitionInterval =
        DOT16_BS_DEFAULT_UCD_TRANSTION_INTERVAL;
    dot16Bs->para.dcdTransitionInterval =
        DOT16_BS_DEFAULT_UCD_TRANSTION_INTERVAL;

    // dcd related
    dot16Bs->para.bs_EIRP = 10; // Be sure to give the right value
    dot16Bs->para.rssIRmax = 10; // Be sure to give the right value

    // mearement reprot interval
    dot16Bs->para.dlMeasReportInterval =
        DOT16_BS_DEFAULT_DL_MEASURE_REPORT_INTERVAL;
    //dsx related
    dot16Bs->para.dsxReqRetries = DOT16_DEFAULT_DSX_REQ_RETRIES;
    dot16Bs->para.dsxRspRetries = DOT16_DEFAULT_DSX_RSP_RETRIES;

    // PKM support
    dot16Bs->para.authPolicySupport = FALSE;

    // dot16e
    dot16Bs->para.resrcRetainTimeout = DOT16e_HO_SYTEM_RESOURCE_RETAIN_TIME;

    // As per section "6.3.22.1.2 MS scanning of neighbor BSs" 
    // When the Trigger Action in the DCD message in encoded as 0x3, the 
    // MS shall send the MOB_SCN-REQ message to the BS to begin the neighbor 
    // BS scanning process when the trigger condition is met
    // Thus NEIGHBOR-SCAN-RSS-TRIGGER should be read at BS and this value 
    // should be used while sending DCD.
    IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-802.16e-NEIGHBOR-SCAN-RSS-TRIGGER",
        &wasFound,
        &doubleVal);

    if (wasFound)
    {

        dot16Bs->para.nbrScanRssTrigger = doubleVal;
    }
    else
    {
        dot16Bs->para.nbrScanRssTrigger =
            DOT16e_DEFAULT_NBR_SCAN_RSS_TRIGGER;
    }

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

        dot16Bs->para.hoRssTrigger = doubleVal;
    }
    else
    {
        dot16Bs->para.hoRssTrigger = DOT16e_DEFAULT_HO_RSS_TRIGGER;
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

        dot16Bs->para.hoRssMargin = doubleVal;
    }
    else
    {
        dot16Bs->para.hoRssMargin = DOT16e_DEFAULT_HO_RSS_MARGIN;
    }

     // Admission control unit algorithm name
    dot16Bs->acuAlgorithm = DOT16_ACU_NONE;
    dot16Bs->isCRCEnabled = DOT16_CRC_STATUS;
    dot16Bs->isFragEnabled = DOT16_FRAGMENTATION_STATUS;
    dot16Bs->isPackingEnabled = FALSE;
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
            dot16Bs->isPackingEnabled = TRUE;
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

    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-BS-ADMISSION-CONTROL-SCHEME",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "BASIC") == 0)
        {
            dot16Bs->acuAlgorithm = DOT16_ACU_BASIC;
        }
        else if (strcmp(stringVal, "NONE") != 0)
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-BS-ADMISSION-CONTROL-SCHEME "
                    "shoudle be NONE or DEFAULT. Use default value as NONE.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
    }

    IO_ReadDouble(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-BS-MAX-ALLOWED-UPLINK-LOAD-LEVEL",
                  &wasFound,
                  &doubleVal);

    if (wasFound)
    {
        if (doubleVal > 1.0 || doubleVal < 0.0)
        {
            sprintf(buff,
                    "MAC-802.16-BS-MAX-ALLOWED-UPLINK-LOAD-LEVEL "
                    "Range of the variable is [0.0 - 1.0]. "
                    "Use.default value as %lf\n",
                    DOT16_DEFAULT_MAX_ALLOWED_UPLINK_LOAD_LEVEL);
            ERROR_ReportWarning(buff);
        }
        else
        {
            dot16Bs->maxAllowedUplinkLoadLevel = doubleVal;
        }
    }
    else
    {
        dot16Bs->maxAllowedUplinkLoadLevel =
            DOT16_DEFAULT_MAX_ALLOWED_UPLINK_LOAD_LEVEL;
    }

    IO_ReadDouble(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-BS-MAX-ALLOWED-DOWNLINK-LOAD-LEVEL",
                  &wasFound,
                  &doubleVal);

    if (wasFound)
    {
        if (doubleVal > 1.0 || doubleVal < 0.0)
        {
            sprintf(buff,
                    "MAC-802.16-Bs-MAX-ALLOWED-DOWNLINK-LOAD-LEVEL"
                    "Range of the variable is [0.0 - 1.0]. "
                    "Use.default value as %lf\n",
                    DOT16_DEFAULT_MAX_ALLOWED_DOWNLINK_LOAD_LEVEL);
            ERROR_ReportWarning(buff);
        }
        else
        {
            dot16Bs->maxAllowedDownlinkLoadLevel = doubleVal;
        }
    }
    else
    {
        dot16Bs->maxAllowedDownlinkLoadLevel =
            DOT16_DEFAULT_MAX_ALLOWED_DOWNLINK_LOAD_LEVEL;
    }

    dot16Bs->bwReqType = DOT16_BWReq_NORMAL;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-CONTENTION-BASED-BWREQ-TYPE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "CDMA") == 0)
        {
            dot16Bs->bwReqType = DOT16_BWReq_CDMA;
        }
        else if (strcmp(stringVal, "NORMAL") != 0)
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-CONTENTION-BASED-BWREQ-TYPE "
                    "shoudle be NORMAL or CDMA. Use default value as Normal.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
    }

    dot16Bs->rngType = DOT16_NORMAL;
    IO_ReadString(node,
                  node->nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-802.16-RANGING-TYPE",
                  &wasFound,
                  stringVal);

    if (wasFound)
    {
        if (strcmp(stringVal, "CDMA") == 0)
        {
            dot16Bs->rngType = DOT16_CDMA;
        }
        else if (strcmp(stringVal, "NORMAL") != 0)
        {
            sprintf(buff,
                "Node Id: %d, MAC-802.16-RANGING-TYPE "
                    "shoudle be NORMAL or CDMA. Use default value as Normal.\n",
                    node->nodeId);
            ERROR_ReportWarning(buff);
        }
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
            dot16Bs->isARQEnabled = TRUE;
        }
        else if (strcmp(stringVal, "NO") == 0)
        {
            // ARQ not enabled
            dot16Bs->isARQEnabled = FALSE;
        }
        else
        {
            ERROR_ReportWarning(
                "MAC802.16: Wrong value of MAC-802.16-ARQ-ENABLE, should"
                " be YES or NO. Use default value as NO.");

            // by default, ARQ not enabled
             dot16Bs->isARQEnabled = FALSE;
        }
    }
    else
    {
            // not configured. Assume ARQ not enabled by default
            dot16Bs->isARQEnabled = FALSE;
    }

    if (dot16Bs->isARQEnabled == TRUE)
    {
        dot16Bs->arqParam =
            (MacDot16ARQParam*) MEM_malloc(sizeof(MacDot16ARQParam));

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

                dot16Bs->arqParam->arqWindowSize =
                        DOT16_ARQ_DEFAULT_WINDOW_SIZE ;

            }
            else
            {
                dot16Bs->arqParam->arqWindowSize = intVal;
            }
        }
        else
        {
            //take default value
            dot16Bs->arqParam->arqWindowSize =
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
                dot16Bs->arqParam->arqRetryTimeoutTxDelay =
                                    DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
                dot16Bs->arqParam->arqRetryTimeoutRxDelay =
                                    DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
            }
            else
            {
                dot16Bs->arqParam->arqRetryTimeoutTxDelay = intVal/2;
                dot16Bs->arqParam->arqRetryTimeoutRxDelay = intVal/2 ;
            }
        }
        else
        {
            dot16Bs->arqParam->arqRetryTimeoutTxDelay =
                                DOT16_ARQ_DEFAULT_RETRY_TIMEOUT/2 ;
            dot16Bs->arqParam->arqRetryTimeoutRxDelay =
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
                dot16Bs->arqParam->arqBlockLifeTime =
                                    DOT16_ARQ_DEFAULT_RETRY_COUNT;
            }
            else
            {
                dot16Bs->arqParam->arqBlockLifeTime = intVal;
            }
        }
        else
        {
            dot16Bs->arqParam->arqBlockLifeTime =
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
            dot16Bs->arqParam->arqSyncLossTimeout =
                                DOT16_ARQ_DEFAULT_SYNC_LOSS_INTERVAL ;
            }
            else
            {
                dot16Bs->arqParam->arqSyncLossTimeout = intVal ;
            }
        }
        else
        {
            dot16Bs->arqParam->arqSyncLossTimeout =
                                    DOT16_ARQ_DEFAULT_SYNC_LOSS_INTERVAL;
        }

            dot16Bs->arqParam->arqDeliverInOrder =
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
                dot16Bs->arqParam->arqRxPurgeTimeout =
                                DOT16_ARQ_DEFAULT_RX_PURGE_TIMEOUT ;
            }
            else
            {
                dot16Bs->arqParam->arqRxPurgeTimeout = intVal ;
            }
        }
        else
        {
            dot16Bs->arqParam->arqRxPurgeTimeout =
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

                dot16Bs->arqParam->arqBlockSize =
                                DOT16_ARQ_DEFAULT_BLOCK_SIZE;
            }
            else
            {
                dot16Bs->arqParam->arqBlockSize = intVal;
            }
        }
        else
        {
            //take default value
            dot16Bs->arqParam->arqBlockSize = DOT16_ARQ_DEFAULT_BLOCK_SIZE;
        }

        MacDot16ARQConvertParam(dot16Bs->arqParam,
                                        dot16Bs->para.frameDuration);
    }// end of if (dot16Bs->isARQEnabled == TRUE )
    // init PS3 parameter
    dot16Bs->isSleepEnabled = DOT16e_BS_SLEEP_MODE_STATUS;
    if (dot16Bs->isSleepEnabled)
    {
        MacDot16ePSClasses* psClass3 = &dot16Bs->psClass3;
        memset(psClass3, 0, sizeof(MacDot16ePSClasses));
        psClass3->classType = POWER_SAVING_CLASS_3;
        psClass3->currentPSClassStatus = POWER_SAVING_CLASS_STATUS_NONE;
        psClass3->initialSleepWindow = DOT16e_DEFAULT_PS3_SLEEP_WINDOW;
        psClass3->finalSleepWindowBase =
            DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_BASE;
        psClass3->finalSleepWindowExponent =
            DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_EXPONENT;
        psClass3->psDirection = DOT16_DOWNLINK_SERVICE_FLOW;
    }

    dot16Bs->isIdleEnabled = DOT16e_BS_IDLE_MODE_STATUS;
    if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
    {
        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-802.16e-BS-PAGING-CONTROLLER",
                      &wasFound,
                      buff);

        if (wasFound)
        {
            BOOL nodeId;
            IO_ParseNodeIdHostOrNetworkAddress(
                                    buff,
                                    &dot16Bs->pagingController,
                                    &nodeId);

            ERROR_Assert(nodeId == FALSE, "");
        }
        else
        {
            dot16Bs->isIdleEnabled = FALSE;
        }
    }

    if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
    {
        //paging/idle mode
        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16e-BS-PAGING-CYCLE",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 1)
            {
                ERROR_ReportWarning(
                        "MAC802.16: MAC-802.16e-BS-PAGING-CYCLE "
                        "must be no smaller than "
                        "1!");
                dot16Bs->para.pagingCycle = DOT16e_BS_DEFAULT_PAGING_CYCLE;
            }
            else
            {
                dot16Bs->para.pagingCycle = (UInt16)intVal;
            }
        }
        else
        {
            dot16Bs->para.pagingCycle = DOT16e_BS_DEFAULT_PAGING_CYCLE;
        }

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16e-BS-PAGING-INTERVAL-LENGTH",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 1)
            {
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16e-BS-PAGING-INTERVAL-LENGTH "
                    "must be no smaller than "
                    "1!");
                dot16Bs->para.pagingInterval =
                            DOT16e_BS_DEFAULT_PAGING_INTERVAL_LENGTH;
            }
            else if (intVal > 5)
            {
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16e-BS-PAGING-INTERVAL-LENGTH "
                    "must be no greater than "
                    "5!");
                dot16Bs->para.pagingInterval =
                            DOT16e_BS_DEFAULT_PAGING_INTERVAL_LENGTH;
            }
            else
            {
                dot16Bs->para.pagingInterval = (UInt8)intVal;
            }
        }
        else
        {
            dot16Bs->para.pagingInterval =
                    DOT16e_BS_DEFAULT_PAGING_INTERVAL_LENGTH;
        }
        ERROR_Assert((dot16Bs->para.pagingInterval <
                        dot16Bs->para.pagingCycle),
                        "MAC802.16: MAC-802.16e-BS-PAGING-INTERVAL-LENGTH "
                        "must be no greater than "
                        "MAC-802.16e-BS-PAGING-CYCLE!");

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16e-BS-PAGING-OFFSET",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 1)
            {
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16e-BS-PAGING-OFFSET "
                    "must be no smaller than "
                    "1!");
                dot16Bs->para.pagingOffset = DOT16e_BS_DEFAULT_PAGING_OFFSET;
            }
            else
            {
                dot16Bs->para.pagingOffset = (UInt8)intVal;
            }
        }
        else
        {
            dot16Bs->para.pagingOffset = DOT16e_BS_DEFAULT_PAGING_OFFSET;
        }

        ERROR_Assert((dot16Bs->para.pagingOffset <
                        dot16Bs->para.pagingCycle),
                        "MAC802.16: MAC802.16: MAC-802.16e-BS-PAGING-OFFSET "
                        "must be no greater than "
                        "MAC-802.16e-BS-PAGING-CYCLE!");

        IO_ReadInt(node,
                   node->nodeId,
                   interfaceIndex,
                   nodeInput,
                   "MAC-802.16e-BS-PAGING-GROUP-ID",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            if (intVal < 1)
            {
                ERROR_ReportWarning(
                        "MAC802.16: MAC-802.16e-BS-PAGING-GROUP-ID "
                        "must be no smaller than "
                        "1!");
                dot16Bs->pagingGroup[0] = DOT16e_BS_DEFAULT_PAGING_GROUP_ID;
            }
            else
            {
                dot16Bs->pagingGroup[0] = (UInt16)intVal;
            }
        }
        else
        {
            dot16Bs->pagingGroup[0] = DOT16e_BS_DEFAULT_PAGING_GROUP_ID;
        }

        //findout if Paging controller is co-located on this BS
        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-802.16e-BS-IS-PAGING-CONTROLLER",
                      &wasFound,
                      buff);

        if (wasFound)
        {
            if (strcmp(buff, "YES") == 0)
            {
                dot16Bs->pgCtrlData = (MacDot16ePagingCtrl*) MEM_malloc(
                                        sizeof (MacDot16ePagingCtrl));
                memset(dot16Bs->pgCtrlData, 0, sizeof(MacDot16ePagingCtrl));
            }
            else if (strcmp(buff, "NO") == 0)
            {
                dot16Bs->pgCtrlData = NULL;
            }
            else 
            {
                dot16Bs->pgCtrlData = NULL;
                ERROR_ReportWarning(
                    "MAC802.16: MAC-802.16e-BS-IS-PAGING-CONTROLLER "
                    "value can be YES/NO: setting "
                    "the Parameter to Default value NO\n");
            }
        }
        else
        {
            dot16Bs->pgCtrlData = NULL;
        }


        dot16Bs->para.t46Interval = DOT16e_BS_DEFAULT_T46_INTERVAL;
        dot16Bs->para.tMgmtRsrcHldgInterval =
                                DOT16e_BS_MGMT_RSRC_HOLDING_TIMEOUT;
    }// end of paging
}

// /**
// FUNCTION   :: MacDot16BsInitBurstProfiles
// LAYER      :: MAC
// PURPOSE    :: Initialize downlink and uplink burst profiles
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput : const NodeInput* : Pointer to node input
// + interfaceIndex : int : Interface Index
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16BsInitBurstProfiles(
         Node* node,
         const NodeInput* nodeInput,
         int interfaceIndex,
         MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    // initialize the default burst profiles
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        // OFDMA specific burst profiles

    // DL burst profiles, only Convolutional Coding(CC) is supported
        // 7 different combinations of modulations and coding rates
        // Thresholds are in the unit of 0.25 dB

        // QPSK modulation, 1/2 coding rate
        dot16Bs->dlBurstProfile[0].ofdma.fecCodeModuType =
            PHY802_16_QPSK_R_1_2;
        dot16Bs->dlBurstProfile[0].ofdma.exitThreshold =  22;
        dot16Bs->dlBurstProfile[0].ofdma.entryThreshold = 24;

        // QPSK modulation, 3/4 coding rate
        dot16Bs->dlBurstProfile[1].ofdma.fecCodeModuType =
            PHY802_16_QPSK_R_3_4;
        dot16Bs->dlBurstProfile[1].ofdma.exitThreshold =  34;
        dot16Bs->dlBurstProfile[1].ofdma.entryThreshold = 36;

        // 16 QAM modulation, 1/2 coding rate
        dot16Bs->dlBurstProfile[2].ofdma.fecCodeModuType =
            PHY802_16_16QAM_R_1_2;
        dot16Bs->dlBurstProfile[2].ofdma.exitThreshold =  44;
        dot16Bs->dlBurstProfile[2].ofdma.entryThreshold = 46;

        // 16 QAM modulation, 3/4 coding rate
        dot16Bs->dlBurstProfile[3].ofdma.fecCodeModuType =
            PHY802_16_16QAM_R_3_4;
        dot16Bs->dlBurstProfile[3].ofdma.exitThreshold =  58;
        dot16Bs->dlBurstProfile[3].ofdma.entryThreshold = 60;

        // 64 QAM modulation, 1/2 coding rate
        dot16Bs->dlBurstProfile[4].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_1_2;
        dot16Bs->dlBurstProfile[4].ofdma.exitThreshold =  66;
        dot16Bs->dlBurstProfile[4].ofdma.entryThreshold = 68;

        // 64 QAM modulation, 2/3 coding rate
        dot16Bs->dlBurstProfile[5].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_2_3;
        dot16Bs->dlBurstProfile[5].ofdma.exitThreshold =  74;
        dot16Bs->dlBurstProfile[5].ofdma.entryThreshold = 76;

        // 64 QAM modulation, 3/4 coding rate
        dot16Bs->dlBurstProfile[6].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_3_4;
        dot16Bs->dlBurstProfile[6].ofdma.exitThreshold =  82;
        dot16Bs->dlBurstProfile[6].ofdma.entryThreshold = 84;

        // BPSK modulation
        dot16Bs->dlBurstProfile[7].ofdma.fecCodeModuType =
            PHY802_16_BPSK;
        dot16Bs->dlBurstProfile[7].ofdma.exitThreshold =  22;
        dot16Bs->dlBurstProfile[7].ofdma.entryThreshold = 24;

        // UL burst profiles are same as DL burst profiles in
        // this implementation.
        // QPSK modulation, 1/2 coding rate
        dot16Bs->ulBurstProfile[0].ofdma.fecCodeModuType =
            PHY802_16_QPSK_R_1_2;
        dot16Bs->ulBurstProfile[0].ofdma.exitThreshold =  22;
        dot16Bs->ulBurstProfile[0].ofdma.entryThreshold = 24;

        // QPSK modulation, 3/4 coding rate
        dot16Bs->ulBurstProfile[1].ofdma.fecCodeModuType =
            PHY802_16_QPSK_R_3_4;
        dot16Bs->ulBurstProfile[1].ofdma.exitThreshold =  34;
        dot16Bs->ulBurstProfile[1].ofdma.entryThreshold = 36;

        // 16 QAM modulation, 1/2 coding rate
        dot16Bs->ulBurstProfile[2].ofdma.fecCodeModuType =
            PHY802_16_16QAM_R_1_2;
        dot16Bs->ulBurstProfile[2].ofdma.exitThreshold =  44;
        dot16Bs->ulBurstProfile[2].ofdma.entryThreshold = 46;

        // 16 QAM modulation, 3/4 coding rate
        dot16Bs->ulBurstProfile[3].ofdma.fecCodeModuType =
            PHY802_16_16QAM_R_3_4;
        dot16Bs->ulBurstProfile[3].ofdma.exitThreshold =  58;
        dot16Bs->ulBurstProfile[3].ofdma.entryThreshold = 60;

        // 64 QAM modulation, 1/2 coding rate
        dot16Bs->ulBurstProfile[4].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_1_2;
        dot16Bs->ulBurstProfile[4].ofdma.exitThreshold =  66;
        dot16Bs->ulBurstProfile[4].ofdma.entryThreshold = 68;

        // 64 QAM modulation, 2/3 coding rate
        dot16Bs->ulBurstProfile[5].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_2_3;
        dot16Bs->ulBurstProfile[5].ofdma.exitThreshold =  74;
        dot16Bs->ulBurstProfile[5].ofdma.entryThreshold = 76;

        // 64 QAM modulation, 3/4 coding rate
        dot16Bs->ulBurstProfile[6].ofdma.fecCodeModuType =
            PHY802_16_64QAM_R_3_4;
        dot16Bs->ulBurstProfile[6].ofdma.exitThreshold =  82;
        dot16Bs->ulBurstProfile[6].ofdma.entryThreshold = 84;

        // BPSK modulation
        dot16Bs->ulBurstProfile[7].ofdma.fecCodeModuType =
            PHY802_16_BPSK;
        dot16Bs->ulBurstProfile[7].ofdma.exitThreshold =  22;
        dot16Bs->ulBurstProfile[7].ofdma.entryThreshold = 24;
    }
    else
    {
        ERROR_ReportError("MAC802.16: only OFDMA PHY model is supported!");
    }

}

// /**
// FUNCTION   :: MacDot16eBsInitNbrBsInfo
// LAYER      :: MAC
// PURPOSE    :: Initialize downlink and uplink burst profiles
// PARAMETERS ::
// + node : Node* : Pointer to node
// + interfaceIndex : int : Index of the interface where the MAC is running.
// + nodeInput : const NodeInput* : Pointer to node input
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eBsInitNbrBsInfo(
         Node* node,
         int interfaceIndex,
         const NodeInput* nodeInput,
         MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    int i;

    // init neightbor BS info filed
    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i++)
    {
        // assume the bsIndex is the same as array index
        dot16Bs->nbrBsInfo[i].nbrBsIndex = i;
        dot16Bs->nbrBsInfo[i].bsNodeId = DOT16_INVALID_BS_NODE_ID;
        dot16Bs->nbrBsInfo[i].isActive = FALSE;
    }

    dot16Bs->numNbrBs = 0;

    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-802.16-BS-NEIGHBOR",
        &retVal,
        buf);

    if (retVal == FALSE)
    {
        return;
    }
    else
    {
        // For IO_GetDelimitedToken
        char* next;
        char* token;
        char* p;
        char nbrBsString[MAX_STRING_LENGTH];
        const char* delims = "{,} \n";
        char iotoken[MAX_STRING_LENGTH];


        strcpy(nbrBsString, buf);
        p = nbrBsString;
        p = strchr(p, '{');
        BOOL minNodeIdSpecified = FALSE;
        NodeAddress minNodeId = 0;

        if (p == NULL)
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(
                errorBuf, "Could not find '{' character:\n in %s\n",
                nbrBsString);
            ERROR_Assert(FALSE, errorBuf);
        }
        else
        {
            token = IO_GetDelimitedToken(iotoken, p, delims, &next);

            if (!token)
            {
                char errorBuf[MAX_STRING_LENGTH];
                sprintf(errorBuf, "Can't find bs list, e.g. "
                    "{ 1, 2, ... }:in \n  %s\n", nbrBsString);
                ERROR_Assert(FALSE, errorBuf);
            }
            else
            {
                while (token)
                {
                    NodeAddress bsNodeId;

                    if (strncmp("thru", token, 4) == 0)
                    {
                        if (minNodeIdSpecified)
                        {
                            NodeAddress maxNodeId;

                            token = IO_GetDelimitedToken(iotoken,
                                                 next,
                                                 delims,
                                                 &next);

                            maxNodeId = (NodeAddress)atoi(token);

                            ERROR_Assert(maxNodeId > minNodeId,
                                "in { M Thru N}, N has to be"
                                "greater than M");

                            ERROR_Assert((dot16Bs->numNbrBs + maxNodeId -
                                     minNodeId) <=
                                     DOT16e_DEFAULT_MAX_NBR_BS,
                                     "Too many neighbor BSs, try to "
                                     "increase DOT16e_DEFAULT_MAX_NBR_BS");

                            bsNodeId = minNodeId + 1;
                            while (bsNodeId <= maxNodeId)
                            {
                                dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].
                                    bsNodeId = bsNodeId;
                                dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].
                                    bsDefaultAddress =
                                    MAPPING_GetDefaultInterfaceAddressInfoFromNodeId
                                        (node,
                                         bsNodeId);

                                dot16Bs->numNbrBs ++;
                                bsNodeId ++;
                            }

                            minNodeIdSpecified = FALSE;
                        }
                        else
                        {
                            char errorBuf[MAX_STRING_LENGTH];
                            sprintf(errorBuf, "Can't min node M, e.g. "
                                "when using { M thru n... }:in \n  %s\n",
                                 nbrBsString);
                            ERROR_Assert(FALSE, errorBuf);
                        }
                    }
                    else
                    {
                        bsNodeId = (NodeAddress)atoi(token);

                        dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].
                            bsNodeId = bsNodeId;
                        dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].
                            bsDefaultAddress =
                            MAPPING_GetDefaultInterfaceAddressInfoFromNodeId
                                (node,
                                bsNodeId);

                        dot16Bs->numNbrBs ++;

                        if (dot16Bs->numNbrBs > DOT16e_DEFAULT_MAX_NBR_BS)
                        {
                            ERROR_Assert(FALSE, "Too many neighbor BSs, "
                                "try to increase"
                                "DOT16e_DEFAULT_MAX_NBR_BS");
                        }

                        minNodeId = bsNodeId;
                        minNodeIdSpecified = TRUE;
                    }

                    token = IO_GetDelimitedToken(iotoken,
                                                 next,
                                                 delims,
                                                 &next);
                } // while (token)
            } // if (token)
        } // if (p)
    } // if (retVal)

} // MacDot16eBsInitNbrBsInfo

// /**
// FUNCTION   :: MacDot16BsInitDynamicStats
// LAYER      :: MAC
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIndex : int      : Interface where the MAC is running
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16BsInitDynamicStats(Node* node,
                                int interfaceIndex,
                                MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    if (!node->guiOption)
    {
        return;
    }

    dot16Bs->stats.numPktToPhyGuiId =
        GUI_DefineMetric("802.16 MAC (BS): Number of Data Packets Transmitted",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

    dot16Bs->stats.numPktFromPhyGuiId =
        GUI_DefineMetric("802.16 MAC (BS): Number of Data Packets Received",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

    dot16Bs->stats.numPktDropInHoGuiId =
        GUI_DefineMetric("802.16 MAC (BS): Number of Data Packets Dropped due to Handover",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);
}

// /**
// FUNCTION   :: MacDot16BsPrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + interfaceIndex : int      : Interface index
// RETURN     :: void : NULL
// **/
static
void MacDot16BsPrintStats(Node* node,
                          MacDataDot16* dot16,
                          int interfaceIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    char buf[MAX_STRING_LENGTH];

    // print out # of data packets from upper layer
    sprintf(buf, "Number of data packets from upper layer = %d",
            dot16Bs->stats.numPktsFromUpper);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets from upper layer
    if (dot16Bs->isFragEnabled == TRUE)
    {
        sprintf(buf, "Number of fragments sent in MAC frames= %d",
                dot16Bs->stats.numFragmentsSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf, "Number of fragments received in MAC frames= %d",
                dot16Bs->stats.numFragmentsRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }
    // print out # of data packets drooped when outdated routes and
    // SS is not mangeable any more
    sprintf(buf, "Number of data packets dropped due to unknown SS = %d",
            dot16Bs->stats.numPktsDroppedUnknownSs);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets drooped at MAC  queue
    // SS is not mangeable any more
    sprintf(buf,
            "Number of data packets dropped due to SS moved out "
            "of cell = %d",
            dot16Bs->stats.numPktsDroppedRemovedSs);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets drooped at MAC  queue
    // SS is in handover
    sprintf(buf,
            "Number of data packets dropped due to SS in handover "
            "status = %d",
            dot16Bs->stats.numPktsDroppedSsInHandover);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets txed in MAC frames
    sprintf(buf, "Number of data packets sent in MAC frames = %d",
            dot16Bs->stats.numPktsToLower);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of data packets received in MAC frames
    sprintf(buf, "Number of data packets rcvd in MAC frames = %d",
            dot16Bs->stats.numPktsFromLower);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DL-MAP messages sent
    sprintf(buf, "Number of DL-MAP messages sent = %d",
            dot16Bs->stats.numDlMapSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of UL-MAP messages sent
    sprintf(buf, "Number of UL-MAP messages sent = %d",
            dot16Bs->stats.numUlMapSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DCD messages sent
    sprintf(buf, "Number of DCD messages sent = %d",
            dot16Bs->stats.numDcdSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of UCD messages sent
    sprintf(buf, "Number of UCD messages sent = %d",
            dot16Bs->stats.numUcdSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of RNG-REQ messages received
    sprintf(buf, "Number of RNG-REQ messages rcvd = %d",
            dot16Bs->stats.numRngReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of RNG-RSP messages sent
    sprintf(buf, "Number of RNG-RSP messages sent = %d",
            dot16Bs->stats.numRngRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of SBC-REQ messages received
    sprintf(buf, "Number of SBC-REQ messages rcvd = %d",
            dot16Bs->stats.numSbcReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of SBC-RSP messages sent
    sprintf(buf, "Number of SBC-RSP messages sent = %d",
            dot16Bs->stats.numSbcRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of PKM-REQ messages received
    sprintf(buf, "Number of PKM-REQ messages rcvd = %d",
            dot16Bs->stats.numPkmReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of PKM-RSP messages sent
    sprintf(buf, "Number of PKM-RSP messages sent = %d",
            dot16Bs->stats.numPkmRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REG-REQ messages received
    sprintf(buf, "Number of REG-REQ messages rcvd = %d",
            dot16Bs->stats.numRegReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REG-RSP messages sent
    sprintf(buf, "Number of REG-RSP messages sent = %d",
            dot16Bs->stats.numRegRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);
    // print out # of REP-REQ messages sent
    sprintf(buf, "Number of REP-REQ messages sent = %d",
            dot16Bs->stats.numRepReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of REP-RSP messages received
    sprintf(buf, "Number of REP-RSP messages rcvd = %d",
            dot16Bs->stats.numRepRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-REQ messages received
    sprintf(buf, "Number of DSA-REQ messages rcvd = %d",
            dot16Bs->stats.numDsaReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSX-RVD messages sent
    sprintf(buf, "Number of DSX-RVD messages sent = %d",
            dot16Bs->stats.numDsxRvdSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-RSP messages sent
    sprintf(buf, "Number of DSA-RSP messages sent = %d",
            dot16Bs->stats.numDsaRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-ACK messages received
    sprintf(buf, "Number of DSA-ACK messages rcvd = %d",
            dot16Bs->stats.numDsaAckRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-REQ messages sent
    sprintf(buf, "Number of DSA-REQ messages sent = %d",
            dot16Bs->stats.numDsaReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-REQ rejected
    if (dot16Bs->acuAlgorithm != DOT16_ACU_NONE)
    {
        sprintf(buf, "Number of DSA-REQ rejected = %d",
                dot16Bs->stats.numDsaReqRej);
        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }
    // print out # of DSA-RSP messages received
    sprintf(buf, "Number of DSA-RSP messages rcvd = %d",
            dot16Bs->stats.numDsaRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSA-ACK messages sent
    sprintf(buf, "Number of DSA-ACK messages sent = %d",
            dot16Bs->stats.numDsaAckSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-REQ messages received
    sprintf(buf, "Number of DSC-REQ messages rcvd = %d",
            dot16Bs->stats.numDscReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-RSP messages sent
    sprintf(buf, "Number of DSC-RSP messages sent = %d",
            dot16Bs->stats.numDscRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-ACK messages received
    sprintf(buf, "Number of DSC-ACK messages rcvd = %d",
            dot16Bs->stats.numDscAckRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-REQ messages sent
    sprintf(buf, "Number of DSC-REQ messages sent = %d",
            dot16Bs->stats.numDscReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-RSP messages received
    sprintf(buf, "Number of DSC-RSP messages rcvd = %d",
            dot16Bs->stats.numDscRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSC-ACK messages sent
    sprintf(buf, "Number of DSC-ACK messages sent = %d",
            dot16Bs->stats.numDscAckSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-REQ messages received
    sprintf(buf, "Number of DSD-REQ messages rcvd = %d",
            dot16Bs->stats.numDsdReqRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-RSP messages sent
    sprintf(buf, "Number of DSD-RSP messages sent = %d",
            dot16Bs->stats.numDsdRspSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-REQ messages sent
    sprintf(buf, "Number of DSD-REQ messages sent = %d",
            dot16Bs->stats.numDsdReqSent);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

    // print out # of DSD-RSP messages received
    sprintf(buf, "Number of DSD-RSP messages rcvd = %d",
            dot16Bs->stats.numDsdRspRcvd);
    IO_PrintStat(node,
                 "MAC",
                 "802.16",
                 ANY_DEST,
                 interfaceIndex,
                 buf);

     // print out # of DSC-REQ messages rejected
     sprintf(buf, "Number of DSC-REQ messages rejected = %d",
             dot16Bs->stats.numDscReqRej);
     IO_PrintStat(node,
                  "MAC",
                  "802.16",
                  ANY_DEST,
                  interfaceIndex,
                  buf);

     if (dot16Bs->rngType == DOT16_CDMA)
     {
         sprintf(buf, "Number of CDMA ranging code received = %d",
                 dot16Bs->stats.numCdmaRngCodeRcvd);
         IO_PrintStat(node,
                      "MAC",
                      "802.16",
                      ANY_DEST,
                      interfaceIndex,
                      buf);
     }
     if (dot16Bs->bwReqType == DOT16_BWReq_CDMA)
     {
         sprintf(buf, "Number of CDMA bandwidth ranging code received = %d",
                 dot16Bs->stats.numCdmaBwRngCodeRcvd);
         IO_PrintStat(node,
                      "MAC",
                      "802.16",
                      ANY_DEST,
                      interfaceIndex,
                      buf);
     }
     //ARQ related stats.
    if (dot16Bs->isARQEnabled)
    {
        sprintf(buf, "Number of ARQ blocks received = %d",
                    dot16Bs->stats.numARQBlockRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

        sprintf(buf, "Number of ARQ blocks sent = %d",
                    dot16Bs->stats.numARQBlockSent);


        IO_PrintStat(node,
                     "MAC",
                     "802.16",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        sprintf(buf, "Number of ARQ blocks discarded = %d",
                       dot16Bs->stats.numARQBlockDiscard);

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
        // print out # of HELLO Msg sent
        sprintf(buf, "Number of Inter-BS Hello messages sent = %d",
                dot16Bs->stats.numInterBsHelloSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of HELLO Msg Rcvd
        sprintf(buf, "Number of Inter-BS Hello messages rcvd = %d",
                dot16Bs->stats.numInterBsHelloRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of NBR-ADV sent
        sprintf(buf, "Number of MOB-NBR-ADV messages sent = %d",
                dot16Bs->stats.numNbrAdvSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of SCN-REQ received
        sprintf(buf, "Number of MOB-SCN-REQ messages rcvd = %d",
                dot16Bs->stats.numNbrScanReqRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of SCN-RSP sent
        sprintf(buf, "Number of MOB-SCN-RSP messages sent = %d",
                dot16Bs->stats.numNbrScanRspSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-SCN-REP received
        sprintf(buf, "Number of MOB-SCN-REP messages rcvd = %d",
                dot16Bs->stats.numNbrScanRepRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-MSHO-REQ received
        sprintf(buf, "Number of MOB-MSHO-REQ messages rcvd = %d",
                dot16Bs->stats.numMsHoReqRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-BSHO-RSP sent
        sprintf(buf, "Number of MOB-BSHO-RSP messages sent = %d",
                dot16Bs->stats.numBsHoRspSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of MOB-BSHO-REQ sent
        sprintf(buf, "Number of MOB-BSHO-REQ messages sent = %d",
                dot16Bs->stats.numBsHoReqSent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of HO-IND received
        sprintf(buf, "Number of MOB-HO-IND messages rcvd = %d",
                dot16Bs->stats.numHoIndRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out # of HO Finish Msg sent
        sprintf(buf, "Number of Inter-BS HO Finish messages sent = %d",
                dot16Bs->stats.numInterBsHoFinishNotifySent);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

         // print out # of HO Finish Msg rcvd
        sprintf(buf, "Number of Inter-BS HO Finish messages rcvd = %d",
                dot16Bs->stats.numInterBsHoFinishNotifyRcvd);
        IO_PrintStat(node,
                     "MAC",
                     "802.16e",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
        {
             // print out # of DREG-CMD sent
            sprintf(buf, "Number of DREG-CMD messages sent = %d",
                    dot16Bs->stats.numDregCmdSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

             // print out # of DREG-REQ rcvd
            sprintf(buf, "Number of DREG-REQ messages rcvd = %d",
                    dot16Bs->stats.numDregReqRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            // print out # of MOB-PAG-ADV sent
            sprintf(buf, "Number of MOB-PAG-ADV messages sent = %d",
                    dot16Bs->stats.numMobPagAdvSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

            // print out # of packets dropped by BS for SS in idle mode
            sprintf(buf, "Number of Packets dropped due to SS in idle mode = %d",
                    dot16Bs->stats.numPktsDroppedSsInIdleMode);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
            //paging controller related stats
            if (((MacDot16Bs*)dot16->bsData)->pgCtrlData)
            {
                Dot16BackbonePrintStats(node, dot16, interfaceIndex);
            }
        }
// Sleep mode statistics parameter
        if (dot16Bs->isSleepEnabled)
        {
             // print out # Number of MOB-SLP-REQ messages rcvd
            sprintf(buf, "Number of MOB-SLP-REQ messages rcvd = %d",
                    dot16Bs->stats.numMobSlpReqRcvd);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

             // print out # Number of MOB-SLP-RSP messages sent
            sprintf(buf, "Number of MOB-SLP-RSP messages sent = %d",
                    dot16Bs->stats.numMobSlpRspSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);

             // print out # Number of MOB-TRF-IND messages sent
            sprintf(buf, "Number of MOB-TRF-IND messages sent = %d",
                    dot16Bs->stats.numMobTrfIndSent);
            IO_PrintStat(node,
                         "MAC",
                         "802.16e",
                         ANY_DEST,
                         interfaceIndex,
                         buf);
        }
    }
// end of 802.16e related statistics
}

// /**
// FUNCTION   :: MacDot16BsAllocTransportCid
// LAYER      :: MAC
// PURPOSE    :: Allocat a CID for data service flows
// PARAMETERS ::
// + node      : Node* : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the SS info
// RETURN     :: Dot16CIDType : Allocated CID for data service flow
// **/
static
Dot16CIDType MacDot16BsAllocTransportCid(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo)
{
    int index = 0;
    int i;
    BOOL found = FALSE;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    // find an unused CID
    for (i = (dot16Bs->lastTransCidAssigned + 1);
         i <= DOT16_TRANSPORT_CID_END;
         i ++)
    {
        index = MacDot16BsHashTransportCid(i);
        if (dot16Bs->transportCidUsage[index] == 0)
        {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE)
    {
        for (i = DOT16_TRANSPORT_CID_START;
             i <= dot16Bs->lastTransCidAssigned;
             i ++)
        {
            index = MacDot16BsHashTransportCid(i);
            if (dot16Bs->transportCidUsage[index] == 0)
            {
                found = TRUE;
                break;
            }
        }
    }

    if (found == TRUE)
    {
        dot16Bs->transportCidUsage[index] = 1;
        dot16Bs->lastTransCidAssigned = (Dot16CIDType)i;
        return (Dot16CIDType)i;
    }
    else
    {
        ERROR_ReportError("MAC802.16: Unable to allocate transport cid");
        return DOT16_TRANSPORT_CID_START;
    }
}

// /**
// FUNCTION   :: MacDot16BsFreeTransportCid
// LAYER      :: MAC
// PURPOSE    :: Free a CID allocated to a data service flows
// PARAMETERS ::
// + node      : Node* : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + cid       : Dot16CIDType  : transport cid to free
// RETURN     :: void : NULL
// **/
static
void MacDot16BsFreeTransportCid(
                 Node* node,
                 MacDataDot16* dot16,
                 Dot16CIDType cid)
{
    int index = 0;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    if (cid < DOT16_TRANSPORT_CID_START ||
        cid > DOT16_TRANSPORT_CID_END)
    {
        ERROR_ReportWarning("Try to free a non transport Cid");
        return;
    }

    index = MacDot16BsHashTransportCid(cid);
    dot16Bs->transportCidUsage[index] = 0;
}

// /**
// FUNCTION   :: MacDot16BsAllocSecondaryCid
// LAYER      :: MAC
// PURPOSE    :: Allocat a CID for Secondary connection
// PARAMETERS ::
// + node      : Node* : Pointer to node
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the SS info
// RETURN     :: Dot16CIDType : Allocated CID for Secondary connection
// **/
static
Dot16CIDType MacDot16BsAllocSecondaryCid(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo)
{

    return ssInfo->basicCid;
}

// /**
// FUNCTION   :: MacDot16BsScheduleMgmtMsgToSs
// LAYER      :: MAC
// PURPOSE    :: Add a mgmt message to be txed for a SS
// PARAMETERS ::
// + node   : Node* : Pointer to node
// + dot16  : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to a SS info record
// + cid    : DOT16CIDType : cid of the connection
// + pduMsg : Message* : Message containing the mgmt PDU
// RETURN     :: BOOL : TRUE if successful, otherwise FALSE
//                      Note: if non-successful, msg will be freed.
// **/
//static

BOOL MacDot16BsScheduleMgmtMsgToSs(
         Node* node,
         MacDataDot16* dot16,
         MacDot16BsSsInfo* ssInfo,
         Dot16CIDType cid,
         Message* pduMsg)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int priority = DOT16_SCH_BCAST_MGMT_QUEUE_PRIORITY;
    BOOL queueIsFull = FALSE;

    if (cid == DOT16_BROADCAST_CID || cid == DOT16_INITIAL_RANGING_CID)
    {
        priority = DOT16_SCH_BCAST_MGMT_QUEUE_PRIORITY;
        if (ssInfo != NULL)
        {
            ssInfo->needUcastPollNumFrame = 2;
        }
    }
    else if (MacDot16IsBasicCid(cid))
    {
        priority = DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY;
        ssInfo->needUcastPoll = TRUE;
    }
    else if (MacDot16IsPrimaryCid(cid))
    {
        priority = DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY;
        ssInfo->needUcastPoll = TRUE;
    }
    else if (MacDot16IsSecondaryCid(cid))
    {
        priority = DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY;
    }

    // enqueue the PDU message into the corresponding queue
    dot16Bs->mgmtScheduler->insert(pduMsg,
                                   &queueIsFull,
                                   priority,
                                   NULL,
                                   node->getNodeTime());

    if (queueIsFull)
    {
        MESSAGE_Free(node, pduMsg);
    }

    return !queueIsFull;
}

// /**
// FUNCTION   :: MacDot16BsAddNewSs
// LAYER      :: MAC
// PURPOSE    :: Add a new SS into the registered SS list
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress : unsigned char* : SS MAC Address
// RETURN     :: MacDot16BsSsInfo* : Pointer to the added entry
// **/
static
MacDot16BsSsInfo* MacDot16BsAddNewSs(
                      Node* node,
                      MacDataDot16* dot16,
                      unsigned char* macAddress)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    int index = 0;
    Dot16CIDType cid;
    int numIterations;

    // find an unused CID
    numIterations = DOT16_BASIC_CID_END - DOT16_BASIC_CID_START + 1;
    cid = dot16Bs->lastBasicCidAssigned + 1;
    while (numIterations > 0)
    {
        if (cid > DOT16_BASIC_CID_END)
        {
            cid = cid - DOT16_BASIC_CID_END + DOT16_BASIC_CID_START - 1;
        }
        index = MacDot16BsHashBasicCid(cid);

        if (dot16Bs->ssHash[index] == NULL)
        {
            break;
        }
        else
        {
            ssInfo = dot16Bs->ssHash[index];
            while (ssInfo != NULL)
            {
                if (ssInfo->basicCid == cid)
                {
                    break;
                }

                ssInfo = ssInfo->next;
            }

            if (ssInfo == NULL)
            {
                break;
            }
        }

        cid ++;
        numIterations --;
    }

    if (cid == dot16Bs->lastBasicCidAssigned)
    {
        // No enough CID space to allocate a basic CID
        ERROR_ReportError("MAC 802.16: No free basic CID to allocate. Try "
                          "to increase constant DOT16_CID_BLOCK_M");
    }

    // cid is the basic CID to be allocated to this SS
    ssInfo = (MacDot16BsSsInfo*) MEM_malloc(sizeof(MacDot16BsSsInfo));
    ERROR_Assert(ssInfo != NULL, "MAC 802.16: Out of memory!");
    memset((char*) ssInfo, 0, sizeof(MacDot16BsSsInfo));

    ssInfo->basicCid = cid;
    ssInfo->primaryCid = cid + DOT16_CID_BLOCK_M;
    MacDot16CopyMacAddress(ssInfo->macAddress, macAddress);
    ssInfo->lastRangeGrantTime = node->getNodeTime();

    dot16Bs->lastBasicCidAssigned = cid;

    // insert into the table
    ssInfo->next = dot16Bs->ssHash[index];
    dot16Bs->ssHash[index] = ssInfo;

    return ssInfo;
}

// /**
// FUNCTION   :: MacDot16BsGetSsByCid
// LAYER      :: MAC
// PURPOSE    :: Get a record of the SS using its basic CID or primary CID
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + cid   : Dot16CIDType  : Basic or primary CID of the SS
// RETURN     :: MacDot16BsSsInfo* : Pointer to the added entry
// **/
MacDot16BsSsInfo* MacDot16BsGetSsByCid(
                      Node* node,
                      MacDataDot16* dot16,
                      Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int i;

    if (cid >= DOT16_BASIC_CID_START && cid <= DOT16_PRIMARY_CID_END)
    { // basic or primary CID of the SS
        if (cid <= DOT16_BASIC_CID_END)
        {
            index = MacDot16BsHashBasicCid(cid);
        }
        else
        {
            index = MacDot16BsHashPrimaryCid(cid);
        }

        ssInfo = dot16Bs->ssHash[index];

        while (ssInfo != NULL)
        {
            if (ssInfo->basicCid == cid || ssInfo->primaryCid == cid)
            {
                return ssInfo;
            }

            ssInfo = ssInfo->next;
        }
    }
    else if (cid >= DOT16_TRANSPORT_CID_START &&
             cid <= DOT16_TRANSPORT_CID_END)
    { // CID of service flows
        MacDot16ServiceFlow* serviceFlow;

        for (index = 0; index < DOT16_BS_SS_HASH_SIZE; index ++)
        {
            ssInfo = dot16Bs->ssHash[index];

            while (ssInfo != NULL)
            {
                for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
                {
                    serviceFlow = ssInfo->ulFlowList[i].flowHead;

                    while (serviceFlow != NULL)
                    {
                        if (serviceFlow->cid == cid)
                        {
                            return ssInfo;
                        }
                        serviceFlow = serviceFlow->next;
                    }
                    serviceFlow = ssInfo->dlFlowList[i].flowHead;

                    while (serviceFlow != NULL)
                    {
                        if (serviceFlow->cid == cid)
                        {
                            return ssInfo;
                        }
                        serviceFlow = serviceFlow->next;
                    }
                }

                ssInfo = ssInfo->next;
            }
        }
    }
    else
    {
        // Unsupported CID type
        return NULL;
    }

    return NULL;
}

// /**
// FUNCTION   :: MacDot16BsGetServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by transaction id
//               of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + ssInfo    : MacDot16BsSsInfo*     : Pointer to BsSsInfo
// + transId   : unsinged int          : transaction Id assoctated
//                                       with the service flow
// + sflowPtr  : MacDot16ServiceFlow** : To return the SFlow pointer
// RETURN     :: void : NULL
// **/
void MacDot16BsGetServiceFlowByTransId(Node* node,
                                       MacDataDot16* dot16,
                                       MacDot16BsSsInfo* ssInfo,
                                       unsigned int transId,
                                       MacDot16ServiceFlow** sflowPtr)
{
    MacDot16ServiceFlow* serviceFlow;
    int i;

    // for the ul service flow, the transId is in the range 0x0000 - 0x7FFF,
    // for the dl service flow, the transId is in the range 0x8000 - 0xFFFF

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = ssInfo->ulFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                *sflowPtr = serviceFlow;
                return;
            }
            serviceFlow = serviceFlow->next;
        }
    }
    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = ssInfo->dlFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                *sflowPtr = serviceFlow;
                return;
            }
            serviceFlow = serviceFlow->next;
        }
    }

    // not found
    *sflowPtr = NULL;
    return;
}

// /**
// FUNCTION   :: MacDot16BsDeleteServiceFlowByTransId
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow & its SS pointer by transaction
//               id of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + ssInfo    : MacDot16BsSsInfo*     : Pointer to BsSsInfo
// + transId   : unsinged int          : transaction Id assoctated with
//                                       the service flow
// RETURN     :: BOOL : TRUE = deletion success; FALSE = deletion failed
// **/
static
BOOL MacDot16BsDeleteServiceFlowByTransId(Node* node,
                                          MacDataDot16* dot16,
                                          MacDot16BsSsInfo* ssInfo,
                                          unsigned int transId)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ServiceFlow* serviceFlow = NULL;
    MacDot16ServiceFlow* lastServiceFlow = NULL;
    BOOL found = FALSE;
    BOOL isDlFlow = FALSE;
    int serviceType = 0;
    int i;

    // for the ul service flow, the transId is in the range 0x0000 - 0x7FFF,
    // for the dl service flow, the transId is in the range 0x8000 - 0xFFFF

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        serviceFlow = ssInfo->ulFlowList[i].flowHead;

        while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                serviceType = i;
                found = TRUE;

                break;
            }

            lastServiceFlow = serviceFlow;
            serviceFlow = serviceFlow->next;
        }
    }

    for (i = 0; found == FALSE && i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
         serviceFlow = ssInfo->dlFlowList[i].flowHead;

         while (serviceFlow != NULL)
        {
            if (serviceFlow->dsaInfo.dsxTransId == transId ||
                serviceFlow->dscInfo.dsxTransId == transId ||
                serviceFlow->dsdInfo.dsxTransId == transId)
            {
                serviceType = i;
                found = TRUE;
                isDlFlow = TRUE;

                break;
            }

            lastServiceFlow = serviceFlow;
            serviceFlow = serviceFlow->next;
        }
    }

    if (found == TRUE)
    {
        if (serviceFlow == ssInfo->ulFlowList[serviceType].flowHead)
        {
            ssInfo->ulFlowList[serviceType].flowHead = serviceFlow->next;
            ssInfo->ulFlowList[serviceType].numFlows --;
            ssInfo->ulFlowList[serviceType].maxSustainedRate -=
                serviceFlow->qosInfo.maxSustainedRate;
            ssInfo->ulFlowList[serviceType].minReservedRate -=
                serviceFlow->qosInfo.minReservedRate;
        }
        else if (serviceFlow == ssInfo->dlFlowList[serviceType].flowHead)
        {
            ssInfo->dlFlowList[serviceType].flowHead = serviceFlow->next;
            ssInfo->dlFlowList[serviceType].numFlows --;
            ssInfo->dlFlowList[serviceType].maxSustainedRate -=
                serviceFlow->qosInfo.maxSustainedRate;
            ssInfo->dlFlowList[serviceType].minReservedRate -=
                serviceFlow->qosInfo.minReservedRate;
        }
        else
        {
             lastServiceFlow->next =  serviceFlow->next;
        }

        // free mem in dsxInfo
        // dsa
        MacDot16ResetDsxInfo(node,
                             dot16,
                             serviceFlow,
                             &(serviceFlow->dsaInfo));
        // dsc
        MacDot16ResetDsxInfo(node,
                             dot16,
                             serviceFlow,
                             &(serviceFlow->dscInfo));
        // dsd
        MacDot16ResetDsxInfo(node,
                             dot16,
                             serviceFlow,
                             &(serviceFlow->dsdInfo));

        // free others if present

        if (isDlFlow)
        {
            // delete the corresponding queue from the scheduler
            dot16Bs->outScheduler[serviceType]->removeQueue(
                serviceFlow->queuePriority);
            dot16Bs->outScheduler[serviceType]->normalizeWeight();
        }

        // free temp queue
        MESSAGE_FreeList(node, serviceFlow->tmpMsgQueueHead);

        if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled
            && ssInfo->isSleepEnabled)
        {
            MacDot16ePSClasses* psClass = NULL;
            psClass = &ssInfo->psClassInfo[serviceFlow->psClassType - 1];
            if (psClass->numOfServiceFlowExists > 0)
            {
                psClass->numOfServiceFlowExists--;
            }
        }

        if (serviceFlow->isARQEnabled)
        {
            MacDot16ARQResetWindow(node, dot16, serviceFlow);
            //Deleting the ARQ Block Array.
            MEM_free(serviceFlow->arqControlBlock->arqBlockArray);
            serviceFlow->arqControlBlock->arqBlockArray = NULL;
            //Deleting the ARQ Control Block
            MEM_free(serviceFlow->arqControlBlock);
            serviceFlow->arqControlBlock = NULL;
            //Deleting the ARQ Parameters
            MEM_free(serviceFlow->arqParam);
            serviceFlow->arqParam = NULL;
       }

        // free service flow itself
        MEM_free(serviceFlow);

        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("delete service flow with transId %d "
                   " from the service list \n",
                   transId);
        }
    }

    return found;
}

// /**
// FUNCTION   :: MacDot16BsGetServiceFlowByCid
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by CID of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + cid       : Dot16CIDType          : CID of the service flow
// + ssInfoPtr : MacDot16BsSsInfo**    : To return the SS info pointer
// + sflowPtr  : MacDot16ServiceFlow** : To return the SFlow pointer
// RETURN     :: void : NULL
// **/
void MacDot16BsGetServiceFlowByCid(Node* node,
                                   MacDataDot16* dot16,
                                   Dot16CIDType cid,
                                   MacDot16BsSsInfo** ssInfoPtr,
                                   MacDot16ServiceFlow** sflowPtr)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int i;

    if (cid >= DOT16_TRANSPORT_CID_START && cid <= DOT16_TRANSPORT_CID_END)
    {
        MacDot16ServiceFlow* serviceFlow;

        for (index = 0; index < DOT16_BS_SS_HASH_SIZE; index ++)
        {
            ssInfo = dot16Bs->ssHash[index];

            while (ssInfo != NULL)
            {
                for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
                {
                     serviceFlow = ssInfo->ulFlowList[i].flowHead;

                    while (serviceFlow != NULL)
                    {
                        if (serviceFlow->cid == cid)
                        {
                            *ssInfoPtr = ssInfo;
                            *sflowPtr = serviceFlow;

                            return;
                        }

                        serviceFlow = serviceFlow->next;
                    }
                }
                for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
                {
                     serviceFlow = ssInfo->dlFlowList[i].flowHead;

                    while (serviceFlow != NULL)
                    {
                        if (serviceFlow->cid == cid)
                        {
                            *ssInfoPtr = ssInfo;
                            *sflowPtr = serviceFlow;

                            return;
                        }

                        serviceFlow = serviceFlow->next;
                    }
                }

               ssInfo = ssInfo->next;
            }
        }
    }
    else
    {
        // Unsupported CID type
    }

    // not found
    *ssInfoPtr = NULL;
    *sflowPtr = NULL;

    return;
}

// /**
// FUNCTION   :: MacDot16BsGetServiceFlowByServiceFlowId
// LAYER      :: MAC
// PURPOSE    :: Get a service flow & its SS pointer by sfID of the flow
// PARAMETERS ::
// + node      : Node*                 : Pointer to node
// + dot16     : MacDataDot16*         : Pointer to DOT16 structure
// + sfId      : unsigned int          : sfId of the service flow
// + ssInfoPtr : MacDot16BsSsInfo**    : To return the SS info pointer
// + sflowPtr  : MacDot16ServiceFlow** : To return the SFlow pointer
// RETURN     :: void : NULL
// **/
static
void MacDot16BsGetServiceFlowByServiceFlowId(
         Node* node,
         MacDataDot16* dot16,
         unsigned int sfId,
         MacDot16BsSsInfo** ssInfoPtr,
         MacDot16ServiceFlow** sflowPtr)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int i;

    MacDot16ServiceFlow* serviceFlow;

    for (index = 0; index < DOT16_BS_SS_HASH_SIZE; index ++)
    {
        ssInfo = dot16Bs->ssHash[index];

        while (ssInfo != NULL)
        {
            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                 serviceFlow = ssInfo->ulFlowList[i].flowHead;

                while (serviceFlow != NULL)
                {
                    if (serviceFlow->sfid == sfId)
                    {
                        *ssInfoPtr = ssInfo;
                        *sflowPtr = serviceFlow;

                        return;
                    }

                    serviceFlow = serviceFlow->next;
                }
            }

            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                 serviceFlow = ssInfo->dlFlowList[i].flowHead;

                while (serviceFlow != NULL)
                {
                    if (serviceFlow->sfid == sfId)
                    {
                        *ssInfoPtr = ssInfo;
                        *sflowPtr = serviceFlow;

                        return;
                    }

                    serviceFlow = serviceFlow->next;
                }
            }

            ssInfo = ssInfo->next;
        }
    }

    // not found
    *ssInfoPtr = NULL;
    *sflowPtr = NULL;

    return;
}

// /**
// FUNCTION   :: MacDot16BsTimeoutServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Go through the service flow list and delete old flows
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16BsTimeoutServiceFlow(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ServiceFlow* sFlow;
    clocktype currentTime;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int i;

    currentTime = node->getNodeTime();
    for (index = 0; index < DOT16_BS_SS_HASH_SIZE; index ++)
    {
        ssInfo = dot16Bs->ssHash[index];
        while (ssInfo != NULL)
        {
            // Here We assume deletion of DL service flow should
            // be initated by BS
            // For UL service flow, so does SS
            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                sFlow = ssInfo->dlFlowList[i].flowHead;

                while (sFlow != NULL)
                {
                    if (sFlow->isEmpty == TRUE &&
                        dot16Bs->outScheduler[i]->isEmpty
                        (sFlow->queuePriority) &&
                        (currentTime - sFlow->emptyBeginTime) >
                            dot16Bs->para.flowTimeout &&
                        ((sFlow->status == DOT16_FLOW_Nominal)))
                    {
                        MacDot16BsDeleteServiceFlow(node,
                                                    dot16,
                                                    ssInfo,
                                                    sFlow);
                    } // if timeout

                    sFlow = sFlow->next;
                } // while sFlow
            } // for i

            ssInfo = ssInfo->next;
        } // while ssInfo
    } // for index
}

// /**
// FUNCTION   :: MacDot16BsGetSsByMacAddress
// LAYER      :: MAC
// PURPOSE    :: Get a record of the SS by its MAC address
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress : unsigned char* : MAC address of the SS
// RETURN     :: MacDot16BsSsInfo* : Pointer to the SS
// **/
MacDot16BsSsInfo* MacDot16BsGetSsByMacAddress(
                      Node* node,
                      MacDataDot16* dot16,
                      unsigned char* macAddress)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    int i;

    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        while (ssInfo != NULL)
        {
            if (MacDot16SameMacAddress(macAddress, ssInfo->macAddress))
            {
                break;
            }

            ssInfo = ssInfo->next;
        }

        if (ssInfo != NULL)
        {
            break;
        }
    }

    return ssInfo;
}

// /**
// FUNCTION   :: MacDot16BsRemoveSsByCid
// LAYER      :: MAC
// PURPOSE    :: Remove a record of the SS using its basic CID or
//               primary CID
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + cid   : Dot16CIDType  : Basic or primary CID of the SS
// + freeIt: BOOL          : Whether free the SS structure
// RETURN     :: MacDot16BsSsInfo* : Pointer to the SS structure
// **/
static
MacDot16BsSsInfo* MacDot16BsRemoveSsByCid(
                      Node* node,
                      MacDataDot16* dot16,
                      Dot16CIDType cid,
                      BOOL freeIt)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16BsSsInfo* prevInfo = NULL;
    int index;

    if (DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("remove SS with CID as %d\n", cid);
    }

    if (cid >= DOT16_BASIC_CID_START && cid <= DOT16_PRIMARY_CID_END)
    { // basic or primary CID of the SS
        if (cid <= DOT16_BASIC_CID_END)
        {
            index = MacDot16BsHashBasicCid(cid);
        }
        else
        {
            index = MacDot16BsHashPrimaryCid(cid);
        }

        ssInfo = dot16Bs->ssHash[index];
        prevInfo = NULL;

        while (ssInfo != NULL)
        {
            if (ssInfo->basicCid == cid || ssInfo->primaryCid == cid)
            {
                break;
            }

            prevInfo = ssInfo;
            ssInfo = ssInfo->next;
        }

        if (ssInfo != NULL)
        {
            if (prevInfo == NULL)
            {
                // head of the list
                if (ssInfo->next == NULL)
                {
                   dot16Bs->ssHash[index] = NULL;
                }
                else
                {
                    dot16Bs->ssHash[index] = ssInfo->next;
                }
            }
            else
            {
                prevInfo->next = ssInfo->next;
            }
        }
    }

    if (freeIt)
    {
        if (ssInfo != NULL)
        {
            MacDot16ServiceFlow* serviceFlow;
            MacDot16ServiceFlow* tmpFlow;
            int i;

            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                serviceFlow = ssInfo->dlFlowList[i].flowHead;
                while (serviceFlow != NULL)
                {
                    // invalid claissier at the CS sublayer
                    MacDot16CsInvalidClassifier(node,
                                                dot16,
                                                serviceFlow->csfId);

                    // remove the queue from scheduler
                    dot16Bs->outScheduler[i]->removeQueue(
                        serviceFlow->queuePriority);

                    // free mem in dsxInfo
                    // dsa
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dsaInfo));
                    // dsc
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dscInfo));
                    // dsd
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dsdInfo));

                    MESSAGE_FreeList(node, serviceFlow->tmpMsgQueueHead);

                    tmpFlow = serviceFlow;
                    serviceFlow = serviceFlow->next;
                    MEM_free(tmpFlow);
                }

                serviceFlow = ssInfo->ulFlowList[i].flowHead;
                while (serviceFlow != NULL)
                {
                    // free mem in dsxInfo
                    // dsa
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dsaInfo));
                    // dsc
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dscInfo));
                    // dsd
                    MacDot16ResetDsxInfo(node,
                                         dot16,
                                         serviceFlow,
                                         &(serviceFlow->dsdInfo));

                    MESSAGE_FreeList(node, serviceFlow->tmpMsgQueueHead);

                    tmpFlow = serviceFlow;
                    serviceFlow = serviceFlow->next;
                    MEM_free(tmpFlow);
                }
            }

            // Free timer
            if (ssInfo->timerRngRspProc != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerRngRspProc);
            }
            if (ssInfo->timerT17 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT17);
            }
            if (ssInfo->timerT27 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
            }
            if (ssInfo->timerT9 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT9);
            }

            // Free content in outBuffer
            MESSAGE_FreeList(node, ssInfo->outBuffHead);

            // Free the ssInfo itself
            MEM_free(ssInfo);
            ssInfo = NULL;
        }
    }

    return ssInfo;
}

// /**
// FUNCTION   :: MacDot16eGetBsIndexByBsNodeId
// LAYER      :: MAC
// PURPOSE    :: Get Bs index by neighbor BS's Node Id
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + bsNodeId  : NodeAddress   : Node Id of the neighbor BS
// + bsIndex   : int*          : Index of the neighbor BS
// RETURN     :: BOOL          : TRUE, if find, FALSE, if not found
// **/
static
BOOL MacDot16eGetBsIndexByBsNodeId(Node* node,
                                   MacDataDot16* dot16,
                                   NodeAddress bsNodeId,
                                   int* bsIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int i;

    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i ++)
    {
        if (dot16Bs->nbrBsInfo[i].bsNodeId == bsNodeId)
        {
            *bsIndex = dot16Bs->nbrBsInfo[i].nbrBsIndex;

            return TRUE;
        }
    }

    // not found yet
    if (dot16Bs->numNbrBs < DOT16e_DEFAULT_MAX_NBR_BS)
    {
        // add this Bs into BNR list
        dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].bsNodeId = bsNodeId;
        dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].bsDefaultAddress =
                        MAPPING_GetDefaultInterfaceAddressInfoFromNodeId
                            (node,
                             bsNodeId);

        *bsIndex = dot16Bs->nbrBsInfo[dot16Bs->numNbrBs].nbrBsIndex;
        dot16Bs->numNbrBs ++;

        return TRUE;
    }
    else
    {
        ERROR_ReportWarning("Too many neighbor BSs, try to increase "
                            "DOT16e_DEFAULT_MAX_NBR_BS");

        return FALSE;
    }

}

// /**
// FUNCTION   :: MacDot16eGetBsIndexByBsId
// LAYER      :: MAC
// PURPOSE    :: Get Bs index by neighbor BS's Id (48 bits)
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + bsId      : unsigned char*: BS Id of the neighbor BS
// + bsIndex   : int*          : Index of the neighbor BS
// RETURN     :: BOOL          : TRUE, if find, FALSE, if not found
// **/
static
BOOL MacDot16eGetBsIndexByBsId(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* bsId,
                               int* bsIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int i;

    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i ++)
    {
        if (memcmp(dot16Bs->nbrBsInfo[i].nbrBsId, bsId,
            DOT16_BASE_STATION_ID_LENGTH) == 0
            && dot16Bs->nbrBsInfo[i].bsNodeId != DOT16_INVALID_BS_NODE_ID &&
            dot16Bs->nbrBsInfo[i].isActive)
        {
            *bsIndex = dot16Bs->nbrBsInfo[i].nbrBsIndex;

            return TRUE;
        }
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16BsBuildDcdPdu
// LAYER      :: MAC
// PURPOSE    :: Build a DCD message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: int : Number of bytes of the DCD PDU
// **/
static
int MacDot16BsBuildDcdPdu(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    MacDot16MacHeader* macHeader;
    MacDot16DcdMsg* dcd;
    MacDot16DlBurstProfile* burstProfile;

    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    int index;
    int startIndex;
    int i;
    int triggerIndex;

    if (dot16Bs->dcdPdu != NULL)
    {
        MESSAGE_Free(node, dot16Bs->dcdPdu);
        dot16Bs->dcdPdu = NULL;
    }

    index = 0;

    // add the generic MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));

    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID); // broadcast

    // add the non-PHY specific part of the DCD message
    dcd = (MacDot16DcdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DcdMsg);

    dcd->type = DOT16_DCD;
    dcd->dlChannelId = (UInt8)dot16Bs->dlChannel;
    dcd->changeCount = dot16Bs->dcdCount;

    // Next to add encoded info. of the overall channel

    // Common channel encodings

    // BS_ERIP
    macFrame[index ++] = TLV_DCD_BsEirp; // type
    macFrame[index ++] = 2;  // length
    MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           dot16Bs->para.bs_EIRP);
    index += 2;

    // RSS Init ranging max
    macFrame[index ++] = TLV_DCD_RrsInitRngMax;
    macFrame[index ++] = 2;
    MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           dot16Bs->para.rssIRmax);
    index += 2;

    // frequency

    // macVersion

    // PHY specific channel coding
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        // DL channel number
        macFrame[index ++] = TLV_DCD_ChNumber;
        macFrame[index ++] = 1;
        macFrame[index ++] = (UInt8)dot16Bs->dlChannel;

        // TTG
        macFrame[index ++] = TLV_DCD_TTG;
        macFrame[index ++] = 1;
        macFrame[index ++] = PhyDot16ConvertTTGRTGinPS(node,
                                    dot16->myMacData->phyNumber,
                                    dot16Bs->para.ttg);
        // RTG
        macFrame[index ++] = TLV_DCD_RTG;
        macFrame[index ++] = 1;
        macFrame[index ++] = PhyDot16ConvertTTGRTGinPS(node,
                                    dot16->myMacData->phyNumber,
                                    dot16Bs->para.rtg);
        // BS ID
        macFrame[index ++] = TLV_DCD_BsId;
        macFrame[index ++] = 6;
        MacDot16CopyStationId(&(macFrame[index]), dot16->macAddress);
        index += 6;
    }

    if (dot16->dot16eEnabled)
    {
        if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
        {
            //Paging interval length
            macFrame[index ++] = TLV_DCD_PagingIntervalLength;
            macFrame[index ++] = 1;
            macFrame[index ++] = dot16Bs->para.pagingInterval;

            macFrame[index ++] = TLV_DCD_PagingGroupId;
            macFrame[index ++] = 2;
            MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
                                   dot16Bs->pagingGroup[0]);
            index += 2;
        }
        // trigger
        macFrame[index ++] = TLV_DCD_Trigger;
        macFrame[index ++] = 0; // adjust later
        triggerIndex = index;

        // trigger type. function and action
        macFrame[index ++] = TLV_DCD_TriggerTypeFuncAction;
        macFrame[index ++] = 1;
        macFrame[index ++] = (dot16Bs->trigger.triggerType << 6 ) |
                             (dot16Bs->trigger.triggerFunc << 3 ) |
                             (dot16Bs->trigger.triggerAction);

        // trigger value
        macFrame[index ++] = TLV_DCD_TriggerValue;
        macFrame[index ++] = 1;
        macFrame[index ++] = dot16Bs->trigger.triggerValue;

        // trigger average uration
        macFrame[index ++] = TLV_DCD_TriggerAvgDuration;
        macFrame[index ++] = 1;
        macFrame[index ++] = dot16Bs->trigger.triggerAvgDuration;

        // adjust the trigger index
        macFrame[triggerIndex -1] = (UInt8)(index - triggerIndex);
    }

    // Next to add Downlink_Burst_Profiles
    for (i = 0; i < dot16Bs->numOfDLBurstProfileInUsed; i ++)
    {
        macFrame[index ++] = TLV_DCD_DlBurstProfile; // type
        macFrame[index ++] = 10;  // will be reset later
        startIndex = index;

        macFrame[index ++] = (UInt8)i; // i is the UIUC

        // PHY specific profile encoding
        if (dot16->phyType == DOT16_PHY_OFDMA)
        {
            burstProfile = (MacDot16DlBurstProfile*)
                           &(dot16Bs->dlBurstProfile[i]);

            // FEC code type and modulation type
            macFrame[index ++] = TLV_DCD_PROFILE_OfdmaFecCodeModuType;
            macFrame[index ++] = 1;
            macFrame[index ++] = burstProfile->ofdma.fecCodeModuType;

            // CINR exit threshold, unit is 0.25 dB
            macFrame[index ++] = TLV_DCD_PROFILE_OfdmaExitThreshold;
            macFrame[index ++] = 1;
            macFrame[index ++] = burstProfile->ofdma.exitThreshold;

            // CINR entry threshold, unit is 0.25 dB
            macFrame[index ++] = TLV_DCD_PROFILE_OfdmaEntryThreshold;
            macFrame[index ++] = 1;
            macFrame[index ++] = burstProfile->ofdma.entryThreshold;
        }

        // reset the busrt profile length
        macFrame[startIndex - 1] = (UInt8)(index - startIndex);
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    //copy macFrame to dcdPdu and set pduLength
    dot16Bs->dcdPdu = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, dot16Bs->dcdPdu, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(dot16Bs->dcdPdu), macFrame, index);

    return index;
}

// /**
// FUNCTION   :: MacDot16BsBuildUcdPdu
// LAYER      :: MAC
// PURPOSE    :: Build a UCD message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: int : Length of the UCD PDU
// **/
static
int MacDot16BsBuildUcdPdu(Node* node,
                          MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    MacDot16MacHeader* macHeader;
    MacDot16UcdMsg* ucd;
    MacDot16UlBurstProfile* burstProfile;

    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    int index;
    int startIndex;
    int i;

    if (dot16Bs->ucdPdu != NULL)
    {
        MESSAGE_Free(node, dot16Bs->ucdPdu);
        dot16Bs->ucdPdu = NULL;
    }

    index = 0;

    // add the generic MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID); // broadcast

    // add the non-PHY specific part of the UCD message
    ucd = (MacDot16UcdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16UcdMsg);

    ucd->type = DOT16_UCD;
    ucd->changeCount = dot16Bs->ucdCount;
    ucd->rangeBOStart = dot16Bs->para.rangeBOStart;
    ucd->rangeBOEnd = dot16Bs->para.rangeBOEnd;
    ucd->requestBOStart = dot16Bs->para.requestBOStart;
    ucd->requestBOEnd = dot16Bs->para.requestBOEnd;

    // Next to add encoded info. of the overall channel

    // Common channel encodings

    // Contention-based reservation timeout
    macFrame[index ++] = TLV_UCD_ContentionRsvTimout;
    macFrame[index ++] = 1;
    macFrame[index ++] = dot16Bs->para.contRsvTimeout;

    // Bandwidth request opportunity size
    macFrame[index ++] = TLV_UCD_BwReqOppSize;
    macFrame[index ++] = 2;
    MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           dot16Bs->para.requestOppSize);
    index += 2;

    // Ranging request opportunity size
    macFrame[index ++] = TLV_UCD_RngReqOppSize;
    macFrame[index ++] = 2;
    MacDot16ShortToTwoByte(macFrame[index],
                           macFrame[index + 1],
                           dot16Bs->para.rangeOppSize);
    index += 2;

    // PHY specific channel encoding
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        // Periodic ranging backoff start
        macFrame[index ++] = TLV_UCD_OfdmaPeriodicRngBackoffStart;
        macFrame[index ++] = 1;
        macFrame[index ++] = dot16Bs->para.rangeBOStart;

        // Periodic ranging backoff end
        macFrame[index ++] = TLV_UCD_OfdmaPeriodicRngbackoffEnd;
        macFrame[index ++] = 1;
        macFrame[index ++] = dot16Bs->para.rangeBOEnd;

        // SSTG
        // canned for the purpose of implementation
        macFrame[index ++] = TLV_UCD_SSTG;
        macFrame[index ++] = 1;
        macFrame[index ++] = (UInt8)dot16Bs->para.sstgInPs;
    }

    // Next to add Uplink_Burst_Profiles
    for (i = 0; i < dot16Bs->numOfULBurstProfileInUsed; i ++)
    {
        macFrame[index ++] = TLV_UCD_UlBurstProfile;
        macFrame[index ++] = 13;

        startIndex = index;
        macFrame[index ++] = (UInt8)(i + 1); // UIUC for the UL burst profile

        // burst  profile
        // PHY specific profile encoding
        if (dot16->phyType == DOT16_PHY_OFDMA)
        {
            burstProfile = (MacDot16UlBurstProfile*)
                           &(dot16Bs->ulBurstProfile[i]);

            // FEC code type and modulation type
            macFrame[index ++] = TLV_UCD_PROFILE_OfdmaFecCodeModuType;
            macFrame[index ++] = 1;
            macFrame[index ++] = burstProfile->ofdma.fecCodeModuType;

            // Ranging data ratio
            macFrame[index ++] = TLV_UCD_PROFILE_OfdmaRngDataRatio;
            macFrame[index ++] = 1;
            macFrame[index ++] = burstProfile->ofdma.exitThreshold;

            // normalizaed C/N override
            macFrame[index ++] = TLV_UCD_PROFILE_OfdmaCnOverride;
            macFrame[index ++] = 5;
            macFrame[index] = burstProfile->ofdma.entryThreshold;

            index += 5;
        }

        // reset the busrt profile length
        macFrame[startIndex - 1] = (UInt8)(index - startIndex);
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // Copy macFrame to ucdPdu
    dot16Bs->ucdPdu = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, dot16Bs->ucdPdu, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(dot16Bs->ucdPdu), macFrame, index);

    return index;
}

// /**
// FUNCTION   :: MacDot16BsBuildDlMapPdu
// LAYER      :: MAC
// PURPOSE    :: Build a DL-MAP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + dlMapLength : int*        : To return the length of the DL-MAP
// RETURN     :: Message* : Point to the created message
// **/
static
Message* MacDot16BsBuildDlMapPdu(Node* node,
                                 MacDataDot16* dot16,
                                 int* dlMapLength)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DlMapMsg* dlMap;

    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));

    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID); // broadcast

    // add the non-PHY specific part of the DL-MAP
    dlMap = (MacDot16DlMapMsg*) &(macFrame[index]);

    dlMap->type = DOT16_DL_MAP;
    if (dot16Bs->dcdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT ||
        dot16Bs->dcdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL ||
        dot16Bs->dcdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END)
    {
        // use the current dcd count
        dlMap->dcdCount = dot16Bs->dcdCount;

        if (dot16Bs->dcdUpdateInfo.chUpdateState ==
            DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END)
        {
            // move the state back to normal operaiton
            dot16Bs->dcdUpdateInfo.chUpdateState =
                DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL;
            dot16Bs->dcdUpdateInfo.numChDescSentInTransition = 0;
        }
    }
    else
    {
        // when state is START or COUNT DOWN
        dlMap->dcdCount = dot16Bs->dcdCount - 1;
    }

    // use MAC address as the base station ID
    MacDot16CopyMacAddress(dlMap->baseStationId, dot16->macAddress);

    index += sizeof(MacDot16DlMapMsg);

    // now, add PHY Specific fields of DL-MAP

    // add PHY synchronization field. Note: the position of PHY
    // synchronization filed is slightly adjusted which won't affect
    // simulation results
    index += MacDot16PhyAddPhySyncField(node,
                                        dot16,
                                        macFrame,
                                        index,
                                        DOT16_MAX_MGMT_MSG_SIZE);

    // reserve space for DL-MAP IEs
    index += dot16Bs->numDlMapIEScheduled * sizeof(MacDot16PhyOfdmaDlMapIE);

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // increase statistics
    dot16Bs->stats.numDlMapSent ++;

    // create a message for this PDU
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    *dlMapLength = index;

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildUlMapPdu
// LAYER      :: MAC
// PURPOSE    :: Build a UL-MAP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: Message* : Pointe to the created message
// **/
static
Message* MacDot16BsBuildUlMapPdu(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;

    MacDot16MacHeader* macHeader;
    MacDot16UlMapMsg* ulMap;

    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    int index = 0;
    int i;
    int allocStartTime;

    if (DEBUG_BWREQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("***allocate BW to SS and Build UL MAP***\n");
    }
    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID); // broadcast

    // add the non-PHY specific part of the DL-MAP
    ulMap = (MacDot16UlMapMsg*) &(macFrame[index]);
    index += sizeof(MacDot16UlMapMsg);

    ulMap->type = DOT16_UL_MAP;
    ulMap->ulChannelId = (UInt8)dot16Bs->ulChannel;
    if (dot16Bs->ucdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT ||
        dot16Bs->ucdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL ||
        dot16Bs->ucdUpdateInfo.chUpdateState ==
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END)
    {
        ulMap->ucdCount = dot16Bs->ucdCount;

        if (dot16Bs->ucdUpdateInfo.chUpdateState ==
            DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END)
        {
            // move the state back to normal NULL
            dot16Bs->ucdUpdateInfo.chUpdateState =
                DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL;
            dot16Bs->ucdUpdateInfo.numChDescSentInTransition = 0;
        }

    }
    else
    {
        // when state is START or COUNT DOWN
        ulMap->ucdCount = dot16Bs->ucdCount - 1;
    }

    if (dot16->duplexMode == DOT16_TDD)
    {
        // in TDD, UL part is after DL part
        allocStartTime = (int) (dot16Bs->para.tddDlDuration +
                                       dot16Bs->para.ttg);
    }
    else
    {
        // for FDD, UL sub-frame starts from begining of the frame
        allocStartTime = 0;
    }

    // write the value to the macFrame
    MacDot16IntToFourByte((unsigned char*)ulMap->allocStartTime,
                          allocStartTime);

    // now, add PHY Specific fields of UL-MAP

    if (dot16Bs->rngType == DOT16_CDMA
        || dot16Bs->bwReqType == DOT16_BWReq_CDMA)
    {
        index += MacDot16PhyAddUlMapIE(
                     node,
                     dot16,
                     macFrame,
                     index,
                     DOT16_MAX_MGMT_MSG_SIZE,
                     DOT16_BROADCAST_CID,
                     DOT16_UIUC_CDMA_RANGE,
                     dot16Bs->rangePsAllocated,
                     DOT16_CDMA_INITIAL_RANGING_OVER_2SYMBOL);
        if (dot16Bs->rngType == DOT16_CDMA)
        {
            dot16Bs->rangePsAllocated = 0;
        }
        if (dot16Bs->bwReqType == DOT16_BWReq_CDMA)
        {
            dot16Bs->requestPsAllocated = 0;
        }
    }

    // 1. Add IE for the contention based ranging burst
    if (dot16Bs->rangePsAllocated > 0)
    {
        if (dot16Bs->rngType != DOT16_CDMA)
        {
            index += MacDot16PhyAddUlMapIE(
                         node,
                         dot16,
                         macFrame,
                         index,
                         DOT16_MAX_MGMT_MSG_SIZE,
                         DOT16_BROADCAST_CID,
                         DOT16_UIUC_RANGE,
                         dot16Bs->rangePsAllocated);
        }
        dot16Bs->rangePsAllocated = 0;
    }

    if ((dot16Bs->rngType == DOT16_CDMA
        || dot16Bs->bwReqType == DOT16_BWReq_CDMA)
        && dot16Bs->cdmaAllocationIEPsAllocated > 0)
    {
        index += MacDot16PhyAddUlMapIE(
                     node,
                     dot16,
                     macFrame,
                     index,
                     DOT16_MAX_MGMT_MSG_SIZE,
                     DOT16_BROADCAST_CID,
                     DOT16_UIUC_CDMA_ALLOCATION_IE,
                     dot16Bs->cdmaAllocationIEPsAllocated);
        dot16Bs->cdmaAllocationIEPsAllocated = 0;
    }

    // 2. Add IE for the contention based bandwidth request burst
    if (dot16Bs->requestPsAllocated > 0)
    {
        if (dot16Bs->bwReqType != DOT16_BWReq_CDMA)
        {
            index += MacDot16PhyAddUlMapIE(
                         node,
                         dot16,
                         macFrame,
                         index,
                         DOT16_MAX_MGMT_MSG_SIZE,
                         DOT16_BROADCAST_CID,
                         DOT16_UIUC_REQUEST,
                         dot16Bs->requestPsAllocated);
        }
        dot16Bs->requestPsAllocated = 0;
    }

    // 3. Add IE for any other multicast pulling request burst
    // Not implemented yet.

    // 4. Go through the SS list to add UL burst for each scheduled SS
    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        while (ssInfo != NULL)
        {
            BOOL setT27 = FALSE;
            clocktype t27Interval = dot16Bs->para.t27ActiveInterval;
            ssInfo->ucastTxOppGranted = FALSE;
            ssInfo->ucastTxOppUsed = FALSE;

            if ((ssInfo->needInitRangeGrant ||
                ssInfo->needPeriodicRangeGrant) &&
                ssInfo->rangePsAllocated > 0)
            {
                if (dot16Bs->rngType == DOT16_CDMA)
                {
                    index += MacDot16PhyAddUlMapIE(
                                 node,
                                 dot16,
                                 macFrame,
                                 index,
                                 DOT16_MAX_MGMT_MSG_SIZE,
                                 ssInfo->basicCid,
                                 DOT16_UIUC_CDMA_RANGE,
                                 dot16Bs->rangePsAllocated,
                                 DOT16_CDMA_INITIAL_RANGING_OVER_2SYMBOL);
                }
                else
                {
                    index += MacDot16PhyAddUlMapIE(
                                 node,
                                 dot16,
                                 macFrame,
                                 index,
                                 DOT16_MAX_MGMT_MSG_SIZE,
                                 ssInfo->basicCid,
                                 DOT16_UIUC_RANGE,
                                 ssInfo->rangePsAllocated);
                }
                if (ssInfo->needInitRangeGrant)
                {
                    ssInfo->needInitRangeGrant = FALSE;
                    ssInfo->initRangePolled = TRUE;
                    if (DEBUG_BWREQ)
                    {
                        printf("    Allo Init Invited range Opp to %d\n",
                               ssInfo->basicCid);
                    }
                }

                if (ssInfo->needPeriodicRangeGrant)
                {
                    ssInfo->needPeriodicRangeGrant = FALSE;
                    ssInfo->ucastTxOppGranted = TRUE;
                    setT27 = TRUE;
                    t27Interval = dot16Bs->para.t27IdleInterval;
                    if (DEBUG_BWREQ)
                    {
                        printf("    Allo Periodic Invite range Opp to %d\n",
                               ssInfo->basicCid);
                    }
                }
            }
            ssInfo->rangePsAllocated = 0;

            // 4.2. Check if need a request IE for unicast polling
            if (ssInfo->requestPsAllocated > 0
                && dot16Bs->bwReqType != DOT16_BWReq_CDMA)
            {
                // use UIUC_REQUEST to indicate the burst is for BW req
                index += MacDot16PhyAddUlMapIE(
                             node,
                             dot16,
                             macFrame,
                             index,
                             DOT16_MAX_MGMT_MSG_SIZE,
                             ssInfo->basicCid,
                             DOT16_UIUC_REQUEST,
                             ssInfo->requestPsAllocated);

                ssInfo->needUcastPoll = FALSE;
                if (DEBUG_BWREQ)
                {
                    printf("    unicast Poll Req Opp to ss w/basicCid %d\n",
                            ssInfo->basicCid);
                }
            }
            ssInfo->requestPsAllocated = 0;

            // 4.3. BW grant IE for the data service
            while (ssInfo->ulPsAllocated > 0)
            {
                int duration;

                // the duration field of UL-MAP_IE is only 10 bits
                if (ssInfo->ulPsAllocated >= 1024)
                {
                    duration = 1023;
                }
                else
                {
                    duration = ssInfo->ulPsAllocated;
                }
                // Allocate PS should be more then 2 PS for any SS
                if (duration > 1)
                {
                    index += MacDot16PhyAddUlMapIE(
                                 node,
                                 dot16,
                                 macFrame,
                                 index,
                                 DOT16_MAX_MGMT_MSG_SIZE,
                                 ssInfo->basicCid,
                                 ssInfo->uiuc,
                                 duration);
                    ssInfo->ulPsAllocated = ssInfo->ulPsAllocated -
                        (UInt16)duration;
                }
                else
                {
                    break;
                }
                ssInfo->ucastTxOppGranted = TRUE;

                if (DEBUG_BWREQ)
                {
                    printf("   allocate data grant with ps %d\n", duration);
                    printf("   schedule UL burst "
                           "for basicCID %d with uiuc %d #PS %d\n",
                            ssInfo->basicCid,
                            ssInfo->uiuc,
                            duration);
                }

                setT27 = TRUE;
                t27Interval = dot16Bs->para.t27ActiveInterval;
            }

            if (setT27 && (dot16Bs->rngType != DOT16_CDMA))
            {
                if (ssInfo->timerT27 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
                    ssInfo->timerT27 = NULL;
                }
                ssInfo->timerT27 =
                        MacDot16SetTimer(node,
                                         dot16,
                                         DOT16_TIMER_T27,
                                         t27Interval,
                                         NULL,
                                         ssInfo->basicCid);
                if (DEBUG_PERIODIC_RANGE)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("reset T27 for cid %d due to new Tx Opp grant\n",
                            ssInfo->basicCid);
                }
            }
            ssInfo = ssInfo->next;
        }
    }

    // 5. Add the End of MAP IE
    index += MacDot16PhyAddUlMapIE(
                 node,
                 dot16,
                 macFrame,
                 index,
                 DOT16_MAX_MGMT_MSG_SIZE,
                 DOT16_BROADCAST_CID,
                 DOT16_UIUC_END_OF_MAP,
                 (int) 0);

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // increase statistics
    dot16Bs->stats.numUlMapSent ++;

    // create a message to hold the PDU
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildRngRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a RNG-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + cid       : Dot16CIDType  : CID in header, indicate ranging type
// + rngStatus : MacDot16RangeStatus : Ranging status
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildRngRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             Dot16CIDType cid,
             MacDot16RangeStatus rngStatus,
             char powerDeviation,
             MacDot16RngCdmaInfo cdmaInfo,
             MacDot16RngReqPurpose rngReqPurpose =
                        DOT16_RNG_REQ_InitJoinOrPeriodicRng,
             BOOL flag = FALSE)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16RngRspMsg* rngRsp;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];

    int index = 0;
    Message* pduMsg;

    if (DEBUG_NETWORK_ENTRY || DEBUG_INIT_RANGE || DEBUG_PERIODIC_RANGE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a RNG-RSP to SS with cid = %d\n", cid);
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, cid);

    // Set the common message body
    rngRsp = (MacDot16RngRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RngRspMsg);

    rngRsp->type = DOT16_RNG_RSP;
    rngRsp->ulChannelId = (UInt8)dot16Bs->ulChannel;

    // Add related TLVs

    // Ranging Status TLV shall be added
    macFrame[index ++] = TLV_RNG_RSP_RngStatus; // type
    macFrame[index ++] = 1;  // length
    macFrame[index ++] = (UInt8)rngStatus;

    if (cid == DOT16_INITIAL_RANGING_CID)
    {
        if (ssInfo != NULL)
        {
            // add basic CID
            macFrame[index ++] = TLV_RNG_RSP_BasicCid;
            macFrame[index ++] = 2;
            macFrame[index ++] = (unsigned char) (ssInfo->basicCid / 256);
            macFrame[index ++] = (unsigned char) (ssInfo->basicCid % 256);

            // add primary CID
            macFrame[index ++] = TLV_RNG_RSP_PrimaryCid;
            macFrame[index ++] = 2;
            macFrame[index ++] = (unsigned char) (ssInfo->primaryCid / 256);
            macFrame[index ++] = (unsigned char) (ssInfo->primaryCid % 256);

            if ((dot16Bs->rngType == DOT16_CDMA && flag == FALSE)
                || (dot16Bs->rngType != DOT16_CDMA))
            {// No need to send the SSInfo for CDMA based ranging
                // add SS's MAC address
                macFrame[index ++] = TLV_RNG_RSP_MacAddr;
                macFrame[index ++] = DOT16_MAC_ADDRESS_LENGTH;
                MacDot16CopyMacAddress(&(macFrame[index]), ssInfo->macAddress);
                index += DOT16_MAC_ADDRESS_LENGTH;
            }
        }
    }
    else
    {
        if (ssInfo->dlBurstRequestedBySs == TRUE)
        {
            macFrame[index ++] = TLV_RNG_RSP_DlOpBurst;
            macFrame[index ++] = 2;

            macFrame[index ++] = ssInfo->diuc;
            macFrame[index ++] = dot16Bs->dcdCount;
            ssInfo->dlBurstRequestedBySs = FALSE;
        }

    }

    // add power. timing, freq correction TLVs
    if (rngStatus == RANGE_CONTINUE || rngStatus == RANGE_SUCC)
    {
        if (ssInfo == NULL)
        {
            if (powerDeviation != 0)
            {
                macFrame[index ++] = TLV_RNG_RSP_PowerAdjust;
                macFrame[index ++] = 1;
                macFrame[index ++] = powerDeviation;
            }
        }
        else if (ssInfo->txPowerAdjust != 0)
        {
            macFrame[index ++] = TLV_RNG_RSP_PowerAdjust;
            macFrame[index ++] = 1;
            macFrame[index ++] = ssInfo->txPowerAdjust;
            ssInfo->txPowerAdjust = 0;
        }
    }

    if (dot16Bs->rngType == DOT16_CDMA && flag == TRUE)
    {
        macFrame[index ++] = TLV_RNG_RSP_OfdmaRngCodeAttr; // type
        macFrame[index ++] = sizeof(MacDot16RngCdmaInfo); // length
        memcpy(&(macFrame[index]), &cdmaInfo, sizeof(MacDot16RngCdmaInfo));
        index += sizeof(MacDot16RngCdmaInfo);
    }

    if (rngReqPurpose == DOT16_RNG_REQ_LocUpd)
    {
        macFrame[index ++] = TLV_RNG_RSP_LocUpdRsp;
        macFrame[index ++] = 1;
        macFrame[index ++] = 0x01;  //success

        macFrame[index ++] = TLV_RNG_RSP_PagingInfo;
        macFrame[index ++] = 5;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
                               dot16Bs->para.pagingCycle);
        macFrame[index + 2] = dot16Bs->para.pagingOffset;
        MacDot16ShortToTwoByte(macFrame[index + 3], macFrame[index + 4],
                               dot16Bs->pagingGroup[0]);
        index += 5;
    }
    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // increase statistics
    dot16Bs->stats.numRngRspSent ++;

    // put the pud into a message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildSbcRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a SBC-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildSbcRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo)
{
    MacDot16MacHeader* macHeader;
    MacDot16SbcRspMsg* sbcRsp;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    Message* pduMsg;

    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a SBC-RSP to SS\n");
    }

    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    sbcRsp = (MacDot16SbcRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16SbcRspMsg);

    sbcRsp->type = DOT16_SBC_RSP;

    //add TLV
    macFrame[index ++] = TLV_SBC_REQ_RSP_BwAllocSupport;  // type
    macFrame[index ++] = 1;  // length
    macFrame[index ++] = ssInfo->ssBasicCapability.bwAllocSupport;

    macFrame[index ++] = TLV_SBC_REQ_RSP_TranstionGaps;  // type
    macFrame[index ++] = 2;  // length
    macFrame[index ++] = ssInfo->ssBasicCapability.transtionGaps[0];
    macFrame[index ++] = ssInfo->ssBasicCapability.transtionGaps[1];

    macFrame[index ++] = TLV_SBC_REQ_MaxTransmitPower;  // type
    macFrame[index ++] = 4;  // length
    macFrame[index ++] = ssInfo->ssBasicCapability.maxTxPower[0];
    macFrame[index ++] = ssInfo->ssBasicCapability.maxTxPower[1];
    macFrame[index ++] = ssInfo->ssBasicCapability.maxTxPower[2];
    macFrame[index ++] = ssInfo->ssBasicCapability.maxTxPower[3];

    macFrame[index ++] = TLV_COMMON_CurrTransPow;  // type
    macFrame[index ++] = 1;  // length
    macFrame[index ++] = ssInfo->ssBasicCapability.currentTxPower;

    if (dot16->dot16eEnabled &&
        ssInfo->ssBasicCapability.supportedPSMOdeClasses)
    {
        macFrame[index++] = TLV_SBC_REQ_RSP_PowerSaveClassTypesCapability;
        macFrame[index++] = 1;  // length
        macFrame[index++] = ssInfo->ssBasicCapability.supportedPSMOdeClasses;
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildPkmRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a PKM-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + pkmCode   : MacDot16PKMCode : PKM code for this message
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildPkmRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             MacDot16PKMCode pkmCode)
{
    MacDot16MacHeader* macHeader;
    MacDot16PkmRspMsg* pkmRsp;
    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    if (DEBUG_NETWORK_ENTRY )
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a PKM-RSP to SS\n");
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->primaryCid);

    // Set the common message body
    pkmRsp = (MacDot16PkmRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16PkmRspMsg);

    pkmRsp->type = DOT16_PKM_RSP;
    pkmRsp->code = (UInt8)pkmCode;
    pkmRsp->pkmId = ssInfo->ssAuthKeyInfo.pkmId;

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // pack into a message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildRegRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a REG-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// + reqResponse : MacDot16RegResponse : Response to the REG-RSP message
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildRegRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             MacDot16RegResponse reqResponse)
{
    MacDot16MacHeader* macHeader;
    MacDot16RegRspMsg* regRsp;

    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;
    int i;

    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a REG-RSP to SS\n");
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->primaryCid);

    // Set the common message body
    regRsp = (MacDot16RegRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RegRspMsg);

    regRsp->type = DOT16_REG_RSP;
    regRsp->response = (UInt8)reqResponse;

    macFrame[index ++] = TLV_REG_REQ_RSP_MgmtSupport;  // type
    macFrame[index ++] = 1;  // length
    macFrame[index ++] = (UInt8)ssInfo->managed;

    if (ssInfo->managed == TRUE)
    {
        macFrame[index ++] = TLV_REG_REQ_RSP_SecondaryCid;  // type
        macFrame[index ++] = 2;  // length
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
                               ssInfo->secondaryCid);
        index += 2;
    }

    // Be sure to put HMAC tuple as the last TLV
    macFrame[index ++] = TLV_COMMON_HMacTuple;  // type
    macFrame[index ++] = 21; // length
    macFrame[index ++] = ssInfo->ssAuthKeyInfo.hmacKeySeq;
    for (i = 0; i < 21; i ++)
    {
        // message digest

        macFrame[index ++] = 0; // be sure to give the right value
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // make the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildRepReqPdu
// LAYER      :: MAC
// PURPOSE    :: Build a REG-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildRepReqPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo)
{
    MacDot16MacHeader* macHeader;
    MacDot16RepReqMsg* repReq;

    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a REP-REQ to SS\n");
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    repReq = (MacDot16RepReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RepReqMsg);

    repReq->type = DOT16_REP_REQ;

    macFrame[index ++] = TLV_REP_REQ_ReportType;  // type
    macFrame[index ++] = 1;  // length
    macFrame[index ++] = 0x06; // only CINR & RSSI reprot are supported

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // make the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16BsBuildDsxRvdPdu
// LAYER      :: MAC
// PURPOSE    :: Build a DSX-RVD PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// + transId   : unsigned int  : Transaction ID
// + confirmCode: unsigned char: Confirmation code
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16BsBuildDsxRvdPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             unsigned int transId,
             unsigned char confirmCode)
{
    MacDot16MacHeader* macHeader;
    MacDot16DsxRvdMsg* dsxRvd;

    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("built a DSX-RVD to SS"
               "with primary cid %d for transId %d\n",
               ssInfo->primaryCid, transId);
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->primaryCid);

    // Set the common message body
    dsxRvd = (MacDot16DsxRvdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsxRvdMsg);

    dsxRvd->type = (UInt8)DOT16_DSX_RVD;
    MacDot16ShortToTwoByte(dsxRvd->transId[0], dsxRvd->transId[1],
                           transId);

    dsxRvd->confirmCode = confirmCode;

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

//--------------------------------------------------------------------------
// Start dot16e: build msg part
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16eBsBuildMobNbrAdvPdu
// LAYER      :: MAC
// PURPOSE    :: Build a MOB-NBR-ADV PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16eBsBuildMobNbrAdvPdu(
             Node* node,
             MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader macHeader;
    MacDot16MacHeader* macHeaderPtr;
    MacDot16eMobNbrAdvMsg nbrAdv;
    Message* pduMsg;
    std::string macFrame;

    int i;
    int effectiveNbrBs = 0;

    if (DEBUG_HO && 0) // disable it
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build NBR ADV msg\n");
    }

    // reserve sm space
    macFrame.reserve(5000);

    // add the generic MAC header first
    //index += sizeof(MacDot16MacHeader);

    memset((char*) &macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(&macHeader, DOT16_BROADCAST_CID);
    macFrame.append((char*)&macHeader, sizeof(MacDot16MacHeader));

    // Set the common message body
    //index += sizeof(MacDot16eMobNbrAdvMsg);

    nbrAdv.type = DOT16e_MOB_NBR_ADV;

    // populate other members
    nbrAdv.opFiledBitMap = 0x0F; // no operationId, no Bs Id,
                                  // no HO optimization and no QoS field
    nbrAdv.fragmentaion =0x10;   // no fragmnetaion

    nbrAdv.configChangeCount = dot16Bs->configChangeCount;

    macFrame.append((char*)&nbrAdv, sizeof(MacDot16eMobNbrAdvMsg));

    // Effective BS may not the same as dot16Bs->numNbrBs

    // for each active neighbor BS, put its info in the msg
    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i++)
    {
        if (!dot16Bs->nbrBsInfo[i].isActive)
        {
           continue;
        }

        // effective NBR BS, BS whose informaiton is in hand
        effectiveNbrBs ++;

        // add nbr info for each bnr BS
        int lenIndex = (int)macFrame.size();
        UInt8 tempVal = 0;
        
        // Length
        macFrame.append((char*)&tempVal, sizeof(tempVal)); // will be adjusted at the end of the
                                // iteration

        // PHY Profile Id
        macFrame.append((char*)&tempVal, sizeof(tempVal)); // phy profile Id, needs correct values

        // DCD UCD LSBs
        macFrame.append((char*)&tempVal, sizeof(tempVal)); // To give the rght value

        // DCD settings
        tempVal =  (UInt8)TLV_MOB_NBR_ADV_DcdSettings;     
        macFrame.append((char*)&tempVal, sizeof(tempVal));

        int dcdIndex = (int)macFrame.size();

        tempVal = 0;
        macFrame.append((char*)&tempVal, sizeof(tempVal)); // length will be adjusted later

        // DL channel number
        tempVal = (UInt8)TLV_DCD_ChNumber;
        macFrame.append((char*)&tempVal, sizeof(tempVal));
        tempVal = 1;
        macFrame.append((char*)&tempVal, sizeof(tempVal));
        tempVal = (UInt8)dot16Bs->nbrBsInfo[i].dlChannel;
        macFrame.append((char*)&tempVal, sizeof(tempVal));

        // BS ID
        tempVal = (UInt8)TLV_DCD_BsId;
        macFrame.append((char*)&tempVal, sizeof(tempVal));
        tempVal = 6;
        macFrame.append((char*)&tempVal, sizeof(tempVal));

        macFrame.append((char*)(dot16Bs->nbrBsInfo[i].nbrBsId), 
                        sizeof(dot16Bs->nbrBsInfo[i].nbrBsId));

        // more
        // we assume all the other settings are the same as serving BS

        // adjust the length for dcd setting
        macFrame[dcdIndex] = (UInt8)(macFrame.size() - dcdIndex -1);

        // UCD settings
        tempVal = (UInt8)TLV_MOB_NBR_ADV_UcdSettings;
        macFrame.append((char*)&tempVal, sizeof(tempVal));

        int ucdIndex = (int)macFrame.size();

        tempVal = 0;
        macFrame.append((char*)&tempVal, sizeof(tempVal));

        // more
        // we assume all the other settings are the same as serving BS

        // adjust the length for ucd settings
        macFrame[ucdIndex] = (UInt8)(macFrame.size() - ucdIndex - 1);

        // adjust the length for this iteration
        macFrame[lenIndex] = (UInt8)(macFrame.size() - lenIndex - 1);
    }

    // Put numNeighbors as the last memeber in MacDot16eMobNbrAdvMsg
    macFrame[sizeof(MacDot16MacHeader) +
             sizeof(MacDot16eMobNbrAdvMsg) -1] = (UInt8)effectiveNbrBs;

    // Set the length field of the MAC header
    macHeaderPtr = (MacDot16MacHeader*)macFrame.data();
    MacDot16MacHeaderSetLEN(macHeaderPtr, macFrame.size());

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, (int)macFrame.size(), TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg), macFrame.data(), macFrame.size());

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16eBsBuildMobScnRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a MOB-SCN-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// + scanReq   : MacDot16MobScnReqMsg* : MOB-SCN-RSP msg
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16eBsBuildMobScnRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             MacDot16eMobScnReqMsg* scanReq)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobScnRspMsg* scanRsp;
    MacDot16eMobScnRspInterval* scanInterval;
    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    scanRsp = (MacDot16eMobScnRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobScnRspMsg);

    scanRsp->type = DOT16e_MOB_SCN_RSP;
    scanRsp->duration = scanReq->duration;

    // no report needed, as default
    // scanRsp->reportMode = DOT16e_SCAN_NoReport;

    scanRsp->reportMode = DOT16e_SCAN_EventTrigReport;
    scanRsp->reportMetric = 0x03; // only CINR and RSSI are used now

    // update stats
    dot16Bs->stats.numNbrScanReqRcvd ++;

    if (scanRsp->duration != 0)
    {
        // scan interval
        scanInterval = (MacDot16eMobScnRspInterval*) &(macFrame[index]);
        index += sizeof(MacDot16eMobScnRspInterval);

        scanInterval->startFrame = 0; // start in next frame
        scanInterval->interval = scanReq->interval;
        scanInterval->iteration = scanReq->iteration;
        scanInterval->numBsIndex = scanReq->numBsIndex;
        scanInterval->numBsId = scanReq->numBsId;

        // set variables for local scheduling, if a SS in nbr scan mode,
        // it will be ignored for DL/UL scheduling

        // set TRUE after check outgoing msg whn scheduling

        ssInfo->scanDuration = scanRsp->duration;
        ssInfo->scanInterval = scanInterval->interval;
        ssInfo->scanIteration = scanInterval->iteration;
        ssInfo->numInterFramesLeft = 0;
        ssInfo->numScanFramesLeft = 0;
        ssInfo->numScanIterationsLeft = 0;

        if (DEBUG_NBR_SCAN)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("SS w/basic CID %d need scanning\n", ssInfo->basicCid);
        }
    }

    // HMAC

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16eBsBuildMobBshoRspPdu
// LAYER      :: MAC
// PURPOSE    :: Build a BSHO-RSP msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the SS info
// RETURN     :: Message* : Point to the MOB_BSHO_RSP msg
// **/
static
Message* MacDot16eBsBuildMobBshoRspPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobBshoRspMsg* bsHoRsp;
    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    bsHoRsp = (MacDot16eMobBshoRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobBshoRspMsg);

    bsHoRsp->type = DOT16e_MOB_BSHO_RSP;
    bsHoRsp->mode = DOT16e_BSHO_MODE_HoRequest;

    // HO operation mode
    // currently, recommend HO is used
    bsHoRsp->hoOpMode = DOT16e_BSHO_RecomHo;

    // resource flag
    bsHoRsp->resrcRetainFlag = dot16Bs->resrcRetainFlag;

    bsHoRsp->actionTime = 0;

    if (bsHoRsp->mode == DOT16e_BSHO_MODE_HoRequest)
    {
        // N_recommended
        // currently, leave it to SS to choose the target BS
        macFrame[index ++] = 0;

        // for each recommend BS

        // skip others right now
    }

    // HMAC

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16eBsBuildMobBshoReqPdu
// LAYER      :: MAC
// PURPOSE    :: Build a BSHO-REQ msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the SS info
// + bsId      : unsigned char* : target Bs Id
// RETURN     :: Message* : Point to the MOB_BSHO_REQ msg
// **/
static
Message* MacDot16eBsBuildMobBshoReqPdu(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo,
             unsigned char* bsId)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobBshoReqMsg* bsHoReq;
    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    bsHoReq = (MacDot16eMobBshoReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobBshoReqMsg);

    bsHoReq->type = DOT16e_MOB_BSHO_REQ;
    bsHoReq->mode = DOT16e_BSHO_MODE_HoRequest;

    // HO operation mode
    // currently, recommend HO is used
    bsHoReq->hoOpMode = DOT16e_BSHO_RecomHo;

    // resource flag
    bsHoReq->resrcRetainFlag = dot16Bs->resrcRetainFlag;

    bsHoReq->actionTime = 0;

    if (bsHoReq->mode == DOT16e_BSHO_MODE_HoRequest)
    {
        // N_recommended
        // currently, leave it to SS to choose the target BS
        macFrame[index ++] = 0;

        // for each recommend BS

        // skip others right now
    }

    // HMAC

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}
// /**
// FUNCTION   :: MacDot16eBsBuildDregCmdPdu
// LAYER      :: MAC
// PURPOSE    :: Build a DREG-CMD msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the SS info
// + actCode   : MacDot16eDregCmdActCode : DREG CMD action code
// + dregReqDur: UInt8         : DREG-REQ Duration, if used
// RETURN     :: Message* : Point to the MOB_BSHO_REQ msg
// **/
static
Message* MacDot16eBsBuildDregCmdPdu(Node* node,
                                    MacDataDot16* dot16,
                                    MacDot16BsSsInfo* ssInfo,
                                    MacDot16eDregCmdActCode actCode,
                                    UInt8 dregReqDur = 0)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eDregCmdMsg* dregCmd;
    Message* pduMsg;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    int index = 0;
    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Build a DREG-CMD msg\n");
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);

    // Set the common message body
    dregCmd = (MacDot16eDregCmdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eDregCmdMsg);

    dregCmd->type = (UInt8)DOT16_DREG_CMD;
    dregCmd->actCode = (UInt8)actCode;

    if (actCode == DOT16e_DREGCMD_Code5)
    {
        UInt8 idleModeRetainInfo;

        //set required idle mode retain info
        idleModeRetainInfo = DOT16e_RETAIN_Sbc | DOT16e_RETAIN_Reg |
                DOT16e_RETAIN_Nw;

        macFrame[index++] = TLV_DREG_PagingInfo;
        macFrame[index++] = 5;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
                               dot16Bs->para.pagingCycle);
        macFrame[index + 2] = dot16Bs->para.pagingOffset;
        MacDot16ShortToTwoByte(macFrame[index + 3], macFrame[index + 4],
                               dot16Bs->pagingGroup[0]);
        index += 5;

        macFrame[index++] = TLV_DREG_PagingCtrlId;
        macFrame[index++] = sizeof(Address);
        memcpy(&(macFrame[index]), &dot16Bs->pagingController, sizeof(Address));
        index += sizeof(Address);

        macFrame[index ++] = TLV_DREG_IdleModeRetainInfo;
        macFrame[index ++] = 1;
        macFrame[index ++] = idleModeRetainInfo;

        macFrame[index++] = TLV_DREG_MacHashSkipThshld;
        macFrame[index++] = 2;
        MacDot16ShortToTwoByte(macFrame[index], macFrame[index + 1],
                               dot16Bs->para.macHashSkipThshld);
        index += 2;

        if (dregReqDur)
        {
            macFrame[index++] = TLV_DREG_ReqDuration;
            macFrame[index++] = 1;
            macFrame[index++] = dregReqDur;
        }
    }
    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);

    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16eBsBuildMobPagAdvPdu
// LAYER      :: MAC
// PURPOSE    :: Build a MOB-PAG-ADV PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16eBsBuildMobPagAdvPdu(
                                Node* node,
                                MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    //MacDot16eMobPagAdvMsg* pagAdv;
    Message* pduMsg = NULL;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    int index = 0;
    int i;
    //int num_macs = 0;

    if (DEBUG_IDLE)
    {   MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build PAG ADV msg. Current FrameNumber = %d\n",
        dot16Ofdma->frameNumber);
    }

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID);
/*
    // Set the common message body
    pagAdv = (MacDot16eMobPagAdvMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobPagAdvMsg);

    pagAdv->type = DOT16e_MOB_PAG_ADV;

    //currently handling only one group id
    pagAdv->numPagingGrpId = 1;
    pagAdv->numMac = 0;
*/

    macFrame[index++] = DOT16e_MOB_PAG_ADV;
    MacDot16ShortToTwoByte (macFrame[index], macFrame[index + 1], 1);
    index += 2;

    UInt8 numMac = 0;
    int numMacIndex = index;
    index++;
    MacDot16ShortToTwoByte (macFrame[index], macFrame[index + 1],
                            dot16Bs->pagingGroup[0]);
    index += 2;
    // for each active neighbor BS, put its info in the msg
    for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
    {
        if (!dot16Bs->msPgInfo[i].isValid)
        {
            continue;
        }
        unsigned char macHash[DOT16e_MAC_HASH_LENGTH];

        MacDot16eUInt64ToMacHash(macHash,
            MacDot16eCalculateMacHash(dot16Bs->msPgInfo[i].msMacAddress));
        macFrame[index++] = macHash[0];
        macFrame[index++] = macHash[1];
        macFrame[index++] = macHash[2];
        macFrame[index++] = dot16Bs->msPgInfo[i].actCode;
        // number of mac hash sent
        //pagAdv->numMac ++;
        numMac ++;
    }
    if (numMac > 0)
    {
    macFrame[numMacIndex] = numMac;
    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);
    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg), macFrame, index);
    }
    return pduMsg;
}

// /**
// FUNCTION   :: MacDot16eBsSendImExitReq
// LAYER      :: MAC
// PURPOSE    :: Build and send IM_exit Req to PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo*  : Pointer to SS related info
// RETURN     :: Message* : Message containing the PDU
// **/
static
void MacDot16eBsSendImExitReq(
                    Node* node,
                    MacDataDot16* dot16,
                    MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* exitMsg;
    Dot16eIMExitReqPdu exitReq;
    Dot16BackboneMgmtMsgHeader headInfo;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending IM Exit Request\n");
    }

    headInfo.msgType =
            DOT16e_BACKBONE_MGMT_IM_Exit_State_Change_req;
    exitMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, exitMsg,
                        sizeof(Dot16eIMExitReqPdu),
                        TRACE_DOT16);

    exitReq.BSId = node->nodeId;
    MacDot16CopyMacAddress(exitReq.msMacAddress,
                           ssInfo->macAddress);
    memcpy(MESSAGE_ReturnPacket(exitMsg),
           &exitReq,
           sizeof(Dot16eIMExitReqPdu));

    Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                node,
                dot16,
                dot16Bs->pagingController,
                exitMsg,
                headInfo,
                0);

}

// /**
// FUNCTION   :: MacDot16eBsSendImEntryReq
// LAYER      :: MAC
// PURPOSE    :: Build and send IM_exit Req to PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo*  : Pointer to SS related info
// RETURN     :: Message* : Message containing the PDU
// **/
static
void MacDot16eBsSendImEntryReq(
            Node* node,
            MacDataDot16* dot16,
            MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* entryMsg;
    Dot16eIMEntryReqPdu entryReq;
    Dot16BackboneMgmtMsgHeader headInfo;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending IM Entry Request\n");
    }

    headInfo.msgType =
            DOT16e_BACKBONE_MGMT_IM_Entry_State_Change_req;
    entryMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, entryMsg,
                        sizeof(Dot16eIMEntryReqPdu),
                        TRACE_DOT16);

    MacDot16CopyMacAddress(entryReq.msMacAddress,
                           ssInfo->macAddress);

    NetworkGetInterfaceInfo(node, dot16->myMacData->interfaceIndex,
        &entryReq.bsIpAddress);
    memcpy(&entryReq.initialBsIpAddress, &entryReq.bsIpAddress,
        sizeof(Address));
    entryReq.BSId = node->nodeId;
    entryReq.initialBsId = entryReq.BSId;
    entryReq.idleModeRetainInfo = ssInfo->idleModeRetainInfo;
    entryReq.pagingGroupId = dot16Bs->pagingGroup[0];
    entryReq.pagingCycle = dot16Bs->para.pagingCycle;
    entryReq.pagingOffset = dot16Bs->para.pagingOffset;
    entryReq.basicCid = ssInfo->basicCid;
    entryReq.primaryCid = ssInfo->primaryCid;
    entryReq.secondaryCid = ssInfo->secondaryCid;
    entryReq.ssBasicCapability = ssInfo->ssBasicCapability;
    entryReq.ssAuthKeyInfo = ssInfo->ssAuthKeyInfo;
    entryReq.numCidSupport = ssInfo->numCidSupport;

    memcpy(MESSAGE_ReturnPacket(entryMsg),
           &entryReq,
           sizeof(Dot16eIMEntryReqPdu));

    Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    dot16Bs->pagingController,
                    entryMsg,
                    headInfo,
                    0);
}

// /**
// FUNCTION   :: MacDot16eBsSendLUReq
// LAYER      :: MAC
// PURPOSE    :: Build and send Location update Req to PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo*  : Pointer to SS related info
// RETURN     :: void
// **/
static
void MacDot16eBsSendLUReq(
            Node* node,
            MacDataDot16* dot16,
            MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* luMsg;
    Dot16eLUPdu luReq;
    Dot16BackboneMgmtMsgHeader headInfo;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending Location Update Request\n");
    }

    headInfo.msgType =
            DOT16e_BACKBONE_MGMT_LU_Req;
    luMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, luMsg,
                        sizeof(Dot16eLUPdu),
                        TRACE_DOT16);
    luReq.BSId = node->nodeId;
    luReq.pagingGroupId = dot16Bs->pagingGroup[0];
    luReq.pagingCycle = dot16Bs->para.pagingCycle;
    luReq.pagingOffset = dot16Bs->para.pagingOffset;
    MacDot16CopyMacAddress(luReq.msMacAddress,
                           ssInfo->macAddress);
    memcpy(MESSAGE_ReturnPacket(luMsg),
           &luReq,
           sizeof(Dot16eLUPdu));

    Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    dot16Bs->pagingController,
                    luMsg,
                    headInfo,
                    0);

}

// /**
// FUNCTION   :: MacDot16eBsSendIniPagReq
// LAYER      :: MAC
// PURPOSE    :: Build and send Initiate Paging Req to PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo*  : Pointer to SS related info
// RETURN     :: void
// **/
static
void MacDot16eBsSendIniPagReq(
        Node* node,
        MacDataDot16* dot16,
        MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* iniPagMsg;
    Dot16eInitiatePagingPdu iniPagReq;
    Dot16BackboneMgmtMsgHeader headInfo;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending Initiate Paging Request to Paging Controller");
    }

    headInfo.msgType =
            DOT16e_BACKBONE_MGMT_Initiate_Paging_Req;
    iniPagMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, iniPagMsg,
                        sizeof(Dot16eInitiatePagingPdu),
                        TRACE_DOT16);

    iniPagReq.BSId = node->nodeId;

    MacDot16CopyMacAddress(iniPagReq.msMacAddress,
                           ssInfo->macAddress);
    memcpy(MESSAGE_ReturnPacket(iniPagMsg),
           &iniPagReq,
           sizeof(Dot16eInitiatePagingPdu));

    Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    dot16Bs->pagingController,
                    iniPagMsg,
                    headInfo,
                    0);
}

// /**
// FUNCTION   :: MacDot16eBsSendPagInfo
// LAYER      :: MAC
// PURPOSE    :: Build and send Paging Info to PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void
// **/
static
void MacDot16eBsSendPagInfo(
                Node* node,
                MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* pagInfoMsg = NULL;
    Dot16BackboneMgmtMsgHeader headInfo;
    Dot16eBackbonePagingInfo pagInfo;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending Paging Info to Paging Controller");
    }

    headInfo.msgType =
            DOT16e_BACKBONE_MGMT_Paging_info;

    pagInfoMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pagInfoMsg,
                        sizeof(Dot16eBackbonePagingInfo),
                        TRACE_DOT16);

    NetworkGetInterfaceInfo(node, dot16->myMacData->interfaceIndex,
        &pagInfo.bsIPAddress);
    pagInfo.bsId = node->nodeId;

    pagInfo.pagingGroupId = dot16Bs->pagingGroup[0];
    memcpy(MESSAGE_ReturnPacket(pagInfoMsg),
           &pagInfo,
           sizeof(Dot16eBackbonePagingInfo));

    Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                    node,
                    dot16,
                    dot16Bs->pagingController,
                    pagInfoMsg,
                    headInfo,
                    0);

}
//--------------------------------------------------------------------------
// End dot16e: build msg part
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16BsBuildMacFrame
// LAYER      :: MAC
// PURPOSE    :: Build a MAC frame to be transmitted
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + dlMapLength : int*        : To return length of the DL-MAP
// RETURN     :: Message* : Point to the first message of the MAC frame
// **/
static
Message* MacDot16BsBuildMacFrame(Node* node,
                                 MacDataDot16* dot16,
                                 int* dlMapLength)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    Message* firstMsg = NULL;
    Message* lastMsg = NULL;
    Message* tmpMsg = NULL;
    int i;

    int dlMapIEIndex = 0;
    unsigned char* macFrame;
    int startIndex;
    int startIndexBase;

    int burstSizeInBytes;
    Dot16BurstInfo burstInfo;

    // reset the allocation information
    MacDot16SchResetDlAllocation(node, dot16, dot16Bs->para.tddDlDuration);

    // adjust numDlMapIEScheduled
    // since DCD and UCD are treated equally
    // as broadcast flow
    if (dot16Bs->dcdScheduled &&
        dot16Bs->ucdScheduled)
    {
        dot16Bs->numDlMapIEScheduled -= 1;
    }
    if (dot16Bs->bcastPsAllocated > 0 &&
        (dot16Bs->dcdScheduled ||
        dot16Bs->ucdScheduled))
    {
        // if there is any broadcast flow
        dot16Bs->numDlMapIEScheduled -= 1;
    }

    // add DL-MAP/UL-MAP message
    firstMsg = MacDot16BsBuildDlMapPdu(node, dot16, dlMapLength);
    lastMsg = MacDot16BsBuildUlMapPdu(node, dot16);


    // Modify the status for send CDMA code
    if ((dot16Bs->rngType == DOT16_CDMA ||
        dot16Bs->bwReqType == DOT16_BWReq_CDMA))
    {
        MacDot16RngCdmaInfoList* tempCdmaInfo = NULL;
        tempCdmaInfo = dot16Bs->recCDMARngList;
        while (tempCdmaInfo != NULL)
        {
            if (tempCdmaInfo->rngRspSentFlag == FALSE
                && tempCdmaInfo->codetype == DOT16_CDMA_INITIAL_RANGING_CODE)
            {
                tempCdmaInfo->rngRspSentFlag = TRUE;
            }
            tempCdmaInfo = tempCdmaInfo->next;
        }
    }

    firstMsg->next = lastMsg;

    // alloc burst for DL-MAP and UL-MAP and put in info field for PHY
    burstSizeInBytes = MESSAGE_ReturnPacketSize(firstMsg);
    MacDot16SchAllocDlBurst(node,
                            dot16,
                            burstSizeInBytes,
                            DOT16_DIUC_BCAST,
                            &burstInfo);
    MacDot16AddBurstInfo(node, firstMsg, &burstInfo);

    burstSizeInBytes = MESSAGE_ReturnPacketSize(lastMsg);
    MacDot16SchAllocDlBurst(node,
                            dot16,
                            burstSizeInBytes,
                            DOT16_DIUC_BCAST,
                            &burstInfo);
    MacDot16AddBurstInfo(node, lastMsg, &burstInfo);

    startIndexBase = sizeof(MacDot16MacHeader) +
                     sizeof(MacDot16DlMapMsg) +
                     sizeof(MacDot16PhyOfdmaSyncField);

    // get the DL_MAP
    macFrame = (unsigned char*) MESSAGE_ReturnPacket(firstMsg);
    startIndex = startIndexBase +
                 dlMapIEIndex *
                 sizeof(MacDot16PhyOfdmaDlMapIE);

    // update DL MAP IE  for UL MAP
    MacDot16PhyAddDlMapIE(node,
                          dot16,
                          macFrame,
                          startIndex,
                          DOT16_BROADCAST_CID,
                          DOT16_DIUC_BCAST,
                          burstInfo);
    dlMapIEIndex ++;

    burstSizeInBytes = 0;
    tmpMsg = lastMsg;

    // add DCD message if needed
    if (dot16Bs->dcdScheduled)
    {
        dot16Bs->dcdScheduled = FALSE;
        dot16Bs->lastDcdSentTime = node->getNodeTime();

        lastMsg->next = MESSAGE_Duplicate(node, dot16Bs->dcdPdu);
        lastMsg = lastMsg->next;
        burstSizeInBytes += MESSAGE_ReturnPacketSize(lastMsg);

        if (dot16Bs->dcdUpdateInfo.chUpdateState ==
            DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_START)
        {
            dot16Bs->dcdUpdateInfo.numChDescSentInTransition ++;
            if (dot16Bs->dcdUpdateInfo.numChDescSentInTransition >=
                DOT16_MAX_CH_DESC_SENT_IN_TRANSITION)
            {
                // start transtion timer
                if (dot16Bs->dcdUpdateInfo.timerChDescTransition != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                            node,
                            dot16Bs->dcdUpdateInfo.timerChDescTransition);
                }
                dot16Bs->dcdUpdateInfo.timerChDescTransition =
                        MacDot16SetTimer(
                            node,
                            dot16,
                            DOT16_TIMER_DcdTransition,
                            dot16Bs->para.dcdTransitionInterval,
                            NULL,
                            0);

                //move the state to count down
                dot16Bs->dcdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_COUNTDOWN;
            }
        }

        // increase statistics
        dot16Bs->stats.numDcdSent ++;
    }

    // add UCD message if needed
    if (dot16Bs->ucdScheduled)
    {
        dot16Bs->ucdScheduled = FALSE;
        dot16Bs->lastUcdSentTime = node->getNodeTime();

        lastMsg->next = MESSAGE_Duplicate(node, dot16Bs->ucdPdu);
        lastMsg = lastMsg->next;
        burstSizeInBytes += MESSAGE_ReturnPacketSize(lastMsg);

        if (dot16Bs->ucdUpdateInfo.chUpdateState ==
            DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_START)
        {
            dot16Bs->ucdUpdateInfo.numChDescSentInTransition ++;
            if (dot16Bs->ucdUpdateInfo.numChDescSentInTransition >=
                DOT16_MAX_CH_DESC_SENT_IN_TRANSITION)
            {
                // start transtion timer
                if (dot16Bs->ucdUpdateInfo.timerChDescTransition != NULL)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        dot16Bs->ucdUpdateInfo.timerChDescTransition);
                }
                dot16Bs->ucdUpdateInfo.timerChDescTransition =
                        MacDot16SetTimer(
                            node,
                            dot16,
                            DOT16_TIMER_UcdTransition,
                            dot16Bs->para.ucdTransitionInterval,
                            NULL,
                            0);
                //move the state to count down
                dot16Bs->ucdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_COUNTDOWN;
            }
        }

        // increase statistics
        dot16Bs->stats.numUcdSent ++;
    }

    // add DL burst for multicast/broadcast service flows
    if (dot16Bs->bcastOutBuffHead != NULL)
    {
        lastMsg->next = dot16Bs->bcastOutBuffHead;
        lastMsg = dot16Bs->bcastOutBuffTail;
        burstSizeInBytes += dot16Bs->bytesInBcastOutBuff;

        dot16Bs->bcastOutBuffHead = NULL;
        dot16Bs->bcastOutBuffTail = NULL;
        dot16Bs->bytesInBcastOutBuff = 0;
        dot16Bs->bcastPsAllocated = 0;

    }

    // alloc DL burst for the broadcast messages
    if (burstSizeInBytes > 0)
    {
        MacDot16SchAllocDlBurst(node,
                                dot16,
                                burstSizeInBytes,
                                DOT16_DIUC_BCAST,
                                &burstInfo);
        MacDot16AddBurstInfo(node, tmpMsg->next, &burstInfo);

        // get the DL_MAP
        macFrame = (unsigned char*) MESSAGE_ReturnPacket(firstMsg);
        startIndex = startIndexBase +
                     dlMapIEIndex *
                     sizeof(MacDot16PhyOfdmaDlMapIE);

        // update DL MAP IE  for broadcast burst
        MacDot16PhyAddDlMapIE(node,
                              dot16,
                              macFrame,
                              startIndex,
                              DOT16_BROADCAST_CID,
                              DOT16_DIUC_BCAST,
                              burstInfo);
        dlMapIEIndex ++;
    }

    // add DL burst for individual SSs
    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        while (ssInfo != NULL)
        {
            if (ssInfo->dlPsAllocated > 0)
            {
                if (ssInfo->outBuffHead != NULL)
                {
                    lastMsg->next = ssInfo->outBuffHead;
                    lastMsg = ssInfo->outBuffTail;

                    MacDot16SchAllocDlBurst(node,
                                            dot16,
                                            ssInfo->bytesInOutBuff,
                                            ssInfo->diuc,
                                            &burstInfo);

                    MacDot16AddBurstInfo(node,
                                         ssInfo->outBuffHead,
                                         &burstInfo);

                    // get the DL_MAP
                    macFrame =
                        (unsigned char*) MESSAGE_ReturnPacket(firstMsg);
                    startIndex = startIndexBase +
                                 dlMapIEIndex *
                                 sizeof(MacDot16PhyOfdmaDlMapIE);

                    // update DL MAP IE  for each SS's burst
                    MacDot16PhyAddDlMapIE(node,
                                          dot16,
                                          macFrame,
                                          startIndex,
                                          ssInfo->basicCid,
                                          ssInfo->diuc,
                                          burstInfo);

                    dlMapIEIndex ++;
                    ssInfo->outBuffHead = NULL;
                    ssInfo->outBuffTail = NULL;
                    ssInfo->bytesInOutBuff = 0;
                    ssInfo->dlPsAllocated = 0;
                }
            }

            ssInfo = ssInfo->next;
        }
    }

    lastMsg->next = NULL;

    // Add DIUC-END-OF-MAP
    memset(&burstInfo, 0, sizeof(Dot16BurstInfo));

    // get the DL_MAP
    macFrame = (unsigned char*) MESSAGE_ReturnPacket(firstMsg);
    startIndex = startIndexBase +
                 dlMapIEIndex *
                 sizeof(MacDot16PhyOfdmaDlMapIE);

    // update DL MAP IE  for broadcast burst
    MacDot16PhyAddDlMapIE(node,
                          dot16,
                          macFrame,
                          startIndex,
                          DOT16_BROADCAST_CID,
                          DOT16_DIUC_END_OF_MAP,
                          burstInfo);
    dlMapIEIndex ++;

    if (dot16Bs->numDlMapIEScheduled != dlMapIEIndex)
    {
        ERROR_ReportWarning(" The # of scheudled DL-MAP_IE is "
                            "not the same as the # actually added"
                            "in the DL-MAP msg\n");
    }

    return firstMsg;
}

// /**
// FUNCTION   :: MacDot16BsScheduleMacFrame
// LAYER      :: MAC
// PURPOSE    :: Schedule various portion of a MAC frame.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void : NULL
// **/
static
void MacDot16BsScheduleMacFrame(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    clocktype currentTime;
    clocktype duration;

    currentTime = node->getNodeTime();

    // Check if time to send a DCD message
    if (dot16Bs->dcdChanged ||
        dot16Bs->lastDcdSentTime + dot16Bs->para.dcdInterval <= currentTime)
    {
        // need to send a DCD message
        if (dot16Bs->dcdChanged)
        {
            // if changed, then need to rebuild the DCD message as the
            // cached one is outdated.

            dot16Bs->dcdCount ++;

            if (dot16Bs->dcdUpdateInfo.chUpdateState ==
                DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT)
            {
                //change the state from INIT to NULL after the first DCD
                dot16Bs->dcdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL;
            }
            else
            {
                dot16Bs->dcdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_START;
                dot16Bs->dcdUpdateInfo.numChDescSentInTransition = 0;
            }
            MacDot16BsBuildDcdPdu(node, dot16);
            dot16Bs->dcdChanged = FALSE;

            // send a hello msg to nbr BS
            if (dot16->dot16eEnabled)
            {
                int i;

                for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i++)
                {
                    Message* helloMsg;
                    NodeAddress bsNodeId;
                    Address destNodeAddress;
                    Dot16BackboneMgmtMsgHeader headInfo;

                    bsNodeId = dot16Bs->nbrBsInfo[i].bsNodeId;
                    if (bsNodeId == DOT16_INVALID_BS_NODE_ID)
                    {
                        continue;
                    }

                    helloMsg = MESSAGE_Duplicate(node, dot16Bs->dcdPdu);

                    headInfo.msgType = DOT16_BACKBONE_MGMT_BS_HELLO;

                    destNodeAddress =
                        dot16Bs->nbrBsInfo[i].bsDefaultAddress;

                    Dot16BsSendMgmtMsgToNbrBsOverBackbone(node,
                                                          dot16,
                                                          destNodeAddress,
                                                          helloMsg,
                                                          headInfo,
                                                          15 * SECOND);
                    // update stats
                    dot16Bs->stats.numInterBsHelloSent ++;

                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("send hello to BS %d over backbone\n",
                            bsNodeId);
                    }
                }
            }

        }

        dot16Bs->dcdScheduled = TRUE;
    }

    // Check if time to send a UCD message
    if (dot16Bs->ucdChanged ||
        dot16Bs->lastUcdSentTime + dot16Bs->para.ucdInterval <= currentTime)
    {
        // need to send a UCD message
        if (dot16Bs->ucdChanged)
        {
            // if changed, then need to rebuild the UCD message as the
            // cached one is outdated.

            dot16Bs->ucdCount ++;

            if (dot16Bs->ucdUpdateInfo.chUpdateState ==
                DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT)
            {
                //change the state from INIT to NULL after the first DCD
                dot16Bs->ucdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_NULL;
            }
            else
            {
                dot16Bs->ucdUpdateInfo.chUpdateState =
                    DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_START;
                dot16Bs->ucdUpdateInfo.numChDescSentInTransition = 0;
            }

            MacDot16BsBuildUcdPdu(node, dot16);

            dot16Bs->ucdChanged = FALSE;
        }

        dot16Bs->ucdScheduled = TRUE;
    }

    //
    // Schedule the UL part of the MAC frame
    //
    clocktype actualFrameDuration = dot16Bs->para.frameDuration - 
                                    dot16Bs->para.rtg;
    if (dot16->duplexMode == DOT16_TDD)
    {
        // for TDD, UL part is after DL
        duration = actualFrameDuration -
                   dot16Bs->para.tddDlDuration;
    }
    else
    {
        // for FDD, both DL and UL uses the whole frame duration
        duration = actualFrameDuration;
    }
    duration -= dot16Bs->para.ttg;

    // reset the # of UL-MAP IE in this round of scheduling
    dot16Bs->numUlMapIEScheduled = 0;
    MacDot16ScheduleUlSubframe(node, dot16, duration);

    //
    // Schedule the DL part of the MAC frame
    //
    if (dot16->duplexMode == DOT16_TDD)
    {
        duration = dot16Bs->para.tddDlDuration;
    }
    else
    {
        duration = actualFrameDuration;

    }

    // call the outbound scheduler to decide PDUs to be transmitted in the
    // DL subframe. The scheduler will move PDUs from individual queues to
    // buffer in each SS's structure.

    // reset the # of DL-MAP IE scheduled in this round of scheduling
    dot16Bs->numDlMapIEScheduled = 0;
    MacDot16ScheduleDlSubframe(node, dot16, duration);
}

// /**
// FUNCTION   :: MacDot16eBsEnableDisableServiceFlowQueues
// LAYER      :: MAC
// PURPOSE    :: Enable / Disable Service flow queues
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// +ssInfo     : MacDot16BsSsInfo* : Pointer to SS info
// +index      : int : index of the service types
// +queueBehavior : QueueBehavior : It can be SUSPEND / RESUME
// RETURN     :: void : NULL
// **/
static
void MacDot16eBsEnableDisableServiceFlowQueues(
    Node* node,
    MacDataDot16* dot16,
    MacDot16BsSsInfo* ssInfo,
    int index,
    QueueBehavior queueBehavior)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Scheduler* scheduler = dot16Bs->outScheduler[index];
    MacDot16ServiceFlow* sFlow;

    if (ssInfo == NULL)// for management packet
    {
        sFlow = dot16Bs->mcastFlowList[index].flowHead;
    }
    else
    {
        sFlow = ssInfo->ulFlowList[index].flowHead;
        while (sFlow != NULL)
        {
            scheduler->setQueueBehavior(sFlow->queuePriority, queueBehavior);
            sFlow = sFlow->next;
        }
        sFlow = ssInfo->dlFlowList[index].flowHead;
    }
    while (sFlow != NULL)
    {
        scheduler->setQueueBehavior(sFlow->queuePriority, queueBehavior);
        sFlow = sFlow->next;
    }
    return;
}// end of MacDot16eBsEnableDisableServiceFlowQueues

// /**
// FUNCTION   :: MacDot16eBsNeedToDeactivaePSClassType
// LAYER      :: MAC
// PURPOSE    :: EBS need to deactivate PS defined class
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// +ssInfo     : MacDot16BsSsInfo* : Pointer to SS info
// +classType  : MacDot16ePSClassType : class type
// RETURN     :: BOOL : TRUE /FALSE
// **/
static
BOOL MacDot16eBsNeedToDeactivaePSClassType(Node* node,
                                           MacDataDot16* dot16,
                                           MacDot16BsSsInfo* ssInfo,
                                           MacDot16ePSClassType classType)
{
    int i, j;
    BOOL retVal = FALSE;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16ePSClasses* psClass = NULL;
    Scheduler* scheduler;
    int bytesNeedToSend = 0;
    int bwGranted = 0;
    for (i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        psClass = &ssInfo->psClassInfo[i];
        if ((psClass->classType == classType) &&
            (psClass->isWaitForSlpRsp == FALSE)
            && (psClass->statusNeedToChange == FALSE))
        {
            for (j = 0; j < DOT16_NUM_SERVICE_TYPES; j++)
            {
                sFlow = ssInfo->dlFlowList[j].flowHead;
                scheduler = dot16Bs->outScheduler[j];
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
}// end of MacDot16eBsNeedToDeactivaePSClass

// /**
// FUNCTION   :: MacDot16eBsDeactivatePSClass
// LAYER      :: MAC
// PURPOSE    :: Deactivate defined PS class
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// +ssInfo     : MacDot16BsSsInfo* : Pointer to SS info
// +psClass    : MacDot16ePSClasses* : pointer to PS class
// RETURN     :: BOOL : TRUE /FALSE
// **/
static
BOOL MacDot16eBsDeactivatePSClass(Node* node,
                                  MacDataDot16* dot16,
                                  MacDot16BsSsInfo* ssInfo,
                                  MacDot16ePSClasses* psClass)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ServiceFlow* sFlow = NULL;
    // need to deactivate the class
    // deactivate the class 1 & 2
    MacDot16eSetPSClassStatus(psClass, POWER_SAVING_CLASS_DEACTIVATE);
    psClass->sleepDuration = FALSE;
    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf(" BS Deactivate the class: %d", psClass->classType);
        printf(" for SS %d %d %d"
            "%d %d %d\n",
            ssInfo->macAddress[0],
            ssInfo->macAddress[1],
            ssInfo->macAddress[2],
            ssInfo->macAddress[3],
            ssInfo->macAddress[4],
            ssInfo->macAddress[5]);
    }

    psClass->isSleepDuration = FALSE;
    psClass->statusNeedToChange = FALSE;
    psClass->psClassIdleLastNoOfFrames = 0;
    for (int i =0; i < DOT16_NUM_SERVICE_TYPES; i++)
    {
        sFlow = ssInfo->ulFlowList[i].flowHead;
        if (sFlow == NULL)
        {
            sFlow = ssInfo->dlFlowList[i].flowHead;
        }
        if (sFlow && (psClass->classType ==
            sFlow->psClassType))
        {
            if (DEBUG_SLEEP)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS RESUME class: %d for "
                    "service type: %d\n",
                    psClass->classType,
                    sFlow->serviceType);
            }
            // disable/enable the serviceFlowQueues
            MacDot16eBsEnableDisableServiceFlowQueues(
                node,
                dot16,
                ssInfo,
                i,
                RESUME);
        }
    }// end of for
    return TRUE;
}// end of MacDot16eSsDeactivatePSClass

// /**
// FUNCTION   :: MacDot16eBsHasPacketForPSClassType
// LAYER      :: MAC
// PURPOSE    :: check packet present at BS for given PS class type
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// +classType  : MacDot16ePSClassType : PS class type
// RETURN     :: BOOL : TRUE / FALSE
// **/
static
BOOL MacDot16eBsHasPacketForPSClassType(Node* node,
                                          MacDataDot16* dot16,
                                          MacDot16BsSsInfo* ssInfo,
                                          MacDot16ePSClassType classType)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16ServiceFlowList* flowList = NULL;
    Scheduler* scheduler = NULL;
    if (classType == POWER_SAVING_CLASS_3)
    {
        flowList = dot16Bs->mcastFlowList;
    }
    else
    {
        flowList = ssInfo->dlFlowList;
    }

    for (int i = 0; (i < DOT16_NUM_SERVICE_TYPES); i++)
    {
        sFlow = flowList[i].flowHead;
        scheduler = dot16Bs->outScheduler[i];
        while (sFlow != NULL && sFlow->activated && sFlow->admitted &&
            sFlow->psClassType == classType
            && sFlow->TrafficIndicationPreference)
        {
            if (scheduler->numberInQueue(sFlow->queuePriority) > 0)
            {
                return TRUE;
            }
            sFlow = sFlow->next;
        }// end of while
    }// end of for

    if (classType == POWER_SAVING_CLASS_3)
    {

        scheduler = dot16Bs->mgmtScheduler;
        if (scheduler->numberInQueue(ALL_PRIORITIES) > 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}// end of MacDot16eBsHasPacketForPSClassType

// /**
// FUNCTION   :: MacDot16eBsBuildMobSlpRsp
// LAYER      :: MAC
// PURPOSE    :: Build a MOB-SLP-RSP PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS info
// RETURN     :: Message* : Message containing the PDU
// **/
static
Message* MacDot16eBsBuildMobSlpRsp(
             Node* node,
             MacDataDot16* dot16,
             MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE] = {0, 0};
    Message* pduMsg = NULL;
    MacDot16ePSClasses* psClass = NULL;
    int totalPSClassInfoindex = 0;
    UInt8 totalPSClassInfoNeedToSend = 0;

    int index = 0;

    // add the generic MAC header first
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    if (ssInfo == NULL)
    {
        MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID);
    }
    else
    {
        MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid);
    }

    // Set the common message body
     macFrame[index ++] = DOT16e_MOB_SLP_RSP;
    // maximum number of defined power saving class
    //macFrame[index++] = DOT16e_MAX_POWER_SAVING_CLASS;
    totalPSClassInfoindex = index;
    index++;
    //add TLV
    for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
    {
        if (ssInfo == NULL)
        {
            psClass = &dot16Bs->psClass3;
        }
        else
        {
            psClass = &ssInfo->psClassInfo[i];
        }
        if (psClass->statusNeedToChange)
        {
            int lengthIndex = index;
            totalPSClassInfoNeedToSend++;
            BOOL operation = FALSE;
            BOOL definition = TRUE;
            //BOOL isSleepApproved = TRUE;
            // Set sleep approved
            macFrame[index++] |= 0x80;

            // Set definition
            macFrame[index] = 0x01;
            // Set operation
            if (psClass->currentPSClassStatus ==
                POWER_SAVING_CLASS_ACTIVATE)
            {
                operation = TRUE;
                macFrame[index] |= 0x02;
            }
            psClass->statusNeedToChange = FALSE;
            // Set power saving class id
            index++;
            if (operation == TRUE)
            {
                // set start frame number
                MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)
                    dot16->phyData;
                psClass->startFrameNum = dot16Ofdma->frameNumber + 1;
                // Set 6 bits
                macFrame[index] = (UInt8)
                    (psClass->startFrameNum & 0x0000003F);
                // 2 bits RFU
                index++;
                psClass->sleepDuration = psClass->initialSleepWindow;
            }
            else
            {
                // deactivate the class
                MacDot16eBsDeactivatePSClass(node, dot16, ssInfo, psClass);
            }
            if (definition == TRUE)
            {
                UInt16 twoByteVal = 0;
                // Power saving class type 2 bit
                macFrame[index] = (UInt8)psClass->classType;
                // direction class type 2 bit
                macFrame[index] |= (psClass->psDirection << 2);
                // 4 bit Padding
                index++;

                macFrame[index++] = psClass->initialSleepWindow;
                macFrame[index++] = psClass->listeningWindow;
                // 10 bit  sleep window base
                twoByteVal = psClass->finalSleepWindowBase;
                // 3 bit sleep window exponent
                twoByteVal |= (((UInt16)psClass->finalSleepWindowExponent)
                    << 10);
                // TRF-IND required 1 bit
                // TRUE only for PS class 1
                if (psClass->classType == POWER_SAVING_CLASS_1
                    && DOT16e_TRAFFIC_INDICATION_PREFERENCE)
                {
                    twoByteVal |= (0x0001 << 13);
                }
                // Traffic triggered triggered flag 1 bit
                twoByteVal |= (((UInt16)psClass->trafficTriggeredFlag)
                    << 14);
                // 1 bit RFU
                MacDot16ShortToTwoByte(macFrame[index],
                    macFrame[index + 1],
                    twoByteVal);
                index += 2;
                if (psClass->classType == POWER_SAVING_CLASS_1)
                {
                    if (psClass->currentPSClassStatus ==
                        POWER_SAVING_CLASS_ACTIVATE)
                    {
                        // 10 bits slipid
                        psClass->slipId =
                            dot16Bs->psSleepId & 0x03FF;
                        dot16Bs->psSleepId++;
                    }
                    MacDot16ShortToTwoByte(macFrame[index],
                        macFrame[index + 1],
                        psClass->slipId);
                    // 2 bits RFU
                    // 4 bits no of CIDs
                    index += 2;
                }
                else
                {
                    // 4 bits no of CIDs
                    macFrame[index++] = 0;
                    psClass->slipId = 0;
                }
            }
            // length of the data
            macFrame[lengthIndex] |= (index - lengthIndex - 1);
        }// end of if ssInfo->psClassInfo[i].statusNeedToChange
        if (ssInfo == NULL)
        {
            break;
        }
    }// end of for DOT16e_MAX_POWER_SAVING_CLASS


    macFrame[totalPSClassInfoindex] = totalPSClassInfoNeedToSend;


    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    // create the message
    pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(pduMsg != NULL, "MAC 802.16: Out of memory!");
    MESSAGE_PacketAlloc(node, pduMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(pduMsg),
           macFrame,
           index);
    dot16Bs->stats.numMobSlpRspSent++;
    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("build MOB-SLP-RSP message\n");
    }
    return pduMsg;
}// end of MacDot16eBsBuildMobSlpRsp

// /**
// FUNCTION   :: MacDot16eBsPerformSleepModeOpoeration
// LAYER      :: MAC
// PURPOSE    :: Activate and deactivate PS class
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void : NULL
// **/
static
void MacDot16eBsPerformSleepModeOpoeration(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16ePSClasses* psClass = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
    int i, j, index;

    int currentFrameNumber = dot16Ofdma->frameNumber;
//    clocktype frameDuration = dot16Bs->para.frameDuration;

    if (dot16Bs->psClass3.isSleepDuration == FALSE)
    {
        if ((dot16Bs->psClass3.currentPSClassStatus ==
            POWER_SAVING_CLASS_ACTIVATE) && (dot16Bs->psClass3.startFrameNum
            == currentFrameNumber))
        {
            if (DEBUG_SLEEP_PS_CLASS_3)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Activate PS Class 3\n");
            }
            dot16Bs->psClass3.isSleepDuration = TRUE;
            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i++)
            {
                if (dot16Bs->mcastFlowList[i].flowHead != NULL)
                {
                    MacDot16eBsEnableDisableServiceFlowQueues(node,
                        dot16,
                        NULL,
                        i,
                        SUSPEND);
                }
            }
        }
        else
        {
            BOOL isAnySsAssociated = FALSE;
            for (i = 0; !isAnySsAssociated && (i < DOT16_BS_SS_HASH_SIZE);
                i++)
            {
                ssInfo = dot16Bs->ssHash[i];
                while (ssInfo != NULL)
                {
                    if (ssInfo->isRegistered)
                    {
                        isAnySsAssociated = TRUE;
                        break;
                    }
                    ssInfo = ssInfo->next;
                }
            }
            // check PS3 need to ACTIVATE
            if (isAnySsAssociated &&
                !MacDot16eBsHasPacketForPSClassType(node, dot16, NULL,
                POWER_SAVING_CLASS_3))
            {
                if (dot16Bs->psClass3.psClassIdleLastNoOfFrames <
                    DOT16e_DEFAULT_PS3_IDLE_TIME)
                {
                    dot16Bs->psClass3.psClassIdleLastNoOfFrames++;
                }
                else
                {
                // Send Mob SLP rsp broadcast packet to Activate PS class 3
                    Message* mobRspMsg = NULL;
                    dot16Bs->psClass3.statusNeedToChange = TRUE;
                    dot16Bs->psClass3.psClassIdleLastNoOfFrames = 0;
                    MacDot16eSetPSClassStatus((&dot16Bs->psClass3),
                        POWER_SAVING_CLASS_ACTIVATE);
                    mobRspMsg = MacDot16eBsBuildMobSlpRsp(node, dot16, NULL);
                    if (DEBUG_SLEEP_PS_CLASS_3)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("BS build MOB-SLP-RSP for PS class 3\n");
                    }
                    MacDot16BsScheduleMgmtMsgToSs(node,
                            dot16,
                            NULL,
                            DOT16_BROADCAST_CID,
                            mobRspMsg);
                }
            }
            else
            {
                dot16Bs->psClass3.psClassIdleLastNoOfFrames = 0;
            }
        }
    }
    else
    {
        // check PS3 need to DEACTIVATE
        if ((dot16Bs->psClass3.initialSleepWindow +
            dot16Bs->psClass3.startFrameNum) == currentFrameNumber)
        {
            // deactivate PS3 class
            if (DEBUG_SLEEP_PS_CLASS_3)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Deactivate PS Class 3\n");
            }
            dot16Bs->psClass3.isSleepDuration
                = FALSE;
            for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i++)
            {
                MacDot16eBsEnableDisableServiceFlowQueues(node,
                    dot16,
                    NULL,
                    i,
                    RESUME);
            }
        }
    }
    for (index = 0; index < DOT16_BS_SS_HASH_SIZE; index ++)
    {
        ssInfo = dot16Bs->ssHash[index];
        while (ssInfo != NULL)
        {
            for (i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
            {
                psClass = &ssInfo->psClassInfo[i];
                if ((psClass->isSleepDuration == FALSE) &&
                    (psClass->startFrameNum == currentFrameNumber))
                {
                    ssInfo->mobTrfSent = FALSE;
                    for (j =0; j < DOT16_NUM_SERVICE_TYPES; j++)
                    {
                        // PS class goes in to sleep mode
                        sFlow = ssInfo->ulFlowList[j].flowHead;
                        if (sFlow == NULL)
                        {
                            sFlow = ssInfo->dlFlowList[j].flowHead;
                        }
                        if (sFlow && (psClass->classType == sFlow->psClassType)
                            && (psClass->currentPSClassStatus ==
                            POWER_SAVING_CLASS_ACTIVATE) &&
                            (psClass->statusNeedToChange == FALSE))
                        {
                            // disable the serviceFlowQueue
                            MacDot16eBsEnableDisableServiceFlowQueues(
                                node,
                                dot16,
                                ssInfo,
                                j,
                                SUSPEND);
                            if (DEBUG_SLEEP)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("BS SUSPEND class: %d service type:"
                                    " %d for SS %d %d %d"
                                    "%d %d %d Sleep Duration: %d frames\n",
                                    psClass->classType,
                                    sFlow->serviceType,
                                    ssInfo->macAddress[0],
                                    ssInfo->macAddress[1],
                                    ssInfo->macAddress[2],
                                    ssInfo->macAddress[3],
                                    ssInfo->macAddress[4],
                                    ssInfo->macAddress[5],
                                    psClass->sleepDuration);
                            }
                        }// end of if
                    }// end of for
                    psClass->isSleepDuration = TRUE;
                }
                else if ((psClass->startFrameNum + psClass->sleepDuration) ==
                    currentFrameNumber)
                {
                    // PS class comes in to awake mode
                    for (j =0; j < DOT16_NUM_SERVICE_TYPES; j++)
                    {
                        // PS class goes in to sleep mode
                        sFlow = ssInfo->ulFlowList[j].flowHead;
                        if (sFlow == NULL)
                        {
                            sFlow = ssInfo->dlFlowList[j].flowHead;
                        }
                        if (sFlow && (psClass->classType ==
                            sFlow->psClassType))
                        {
                            if (DEBUG_SLEEP)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("BS RESUME class: %d service type: %d"
                                    " for SS %d %d %d"
                                    "%d %d %d\n",
                                    psClass->classType,
                                    sFlow->serviceType,
                                    ssInfo->macAddress[0],
                                    ssInfo->macAddress[1],
                                    ssInfo->macAddress[2],
                                    ssInfo->macAddress[3],
                                    ssInfo->macAddress[4],
                                    ssInfo->macAddress[5]);
                            }

                            // start timer for listen interval
                            psClass->isSleepDuration = FALSE;
                            // disable the serviceFlowQueue
                            MacDot16eBsEnableDisableServiceFlowQueues(
                                node,
                                dot16,
                                ssInfo,
                                j,
                                RESUME);
                        }// end of if
                    }// end of for
                    psClass->startFrameNum = currentFrameNumber;
                    if (psClass->classType == POWER_SAVING_CLASS_1)
                    {
                        int previousSleepDuration =
                            psClass->sleepDuration;
                        // incr sleep duration
                        psClass->sleepDuration = MIN(2 *
                            previousSleepDuration,
                            (psClass->finalSleepWindowBase *
                            (int)pow(2.0, (double)
                            psClass->finalSleepWindowExponent)));
                    }
                    // set psClass->startFrameNum, to goes in to
                    psClass->startFrameNum +=
                        psClass->listeningWindow;
                    if (psClass->classType == POWER_SAVING_CLASS_2)
                    {
                        if (MacDot16eBsNeedToDeactivaePSClassType(node,
                            dot16, ssInfo, POWER_SAVING_CLASS_2))
                        {
                            // build unsolicited mob_slp_rsp for PS class 2
                            Message* mobRspMsg = NULL;
                            psClass->statusNeedToChange = TRUE;
                            psClass->psClassIdleLastNoOfFrames = 0;
                            MacDot16eSetPSClassStatus((&dot16Bs->psClass3),
                                POWER_SAVING_CLASS_ACTIVATE);
                            mobRspMsg = MacDot16eBsBuildMobSlpRsp(node,
                                dot16, ssInfo);
                            if (DEBUG_SLEEP)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("BS build MOB-SLP-RSP"
                                    " for PS class 2\n");
                            }
                            MacDot16BsScheduleMgmtMsgToSs(node,
                                    dot16,
                                    ssInfo,
                                    ssInfo->basicCid,
                                    mobRspMsg);
                        }
                    }
                } // end of if
            }//end of for
            ssInfo = ssInfo->next;
        }// end of while
    }// end of for
}// end of MacDot16eBsPerformSleepModeOpoeration

// /**
// FUNCTION   :: MacDot16eBsBuildMobTrfInd
// LAYER      :: MAC
// PURPOSE    :: Build a MOB-TRF-IND PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: int : Return length of the message
// **/
static
int MacDot16eBsBuildMobTrfInd(Node* node,
                              MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    unsigned char macFrame[DOT16_MAX_MGMT_MSG_SIZE];
    MacDot16BsSsInfo* ssInfo;
    MacDot16ePSClasses* psClass = NULL;
    BOOL fmt = 0;
    int index;
    UInt32 slipIdGrp = 0;
    UInt32 trafficIndication[32] = {0, 0};
    int j = 0;

    index = 0;

    // add the generic MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetCID(macHeader, DOT16_BROADCAST_CID); // broadcast

    macFrame[index++] = DOT16e_MOB_TRF_IND;
    // Set FMT 1 bit info, value is zero
    // 7 bit padding
    macFrame[index++] = (UInt8)fmt;
    if (fmt == 0)
    {
        // set slip id group indication bit map
        // and traffic indication bit map
        for (int i = 0; i < DOT16_BS_SS_HASH_SIZE; i++)
        {
            ssInfo = dot16Bs->ssHash[i];
            while (ssInfo != NULL && ssInfo->isSleepEnabled
                && (ssInfo->mobTrfSent == FALSE))
            {
                for (j =0; j < DOT16e_MAX_POWER_SAVING_CLASS; j++)
                {
                    psClass = &ssInfo->psClassInfo[j];
                    if ((psClass->classType == POWER_SAVING_CLASS_1) &&
                        (psClass->isSleepDuration == FALSE) &&
                        (psClass->currentPSClassStatus ==
                        POWER_SAVING_CLASS_ACTIVATE) &&
                        (psClass->statusNeedToChange == FALSE))
                    {
                        // check ss has packet for this PS class
                        // Add slip id and traffic info here
                        UInt16 slipId = psClass->slipId;
                        if (MacDot16eBsHasPacketForPSClassType(node, dot16,
                            ssInfo, POWER_SAVING_CLASS_1))
                        {
                            UInt8 temp = (UInt8) slipId / 32;
                            UInt32 intVal = 1;
                            slipIdGrp |= (intVal << temp);
                            trafficIndication[temp] |= (intVal <<
                                (slipId % 32));
                            ssInfo->mobTrfSent = TRUE;
                            if (psClass->trafficTriggeredFlag)
                            {
                                // deactivate the PS class
                                MacDot16eBsDeactivatePSClass(node,
                                    dot16,
                                    ssInfo,
                                    psClass);
                            }
                            else
                            {
                                // Remove previous added sleep duration
                                // sleep continue with initial sleep duration
                                psClass->sleepDuration =
                                    psClass->initialSleepWindow;
                                psClass->startFrameNum +=
                                    psClass->listeningWindow ;
                                if (DEBUG_SLEEP)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf(" BS Set the next sleep duration"
                                        " %d is initial sleep duration",
                                        psClass->sleepDuration);
                                    printf(" for SS %d %d %d"
                                        "%d %d %d\n",
                                        ssInfo->macAddress[0],
                                        ssInfo->macAddress[1],
                                        ssInfo->macAddress[2],
                                        ssInfo->macAddress[3],
                                        ssInfo->macAddress[4],
                                        ssInfo->macAddress[5]);
                                }
                            }
                        }
                    }
                }
                ssInfo = ssInfo->next;
            }
        }// end of for
        if (slipIdGrp != 0)
        {
            MacDot16IntToFourByte((macFrame + index), slipIdGrp);
            index += 4;
            for (int i = 0; i < 32; i++)
            {
                if (trafficIndication[i] != 0)
                {
                    MacDot16IntToFourByte((macFrame + index),
                        trafficIndication[i]);
                    index += 4;
                }
            }
        }// end of if slipIdGrp != 0
    }
    else
    {
        // if fmt == 1
    }

    // Set the length field of the MAC header
    MacDot16MacHeaderSetLEN(macHeader, index);

    if (slipIdGrp != 0)
    {
        Message* mobTrfIndPdu = NULL;
        //copy macFrame to dcdPdu and set pduLength
        mobTrfIndPdu = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(node, mobTrfIndPdu, index, TRACE_DOT16);
        memcpy(MESSAGE_ReturnPacket(mobTrfIndPdu), macFrame, index);
        if (DEBUG_SLEEP)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("build MOB-TRF-IND message\n");
        }
        dot16Bs->stats.numMobTrfIndSent++;
        MacDot16BsScheduleMgmtMsgToSs(node,
                dot16,
                NULL,
                DOT16_BROADCAST_CID,
                mobTrfIndPdu);
    }
// Increase statistics for Mob-Trf-Ind message
    return index;
}// end of MacDot16eBsBuildMobTrfInd


// /**
// FUNCTION   :: MacDot16BsStartFrame
// LAYER      :: MAC
// PURPOSE    :: Start a new frame
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void : NULL
// **/
static
void MacDot16BsStartFrame(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int dlMapLength;
    Message* firstMsg;
    // For Idle Mode
    if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
    {
        Message* mobPagAdv;
        MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)dot16->phyData;
        int currentFrameNumber = dot16Ofdma->frameNumber;

        if (currentFrameNumber == dot16Bs->pagingFrameNo)
        {
            mobPagAdv = MacDot16eBsBuildMobPagAdvPdu(node, dot16);
            if (mobPagAdv != NULL)
            {
                dot16Bs->stats.numMobPagAdvSent++;
                MacDot16BsScheduleMgmtMsgToSs(node,
                                            dot16,
                                            NULL,
                                            DOT16_BROADCAST_CID,
                                            mobPagAdv);
            }
            dot16Bs->pagingFrameNo =
                        currentFrameNumber +
                        dot16Bs->para.pagingCycle;
        }
    }
    // For Sleep Mode
    if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled)
    {
        MacDot16eBsPerformSleepModeOpoeration(node, dot16);
        MacDot16eBsBuildMobTrfInd(node, dot16);
    }

    // Schedule PDUs for this MAC frame
    MacDot16BsScheduleMacFrame(node, dot16);

    // Now, start building the MAC frame
    firstMsg = MacDot16BsBuildMacFrame(node, dot16, &dlMapLength);

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("MAC 802.16: starts txing a MAC frame\n");
    }

    // start transmitting the MAC frame on DL channel
    MacDot16PhyTransmitMacFrame(node,
                                dot16,
                                firstMsg,
                                (unsigned char) dlMapLength,
                                dot16Bs->para.tddDlDuration);
}

// /**
// FUNCTION   :: MacDot16BsStartOperation
// LAYER      :: MAC
// PURPOSE    :: Start the operation of the BS. This is for start/stop
//               BSs or start BS with random delays
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void : NULL
// **/
static
void MacDot16BsStartOperation(Node* node, MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = NULL;

    dot16Bs = (MacDot16Bs*) dot16->bsData;
    ERROR_Assert(dot16Bs != NULL, "MAC 802.16: BS is not initialized!");

    // set a new frame
    MacDot16BsStartFrame(node, dot16);

    // set the timer to start next frame
    MacDot16SetTimer(node,
                     dot16,
                     DOT16_TIMER_FrameBegin,
                     dot16Bs->para.frameDuration,
                     NULL,
                     0);
    if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
    {
        dot16Bs->pagingFrameNo = dot16Bs->para.pagingOffset;
    }
}

// /**
// FUNCTION   :: MacDot16BsSignalStrongEnough
// LAYER      :: MAC
// PURPOSE    :: The received signal is strong enough to decode
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + signalMea : PhySignalMeasurement* : Pointer to the received
//                                       singal strength
// RETURN     :: BOOL          : TRUE or FALSE
// **/
static
BOOL MacDot16BsSignalStrongEnough(Node* node,
                                  MacDataDot16* dot16,
                                  PhySignalMeasurement* signalMea,
                                  char &powerDeviation)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int moduCodeType = PHY802_16_QPSK_R_1_2;
    double signalIndb;
    double diff;
    powerDeviation = 0;
    if (dot16Bs->rngType == DOT16_CDMA)
    {
        moduCodeType = PHY802_16_BPSK;
    }
    signalIndb = IN_DB(PhyDot16GetRxSensitivity_mW(
        node,
        dot16->myMacData->phyNumber,
        moduCodeType));
    diff = signalIndb - signalMea->rss;
    diff = diff / DOT16_SS_DEFAULT_POWER_UNIT_LEVEL;
    powerDeviation = (char)diff;
    if (diff > (int)diff)
    {
        powerDeviation = powerDeviation + 1;
    }
    // ATTENTION: need revisit this if clause
    if (signalMea->rss >= signalIndb)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: MacDot16BsPerformPeriodicRangeRcvProc
// LAYER      :: MAC
// PURPOSE    :: BS perform periodic ranging receiver process
//               after reveiving a signal from phy(pg.203 Fig. 82)
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + cid       : Dot16CIDType  : basic cid associated with this burst
// rngReqInData : BOOL         : Is range request received?
// + signalMea : PhySignalMeasurement* : Pointer to signal strength
//                                       measurement
// RETURN     :: BOOL          : whether SSInfo should be removed
static
BOOL MacDot16BsPerformPeriodicRangeRcvProc(Node* node,
                                           MacDataDot16* dot16,
                                           Dot16CIDType cid,
                                           BOOL rngReqInData,
                                           PhySignalMeasurement* signalMea,
                                           MacDot16RngCdmaInfo cdmaInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    Message* pduMsg;
    char powerDeviation = 0;

    if (DEBUG_PERIODIC_RANGE)
    {
        printf("node %d: perform periodic reception from Cid %d\n",
                node->nodeId, cid);
    }

    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);
    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("No ssInfo for perioidic range rcvd proc");
        }
        return FALSE;
    }

    // clear the invited count
    ssInfo->invitedRngRetry = 0;

    // signal strong enough?
    if (MacDot16BsSignalStrongEnough(node, dot16, signalMea, powerDeviation)
        == TRUE)
    {
        // reset retry count
        ssInfo->rngCorrectRetry = 0;

        if (rngReqInData)
        {
            // RNG-REQ in Data

            // send RNG-RSP with success
            //schedule to send a RNG-RSP to the SS with SUCC status
            pduMsg = MacDot16BsBuildRngRspPdu(
                         node,
                         dot16,
                         ssInfo,
                         ssInfo->basicCid,
                         RANGE_SUCC,
                         powerDeviation,
                         cdmaInfo);

            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->basicCid,
                                          pduMsg);
        }

        //start T27 with idle interval
        if (ssInfo->timerT27 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
            ssInfo->timerT27 = NULL;

        }
        if (dot16Bs->rngType != DOT16_CDMA)
        {
            ssInfo->timerT27 = MacDot16SetTimer(
                                       node,
                                       dot16,
                                       DOT16_TIMER_T27,
                                       dot16Bs->para.t27IdleInterval,
                                       NULL,
                                       ssInfo->basicCid);
            if (DEBUG_PERIODIC_RANGE)
            {
                printf("    periodic reception from Cid %d, signal"
                       "is good\n", cid);
                printf("    reset T27 with idle interval\n");
            }
        }
    }
    else
    {
        //weak signal

        // increment the correction count
        ssInfo->rngCorrectRetry ++;
        if (DEBUG_PERIODIC_RANGE)
        {
            printf("    periodic reception from Cid %d, "
                   " signal is bad retry %d\n",
                   cid, ssInfo->rngCorrectRetry);
        }
        if (ssInfo->rngCorrectRetry >=
                DOT16_MAX_RANGE_CORRECTION_RETRIES)
        {
            // remove SS from BS management
            // deallocate basic and primary CID

            pduMsg = MacDot16BsBuildRngRspPdu(
                         node,
                         dot16,
                         ssInfo,
                         ssInfo->basicCid,
                         RANGE_ABORT,
                         powerDeviation,
                         cdmaInfo);

            MacDot16BsScheduleMgmtMsgToSs(
                        node,
                        dot16,
                        ssInfo,
                        ssInfo->basicCid,
                        pduMsg);

            return TRUE;
        } // rngCorrectionretry
        else
        {
            // send RNG-RSP (continue)
            ssInfo->txPowerAdjust = powerDeviation;
            pduMsg = MacDot16BsBuildRngRspPdu(node,
                                              dot16,
                                              ssInfo,
                                              ssInfo->basicCid,
                                              RANGE_CONTINUE,
                                              powerDeviation,
                                              cdmaInfo);

            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->basicCid,
                                          pduMsg);

            //start T27 with active interval
            if (ssInfo->timerT27 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
                ssInfo->timerT27 = NULL;
            }
            if (dot16Bs->rngType != DOT16_CDMA)
            {
                ssInfo->timerT27 = MacDot16SetTimer(
                                           node,
                                           dot16,
                                           DOT16_TIMER_T27,
                                           dot16Bs->para.t27ActiveInterval,
                                           NULL,
                                           ssInfo->basicCid);
                if (DEBUG_PERIODIC_RANGE)
                {
                    printf("    reset t27 with active interval due to "
                           "perio rng retry\n");
                }
            }
        }// rngCorrectionretry
    } // signal strong enough

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16BsInvitedBurstHasNoData
// LAYER      :: MAC
// PURPOSE    :: Process when invited inital ranging burst has no RNG-REQ
//               (pg.181 Fig. 63)
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the ssInfo
// RETURN     :: BOOL          : whether ssInfo should be removed
// **/
static
BOOL MacDot16BsInvitedBurstHasNoData(Node* node,
                                     MacDataDot16* dot16,
                                     MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* pduMsg;
    MacDot16RngCdmaInfo cdmaInfo;
    memset(&cdmaInfo, 0, sizeof(MacDot16RngCdmaInfo));

    ssInfo->invitedRngRetry ++;

    if (ssInfo->invitedRngRetry >= DOT16_MAX_INVITED_RANGE_RETRIES)
    {
        // send RNG-RSP with abort
        if (DEBUG_PERIODIC_RANGE)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("for cid %d "
                   "SS is out of range, remove SS from list\n",
                   ssInfo->basicCid);
        }
        pduMsg = MacDot16BsBuildRngRspPdu(
                         node,
                         dot16,
                         ssInfo,
                         ssInfo->basicCid,
                         RANGE_ABORT,
                         0,
                         cdmaInfo);

        MacDot16BsScheduleMgmtMsgToSs(
                    node,
                    dot16,
                    ssInfo,
                    ssInfo->basicCid,
                    pduMsg);

        if (!ssInfo->initRangeCompleted)
        {
            // remove SS from poll list
            ssInfo->needInitRangeGrant = FALSE;
        }
        else
        {
            ssInfo->needPeriodicRangeGrant = FALSE;
        }

        return TRUE;
    }
    else
    {
        if (ssInfo->invitedRngRetry > DOT16_MAX_INVITED_RANGE_RETRIES / 2)
        {
            // fall abck to most reliable UIUC or DIUC to see what ahppen
            if (DEBUG_PERIODIC_RANGE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("cid %d UIUC %d and DIUC %d to most reliable one\n",
                        ssInfo->basicCid, ssInfo->diuc, ssInfo->uiuc);
            }
            if (ssInfo->diuc != 0)
            {
                ssInfo->diuc = 0;
            }
            if (ssInfo->uiuc != DOT16_UIUC_MOST_RELIABLE)
            {
                ssInfo->uiuc = DOT16_UIUC_MOST_RELIABLE;
            }
        }

        if (!ssInfo->initRangeCompleted)
        {
            // initial range
            // poll again
            ssInfo->needInitRangeGrant = TRUE;
            if (ssInfo->initRangePolled == TRUE)
            {
                ssInfo->initRangePolled = FALSE;
            }
        }
        else
        {
            // periodic ranging
            //start T27 with active interval
            if (ssInfo->timerT27 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
                ssInfo->timerT27 = NULL;
            }
            if (dot16Bs->rngType != DOT16_CDMA)
            {
                ssInfo->timerT27 = MacDot16SetTimer(
                                           node,
                                           dot16,
                                           DOT16_TIMER_T27,
                                           dot16Bs->para.t27ActiveInterval,
                                           NULL,
                                           ssInfo->basicCid);
                if (DEBUG_PERIODIC_RANGE)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("reset T27 for cid %d with active interval due to "
                           "there is no data in burst\n",
                           ssInfo->basicCid);
                }
            }
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: MacDot16BsMoveToNewDlChannel
// LAYER      :: MAC
// PURPOSE    :: Whether BS request SS to move to another dl channel
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: BOOL          : TRUE or FALSE
// **/
static
BOOL MacDot16BsMoveToNewDlChannel(Node* node,
                                  MacDataDot16* dot16)
{
    return FALSE;
}

// /**
// FUNCTION   :: MacDot16BsHandleCDMARangRequest
// LAYER      :: MAC
// PURPOSE    :: Handle CDMA range request
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Pointer to received rng req msg
// + cdmaInfo  : MacDot16RngCdmaInfo* : Pointer to CDMA info
// RETURN     :: void : NULL
// **/
void MacDot16BsHandleCDMARangRequest(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg,
                                    MacDot16RngCdmaInfo* cdmaInfo)
{
    PhySignalMeasurement* signalMea;
    MacDot16RangeStatus rngStatus;
    Message* pduMsg;
    BOOL needToAdd = TRUE;
    MacDot16CdmaRangingCodeType codetype = DOT16_CDMA_INITIAL_RANGING_CODE;
    char powerDeviation = 0;


    signalMea = (PhySignalMeasurement*) MESSAGE_ReturnInfo(msg);
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    // check signal strength
    if (MacDot16BsSignalStrongEnough(node, dot16, signalMea, powerDeviation)
        == TRUE)
    {
        rngStatus = RANGE_SUCC;
    }
    else
    {
        rngStatus = RANGE_CONTINUE;
    }

    if (isInitialRangingCode(cdmaInfo->rangingCode))
    {
        codetype = DOT16_CDMA_INITIAL_RANGING_CODE;
        dot16Bs->stats.numCdmaRngCodeRcvd++;
    }
    else if (isPeriodicRangingCode(cdmaInfo->rangingCode))
    {
        codetype = DOT16_CDMA_PERIODIC_RANGING_CODE;
        if (rngStatus != RANGE_SUCC)
        {
            needToAdd = FALSE;
        }
        dot16Bs->stats.numCdmaRngCodeRcvd++;
    }
    else if (isBWReqRangingCode(cdmaInfo->rangingCode))
    {
        codetype = DOT16_CDMA_BWREQ_CODE;
        dot16Bs->stats.numCdmaBwRngCodeRcvd++;
    }
    else if (isHandoverRangingCode(cdmaInfo->rangingCode))
    {
        codetype = DOT16_CDMA_HANDOVER_CODE;
    }
    else
    {
        ERROR_ReportError("Received Unknown CDMA ranging code");
    }

    if ((dot16Bs->rngType == DOT16_CDMA ||
        dot16Bs->bwReqType == DOT16_BWReq_CDMA))
    {
        MacDot16RngCdmaInfoList* recCDMARngList = dot16Bs->recCDMARngList;
        MacDot16RngCdmaInfoList* tempRecCDMARngList = NULL;
        tempRecCDMARngList = (MacDot16RngCdmaInfoList*)
            MEM_malloc(sizeof(MacDot16RngCdmaInfoList));
        memset(tempRecCDMARngList, 0, sizeof(MacDot16RngCdmaInfoList));
        tempRecCDMARngList->rngRspSentFlag = FALSE;
        memcpy(&tempRecCDMARngList->cdmaInfo,
            (unsigned char*)cdmaInfo,
            sizeof(MacDot16RngCdmaInfo));
        tempRecCDMARngList->codetype = codetype;
        tempRecCDMARngList->rngStatus = rngStatus;
        tempRecCDMARngList->next = NULL;
        if (recCDMARngList == NULL)
        {
            dot16Bs->recCDMARngList = tempRecCDMARngList;
        }
        else
        {
            // store CDMA information here
            while (recCDMARngList != NULL)
            {
                if ((tempRecCDMARngList->cdmaInfo.rangingCode ==
                    recCDMARngList->cdmaInfo.rangingCode)
                    && (tempRecCDMARngList->cdmaInfo.ofdmaSubchannel ==
                    recCDMARngList->cdmaInfo.ofdmaSubchannel)
                    && (tempRecCDMARngList->cdmaInfo.ofdmaFrame ==
                    recCDMARngList->cdmaInfo.ofdmaFrame))
                {
                    if (tempRecCDMARngList->cdmaInfo.ofdmaSymbol ==
                        recCDMARngList->cdmaInfo.ofdmaSymbol)
                    {
                        // these CDMA packet collide here
                        // also remove the existing packet from the list
                        // delete current recCDMARngList entry
                        MacDot16RngCdmaInfoList* ptr1 =
                            dot16Bs->recCDMARngList;
                        if (ptr1 == recCDMARngList)
                        {
                            // if first node of linked list
                            dot16Bs->recCDMARngList = NULL;
                        }
                        else
                        {
                            while (ptr1->next != recCDMARngList)
                            {
                                ptr1 = ptr1->next;
                            }
                            ptr1->next = recCDMARngList->next;
                        }
                        MEM_free(recCDMARngList);
                        needToAdd = FALSE;
                        break;
                    }

                    if ((tempRecCDMARngList->cdmaInfo.ofdmaSymbol ==
                        (recCDMARngList->cdmaInfo.ofdmaSymbol + 1))
                        || ((tempRecCDMARngList->cdmaInfo.ofdmaSymbol - 1)
                        == recCDMARngList->cdmaInfo.ofdmaSymbol))
                    {
                        // Ignore this CDMA range request
                        needToAdd = FALSE;
                        break;
                    }
                }
                recCDMARngList = recCDMARngList->next;
            }// end of while
            if (needToAdd == TRUE)
            {
                recCDMARngList = dot16Bs->recCDMARngList;
                while (recCDMARngList->next != NULL)
                {
                    recCDMARngList = recCDMARngList->next;
                }
                recCDMARngList->next = tempRecCDMARngList;
            }
            else
            {
                MEM_free(tempRecCDMARngList);
            }
        }
    }
    if (needToAdd == TRUE && (codetype == DOT16_CDMA_INITIAL_RANGING_CODE
        || codetype == DOT16_CDMA_PERIODIC_RANGING_CODE))
    {
        if (DEBUG_CDMA_RANGING)
        {
            if (rngStatus == RANGE_SUCC)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Send Range response PDU with status RANGE_SUCC\n");
            }
            else
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Send Range response PDU with status"
                    " RANGE_CONTINUE\n");
            }
        }
        pduMsg = MacDot16BsBuildRngRspPdu(
                     node,
                     dot16,
                     NULL,
                     DOT16_INITIAL_RANGING_CID,
                     rngStatus,
                     powerDeviation,
                     *cdmaInfo,
                     DOT16_RNG_REQ_InitJoinOrPeriodicRng,   //default
                     TRUE);

        MacDot16BsScheduleMgmtMsgToSs(
                    node,
                    dot16,
                    NULL,
                    DOT16_INITIAL_RANGING_CID,
                    pduMsg);
    }
    return;
}

// /**
// FUNCTION   :: MacDot16BsHandleRangeRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a RNG-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the ranging
// + signalMea : PhySignalMeasurement* : Pointer to the received
//               singal strength
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleRangeRequest(Node* node,
                                 MacDataDot16* dot16,
                                 unsigned char* macFrame,
                                 int startIndex,
                                 Dot16CIDType cid,
                                 PhySignalMeasurement* signalMea)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16RngReqMsg* rngRequest;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int length;
    int i;
    Message* pduMsg;

    unsigned char tlvType;
    unsigned char tlvLength;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH] = {0, 0};
    unsigned char prevServBsId[DOT16_MAC_ADDRESS_LENGTH] = {0, 0};
    Address pgCtrlId;
    BOOL pgCtrlPresent = FALSE;
    MacDot16RngReqPurpose rngReqPurpose; // default is 0
    BOOL dlBurstRequestedBySs = FALSE;
    unsigned char dlBurstRequested = 0;
    MacDot16RngCdmaInfo cdmaInfo;
    memset(&cdmaInfo, 0 ,sizeof(MacDot16RngCdmaInfo));
    // default is init join or periodic
    rngReqPurpose = DOT16_RNG_REQ_InitJoinOrPeriodicRng;

    if (DEBUG_NETWORK_ENTRY)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle a RNG-REQ from cid %d\n", cid);
    }

    // increase statistics
    dot16Bs->stats.numRngReqRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REQ message
    rngRequest = (MacDot16RngReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RngReqMsg);

    // thirdly, TLV encoded information
    length = MacDot16MacHeaderGetLEN(macHeader);
    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        if (tlvType == TLV_RNG_REQ_DlBurst)
        {
            dlBurstRequestedBySs = TRUE;
            dlBurstRequested = macFrame[index];
        }
        else if (tlvType == TLV_RNG_REQ_MacAddr)
        {
            // tlvLengh should be equal to DOT16_MAC_ADDRESS_LENGTH
            for (i = 0; i < DOT16_MAC_ADDRESS_LENGTH; i ++)
            {
                macAddress[i] = macFrame[index + i];
            }
        }
        else if (tlvType == TLV_RNG_REQ_RngPurpose)
        {
            rngReqPurpose = (MacDot16RngReqPurpose) macFrame[index];
        }
        else if (tlvType == TLV_RNG_REQ_ServBsId)
        {
            MacDot16CopyStationId(prevServBsId,
                                  (unsigned char*)&macFrame[index]);
        }
        else if (tlvType == TLV_RNG_REQ_PagingCtrlId)
        {
            pgCtrlPresent = TRUE;
            memcpy(&pgCtrlId, &(macFrame[index]), sizeof(Address));

        }
        else if (tlvType == TLV_RNG_REQ_CDMACode)
        {
            memcpy(&cdmaInfo, (macFrame + index), sizeof(MacDot16RngCdmaInfo));
        }

        index += tlvLength;
    }

    // for periodic range relate RNG-REQ
    if (cid != DOT16_INITIAL_RANGING_CID)
    {
        ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);
        if (ssInfo == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("cannot find SS based on RNG-REQ CID");
            }
            return length;
        }

        if (ssInfo->initRangeCompleted == TRUE)
        {
            // periodic range
            MacDot16BsPerformPeriodicRangeRcvProc(node,
                                                  dot16,
                                                  cid,
                                                  TRUE,
                                                  signalMea,
                                                  cdmaInfo);
            return length;
        }
    }

    if (cid == DOT16_INITIAL_RANGING_CID)
    {
        if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
        {
           printf("    rcvd RNG-REQ for init"
                   "rng with initial rng CID %d\n",
                   cid);
        }

        //  move to next dl channel?
        // this is initial ranging, in attempts to join the network
        // determine whether move to another dl channel
        if (MacDot16BsMoveToNewDlChannel(node, dot16) == TRUE)
        {// yes

            //schedule to send a RNG-RSP to the SS with abort status
            pduMsg = MacDot16BsBuildRngRspPdu(
                         node,
                         dot16,
                         NULL,
                         cid,
                         RANGE_ABORT,
                         0,
                         cdmaInfo,
                         rngReqPurpose);

            MacDot16BsScheduleMgmtMsgToSs(
                        node,
                        dot16,
                        NULL,
                        cid,
                        pduMsg);

            return index - startIndex;

        }
        else
        {
            // do not need to move to another dl channel

            // CID has already assigned to this SS
            ssInfo = MacDot16BsGetSsByMacAddress(node, dot16, macAddress);
            if (ssInfo == NULL)
            {
                // No. Need new CID

                if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
                {
                    printf("    init RNG-REQ with CID %d,"
                           "not found assigned CID to this SS, "
                           " assign a new basic cid to this SS and add "
                           " this SS to future poll init range list\n",
                           cid);
                }
                ssInfo = MacDot16BsAddNewSs(node, dot16, macAddress);
                ERROR_Assert(ssInfo != NULL,
                    "MAC 802.16: Unable to add a new SS");

                ssInfo->lastRngReqPurpose = rngReqPurpose;
                if (rngReqPurpose == DOT16_RNG_REQ_HO)
                {
                    if (pgCtrlPresent == TRUE && ssInfo->isIdle)
                    {
                        memcpy(&ssInfo->pagingController, &pgCtrlId,
                            sizeof(Address));
                        if (DEBUG_IDLE)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("rcvd a rng req for Nw Re-entry \n");
                        }
                        // Send IM_Exit_state_change request to PC
                        ssInfo->isIdle = FALSE;
                        ssInfo->isRegistered = FALSE;
                        MacDot16eBsSendImExitReq(node, dot16, ssInfo);
                    }
                    else    // it is a HO RNG REQ
                    {
                    MacDot16CopyStationId(ssInfo->prevServBsId,
                                          prevServBsId);
                    if (DEBUG_HO)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("rcvd a rng req for HO from BS "
                               "%d:%d:%d:%d:%d:%d\n",
                               ssInfo->prevServBsId[0],
                               ssInfo->prevServBsId[1],
                               ssInfo->prevServBsId[2],
                               ssInfo->prevServBsId[3],
                               ssInfo->prevServBsId[4],
                               ssInfo->prevServBsId[5]);
                    }
                }


                }
                else if (rngReqPurpose == DOT16_RNG_REQ_LocUpd)
                {
                    //Send LU_Req to PC
                    MacDot16eBsSendLUReq(node, dot16, ssInfo);
                }
            }
            else
            {
                // yes. CID assigned alread
                // refer to pg. 180 Fig 62
                // It is the case, basic ICD is assigned but RNG-RSP is lost

                //reset invited ranging retry
                ssInfo->invitedRngRetry = 0;

                //reset ranging correction retry
                ssInfo->rngCorrectRetry = 0;

                ssInfo->lastRngReqPurpose = rngReqPurpose;
                if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
                {
                    printf("    init RNG-REQ with CID = 0, "
                           "but the SS has already assigned basic CID,"
                           "reset init rng retry & correction count\n");
                }
                if (rngReqPurpose == DOT16_RNG_REQ_HO)
                {
                    if (pgCtrlPresent == TRUE && ssInfo->isIdle)
                    {
                        memcpy(&ssInfo->pagingController, &pgCtrlId,
                            sizeof(Address));
                        if (DEBUG_IDLE)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("rcvd a rng req for Nw Re-entry \n");
                        }
                        ssInfo->isIdle = FALSE;
                        ssInfo->isRegistered = FALSE;
                        // Send IM_Exit_state_change request to PC
                        MacDot16eBsSendImExitReq(node, dot16, ssInfo);
                    }
                }
                else if (rngReqPurpose == DOT16_RNG_REQ_LocUpd)
                {
                    //Send LU_Req to PC
                    MacDot16eBsSendLUReq(node, dot16, ssInfo);
                }
            }
        }
    }
    else
    {
        if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
        {
            printf("    rcvd RNG-REQ with CID %d,"
                   "it could be init rng or periodic rng\n",
                   cid);
        }

        // ranging with basic CID, already joined the network
        ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

        ERROR_Assert(ssInfo != NULL,
                     " rcvd a RNG REQ from non init cid,"
                     "but no SS associate with this CID");

        ssInfo->lastRngReqPurpose = rngReqPurpose;
        if (ssInfo->initRangePolled == TRUE)
        {
            // P181 Fig 63, rcvd REQ from basic CID after initial
            // polloing sent

            //clear invited range retry
            ssInfo->invitedRngRetry = 0;

            //increment corretion retries
            ssInfo->rngCorrectRetry ++;
            if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
            {
                printf("    rcvd RNG-REQ with CID %d,"
                       "since BS just invite SS for rng, reset invited rng"
                       " count and increase corerction count\n",
                       cid);
            }

        }

        // DBPC procedure (p180 Fig62)
        // it is used when SS needs to change dl profile
        // but no ul bw is allocated to it

    }

    if (ssInfo != NULL)
    {
        MacDot16RangeStatus rngStatus;
        char powerDeviation = 0;

        // check signal strength
        if (MacDot16BsSignalStrongEnough(node, dot16, signalMea,
                powerDeviation) == TRUE)
        {
            rngStatus = RANGE_SUCC;
        }
        else
        {
            // signal is not enough, need correction
            if (ssInfo->rngCorrectRetry >=
                DOT16_MAX_RANGE_CORRECTION_RETRIES)
            {
                // reaching correction retries
                rngStatus = RANGE_ABORT;
            }
            else
            {
                // strat a new correction

                //ssInfo->txPowerAdjust = 1;
                ssInfo->txPowerAdjust = powerDeviation;
                rngStatus = RANGE_CONTINUE;
            }
        }

        if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
        {
            printf("    rcvd RNG-REQ with CID %d,"
                   "schedule a RNG-RSP and put in the outgoing queue\n",
                    cid);
        }

        // schedule to send a RNG-RSP to the SS
        pduMsg = MacDot16BsBuildRngRspPdu(node,
                                          dot16,
                                          ssInfo,
                                          cid,
                                          rngStatus,
                                          powerDeviation,
                                          cdmaInfo,
                                          rngReqPurpose);

        MacDot16BsScheduleMgmtMsgToSs(node,
                                      dot16,
                                      ssInfo,
                                      cid,
                                      pduMsg);

        // when RANG_SUCC
        if (rngStatus == RANGE_SUCC)
        {

            if (ssInfo->initRangeCompleted == FALSE)
            {
                // Get the least robust uiuc
                unsigned char leastRobustUiuc;

                if (MacDot16GetLeastRobustBurst(node,
                                                dot16,
                                                DOT16_UL,
                                                signalMea->cinr,
                                                &leastRobustUiuc))
                {
                    ssInfo->uiuc = leastRobustUiuc;
                }
                else
                {
                    // assume UIUC 1 always satisfy
                    ssInfo->uiuc = 1;
                }

                if (dlBurstRequestedBySs)
                {
                    ssInfo->diuc = dlBurstRequested;
                }
                else
                {
                    // use the most reliable
                    ssInfo->diuc = 0;
                }

                // remove SS from poll list
                ssInfo->needInitRangeGrant = FALSE;
                if (ssInfo->initRangePolled == TRUE)
                {
                    ssInfo->initRangePolled = FALSE;
                    if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
                    {
                        printf("    rcvd RNG-REQ with CID %d, rng stauts is"
                               "succ, remove SS from poll list\n",
                               cid);
                    }
                }
                ssInfo->initRangeCompleted = TRUE;

                // Start T9 wait for SBC-REQ
                if (ssInfo->timerT9 != NULL)
                {
                    MESSAGE_CancelSelfMsg(node, ssInfo->timerT9);
                    ssInfo->timerT9 = NULL;
                }
                ssInfo->timerT9 = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_T9,
                                          dot16Bs->para.t9Interval,
                                          NULL,
                                          ssInfo->basicCid);

                if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
                {
                    printf("    rcvd RNG-REQ with CID %d,"
                           "rng stauts is succ, init rng complete, start T9"
                           "for basic capability negotiation for basic Cid "
                           " %d reset invited and correction counts\n"
                           "    current DIUC %d UIUC %d \n",
                           cid, ssInfo->basicCid,
                           ssInfo->diuc, ssInfo->uiuc);
                }
            }

            // start T27 with idle interval
            if (ssInfo->timerT27 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT27);
                ssInfo->timerT27 = NULL;
            }
            if (dot16Bs->rngType != DOT16_CDMA)
            {
                ssInfo->timerT27 = MacDot16SetTimer(
                                           node,
                                           dot16,
                                           DOT16_TIMER_T27,
                                           dot16Bs->para.t27IdleInterval,
                                           NULL,
                                           ssInfo->basicCid);
            }
            ssInfo->invitedRngRetry = 0;
            ssInfo->rngCorrectRetry = 0;

            if (DEBUG_PERIODIC_RANGE ||
                DEBUG_INIT_RANGE ||
                DEBUG_NETWORK_ENTRY)
            {
                printf("    rcvd RNG-REQ with CID %d,"
                       "rng stauts is success basic cid %d,"
                       "start first T27 with idle interval,"
                       "reset invited and correction counts\n",
                       cid, ssInfo->basicCid);
            }
        }
        else if (rngStatus == RANGE_CONTINUE)
        {
            // start RNG RSP processing timer before
            // sending out polling raning grant
            if (ssInfo->timerRngRspProc != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerRngRspProc);
                ssInfo->timerRngRspProc = NULL;
            }
            ssInfo->timerRngRspProc = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_RgnRspProc,
                                          dot16Bs->para.rngRspProcInterval,
                                          NULL,
                                          ssInfo->basicCid);

            if (ssInfo->initRangePolled == TRUE)
            {
                ssInfo->initRangePolled = FALSE;
            }
            if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
            {
                printf("    rcvd RNG-REQ with CID %d,"
                       "rng stauts is continue, wait a RngRspPRoceeTime"
                       " to send a new invited rng opp\n"
                       " reset invited and correction counts\n",
                       cid);
            }

        }
        else if (rngStatus == RANGE_ABORT)
        {
            // remove SS from poll list
            ssInfo->needInitRangeGrant = FALSE;
            if (ssInfo->initRangePolled == TRUE)
            {
                ssInfo->initRangePolled = FALSE;
            }

            // deallocate basic and primary CID
            MacDot16BsRemoveSsByCid(
                node,
                dot16,
                ssInfo->basicCid,
                TRUE);

            if (DEBUG_INIT_RANGE || DEBUG_NETWORK_ENTRY)
            {
                printf("    rcvd RNG-REQ with CID %d,"
                       "rng stauts is abort, remove from poll list and"
                       " reset invited and correction counts\n",
                       cid);
            }
        }
    }
    else
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("MAC 802.16: Unknown basic CID");
        }
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsHandleSbcRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a SBC-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the ranging
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleSbcRequest(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16SbcReqMsg* sbcRequest;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int length;

    unsigned char tlvType;
    unsigned char tlvLength;

    Message* pduMsg;

    // increase statistics
    dot16Bs->stats.numSbcReqRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REQ message
    sbcRequest = (MacDot16SbcReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16SbcReqMsg);

    // thirdly, TLV encoded information
    length = MacDot16MacHeaderGetLEN(macHeader);
    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "Unknown CID %d in SBC-REQ, SS may have been timed out\n",
                    cid);
            ERROR_ReportWarning(errString);
        }

        return length;
    }

    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        // handle TLVs
        if (tlvType == TLV_SBC_REQ_RSP_BwAllocSupport)
        {
            ssInfo->ssBasicCapability.bwAllocSupport = macFrame[index];
        }
        else if (tlvType == TLV_SBC_REQ_RSP_TranstionGaps)
        {
            ssInfo->ssBasicCapability.transtionGaps[0] = macFrame[index];
            ssInfo->ssBasicCapability.transtionGaps[1] = macFrame[index + 1];
        }
        else if (tlvType == TLV_SBC_REQ_MaxTransmitPower)
        {
            ssInfo->ssBasicCapability.maxTxPower[0] = macFrame[index];
            ssInfo->ssBasicCapability.maxTxPower[1] = macFrame[index + 1];
            ssInfo->ssBasicCapability.maxTxPower[2] = macFrame[index + 2];
            ssInfo->ssBasicCapability.maxTxPower[3] = macFrame[index + 3];
        }
        else if (tlvType == TLV_COMMON_CurrTransPow)
        {
            ssInfo->ssBasicCapability.currentTxPower = macFrame[index];
        }
        else if (tlvType == TLV_SBC_REQ_RSP_PowerSaveClassTypesCapability)
        {
            ssInfo->ssBasicCapability.supportedPSMOdeClasses =
                macFrame[index];
            for (int i = 0; (macFrame[index] != 0) &&
                (i < DOT16e_MAX_POWER_SAVING_CLASS); i++)
            {
                if (macFrame[index] & 0x01)
                {
                    ssInfo->psClassInfo[i].psClassStatus =
                        POWER_SAVING_CLASS_SUPPORTED;
                        MacDot16eSetPSClassStatus((&ssInfo->psClassInfo[i]),
                            POWER_SAVING_CLASS_STATUS_NONE);
                }
                macFrame[index] = (macFrame[index] >> 1);
            }
        }
        index += tlvLength;
    }

    if (ssInfo->timerT9 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, ssInfo->timerT9);
        ssInfo->timerT9 = NULL;
    }

     // Build SBC-RSP and schedule the transmission to SS
    pduMsg = MacDot16BsBuildSbcRspPdu(node, dot16, ssInfo);
    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->basicCid,
                                  pduMsg);

    // increase stats
    dot16Bs->stats.numSbcRspSent ++;

    // start / reset T17
    if (ssInfo->timerT17 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, ssInfo->timerT17);
        ssInfo->timerT17 = NULL;
    }
    ssInfo->timerT17 = MacDot16SetTimer(node,
                                        dot16,
                                        DOT16_TIMER_T17,
                                        dot16Bs->para.t17Interval,
                                        NULL,
                                        cid);

    if (DEBUG_NETWORK_ENTRY || DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd SBC-REQ, enable capbility,"
               "build and schedule SBC RSP and set T17\n");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsHandlePkmRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a PKM-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the pkm msg
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandlePkmRequest(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16PkmReqMsg* pkmRequest;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int length;
    MacDot16PKMCode pkmCode = PKM_CODE_AuthReply;

    unsigned char tlvType;
    unsigned char tlvLength;

    Message* pduMsg;

    // increase statistics
    dot16Bs->stats.numPkmReqRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REQ message
    pkmRequest = (MacDot16PkmReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16PkmReqMsg);

    // thirdly, TLV encoded information
    length = MacDot16MacHeaderGetLEN(macHeader);
    ssInfo = MacDot16BsGetSsByCid(
                     node,
                     dot16,
                     cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning(
                "Unknown basic CID in PKM-REQ... cannot find ssInfo in list");
        }

        return length;
    }

    ssInfo->ssAuthKeyInfo.pkmId = pkmRequest->pkmId;
    switch (pkmRequest->code)
    {
        case PKM_CODE_AuthRequest:
        {

            pkmCode  = PKM_CODE_AuthReply;

            break;
        }
        case PKM_CODE_KeyRequest:
        {

            pkmCode  = PKM_CODE_KeyReply;

            break;
        }
        case PKM_CODE_AuthInfo:
        {
            break;
        }
        default:
        {
            ERROR_ReportWarning("MAC 802.16: Unknown PKM code");
        }

    }
    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        index += tlvLength;
    }

    // Build PKM-RSP and schedule the transmission to SS

    //build the PKM-RSP
    pduMsg = MacDot16BsBuildPkmRspPdu(node, dot16, ssInfo, pkmCode);

    //schedule the transmission
    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->primaryCid,
                                  pduMsg);

    //increase stats
    dot16Bs->stats.numPkmRspSent ++;

    if (DEBUG_NETWORK_ENTRY || DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd PKM-REQ, process it,"
               "build and schedule PKM RSP\n");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsCalculateHmac
// LAYER      :: MAC
// PURPOSE    :: Handle a HMAC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the HMAC
// + hmacLen   : int           : length of HMAC
// RETURN     :: BOOL          : TRUE if HMAC pass authentication check,
//                               FALSE if not pass authentication check
// **/
static
BOOL MacDot16BsCalculateHmac(Node* node,
                             MacDataDot16* dot16,
                             unsigned char* macFrame,
                             int startIndex,
                             int hmacLen)
{

    return TRUE;
}

// /**
// FUNCTION   :: MacDot16BsHandleRegRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a REG-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the pkm msg
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleRegRequest(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader = NULL;
    MacDot16RegReqMsg* regRequest = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    int index;
    int length;
    int i;
    BOOL hmacValid = TRUE;
    MacDot16RegResponse regResponse;
    Scheduler* scheduler;

    unsigned char tlvType;
    unsigned char tlvLength;

    Message* pduMsg = NULL;

    // increase statistics
    dot16Bs->stats.numRegReqRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the REG-REQ message
    regRequest = (MacDot16RegReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RegReqMsg);

    // thirdly, TLV encoded information
    length = MacDot16MacHeaderGetLEN(macHeader);
    ssInfo = MacDot16BsGetSsByCid(
                     node,
                     dot16,
                     cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("Unknown basic CID with REG-REQ!");
        }

        return length;
    }

     // cancel/stop T17
    if (ssInfo->timerT17 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, ssInfo->timerT17);
        ssInfo->timerT17 = NULL;
    }

    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        if (tlvType == TLV_REG_REQ_RSP_NumUlCidSupport)
        {
            ssInfo->numCidSupport = macFrame[index];
        }
        else if (tlvType == TLV_REG_REQ_RSP_MgmtSupport)
        {
            if (macFrame[index] == 1)
            {
                ssInfo->managed = TRUE;
                ssInfo->secondaryCid =
                    MacDot16BsAllocSecondaryCid(node, dot16, ssInfo);

            }
            else
            {
                ssInfo->managed = FALSE;
            }

        }
        else if (tlvType == TLV_COMMON_HMacTuple)
        {
            // perform HMAC calculation?

            if (dot16Bs->para.authPolicySupport &&
                MacDot16BsCalculateHmac(node,
                                        dot16,
                                        macFrame,
                                        index,
                                        tlvLength))
            {
                hmacValid = TRUE;
            }
            else if (dot16Bs->para.authPolicySupport == FALSE)
            {
                hmacValid = TRUE;
            }
            else
            {
                hmacValid = FALSE;
            }
        }

        // handle more TLVs...
        else if (tlvType == TLV_REG_REQ_RSP_MobilitySupportedParameters)
        {
            ssInfo->mobilitySupportedParameters = macFrame[index];
            if (ssInfo->mobilitySupportedParameters & SLEEP_MODE_SUPPORTED)
            {
                ssInfo->isSleepEnabled = TRUE;
            }
        }

        index += tlvLength;
    }

    if (hmacValid == FALSE)
    {
        regResponse = DOT16_REG_RSP_FAILURE;
    }
    else
    {
        regResponse = DOT16_REG_RSP_OK;
    }

    // Build REG-RSP and schedule the transmission to SS
    // build the REG-RSP
    pduMsg = MacDot16BsBuildRegRspPdu(node, dot16, ssInfo, regResponse);

    //schedule the transmission
    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->primaryCid,
                                  pduMsg);

    // increase stats
    dot16Bs->stats.numRegRspSent ++;

    ssInfo->isRegistered = TRUE;

    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i++)
    {
        sFlow = ssInfo->dlFlowList[i].flowHead;
        if (sFlow)
        {
            scheduler = dot16Bs->outScheduler[sFlow->serviceType];
            scheduler->setQueueBehavior(sFlow->queuePriority, RESUME);
        }
    }

    // begin dot16e support
    if (dot16->dot16eEnabled &&
        ssInfo->lastRngReqPurpose == DOT16_RNG_REQ_HO)
    {
        int bsIndex = 0;

        // send a HO finished msg to old serving BS
        if (MacDot16eGetBsIndexByBsId(node,
                                      dot16,
                                      ssInfo->prevServBsId,
                                      &bsIndex))
        {
            Message* hoFinishedMsg;
            Address destNodeAddress;
            Dot16InterBsHoFinishNotificationMsg* hoFinishedPdu;
            Dot16BackboneMgmtMsgHeader headInfo;

            hoFinishedMsg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node,
                                hoFinishedMsg,
                                sizeof(Dot16InterBsHoFinishNotificationMsg),
                                TRACE_DOT16);
            hoFinishedPdu =
                (Dot16InterBsHoFinishNotificationMsg*)
                MESSAGE_ReturnPacket(hoFinishedMsg);

            // put ms's  mac a ddress in the msg
            memcpy((unsigned char*)(hoFinishedPdu->macAddress),
                    ssInfo->macAddress,
                    DOT16_MAC_ADDRESS_LENGTH);


            headInfo.msgType = DOT16_BACKBONE_MGMT_HO_FINISH_NOTIFICATION;

            destNodeAddress = dot16Bs->nbrBsInfo[bsIndex].bsDefaultAddress;

            Dot16BsSendMgmtMsgToNbrBsOverBackbone(
                node,
                dot16,
                destNodeAddress,
                hoFinishedMsg,
                headInfo,
                0);

            // update stas
            dot16Bs->stats.numInterBsHoFinishNotifySent ++;

            if (DEBUG_HO)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Send a BS finished msg to old BS %d "
                       "for MS w/cid %d  over backbone\n",
                       dot16Bs->nbrBsInfo[bsIndex].bsNodeId,
                       ssInfo->basicCid);
            }
        }
    }
    // end dot16e support

    if (DEBUG_NETWORK_ENTRY || DEBUG_HO)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd REG-REQ, process it, build and schedule REG RSP\n");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsHandleRepRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a REP-RSP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the pkm msg
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleRepRsp(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16RepRspMsg* repRsp;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int length;

    unsigned char tlvType;
    unsigned char tlvLength;
    unsigned char leastRobustDiuc;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle a REP-RSP msg from SSw/cid %d\n", cid);
    }

    // increase statistics
    dot16Bs->stats.numRepRspRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the REP-RSP message
    repRsp = (MacDot16RepRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16RepRspMsg);

    // thirdly, TLV encoded information
    length = MacDot16MacHeaderGetLEN(macHeader);
    ssInfo = MacDot16BsGetSsByCid(
                     node,
                     dot16,
                     cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("Unknown basic CID with REP-RSP!");
        }

        return length;
    }

    if (ssInfo->repReqSent)
    {
        // get a response for the request, reset state for future request
        ssInfo->repReqSent = FALSE;
    }

    while (index < startIndex + length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];

        if (tlvType == TLV_REP_RSP_CinrReport)
        {
            ssInfo->dlCinrMean = macFrame[index] + (-10);

            // skip deviation

            ssInfo->lastDlMeaReportTime = node->getNodeTime();
        }
        else if (tlvType == TLV_REP_RSP_RssiReport)
        {
            ssInfo->dlRssMean = macFrame[index] + (-123);

            // skip deviation

            ssInfo->lastDlMeaReportTime = node->getNodeTime();
        }

        // handle more TLVs...

        index += tlvLength;
    }

    // check if  need to change diuc
    if (MacDot16GetLeastRobustBurst(node,
                                dot16,
                                DOT16_DL,
                                ssInfo->dlCinrMean,
                                &leastRobustDiuc))
    {

        if (ssInfo->diuc != leastRobustDiuc)
        {
            if (DEBUG || DEBUG_NETWORK_ENTRY || DEBUG_PERIODIC_RANGE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("RCVD REP-RSP change the SS with cid %d "
                       "DIUC from %d to %d\n",
                       ssInfo->basicCid,
                       ssInfo->diuc,
                       leastRobustDiuc);

            }
            ssInfo->diuc = leastRobustDiuc;
        }
    }
    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsPerformAdmissionControl
// LAYER      :: MAC
// PURPOSE    :: Determine the service flow can be supported or not
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + bwRequest : double        : Bandwidth requested by the flow
// + direction : int           : service flow direction
// RETURN     :: BOOL : TRUE if can support; FALSE if not
static
BOOL MacDot16BsPerformAdmissionControl(Node* node,
                                       MacDataDot16* dot16,
                                       MacDot16BsSsInfo* recSSInfo,
                                       MacDot16ServiceType serviceType,
                                       MacDot16QoSParameter recQosInfo,
                                       int direction)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    MacDot16ServiceFlow* serviceFlow;
    unsigned int numReservedPs = 0;
    MacDot16BsSsInfo* ssInfo;
    int index, i;
    int avgDataRate = 0;
    clocktype duration;
    unsigned int noOfAvailablePs = 0;
    double controlPacketOverhead;
    BOOL isApproved = TRUE;

    if (serviceType == DOT16_SERVICE_BE || dot16Bs->acuAlgorithm ==
        DOT16_ACU_NONE)
    {
        return isApproved;
    }

    if (direction == DOT16_UPLINK_SERVICE_FLOW)
    {
         // convert the duration to # of physical slots
         // for TDD, total frame duration minus the DL duration
         duration = dot16Bs->para.frameDuration -
             dot16Bs->para.tddDlDuration;
         controlPacketOverhead = (double)1.0
             - dot16Bs->maxAllowedUplinkLoadLevel;
         duration = duration - (duration * (clocktype)controlPacketOverhead);
         noOfAvailablePs = (unsigned int)(duration / dot16->ulPsDuration);
    }
    else
    {
         // convert the duration to # of physical slots
         // for TDD, total frame duration minus the DL duration
         controlPacketOverhead = (double)1.0
             - dot16Bs->maxAllowedDownlinkLoadLevel;
         duration = dot16Bs->para.tddDlDuration;
         duration = duration - (duration * (clocktype)controlPacketOverhead);
         noOfAvailablePs = (unsigned int)(duration / dot16->dlPsDuration);
    }

    if (direction == DOT16_UPLINK_SERVICE_FLOW)
    {
        noOfAvailablePs *= MacDot16PhyGetUplinkNumSubchannels(node, dot16);
    }
    else
    {
        noOfAvailablePs *= MacDot16PhyGetNumSubchannels(node, dot16);
    }
    noOfAvailablePs = (unsigned int)(noOfAvailablePs *
        (double)(SECOND / dot16Bs->para.frameDuration));

    if (serviceType == DOT16_SERVICE_rtPS ||
        serviceType == DOT16_SERVICE_nrtPS )
    {
        // convert the rate to number of PSs
        avgDataRate = (recQosInfo.minReservedRate
            + recQosInfo.maxSustainedRate) / 2;
    }
    else
    {
        avgDataRate = recQosInfo.maxSustainedRate;
    }
    if (direction == DOT16_UPLINK_SERVICE_FLOW)
    {
        numReservedPs = MacDot16PhyBytesToPs(
            node,
            dot16,
            avgDataRate / 8,
            &(dot16Bs->ulBurstProfile[recSSInfo->uiuc - 1]),
            DOT16_UL);
    }
    else
    {
        numReservedPs = MacDot16PhyBytesToPs(
            node,
            dot16,
            avgDataRate / 8,
            &(dot16Bs->dlBurstProfile[recSSInfo->diuc]),
            DOT16_DL);
    }
    if (DEBUG_ADMISSION_CONTROL)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("For SS %d.%d.%d.%d.%d.%d, Bandwidth req. (bytes/sec) %d,\n",
            recSSInfo->macAddress[0],
            recSSInfo->macAddress[1],
            recSSInfo->macAddress[2],
            recSSInfo->macAddress[3],
            recSSInfo->macAddress[4],
            recSSInfo->macAddress[5], avgDataRate / 8);
    }
    if (numReservedPs > noOfAvailablePs)
    {
        isApproved = FALSE;
    }

    // check if we can admit a new flow
    for (index = 0; isApproved && (index < DOT16_BS_SS_HASH_SIZE); index ++)
    {
        ssInfo = dot16Bs->ssHash[index];

        while (isApproved && ssInfo != NULL)
        {
            for (i = 0; isApproved && i < DOT16_NUM_SERVICE_TYPES; i ++)
            {
                if (direction == DOT16_UPLINK_SERVICE_FLOW)
                {
                     serviceFlow = ssInfo->ulFlowList[i].flowHead;

                }
                else
                {
                     serviceFlow = ssInfo->dlFlowList[i].flowHead;
                }

                while (isApproved && serviceFlow != NULL)
                {
                    if ((serviceFlow->serviceType != DOT16_SERVICE_BE)
                        && (serviceFlow->activated) &&
                        (serviceFlow->admitted))
                    {
                        if (serviceFlow->serviceType == DOT16_SERVICE_rtPS ||
                            serviceFlow->serviceType == DOT16_SERVICE_nrtPS )
                        {
                            // convert the rate to number of PSs
                            avgDataRate =
                                (serviceFlow->qosInfo.minReservedRate
                                + serviceFlow->qosInfo.maxSustainedRate) / 2;
                        }
                        else
                        {
                            avgDataRate =
                                serviceFlow->qosInfo.maxSustainedRate;
                        }
                        if (direction == DOT16_UPLINK_SERVICE_FLOW)
                        {
                            numReservedPs += MacDot16PhyBytesToPs(
                                node,
                                dot16,
                                avgDataRate / 8,
                                &(dot16Bs->ulBurstProfile[ssInfo->uiuc - 1]),
                                DOT16_UL);
                        }
                        else
                        {
                            numReservedPs += MacDot16PhyBytesToPs(
                                node,
                                dot16,
                                avgDataRate / 8,
                                &(dot16Bs->dlBurstProfile[ssInfo->diuc]),
                                DOT16_DL);
                        }
                    }
                    if (numReservedPs > noOfAvailablePs)
                    {
                        isApproved = FALSE;
                    }
                    serviceFlow = serviceFlow->next;
                }

             }

             ssInfo = ssInfo->next;
        }
    }

    if (DEBUG_ADMISSION_CONTROL)
    {
        // More impl. needed here
        if (isApproved == FALSE)
        {
            if (direction == DOT16_UPLINK_SERVICE_FLOW)
            {
                printf("\tBS reject uplink service flow request \n");
            }
            else
            {
                printf("\tBS reject downlink service flow request \n");
            }
        }
        else
        {
            if (direction == DOT16_UPLINK_SERVICE_FLOW)
            {
                printf("\tBS accept uplink service flow request \n");
            }
            else
            {
                printf("\tBS accept downlink service flow request \n");
            }
        }
     }
    return isApproved;
}

// /**
// FUNCTION   :: MacDot16BsHandleDsaRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a DSA-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the DSA msg
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDsaRequest(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               Dot16CIDType cid,
                               TraceProtocolType appType)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaReqMsg* dsaReq;
    MacDot16BsSsInfo* ssInfo;

    MacDot16ServiceType serviceType = DOT16_SERVICE_BE;
    MacDot16CsClassifier pktInfo;
    MacDot16QoSParameter qosInfo;
    unsigned int transId;

    MacDot16ServiceFlowDirection sfDirection = DOT16_UPLINK_SERVICE_FLOW;
    MacDot16ServiceFlowInitial sfInitial;
    unsigned char confirmCode;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];

    Dot16CIDType transportCid;
    MacDot16ServiceFlow* sFlow;
    int index;
    int length;

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

    Message* pduMsg;
    BOOL isPackingEnabled = FALSE;
    BOOL isFixedLengthSDU = FALSE;
    BOOL TrafficIndicationPreference = FALSE;
    BOOL isARQEnabled = FALSE ;
    MacDot16ARQParam arqParam ;
    unsigned int arqPar = 0;
    unsigned int fixedLengthSDUSize = DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE;
    memset(&arqParam,0,sizeof(MacDot16ARQParam));

    // increase statistics
    dot16Bs->stats.numDsaReqRcvd ++;

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
        printf("rcvd DSA-REQ for transId %d cid %d,"
               "build and schedule DSA RSP\n",
               transId, cid);
    }

    memset((char*) &qosInfo, 0, sizeof(MacDot16QoSParameter));
    length = MacDot16MacHeaderGetLEN(macHeader);
    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning(
                "Unknown basic CID with DSA REQ... cannot find ssInfo in list");
        }

        return 0;
    }

    MacDot16BsGetServiceFlowByTransId(node, dot16, ssInfo, transId, &sFlow);
    if (sFlow != NULL)
    {
        if (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_REMOTE_DsaAckPending)
        {
            if (sFlow->dsaInfo.dsxRetry > 0)
            {// retry available
                //schedule the transmission of DSA-RSP
                MacDot16BsScheduleMgmtMsgToSs(
                    node,
                    dot16,
                    ssInfo,
                    ssInfo->primaryCid,
                    MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxRspCopy));

            }

            // stay in current status
        }
        else if (sFlow->dsaInfo.dsxTransStatus ==
                 DOT16_FLOW_DSA_REMOTE_DeleteFlow)
        {
            // stay in current status
        }
        return length;
    }

    // This is the first time the REQ is received with spesified transId
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
                                (MacDot16ServiceType)
                                macFrame[sfParamIndex];
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_ToleratedJitter:
                        {
                            unsigned int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                    (unsigned char*)&macFrame[sfParamIndex]);

                            qosInfo.toleratedJitter =
                                (clocktype) (delayJitter) * MILLI_SECOND;

                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_MaxLatency:
                        {
                            unsigned int delayJitter;

                            delayJitter =
                                MacDot16FourByteToInt(
                                    (unsigned char*)&macFrame[sfParamIndex]);

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
                            arqParam.arqWindowSize = arqPar;
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
                            arqParam.arqDeliverInOrder = macFrame[
                                sfParamIndex];
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
                                                        macFrame[csRuleIndex
                                                            + 1]);
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
                                                        macFrame[csRuleIndex
                                                            + 1]);
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
                                                        printf(" DSA-REQ "
                                                            " CSRule: "
                                                            " Unknown"
                                                            " TLV, type"
                                                            "= %d,"
                                                            "length ="
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
                                            printf(" DSA-REQ CSSpec:Unknown"
                                                   "TLV, type = %d, length"
                                                   "= %d\n",
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
                                                case
                                                TLV_CS_PACKET_Protocol:
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
                                                    in6_addr addr;

                                                    memcpy(
                                                        (char*) &addr,
                                                        &(macFrame
                                                        [csRuleIndex]),
                                                        16);
                                                    SetIPv6AddressInfo
                                                        (&(pktInfo.srcAddr),
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
                                                        [csRuleIndex]), 16);
                                                    SetIPv6AddressInfo
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
                                                        macFrame[csRuleIndex
                                                            + 1]);
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
                                                        macFrame[csRuleIndex
                                                            + 1]);
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
                                                        printf(" DSA-REQ "
                                                           "CSRule:Unknown"
                                                           "TLV, type = %d,"
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
                                        if (DEBUG_PACKET_DETAIL ||
                                            DEBUG_FLOW)
                                        {
                                            printf(" DSA-REQ CSSpec:Unknown"
                                                   "TLV, type = %d,"
                                                   "length = %d\n",
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
                                printf(" DSA-REQ flow info:   Unknown TLV,"
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
                    printf(" DSA-REQ:   Unknown TLV, type = %d,"
                           "length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    confirmCode = DOT16_FLOW_DSX_CC_OKSUCC;

    int admitted = TRUE;

    // Check if we can admit new flow
    // we always admit a BE or UGS or ertPS service flow and a
    // nrtPS or rtPS service flow with a Min Reserved Rate of 0
    if ((serviceType != DOT16_SERVICE_BE)  &&
       (qosInfo.minReservedRate > 0 ) )
    {
          admitted =
                MacDot16BsPerformAdmissionControl(
                node,
                dot16,
                ssInfo,
                serviceType,
                qosInfo,
                sfDirection);

         if (!admitted) {
             confirmCode =
                DOT16_FLOW_DSX_CC_RejectExceedDynamicServiceLimit;
         }
    }

    // build DSX-RVD and schedule the transmission to SS
    pduMsg = MacDot16BsBuildDsxRvdPdu(node,
                                      dot16,
                                      ssInfo,
                                      transId,
                                      confirmCode);

    //schedule the transmission
    MacDot16BsScheduleMgmtMsgToSs(
        node,
        dot16,
        ssInfo,
        ssInfo->primaryCid,
        pduMsg);

    // increase stat
    dot16Bs->stats.numDsxRvdSent ++;

     if (confirmCode == DOT16_FLOW_DSX_CC_RejectExceedDynamicServiceLimit)
     {
         //increase stats
         dot16Bs->stats.numDsaReqRej++;
     }

    // create a service flow in SsInfo
    if (sfDirection == DOT16_UPLINK_SERVICE_FLOW)
    {
        memcpy(&macAddress, &ssInfo->macAddress, DOT16_MAC_ADDRESS_LENGTH);
    }
    else
    {
        memcpy(&macAddress, &ssInfo->macAddress, DOT16_MAC_ADDRESS_LENGTH);
    }

    // invalid csfId, no use for remote initiated servic flow

//We need to negotiate the ARQ parameters sent by the SS.
//arqParam variable is already assigned the ARQ parameters of the Service
//Flow received from SS above.
//Negotiating ARQ Enable.
//Only when BS and SS both have ARQ Enabled ,the parameter set as
//TRUE for the service flow.


    if (!(isARQEnabled && dot16Bs->isARQEnabled))
    {
        isARQEnabled = FALSE ;
    }

    if (isARQEnabled)
    {
        if (isPackingEnabled && isFixedLengthSDU)
        {
            isFixedLengthSDU = FALSE;
        }

         //Negotiating ARQ_WINDOW_SIZE.
         //Smaller of the two will be used.
        if (arqParam.arqWindowSize > dot16Bs->arqParam->arqWindowSize)
        {
          arqParam.arqWindowSize = dot16Bs->arqParam->arqWindowSize ;
        }

         //Negotiating ARQ_RETRY_TIMEOUT_TRANSMITTER_DELAY.
         //Greater of the two will be used.

        if (arqParam.arqRetryTimeoutTxDelay <
                                    dot16Bs->arqParam->arqRetryTimeoutTxDelay)
        {
          arqParam.arqRetryTimeoutTxDelay =
                                    dot16Bs->arqParam->arqRetryTimeoutTxDelay ;
        }

          //Negotiating ARQ_RETRY_TIMEOUT_RECEIVER_DELAY.
          //Greater of the two will be used.

        if (arqParam.arqRetryTimeoutRxDelay <
                                    dot16Bs->arqParam->arqRetryTimeoutRxDelay)
        {
          arqParam.arqRetryTimeoutRxDelay =
                                    dot16Bs->arqParam->arqRetryTimeoutRxDelay ;
        }

        //Negotiating ARQ_BLOCK_LIFETIME.
        //The DSA-REQ message shall contain the value of this parameter as
        //defined by the parent service flow.

        //Negotiating ARQ_SYNC_LOSS_TIMEOUT.
        //According to the standard the parameter is decided by the BS.

        arqParam.arqSyncLossTimeout = dot16Bs->arqParam->arqSyncLossTimeout ;

        //Negotiating ARQ_DELIVER_IN_ORDER.
        //According to the standard the value is decided by the SS.

        //Negotiating ARQ_RX_PURGE_TIMEOUT.
        //The DSA-REQ message shall contain the value of this parameter as
        //defined by the parent service flow.

        //Negotiating ARQ_BLOCK_SIZE.
        //According to the standard
        //Absence of the parameter during a DSA dialog indicates
        //the originator of the message desires the maximum value.

        if (!arqParam.arqBlockSize)
        {
          arqParam.arqBlockSize = DOT16_ARQ_MAX_BLOCK_SIZE ;
        }
        //Else Smaller of the two will be used.
        else if (arqParam.arqBlockSize > dot16Bs->arqParam->arqBlockSize)
        {
          arqParam.arqBlockSize = dot16Bs->arqParam->arqBlockSize ;
        }

    }
    if (dot16Bs->isARQEnabled && DEBUG_ARQ_INIT)
    {
        printf("    BS: ARQ parameters after negotiation \n");
            MacDot16PrintARQParameter(dot16Bs->arqParam);
    }

    MacDot16BsAddServiceFlow(node,
                             dot16,
                             macAddress,
                             serviceType,
                             (unsigned int)-1,
                             &qosInfo,
                             &pktInfo,
                             transId,
                             sfDirection,
                             sfInitial,
                             cid,
                             &transportCid,
                             isPackingEnabled,
                             isFixedLengthSDU,
                             fixedLengthSDUSize,
                             appType,
                             isARQEnabled,
                             &arqParam);


    // get the newly added service flow
    MacDot16BsGetServiceFlowByCid(node,
                                  dot16,
                                  transportCid,
                                  &ssInfo,
                                  &sFlow);

    if (TrafficIndicationPreference == TRUE)
    {
        sFlow->TrafficIndicationPreference = TrafficIndicationPreference;
    }
    // set dsxCC
    sFlow->dsaInfo.dsxCC = (MacDot16FlowTransConfirmCode)confirmCode;
    // build the DSA-RSP
    // assume no qos params change has been made
    pduMsg = MacDot16BuildDsaRspMsg(node,
                                    dot16,
                                    sFlow,
                                    transId,
                                    FALSE,
                                    ssInfo->primaryCid);

    // save a copy of DSA-RSP
    sFlow->dsaInfo.dsxRspCopy = MESSAGE_Duplicate(node, pduMsg);

    //schedule the transmission
    MacDot16BsScheduleMgmtMsgToSs(
        node,
        dot16,
        ssInfo,
        ssInfo->primaryCid,
        pduMsg);


    if (DEBUG_FLOW)
    {
        printf("    rcvd DSA-REQ, process it,build and schedule"
               "DSA-RSP, start T8, status REMOTE_DsaAckPending\n");
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16BsHandleDsaRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a DSA-RSP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSA-RSP message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDsaRsp(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaRspMsg* dsaRsp;
    MacDot16ServiceFlow *sFlow;

    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlowDirection sfDirection;
    MacDot16FlowTransConfirmCode confirmCode;

    unsigned int transId;
    unsigned int sfId;
    unsigned int transportCid;
    int index;

    unsigned char tlvType;
    int length;
    int tlvLength;
    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;
    BOOL trafficIndicationPreference = FALSE;

    Message* pduMsg;

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
    dot16Bs->stats.numDsaRspRcvd ++;

    // confirm code
    confirmCode = (MacDot16FlowTransConfirmCode)dsaRsp->confirmCode;

    // transId
    transId = dsaRsp->transId[0] * 256 + dsaRsp->transId[1];

    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Handling DSA-RSP message with TransId %d\n", transId);
    }

    if (DEBUG_PACKET)
    {
        printf("    Now, handling TLVs in the DSX-RSP message\n");
    }

    // Handle all TLVs in the DSA-RSP message
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
                        {
                            // Service Flow Identifier, 4 bytes
                            memcpy((char*) &(sfId),
                                  &(macFrame[sfParamIndex]), 4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_Cid:
                        {
                            // CID assigned to the service flow, 2 bytes
                            MacDot16TwoByteToShort(
                                            transportCid,
                                            macFrame[sfParamIndex],
                                            macFrame[sfParamIndex + 1]);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_TrafficIndicationPreference:
                        {
                            trafficIndicationPreference =
                                macFrame[sfParamIndex];
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
                            arqParam.arqWindowSize = arqPar;
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
                            arqParam.arqDeliverInOrder = macFrame[
                                sfParamIndex];
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
                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf(" DSA-REQ flow info:   Unknown TLV,"
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
                    printf(" DSA-REQ:   Unknown TLV, type = %d,"
                           "length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);
    if (ssInfo != NULL)
    {
        // Get the service flow
        MacDot16BsGetServiceFlowByTransId(node,
                                          dot16,
                                          ssInfo,
                                          transId,
                                          &sFlow);
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("DSA-RSP with unrecognized transId");
            }

            return length;
        }


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
                // enable SF
                sFlow->admitted = TRUE; // assume admitted and activated
                sFlow->activated = TRUE;
                sFlow->isARQEnabled = isARQEnabled;

                // create the queue for holding packets of the service
                // flow and add into the outbound scheduler
                MacDot16SchAddQueueForServiceFlow(node, dot16, sFlow);

                // notify CS ? DSA add succeeded
                if (isARQEnabled)
                {
                    memcpy(sFlow->arqParam, &arqParam, sizeof(
                        MacDot16ARQParam));
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
                sFlow->lastAllocTime = node->getNodeTime();
                if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled && ssInfo
                    && ssInfo->isSleepEnabled)
                {
                    sFlow->TrafficIndicationPreference =
                        DOT16e_TRAFFIC_INDICATION_PREFERENCE;
                    sFlow->psClassType =
                        MacDot16eGetPSClassTypebyServiceType(
                        sFlow->serviceType);
                    ssInfo->psClassInfo[sFlow->psClassType - 1].\
                        numOfServiceFlowExists++;
                }// end of if

                if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled)
                {
                    sFlow->TrafficIndicationPreference =
                        trafficIndicationPreference;
                }

                // set CC to OK
                sFlow->dsaInfo.dsxCC = confirmCode;

                if (DEBUG_PACKET || DEBUG_FLOW)
                {
                    printf("    DSA-RSP message with OK/success code\n");
                }
            }
            else
            {
                // set CC to reject
                sFlow->dsaInfo.dsxCC = confirmCode;
                if (DEBUG_PACKET || DEBUG_FLOW)
                {
                    printf("    DSA-RSP message with rejet code\n");
                }
            }

            // build DSA-ACK
            // schedule transmission
            // make a copy of the DSA-ACK msg
             pduMsg = MacDot16BuildDsaAckMsg(node,
                                             dot16,
                                             sFlow,
                                             ssInfo->primaryCid,
                                             transId);

            // make a copy of the dsa-ack msg for future usage
            sFlow->dsaInfo.dsxAckCopy = MESSAGE_Duplicate(node, pduMsg);

            //schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->primaryCid,
                                          pduMsg);

            // increase stats
            dot16Bs->stats.numDsaAckSent ++;

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
                                          dot16Bs->para.t10Interval,
                                          NULL,
                                          sFlow->cid,
                                          DOT16_FLOW_XACT_Add);

            // Move the status to holding down
            sFlow->dsaInfo.dsxTransStatus =
                        DOT16_FLOW_DSA_LOCAL_HoldingDown;
        }
        else if (sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_HoldingDown)
        {
            // implementing pg 241 figure 108
            //schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(node, sFlow->dsaInfo.dsxAckCopy));

            // increase stats
            dot16Bs->stats.numDsaAckSent ++;
            // stay in the current status

            if (DEBUG_PACKET || DEBUG_FLOW)
            {
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
                printf("    this is duplicated DSA-RSP in DeleteFlow,"
                       "so DSA-ACK message was lost \n");
            }
        }
    }
    else
    {
        // not in the list
    }

    return length;
}
// /**
// FUNCTION   :: MacDot16BsHandleDsaAck
// LAYER      :: MAC
// PURPOSE    :: Handle a DSA-ACK message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSA-ACK message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDsaAck(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DsaAckMsg* dsaAck;
    MacDot16BsSsInfo* ssInfo;
    int index;
    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    int length;

    // increase stat
    dot16Bs->stats.numDsaAckRcvd ++;
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
        printf("handle DSA-ACK with cid as %d, transId %d\n", cid, transId);
    }
    // process TLVs
    length = MacDot16MacHeaderGetLEN(macHeader);

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

    if (ssInfo != NULL)
    {
         // Get the service flow
        MacDot16BsGetServiceFlowByTransId(node,
                                          dot16,
                                          ssInfo,
                                          transId,
                                          &sFlow);
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning(
                    "no service flow associates with the DSA-ACK msg");
            }

            return length;
        }

        if (sFlow->isARQEnabled == TRUE)
        {
            MacDot16ARQCbInit(node, dot16, sFlow);
        }
        sFlow->lastAllocTime = node->getNodeTime();
        if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled && ssInfo
            && ssInfo->isSleepEnabled)
        {
            sFlow->TrafficIndicationPreference =
                DOT16e_TRAFFIC_INDICATION_PREFERENCE;
            sFlow->psClassType =
                MacDot16eGetPSClassTypebyServiceType(sFlow->serviceType);
            ssInfo->psClassInfo[sFlow->psClassType - 1].\
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
            else
            {
                // disable service flow
                // Notify CS ERROR
            }

            // start T8 again
            sFlow->dsaInfo.timerT8 = MacDot16SetTimer(
                                         node,
                                         dot16,
                                         DOT16_TIMER_T8,
                                         dot16Bs->para.t8Interval,
                                         NULL,
                                         sFlow->cid,
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
    }
    else
    {
        // not in the list
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16BsHandleDscReq
// LAYER      :: MAC
// PURPOSE    :: Handle a DSC-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : cid of the DSC msg
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDscReq(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DscReqMsg* dscReq;
    MacDot16BsSsInfo* ssInfo;

    MacDot16QoSParameter qosInfo;
    unsigned int transId;
    MacDot16ServiceFlowDirection sfDirection;
    unsigned char confirmCode;

    unsigned int sfId = 0;
    MacDot16ServiceFlow* sFlow;

    int index;
    int length;
    unsigned char tlvType;
    int tlvLength;

    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    Message* pduMsg;

    // increase statistics
    dot16Bs->stats.numDscReqRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSC-REQ message
    dscReq = (MacDot16DscReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscReqMsg);

    MacDot16TwoByteToShort(transId, dscReq->transId[0], dscReq->transId[1]);

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("rcvd DSC-REQ for transId %d cid %d", transId, cid);
    }

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
                        {
                            // Service Flow Identifier, 4 bytes
                            memcpy((char*)
                                   &(sfId), &(macFrame[sfParamIndex]), 4);
                            sfParamIndex += sfParamTlvLen;

                            break;
                        }
                        case TLV_DSX_QoSParamSetType:
                        {
                            // assume admitted and active QoS set
                            //are the same and 0x6 is used
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

                        default:
                        {
                            // TLVs not supported right now
                            sfParamIndex += sfParamTlvLen;
                            if (DEBUG_PACKET_DETAIL || DEBUG_FLOW)
                            {
                                printf(" DSC-REQ flow info:   Unknown TLV,"
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
                    printf(" DSC-REQ:   Unknown TLV, type = %d,"
                           " length = %d\n", tlvType, tlvLength);
                }

                break;
            }
        } // switch tlvType
    }// while index

    MacDot16BsGetServiceFlowByServiceFlowId(node,
                                            dot16,
                                            sfId,
                                            &ssInfo,
                                            &sFlow);
    if (ssInfo == NULL || sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning(
                "no service flow/ssInfo associated with the DSC REQ with sfId");
        }

        return length;
    }


    if (sFlow->dscInfo.dsxTransStatus ==
        DOT16_FLOW_DSC_REMOTE_DscAckPending)
    {
        if (sFlow->dscInfo.dsxRetry > 0)
        {// retry available
            //schedule the transmission of DSA-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(node, sFlow->dscInfo.dsxRspCopy));

            dot16Bs->stats.numDscReqSent ++;
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
        // Now assume it always can support
        sFlow->status = DOT16_FLOW_ChangingRemote;
        sFlow->numXact ++;
        confirmCode = DOT16_FLOW_DSX_CC_OKSUCC;
        sFlow->dscInfo.dsxTransStatus = DOT16_FLOW_DSC_REMOTE_Begin;
        sFlow->dscInfo.dsxTransId = transId;

        // build DSX-RVD and schedule the transmission to SS
        pduMsg = MacDot16BsBuildDsxRvdPdu(node,
                                          dot16,
                                          ssInfo,
                                          transId,
                                          confirmCode);
        //schedule the transmission
        MacDot16BsScheduleMgmtMsgToSs(node,
                                      dot16,
                                      ssInfo,
                                      ssInfo->primaryCid,
                                      pduMsg);

        // increase stat
        dot16Bs->stats.numDsxRvdSent ++;

        // set dsxCC
        sFlow->dscInfo.dsxCC = (MacDot16FlowTransConfirmCode)confirmCode;

        // save old and new QoS info
        // save the QoS state
        sFlow->dscInfo.dscOldQosInfo =
           (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
        memcpy(sFlow->dscInfo.dscOldQosInfo,
               &(sFlow->qosInfo),
               sizeof(MacDot16QoSParameter));

        sFlow->dscInfo.dscNewQosInfo =
           (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
        memcpy(sFlow->dscInfo.dscNewQosInfo,
               &qosInfo,
               sizeof(MacDot16QoSParameter));

        // change the bandwidth if it is increase BW
        // if new QoS parameters require more bandwidth
        if (qosInfo.minReservedRate > sFlow->qosInfo.minReservedRate)
        {
            // modify policing and scheduling params
            // decrease bandwidth
            memcpy((char*) &(sFlow->qosInfo),
                    &qosInfo,
                    sizeof(MacDot16QoSParameter));
        }

        // build the DSC-RSP
        // assume no qos params change has been made
        pduMsg = MacDot16BuildDscRspMsg(node,
                                        dot16,
                                        sFlow,
                                        transId,
                                        ssInfo->primaryCid);

        // save a copy of DSC-RSP
        sFlow->dscInfo.dsxRspCopy = MESSAGE_Duplicate(node, pduMsg);

        //schedule the transmission
        MacDot16BsScheduleMgmtMsgToSs(node,
                                      dot16,
                                      ssInfo,
                                      ssInfo->primaryCid,
                                      pduMsg);

        //increase stats
        dot16Bs->stats.numDscRspSent ++;

        // start T8
        if (sFlow->dscInfo.timerT8 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT8);
            sFlow->dscInfo.timerT8 = NULL;
        }

        // using transport cid a reference
        sFlow->dscInfo.timerT8 = MacDot16SetTimer(node,
                                                  dot16,
                                                  DOT16_TIMER_T8,
                                                  dot16Bs->para.t8Interval,
                                                  NULL,
                                                  sFlow->cid,
                                                  DOT16_FLOW_XACT_Change);

        // set dsx retry
        sFlow->dscInfo.dsxRetry --;

        // move the dsx status
        sFlow->dscInfo.dsxTransStatus = DOT16_FLOW_DSC_REMOTE_DscAckPending;

        if (DEBUG_FLOW)
        {
            printf("    rcvd DSC-REQ, process it,build and schedule"
                   "DSC-RSP, start T8, status REMOTE_DscAckPending\n");
        }
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16BsHandleDscRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a DSC-RSP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSC-RSP message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDscRsp(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DscRspMsg* dscRsp;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlowDirection sfDirection;

    int index;
    unsigned char tlvType;
    int length;
    int tlvLength;

    int sfParamIndex;
    unsigned char sfParamTlvType;
    unsigned char sfParamTlvLen;

    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    Message* pduMsg;

    // increase stat
    dot16Bs->stats.numDscRspRcvd ++;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSC-RSP message
    dscRsp = (MacDot16DscRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DscRspMsg);

    MacDot16TwoByteToShort(transId, dscRsp->transId[0], dscRsp->transId[1]);
    confirmCode = (MacDot16FlowTransConfirmCode)dscRsp->confirmCode;

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSC-RSP with cid as %d\n", cid);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

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
                    sfParamIndex += sfParamTlvLen;

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
                    printf("  DSC-RSP: Unknown TLV, type = %d,"
                           " length = %d\n", tlvType, tlvLength);

                }

                break;
            }
        } // switch tlvType
    }// while index


    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 cid);

    if (ssInfo != NULL)
    {
         // Get the service flow
        MacDot16BsGetServiceFlowByTransId(node,
                                          dot16,
                                          ssInfo,
                                          transId,
                                          &sFlow);
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("no sflow associates with the DSC-RSP msg");
            }

            return length;
        }

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
                memcpy((char*) &(sFlow->qosInfo),
                        sFlow->dscInfo.dscNewQosInfo,
                        sizeof(MacDot16QoSParameter));

                // notify CS DSC succeeded

                // set CC to OK
                sFlow->dscInfo.dsxCC = confirmCode;
                if (DEBUG_PACKET || DEBUG_FLOW)
                {
                    printf("    DSC-RSP message with OK/success code,"
                           "change policy and schedule if needed\n");
                }
            }
            else
            {
                // set CC to reject
                sFlow->dscInfo.dsxCC = confirmCode;
                if (DEBUG_PACKET || DEBUG_FLOW)
                {
                    printf("    DSC-RSP message with reject code,"
                           "restore old QoS if needed\n");
                }
            }

            // build DSC-ACK
            // schedule transmission
            // make a copy of the DSC-ACK msg
            pduMsg = MacDot16BuildDscAckMsg(node,
                                            dot16,
                                            sFlow,
                                            ssInfo->primaryCid,
                                            transId);

            // make a copy of the dsc-ack msg for future usage
            sFlow->dscInfo.dsxAckCopy = MESSAGE_Duplicate(node, pduMsg);

            //schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->primaryCid,
                                          pduMsg);

            //increase stats
            dot16Bs->stats.numDscAckSent ++;

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
                                          dot16Bs->para.t10Interval,
                                          NULL,
                                          sFlow->cid,
                                          DOT16_FLOW_XACT_Change);

            // Move the status to holding down
            sFlow->dscInfo.dsxTransStatus =
                            DOT16_FLOW_DSC_LOCAL_HoldingDown;
        }
        else if (sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_LOCAL_HoldingDown)
        {
            // implementing pg 252 figure 117
            // make a copy of the dsc-ack msg  and schedule the transmission
            //schedule the transmission
            pduMsg = MESSAGE_Duplicate(node, sFlow->dscInfo.dsxAckCopy);
            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->primaryCid,
                                          MESSAGE_Duplicate(node, pduMsg));

            //increase stats
            dot16Bs->stats.numDscAckSent ++;

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

        if (DEBUG_PACKET || DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("handled DSC-RSP message for transId %d\n", transId);
        }
    }
    else
    {
        // not in the list
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16BsHandleDscAck
// LAYER      :: MAC
// PURPOSE    :: Handle a DSC-ACK message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSC-ACK message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDscAck(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DscAckMsg* dscAck;
    MacDot16BsSsInfo* ssInfo;
    int index;
    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    int length;

    // increase stat
    dot16Bs->stats.numDscAckRcvd ++;
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
        printf("handle DSC-ACK with cid as %d\n", cid);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 cid);

    if (ssInfo != NULL)
    {
         // Get the service flow
        MacDot16BsGetServiceFlowByTransId(node,
                                          dot16,
                                          ssInfo,
                                          transId,
                                          &sFlow);
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
                // decrease BW if required
                if (sFlow->dscInfo.dscNewQosInfo->minReservedRate <
                    sFlow->dscInfo.dscOldQosInfo->minReservedRate)
                {
                    // decrease BW
                    memcpy((char*) &(sFlow->qosInfo),
                            sFlow->dscInfo.dscNewQosInfo,
                            sizeof(MacDot16QoSParameter));
                }
            }
            else
            {
                // only increae BW case, the qosInfo has been changed
                if (sFlow->dscInfo.dscNewQosInfo->minReservedRate >
                    sFlow->dscInfo.dscOldQosInfo->minReservedRate)
                {
                    //restore old QoS info
                    memcpy((char*) &(sFlow->qosInfo),
                            sFlow->dscInfo.dscOldQosInfo,
                            sizeof(MacDot16QoSParameter));
                }

            }

            // start T8 again
            sFlow->dscInfo.timerT8 = MacDot16SetTimer(
                                         node,
                                         dot16,
                                         DOT16_TIMER_T8,
                                         dot16Bs->para.t8Interval,
                                         NULL,
                                         sFlow->cid,
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
    }
    else
    {
        // not in the list
    }

    return length;
}

// /**
// FUNCTION   :: MacDot16BsHandleDsdReq
// LAYER      :: MAC
// PURPOSE    :: Handle a DSD-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSD-REQ message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDsdReq(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DsdReqMsg* dsdReq;
    MacDot16BsSsInfo* ssInfo;

    int index;
    unsigned int transId;
    MacDot16FlowTransConfirmCode confirmCode;
    MacDot16ServiceFlow* sFlow;
    unsigned int sfId;

    int length;

    // increase stat
    dot16Bs->stats.numDsdReqRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSD REQ message
    dsdReq = (MacDot16DsdReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdReqMsg);

    MacDot16TwoByteToShort(transId, dsdReq->transId[0], dsdReq->transId[1]);
    sfId = MacDot16FourByteToInt(dsdReq->sfid);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSD-REQ with cid as %d\n", cid);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 cid);

    if (ssInfo != NULL)
    {
        // Get the service flow
        MacDot16BsGetServiceFlowByServiceFlowId(node,
                                                dot16,
                                                sfId,
                                                &ssInfo,

                                                &sFlow);
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("no sflow associates with the DSD-REQ msg");
            }

            return length;
        }

        if (sFlow->dsdInfo.dsxTransStatus == DOT16_FLOW_DSX_NULL)
        {
            Message* pduMsg;

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

            // build DSD_RSP
            // make a copy of the DSA-ACK msg
            pduMsg = MacDot16BuildDsdRspMsg(node,
                                            dot16,
                                            sFlow,
                                            ssInfo->primaryCid,
                                            transId);

            // make a copy of the dsd-rsp msg for future usage
            sFlow->dsdInfo.dsxRspCopy = MESSAGE_Duplicate(node, pduMsg);

            //schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->primaryCid,
                                          pduMsg);

            // increase stats
            dot16Bs->stats.numDsdRspSent ++;

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
                                          dot16Bs->para.t10Interval,
                                          NULL,
                                          sFlow->cid,
                                          DOT16_FLOW_XACT_Delete);

            // Move the status to holding down
            sFlow->dsdInfo.dsxTransStatus =
                        DOT16_FLOW_DSD_REMOTE_HoldingDown;
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                printf("    rcvd a DSD-REQ fpr transId %d,"
                       "Sent DSD-RSp and start T10, Move to HoldingDown\n",
                       transId);
            }
        }
        else if (sFlow->dsdInfo.dsxTransStatus ==
                 DOT16_FLOW_DSD_REMOTE_HoldingDown)
        {
            // send the copy again
            //schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(node, sFlow->dsdInfo.dsxRspCopy));

            // increase stats
            dot16Bs->stats.numDsdRspSent ++;

            // stay in current status
            if (DEBUG_PACKET || DEBUG_FLOW)
            {
                printf("    this is a duplicated DSD-REQ in HoldingDown,"
                       "so DSD-RSP message was lost \n");
            }
        }
    }
    else
    {
        // not in the list
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no sflow associates with the DSD-REQ msg");
        }
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
// FUNCTION   :: MacDot16BsHandleDsdRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a DSD-RSP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// + cid       : Dot16CIDType  : Cid of the DSD-RSP message
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleDsdRsp(Node* node,
                           MacDataDot16* dot16,
                           unsigned char* macFrame,
                           int startIndex,
                           Dot16CIDType cid)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16DsdRspMsg* dsdRsp;
    MacDot16BsSsInfo* ssInfo;

    int index;
    unsigned int transId;
    MacDot16ServiceFlow* sFlow;
    unsigned int sfId;
    int length;

    // increase stat
    dot16Bs->stats.numDsdRspRcvd ++;
    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DSD REQ message
    dsdRsp = (MacDot16DsdRspMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DsdRspMsg);

    MacDot16TwoByteToShort(transId, dsdRsp->transId[0], dsdRsp->transId[1]);
    sfId = MacDot16FourByteToInt(dsdRsp->sfid);

    if (DEBUG_PACKET || DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle DSD-RSP with cid as %d\n", cid);
    }

    length = MacDot16MacHeaderGetLEN(macHeader);

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 cid);

    if (ssInfo != NULL)
    {
        // Get the service flow
        MacDot16BsGetServiceFlowByServiceFlowId(node,
                                                dot16,
                                                sfId,
                                                &ssInfo,
                                                &sFlow);
        if (sFlow == NULL)
        {
            if (PRINT_WARNING)
            {
                ERROR_ReportWarning("no sflow associates with the DSD-RSP msg");
            }

            return length;
        }

        if (sFlow->dsdInfo.dsxTransStatus ==
                DOT16_FLOW_DSD_LOCAL_DsdRspPending)
        {
            if (DEBUG_FLOW)
            {
                printf("    rcvd DSD-RSP, stop T7, start T10 move status"
                       "to holding for sflow with transId %d sfId %d\n",
                       transId, sfId);
            }

            // stop T7
            if (sFlow->dsdInfo.timerT7 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT7);
                sFlow->dsdInfo.timerT7 = NULL;
            }

            // start T10
            if (sFlow->dsdInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT10);
                sFlow->dsdInfo.timerT10 = NULL;
            }
            sFlow->dsdInfo.timerT10 = MacDot16SetTimer(
                                          node,
                                          dot16,
                                          DOT16_TIMER_T10,
                                          dot16Bs->para.t10Interval,
                                          NULL,
                                          sFlow->cid,
                                          DOT16_FLOW_XACT_Delete);

            // move status to holding down
            sFlow->dsdInfo.dsxTransStatus =
                DOT16_FLOW_DSD_LOCAL_HoldingDown;
        }

    }
    else
    {
        // not in the list
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no ssInfo associates with the DSD-RSP msg");
        }
        if (DEBUG_FLOW)
        {
            printf("    rcvd DSD-RSP, but cannot "
                   "find the ssInfo service with transId %d sfId %d\n",
                   transId, sfId);
        }
    }

    return length;
}
// /**
// FUNCTION   :: MacDot16BsHandleBandwidthRequest
// LAYER      :: MAC
// PURPOSE    :: Handle a bandwidth request
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16BsHandleBandwidthRequest(Node* node,
                                     MacDataDot16* dot16,
                                     unsigned char* macFrame,
                                     int startIndex)
{
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* serviceFlow = NULL;
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    int bandwidth;
    unsigned char bwType;
    BOOL isMgmtCid;

    macHeader = (MacDot16MacHeader*) &(macFrame[startIndex]);

    cid = MacDot16MacHeaderGetCID(macHeader);
    bwType = MacDot16MacHeaderGetBandwidthType(macHeader);
    bandwidth = MacDot16MacHeaderGetBR(macHeader);

    if (DEBUG_PACKET || DEBUG_BWREQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle BW req for cid %d, bw = %d\n", cid, bandwidth);
    }

    isMgmtCid = MacDot16IsManagementCid(node, dot16, cid);

    if (isMgmtCid)
    {
        ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);
    }
    else
    {
        MacDot16BsGetServiceFlowByCid(node,
                                      dot16,
                                      cid,
                                      &ssInfo,
                                      &serviceFlow);
    }

    if (ssInfo != NULL)
    {
        if (bwType == DOT16_BANDWIDTH_AGG)
        {
            if (isMgmtCid)
            {
                if (cid == ssInfo->basicCid)
                {
                    ssInfo->basicMgmtBwRequested = bandwidth;
                    if (DEBUG_BWREQ)
                    {
                        printf("    Handle BW REQ for basicMgmt %d for SS"
                               "with basicCid %d\n",
                               ssInfo->basicMgmtBwRequested,
                               ssInfo->basicCid);
                    }
                }
                else if (cid == ssInfo->primaryCid)
                {
                    ssInfo->priMgmtBwRequested = bandwidth;
                    if (DEBUG_BWREQ)
                    {
                        printf("    Handle BW REQ for priMgmt %d for SS"
                               "with basicCid %d\n",
                               ssInfo->priMgmtBwRequested,
                               ssInfo->basicCid);
                    }
                }
                else
                {
                    ssInfo->secMgmtBwRequested = bandwidth;
                }
            }
            else
            {
                serviceFlow->bwRequested = bandwidth;

                // for ertPS
                if (serviceFlow->serviceType == DOT16_SERVICE_ertPS)
                {
                    serviceFlow->lastBwRequested = serviceFlow->bwRequested;
                }
                if (DEBUG_BWREQ)
                {
                    printf("    Handle BW REQ for for transCid %d SS with"
                            "basicCid %d\n",
                            serviceFlow->cid, ssInfo->basicCid);
                }
            }
        }
        else if (bwType == DOT16_BANDWIDTH_INC)
        {
            if (isMgmtCid)
            {
                if (cid == ssInfo->basicCid)
                {
                    ssInfo->basicMgmtBwRequested =
                        ssInfo->lastBasicMgmtBwRequested + bandwidth;
                }
                else if (cid == ssInfo->primaryCid)
                {
                    ssInfo->priMgmtBwRequested =
                        ssInfo->lastPriMgmtBwRequested + bandwidth;
                }
                else
                {
                    ssInfo->secMgmtBwRequested =
                        ssInfo->lastSecMgmtBwRequested + bandwidth;
                }
            }
            else
            {
                serviceFlow->bwRequested =
                    serviceFlow->lastBwRequested + bandwidth;

                // for ertPS
                if (serviceFlow->serviceType == DOT16_SERVICE_ertPS)
                {
                    serviceFlow->lastBwRequested = serviceFlow->bwRequested;
                }
            }
        }
        else
        {
            ERROR_ReportError("    MAC 802.16: Unknown allocation type of "
                              "bandwidth request!");
        }
    }

    return sizeof(MacDot16MacHeader);
}
//--------------------------------------------------------------------------
// Handle dot16e msgs (start)
//--------------------------------------------------------------------------
#if 0
// /**
// FUNCTION   :: MacDot16eSuspendFlowQueue
// LAYER      :: MAC
// PURPOSE    :: Suspend serive flow queue in case of scan
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + dot16     : MacDataDot16*     : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS's info structure
// + isEnable  : BOOL              : TRUE if to suspend;
//                                   FLASE if to revoke suspension
// RETURN     :: void          : NULL
static
void MacDot16eSuspendFlowQueue(Node *node,
                               MacDataDot16* dot16,
                               MacDot16BsSsInfo* ssInfo,
                               BOOL toSuspend)
{
    int i;
    MacDot16ServiceFlow* sFlow;

    // suspend service flow queue
    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        sFlow = ssInfo->dlFlowList[i].flowHead;

        while (sFlow != NULL)
        {
            sFlow = sFlow->next;
        } // while sFlow
    } // for i
}
#endif

// /**
// FUNCTION   :: MacDot16eBsUpdateSsScanInfo
// LAYER      :: MAC
// PURPOSE    :: Update SS's scan state
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// RETURN     :: void          : NULL
static
void MacDot16eBsUpdateSsScanInfo(Node *node,
                                 MacDataDot16* dot16)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    int i;
    MacDot16BsSsInfo* ssInfo = NULL;

    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        // check all SSs with the same hash ID
        while (ssInfo != NULL)
        {
            if (ssInfo->inNbrScan)
            {
                if (ssInfo->numScanFramesLeft)
                {// if in this coming frame, ss is still in scan mode

                    ssInfo->numScanFramesLeft --;

                }
                else
                {// now, state should change to interleaved normal operation
                    // back to normal operaiton
                    ssInfo->inNbrScan = FALSE;

                    // reduce the iteration
                    ssInfo->numScanIterationsLeft --;

                    // set interFrame num
                    ssInfo->numInterFramesLeft = ssInfo->scanInterval;

                    // reduce one for this frame
                    ssInfo->numInterFramesLeft --;
                    if (DEBUG_NBR_SCAN)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("SS with basic CID %d change to interleaved"
                               "normal op\n", ssInfo->basicCid);
                    }
                }
            }
            else
            {

                if (ssInfo->numInterFramesLeft)
                {// if it is in interleaved normal operation
                    ssInfo->numInterFramesLeft --;
                }
                else
                {// if interleaved is end or it is always in normal
                    if (ssInfo->numScanIterationsLeft)
                    {// if need more iteration of scan

                        ssInfo->numScanFramesLeft = ssInfo->scanDuration;
                        ssInfo->inNbrScan = TRUE;
                        ssInfo->numScanFramesLeft --;

                        if (DEBUG_NBR_SCAN)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS with basic CID %d begin new"
                                 "scanning iteration\n", ssInfo->basicCid);
                        }
                    }
                }
            }
            ssInfo = ssInfo->next;
        }
    }
}

// /**
// FUNCTION   :: MacDot16eBsHandleHelloMsg
// LAYER      :: MAC
// PURPOSE    :: Handle a HELLO message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + Message*  : msg           : hello msg from neighboring BS
// + msgSrcNodeId : NodeAddress  : BS node address of src of the hello msg
// RETURN     :: void          : NULL
// **/
void MacDot16eBsHandleHelloMsg(Node* node,
                               MacDataDot16* dot16,
                               Message* msg,
                               NodeAddress msgSrcNodeId)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    unsigned char* macFrame;
    MacDot16MacHeader* macHeader;
    MacDot16DcdMsg* dcd;
    int index;
    int length;
    unsigned char tlvType;
    unsigned char tlvLength;
    int bsIndex;

    index = 0;
    macFrame = (unsigned char*) MESSAGE_ReturnPacket(msg);
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);
    dcd = (MacDot16DcdMsg*) &(macFrame[index]);
    index += sizeof(MacDot16DcdMsg);
    length = MacDot16MacHeaderGetLEN(macHeader);

    if (!dot16->dot16eEnabled)
    {
        return;
    }

    // update stats
    dot16Bs->stats.numInterBsHelloRcvd ++;

    // get the bs Idnex
    if (!MacDot16eGetBsIndexByBsNodeId(node,
                                  dot16,
                                  msgSrcNodeId,
                                  &bsIndex))
    {
        // if not found or too many BNR BS already
        return;
    }

    // after get the information for the BS, activate the NBR BS info
    if (!dot16Bs->nbrBsInfo[bsIndex].isActive)
    {
        dot16Bs->nbrBsInfo[bsIndex].isActive = TRUE;
    }
    else // already an active NBR BS
    {
        if (dot16Bs->nbrBsInfo[bsIndex].dcdChangeCount ==
            dcd->changeCount)
        {
            // the same dcd
            return;
        }
    }

    // update change count
    dot16Bs->nbrBsInfo[bsIndex].dcdChangeCount = dcd->changeCount;
    dot16Bs->nbrBsInfo[bsIndex].dlChannel = dcd->dlChannelId;


    dot16Bs->configChangeCount ++;
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        while (index < length)
        {
            tlvType = macFrame[index ++];
            tlvLength = macFrame[index ++];

            switch (tlvType)
            {
                // common channel encoding
                case TLV_DCD_BsEirp:
                {
                    MacDot16TwoByteToShort(
                        dot16Bs->nbrBsInfo[bsIndex].bs_EIRP,
                        macFrame[index],
                        macFrame[index + 1]);
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_RrsInitRngMax:
                {
                    MacDot16TwoByteToShort(
                        dot16Bs->nbrBsInfo[bsIndex].rssIRmax,
                        macFrame[index],
                        macFrame[index + 1]);
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
                    dot16Bs->nbrBsInfo[bsIndex].ttg = 
                        PhyDot16ConvertTTGRTGinclocktype( 
                                            node,
                                            dot16->myMacData->phyNumber,
                                            macFrame[index ++]);

                    break;
                }
                case TLV_DCD_RTG:
                {
                    dot16Bs->nbrBsInfo[bsIndex].rtg = 
                        PhyDot16ConvertTTGRTGinclocktype( 
                                            node,
                                            dot16->myMacData->phyNumber,
                                            macFrame[index ++]);
                    break;
                }
                case TLV_DCD_BsId:
                {
                    memcpy((char*)dot16Bs->nbrBsInfo[bsIndex].nbrBsId,
                           (char*) &macFrame[index],
                           6);
                    index += tlvLength;

                    break;
                }
                case TLV_DCD_Trigger:
                {
                    int triggerIndex;
                    unsigned char triggerTlvType;
                    unsigned char triggerTlvLength;

                    triggerIndex = index;
                    while (triggerIndex < index + tlvLength)
                    {
                        triggerTlvType = macFrame[triggerIndex ++];
                        triggerTlvLength = macFrame[triggerIndex ++];
                        switch (triggerTlvType)
                        {
                            case TLV_DCD_TriggerTypeFuncAction:
                            {
                                dot16Bs->nbrBsInfo[bsIndex].
                                    trigger.triggerAction =
                                    (macFrame[triggerIndex] & 0x07);

                                dot16Bs->nbrBsInfo[bsIndex].
                                    trigger.triggerFunc =
                                    (macFrame[triggerIndex] & 0x38) >> 3;

                                dot16Bs->nbrBsInfo[bsIndex].
                                    trigger.triggerType =
                                    (macFrame[triggerIndex] & 0xc0) >> 6;

                                triggerIndex ++; // one byte in length

                                break;
                            }
                            case TLV_DCD_TriggerValue:
                            {
                                dot16Bs->nbrBsInfo[bsIndex].
                                    trigger.triggerValue =
                                        macFrame[triggerIndex ++];

                                break;
                            }
                            case TLV_DCD_TriggerAvgDuration:
                            {
                                dot16Bs->nbrBsInfo[bsIndex].
                                   trigger.triggerAvgDuration =
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
                                         triggerTlvType, triggerTlvLength);
                                }

                                break;
                            }

                        }

                    }

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
                    burstProfile =
                        (MacDot16DlBurstProfile*)
                        &(dot16Bs->nbrBsInfo[bsIndex].dlBurstProfile[diuc]);

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

                                if (DEBUG_PACKET_DETAIL)
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

                    if (DEBUG_PACKET_DETAIL)
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

    if (DEBUG_HO)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Handle hello msg from %d\n", msgSrcNodeId);
    }
}

// /**
// FUNCTION   :: MacDot16eBsHandleHoFinishedMsg
// LAYER      :: MAC
// PURPOSE    :: Handle a HELLO message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + Message*  : msg           : hello msg from neighboring BS
// RETURN     :: void          : NULL
// **/
void MacDot16eBsHandleHoFinishedMsg(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16InterBsHoFinishNotificationMsg *hoFinishedPdu;
    MacDot16BsSsInfo* ssInfo = NULL;

    hoFinishedPdu = (Dot16InterBsHoFinishNotificationMsg*)
                                MESSAGE_ReturnPacket(msg);

    ssInfo = MacDot16BsGetSsByMacAddress(node,
                                         dot16,
                                         hoFinishedPdu->macAddress);

    // update stats
    dot16Bs->stats.numInterBsHoFinishNotifyRcvd ++;
    if (ssInfo)
    {
        if (DEBUG_HO)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("rcvd a HO finished msg for node w/Cid %d, "
                   "remove from management\n",
                   ssInfo->basicCid);
        }

        // cancel resrc retain Timer
        if (ssInfo->resrcRetainTimer != NULL)
        {
            MESSAGE_CancelSelfMsg(node, ssInfo->resrcRetainTimer);
            ssInfo->resrcRetainTimer = NULL;
        }

        MacDot16BsRemoveSsByCid(node, dot16, ssInfo->basicCid, TRUE);
    }

}

// /**
// FUNCTION   :: MacDot16eBsHandleMobScnReq
// LAYER      :: MAC
// PURPOSE    :: Handle a MOB-SCN-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleMobScnReq(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobScnReqMsg* scanReq;
    MacDot16BsSsInfo* ssInfo;
    int index;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REQ message
    scanReq = (MacDot16eMobScnReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobScnReqMsg);

    if (DEBUG_PACKET || DEBUG_NBR_SCAN)
    {
        printf("Node%d handle MOB-SCN-REQ with cid as %d\n",
               node->nodeId, MacDot16MacHeaderGetCID(macHeader));
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update stat
    dot16Bs->stats.numNbrScanReqRcvd ++;

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 MacDot16MacHeaderGetCID(macHeader));

    if (ssInfo != NULL)
    {
        if (((scanReq->duration == 0) &&
            (ssInfo->numInterFramesLeft > 0)) ||
            ((ssInfo->numScanIterationsLeft > 0) &&
             (ssInfo->numInterFramesLeft == 0)))
        {
            // receive 0 duration SCN-REQ while in interleave
            // 1. still in interleave or
            // 2. finish interleave and need a new round scan interval
            ssInfo->inNbrScan = FALSE;
            ssInfo->scanDuration = 0;
            ssInfo->scanInterval = 0;
            ssInfo->scanIteration = 0;
            ssInfo->numInterFramesLeft = 0;
            ssInfo->numScanFramesLeft = 0;
            ssInfo->numScanIterationsLeft = 0;
        }
        Message* pduMsg;

        // schedule to send a MOB-SCN-RSP to the SS
        pduMsg = MacDot16eBsBuildMobScnRspPdu(node, dot16, ssInfo, scanReq);

        MacDot16BsScheduleMgmtMsgToSs(node,
                                      dot16,
                                      ssInfo,
                                      ssInfo->basicCid,
                                      pduMsg);

        //update stats
        dot16Bs->stats.numNbrScanRspSent ++;
    }
    else
    {
        // not in the list
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16BsUpdateSsNbrSigMeanMeas
// LAYER      :: MAC
// PURPOSE    :: update the signal meas for the nbr BS of a SS
//               The NBR BS may not in the list of BS's own nrb list
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + ssInfo    : MacDot16BsSsInfo* : Pointer to the ssInfo
// + reportMeteric : unsigned char : reprot metric regardign the signal
// + bsId      : unsigned char* : NBR Bs Id
// + meanMeas  : MacDot16SignalMeasurementInfo : measurement
// RETURN     :: void : NULL
// **/
static
void MacDot16BsUpdateSsNbrSigMeanMeas(
                                Node* node,
                                MacDataDot16* dot16,
                                MacDot16BsSsInfo* ssInfo,
                                unsigned char reportMetric,
                                unsigned char* bsId,
                                MacDot16SignalMeasurementInfo meanMeas)
{
    int i;
    BOOL found = FALSE;

    // find the one with the same bsId
    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i ++)
    {
        if (ssInfo->nbrBsSignalMeas[i].bsValid &&
            (memcmp(ssInfo->nbrBsSignalMeas[i].bsId,
                    bsId,
                    DOT16_BASE_STATION_ID_LENGTH) == 0))
        {
            found = TRUE;

            break;
        }
    }
    if (!found)
    {
        // find a non used space
        for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i ++)
        {
            if (!ssInfo->nbrBsSignalMeas[i].bsValid)
            {
                found = TRUE;
                memcpy(ssInfo->nbrBsSignalMeas[i].bsId,
                       bsId,
                       DOT16_BASE_STATION_ID_LENGTH);

                ssInfo->nbrBsSignalMeas[i].bsValid = TRUE;
                ssInfo->nbrBsSignalMeas[i].dlMeanMeas.isActive = TRUE;

                break;
            }
        }
    }

    // reset the measurement
    if (found)
    {
        if (reportMetric & DOT16e_MSHO_ReportMetricCINR)
        {
            ssInfo->nbrBsSignalMeas[i].dlMeanMeas.cinr = meanMeas.cinr;
        }
        if (reportMetric & DOT16e_MSHO_ReportMetricRSSI)
        {
            ssInfo->nbrBsSignalMeas[i].dlMeanMeas.rssi = meanMeas.rssi;
        }
        if (reportMetric & DOT16e_MSHO_ReportMetricRelativeDelay)
        {
            ssInfo->nbrBsSignalMeas[i].dlMeanMeas.relativeDelay =
                meanMeas.relativeDelay;
        }

        ssInfo->nbrBsSignalMeas[i].dlMeanMeas.measTime = node->getNodeTime();
        if (!ssInfo->nbrBsSignalMeas[i].dlMeanMeas.isActive)
        {
            ssInfo->nbrBsSignalMeas[i].dlMeanMeas.isActive = TRUE;
        }

    }

    for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i ++)
    {
        if (ssInfo->nbrBsSignalMeas[i].bsValid &&
            (ssInfo->nbrBsSignalMeas[i].dlMeanMeas.measTime +
            2 * DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP <
            node->getNodeTime()))
        {
            ssInfo->nbrBsSignalMeas[i].bsValid = FALSE;
        }
    }
}

// /**
// FUNCTION   :: MacDot16eBsHandleMobScnRep
// LAYER      :: MAC
// PURPOSE    :: Handle a MOB-SCN-REP message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleMobScnRep(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobScnRepMsg* scanRep;
    MacDot16BsSsInfo* ssInfo;
    int index;
    int numBsId;
    int numBsIndex;
    unsigned char reportMetric;
    int i;
    unsigned char bsId[DOT16_BASE_STATION_ID_LENGTH];
    MacDot16SignalMeasurementInfo meanMeas;
    memset(&meanMeas, 0, sizeof(MacDot16SignalMeasurementInfo));

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REP message
    scanRep = (MacDot16eMobScnRepMsg*) &(macFrame[index]);
    numBsId = scanRep->numBsId;
    numBsIndex = scanRep->numBsIndex;
    reportMetric = scanRep->reportMetric;

    index += sizeof(MacDot16eMobScnRepMsg);

    if (DEBUG_PACKET || DEBUG_NBR_SCAN)
    {
        printf("Node%d handle MOB-SCN-REP with cid as %d\n",
               node->nodeId, MacDot16MacHeaderGetCID(macHeader));
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update the stat
    dot16Bs->stats.numNbrScanRepRcvd ++;

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 MacDot16MacHeaderGetCID(macHeader));

    if (ssInfo != NULL)
    {
        // handle the contents in the msg

        // now numNsIndex = 0 is supported
        for (i = 0; i < numBsId; i ++)
        {
            // copy the bs Id
            memcpy(bsId,
                   (unsigned char*) &macFrame[index],
                   DOT16_BASE_STATION_ID_LENGTH);

            index += DOT16_BASE_STATION_ID_LENGTH;

            if (reportMetric & DOT16e_MSHO_ReportMetricCINR)
            {
                meanMeas.cinr = 0.5 * macFrame[index ++];

            }
            if (reportMetric & DOT16e_MSHO_ReportMetricRSSI)
            {
                meanMeas.rssi = macFrame[index ++] *
                                DOT16_SS_DEFAULT_POWER_UNIT_LEVEL +
                                DOT16e_RSSI_BASE;
            }
            if (reportMetric & DOT16e_MSHO_ReportMetricRelativeDelay)
            {
                meanMeas.relativeDelay = macFrame[index ++];
            }

            MacDot16BsUpdateSsNbrSigMeanMeas(node,
                                             dot16,
                                             ssInfo,
                                             reportMetric,
                                             bsId,
                                             meanMeas);
        }
    }
    else
    {
        // not in the list
    }

    return index - startIndex;
}

// /**
// FUNCITON   :: MacDot16eBsHandleMobMshoReq
// LAYER      :: MAC
// PURPOSE    :: Handle a MOB MSHO REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleMobMshoReq(Node* node,
                                MacDataDot16* dot16,
                                unsigned char* macFrame,
                                int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobMshoReqMsg* msHoReq;
    MacDot16BsSsInfo* ssInfo;
    int index;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the MSHO-REQ message
    msHoReq = (MacDot16eMobMshoReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobMshoReqMsg);

    if (DEBUG_PACKET || DEBUG_HO)
    {
        printf("Node%d handle MOB-MSHO-REQ with cid as %d\n",
               node->nodeId, MacDot16MacHeaderGetCID(macHeader));
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // update stats
    dot16Bs->stats.numMsHoReqRcvd ++;

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 MacDot16MacHeaderGetCID(macHeader));

    if (ssInfo != NULL)
    {
        Message* pduMsg;

        if (ssInfo->bsInitHoStart)
        {
            // if BS inited handover as well, cancel BS initiated one
            // ssInfo->bsInitHoStart = FALSE;

            if (DEBUG_HO)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS rcvd MS init HO Req, Cancel BS init HO Req\n");
            }
        }

        // schedule to send a MOB-SCN-RSP to the SS
        pduMsg = MacDot16eBsBuildMobBshoRspPdu(node, dot16, ssInfo);

        MacDot16BsScheduleMgmtMsgToSs(node,
                                      dot16,
                                      ssInfo,
                                      ssInfo->basicCid,
                                      pduMsg);

        // update stats
        dot16Bs->stats.numBsHoRspSent ++;
    }
    else
    {
        // not in the list
    }

    return index - startIndex;
}
// /**
// FUNCTION   :: MacDot16eBsHandleMobHoInd
// LAYER      :: MAC
// PURPOSE    :: Handle a MOB-HO-IND message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleMobHoInd(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eMobHoIndMsg* hoInd;
    MacDot16eMobHoIndHoMod* hoIndHoMod = NULL;
    MacDot16BsSsInfo* ssInfo;
    int index;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the RNG-REQ message
    hoInd = (MacDot16eMobHoIndMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eMobHoIndMsg);

    if (DEBUG_PACKET || DEBUG_HO)
    {
        printf("Node%d handle MOB-HO-IND with cid as %d\n",
               node->nodeId, MacDot16MacHeaderGetCID(macHeader));
    }

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    dot16Bs->stats.numHoIndRcvd ++;

    if (hoInd->mode == DOT16e_HOIND_Ho)
    {
        hoIndHoMod = (MacDot16eMobHoIndHoMod*) &(macFrame[index]);
        index += sizeof(MacDot16eMobHoIndHoMod);

        if (hoIndHoMod->hoIndType == DOT16e_HOIND_ReleaseServBs)
        {
            index += DOT16_BASE_STATION_ID_LENGTH;
        }
    }

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 MacDot16MacHeaderGetCID(macHeader));

    if (ssInfo  && !ssInfo->inHandover)
    {
        if (ssInfo->bsInitHoStart)
        {
            // if BS inited handover as well, cancel BS initiated one
            // or this is SS's indication to the BS initiated HO Req
            ssInfo->bsInitHoStart = FALSE;
        }

        ssInfo->inHandover = TRUE;

        // remove SS from the list
        if (hoInd->mode == DOT16e_HOIND_Ho &&
            hoIndHoMod->hoIndType == DOT16e_HOIND_ReleaseServBs)
        {
            // start resource retain timer
            if (ssInfo->resrcRetainTimer != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->resrcRetainTimer);
                ssInfo->resrcRetainTimer = NULL;
            }

            ssInfo->resrcRetainTimer = MacDot16SetTimer(
                                           node,
                                           dot16,
                                           DOT16e_TIMER_ResrcRetain,
                                           dot16Bs->para.resrcRetainTimeout,
                                           NULL,
                                           ssInfo->basicCid);

            if (DEBUG_HO)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("schedule a resrc retain timer for SS with cid %d\n",
                        ssInfo->basicCid);
            }
        }

    }
    else
    {
        // SS not in the list, ignore the message
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eBsHandleMobSlpReq
// LAYER      :: MAC
// PURPOSE    :: Handle a MOB-SLP-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleMobSlpReq(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16BsSsInfo* ssInfo = NULL;
    Message* rspMsg = NULL;
    int index;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    if (DEBUG_SLEEP)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("handle MOB-SLP-REQ with cid as %d\n",
               MacDot16MacHeaderGetCID(macHeader));
    }


    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }
    dot16Bs->stats.numMobSlpReqRcvd ++;

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(
                 node,
                 dot16,
                 MacDot16MacHeaderGetCID(macHeader));

    index++; // skip type of message
    if (ssInfo != NULL)
    {
        UInt8 numOfPSClassInfoReceived =  macFrame[index++];// no of classes
        MacDot16ePSClasses* psClass = NULL;
        for (int i = 0; i < numOfPSClassInfoReceived; i++)
        {
            UInt8 startFrameNo = 0;
            BOOL needToActivate = FALSE; // for class 1 & 2
            //Get Definition(1 bit), Operation(1 bit)
            BOOL defination = ((macFrame[index] & 0x01) == 0x01);
            BOOL operation = ((macFrame[index] & 0x02) == 0x02);
            // Get Power_Saving_Class_ID(6 bit)
            //UInt8 powerSavingClassId = ((macFrame[index] & 0xFC) >> 2);
            index++;

            if (operation)
            {
                startFrameNo = macFrame[index++] & 0x3F;
                needToActivate = TRUE;
            }
            if (defination)
            {
                UInt16 finalSleepWindow = 0;
                UInt8 noOfCIDs = 0;
                psClass = &ssInfo->psClassInfo[(macFrame[index] & 0x03) - 1];
                psClass->classType =
                    (MacDot16ePSClassType) (macFrame[index] & 0x03);
                psClass->psDirection = ((macFrame[index] & 0x0C) >> 2);
                psClass->trafficTriggeredFlag =
                    ((macFrame[index] & 0x10) >> 4);
                index++;

                psClass->initialSleepWindow = macFrame[index++];
                psClass->listeningWindow = macFrame[index++];
                MacDot16TwoByteToShort(finalSleepWindow,
                    macFrame[index],
                    macFrame[index + 1]);
                index += 2;
                psClass->finalSleepWindowBase =
                    (finalSleepWindow & 0x03FF);
                psClass->finalSleepWindowExponent =
                    (UInt8)((finalSleepWindow & 0x1C00) >> 10);
                noOfCIDs = (UInt8)(finalSleepWindow >> 13);
                if (noOfCIDs != 0)
                {
                    index += (noOfCIDs * 2);
                }

                if (needToActivate == TRUE)
                {
                    MacDot16PhyOfdma* dot16Ofdma =
                        (MacDot16PhyOfdma*) dot16->phyData;
                    int tempFrameNumInfo = dot16Ofdma->frameNumber
                        & 0x0000003F;
                    if (tempFrameNumInfo <= startFrameNo)
                    {
                        psClass->startFrameNum =
                            dot16Ofdma->frameNumber +
                            (startFrameNo - tempFrameNumInfo);
                    }
                    else
                    {
                        psClass->startFrameNum =
                            dot16Ofdma->frameNumber + 1;
                    }
                    MacDot16eSetPSClassStatus(psClass,
                        POWER_SAVING_CLASS_ACTIVATE);
                    ssInfo->mobTrfSent = FALSE;
                    psClass->statusNeedToChange = TRUE;
                    psClass->sleepDuration = psClass->initialSleepWindow;
                }
                else
                {

                    MacDot16eSetPSClassStatus(psClass,
                        POWER_SAVING_CLASS_DEACTIVATE);
                    psClass->statusNeedToChange = TRUE;
                }

                // check if the packet is present at BS for this PS class
                // set the psClass->statusNeedToChange = FALSE;
                if ((psClass->currentPSClassStatus ==
                    POWER_SAVING_CLASS_ACTIVATE) &&
                    MacDot16eBsHasPacketForPSClassType(node, dot16, ssInfo,
                    psClass->classType))
                {
                    psClass->statusNeedToChange = FALSE;
                }
            }
            else
            {
                // Yet, Power saving class id feature does not support
            }
        }// end of for
        // build mob-slp-rsp message
        rspMsg = MacDot16eBsBuildMobSlpRsp(node, dot16, ssInfo);
        // schedule the transmission
        MacDot16BsScheduleMgmtMsgToSs(
            node,
            dot16,
            ssInfo,
            ssInfo->basicCid,
            rspMsg);
    }
    else
    {
        // SS not in the list, ignore the message
    }// end of if-else ssInfo != NULL

    return index - startIndex;
}// end of MacDot16eBsHandleMobSlpReq

static
void MacDot16eBSInitPSClassParameter(MacDot16ePSClasses* psClass)
{
    memset(psClass, 0 ,sizeof(MacDot16ePSClasses));
    psClass->classType = POWER_SAVING_CLASS_NONE;
    psClass->currentPSClassStatus = POWER_SAVING_CLASS_STATUS_NONE;
}// end of MacDot16eBSInitPSClassParameter

// /**
// FUNCTION   :: MacDot16eBsHandleDregReqMsg
// LAYER      :: MAC
// PURPOSE    :: Handle a DREG-REQ message
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + macFrame  : unsigned char*: Pointer to the mac PDU
// + startIndex: int           : start of the payload in the PDU
// RETURN     :: int : Number of bytes handled
// **/
static
int MacDot16eBsHandleDregReqMsg(
                            Node* node,
                            MacDataDot16* dot16,
                            unsigned char* macFrame,
                            int startIndex)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16MacHeader* macHeader;
    MacDot16eDregReqMsg* dregReq;
    MacDot16BsSsInfo* ssInfo;
    int index;
    unsigned char tlvType;
    int tlvLength;
    int length;

    index = startIndex;

    // firstly, MAC header
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    // secondly common part of the DREG-REQ message
    dregReq = (MacDot16eDregReqMsg*) &(macFrame[index]);
    index += sizeof(MacDot16eDregReqMsg);

    length = MacDot16MacHeaderGetLEN(macHeader);
    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Handle DREG-REQ with cid as %d\n",
               MacDot16MacHeaderGetCID(macHeader));
    }

    dot16Bs->stats.numDregReqRcvd ++;

    if (!dot16->dot16eEnabled)
    {
        return MacDot16MacHeaderGetLEN(macHeader);
    }

    // find the corresponding SS in the list
    ssInfo = MacDot16BsGetSsByCid(node,
                                  dot16,
                                  MacDot16MacHeaderGetCID(macHeader));

    if (ssInfo != NULL)
    {
        if (dot16Bs->isSleepEnabled && ssInfo->isSleepEnabled)
        {
            for (int i = 0; i < DOT16e_MAX_POWER_SAVING_CLASS; i++)
            {
                MacDot16eBSInitPSClassParameter(&ssInfo->psClassInfo[i]);
            }
        }
    }

    // get all TLVs in the REP-REQ message
    while ((index - startIndex) < length)
    {
        tlvType = macFrame[index ++];
        tlvLength = macFrame[index ++];
        switch (tlvType)
        {
            case TLV_DREG_PagingCycleReq:
            {
                MacDot16TwoByteToShort(ssInfo->pagingCycle, macFrame[index],
                                       macFrame[index + 1]);
                break;
            }
            case TLV_DREG_IdleModeRetainInfo:
            {
                ssInfo->idleModeRetainInfo = macFrame[index];
                break;
            }
            case TLV_DREG_MacHashSkipThshld:
            {
                MacDot16TwoByteToShort(ssInfo->macHashSkipThshld,
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

    switch (dregReq->reqCode)
    {
        case DOT16e_DREGREQ_FromBS:
        {
            //currently not implemented
            break;
        }
        case DOT16e_DREGREQ_MSReq:
        {
            //MS initiated
            Message* dregCmdMsg = NULL;

            dregCmdMsg = MacDot16eBsBuildDregCmdPdu(node,
                                dot16,
                                ssInfo,
                                DOT16e_DREGCMD_Code5,
                                0);
            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->basicCid,
                                          dregCmdMsg);

            dot16Bs->stats.numDregCmdSent++;

            if (ssInfo->timerMgmtRsrcHldg != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerMgmtRsrcHldg);
                ssInfo->timerMgmtRsrcHldg = NULL;
            }
            ssInfo->timerMgmtRsrcHldg = MacDot16SetTimer(node,
                                        dot16,
                                        DOT16e_TIMER_MgmtRsrcHldg,
                                        dot16Bs->para.tMgmtRsrcHldgInterval,
                                        NULL,
                                        ssInfo->basicCid);

            //prepare and send IM_entry_state_change_req to PC
            MacDot16eBsSendImEntryReq(node, dot16, ssInfo);
            break;
        }
        case DOT16e_DREGREQ_MSRes:
        {
            //response to BS initiated idle mode
            if (ssInfo->timerT46 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, ssInfo->timerT46);
                ssInfo->timerT46 = NULL;
            }

            //prepare and send IM_entry_state_change_req to PC
            MacDot16eBsSendImEntryReq(node, dot16, ssInfo);
            break;
        }
        default:
        {
            // Not supported
            if (DEBUG_IDLE)
            {
                printf("Unknown DREG-REQ code = %d\n",
                       dregReq->reqCode);
            }
            break;
        }
    }


    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16eBsHandleImEntryRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a IM Entry response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleImEntryRsp(
                    Node* node,
                    MacDataDot16* dot16,
                    Message* msg)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
}

// /**
// FUNCTION   :: MacDot16eBsHandleImExitRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a IM Exit response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleImExitRsp(
                    Node* node,
                    MacDataDot16* dot16,
                    Message* msg)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

}

// /**
// FUNCTION   :: MacDot16eBsHandleLURsp
// LAYER      :: MAC
// PURPOSE    :: Handle a LU response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleLURsp(
                    Node* node,
                    MacDataDot16* dot16,
                    Message* msg)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

}

// /**
// FUNCTION   :: MacDot16eBsHandleIniPagRsp
// LAYER      :: MAC
// PURPOSE    :: Handle a Initiate Paging response from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleIniPagRsp(
                    Node* node,
                    MacDataDot16* dot16,
                    Message* msg)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    //currently nothing to be done
}

// /**
// FUNCTION   :: MacDot16eBsHandlePagingAnounce
// LAYER      :: MAC
// PURPOSE    :: Handle a Paging announce message from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandlePagingAnounce(
                            Node* node,
                            MacDataDot16* dot16,
                            Message* msg)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16ePagingAnnouncePdu* pgAnn = (Dot16ePagingAnnouncePdu*)
                                            MESSAGE_ReturnPacket(msg);
    int i;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Received paging announce message for "
               "node: %d:%d:%d:%d:%d:%d\n",
                pgAnn->msMacAddress[0],
                pgAnn->msMacAddress[1],
                pgAnn->msMacAddress[2],
                pgAnn->msMacAddress[3],
                pgAnn->msMacAddress[4],
                pgAnn->msMacAddress[5]);
    }


    if (pgAnn->pagingStart == FALSE)    //stop paging
    {
        for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
        {
            if (dot16Bs->msPgInfo[i].isValid == TRUE &&
                MacDot16SameMacAddress(dot16Bs->msPgInfo[i].msMacAddress,
                                       pgAnn->msMacAddress))
            {
                dot16Bs->msPgInfo[i].isValid = FALSE;
                break;
            }
        }

        if (DEBUG_IDLE)
        {
            printf("Stop Paging for the mentioned node\n");
        }
    }
    else    // start paging
    {
        // just add to msPgInfo, BS Paging Timer would take care of
        // generating MOB_PAG_ADV
        for (i = 0; i < DOT16e_PC_MAX_SS_IDLE_INFO; i++)
        {
            if (dot16Bs->msPgInfo[i].isValid == FALSE)
            {
                dot16Bs->msPgInfo[i].isValid = TRUE;
                MacDot16CopyMacAddress(dot16Bs->msPgInfo[i].msMacAddress,
                                       pgAnn->msMacAddress);
                dot16Bs->msPgInfo[i].actCode = DOT16e_MOB_PAG_ADV_EntNw;
                break;
            }
        }

        if (DEBUG_IDLE)
         {   printf("Adding macaddress to Paging Info so that it is"
                    " included in MOB_PAG_ADV\n");
         }
    }
}

// /**
// FUNCTION   :: MacDot16eBsHandleDeleteMsEntry
// LAYER      :: MAC
// PURPOSE    :: Handle a Delete MS entry msg from PC
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the PDU
// RETURN     :: void : NULL
// **/
void MacDot16eBsHandleDeleteMsEntry(
                                    Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow;
    MacDot16BsSsInfo* prevSsInfo = NULL;
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];
    int i, j;

    MacDot16CopyMacAddress(macAddress, MESSAGE_ReturnPacket(msg));

    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];
        prevSsInfo = NULL;

        while (ssInfo != NULL)
        {
            if (MacDot16SameMacAddress(macAddress, ssInfo->macAddress))
            {
                break;
            }
            prevSsInfo = ssInfo;
            ssInfo = ssInfo->next;
        }

        if (ssInfo != NULL)
        {
            if (ssInfo->isIdle)
            {
                if (prevSsInfo == NULL)
                {
                    dot16Bs->ssHash[i] = ssInfo->next;
                }
                else
                {
                    prevSsInfo->next = ssInfo->next;
                }
                MEM_free((void*) ssInfo);
                break;
            }
            else
            {
                ssInfo->sentIniPagReq = FALSE;
                for (j = 0; j < DOT16_NUM_SERVICE_TYPES; j++)
                {
                    sFlow = ssInfo->dlFlowList[j].flowHead;
                    while (sFlow != NULL)
                    {
                        if (sFlow->numBytesInTmpMsgQueue > 0)
                        {
                            if (sFlow->activated == FALSE)
                            {
                                MacDot16BsScheduleMgmtMsgToSs(
                                    node,
                                    dot16,
                                    ssInfo,
                                    ssInfo->primaryCid,
                                    MESSAGE_Duplicate(
                                    node,
                                    sFlow->dsaInfo.dsxReqCopy));
                            }
                        }
                        else
                        {
                            MacDot16CsInvalidClassifier(node,
                                dot16,
                                sFlow->csfId);
                        }
                        sFlow = sFlow->next;
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------
// Handle dot16e msgs (end)
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16BsHandleDataPdu
// LAYER      :: MAC
// PURPOSE    :: Handle a data PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 data struct
// + msg       : Message*      : Message containing the data PDU
// RETURN     :: int : Number of bytes handled
// **/
static
Message* MacDot16BsHandleDataPdu(Node* node,
                                 MacDataDot16* dot16,
                                 Message* msg)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    int dataLen = 0;
    MacDot16DataGrantSubHeader* subhead;
    Message* retMsg ;


    dataLen = MESSAGE_ReturnPacketSize(msg);

    // Copy the generic mac header
    macHeader = (MacDot16MacHeader*)MEM_malloc(sizeof(MacDot16MacHeader));
    memcpy(macHeader, MESSAGE_ReturnPacket(msg),
        sizeof(MacDot16MacHeader));

    cid = MacDot16MacHeaderGetCID(macHeader);

    // find the corresponding service flow
    if (cid < DOT16_TRANSPORT_CID_START)
    {
        // currently mgmt cid  does not support subheader
        ssInfo = MacDot16BsGetSsByCid(
                     node,
                     dot16,
                     cid);
    }
    else
    {
        // for service flow cid
        MacDot16BsGetServiceFlowByCid(node,
                                      dot16,
                                      cid,
                                      &ssInfo,
                                      &sFlow);
    }

    if (ssInfo == NULL || sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no ssInfo or sFlow match the CID in data PDU");
        }
        MEM_free(macHeader);
        Message* nextMsg = msg->next;
        MESSAGE_Free(node, msg);
        return nextMsg;
    }

// Remove CRC iformation
    if (MacDot16MacHeaderGetCI(macHeader))
    {
        if (DEBUG_CRC)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Remove %d bytes virtual CRC information\n",
                DOT16_CRC_SIZE);
        }
        MESSAGE_RemoveVirtualPayload(node, msg, DOT16_CRC_SIZE);
        UInt16 newLen =
            (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
            - DOT16_CRC_SIZE);
        MacDot16MacHeaderSetLEN(macHeader,
            newLen);
    }
    // Remove Generic MAC header
    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(MacDot16MacHeader),
                         TRACE_DOT16);
    UInt16 newLen = (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
        - sizeof(MacDot16MacHeader));
    MacDot16MacHeaderSetLEN(macHeader,
        newLen);

    // handle subheader
    if (MacDot16MacHeaderGetGeneralType(macHeader) != 0 )
    {
        if ((MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_FAST_FEEDBACK_ALLOCATION_SUBHEADER))
        {
            // remove grant management sub header
            if (sFlow != NULL)
            {
                // service flow
                // data grant management
                if (sFlow->serviceType == DOT16_SERVICE_UGS)
                {

                }
                else
                {
                    int bwReq;

                    subhead = (MacDot16DataGrantSubHeader*)
                               MESSAGE_ReturnPacket(msg);

                    bwReq = MacDot16DataGrantSubHeaderGetPBR(subhead);
                    sFlow->bwRequested = sFlow->lastBwRequested + bwReq;
                    if (sFlow->serviceType == DOT16_SERVICE_ertPS)
                    {
                        sFlow->lastBwRequested = sFlow->bwRequested;
                    }
                    if (DEBUG_BWREQ)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("Get a pigyback a BW INCREASE"
                               "req %d for sFlow with cid %d\n",
                                bwReq, sFlow->cid);
                    }
                }
            }
            // remove MAC header
            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(MacDot16DataGrantSubHeader),
                                 TRACE_DOT16);
            UInt16 newLen =
                (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
                - sizeof(MacDot16DataGrantSubHeader));
            MacDot16MacHeaderSetLEN(macHeader,
                newLen);
        }
    }
    //Check if ARQ is enabled.
    if (sFlow->isARQEnabled)
    {
        int pduLength = MacDot16MacHeaderGetLEN(macHeader);
        retMsg = MacDot16ARQHandleDataPdu(
            node,
            dot16,
            sFlow,
            ssInfo,
            msg,
            pduLength,
            (BOOL)((UInt8)MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_PACKING_SUBHEADER));
        MEM_free(macHeader);
        return  retMsg;
    }
    // Remove Other SubHeader
    // Remove Variable and fragmented header
        if ((MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_FRAGMENTATION_SUBHEADER))
        {
            UInt8 fragMsgFC;
            UInt16 fragMsgFSN;
            Message* fragMsg = msg;
            msg = msg->next;
            fragMsg->next = NULL;
            // Remove fragmentation subheader
            if (DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Remove Fragmentation Subheader\n");
            }
            dot16Bs->stats.numFragmentsRcvd++;
            MacDot16NotExtendedARQDisableFragSubHeader* fragSubHeader =
                (MacDot16NotExtendedARQDisableFragSubHeader*)
                MESSAGE_ReturnPacket(fragMsg);
//            UInt16 dataLen = (UInt16)(MacDot16MacHeaderGetLEN(macHeader)
//                - sizeof(MacDot16NotExtendedARQDisableFragSubHeader));
            fragMsgFC = MacDot16FragSubHeaderGetFC(fragSubHeader);
            fragMsgFSN = MacDot16FragSubHeaderGet3bitFSN(fragSubHeader);
            // remove Packing header
            MESSAGE_RemoveHeader(
                node,
                fragMsg,
                sizeof(MacDot16NotExtendedARQDisableFragSubHeader),
                TRACE_DOT16);

            switch(fragMsgFC)
            {
                case DOT16_NO_FRAGMENTATION:
                    {
                        ERROR_ReportWarning("MAC 802.16: Fragmentation"
                            " subheader could not contain unfragmented"
                            " packet");
                    }
                    break;
                case DOT16_FIRST_FRAGMENT:
                    {
                        MacDot16HandleFirstFragDataPdu(
                            node,
                            dot16,
                            fragMsg,
                            sFlow,
                            fragMsgFSN);
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("BS handle first fragmented packet\n");
                        }
                    }
                    break;
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
                                printf("BS Drop the middle fragmented"
                                    " packet\n");
                            }
                            MESSAGE_Free(node, fragMsg);
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
                            tempMsg->next = fragMsg;
                            fragMsg->next = NULL;
                        }
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("BS handle middle fragmented packet\n");
                        }
                    }
                     break;
                case DOT16_LAST_FRAGMENT:
                    {
                        MacDot16HandleLastFragDataPdu(
                            node,
                            dot16,
                            fragMsg,
                            sFlow,
                            fragMsgFSN,
                            ssInfo->macAddress);
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("BS handle last fragmented packet\n");
                        }
                    }
                    break;
                default:
                    break;
                }// end of switch
            MEM_free(macHeader);
            return msg;
        }// end of if DOT16_FRAGMENTATION_SUBHEADER


    if ((MacDot16MacHeaderGetGeneralType(macHeader) &
            DOT16_PACKING_SUBHEADER) || (sFlow->isPackingEnabled
            && sFlow->isFixedLengthSDU))
    {
        int pduLength = (int)MacDot16MacHeaderGetLEN(macHeader);
        Message* packMsg = msg;
        UInt16 noOfPackPacketRecvd = 0;
        UInt8 packetDataFC = 0;
        UInt8 packetDataFSN = 0;

        while (pduLength > 0)
        {
            if (DEBUG_PACKING_FRAGMENTATION &&
                sFlow->isFixedLengthSDU == FALSE)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS Remove Packing Subheader\n");
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
                    packHeader = (MacDot16NotExtendedARQDisablePackSubHeader*)
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
                        sizeof(MacDot16NotExtendedARQDisablePackSubHeader),
                        TRACE_DOT16);
                    packetDataFC =
                        MacDot16PackSubHeaderGetFC(packHeader);
                    packingHeaderSize =
                            sizeof(MacDot16NotExtendedARQDisablePackSubHeader);
                }
                else
                {
                    packingHeaderSize = 0;
                    dataLen = (UInt16) sFlow->fixedLengthSDUSize;
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
                            printf("BS Received packed packet\n");
                        }
                    }
                    break;
                case DOT16_LAST_FRAGMENT:
                    {
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("BS Received last fragmented packet\n");
                        }
                    }
                    break;
                case DOT16_FIRST_FRAGMENT:
                    {
                        if (DEBUG_PACKING_FRAGMENTATION)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("BS Received first fragmented packet\n");
                        }
                    }
                    break;
                case DOT16_MIDDLE_FRAGMENT:
                    {
                        ERROR_ReportWarning("MAC 802.16: Packing subheader"
                            " could not contain middle fragmented packet");
                    }
                    break;
                default:
                    break;
                }
            }
            noOfPackPacketRecvd++;
            if (packetDataFC == DOT16_NO_FRAGMENTATION)
            {
                MacDot16CsPacketFromLower(node,
                                          dot16,
                                          packMsg,
                                          ssInfo->macAddress,
                                          ssInfo);
                // increase statistics
                dot16Bs->stats.numPktsFromLower ++;
                // update dynamic stats
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        dot16Bs->stats.numPktFromPhyGuiId,
                                        dot16Bs->stats.numPktsFromLower,
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
                        ssInfo->macAddress);
                    dot16Bs->stats.numFragmentsRcvd++;
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
                        dot16Bs->stats.numFragmentsRcvd++;
                    }// end of if packetDataFC[i] == DOT16_FIRST_FRAGMENT
                    else
                    {
                        // ignore
                    }
                }
            }
        }// end of while
        MEM_free(macHeader);
        return msg;
    }
    Message* nextMsg = msg->next;
    // pass to CS layer
    MacDot16CsPacketFromLower(node,
                              dot16,
                              msg,
                              ssInfo->macAddress,
                              ssInfo);

    // increase statistics
    dot16Bs->stats.numPktsFromLower ++;
    // update dynamic stats
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            dot16Bs->stats.numPktFromPhyGuiId,
                            dot16Bs->stats.numPktsFromLower,
                            node->getNodeTime());
    }
    MEM_free(macHeader);
    return nextMsg;
}

// /**
// FUNCTION   :: MacDot16BsAddServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Add a new service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + macAddress: unsigned char* : MAC address of nextHop
// + serviceType : MacDot16ServiceType : Service type of the flow
// + csfId : unsigned int : Classifier ID
// + qosInfo : MacDot16QoSParameter : QoS parameter of the flow
// + pktInfo : MacDot16CsIpv4Classifier* : Classifier of the flow
// + transId : unsigned int : transaction id for this service flow,
//                            it is only meaningful for uplink service flow
// + sfDirection : MacDot16ServiceFlowDirection : Service flow direction,
//                 ul or dl
// + sfInitial : MacDot16ServiceFlowInitial : Locally initiated or
//               remotely initaited
// + ssCid   : Dot16CIDType : SS's primary cid
// + transportCid : Dot16CIDType* : Pointer to the transport Cid
//                  assigned to this service flow
// + isARQEnabled : BOOL : Indicates whether ARQ is to be enabled for the
//                         flow
// + arqParam : MacDot16ARQParam* : Pointer to the structure containing the
//                                  ARQ Parameters to be used in case ARQ is
//                                  enabled.
// RETURN     :: Dot16CIDType : Basic CID of SS that the flow intended for
// **/

Dot16CIDType MacDot16BsAddServiceFlow(
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
                 Dot16CIDType ssCid,
                 Dot16CIDType* transportCid,
                 BOOL isPackingEnabled,
                 BOOL isFixedLengthSDU,
                 unsigned int fixedLengthSDUSize,
                 TraceProtocolType appType,
                 BOOL isARQEnabled,
                 MacDot16ARQParam* arqParam)

{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    BOOL isMcastFlow;
    if (DEBUG_FLOW)
    {
        char clockStr[MAX_CLOCK_STRING_LENGTH];
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Adding a new service flow dl/ul %d csfId %d\n",
               sfDirection, csfId);

        printf("    nextHop(DL)/prevHop(UL) MAC address = "
               " %d:%d:%d:%d:%d:%d\n",
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

    isMcastFlow = MacDot16IsMulticastMacAddress(node, dot16, macAddress);

    if (!isMcastFlow && sfDirection == DOT16_DOWNLINK_SERVICE_FLOW)
    {
        ssInfo = MacDot16BsGetSsByMacAddress(node, dot16, macAddress);

        if (ssInfo == NULL)
        {
            // this SS doesn't exist
            // BS is not unable to create a service flow to it
            // Indicated by returnning DOT16_INITIAL_RANGING_CID;
            // this transportCid is to indicate SS is not exist
            return DOT16_INITIAL_RANGING_CID;
        }
        else if (!ssInfo->isRegistered)
        {
            // this SS doesn't registered with me
            return DOT16_INITIAL_RANGING_CID;
        }
    }

    // SS found

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
    sFlow->admitted = FALSE;
    sFlow->sfid = dot16Bs->serviceFlowId;
    dot16Bs->serviceFlowId ++;

    sFlow->csfId = csfId; // for remote initated sf, no meaning
    sFlow->serviceType = serviceType;
    sFlow->emptyBeginTime = node->getNodeTime();
    sFlow->lastAllocTime = node->getNodeTime();

    // copy over QoS parameters
    memcpy((char*) &(sFlow->qosInfo),
           (char*) qosInfo,
           sizeof(MacDot16QoSParameter));
    sFlow->sfDirection = sfDirection;

    // initialize the dsx transaction info
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsaInfo));
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dscInfo));
    MacDot16ResetDsxInfo(node, dot16, sFlow, &(sFlow->dsdInfo));

    // start dsa transaction
    sFlow->dsaInfo.sfInitial = sfInitial;

    sFlow->isPackingEnabled = isPackingEnabled;
    sFlow->isFixedLengthSDU = isFixedLengthSDU;
    sFlow->fixedLengthSDUSize = fixedLengthSDUSize;
    sFlow->isFragStart = FALSE;
    sFlow->bytesSent = 0;
    sFlow->fragMsg = NULL;
    sFlow->fragFSNno = 0;
    sFlow->appType = appType;
    sFlow->TrafficIndicationPreference = DOT16e_TRAFFIC_INDICATION_PREFERENCE;
    sFlow->isARQEnabled = isARQEnabled;


  if (sFlow->isARQEnabled)
    {
        sFlow->arqParam = (MacDot16ARQParam*)
                                        MEM_malloc(sizeof(MacDot16ARQParam));
        memset(sFlow->arqParam, 0, sizeof(MacDot16ARQParam));

        memcpy(sFlow->arqParam, arqParam, sizeof(MacDot16ARQParam));

        if (sFlow->isARQEnabled && DEBUG_ARQ_INIT )
        {
            printf("MacDot16BsAddServiceFlow ARQ Enabled = %d\n",
                sFlow->isARQEnabled);
            MacDot16PrintARQParameter(sFlow->arqParam);
        }
    }

    if (isMcastFlow && sfDirection == DOT16_DOWNLINK_SERVICE_FLOW)
    {
        // bcast/mcast flows must be downlink
        sFlow->admitted = TRUE;
        sFlow->activated = TRUE;
        sFlow->cid = DOT16_MULTICAST_ALL_SS_CID;
        MacDot16SchAddQueueForServiceFlow(node, dot16, sFlow);
        sFlow->next = dot16Bs->mcastFlowList[serviceType].flowHead;
        dot16Bs->mcastFlowList[serviceType].flowHead = sFlow;
        dot16Bs->mcastFlowList[serviceType].numFlows ++;
        if (DEBUG_FLOW)
        {
            printf("a bcast/mcast flow with flow ID %d service type %d \n",
                    sFlow->sfid,serviceType);
        }
        return  sFlow->cid;
    }
    else
    {
        // unicast service flow
        if (ssInfo == NULL)
        {
            ssInfo = MacDot16BsGetSsByCid(node, dot16, ssCid);
        }
        sFlow->cid = MacDot16BsAllocTransportCid(node, dot16, ssInfo);
        *transportCid = sFlow->cid;
        // increase xActNum;
        sFlow->numXact ++;
        // add into the list
        if (sfDirection == DOT16_DOWNLINK_SERVICE_FLOW)
        {
            sFlow->next = ssInfo->dlFlowList[serviceType].flowHead;
            ssInfo->dlFlowList[serviceType].flowHead = sFlow;
            ssInfo->dlFlowList[serviceType].numFlows ++;
            ssInfo->dlFlowList[serviceType].maxSustainedRate +=
                                    sFlow->qosInfo.maxSustainedRate;
            ssInfo->dlFlowList[serviceType].minReservedRate +=
                                    sFlow->qosInfo.minReservedRate;
            if (DEBUG_FLOW)
            {
                printf("    a DL service flow with sfid %d and CID %d\n",
                       sFlow->sfid, sFlow->cid);
            }
        }
        else
        {
            sFlow->next = ssInfo->ulFlowList[serviceType].flowHead;
            ssInfo->ulFlowList[serviceType].flowHead = sFlow;
            ssInfo->ulFlowList[serviceType].numFlows ++;
            ssInfo->ulFlowList[serviceType].maxSustainedRate +=
                                    sFlow->qosInfo.maxSustainedRate;
            ssInfo->dlFlowList[serviceType].minReservedRate +=
                                    sFlow->qosInfo.minReservedRate;
            if (DEBUG_FLOW)
            {
                printf("    an UL service flow with sfid %d and CID %d\n",
                       sFlow->sfid, sFlow->cid);
            }
        }

        if (sfInitial == DOT16_SERVICE_FLOW_LOCALLY_INITIATED)
        {
            Message* pduMsg;
            // Check if we can admit new flow
            // we always admit a BE or UGS or ertPS service flow and a
            // nrtPS or rtPS service flow with a Min Reserved Rate of 0
            if (//(serviceType != DOT16_SERVICE_UGS) &&
                //(serviceType != DOT16_SERVICE_ertPS) &&
               (dot16Bs->acuAlgorithm != DOT16_ACU_NONE)
               && (sFlow->serviceType != DOT16_SERVICE_BE)  &&
               (sFlow->qosInfo.minReservedRate > 0 ) )
            {
                int admitted =
                    MacDot16BsPerformAdmissionControl(
                        node,
                        dot16,
                        ssInfo,
                        sFlow->serviceType,
                        sFlow->qosInfo,
                            //qosInfo.minReservedRate,
                            sfDirection);

                 if (!admitted)
                 {
                     sFlow->dsaInfo.dsxTransStatus =
                        DOT16_FLOW_DSX_NULL;
                     return ssInfo->basicCid;
                 }
            }

            sFlow->dsaInfo.dsxTransStatus = DOT16_FLOW_DSA_LOCAL_Begin;
            // set the dsx with a transactonId
            sFlow->dsaInfo.dsxTransId = dot16Bs->transId;

            // flow management, dsa-req
            pduMsg = MacDot16BuildDsaReqMsg(
                         node,
                         dot16,
                         sFlow,
                         pktInfo,
                         sfDirection,
                         ssInfo->primaryCid);

            // adjust the transactionId for future usage
            if (dot16Bs->transId == DOT16_BS_TRANSACTION_ID_END)
            {
                dot16Bs->transId = DOT16_BS_TRANSACTION_ID_START;
            }
            else
            {
                dot16Bs->transId ++;
            }

            // make a copy of the dsa-req msg
            sFlow->dsaInfo.dsxReqCopy = MESSAGE_Duplicate(node, pduMsg);

            // schedule the transmission
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                pduMsg);

            // increase stat
            dot16Bs->stats.numDsaReqSent ++;

            // Set retry
            sFlow->dsaInfo.dsxRetry --;

            // start T7
            if (sFlow->dsaInfo.timerT7 != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->dsaInfo.timerT7);
                sFlow->dsaInfo.timerT7 = NULL;
            }

            // at BS, transport cid is used as reference
            sFlow->dsaInfo.timerT7 = MacDot16SetTimer(
                                         node,
                                         dot16,
                                         DOT16_TIMER_T7,
                                         dot16Bs->para.t7Interval,
                                         NULL,
                                         sFlow->cid,
                                         DOT16_FLOW_XACT_Add);

            // move the dsx status to local RSA-RSP pending
            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_LOCAL_DsaRspPending;

            if (DEBUG_FLOW)
            {
                printf("  sche DSA-REQ & status local DSA-RSP pending\n");
            }
        }
        else
        {
            sFlow->dsaInfo.dsxTransStatus = DOT16_FLOW_DSA_REMOTE_Begin;
            sFlow->dsaInfo.dsxTransId = transId;

            if (DEBUG_FLOW)
            {
                printf("    create flow & status is remote DSA begin\n");
            }
        }

        return ssInfo->basicCid;
    }
}

// /**
// FUNCTION   :: MacDot16BsChangeServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Change a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + ssInfo : MacDot16BsSsInfo* : Pointer to the SS Info
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16BsChangeServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo,
                 MacDot16ServiceFlow* sFlow,
                 MacDot16QoSParameter *newQoSInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* pduMsg;
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
    sFlow->dscInfo.dsxTransId = dot16Bs->transId;

    // save the QoS state
    if (!sFlow->dscInfo.dscOldQosInfo){
    sFlow->dscInfo.dscOldQosInfo =
        (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
    }
    memcpy(sFlow->dscInfo.dscOldQosInfo,
           &(sFlow->qosInfo),
           sizeof(MacDot16QoSParameter));
    if (!sFlow->dscInfo.dscNewQosInfo){
    sFlow->dscInfo.dscNewQosInfo =
        (MacDot16QoSParameter*) MEM_malloc(sizeof(MacDot16QoSParameter));
    }    

    memcpy(sFlow->dscInfo.dscNewQosInfo,
           newQoSInfo,
           sizeof(MacDot16QoSParameter));

    // flow management, dsc-req
    pduMsg = MacDot16BuildDscReqMsg(
                 node,
                 dot16,
                 sFlow,
                 newQoSInfo,
                 ssInfo->primaryCid);

    // adjust the transactionId for future usage
    if (dot16Bs->transId == DOT16_BS_TRANSACTION_ID_END)
    {
        dot16Bs->transId = DOT16_BS_TRANSACTION_ID_START;
    }
    else
    {
        dot16Bs->transId ++;
    }

    // make a copy of the dsc-req msg
    if (sFlow->dscInfo.dsxReqCopy){
         MESSAGE_Free(node, sFlow->dscInfo.dsxReqCopy);
    }
    sFlow->dscInfo.dsxReqCopy = MESSAGE_Duplicate(node, pduMsg);

    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->primaryCid,
                                  pduMsg);

    // increase stat
    dot16Bs->stats.numDscReqSent ++;

    // Set retry
    sFlow->dscInfo.dsxRetry --;

    // start T7
    if (sFlow->dscInfo.timerT7 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, sFlow->dscInfo.timerT7);
        sFlow->dscInfo.timerT7 = NULL;
    }

    // at BS, transport cid is used as reference
    sFlow->dscInfo.timerT7 = MacDot16SetTimer(
                                 node,
                                 dot16,
                                 DOT16_TIMER_T7,
                                 dot16Bs->para.t7Interval,
                                 NULL,
                                 sFlow->cid,
                                 DOT16_FLOW_XACT_Change);

    // move the dsx status to local RSC-RSP pending
    sFlow->dscInfo.dsxTransStatus =
        DOT16_FLOW_DSC_LOCAL_DscRspPending;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("req BW and scheudle DSC-REQ for sf %d with dsxTransId %d\n",
               sFlow->sfid,
               sFlow->dscInfo.dsxTransId);
    }

}

// /**
// FUNCTION   :: MacDot16BsDeleteServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Delete a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to the SS Info
// + sFlow : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     ::
// **/
void MacDot16BsDeleteServiceFlow(
                 Node* node,
                 MacDataDot16* dot16,
                 MacDot16BsSsInfo* ssInfo,
                 MacDot16ServiceFlow* sFlow)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* pduMsg;

    // The service flow is inactive for a long time
    // in either NUll or nominal status, needs to delete it

    // start dsd transaction
    sFlow->status = DOT16_FLOW_Deleting;
    sFlow->numXact ++;
    sFlow->dsdInfo.dsxTransStatus = DOT16_FLOW_DSD_LOCAL_Begin;

    // set the dsx with a transactonId
    sFlow->dsdInfo.dsxTransId = dot16Bs->transId;

    // flow management, dsa-req
    pduMsg = MacDot16BuildDsdReqMsg(
                 node,
                 dot16,
                 sFlow,
                 ssInfo->primaryCid);

    // adjust the transactionId for future usage
    if (dot16Bs->transId == DOT16_BS_TRANSACTION_ID_END)
    {
        dot16Bs->transId = DOT16_BS_TRANSACTION_ID_START;
    }
    else
    {
        dot16Bs->transId ++;
    }

    // make a copy of the dsa-req msg
    if (sFlow->dsdInfo.dsxReqCopy){
         MESSAGE_Free(node, sFlow->dsdInfo.dsxReqCopy);
    }
    sFlow->dsdInfo.dsxReqCopy = MESSAGE_Duplicate(node, pduMsg);

    // schedule the transmission
    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->primaryCid,
                                  pduMsg);

    // decrease retry
    sFlow->dsdInfo.dsxRetry --;

    // increase stat
    dot16Bs->stats.numDsdReqSent ++;

    // start T7
    if (sFlow->dsdInfo.timerT7 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, sFlow->dsdInfo.timerT7);
        sFlow->dsdInfo.timerT7 = NULL;
    }
    sFlow->dsdInfo.timerT7 = MacDot16SetTimer(node,
                                              dot16,
                                              DOT16_TIMER_T7,
                                              dot16Bs->para.t7Interval,
                                              NULL,
                                              sFlow->cid,
                                              DOT16_FLOW_XACT_Delete);

    sFlow->dsdInfo.dsxTransStatus = DOT16_FLOW_DSD_LOCAL_DsdRspPending;

    // Notify CS to invalidate the classfier
    MacDot16CsInvalidClassifier(node, dot16, sFlow->csfId);

    // disable the service flow
    // Assume (de)admit and (de)activate happen at the same time
    sFlow->admitted = FALSE;
    sFlow->activated = FALSE;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("scheudle DSD-REQ for sf %d with "
               "dsTransId %d, start T7, move to DSD RSP pending\n",
               sFlow->sfid, sFlow->dsdInfo.dsxTransId);
    }
}

// /**
// FUNCTION   :: MacDot16BsEnqueuePacket
// LAYER      :: MAC
// PURPOSE    :: Enqueue packet of a service flow
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + cid   : Dot16CIDType  : Basic CID of the SS or multicast CID
// + csfId : unsigned int  : ID of the classifier at CS sublayer
// + serviceType : MacDot16ServiceType : Type of the data service flow
// + msg   : Message*      : The PDU from upper layer
// + flow  : MacDot16ServiceFlow** : Pointer to the service flow
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE: successfully enqueued, FALSE, dropped
// **/
BOOL MacDot16BsEnqueuePacket(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid,
         unsigned int csfId,
         MacDot16ServiceType serviceType,
         Message* msg,
         MacDot16ServiceFlow** flow,
         BOOL* msgDropped)
{
    *msgDropped = FALSE;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;
    BOOL queueIsFull = FALSE;

    if (MacDot16IsMulticastCid(node, dot16, cid))
    {
        sFlow = dot16Bs->mcastFlowList[serviceType].flowHead;
        if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled)
        {
            dot16Bs->psClass3.psClassIdleLastNoOfFrames = 0;
        }
    }
    else
    {
        ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

        if (ssInfo == NULL)
        {
            // this SS doesn't exist or not registered with me
            // I am unable to handle the packet
            MESSAGE_Free(node, msg);

            // increase statistics
            //dot16Bs->stats.numPktsFromUpper ++;
            dot16Bs->stats.numPktsDroppedUnknownSs ++;
            *msgDropped = TRUE;
            return TRUE;
        }

        if (dot16->dot16eEnabled &&
            ssInfo->isIdle == TRUE &&
            ssInfo->sentIniPagReq == FALSE)
        {
            if (DEBUG_IDLE)
        {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Packet received for node: %d:%d:%d:%d:%d:%d which is"
                        " in Idle mode.\n",
                ssInfo->macAddress[0],
                ssInfo->macAddress[1],
                ssInfo->macAddress[2],
                ssInfo->macAddress[3],
                ssInfo->macAddress[4],
                ssInfo->macAddress[5]);
            }

            MacDot16eBsSendIniPagReq(node, dot16, ssInfo);
            ssInfo->sentIniPagReq = TRUE;
        }

        // found the service flow record
        sFlow = ssInfo->dlFlowList[serviceType].flowHead;
    }

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

        // increase statistics
        dot16Bs->stats.numPktsFromUpper ++;
        *msgDropped = TRUE;
        return TRUE;
    }

    // pass flow back up to the calling function
    *flow = sFlow;

    // enqueue the packet
    msg->next = NULL;
    if (sFlow->activated == FALSE)
    {
        if (sFlow->numBytesInTmpMsgQueue + MESSAGE_ReturnPacketSize(msg) +
            sizeof(MacDot16MacHeader) <= DOT16_DEFAULT_DATA_QUEUE_SIZE)
        {
            // not activated yet, put into temp queue
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
            // Packet can be removed from queue if flow is not admitted
            // So that incase of multiple flows the other flow can continue

            if (sFlow->dsaInfo.dsxTransStatus == DOT16_FLOW_DSX_NULL)
            {
                MESSAGE_Free(node, msg);
                *msgDropped = TRUE;
                return TRUE;
            }
            return FALSE;
        }
    }
    else
    {
        dot16Bs->outScheduler[serviceType]->insert(msg,
                                                   &queueIsFull,
                                                   sFlow->queuePriority,
                                                   NULL,
                                                   node->getNodeTime());

        if (queueIsFull)
        {
            // notify CS to stop getting packets from upperl layer
            return FALSE;
        }
    }

    // increase statistics
    dot16Bs->stats.numPktsFromUpper ++;

    if (sFlow->isEmpty == TRUE)
    {
        sFlow->isEmpty = FALSE;
    }

    return TRUE;
}

// /**
// FUNCTION   :: MacDot16BsCheckHandover
// LAYER      :: MAC
// PURPOSE    : Check if SS needs to perform handover
// PARAMETERS ::
// + node   : Node* : Pointer to node
// + dot16  : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to SS info
// RETURN     :: void : NULL
static
void MacDot16BsCheckHandover(Node* node,
                             MacDataDot16* dot16,
                             MacDot16BsSsInfo* ssInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Message* pduMsg;

    // if ul link quality is bad or
    // system wise, for load balance
    // it is time to ask SS handover
    if (!ssInfo->inHandover &&
        !ssInfo->bsInitHoStart &&
        ssInfo->ulRssMean < dot16Bs->para.hoRssTrigger)
    {
        int i;
        double maxBsRssi;
        int maxIndex;
        BOOL found = FALSE;

        // ATTEN: need further revisit
        // assume UL and DL are symmetric?
        maxBsRssi = ssInfo->ulRssMean;
        for (i = 0; i < DOT16e_DEFAULT_MAX_NBR_BS; i++)
        {
            if (ssInfo->nbrBsSignalMeas[i].bsValid &&
                ((ssInfo->nbrBsSignalMeas[i].dlMeanMeas.measTime +
                2 * DOT16_SS_DEFAULT_NBR_SCAN_MIN_GAP) <
                node->getNodeTime()) &&
                ssInfo->nbrBsSignalMeas[i].dlMeanMeas.rssi > maxBsRssi)
            {
                found = TRUE;
                maxBsRssi =  ssInfo->nbrBsSignalMeas[i].dlMeanMeas.rssi;
                maxIndex = i;
            }
        }

        if (found &&
            (maxBsRssi - ssInfo->ulRssMean > dot16Bs->para.hoRssMargin))
        {
            pduMsg = MacDot16eBsBuildMobBshoReqPdu(
                         node,
                         dot16,
                         ssInfo,
                         ssInfo->nbrBsSignalMeas[i].bsId);

            MacDot16BsScheduleMgmtMsgToSs(node,
                                          dot16,
                                          ssInfo,
                                          ssInfo->basicCid,
                                          pduMsg);

            ssInfo->bsInitHoStart = TRUE;
            ssInfo->lastBsHoReqSent = node->getNodeTime();

            // update stat
            dot16Bs->stats.numBsHoReqSent ++;

            if (DEBUG_HO)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS inits a handover for SS w/ basicCid %d to"
                        "bs %d:%d:%d:%d:%d:%d\n",
                        ssInfo->basicCid,
                        ssInfo->nbrBsSignalMeas[i].bsId[0],
                        ssInfo->nbrBsSignalMeas[i].bsId[1],
                        ssInfo->nbrBsSignalMeas[i].bsId[2],
                        ssInfo->nbrBsSignalMeas[i].bsId[3],
                        ssInfo->nbrBsSignalMeas[i].bsId[4],
                        ssInfo->nbrBsSignalMeas[i].bsId[5]);
            }
        }
    }

}

// /**
// FUNCTION   :: MacDot16BsUpdateSsMeasurement
// LAYER      :: MAC
// PURPOSE    :: Update measurement of SS signal quality
// PARAMETERS ::
// + node   : Node* : Pointer to node
// + dot16  : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to SS info
// + signalMea : PhySignalMeasurement* : singal meaurement info
// RETURN     :: void : NULL
// **/
static
void MacDot16BsUpdateSsMeasurement(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16BsSsInfo* ssInfo,
                                   PhySignalMeasurement* signalMea)
{
    MacDot16SignalMeasurementInfo meanMeas;


    if (signalMea == NULL)
    {
        // no measurement data, do nothing
        return;
    }

    // update the measurement histroy and return the mean
    MacDot16UpdateMeanMeasurement(node,
                                  dot16,
                                  &(ssInfo->ulMeasInfo[0]),
                                  signalMea,
                                  &meanMeas);

    ssInfo->ulCinrMean = meanMeas.cinr;
    ssInfo->ulRssMean = meanMeas.rssi;
    ssInfo->lastUlMeaTime = node->getNodeTime();
}

// /**
// FUNCTION   :: MacDot16eBsInitiateIdleMode
// LAYER      :: MAC
// PURPOSE    :: BS initiates Idle mode
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo : MacDot16BsSsInfo* : Pointer to SS info
// + dregReqDur : UInt8         : Dreg-Req Duration, 0 = BS initiated
// RETURN     :: void : NULL
// **/
void MacDot16eBsInitiateIdleMode(Node * node,
                                 MacDataDot16* dot16,
                                 MacDot16BsSsInfo* ssInfo,
                                 UInt8 dregReqDur)
{
    Message* dregCmdMsg;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    dregCmdMsg = MacDot16eBsBuildDregCmdPdu(node,
                                            dot16,
                                            ssInfo,
                                            DOT16e_DREGCMD_Code5,
                                            dregReqDur);
    MacDot16BsScheduleMgmtMsgToSs(node,
                                  dot16,
                                  ssInfo,
                                  ssInfo->basicCid,
                                  dregCmdMsg);

    dot16Bs->stats.numDregCmdSent++;

    //Start T46 only when BS initiated Idle mode
    if (dregReqDur)
    {
        ssInfo->timerT46 = MacDot16SetTimer(node,
                                    dot16,
                                    DOT16e_TIMER_T46,
                                    dot16Bs->para.t46Interval,
                                    NULL,
                                    ssInfo->basicCid);
        ssInfo->timerMgmtRsrcHldg= MacDot16SetTimer(node,
                                        dot16,
                                        DOT16e_TIMER_MgmtRsrcHldg,
                                        dot16Bs->para.tMgmtRsrcHldgInterval,
                                        NULL,
                                        ssInfo->basicCid);
    }
}

// /**
// FUNCTION   :: MacDot16BsHandleT7T14Timeout
// LAYER      :: MAC
// PURPOSE    :: BS handle T7/T14 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16BsHandleT7Timeout(Node * node,
                               MacDataDot16* dot16,
                               MacDot16Timer* timerInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16CIDType transportCid;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;
    MacDot16ServieFlowXactType xactType;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T7 timeout\n");
    }

    transportCid = (Dot16CIDType)timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;
    MacDot16BsGetServiceFlowByCid(node,
                                  dot16,
                                  transportCid,
                                  &ssInfo,
                                  &sFlow);

    if (ssInfo == NULL || sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no ssInfo/sflow associate with transport Cid");
        }

        return;
    }

    if (xactType == DOT16_FLOW_XACT_Add)
    {
        sFlow->dsaInfo.timerT7 = NULL;
    }
    else if (xactType == DOT16_FLOW_XACT_Delete)
    {
        sFlow->dsdInfo.timerT7 = NULL;
    }
    else
    {
        sFlow->dscInfo.timerT7 = NULL;
    }

    if (sFlow != NULL &&
        xactType == DOT16_FLOW_XACT_Add &&
        sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_LOCAL_DsaRspPending)
    {
        if (sFlow->dsaInfo.dsxRetry > 0)
        {
            //schedule the transmission of DSA-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(
                    node,
                    sFlow->dsaInfo.dsxReqCopy));

            sFlow->dsaInfo.dsxRetry --;

            // increase stats
            dot16Bs->stats.numDsaReqSent ++;

            //reset T7
            sFlow->dsaInfo.timerT7 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T7,
                    dot16Bs->para.t8Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Add);

            // stay in the current status

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 Timeout resend DSA-req "
                       "transportCid %d, reset T7\n",
                       transportCid);
            }
        }
        else
        {
            // start Timer t10 and move status to retry-exhausted
            if (sFlow->dsaInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(
                        node,
                        sFlow->dsaInfo.timerT10);
                sFlow->dsaInfo.timerT10 = NULL;
            }

            sFlow->dsaInfo.timerT10 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T10,
                    dot16Bs->para.t10Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Add);

            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_LOCAL_RetryExhausted;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 Timeout opp DSA-req "
                       "transportCid %d is exhausted\n",
                       transportCid);
            }
        }
    } // add
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Delete &&
             sFlow->dsdInfo.dsxTransStatus ==
             DOT16_FLOW_DSD_LOCAL_DsdRspPending)
    {
        // determine retry availability
        if (sFlow->dsdInfo.dsxRetry > 0)
        {// still available to retry

            //schedule the transmission of DSD-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(
                    node,
                    sFlow->dsdInfo.dsxReqCopy));

            //increase stats
            dot16Bs->stats.numDsdReqSent ++;

            //decrease retry
            sFlow->dsdInfo.dsxRetry --;

            // reset T7
            sFlow->dsdInfo.timerT7 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T7,
                    dot16Bs->para.t7Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Delete);

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 Timeout "
                        "for DSD-REQ transportCId %d transId %d\n",
                        transportCid, sFlow->dsdInfo.dsxTransId);
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
                printf("T7 imeout opp DSD-req "
                       "tranCId %d transId %d is exhausted\n",
                       transportCid,
                       sFlow->dsdInfo.dsxTransId);
            }
        } // retry not available
    } // delete
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             sFlow->dscInfo.dsxTransStatus ==
             DOT16_FLOW_DSC_LOCAL_DscRspPending)
    {
        if (sFlow->dscInfo.dsxRetry > 0)
        {
            // still available to retry

            //schedule the transmission of DSC-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(
                    node,
                    sFlow->dscInfo.dsxReqCopy));

            //increase stats
            dot16Bs->stats.numDscReqSent ++;

            //decrease retry
            sFlow->dscInfo.dsxRetry --;

            // reset T7
            sFlow->dscInfo.timerT7 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T7,
                    dot16Bs->para.t7Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Change);

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 Timeout for DSC-REQ "
                       "transportCId %d transId %d\n",
                        transportCid,
                        sFlow->dscInfo.dsxTransId);
            }
        }
        else
        {
            // start Timer t10 and move status to retry-exhausted
            if (sFlow->dscInfo.timerT10 != NULL)
            {
                MESSAGE_CancelSelfMsg(
                        node,
                        sFlow->dscInfo.timerT10);
                sFlow->dscInfo.timerT10 = NULL;
            }
            sFlow->dscInfo.timerT10 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T10,
                    dot16Bs->para.t10Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Change);

            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_LOCAL_RetryExhausted;

            if (DEBUG_FLOW)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("T7 Timeout opp DSC-req"
                       "transportCid %d is exhausted\n",
                       transportCid);
            }
        }
    } // change
}

// /**
// FUNCTION   :: MacDot16BsHandleT8Timeout
// LAYER      :: MAC
// PURPOSE    :: BS handle T8 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16BsHandleT8Timeout(Node * node,
                               MacDataDot16* dot16,
                               MacDot16Timer* timerInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16CIDType transportCid;
    MacDot16ServieFlowXactType xactType;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T8 timeout for transportCid %d\n",
                timerInfo->info);
    }

    transportCid = (Dot16CIDType)timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;
    MacDot16BsGetServiceFlowByCid(node,
                                  dot16,
                                  transportCid,
                                  &ssInfo,
                                  &sFlow);

    if (ssInfo == NULL ||sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning(
                "no ssInfo/sflow associate with the T8's transport Cid");
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
            //schedule the transmission of DSA-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(
                    node,
                    sFlow->dsaInfo.dsxRspCopy));

            // stay in the current status
        }
        else
        {

            sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_REMOTE_End;

            // reset the dsaInfo
            MacDot16ResetDsxInfo(
                node,
                dot16,
                sFlow,
                &(sFlow->dsaInfo));
            sFlow->numXact --;

            // now service flow is not operational
            if (sFlow->numXact == 0)
            {
                sFlow->status = DOT16_FLOW_NULL;
            }
        }
    } // dsa & AckPending (remote)
    else if (sFlow != NULL &&
            xactType == DOT16_FLOW_XACT_Add &&
            sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_REMOTE_HoldingDown)
    {
        sFlow->dsaInfo.dsxTransStatus =
                DOT16_FLOW_DSA_REMOTE_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));
        sFlow->numXact --;

        // now service flow is operational
        sFlow->status = DOT16_FLOW_Nominal;
    } // dsa & holdingdown(remote)
    else if (sFlow != NULL &&
            xactType == DOT16_FLOW_XACT_Add)
    {
        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));
        sFlow->numXact --;

        // now service flow is not operational
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }
    } // dsa other status
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             sFlow->dscInfo.dsxTransStatus ==
             DOT16_FLOW_DSC_REMOTE_DscAckPending)
    {
        if (sFlow->dscInfo.dsxRetry > 0)
        {
            //schedule the transmission of DSA-RSP
            MacDot16BsScheduleMgmtMsgToSs(
                node,
                dot16,
                ssInfo,
                ssInfo->primaryCid,
                MESSAGE_Duplicate(
                    node,
                    sFlow->dscInfo.dsxRspCopy));

            //increase stats
            dot16Bs->stats.numDscRspSent ++;

            sFlow->dscInfo.dsxRetry --;
            sFlow->dscInfo.timerT8 =
                MacDot16SetTimer(
                    node,
                    dot16,
                    DOT16_TIMER_T8,
                    dot16Bs->para.t8Interval,
                    NULL,
                    transportCid,
                    DOT16_FLOW_XACT_Change);

            // stay in the current status
        }
        else
        {

            sFlow->dscInfo.dsxTransStatus =
                DOT16_FLOW_DSC_REMOTE_End;

            // reset the dsaInfo
            MacDot16ResetDsxInfo(
                node,
                dot16,
                sFlow,
                &(sFlow->dscInfo));
            sFlow->numXact --;

            // move back to Nominal
            sFlow->status = DOT16_FLOW_Nominal;
        }
    } // change & DSC ACK pending
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             sFlow->dscInfo.dsxTransStatus ==
                DOT16_FLOW_DSC_REMOTE_HoldingDown)
    {
        sFlow->dscInfo.dsxTransStatus =
            DOT16_FLOW_DSC_REMOTE_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dscInfo));
        sFlow->numXact --;

        // move back to Nominal
        sFlow->status = DOT16_FLOW_Nominal;
    }

}

// /**
// FUNCTION   :: MacDot16BsHandleT10Timeout
// LAYER      :: MAC
// PURPOSE    :: BS handle T10 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + timerInfo : MacDot16Timer : timer information
// RETURN     :: void : NULL
// **/
static
void MacDot16BsHandleT10Timeout(Node * node,
                                MacDataDot16* dot16,
                                MacDot16Timer* timerInfo)
{
    Dot16CIDType transportCid;
    MacDot16ServieFlowXactType xactType;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;

    if (DEBUG_FLOW)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T10 timeout\n");
    }

    transportCid = (Dot16CIDType)timerInfo->info;
    xactType = (MacDot16ServieFlowXactType)timerInfo->Info2;
    MacDot16BsGetServiceFlowByCid(node,
                                  dot16,
                                  transportCid,
                                  &ssInfo,
                                  &sFlow);

    if (ssInfo == NULL || sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning(
                "no ssInfo/sflow associates with the T10's transport Cid");
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
        sFlow->dsaInfo.dsxTransStatus ==
            DOT16_FLOW_DSA_REMOTE_DeleteFlow)
    {

        // move status to end
        sFlow->dsaInfo.dsxTransStatus =
            DOT16_FLOW_DSA_REMOTE_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dsaInfo));

        sFlow->numXact --;

        // now service flow is not operational
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }
    } // dsa & deleteflow(remote)
    else if (sFlow != NULL &&
            xactType == DOT16_FLOW_XACT_Add &&
            (sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_LOCAL_RetryExhausted ||
                sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_LOCAL_DeleteFlow) &&
            ssInfo->isIdle == FALSE)
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
        sFlow->numXact --;

        // now service flow is not operational
        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }
    } // dsa & retryexhausted/deleteflow(LOCAL)
    else if (sFlow != NULL &&
            xactType == DOT16_FLOW_XACT_Add &&
            sFlow->dsaInfo.dsxTransStatus ==
                DOT16_FLOW_DSA_LOCAL_HoldingDown)
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
        sFlow->numXact --;

        // now service flow is operational
        sFlow->status = DOT16_FLOW_Nominal;
    } // dsa & Holdingdown(Local)
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
        sFlow->status = DOT16_FLOW_Deleted;

        // do not need to reset because delete will do it
        sFlow->numXact --;

        if (sFlow->numXact == 0)
        {
            sFlow->status = DOT16_FLOW_NULL;
        }

        // remove Sflow from the list
        MacDot16BsDeleteServiceFlowByTransId(
            node,
            dot16,
            ssInfo,
            sFlow->dsdInfo.dsxTransId);

        // Free transportCid
        MacDot16BsFreeTransportCid(node, dot16, transportCid);

        if (DEBUG_FLOW)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("DSD T10 timeout, free the flow"
                   "from list, free transport Cid"
                   " then DSD complete \n");
        }
    } // dsd local/remote holding down
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
        sFlow->numXact --;

        // move back to Nominal
        sFlow->status = DOT16_FLOW_Nominal;
    } // change remote deleteFLow
    else if (sFlow != NULL &&
             xactType == DOT16_FLOW_XACT_Change &&
             (sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_LOCAL_HoldingDown ||
              sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_LOCAL_RetryExhausted ||
              sFlow->dscInfo.dsxTransStatus ==
                 DOT16_FLOW_DSC_LOCAL_DeleteFlow))
    {

        // Move the status
        sFlow->dscInfo.dsxTransStatus =
            DOT16_FLOW_DSC_LOCAL_End;

        // reset the dsaInfo
        MacDot16ResetDsxInfo(
            node,
            dot16,
            sFlow,
            &(sFlow->dscInfo));

        sFlow->numXact --;

        // move back to nomical
        sFlow->status = DOT16_FLOW_Nominal;

        // Call any pending QOS change saved
        if (sFlow->dscInfo.dscPendingQosInfo)
        {
            MacDot16QoSParameter pendingQosInfo;
            memcpy(&pendingQosInfo, sFlow->dscInfo.dscPendingQosInfo, 
                   sizeof(MacDot16QoSParameter));
            MEM_free(sFlow->dscInfo.dscPendingQosInfo);
            sFlow->dscInfo.dscPendingQosInfo = NULL;
            MacDot16BsChangeServiceFlow(node,
                                        dot16,
                                        ssInfo,
                                        sFlow,
                                        &pendingQosInfo);
        }
    } // change local holding down,retry exhausted ,delete flow
}

#if 0
// /**
// FUNCTION   :: MacDot16SsHandleT46Timeout
// LAYER      :: MAC
// PURPOSE    :: SS handle T46 timeout
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eBsHandleT46Timeout(Node * node,
                                 MacDataDot16* dot16,
                                 MacDot16Timer* timerInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16CIDType transportCid;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;

    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("T46 timeout\n");
    }

    transportCid = (Dot16CIDType)timerInfo->info;
    MacDot16BsGetServiceFlowByCid(node,
                                  dot16,
                                  transportCid,
                                  &ssInfo,
                                  &sFlow);

    if (ssInfo == NULL || sFlow == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no ssInfo/sflow associates with the T46's "
                                "transport Cid");
        }
        return;
    }
    ssInfo->dregCmdRetCount++;

    if (ssInfo->dregCmdRetCount < DOT16e_BS_DREG_REQ_RETRY_COUNT)
    {
        if (ssInfo->timerMgmtRsrcHldg != NULL)
        {
            MESSAGE_CancelSelfMsg(node, ssInfo->timerMgmtRsrcHldg);
            ssInfo->timerMgmtRsrcHldg = NULL;
        }
        ssInfo->timerMgmtRsrcHldg= MacDot16SetTimer(node,
                                    dot16,
                                    DOT16e_TIMER_MgmtRsrcHldg,
                                    dot16Bs->para.tMgmtRsrcHldgInterval,
                                    NULL,
                                    ssInfo->basicCid);
        MacDot16eBsInitiateIdleMode(node, dot16, ssInfo);
    }
    else
    {
        //do nothing
    }
}
#endif

// /**
// FUNCTION   :: MacDot16eBsHandleMgmtRscHldgTimeout
// LAYER      :: MAC
// PURPOSE    :: SS handle Mgmt Resource Holding timer
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// RETURN     :: void : NULL
// **/
static
void MacDot16eBsHandleMgmtRscHldgTimeout(Node * node,
                                         MacDataDot16* dot16,
                                         MacDot16Timer* timerInfo)
{
    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    Dot16CIDType cid;
    MacDot16BsSsInfo* ssInfo = NULL;

    cid = (Dot16CIDType)timerInfo->info;
    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

    if (ssInfo == NULL)
    {
        if (PRINT_WARNING)
        {
            ERROR_ReportWarning("no ssInfo associated with the Mgmt "
                    "Resource Holding Timer's transport Cid");
        }
        return;
    }
    ssInfo->isIdle = TRUE;
    ssInfo->timerMgmtRsrcHldg = NULL;
    if (DEBUG_IDLE)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Mgmt Resource Holding timeout."
               "Activating Idle Mode for interface: %d:%d:%d:%d:%d:%d\n",
                ssInfo->macAddress[0],
                ssInfo->macAddress[1],
                ssInfo->macAddress[2],
                ssInfo->macAddress[3],
                ssInfo->macAddress[4],
                ssInfo->macAddress[5]);
    }
}

//------------------------------------------------------------------------
//  Key API functions
//------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16BsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 BS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16BsInit(Node* node,
                    int interfaceIndex,
                    const NodeInput* nodeInput)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    MacDot16Bs* dot16Bs;
    int phyNumber;
    int index;
    int i;

    if (DEBUG)
    {
        printf("Node%u BS is initializing 802.16 MAC on interface %d\n",
               node->nodeId, interfaceIndex);
    }

    dot16Bs = (MacDot16Bs*) MEM_malloc(sizeof(MacDot16Bs));
    ERROR_Assert(dot16Bs != NULL,
                 "802.16: Out of memory!");

    // using memset to initialize the whole DOT16 data strucutre
    memset((char*)dot16Bs, 0, sizeof(MacDot16Bs));

    dot16->bsData = (void *) dot16Bs;

    // init basic properties
    dot16Bs->transId = DOT16_BS_TRANSACTION_ID_START;
    dot16Bs->lastBasicCidAssigned = DOT16_BASIC_CID_START - 1;
    dot16Bs->lastTransCidAssigned = DOT16_TRANSPORT_CID_START - 1;

    dot16Bs->dcdChanged = TRUE;
    dot16Bs->dcdUpdateInfo.chUpdateState =
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT;
    dot16Bs->dcdUpdateInfo.numChDescSentInTransition = 0;
    dot16Bs->dcdUpdateInfo.timerChDescTransition = NULL;

    dot16Bs->ucdChanged = TRUE;
    dot16Bs->ucdUpdateInfo.chUpdateState =
        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_INIT;
    dot16Bs->ucdUpdateInfo.numChDescSentInTransition = 0;
    dot16Bs->ucdUpdateInfo.timerChDescTransition = NULL;

    // init downlink and uplink burst profiles
    MacDot16BsInitBurstProfiles(node, nodeInput, interfaceIndex, dot16);

    // init system parameters
    MacDot16BsInitParameter(node, nodeInput, interfaceIndex, dot16);

    // init dynamic stats
    MacDot16BsInitDynamicStats(node, interfaceIndex, dot16);

    // start dot16e realted init
    if (dot16->dot16eEnabled)
    {
        dot16Bs->configChangeCount = 0;
        dot16Bs->lastNbrAdvSent = 0;
        dot16Bs->trigger.triggerType = DOT16e_TRIGEGER_METRIC_TYPE_RSSI;
        dot16Bs->trigger.triggerFunc =
                DOT16e_TRIGEGER_METRIC_FUNC_SERV_LESS_ABSO;
        dot16Bs->trigger.triggerAction = DOT16e_TRIGGER_ACTION_MOB_SCN_REQ;
        dot16Bs->trigger.triggerValue =
                (signed char) dot16Bs->para.nbrScanRssTrigger;
        dot16Bs->trigger.triggerAvgDuration =
                (unsigned char) DOT16e_DEFAULT_TRIGGER_AVG_DURATION;

        // read neighbor BS info
        MacDot16eBsInitNbrBsInfo(node, interfaceIndex, nodeInput, dot16);

        dot16Bs->resrcRetainFlag = DOT16e_HO_ResrcRelease;
    }

    // end dot16e related init
    // init my DL and UL channel, for TDD, they are the same one
    // for FDD, assume the first listenable channel is DL and the
    // second one is UL
    phyNumber = dot16->myMacData->phyNumber;
    index = 0;
    for (i = 0; i < dot16->numChannels; i ++)
    {
        if (PHY_IsListeningToChannel(node,
                                     phyNumber,
                                     dot16->channelList[i]))
        {
            index = i;
            break;
        }
    }

    dot16Bs->dlChannel = dot16->channelList[index];

    if (dot16->duplexMode == DOT16_TDD)
    {
        dot16Bs->ulChannel = dot16Bs->dlChannel;
    }
    else
    {
        dot16Bs->ulChannel = dot16->channelList[1];
    }

    // stop listening to any channel at the beginning
    for (i = 0; i < dot16->numChannels; i ++)
    {
        MacDot16StopListening(
            node,
            dot16,
            dot16->channelList[i]);
    }

    // set the DL channel as the transmission channel
    PHY_SetTransmissionChannel(
        node,
        dot16->myMacData->phyNumber,
        dot16Bs->dlChannel);

    if (DEBUG_PARAMETER)
    {
        // print out parameters
        MacDot16BsPrintParameter(node, dot16);
    }

    if (DEBUG)
    {
        printf("Node%u(BS) 802.16 MAC init on interface %d complete\n",
               node->nodeId, interfaceIndex);
    }

    if (dot16Bs->rngType == DOT16_CDMA
        || dot16Bs->bwReqType == DOT16_BWReq_CDMA)
    {
        dot16Bs->numOfDLBurstProfileInUsed = DOT16_NUM_DL_BURST_PROFILES;
        dot16Bs->numOfULBurstProfileInUsed = DOT16_NUM_UL_BURST_PROFILES;
    }
    else
    {
        dot16Bs->numOfDLBurstProfileInUsed = DOT16_NUM_DL_BURST_PROFILES - 1;
        dot16Bs->numOfULBurstProfileInUsed = DOT16_NUM_UL_BURST_PROFILES - 1;
    }
}


// /**
// FUNCTION   :: MacDot16BsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : int      : Interface index
// + msg              : Message* : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16BsLayer(Node* node, int interfaceIndex, Message* msg)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    if (DEBUG)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("DOT16: interface%d get a message\n", interfaceIndex);
    }

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            MacDot16TimerType timerType;
            MacDot16Timer* timerInfo = NULL;

            // get info from message
            timerInfo = (MacDot16Timer*) MESSAGE_ReturnInfo(msg);
            timerType = timerInfo->timerType;

            if (DEBUG_TIMER)
            {
                printf("DOT16: node%u interface%d get a timer message %d\n",
                       node->nodeId, interfaceIndex, timerType);
            }

            switch (timerType)
            {
                case DOT16_TIMER_OperationStart:
                {
                    MacDot16BsStartOperation(node, dot16);
                    if (dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
                    {
                        //send paging group id to PC
                        MacDot16eBsSendPagInfo(node, dot16);
                    }
                    MESSAGE_Free(node, msg);
                    break;
                }

                case DOT16_TIMER_FrameBegin:
                {
                    if (DEBUG_TIMER)
                    {
                        printf("      FrameBegin timer\n");
                    }

                    int i;

                    MacDot16BsSsInfo* ssInfo = NULL;

                    // Timeout expired service flows here
                    MacDot16BsTimeoutServiceFlow(node, dot16);


                    // 1. check the init poll opp is used
                    //    This is to implement P181 Fig 63 when RNG-REQ
                    //    not rcvd in the allocated slot
                    // 2. check if DL measurement report request
                    //    need to be sent
                    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
                    {
                        ssInfo = dot16Bs->ssHash[i];

                        // check all SSs with the same hash ID
                        while (ssInfo != NULL)
                        {
                            BOOL ssNeedRemoved = FALSE;

                            if (ssInfo->needUcastPollNumFrame > 0)
                            {
                                ssInfo->needUcastPollNumFrame --;
                                if (ssInfo->needUcastPollNumFrame == 0)
                                {
                                    ssInfo->needUcastPoll = TRUE;
                                }
                            }

                            if (ssInfo->initRangePolled == TRUE)
                            {

                                // since ssInfo->initRangePolled is  set
                                // in the last frame now it is obviously no
                                // RNG-REQ is received in the allocated slot
                                if (DEBUG_INIT_RANGE)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("no init rng req is rcvd in"
                                           "allocated slot from cid %d\n",
                                           ssInfo->basicCid);
                                }
                                if (MacDot16BsInvitedBurstHasNoData(
                                                    node,
                                                    dot16,
                                                    ssInfo))
                                {
                                    ssNeedRemoved = TRUE;
                                }
                            }

                            if (ssNeedRemoved)
                            {
                                MacDot16BsSsInfo* curSsInfo;

                                curSsInfo = ssInfo;

                                // move to next SS
                                ssInfo = ssInfo->next;

                                // remove the cureent SS from management
                                MacDot16BsRemoveSsByCid(node,
                                                        dot16,
                                                        curSsInfo->basicCid,
                                                        TRUE);


                                continue;
                            }

                            if (ssInfo->ucastTxOppGranted == TRUE &&
                                ssInfo->ucastTxOppUsed == FALSE &&
                                ssInfo->isIdle == FALSE)
                            {
                                // allocated opps are not used by the SS
                                // increase the invite count
                                if (DEBUG_PERIODIC_RANGE)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("no signal is rcvd "
                                           "in allocated slot from cid %d"
                                           "\n", ssInfo->basicCid);
                                }

                                if (MacDot16BsInvitedBurstHasNoData(node,
                                                                    dot16,
                                                                    ssInfo))
                                {
                                    ssNeedRemoved = TRUE;
                                }

                            }

                            if (ssNeedRemoved)
                            {
                                MacDot16BsSsInfo* curSsInfo;

                                curSsInfo = ssInfo;

                                // move to next SS
                                ssInfo = ssInfo->next;

                                // remove the cureent SS from management
                                MacDot16BsRemoveSsByCid(node,
                                                        dot16,
                                                        curSsInfo->basicCid,
                                                        TRUE);
                                continue;
                            }

                            if (ssInfo->isRegistered &&
                                ssInfo->repReqSent &&
                                (ssInfo->lastDlMeaReportTime +
                                2 * dot16Bs->para.dlMeasReportInterval <
                                node->getNodeTime()))
                            {
                                // if REP-REQ is not responsed in certain
                                // period reset the state to enable the
                                // retransmission
                                ssInfo->repReqSent = FALSE;
                            }

                            // REP-REQ if needed
                            if (ssInfo->isRegistered &&
                                (ssInfo->lastDlMeaReportTime +
                                dot16Bs->para.dlMeasReportInterval <
                                node->getNodeTime()) &&
                                !ssInfo->repReqSent)
                            {
                                Message* repReqMsg;

                                repReqMsg = MacDot16BsBuildRepReqPdu(node,
                                                         dot16,
                                                         ssInfo);

                                MacDot16BsScheduleMgmtMsgToSs(
                                                node,
                                                dot16,
                                                ssInfo,
                                                ssInfo->basicCid,
                                                repReqMsg);

                                ssInfo->repReqSent = TRUE;

                                //update stat
                                dot16Bs->stats.numRepReqSent ++;

                                if (DEBUG)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("send a REP-REQ to SS w/CId %d"
                                           "\n", ssInfo->basicCid);
                                }
                            }
                            ssInfo = ssInfo->next;
                        }
                    }


                    // dot16e, send NBR-ADV if needed
                    if (dot16->dot16eEnabled &&
                        dot16Bs->numNbrBs > 0 &&
                        (dot16Bs->lastNbrAdvSent +
                         DOT16_BS_MOB_ADV_ADV_INTERVAL) < node->getNodeTime())
                    {
                        // put a nbr in the broadcast queue
                        Message *mobNbrAdv;

                        mobNbrAdv =
                            MacDot16eBsBuildMobNbrAdvPdu(node, dot16);
                        MacDot16BsScheduleMgmtMsgToSs(node,
                                                      dot16,
                                                      NULL,
                                                      DOT16_BROADCAST_CID,
                                                      mobNbrAdv);
                        dot16Bs->lastNbrAdvSent = node->getNodeTime();

                        // update stats
                        dot16Bs->stats.numNbrAdvSent ++;

                        if (DEBUG_HO && 0) // disable it
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("schedule a NBR-ADV msg to SSs\n");
                        }
                    }

                    // dot16e, check the SS in scanning
                    if (dot16->dot16eEnabled)
                    {
                        // update the BS init HO status
                        for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
                        {
                            ssInfo = dot16Bs->ssHash[i];

                            // check all SSs with the same hash ID
                            while (ssInfo != NULL)
                            {
                                if (ssInfo->bsInitHoStart &&
                                    ssInfo->lastBsHoReqSent +
                                    DOT16e_BS_INIT_HO_RSP_WAIT_TIME <
                                    node->getNodeTime())
                                {
                                    // BS init HO REQ is not responsed by SS
                                    ssInfo->bsInitHoStart = FALSE;
                                }
                                ssInfo = ssInfo->next;
                            }
                        }

                        MacDot16eBsUpdateSsScanInfo(node, dot16);
                    }
                    MacDot16BsStartFrame(node, dot16);

                    // set timer for next frame
                    MacDot16SetTimer(node,
                                     dot16,
                                     DOT16_TIMER_FrameBegin,
                                     dot16Bs->para.frameDuration,
                                     NULL,
                                     0);
                    MESSAGE_Free(node, msg);

                    break;
                }

                case DOT16_TIMER_T7:
                {
                    MacDot16BsHandleT7Timeout(node, dot16, timerInfo);
                    MESSAGE_Free(node, msg);

                    break;
                }

                case DOT16_TIMER_T8:
                {
                    MacDot16BsHandleT8Timeout(node, dot16, timerInfo);
                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_T9:
                {
                    if (DEBUG_NETWORK_ENTRY || DEBUG)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T9 timeout, relaese the basic cid %d"
                               "primary cid assigned tothis SS\n",
                               timerInfo->info);
                    }
                    MacDot16BsSsInfo* ssInfo;

                    ssInfo = MacDot16BsGetSsByCid(node,
                                                  dot16,
                        (Dot16CIDType)timerInfo->info);
                    ssInfo->timerT9 = NULL;

                    // release and age out basic and primary CIDs
                    MacDot16BsRemoveSsByCid(
                        node,
                        dot16,
                        (Dot16CIDType)timerInfo->info, // basic  CID
                        TRUE);

                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_T10:
                {
                    MacDot16BsHandleT10Timeout(node, dot16, timerInfo);
                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_RgnRspProc:
                {
                    MacDot16BsSsInfo* ssInfo;
                    ssInfo = MacDot16BsGetSsByCid(node,
                                                  dot16,
                        (Dot16CIDType)timerInfo->info);

                    ssInfo->timerRngRspProc = NULL;

                    // schedule the inital polling ranging
                    ssInfo->needInitRangeGrant = TRUE;

                    if (DEBUG_NETWORK_ENTRY || DEBUG_INIT_RANGE)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("RngRspProc timeout, "
                               "scheudle a invited rng opp to SS\n");
                    }

                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_T17:
                {
                    if (DEBUG_NETWORK_ENTRY)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("T17 timeout, remove SS with cid %d\n",
                               timerInfo->info);
                    }

                    MacDot16BsSsInfo* ssInfo;

                    ssInfo = MacDot16BsGetSsByCid(node,
                                                  dot16,
                        (Dot16CIDType)timerInfo->info);
                    ssInfo->timerT17 = NULL;

                    // deallocate basic and primary CID
                    MacDot16BsRemoveSsByCid(
                        node,
                        dot16,
                        (Dot16CIDType)timerInfo->info, // cid
                        TRUE);

                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_T27:
                {
                    MacDot16BsSsInfo* ssInfo;

                    ssInfo = MacDot16BsGetSsByCid(node,
                                                  dot16,
                        (Dot16CIDType)timerInfo->info);

                    if (ssInfo != NULL)
                    {
                        ssInfo->timerT27 = NULL;

                        // schedule the periodic polling ranging
                        ssInfo->needPeriodicRangeGrant = TRUE;

                        if (DEBUG_PERIODIC_RANGE)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("T27 timeout, It is time for BS to issue"
                                   "a periodic raning opp to SS using basic"
                                   " CID %d\n",
                                    ssInfo->basicCid);
                        }
                    }

                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_DcdTransition:
                {
                    dot16Bs->dcdUpdateInfo.timerChDescTransition = NULL;

                    // state must be COUNTDOWN
                    dot16Bs->dcdUpdateInfo.chUpdateState =
                        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END;

                    if (DEBUG)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DCD transtion timeout, now it is ready to "
                               "broadcast new generation DL-MAP\n");
                    }

                    MESSAGE_Free(node, msg);

                    break;
                }
                case DOT16_TIMER_UcdTransition:
                {
                    dot16Bs->ucdUpdateInfo.timerChDescTransition = NULL;

                    // state must be COUNTDOWN
                    dot16Bs->ucdUpdateInfo.chUpdateState =
                        DOT16_BS_CHANNEL_DESCRIPTOR_UPDATE_END;

                    if (DEBUG)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("UCD transtion timeout, now it is ready "
                               "to broadcast new generation UL-MAP\n");
                    }
                    MESSAGE_Free(node, msg);
                    break;
                }

                case DOT16e_TIMER_ResrcRetain:
                {
                    MacDot16BsSsInfo* ssInfo;

                    ssInfo = MacDot16BsGetSsByCid(node,
                                                  dot16,
                        (Dot16CIDType)timerInfo->info);

                    if (ssInfo != NULL && ssInfo->inHandover)
                    {
                        ssInfo->resrcRetainTimer = NULL;
                        ssInfo->inHandover = FALSE;

                        // remove the SS from management
                        if (DEBUG_HO)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("remove SS with cid %d from mgmt "
                                   "after resrc retain timeout\n",
                                    ssInfo->basicCid);
                        }

                        MacDot16BsRemoveSsByCid(
                            node,
                            dot16,
                            (Dot16CIDType)timerInfo->info,
                            TRUE);
                    }
                    MESSAGE_Free(node, msg);
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
                    MESSAGE_Free(node, msg);
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
                    MESSAGE_Free(node, msg);
                    break;
                }
                case DOT16_ARQ_SYNC_LOSS_TIMER:
                {
                    MacDot16BsSsInfo* ssInfo  = NULL;
                    MacDot16ServiceFlow* sFlow = NULL;

                    if (DEBUG_ARQ_TIMER)
                    {
                        MacDot16PrintRunTimeInfo(node, dot16);
                        printf("DOT16_ARQ_SYNC_LOSS_TIMER expired. cid = %d\n",
                        (Dot16CIDType)timerInfo->info);
                    }
                    MacDot16BsGetServiceFlowByCid(node,
                        dot16,
                        (Dot16CIDType)timerInfo->info,
                        &ssInfo,
                        &sFlow);
                    if (sFlow == NULL || ssInfo == NULL)
                    {
                        if (PRINT_WARNING)
                        {
                            ERROR_ReportWarning("No service flow / SS info"
                                " associated with the timer expired \n");
                        }

                        //bug fix start #5373
                        MESSAGE_Free(node, msg);
                        //bug fix end #5373
                        return;
                    }
                    sFlow->arqSyncLossTimer = NULL;
                    // to initiate reset message
                    MacDot16ARQBuildandSendResetMsg(node,
                        dot16,
                        (Dot16CIDType)timerInfo->info,
                        (UInt8)DOT16_ARQ_RESET_Initiator);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case DOT16e_TIMER_IdleModeSystem:
                {
                    // Backbone timer
                    Dot16eBackboneHandleIdleModeSystemTimeout(node,
                                                            dot16, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case DOT16e_TIMER_MgmtRsrcHldg:
                {
                    MacDot16eBsHandleMgmtRscHldgTimeout(node, dot16,
                                                        timerInfo);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default:
                {
                    char tmpString[MAX_STRING_LENGTH];
                    sprintf(tmpString,
                            "DOT16 BS node%d: Unknow timer type %d\n",
                            node->nodeId, timerType);
                    ERROR_ReportError(tmpString);
                    MESSAGE_Free(node, msg);
                    break;
                }
            }
            break;
        }
        default:
        {
            char tmpString[MAX_STRING_LENGTH];
            sprintf(tmpString,
                    "DOT16 BS node%d: Unknow message event type %d\n",
                    node->nodeId, msg->eventType);
            ERROR_ReportError(tmpString);
            MESSAGE_Free(node, msg);
            break;
        }
    }
}

// /**
// FUNCTION   :: MacDot16BsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16BsFinalize(Node *node, int interfaceIndex)
{
    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    MacDot16BsPrintStats(node, dot16, interfaceIndex);
}

// /**
// FUNCTION   :: MacDot16BsReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16BsReceivePacketFromPhy(Node* node,
                                    MacDataDot16* dot16,
                                    Message* msg)
{
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    unsigned char msgType;
    unsigned char* payload = NULL;
    Message* tmpMsg = NULL;
    Message* nextMsg = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    PhySignalMeasurement* signalMea = NULL;

    if (DEBUG_PACKET)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("recvd a pkt at size = %d\n",
               MESSAGE_ReturnPacketSize(msg));
    }

    // check the signal measurement information
    if (MESSAGE_ReturnInfo(msg) != NULL)
    {
        signalMea = (PhySignalMeasurement*) MESSAGE_ReturnInfo(msg);

        if (signalMea->fecCodeModuType == PHY802_16_BPSK)
        {
            // Handle CDMA range or bandwidth request packet
            MacDot16RngCdmaInfo* cdmaInfo;
            tmpMsg = msg;
            msg = msg->next;
            tmpMsg->next = NULL;

            cdmaInfo = (MacDot16RngCdmaInfo*)MESSAGE_ReturnPacket(tmpMsg);
            if (DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS received Ranging code\n");
            }
            MacDot16BsHandleCDMARangRequest(node, dot16, tmpMsg, cdmaInfo);

            MESSAGE_Free(node, tmpMsg);
        }
        else
        {
            macHeader = (MacDot16MacHeader*) MESSAGE_ReturnPacket(msg);
            cid = MacDot16MacHeaderGetCID(macHeader);

            ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);

            // update signal measurements
            if (ssInfo != NULL && ssInfo->initRangeCompleted)
            {
                unsigned char leastRobustUiuc;

                // update the ul signal measurement
                MacDot16BsUpdateSsMeasurement(node,
                                              dot16,
                                              ssInfo,
                                              signalMea);

                // check if receive msg while SS in scan mode
                if (ssInfo->inNbrScan)
                {
                    // reset the scan state and ready to handle the msg
                    ssInfo->inNbrScan = FALSE;
                    ssInfo->scanDuration = 0;
                    ssInfo->scanInterval = 0;
                    ssInfo->scanIteration = 0;
                    ssInfo->numInterFramesLeft = 0;
                    ssInfo->numScanFramesLeft = 0;
                    ssInfo->numScanIterationsLeft = 0;
                }

                // check if handover is needed
                if (ssInfo->isRegistered)
                {
                    MacDot16BsCheckHandover(node, dot16, ssInfo);
                }

                // get the operational burst profile index
                if (MacDot16GetLeastRobustBurst(node,
                                                dot16,
                                                DOT16_UL,
                                                ssInfo->ulCinrMean,
                                                &leastRobustUiuc))
                {
                    if (ssInfo->uiuc != leastRobustUiuc)
                    {
                        if (DEBUG || DEBUG_NETWORK_ENTRY)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("RCVD UL signal change the SS with cid %d "
                                   "UIUC from %d to %d\n",
                                   ssInfo->basicCid,
                                   ssInfo->uiuc,
                                   leastRobustUiuc);

                        }
                        ssInfo->uiuc = leastRobustUiuc;
                    }
                }
                else
                {
                    // assume UIUC 1 always works
                    ssInfo->uiuc = 1;
                }
            }

            // for periodic ranging recv proc
            if (ssInfo != NULL && ssInfo->initRangeCompleted &&
                ssInfo->ucastTxOppGranted && ssInfo->ucastTxOppUsed == FALSE)
            {
                BOOL ssNeedRemoved = FALSE;
                if (MacDot16MacHeaderGetHT(macHeader) == 0)
                {
                    payload = (unsigned char*) MESSAGE_ReturnPacket(msg);
                    msgType = payload[sizeof(MacDot16MacHeader)];
                    //MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

                    if (msgType != DOT16_RNG_REQ)
                    {
                        MacDot16RngCdmaInfo cdmaInfo;
                        memset(&cdmaInfo, 0, sizeof(MacDot16RngCdmaInfo));
                        ssNeedRemoved =
                            MacDot16BsPerformPeriodicRangeRcvProc(node,
                                                                  dot16,
                                                                  cid,
                                                                  FALSE,
                                                                  signalMea,
                                                                  cdmaInfo);
                    }
                }
                ssInfo->ucastTxOppUsed = TRUE;

                if (ssNeedRemoved)
                {
                    // remove the ss from management
                    MacDot16BsRemoveSsByCid(node,
                                            dot16,
                                            ssInfo->basicCid,
                                            TRUE);
                    // free the msg
                    MESSAGE_Free(node, msg);

                    return;
                }
            }
        }
    }
    tmpMsg = msg;
    while (tmpMsg != NULL)
    {
        signalMea = (PhySignalMeasurement*) MESSAGE_ReturnInfo(tmpMsg);
        if ((signalMea != NULL) && (signalMea->fecCodeModuType
            == PHY802_16_BPSK))
        {
            // Handle CDMA range or bandwidth request packet
            MacDot16RngCdmaInfo* cdmaInfo;
            tmpMsg = msg->next;
            msg->next = NULL;
            cdmaInfo = (MacDot16RngCdmaInfo*)MESSAGE_ReturnPacket(msg);
            if (DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("BS received Ranging code\n");
            }
            MacDot16BsHandleCDMARangRequest(node, dot16, msg, cdmaInfo);
            MESSAGE_Free(node, msg);
        }
        else
        {
            // get the MAC header of the MAC PDU
            payload = (unsigned char*) MESSAGE_ReturnPacket(tmpMsg);
            macHeader = (MacDot16MacHeader*) payload;

            if (MacDot16MacHeaderGetHT(macHeader) == 0)
            {
                // this is a generic MAC header
                cid = MacDot16MacHeaderGetCID(macHeader);
                if (MacDot16IsManagementCid(node, dot16, cid))
                {
                    // this is a management message
                    msgType = payload[sizeof(MacDot16MacHeader)];

                    switch (msgType)
                    {
                        case DOT16_RNG_REQ:
                        { // this is raning request
                            MacDot16BsHandleRangeRequest(node,
                                                         dot16,
                                                         payload,
                                                         0,
                                                         cid,
                                                         signalMea);

                            break;
                        }

                        case DOT16_SBC_REQ:
                        { // SBC request
                            MacDot16BsHandleSbcRequest(node,
                                                       dot16,
                                                       payload,
                                                       0,
                                                       cid);

                            break;

                        }

                        case DOT16_PKM_REQ:
                        { // public key management request
                            MacDot16BsHandlePkmRequest(node,
                                                       dot16,
                                                       payload,
                                                       0,
                                                       cid);

                            break;
                        }

                        case DOT16_REG_REQ:
                        { // registration request
                            MacDot16BsHandleRegRequest(node,
                                                       dot16,
                                                       payload,
                                                       0,
                                                       cid);

                            break;
                        }

                        case DOT16_REP_RSP:
                        { // report response
                            MacDot16BsHandleRepRsp(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSA_REQ:
                        { // dynamic service add request
                            MacDot16BsHandleDsaRequest(
                                node,
                                dot16,
                                payload,
                                0,
                                cid,
                                (TraceProtocolType)tmpMsg->originatingProtocol);
                            break;
                        }

                        case DOT16_DSA_RSP:
                        { // dynamic service add request
                            MacDot16BsHandleDsaRsp(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);
                            break;
                        }

                        case DOT16_DSA_ACK:
                        {// dynamic service add acknowledge
                            MacDot16BsHandleDsaAck(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSC_REQ:
                        {// dynamic service change request
                            MacDot16BsHandleDscReq(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSC_RSP:
                        {// dynamic service change request
                            MacDot16BsHandleDscRsp(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSC_ACK:
                        {// dynamic service change acknowledgement
                            MacDot16BsHandleDscAck(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSD_REQ:
                        {// dynamic service delete request
                            MacDot16BsHandleDsdReq(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16_DSD_RSP:
                        {// dynamic service delete Response
                            MacDot16BsHandleDsdRsp(node,
                                                   dot16,
                                                   payload,
                                                   0,
                                                   cid);

                            break;
                        }

                        case DOT16e_MOB_SCN_REQ:
                        { // request for neighbor scan
                            MacDot16eBsHandleMobScnReq(node,
                                                       dot16,
                                                       payload,
                                                       0);

                            break;
                        }

                        case DOT16e_MOB_SCN_REP:
                        { // scan result report
                            MacDot16eBsHandleMobScnRep(node,
                                                       dot16,
                                                       payload,
                                                       0);

                            break;
                        }
                        case DOT16e_MOB_HO_IND:
                        { // handover indication
                            MacDot16eBsHandleMobHoInd(node,
                                                      dot16,
                                                      payload,
                                                      0);

                            break;
                        }
                        case DOT16e_MOB_MSHO_REQ:
                        {
                            MacDot16eBsHandleMobMshoReq(node,
                                                        dot16,
                                                        payload,
                                                        0);
                            break;
                        }
                        case DOT16e_MOB_SLP_REQ:
                        {
                            MacDot16eBsHandleMobSlpReq(node,
                                                        dot16,
                                                        payload,
                                                        0);
                            break;
                        }
                        case DOT16_PADDED_DATA:
                        {
                            if (DEBUG_PERIODIC_RANGE)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("node %d: rcvd a padding data from"
                                       "CID %d\n",
                                       node->nodeId, cid);
                            }
                            break;
                        }
                        case DOT16_ARQ_FEEDBACK:
                        {
                            //this a an ARQ feedback message.
                            // this is a MOB_BSHO_REQ message
                             MacDot16ARQHandleFeedback(node,
                                                       dot16,
                                                       payload,
                                                       0);
                            break ;
                        }
                        case DOT16_ARQ_DISCARD:
                        {
                            MacDot16ARQHandleDiscardMsg(node,
                                                        dot16,
                                                        payload,
                                                        0);
                            break ;
                        }
                        case DOT16_ARQ_RESET:
                        {
                            MacDot16ARQHandleResetMsg(node,
                                dot16,
                                payload,
                                0);
                            break;
                        }
                    case DOT16_DREG_REQ:
                    {
                        MacDot16eBsHandleDregReqMsg(node,
                                                    dot16,
                                                    payload,
                                                    0);
                        break;
                    }
                        default:
                        {
                            char buff[MAX_STRING_LENGTH];

                            sprintf(buff,
                                    "MAC802.16: Node%d(BS) gets unknow mgmt "
                                    "msg type %d\n",
                                    node->nodeId, msgType);
                            ERROR_ReportError(buff);

                            break;
                        }
                    }
                    nextMsg = tmpMsg->next;
                    MESSAGE_Free(node, tmpMsg);
                }
                else
                {
                    // this is a data packet
                    nextMsg = MacDot16BsHandleDataPdu(node, dot16, tmpMsg);
                }
            }
            else
            {
                // this is a bandwidth request message
                MacDot16BsHandleBandwidthRequest(node, dot16, payload, 0);
                nextMsg = tmpMsg->next;
                MESSAGE_Free(node, tmpMsg);
            }

            tmpMsg = nextMsg;
        }
    }
}

// /**
// FUNCTION   :: MacDot16BsReceivePhyStatusChangeNotification
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
void MacDot16BsReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration)
{
}
